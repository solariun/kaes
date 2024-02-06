#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>  
#include <linux/device.h> 
#include <linux/slab.h>  

#define DEVICE_NAME_CT "aes_ct" // decypher text
#define DEVICE_NAME_CD "aes_cd" // cypher data
#define BUFFER_SIZE 256

struct text_device {
    struct cdev cdev;
    dev_t dev_number;
    struct class *dev_class;
    struct device *device;
    char buffer[BUFFER_SIZE];
    int key;
    int status;
};

static struct text_device *my_device;

static int text_open(struct inode *inode, struct file *file) {
    struct text_device *dev = container_of(inode->i_cdev, struct text_device, cdev);
    file->private_data = dev; 
    printk(KERN_INFO "%s device opened!\n", DEVICE_NAME_CT); 
    return 0;
}

static int text_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "%s device closed!\n", DEVICE_NAME_CT);
    return 0;
}

static ssize_t text_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    struct text_device *dev = file->private_data;

    if (*offset >= BUFFER_SIZE)
        return 0; 

    if (*offset + count > BUFFER_SIZE)
        count = BUFFER_SIZE - *offset;

    if (copy_to_user(buf, dev->buffer + *offset, count))
        return -EFAULT;

    *offset += count;
    return count;
}

static ssize_t text_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    struct text_device *dev = file->private_data; 

    if (*offset >= BUFFER_SIZE)
        return -ENOSPC; 

    if (*offset + count > BUFFER_SIZE)
        count = BUFFER_SIZE - *offset;

    if (copy_from_user(dev->buffer + *offset, buf, count))
        return -EFAULT;

    *offset += count;
    return count;
}

static ssize_t key_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct text_device *tdev = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", tdev->key); 
}

static ssize_t key_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    struct text_device *tdev = dev_get_drvdata(dev);
    sscanf(buf, "%d", &tdev->key);
    return count;
}

static ssize_t status_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct text_device *tdev = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", tdev->status); 
}

static ssize_t status_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    struct text_device *tdev = dev_get_drvdata(dev);
    sscanf(buf, "%d", &tdev->status);
    return count;
}

static DEVICE_ATTR_RW(key);  // dev_attr_key
static DEVICE_ATTR_RW(status); // dev_attr_status

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = text_open,
    .release = text_release,
    .read    = text_read,
    .write   = text_write
};

static int __init text_driver_init(void) {
    int ret; 

    my_device = kmalloc(sizeof(struct text_device), GFP_KERNEL);
    if (!my_device) {
        printk(KERN_ERR "%s: Failed to allocate memory\n", DEVICE_NAME_CT); 
        return -ENOMEM;
    }

    ret = alloc_chrdev_region(&my_device->dev_number, 0, 1, DEVICE_NAME_CT);
    if (ret < 0) {
        goto fail_alloc;
    }

    cdev_init(&my_device->cdev, &fops);
    ret = cdev_add(&my_device->cdev, my_device->dev_number, 1);
    if (ret < 0) {
        goto fail_cdev_add;
    }

    my_device->dev_class = class_create(THIS_MODULE, DEVICE_NAME_CT);
    if (IS_ERR(my_device->dev_class)) {
        ret = PTR_ERR(my_device->dev_class); 
        goto fail_class_create;
    }

    my_device->device = device_create(my_device->dev_class, NULL, my_device->dev_number, NULL, DEVICE_NAME_CT);
    if (IS_ERR(my_device->device)) {
        ret = PTR_ERR(my_device->device); 
        goto fail_device_create;
    }

    ret = device_create_file(my_device->device, &dev_attr_key);
    if (ret < 0) {
        goto fail_create_file;
    }

    ret = device_create_file(my_device->device, &dev_attr_status);
    if (ret < 0) {
        goto fail_create_file; 
    }

    printk(KERN_INFO "%s driver initialized!\n", DEVICE_NAME_CT); 
    return 0; 

// Error handling paths and driver exit
fail_create_file:
    device_destroy(my_device->dev_class, my_device->dev_number);
fail_device_create:
    class_destroy(my_device->dev_class);
fail_class_create:
    cdev_del(&my_device->cdev);
fail_cdev_add:
    unregister_chrdev_region(my_device->dev_number, 1);
fail_alloc:
    kfree(my_device); 
    return ret; 
}

static void __exit text_driver_exit(void) {
    device_remove_file(my_device->device, &dev_attr_status);
    device_remove_file(my_device->device, &dev_attr_key);
    device_destroy(my_device->dev_class, my_device->dev_number);
    class_destroy(my_device->dev_class);
    unregister_chrdev_region(my_device->dev_number, 1);
    cdev_del(&my_device->cdev);
    kfree(my_device);
    printk(KERN_INFO "%s driver removed!\n", DEVICE_NAME_CT); 
}

module_init(text_driver_init);
module_exit(text_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name"); 
MODULE_DESCRIPTION("Sample character device driver with sysfs"); 
MODULE_VERSION("1.0");
