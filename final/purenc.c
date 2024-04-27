#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gcrypt.h>
#include <sys/stat.h>
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
    char *ip;
    char *port;
    int modeop; // 2 for -l and 1 for -d
    int modeop_both = 0;
    char *filename;
    char password[256];
    FILE *file;
    char *buffer;
    long file_size;

    // Checking if correct number of arguments are provided
    if (argc < 3)
    {
        printf("Usage: %s [txt_file] <-d  [IP:port] >  <-l> \n", argv[0]);
        return 1;
    }

    // Checking if the first argument is a txt file
    filename = argv[1];

    // Checking if the second argument is either -d or -l
    char *mode = argv[2];
    if (strcmp(mode, "-d") != 0 && strcmp(mode, "-l") != 0)
    {
        printf("Error: Second argument must be either -d or -l.\n");
        return 1;
    }

    // If -d option is provided, check for IP and port
    if (strcmp(mode, "-d") == 0)
    {
        if (argc < 4)
        {
            printf("Error: When using -d option, you must provide IP:port.\n");
            return 1;
        }
        modeop = 1;
        char *ip_port = argv[3];
        char *delimiter = ":";
        ip = strtok(ip_port, delimiter);
        port = strtok(NULL, delimiter);
        if (ip == NULL || port == NULL)
        {
            printf("Error: Invalid IP:port format.\n");
            return 1;
        }
        printf("File: %s\nMode: %s\nIP: %s\nPort: %s\n", filename, mode, ip, port);
        if (argc == 5)
        {
            modeop_both = 1; // both -d and -l are provided in this order
            printf("File: %s\nMode: l\n", filename);
        }
    }
    else
    {
        modeop = 2;
        printf("File: %s\nMode: %s\n", filename, mode);
        if (argc > 3)

        {
            printf("inside > 3");
            if (argc < 4)
            {
                printf("Error: When using -d option, you must provide IP:port.\n");
                return 1;
            }
            modeop = 1;
            char *ip_port = argv[4];
            char *delimiter = ":";
            ip = strtok(ip_port, delimiter);
            port = strtok(NULL, delimiter);
            if (ip == NULL || port == NULL)
            {
                printf("Error: Invalid IP:port format.\n");
                return 1;
            }
            printf("File: %s\nMode: %s\nIP: %s\nPort: %s\n", filename, mode, ip, port);
            modeop_both = 1; // both -l and -d are provided in this order
        }
    }

    printf("Enter your password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0'; // Remove trailing newline

    file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("Error opening file.\n");
        return 1;
    }

    // fseek(file, 0, SEEK_END);
    // file_size = ftell(file);
    // fseek(file, 0, SEEK_SET);

    struct stat st;
    stat(filename, &st);
    file_size = st.st_size;

    // Allocate memory for the buffer
    buffer = (char *)malloc(file_size + 1);

    // Read file into buffer
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

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

    // Generate salt
    unsigned char salt[16];
    gcry_randomize(salt, sizeof(salt), GCRY_STRONG_RANDOM);

    // Generate Key using PBKDF2
    unsigned char key[KEY_LENGTH];
    error = gcry_kdf_derive(password, strlen(password), GCRY_KDF_PBKDF2, GCRY_MD_SHA256, salt, sizeof(salt), 10000, KEY_LENGTH, key);
    if (error)
    {
        fprintf(stderr, "Error generating key: %s\n", gcry_strerror(error));
        return 1;
    }

    size_t plaintext_len = file_size;
    size_t block_size = gcry_cipher_get_algo_blklen(GCRY_CIPHER_AES256);
    size_t padded_len = ((plaintext_len + block_size - 1) / block_size) * block_size;
    size_t ciphertext_len = padded_len;

    printf("plaintext_len: %ld\n ", plaintext_len);
    printf("block size : %ld\n ", block_size);
    printf("padded len : %ld\n ", padded_len);
    printf("Salt: ");
    for (int i = 0; i < 16; ++i)
    {
        printf("%02x", salt[i]);
    }
    printf("\n");

    unsigned char *ciphertext = malloc(ciphertext_len);
    if (!ciphertext)
    {
        fprintf(stderr, "Error allocating memory for ciphertext\n");
        return 1;
    }

    gcry_cipher_hd_t aes_hd;
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

    unsigned char iv[BLOCK_LENGTH]; // IV length is equal to block size
    gcry_randomize(iv, BLOCK_LENGTH, GCRY_STRONG_RANDOM);
    error = gcry_cipher_setiv(aes_hd, iv, BLOCK_LENGTH);
    if (error)
    {
        fprintf(stderr, "Error setting IV: %s\n", gcry_strerror(error));
        return 1;
    }

    // Pad the plaintext buffer to match block size
    unsigned char *padded_buffer = malloc(padded_len);
    if (!padded_buffer)
    {
        fprintf(stderr, "Error allocating memory for padded buffer\n");
        return 1;
    }
    size_t num_padding_bytes = padded_len - plaintext_len;

    memcpy(padded_buffer, buffer, file_size); // Copy original data
    // memset(padded_buffer + file_size, 0, padded_len - file_size); // Pad the remaining bytes

    memset(padded_buffer + plaintext_len, num_padding_bytes, num_padding_bytes);
    // Encrypt data
    error = gcry_cipher_encrypt(aes_hd, ciphertext, ciphertext_len, padded_buffer, padded_len);
    if (error)
    {
        fprintf(stderr, "Error encrypting data: %s\n", gcry_strerror(error));
        return 1;
    }

    size_t iv_cipher_len = BLOCK_LENGTH + ciphertext_len;
    unsigned char *iv_cipher = malloc(iv_cipher_len);
    if (!iv_cipher)
    {
        fprintf(stderr, "Error allocating memory for IV and ciphertext\n");
        return 1;
    }
    memcpy(iv_cipher, iv, BLOCK_LENGTH);
    memcpy(iv_cipher + BLOCK_LENGTH, ciphertext, ciphertext_len);

    // Calculate HMAC
    unsigned char hmac[HMAC_LENGTH];
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

    // Print HMAC
    printf("HMAC: ");
    for (size_t i = 0; i < HMAC_LENGTH; ++i)
    {
        printf("%02x", hmac[i]);
    }
    printf("\n");

    // Print the ciphertext
    // printf("Ciphertext: ");
    // for (size_t i = 0; i < ciphertext_len; ++i)
    // {
    //     printf("%02x ", ciphertext[i]);
    // }
    // printf("\n");

    size_t concat_len = sizeof(salt) + sizeof(hmac) + BLOCK_LENGTH + ciphertext_len;
    unsigned char *concatenated_data = malloc(concat_len);
    if (!concatenated_data)
    {
        fprintf(stderr, "Error allocating memory for concatenated data\n");
        return 1;
    }

    // Copy salt, hmac, iv, and ciphertext into the concatenated buffer
    memcpy(concatenated_data, salt, sizeof(salt));
    memcpy(concatenated_data + sizeof(salt), hmac, sizeof(hmac));
    memcpy(concatenated_data + sizeof(salt) + sizeof(hmac), iv, BLOCK_LENGTH);
    memcpy(concatenated_data + sizeof(salt) + sizeof(hmac) + BLOCK_LENGTH, ciphertext, ciphertext_len);

    char *output_filename = malloc(strlen(filename) + 5); // 5 for ".pur\0"

    strcpy(output_filename, filename);
    strcat(output_filename, ".pur");

    if (modeop == 2 || modeop_both)
    {
        // printf("inside here!!");
        if (access(output_filename, F_OK) != -1)
        {
            fprintf(stderr, "Error: Output file '%s' already exists.\n", output_filename);
            return 1;
        }

        // Write the concatenated data to a file
        FILE *output_file = fopen(output_filename, "wb");
        if (!output_file)
        {
            fprintf(stderr, "Error opening output file\n");
            return 1;
        }
        fwrite(concatenated_data, 1, concat_len, output_file);
        printf("Concat len: %ld", concat_len);
        // printf("Concatenated data:\n");
        // for (size_t i = 0; i < concat_len; ++i)
        // {
        //     printf("%02x", concatenated_data[i]);
        // }
        // printf("\n");
        fclose(output_file);
    }

    if (modeop == 1 || modeop_both)
    {

        int sock = 0;
        struct sockaddr_in serv_addr;

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Socket creation error");
            exit(EXIT_FAILURE);
        }

        // Set up the address structure
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(atoi(port));

        // Convert IPv4 and IPv6 addresses from text to binary form
        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
        {
            perror("Invalid address/ Address not supported");
            exit(EXIT_FAILURE);
        }

        // Connect to server
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            perror("Connection failed");
            exit(EXIT_FAILURE);
        }

        printf("Connected to server\n");
        // char *messages[] = {"Message 1", "Message 2", "Message 3"};
        // int num_messages = sizeof(messages) / sizeof(messages[0]);

        // Send data to server

        int filename_len = strlen(output_filename);

        // Send the length of the filename to the server
        if (send(sock, &filename_len, sizeof(filename_len), 0) < 0)
        {
            perror("Error sending filename length");
            exit(EXIT_FAILURE);
        }
        printf("Filename length sent: %d\n", filename_len);
        // filename
        if (send(sock, output_filename, strlen(output_filename), 0) < 0)
        {
            perror("Error sending file name");
        }
        printf("Filename sent: %s\n", output_filename);

        // length of file
        if (send(sock, &concat_len, sizeof(concat_len), 0) < 0)
        {
            perror("Error sending length");
        }
        printf("concat_len sent: %zu\n", concat_len);

        if (send(sock, concatenated_data, concat_len, 0) < 0)
        {
            perror("Error sending file");
        }
        printf("File sent success");

        // size_t bytes_read;
        // while ((bytes_read = fread(concatenated_data, 1, concat_len, file)) > 0)
        // {
        //     send(sock, concatenated_data, bytes_read, 0);
        // }
        // for (int i = 0; i < num_messages; i++)
        // {
        //     send(sock, messages[i], strlen(messages[i]), 0);
        //     printf("Message %d sent\n", i + 1);
        // }
    }

    // close(sock);

    // Clean up
    free(buffer);
    free(ciphertext);
    free(padded_buffer);
    gcry_cipher_close(aes_hd);

    return 0;
}
