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
int encrypt_with_key(const uint8_t *plaintext, uint8_t *ciphertext, const DES_cblock *key, const DES_cblock *iv) {
    DES_key_schedule schedule;
    DES_set_key(key, &schedule);  // Use DES_set_key for correct key scheduling
    DES_cbc_encrypt(plaintext, ciphertext, DES_KEY_SIZE, &schedule, iv, DES_ENCRYPT);  // Use DES_cbc_encrypt
    return 0;
}

// Function to decrypt the message using a given key
int decrypt_with_key(const uint8_t *ciphertext, uint8_t *plaintext, const DES_cblock *key, const DES_cblock *iv) {
    DES_key_schedule schedule;
    DES_set_key(key, &schedule);  // Use DES_set_key for correct key scheduling
    DES_cbc_encrypt(ciphertext, plaintext, DES_KEY_SIZE, &schedule, iv, DES_DECRYPT);  // Use DES_cbc_encrypt
    return 0;
}

// Function to brute-force the DES key
void brute_force_des(const uint8_t *ciphertext, const uint8_t *correct_plaintext, int key_bits, const DES_cblock *iv) {
    uint8_t key[DES_KEY_SIZE] = {0};
    uint8_t decrypted[DES_KEY_SIZE];
    uint64_t max_key = (1ULL << key_bits);  // Limit based on key size
    uint64_t i;
    
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Start brute-forcing keys
    for (i = 0; i < max_key; i++) {
        memset(key, 0, DES_KEY_SIZE);  // Clear key array
        memcpy(key, &i, key_bits / 8);  // Copy only the relevant part of i to the key buffer

        DES_cblock iv_copy;
        memcpy(&iv_copy, iv, sizeof(DES_cblock));

        // print_key(key, DES_KEY_SIZE);
        decrypt_with_key(ciphertext, decrypted, (DES_cblock *)key, &iv_copy);

        // printf("Decrypted: ");
        // for (int i = 0; i < DES_KEY_SIZE; i++) {
        //     printf("%02x ", decrypted[i]);
        // }
        // printf("\n");
        
        // Check if decrypted matches the correct plaintext
        if (memcmp(decrypted, correct_plaintext, DES_KEY_SIZE) == 0) {
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            double time_taken = get_time_diff(start_time, end_time);
            printf("Key found: %llx\n", (unsigned long long)i);
            print_key(key, DES_KEY_SIZE);  // Print the found key
            printf("Decrypted: ");
            for (int i = 0; i < DES_KEY_SIZE; i++) {
                printf("%02x ", decrypted[i]);
            }
            printf("\n");
            printf("Time taken for key size %d bits: %f seconds\n", key_bits, time_taken);
            return;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double time_taken = get_time_diff(start_time, end_time);
    printf("Key not found (key size: %d bits) in %f seconds.\n", key_bits, time_taken);
}



int main() {
    const uint8_t plaintext[DES_KEY_SIZE] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x57, 0x72, 0x6C};  // Example: "HelloWrl"
    uint8_t ciphertext[DES_KEY_SIZE];
    DES_cblock iv = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // IV initialization
    uint8_t generated_key[DES_KEY_SIZE];
    
    for (int key_bits = 8; key_bits <= 56; key_bits += 8) { 
        printf("Trying key length %d\n", key_bits);

        memset(generated_key, 0, DES_KEY_SIZE); 
        uint64_t max_key = (1ULL << key_bits) - 1;
        memcpy(generated_key, &max_key, key_bits / 8);
        print_key(generated_key, DES_KEY_SIZE); 

        // Encrypt the plaintext using the generated key
        encrypt_with_key(plaintext, ciphertext, (DES_cblock *)generated_key, &iv);
        printf("Plaintext: ");
        for (int i = 0; i < DES_KEY_SIZE; i++) {
            printf("%02x ", plaintext[i]);
        }
        printf("\n");
        printf("Ciphertext: ");
        for (int i = 0; i < DES_KEY_SIZE; i++) {
            printf("%02x ", ciphertext[i]);
        }
        printf("\n");

        // Brute-force DES to recover the key
        brute_force_des(ciphertext, plaintext, key_bits, &iv);
    }

    return 0;
}
