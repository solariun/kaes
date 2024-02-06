#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h> 
#include <linux/cdev.h> 
#include <linux/uaccess.h> 
#include <linux/device.h> // For device-related improvements

static dev_t my_device_number;
static struct class *my_device_class;
static struct device *my_device; // Use struct device

module_param(a1, int, 0644); // Integer parameter
MODULE_PARM_DESC(a1, "First parameter (integer)"); 

module_param(holder, charp, 0644); // String parameter 
MODULE_PARM_DESC(holder, "Second parameter (string)"); 

static size_t bytes_read = 0;  // Counter for bytes read
#define MY_IOCTL_READ_COUNTER _IOR('M', 0, size_t)  // Simple ioctl magic number

static int my_open(struct inode *inode, struct file *file) {
   printk(KERN_INFO "my_cipher_device opened\n");
   return 0;
}

static int my_release(struct inode *inode, struct file *file) {
   printk(KERN_INFO "my_cipher_device closed\n");
   return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t len, loff_t *off) {
   char *temp_buffer = kmalloc(len, GFP_KERNEL); 
   if (!temp_buffer)
       return -ENOMEM;

   if (copy_from_user(temp_buffer, buf, len) != 0) {
       kfree(temp_buffer);
       return -EFAULT;
   }

   // Since we just copy, send the same data back
   if (copy_to_user(buf, temp_buffer, len) != 0) {
       kfree(temp_buffer);
       return -EFAULT;
   }

   bytes_read++;
   
   kfree(temp_buffer);
   return len; 
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
   char *temp_buffer = kmalloc(len, GFP_KERNEL);
   if (!temp_buffer)
       return -ENOMEM;

   if (copy_from_user(temp_buffer, buf, len) != 0) {
       kfree(temp_buffer);
       return -EFAULT;
   }

   // ... Here you could process/store the data from temp_buffer

   kfree(temp_buffer);
   return len;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    if (cmd == MY_IOCTL_READ_COUNTER) {
        if (copy_to_user((size_t __user *)arg, &bytes_read, sizeof(size_t)) != 0) {
            return -EFAULT; 
        }
        return 0; 
    } else {
        return -ENOTTY; // Indicate unsupported ioctl command
    }
}

static struct file_operations my_fops = {
   .owner   = THIS_MODULE,
   .open    = my_open,
   .release = my_release,
   .read    = my_read,
   .write   = my_write
   .unlocked_ioctl = my_ioctl, // For modern kernels
};

static int __init my_cipher_init(void) {
   int ret;

   ret = alloc_chrdev_region(&my_device_number, 0, 1, "my_passthrough_device");
   if (ret < 0) {
       printk(KERN_ERR "Failed to allocate major number\n");
       return ret;
   }

   my_device_class = class_create(THIS_MODULE, "my_passthrough_class");
   if (IS_ERR(my_device_class)) {
       printk(KERN_ERR "Failed to create device class\n");
       ret = PTR_ERR(my_device_class); 
       goto err_unregister_chrdev;
   }

   my_device = device_create(my_device_class, NULL, my_device_number, NULL, "my_passthrough_device");
   if (IS_ERR(my_device)) {
       printk(KERN_ERR "Failed to create device\n");
       ret = PTR_ERR(my_device);
       goto err_class_destroy;
   }

   cdev_init(&my_device, &my_fops); 
   ret = cdev_add(&my_device, my_device_number, 1);
   if (ret < 0) {
       printk(KERN_ERR "Failed to add device to kernel\n");
       goto err_device_destroy;
   }

   printk(KERN_INFO "my_passthrough_device module loaded successfully\n");
   return 0;

err_device_destroy:
   device_destroy(my_device_class, my_device_number);
err_class_destroy:
   class_destroy(my_device_class);
err_unregister_chrdev:
   unregister_chrdev_region(my_device_number, 1);
   return ret;
}

static void __exit my_cipher_exit(void) {
   // Cleanup in reverse order
   cdev_del(&my_device);
   device_destroy(my_device_class, my_device_number);
   class_destroy(my_device_class);
   unregister_chrdev_region(my_device_number, 1);
   printk(KERN_INFO "my_passthrough_device module unloaded\n");
}

module_init(my_cipher_init);
module_exit(my_cipher_exit);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Your Name"); 
MODULE_DESCRIPTION("My simple passthrough character device"); 
