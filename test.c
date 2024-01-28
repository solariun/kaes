// # A code on user-space to test encrypt and decrypt the code
// **Explanation:**

// 1. This C program is a user-space application that interacts with the vencrypt character device driver to encrypt or decrypt data using the AES algorithm in CBC mode.

// 2. It takes four command-line arguments:
//    - **\<device\>**: The device file path for the vencrypt driver.
//    - **\<encrypt/decrypt\>**: A flag to specify whether to encrypt or decrypt data.
//    - **\<key\>**: The AES key in hexadecimal format.
//    - **\<input\>**: The input data to be encrypted or decrypted.

// 3. The program first opens the vencrypt device file in read-write mode.

// 4. It then sets the AES key using the VENCRYPT_IOCTL_SET_KEY ioctl.

// 5. It sets the encryption/decryption flag using the VENCRYPT_IOCTL_SET_ENCRYPT ioctl.

// 6. It allocates a buffer for the output data.

// 7. Depending on the encryption flag, it writes the input data to the device file and reads the encrypted or decrypted data from the device file.

// 8. Finally, it closes the device file, prints the output data, and frees the allocated buffer.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define VENCRYPT_IOCTL_SET_KEY _IOW('v', 0, char*)
#define VENCRYPT_IOCTL_SET_ENCRYPT _IOW('v', 1, int)

int main(int argc, char **argv) {
  if (argc < 5) {
    printf("Usage: %s <device> <encrypt/decrypt> <key> <input>\n", argv[0]);
    return -1;
  }

  char *device = argv[1];
  int encrypt = atoi(argv[2]);
  char *key = argv[3];
  char *input = argv[4];

  // Open the device file
  int fd = open(device, O_RDWR);
  if (fd < 0) {
    perror("open");
    return -1;
  }

  // Set the AES key
  if (ioctl(fd, VENCRYPT_IOCTL_SET_KEY, key) < 0) {
    perror("ioctl: VENCRYPT_IOCTL_SET_KEY");
    close(fd);
    return -1;
  }

  // Set the encryption/decryption flag
  if (ioctl(fd, VENCRYPT_IOCTL_SET_ENCRYPT, encrypt) < 0) {
    perror("ioctl: VENCRYPT_IOCTL_SET_ENCRYPT");
    close(fd);
    return -1;
  }

  // Get the input data length
  size_t input_len = strlen(input);

  // Allocate a buffer for the output data
  char *output = malloc(input_len + 1);
  if (output == NULL) {
    perror("malloc");
    close(fd);
    return -1;
  }

  // Perform encryption or decryption
  if (encrypt) {
    if (write(fd, input, input_len) < 0) {
      perror("write");
      close(fd);
      free(output);
      return -1;
    }

    if (read(fd, output, input_len) < 0) {
      perror("read");
      close(fd);
      free(output);
      return -1;
    }
  } else {
    if (write(fd, input, input_len) < 0) {
      perror("write");
      close(fd);
      free(output);
      return -1;
    }

    if (read(fd, output, input_len) < 0) {
      perror("read");
      close(fd);
      free(output);
      return -1;
    }

    output[input_len] = '\0';
  }

  // Close the device file
  close(fd);

  // Print the output data
  printf("Output: %s\n", output);

  free(output);
  return 0;
}



