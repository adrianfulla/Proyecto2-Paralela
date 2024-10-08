#include <stdio.h>
#include <openssl/des.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>


// DES key size
#define DES_KEY_SIZE 8

// Function to measure the time
double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

// Function to print key for debugging
void print_key(const uint8_t *key, int size) {
    printf("Key: ");
    for (int i = 0; i < size; i++) {
        printf("%02x ", key[i]);
    }
    printf("\n");
}

// Function to encrypt the message using a given key
int encrypt_with_key(const uint8_t *plaintext, uint8_t *ciphertext, const DES_cblock *key, const DES_cblock *iv, size_t length) {
    DES_key_schedule schedule;
    DES_set_key(key, &schedule);  // Use DES_set_key for correct key scheduling
    DES_cbc_encrypt(plaintext, ciphertext, length, &schedule, iv, DES_ENCRYPT);  // Encrypt the full file contents
    return 0;
}

// Function to decrypt the message using a given key
int decrypt_with_key(const uint8_t *ciphertext, uint8_t *plaintext, const DES_cblock *key, const DES_cblock *iv, size_t length) {
    DES_key_schedule schedule;
    DES_set_key(key, &schedule);  // Use DES_set_key for correct key scheduling
    DES_cbc_encrypt(ciphertext, plaintext, length, &schedule, iv, DES_DECRYPT);  // Decrypt the full file contents
    return 0;
}

void long_to_des_key(unsigned long long key, DES_cblock *des_key) {
    // We have to split the key into bytes and copy it into the DES_cblock (8 bytes).
    for (int i = 0; i < 7; ++i) {
        (*des_key)[7 - i] = (unsigned char)((key >> (i * 8)) & 0xFF);  // Big-endian byte order
    }
    (*des_key)[0] = 0x00;  // Ensure that the highest byte is zero for 56-bit key
}

// Function to brute-force the DES key
void brute_force_des(const uint8_t *ciphertext, const char *keyword, int key_bits, const DES_cblock *iv, size_t length) {
    uint8_t key[DES_KEY_SIZE] = {0};
    uint8_t *decrypted = malloc(length);  // Decrypted buffer for the entire plaintext
    uint64_t max_key = (1ULL << key_bits);  // Limit based on key size
    uint64_t i;

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Start brute-forcing keys
    for (i = 0; i < max_key; i++) {
       DES_cblock des_key; 
       long_to_des_key(i, &des_key);  // Copy only the relevant part of i to the key buffer
        DES_cblock iv_copy;
        memcpy(&iv_copy, iv, sizeof(DES_cblock));

        decrypt_with_key(ciphertext, decrypted, des_key, &iv_copy, length);
        // Check if the keyword exists in the decrypted text
        if (strstr(decrypted, keyword) != NULL) {
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            double time_taken = get_time_diff(start_time, end_time);
            printf("Key found: %llx\n", (unsigned long long)i);
            print_key(des_key, DES_KEY_SIZE);  // Print the found key
            printf("Decrypted: %s\n", decrypted);  // Print the decrypted text
            printf("Time taken for key size %d bits: %f seconds\n", key_bits, time_taken);
            free(decrypted);
            return;
        }
    }

    // If no key was found
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double time_taken = get_time_diff(start_time, end_time);
    printf("Key not found (key size: %d bits) in %f seconds.\n", key_bits, time_taken);
    free(decrypted);
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
    const char keyword[] = "una prueba del";


    const char *plaintext_file = argv[1];
    size_t plaintext_length;
    uint8_t *plaintext = read_plaintext_from_file(plaintext_file, &plaintext_length);
    if (plaintext == NULL) {
        return 1;
    }

    uint8_t *ciphertext = (uint8_t *)malloc(plaintext_length);  // Buffer for ciphertext
    DES_cblock iv = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // IV initialization
    uint8_t generated_key[DES_KEY_SIZE];
    
    for (int key_bits = 8; key_bits <= 9; key_bits += 8) { 
        printf("Trying key length %d\n", key_bits);

        memset(generated_key, 0, DES_KEY_SIZE);
        for (int i = 0; i < key_bits/8;++i){
            generated_key[DES_KEY_SIZE - 1 - i] = 0xff;
        }
        print_key(generated_key, DES_KEY_SIZE);

        // Encrypt the entire plaintext using the generated key
        encrypt_with_key(plaintext, ciphertext, (DES_cblock *)generated_key, &iv, plaintext_length);
        printf("Plaintext: %s\n", plaintext);
        printf("Ciphertext (hex): ");
        for (int i = 0; i < plaintext_length; i++) {
            printf("%02x ", ciphertext[i]);
        }
        printf("\n");

        // Brute-force DES to recover the key
        brute_force_des(ciphertext, keyword, key_bits, &iv, plaintext_length);
    }

    free(plaintext);  // Free allocated memory
    free(ciphertext);  // Free allocated memory
    return 0;
}
