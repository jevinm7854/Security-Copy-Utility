#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    FILE *file1, *file2;
    unsigned char buffer1[BUFFER_SIZE];
    unsigned char buffer2[BUFFER_SIZE];
    size_t bytesRead1, bytesRead2;
    int foundDifference = 0;
    size_t pos = 0;

    // Check if correct number of arguments are provided
    if (argc != 3)
    {
        printf("Usage: %s <file1> <file2>\n", argv[0]);
        return 1;
    }

    // Open the first file
    file1 = fopen(argv[1], "rb");
    if (file1 == NULL)
    {
        printf("Error opening file: %s\n", argv[1]);
        return 1;
    }

    // Open the second file
    file2 = fopen(argv[2], "rb");
    if (file2 == NULL)
    {
        printf("Error opening file: %s\n", argv[2]);
        fclose(file1);
        return 1;
    }
    long size1, size2;

    fseek(file1, 0, SEEK_END);
    size1 = ftell(file1);
    fseek(file1, 0, SEEK_SET);

    // Get the size of the second file
    fseek(file2, 0, SEEK_END);
    size2 = ftell(file2);
    fseek(file2, 0, SEEK_SET);

    // Print the sizes of both files
    printf("Size of %s: %ld bytes\n", argv[1], size1);
    printf("Size of %s: %ld bytes\n", argv[2], size2);

    long startPos = 21718;
    long endPos = 21728;
    size_t bytesRead;
    unsigned char byte;
    unsigned char buffer[BUFFER_SIZE];

    if (fseek(file2, startPos, SEEK_SET) != 0)
    {
        printf("Error seeking to position %ld in file: %s\n", startPos, argv[1]);
        fclose(file2);
        return 1;
    }

    while (ftell(file2) < endPos)
    {
        if (fread(&byte, sizeof(unsigned char), 1, file2) != 1)
        {
            printf("Error reading file: %s\n", argv[1]);
            fclose(file2);
            return 1;
        }
        printf("%02X ", byte);
    }
    printf("\n");

    // Read and compare files chunk by chunk
    do
    {
        bytesRead1 = fread(buffer1, 1, BUFFER_SIZE, file1);
        bytesRead2 = fread(buffer2, 1, BUFFER_SIZE, file2);

        // Compare bytes read
        if (bytesRead1 != bytesRead2)
        {
            printf("Files differ in size.\n");
            foundDifference = 1;
            break;
        }

        // Compare buffer content
        for (size_t i = 0; i < bytesRead1; ++i)
        {
            if (buffer1[i] != buffer2[i])
            {
                printf("Difference at position %zu: file1[%zu] = %u, file2[%zu] = %u\n",
                       pos + i, pos + i, buffer1[i], pos + i, buffer2[i]);
                foundDifference = 1;
            }
        }

        pos += bytesRead1;

    } while (bytesRead1 > 0 && bytesRead2 > 0);

    // If no difference found, indicate that files are identical
    if (!foundDifference)
    {
        printf("Files are identical.\n");
    }

    // Close files
    fclose(file1);
    fclose(file2);

    return 0;
}
