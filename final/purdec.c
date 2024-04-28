#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gcrypt.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    size_t file_size;

    // Checking if correct number of arguments are provided
    if (argc == 3)

    {
        filename = argv[2];
        char *mode_or_port = argv[1];
        if (strcmp(mode_or_port, "-l") != 0)
        {
            printf("need second argument as -l for local mode");
            return 1;
        }
        modeop = 2;
        printf("File: %s\nMode %s\n", filename, mode_or_port);
        // printf("Usage: %s <txt_file> <-l or port_number>\n", argv[0]);
        // return 1;
    }

    else if (argc == 2)
    {
        char *mode_or_port = argv[1];
        port = atoi(mode_or_port);
        if (port <= 0 || port > 65535)
        {
            printf("Error: Should be a valid port number to run on network mode .\n");
            return 1;
        }
        modeop = 1;
        printf("Port: %s\n", mode_or_port);
    }
    else
    {
        printf("Usage: %s <file_name -l> or < port_number>\n", argv[0]);
        return 1;
    }

    // printf("Enter your password: ");
    // fgets(password, sizeof(password), stdin);
    // password[strcspn(password, "\n")] = '\0';

    if (modeop == 1)
    {
        int server_fd, new_socket;
        struct sockaddr_in address;
        int addrlen = sizeof(address);

        // Create socket
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        // Set up the address structure
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        // Bind socket to address
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        // Listen for incoming connections
        if (listen(server_fd, 3) < 0)
        {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        // Accept incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("Connection accepted. Waiting for data...\n");

        int filename_len;
        int bytes_received = read(new_socket, &filename_len, sizeof(filename_len));
        if (bytes_received < 0)
        {
            perror("Error receiving filename length");
            exit(EXIT_FAILURE);
        }
        printf("Filename length received: %d\n", filename_len);

        // Allocate memory for filename
        filename = malloc(filename_len + 1); // Plus 1 for null terminator
        if (filename == NULL)
        {
            perror("Error allocating memory for filename");
            exit(EXIT_FAILURE);
        }

        // Receive filename from client
        bytes_received = read(new_socket, filename, filename_len);
        if (bytes_received < 0)
        {
            perror("Error receiving filename");
            exit(EXIT_FAILURE);
        }

        filename[filename_len] = '\0';

        // for (int i = 0; i <= filename_len; i++)
        // {
        //     printf("!!!!!!!!!!! Filename is %d\n", filename[i]);
        // }

        bytes_received = read(new_socket, &file_size, sizeof(file_size));
        if (bytes_received < 0)
        {
            perror("Error receiving file size");
            exit(EXIT_FAILURE);
        }
        printf("file_size before buffer is %ld\n", file_size);

        buffer = (char *)malloc(file_size + 1);

        bytes_received = read(new_socket, buffer, file_size);
        if (bytes_received < 0)
        {
            perror("Error receiving file content");
            free(buffer); // Free allocated memory
            exit(EXIT_FAILURE);
        }
        if ((unsigned long int)bytes_received != file_size)
        {
            printf("Incomplete file content received\n");
            free(buffer); // Free allocated memory
            exit(EXIT_FAILURE);
        }
    }

    // Checking if the first argument is a txt file
    // filename = argv[1];
    // if (strstr(filename, ".txt") == NULL)
    // {
    //     printf("Error: First argument must be a txt file.\n");
    //     return 1;
    // }

    // Checking if the second argument is either -l or a port number
    // char *mode_or_port = argv[2];
    // if (strcmp(mode_or_port, "-l") != 0)
    // {
    //     // Check if the argument is a valid port number
    //     port = atoi(mode_or_port);
    //     if (port <= 0 || port > 65535)
    //     {
    //         printf("Error: Second argument must be either -l or a valid port number.\n");
    //         return 1;
    //     }
    //     modeop = 1;
    //     printf("File: %s\nPort: %s\n", filename, mode_or_port);
    // }
    // else

    // {
    //     modeop = 2;
    //     printf("File: %s\nMode %s\n", filename, mode_or_port);
    // }

    // printf("Enter your password: ");
    // fgets(password, sizeof(password), stdin);
    // password[strcspn(password, "\n")] = '\0'; // Remove trailing newline

    if (modeop == 2)
    {
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
    }
    // Print contents of the buffer
    // printf("File contents:\n%s\n", buffer);

    printf("Enter your password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';

    char *new_filename = strdup(filename);  // Duplicate the filename
    char *dot = strrchr(new_filename, '.'); // Find the last occurrence of '.'
    if (dot != NULL)
    {
        *dot = '\0'; // Replace the dot with null terminator
    }

    if (access(new_filename, F_OK) != -1)
    {
        fprintf(stderr, "Error: Output file '%s' already exists.\n", new_filename);
        return 1;
    }

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
    printf("Decrypted plain_text len: %ld\n", plaintext_len);
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

    // char *new_filename = strdup(filename);  // Duplicate the filename
    // char *dot = strrchr(new_filename, '.'); // Find the last occurrence of '.'
    // if (dot != NULL)
    // {
    //     *dot = '\0'; // Replace the dot with null terminator
    // }

    // if (access(new_filename, F_OK) != -1)
    // {
    //     fprintf(stderr, "Error: Output file '%s' already exists.\n", new_filename);
    //     return 1;
    // }

    size_t new_plaintext_len = plaintext_len;
    bool valid = true;

    if (new_plaintext_len > 0)
    {
        unsigned char last_byte = plaintext[new_plaintext_len - 1];

        if (last_byte > 0 && last_byte < BLOCK_LENGTH)
        {
            for (int i = 1; i <= last_byte; ++i)
            {
                if (plaintext[new_plaintext_len - i] != last_byte)
                {
                    valid = false;
                    break;
                }
            }

            if (valid)
            {
                new_plaintext_len -= last_byte;
            }
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