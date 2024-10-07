#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <openssl/des.h>

// DES key size
#define DES_KEY_SIZE 8

// Function to decrypt using OpenSSL DES in CBC mode
void decrypt_with_key(const uint8_t *ciphertext, uint8_t *plaintext, const DES_cblock *key, const DES_cblock *iv,size_t length) {
    DES_key_schedule schedule;
    DES_set_key(key, &schedule);  // Set up the key schedule

    // DES_cblock iv_copy;
    // memcpy(&iv_copy, iv, sizeof(DES_cblock));  // Reset IV before decryption

    // Decrypt the cipher using DES in CBC mode
    DES_cbc_encrypt(ciphertext, plaintext, length, &schedule, iv, DES_DECRYPT);
}

// Function to encrypt using OpenSSL DES in CBC mode
void encrypt_with_key(const uint8_t *plaintext, uint8_t *ciphertext, const DES_cblock *key, const DES_cblock *iv,size_t length) {
    DES_key_schedule schedule;
    DES_set_key(key, &schedule);  // Set up the key schedule

    // DES_cblock iv_copy;
    // memcpy(&iv_copy, iv, sizeof(DES_cblock));  // Reset IV before encryption

    // Encrypt the plaintext using DES in CBC mode
    DES_cbc_encrypt(plaintext, ciphertext, length, &schedule, iv, DES_ENCRYPT);
}

void print_key(const uint8_t *key, int size) {
    printf("Key: ");
    for (int i = 0; i < size; i++) {
        printf("%02x ", key[i]);
    }
    printf("\n");
}

void long_to_des_key(unsigned long long key, DES_cblock *des_key) {
    // We have to split the key into bytes and copy it into the DES_cblock (8 bytes).
    for (int i = 0; i < 7; ++i) {
        (*des_key)[7 - i] = (unsigned char)((key >> (i * 8)) & 0xFF);  // Big-endian byte order
    }
    (*des_key)[0] = 0x00;  // Ensure that the highest byte is zero for 56-bit key
}

// Try a key and check if it decrypts correctly
int tryKey(long key, const uint8_t *ciphertext, int len, const char *keyword, const DES_cblock *iv, int id) {
    uint8_t *decrypted = malloc(len); 
    memset(decrypted, 0, len);

    DES_cblock des_key;
    // memcpy(&des_key, &key, sizeof(DES_cblock));
    long_to_des_key(key, &des_key);
    // printf("Process %d using key: %lln\n", id, key);
    // print_key(&key, DES_KEY_SIZE);

    decrypt_with_key(ciphertext, decrypted, &des_key, iv,len);

    // Check if the decrypted text matches the correct plaintext
    return strstr(decrypted, keyword) != NULL;
}



// Function to read the entire plaintext from a file
uint8_t* read_plaintext_from_file(const char *filename, size_t *length) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Failed to open file");
        return NULL;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    if (file_size <= 0) {
        printf("Error: File is empty or has invalid size.\n");
        fclose(file);
        return NULL;
    }

    // Allocate memory for the file contents
    uint8_t *buffer = (uint8_t *)malloc(file_size);
    if (buffer == NULL) {
        printf("Error: Failed to allocate memory.\n");
        fclose(file);
        return NULL;
    }

    // Read the entire file into the buffer
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        printf("Error: Failed to read entire file.\n");
        free(buffer);
        fclose(file);
        return NULL;
    }

    *length = file_size;  // Set the length of the plaintext
    fclose(file);
    return buffer;
}



int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <plaintext_file>\n", argv[0]);
        return 1;
    }


    const char *plaintext_file = argv[1];
    size_t plaintext_length;
    uint8_t *plaintext = read_plaintext_from_file(plaintext_file, &plaintext_length);
    if (plaintext == NULL) {
        return 1;
    }

    int N, id;
    unsigned long long upper = (1ULL << 56);  // Upper bound for DES keys (2^56)
    unsigned long long mylower, myupper;
    long found = 0;  // Moved to a higher scope
    double start_time, end_time;  // Moved to a higher scope
    MPI_Status st;
    MPI_Request req;
    int flag = 0;
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);


    uint8_t *ciphertext = (uint8_t *)malloc(plaintext_length);
    DES_cblock iv = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // IV initialization
    
    DES_cblock generated_key = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00};  // The key you specified

    MPI_Bcast(&plaintext_length, 1, MPI_UNSIGNED_LONG, 0, comm);
    
    MPI_Barrier(comm);

    if (id == 0) {
        // Process 0 will perform the encryption
        printf("Plaintext: %s\n", plaintext);
        printf("Encrypting with key:\n");
        print_key((uint8_t *)&generated_key, DES_KEY_SIZE);

        // Encrypt the plaintext using the specified key
        encrypt_with_key(plaintext, ciphertext, &generated_key, &iv,plaintext_length);

        // Print the ciphertext
        printf("Ciphertext: ");
        for (int i = 0; i < plaintext_length; i++) {
            printf("%02x ", ciphertext[i]);
        }
        printf("\n");

        
    }

    MPI_Barrier(comm);

    // Broadcast the ciphertext to all processes
    MPI_Bcast(ciphertext, plaintext_length, MPI_UNSIGNED_CHAR, 0, comm);

    if (id == 0) {
        printf("Process %d (coordinator) is done with encryption and broadcasting.\n", id);
        start_time = MPI_Wtime();
        // Process 0 also needs to receive the found key from any other process
        MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);
        MPI_Wait(&req, &st);

        if (found >= 0) {
            // After receiving the key, process 0 prints the result
            end_time = MPI_Wtime();
            double elapsed_time = end_time - start_time;

            // Decrypt with the found key and print the result
            DES_cblock des_key;
            // memcpy(&des_key, &found, sizeof(DES_cblock));
            long_to_des_key(found, &des_key);

            uint8_t decrypted[plaintext_length];
            decrypt_with_key(ciphertext, decrypted, &des_key, &iv, plaintext_length);
            printf("Found ");
            print_key(des_key, DES_KEY_SIZE);
            printf("Decrypted: %s\n", decrypted);
            printf("Time taken to find the key: %f seconds\n", elapsed_time);
        }
    } else {
        // Only worker processes (1 to N-1) will search the key space
        unsigned long long range_per_node = upper / (N - 1);  // Divide the key space by N-1 workers
        mylower = range_per_node * (id - 1);  // Start range for this worker process
        myupper = (id == N - 1) ? upper : range_per_node * id - 1;  // Last process gets the remaining keys

        printf("Process %d: Searching keys from %llx to %llx\n", id, mylower, myupper);

        start_time = MPI_Wtime();

        MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

        const char keyword[] = "es una prueba de";
        // Brute-force search for the correct key in the range
        for (unsigned long long i = mylower; i <= myupper; ++i) {
            // Periodically check if a key has been found
            MPI_Test(&req, &flag, &st);
            if (flag) {
                // Key found by another process, stop searching
                break;
            }
            if (tryKey(i, ciphertext, plaintext_length, keyword, &iv, id)) {
                found = i;
                printf("Key found by process %d\n", id);
                // Broadcast the found key to all other processes
                for (int node = 0; node < N; node++) {
                    MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
                }
                break;
            }
        }

        // MPI_Test(&req, &flag, &st);
        // if (flag) {
        //     printf("Process %d received notification to stop.\n", id);
        // }
    }
    
    MPI_Barrier(comm); 
    MPI_Finalize();
    return 0;
}
