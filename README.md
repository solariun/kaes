# kaes
AES secure linux kernel module f

### Steps
1.  **Target Linux Kernel Version:**
Set the target Linux kernel version for the module, for example, Linux 5.15.0.

2.  **AES Implementation:**
Choose an open-source or permissively licensed AES implementation, such as the [Tiny AES](https://github.com/kokke/tiny-AES-c) library, which implements AES in CBC mode.

3.  **Kernel Module Structure:**
Create a Linux kernel module structure with the following components:
- **Module Parameters:**
 - encrypt: To control whether the module should encrypt (1) or decrypt (0).
 - key: To specify the hexadecimal AES key.
- **Character Devices:**
 - /dev/vencrypt_pt: For plaintext input and ciphertext output when encrypt is 1.
 - /dev/vencrypt_ct: For ciphertext input and plaintext output when encrypt is 0.
- **Module Functions:**
 - init_module(): Initializes the module, registers the character devices, and allocates memory for the AES context.
 - cleanup_module(): Cleans up the module, unregisters the character devices, and frees allocated memory.
 - open(): Opens the character device, initializes the AES context with the specified key, and resets the IV to all zeroes.
 - release(): Closes the character device and frees any allocated resources.
 - read(): Reads encrypted or decrypted data from the character device.
 - write(): Writes plaintext or ciphertext data to the character device.

4.  **Implementation:**
- **Initialization and Cleanup:** Implement init_module() and cleanup_module() to handle module initialization and cleanup.
- **Character Device Operations:** Implement the open(), release(), read(), and write() functions to handle open, close, read, and write operations on the character devices.
- **AES Encryption/Decryption:** Implement encryption and decryption functions using the chosen AES implementation library.

5.  **Compilation and Loading:**
Compile the module and load it into the running kernel using insmod.

### Corner Cases

- **Valid Input:**
- Ensure that encrypt is set to 1 for encryption and 0 for decryption.
- Ensure that the key parameter is a valid hexadecimal string representing a 128-bit AES key.
- **Buffer Sizes:**
- Handle cases where the input data is not a multiple of the AES block size (16 bytes).
- **Error Handling:**
- Handle errors during initialization, open, read, write, and cleanup operations.