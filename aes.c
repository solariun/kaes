#include <linux/init.h>
#include <linux/module.h>

int hello_init(void)
{
        printk(KERN_ALERT " I am inside kernel.\n");
        return 0;
}

int hello_exit(void)
{
        printk(KERN_ALERT " Leaving the kernel, bye.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("ECE");
MODULE_AUTHOR("Gustavo Campos");
MODULE_DESCRIPTION("This is a kernel hellow world scheleton");