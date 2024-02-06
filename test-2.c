#include <linux/module.h>   
#include <linux/fs.h>       
#include <linux/cdev.h>     
#include <linux/uaccess.h>  
#include <linux/slab.h>     
#include <linux/device.h> 

#define KEY_LENGTH 16
#define MAX_DATA_SIZE 256

// Data variables
static char aes_key[KEY_LENGTH]; 
static int encrypting = 0;  
static char *plaintext_buffer;
static char *ciphertext_buffer;

// Device related variables
static dev_t aes_dev_number;    
static struct cdev *aes_cdev; 
static struct class *my_device_class; 

// --- AES_TXT ---
int aes_txt_open(struct inode *inode, struct file *file) {
    if ((file->f_mode & FMODE_WRITE) != 0)  
        return -EPERM; 
    return 0;
}

ssize_t aes_txt_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
    int bytes_to_read = min(count, strlen(plaintext_buffer));
    if (copy_to_user(buf, plaintext_buffer + *pos, bytes_to_read))
        return -EFAULT;
    *pos += bytes_to_read;
    return bytes_to_read;
}

// --- AES_EN ---
ssize_t aes_en_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
    int bytes_written = min(count, MAX_DATA_SIZE);

    if (copy_from_user(ciphertext_buffer, buf, bytes_written)) 
        return -EFAULT;

    encrypting = 1;
    xor_encrypt_decrypt(ciphertext_buffer, bytes_written); 
    encrypting = 0;

    return bytes_written;
}

// --- XOR Helper ---
void xor_encrypt_decrypt(char *data, int size) {
    for (int i = 0; i < size; i++)
        data[i] ^= aes_key[i % KEY_LENGTH];
}

// --- SYS FILES --- 
ssize_t sys_key_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
    if (count > KEY_LENGTH)
        return -EINVAL;
    copy_from_user(aes_key, buf, count); 
    return count;
}

ssize_t sys_status_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
    char status_str[15];  
    int len = snprintf(status_str, 15, "Encrypting: %d\n", encrypting);
    return simple_read_from_buffer(buf, count, pos, status_str, len); 
}

// File Ops Structures
static struct file_operations aes_txt_fops = { .owner = THIS_MODULE, .open = aes_txt_open, .read = aes_txt_read };
static struct file_operations aes_en_fops = {  .owner = THIS_MODULE, .write = aes_en_write};
static struct file_operations sys_key_fops = { .owner = THIS_MODULE, .write = sys_key_write };
static struct file_operations sys_status_fops = { .owner = THIS_MODULE, .read = sys_status_read };

// Module Init
static int module_init(void) {
    // Class Creation
    my_device_class = class_create(THIS_MODULE, "aes_class"); 
    if (IS_ERR(my_device_class)) {
        return PTR_ERR(my_device_class);
    }

    // Allocate device number
    int result = alloc_chrdev_region(&aes_dev_number, 0, 2, "aes_demo");
    if (result < 0) 
        goto error_class; 

    // Create cdev structure
    aes_cdev = cdev_alloc(); 
    if (!aes_cdev) {
        result = -ENOMEM;
        goto error_unreg;
    }
    aes_cdev->owner = THIS_MODULE;

        // --- Create /dev/aes_txt ---
    struct device *aes_txt_device = device_create(my_device_class, NULL, MKDEV(MAJOR(aes_dev_number), 0), NULL, "aes_txt"); 
    if (IS_ERR(aes_txt_device)) {
        result = PTR_ERR(aes_txt_device);
        goto error_cdev;
    }
    aes_cdev->ops = &aes_txt_fops; 
    result = cdev_add(aes_cdev, MKDEV(MAJOR(aes_dev_number), 0), 1);
    if (result < 0)
        goto error_dev_aes_txt;

    // --- Create /dev/aes_en ---
    struct device *aes_en_device = device_create(my_device_class, NULL, MKDEV(MAJOR(aes_dev_number), 1), NULL, "aes_en"); 
    if (IS_ERR(aes_en_device)) {
        result = PTR_ERR(aes_en_device);
        goto error_dev_aes_txt; // Cleanup aes_txt on error
    }
    aes_cdev->ops = &aes_en_fops; 
    result = cdev_add(aes_cdev, MKDEV(MAJOR(aes_dev_number), 1), 1);
    if (result < 0) 
        goto error_dev_aes_en; // Cleanup aes_en & aes_txt on error

    // Allocate buffers
    plaintext_buffer = kmalloc(MAX_DATA_SIZE, GFP_KERNEL);
    ciphertext_buffer = kmalloc(MAX_DATA_SIZE, GFP_KERNEL); 
    if (!plaintext_buffer || !ciphertext_buffer) {
        result = -ENOMEM;
        goto error_buffers;
    }

    printk(KERN_INFO "AES Demo Module Loaded\n"); 
    return 0;  // Success

    // --- Error Handling ---
error_buffers:
    kfree(ciphertext_buffer); // If allocation succeeded
error_dev_aes_en:
    device_destroy(my_device_class, MKDEV(MAJOR(aes_dev_number), 1)); 
error_dev_aes_txt:
    device_destroy(my_device_class, MKDEV(MAJOR(aes_dev_number), 0)); 
error_cdev:
    cdev_del(aes_cdev);
error_unreg:
    unregister_chrdev_region(aes_dev_number, 2);
error_class:
    class_destroy(my_device_class); 
    return result;
}

// Module Exit
static void module_exit(void) {
    device_destroy(my_device_class, MKDEV(MAJOR(aes_dev_number), 1)); 
    device_destroy(my_device_class, MKDEV(MAJOR(aes_dev_number), 0)); 
    cdev_del(aes_cdev);
    unregister_chrdev_region(aes_dev_number, 2);
    kfree(plaintext_buffer);
    kfree(ciphertext_buffer);
    class_destroy(my_device_class); 
    printk(KERN_INFO "AES Demo Module Unloaded\n"); 
}

module_init(module_init);
module_exit(module_exit);

MODULE_LICENSE("GPL"); 
// ... Additional MODULE_* macros (description, author, etc.)
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Character Device Driver with R/W and R-Only Devices");