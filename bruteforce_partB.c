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
    if (argc != 4) {
        printf("Usage: %s <plaintext_file> <keyword> <private_key>\n", argv[0]);
        return 1;
    }

    const char *plaintext_file = argv[1];
    const char *keyword = argv[2];
    unsigned long long private_key = strtoull(argv[3], NULL, 10);


    size_t plaintext_length;
    uint8_t *plaintext = read_plaintext_from_file(plaintext_file, &plaintext_length);
    if (plaintext == NULL) {
        return 1;
    }

    int N, id;
    unsigned long long upper = (1ULL << 56);  
    unsigned long long mylower, myupper;
    long found = 0;  
    double start_time, end_time;  
    MPI_Status st;
    MPI_Request req;
    int flag = 0;
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);


    uint8_t *ciphertext = (uint8_t *)malloc(plaintext_length);
    DES_cblock iv = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // IV initialization
    
    DES_cblock generated_key;
    long_to_des_key(private_key, &generated_key);

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

    unsigned long long range_per_node = upper / N;  // Divide the key space by all N workers
    mylower = range_per_node * id;  // Start range for this process
    myupper = (id == N - 1) ? upper : range_per_node * (id + 1) - 1;  // Last process gets the remaining keys

    printf("Process %d: Searching keys from %llx to %llx\n", id, mylower, myupper);

    start_time = MPI_Wtime();

    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

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

    MPI_Test(&req, &flag, &st);
    if (flag) {
        printf("Process %d received notification to stop.\n", id);
    }

    MPI_Barrier(comm);

    if (id == 0) {
        end_time = MPI_Wtime();
        double elapsed_time = end_time - start_time;

        // Decrypt with the found key and print the result
        DES_cblock des_key;
        long_to_des_key(found, &des_key);

        uint8_t *decrypted = (uint8_t *)malloc(plaintext_length + 1);  // Allocate buffer with null terminator
        if (decrypted == NULL) {
            fprintf(stderr, "Failed to allocate memory for decrypted buffer.\n");
            MPI_Abort(comm, EXIT_FAILURE);
        }

        decrypt_with_key(ciphertext, decrypted, &des_key, &iv, plaintext_length);

        decrypted[plaintext_length] = '\0';

        printf("Key found: %li\nDecrypted: %s\n", found, decrypted);
        printf("Time taken to find the key: %f seconds\n", elapsed_time);

        free(decrypted);
    }


    MPI_Barrier(comm);
    MPI_Finalize();
    return 0;
}
