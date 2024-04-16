#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gcrypt.h>

#define KEY_LENGTH 32   // AES-256 key length in bytes
#define BLOCK_LENGTH 16 // AES block size in bytes
#define HMAC_LENGTH 32  // SHA-256 HMAC length in bytes

int main(int argc, char *argv[])
{

    int port;
    int modeop; // 2 for -l and 1 for network
    char *filename;
    char password[256];
    FILE *file;
    char *buffer;
    long file_size;

    // Checking if correct number of arguments are provided
    if (argc != 3)
    {
        printf("Usage: %s <txt_file> <-l or port_number>\n", argv[0]);
        return 1;
    }

    // Checking if the first argument is a txt file
    filename = argv[1];
    // if (strstr(filename, ".txt") == NULL)
    // {
    //     printf("Error: First argument must be a txt file.\n");
    //     return 1;
    // }

    // Checking if the second argument is either -l or a port number
    char *mode_or_port = argv[2];
    if (strcmp(mode_or_port, "-l") != 0)
    {
        // Check if the argument is a valid port number
        port = atoi(mode_or_port);
        if (port <= 0 || port > 65535)
        {
            printf("Error: Second argument must be either -l or a valid port number.\n");
            return 1;
        }
        modeop = 1;
        printf("File: %s\nPort: %s\n", filename, mode_or_port);
    }
    else

    {
        modeop = 2;
        printf("File: %s\nMode %s\n", filename, mode_or_port);
    }

    printf("Enter your password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0'; // Remove trailing newline

    file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Error opening file.\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the buffer
    buffer = (char *)malloc(file_size + 1);

    // Read file into buffer
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    // Print the ciphertext
    printf("file_size: %ld\n", file_size);

    fclose(file);

    // Print contents of the buffer
    // printf("File contents:\n%s\n", buffer);

    gcry_error_t error;

    // Initialize libgcrypt
    if (!gcry_check_version(GCRYPT_VERSION))
    {
        fprintf(stderr, "libgcrypt version mismatch\n");
        return 1;
    }
    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    // salt
    unsigned char salt[16];
    memcpy(salt, buffer, 16);

    unsigned char key[KEY_LENGTH];
    error = gcry_kdf_derive(password, strlen(password), GCRY_KDF_PBKDF2, GCRY_MD_SHA256, salt, sizeof(salt), 10000, KEY_LENGTH, key);
    if (error)
    {
        fprintf(stderr, "Error generating key: %s\n", gcry_strerror(error));
        return 1;
    }

    unsigned char given_hmac[HMAC_LENGTH];
    unsigned char hmac[HMAC_LENGTH];
    size_t ciphertext_len = file_size - BLOCK_LENGTH - HMAC_LENGTH - BLOCK_LENGTH;
    size_t iv_cipher_len = BLOCK_LENGTH + ciphertext_len;
    unsigned char *iv_cipher = malloc(iv_cipher_len);
    memcpy(iv_cipher, buffer + BLOCK_LENGTH + HMAC_LENGTH, iv_cipher_len);

    if (!iv_cipher)
    {
        fprintf(stderr, "Error allocating memory for IV and ciphertext\n");
        return 1;
    }

    gcry_md_hd_t hmac_hd;
    error = gcry_md_open(&hmac_hd, GCRY_MD_SHA256, GCRY_MD_FLAG_HMAC);
    if (error)
    {
        fprintf(stderr, "Error opening HMAC: %s\n", gcry_strerror(error));
        return 1;
    }
    error = gcry_md_setkey(hmac_hd, key, KEY_LENGTH);
    if (error)
    {
        fprintf(stderr, "Error setting HMAC key: %s\n", gcry_strerror(error));
        return 1;
    }
    gcry_md_write(hmac_hd, iv_cipher, iv_cipher_len);
    memcpy(hmac, gcry_md_read(hmac_hd, GCRY_MD_SHA256), HMAC_LENGTH);
    // printf("HMAC: ");
    // for (size_t i = 0; i < HMAC_LENGTH; ++i)
    // {
    //     printf("%02x", hmac[i]);
    // }
    // printf("\n");

    memcpy(given_hmac, buffer + 16, 32);

    if (memcmp(hmac, given_hmac, HMAC_LENGTH) == 0)
    {
        printf("HMAC verification successful\n");
    }
    else
    {
        printf("HMAC verification failed\n");
        return 1;
    }

    unsigned char iv[BLOCK_LENGTH];
    memcpy(iv, buffer + BLOCK_LENGTH + HMAC_LENGTH, BLOCK_LENGTH);

    gcry_cipher_hd_t aes_hd;
    size_t plaintext_len = ciphertext_len;
    unsigned char *plaintext = malloc(plaintext_len);
    if (!plaintext)
    {
        fprintf(stderr, "Error allocating memory for plaintext\n");
        return 1;
    }

    error = gcry_cipher_open(&aes_hd, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_CBC, 0);
    if (error)
    {
        fprintf(stderr, "Error opening AES cipher: %s\n", gcry_strerror(error));
        return 1;
    }

    error = gcry_cipher_setkey(aes_hd, key, KEY_LENGTH);
    if (error)
    {
        fprintf(stderr, "Error setting AES key: %s\n", gcry_strerror(error));
        return 1;
    }

    error = gcry_cipher_setiv(aes_hd, iv, BLOCK_LENGTH);
    if (error)
    {
        fprintf(stderr, "Error setting AES IV: %s\n", gcry_strerror(error));
        return 1;
    }

    error = gcry_cipher_decrypt(aes_hd, plaintext, plaintext_len, iv_cipher + BLOCK_LENGTH, ciphertext_len);
    if (error)
    {
        fprintf(stderr, "Error decrypting ciphertext: %s\n", gcry_strerror(error));
        return 1;
    }

    printf("Decryption successful\n");
    // Print the decrypted plaintext
    // printf("Decrypted plaintext:\n%s\n", plaintext);
    // printf("Decrypted plaintext:\n");
    // for (size_t i = 0; i < plaintext_len; ++i)
    // {
    //     printf("%02x ", (unsigned char)plaintext[i]);
    // }
    // printf("\n");

    // printf("Salt: ");
    // for (int i = 0; i < 16; ++i)
    // {
    //     printf("%02x", salt[i]);
    // }
    // printf("\n");

    char *new_filename = strdup(filename);  // Duplicate the filename
    char *dot = strrchr(new_filename, '.'); // Find the last occurrence of '.'
    if (dot != NULL)
    {
        *dot = '\0'; // Replace the dot with null terminator
    }

    size_t new_plaintext_len = plaintext_len;
    if (new_plaintext_len > 0)
    {
        unsigned char last_byte = plaintext[new_plaintext_len - 1];

        if (last_byte > 0 && last_byte <= BLOCK_LENGTH)
        {
            new_plaintext_len -= last_byte;
        }
    }

    // for (size_t i = 0; i < new_plaintext_len; ++i)
    // {
    //     printf("%02x ", (unsigned char)plaintext[i]);
    // }
    // printf("\n");
    FILE *output_file = fopen(new_filename, "wb"); // Open file in binary write mode
    if (output_file == NULL)
    {
        printf("Error opening output file.\n");
        free(new_filename);
        return 1;
    }

    // fwrite(plaintext, sizeof(unsigned char), plaintext_len, output_file); // Write decrypted binary data to the output file
    fwrite(plaintext, sizeof(unsigned char), new_plaintext_len, output_file);
    fclose(output_file);
    free(new_filename);

    return 0;
}