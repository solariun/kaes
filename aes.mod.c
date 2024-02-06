#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif


static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x92997ed8, "_printk" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x63ac9375, "kmalloc_caches" },
	{ 0x45f84c31, "kmalloc_trace" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x4f2b3590, "cdev_init" },
	{ 0x12567b45, "cdev_add" },
	{ 0x23d20fbd, "__class_create" },
	{ 0x255a40b1, "device_create" },
	{ 0x7144d5bd, "device_create_file" },
	{ 0xedadcbe4, "device_destroy" },
	{ 0x48be46d6, "class_destroy" },
	{ 0x9d8130b9, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x37a0cba, "kfree" },
	{ 0x237ed3b6, "device_remove_file" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0xdcb764ad, "memset" },
	{ 0xf7038a43, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "3837177DA3FE7C88D4AE80E");
