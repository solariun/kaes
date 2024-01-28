//   - It presents two-character devices: /dev/vencrypt_pt and /dev/vencrypt_ct (where "pt" stands for plaintext, and "ct" for ciphertext)
//  - It exposes two module parameters
//       a) one called "encrypt" which can be set to 1 or 0 to control whether the module is encrypting or decrypting
//       b) one called "key" which is set to a hex string representing the AES key to use
//  The character devices should accept only a single open (i.e., only one client can be reading/writing to each at a time). The idea is that if encrypt == 1, then someone in user space can open and write plaintext data to /dev/vencrypt_pt, with the encrypted result available through read of /dev/vencrypt_ct.
//  Conversely, if encrypt == 0, then ciphertext can be written to /dev/vencrypt_ct, with the decrypted plaintext available via read of /dev/vencrypt_pt.
//  You should use AES in its Cipher Block Chaining (CBC) mode, and reset the IV to all zeroes for each new open of the source character device (i.e., /dev/vencrypt_pt for encrypt == 1, or /dev/vencrypt_ct for encrypt == 0).

//  You are not expected to write AES from scratch. Instead please incorporate a suitable implementation that is in the public domain or suitably licensed for our purposes. Please keep this third party code clearly separated from your own, and retain its licensing info.
//  code must be under 4000 chars 


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <crypto/aes.h>

#define MODULE_NAME "vencrypt"
#define DEVICE_NAME "vencrypt"
#define MAX_KEY_SIZE 32 // maximum key size in bytes

// Device structure
struct vencrypt_dev {
  struct cdev cdev;
  struct crypto_cipher *cipher_ctx;
  struct crypto_skcipher *skcipher_ctx;
  u8 iv[AES_BLOCK_SIZE]; // initialization vector
  int encrypt;
  u8 key[MAX_KEY_SIZE];
  int key_len;
};

// Module parameters
static int encrypt = 1;
module_param(encrypt, int, 0644);
MODULE_PARM_DESC(encrypt, "Encrypt (1) or Decrypt (0)");

static char *key = NULL;
module_param(key, charp, 0644);
MODULE_PARM_DESC(key, "AES key in hex string");

// Device file operations
static int vencrypt_open(struct inode *inode, struct file *filp) {
  struct vencrypt_dev *dev = container_of(inode->i_cdev, struct vencrypt_dev, cdev);

  // Allow only one open at a time
  if (!mutex_trylock(&dev->cdev.lock)) {
    return -EBUSY;
  }

  filp->private_data = dev;
  return 0;
}

static int vencrypt_release(struct inode *inode, struct file *filp) {
  struct vencrypt_dev *dev = filp->private_data;

  // Release the lock and reset the device state
  mutex_unlock(&dev->cdev.lock);
  return 0;
}

static long vencrypt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  struct vencrypt_dev *dev = filp->private_data;

  switch (cmd) {
    case VENCRYPT_IOCTL_SET_KEY:
      // Set the AES key
      if (copy_from_user(dev->key, (void __user *)arg, MAX_KEY_SIZE)) {
        return -EFAULT;
      }
      dev->key_len = strlen(dev->key);
      if (dev->key_len % 2 != 0) {
        return -EINVAL;
      }
      dev->key_len /= 2;
      return 0;

    case VENCRYPT_IOCTL_SET_ENCRYPT:
      // Set the encryption/decryption flag
      dev->encrypt = arg;
      return 0;

    default:
      return -ENOTTY;
  }
}

static ssize_t vencrypt_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
  struct vencrypt_dev *dev = filp->private_data;
  int ret;

  // Check if the device is set for encryption or decryption
  if (dev->encrypt) {
    return -EOPNOTSUPP;
  }

  // Perform decryption
  ret = crypto_skcipher_decrypt(dev->skcipher_ctx, buf, buf, count);
  if (ret < 0) {
    return ret;
  }

  return count;
}

static ssize_t vencrypt_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
  struct vencrypt_dev *dev = filp->private_data;
  int ret;

  // Check if the device is set for encryption or decryption
  if (!dev->encrypt) {
    return -EOPNOTSUPP;
  }

  // Perform encryption
  ret = crypto_skcipher_encrypt(dev->skcipher_ctx, buf, buf, count);
  if (ret < 0) {
    return ret;
  }

  return count;
}

static const struct file_operations vencrypt_fops = {
  .owner = THIS_MODULE,
  .open = vencrypt_open,
  .release = vencrypt_release,
  .unlocked_ioctl = vencrypt_ioctl,
  .read = vencrypt_read,
  .write = vencrypt_write,
};

// Module initialization
static int __init vencrypt_init(void) {
  int ret;
  dev_t dev_num;
  struct vencrypt_dev *dev;

  // Allocate a device number
  ret = alloc_chrdev_region(&dev_num, 0, 2, DEVICE_NAME);
  if (ret < 0) {
    pr_err("Failed to allocate device number\n");
    return ret;
  }

  // Initialize the device structure
  dev = kzalloc(sizeof(*dev), GFP_KERNEL);
  if (dev == NULL) {
    ret = -ENOMEM;
    goto err_alloc;
  }

  cdev_init(&dev->cdev, &vencrypt_fops);
  dev->cdev.owner = THIS_MODULE;

  // Add the device to the system
  ret = cdev_add(&dev->cdev, dev_num, 1);
  if (ret < 0) {
    pr_err("Failed to add device\n");
    goto err_add;
  }

  // Create the crypto context
  dev->cipher_ctx = crypto_alloc_cipher("aes", 0, 0);
  if (IS_ERR(dev->cipher_ctx)) {
    ret = PTR_ERR(dev->cipher_ctx);
    goto err_cipher;
  }

  dev->skcipher_ctx = crypto_alloc_skcipher("cbc(aes)", 0, 0);
  if (IS_ERR(dev->skcipher_ctx)) {
    ret = PTR_ERR(dev->skcipher_ctx);
    goto err_skcipher;
  }

  // Set the key and IV
  if (key == NULL) {
    pr_err("AES key not provided\n");
    ret = -EINVAL;
    goto err_key;
  }

  ret = hex2bin(dev->key, key, dev->key_len);
  if (ret < 0) {
    pr_err("Invalid AES key format\n");
    goto err_key;
  }

  memset(dev->iv, 0, AES_BLOCK_SIZE);

  // Initialize the skcipher context
  crypto_skcipher_setkey(dev->skcipher_ctx, dev->key, dev->key_len);
  crypto_skcipher_set_iv(dev->skcipher_ctx, dev->iv, AES_BLOCK_SIZE);

  pr_info("vencrypt: Module loaded\n");
  return 0;

err_key:
  crypto_free_skcipher(dev->skcipher_ctx);
err_skcipher:
  crypto_free_cipher(dev->cipher_ctx);
err_cipher:
  cdev_del(&dev->cdev);
err_add:
  kfree(dev);
err_alloc:
  unregister_chrdev_region(dev_num, 1);
  return ret;
}

// Module cleanup
static void __exit vencrypt_exit(void) {
  dev_t dev_num = MKDEV(MAJOR(register_chrdev_region(0, 1, DEVICE_NAME)), 0);
  struct vencrypt_dev *dev = container_of(cdev_get(dev_num), struct vencrypt_dev, cdev);

  // Release resources
  crypto_free_skcipher(dev->skcipher_ctx);
  crypto_free_cipher(dev->cipher_ctx);
  cdev_del(&dev->cdev);
  kfree(dev);
  unregister_chrdev_region(dev_num, 1);
  pr_info("vencrypt: Module unloaded\n");
}

module_init(vencrypt_init);
module_exit(vencrypt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("vencrypt: A simple character device driver for encrypting and decrypting data using AES in CBC mode");

