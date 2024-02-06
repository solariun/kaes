#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16

// AES key and IV
static uint8_t aes_key[AES_KEY_SIZE];
static uint8_t aes_iv[AES_BLOCK_SIZE];

// Define the byte-to-hex mapping
const char hex_map[] = "0123456789abcdef";

// Function to convert a byte to a hex string
void byte_to_hex(char *dest, unsigned char byte) {
    dest[0] = hex_map[(byte >> 4) & 0xf];
    dest[1] = hex_map[byte & 0xf];
}

// Function to dump a buffer of bytes in hex and ASCII format
void dump_bytes(const char* pnzName, const unsigned char *buf, size_t len) {
    printf("[%10s]---------------------------\n", pnzName);
    // Loop through the buffer
    for (size_t i = 0; i < len; i += 8) {
        // Print the base address
        printf("0x%016llx: ", (unsigned long long)buf + i);

        // Print the bytes in hex format
        for (size_t j = 0; j < 8; j++) {
            if (i + j < len) {
                char hex[3] = {0};
                byte_to_hex(hex, buf[i + j]);
                printf("%s ", hex);
            } else {
                printf("   ");
            }
        }

        // Print the bytes in ASCII format
        printf(" ");
        for (size_t j = 0; j < 8; j++) {
            if (i + j < len) {
                unsigned char byte = buf[i + j];
                if (byte >= 32 && byte < 127) {
                    printf("%c", byte);
                } else {
                    printf(".");
                }
            }
        }

        // Print a newline
        printf("\n");
    }
}

// Helper functions
static int hex_to_byte(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else {
        return -1; // Invalid character
    }
}

/**
 * is_hex_char - Checks if a character is a hexadecimal digit.
 * @c: The character to check.
 *
 * Returns:
 *   1 if the character is a hexadecimal digit, 0 otherwise.
 */
int is_hex_char(char c)
{
    // Check if the character is a hexadecimal digit (0-9, a-f, A-F).

    if ((c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F')) {
        return 1;
    } else {
        return 0;
    }
}


// Function to convert the hexadecimal AES key and IV to binary format
static int parse_key_and_iv(const char *key_str, size_t len) {
    int i;
    const size_t nHexSize = (AES_KEY_SIZE + AES_BLOCK_SIZE) * 2;

    // Check for the correct length of the key and IV combined
    if (len != nHexSize) {
        printf("Error, Invalid key and IV string length: %d (%d).\n", len, nHexSize);
        return -EINVAL;
    }

    // Check if the key and IV are in hexadecimal format
    for (i = 0; i < len; i++) {
        if (!is_hex_char(key_str[i])) {
            printf("Error, Invalid character in key and IV string.\n");
            return -EINVAL;
        }
    }

    size_t offset = 0;

    // Parse the key and IV from the hexadecimal string
    for (i = 0; i < AES_KEY_SIZE; i++) {
        offset = i * 2;
        aes_key[i] = (hex_to_byte(key_str[offset]) << 4) | hex_to_byte(key_str[offset + 1]);
    }

    for (i = 0; i < AES_BLOCK_SIZE; i++) {
        offset = (AES_BLOCK_SIZE * 2) + (i * 2);
        aes_iv[i] = (hex_to_byte(key_str[offset]) << 4) | hex_to_byte(key_str[offset + 1]);
    }

    return 0;
}

char  const key[]="0001020304050607080910111213141515141312111009080706050403020100";

int main(int nArgs, char** ppszArgs)
{
    if (parse_key_and_iv(key, sizeof(key)-1) < 0) {
        printf("Error parsing DATA.\n\n");
        return -EINVAL;
    }

    dump_bytes("KEY", aes_key, sizeof(aes_key));
    dump_bytes("IV", aes_iv, sizeof(aes_iv));

    return 0;
}