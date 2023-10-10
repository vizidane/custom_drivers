#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
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
__used __section(__versions) = {
	{ 0x68cd8b7f, "module_layout" },
	{ 0xd1230dea, "class_destroy" },
	{ 0x922b4e4f, "platform_driver_unregister" },
	{ 0xccd6a833, "__platform_driver_register" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x29329b84, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xaf792eaf, "kmalloc_caches" },
	{ 0xa4b101f4, "device_create" },
	{ 0x2b1625d2, "cdev_add" },
	{ 0x8cf147a, "cdev_init" },
	{ 0x2d6fcc06, "__kmalloc" },
	{ 0x607fa1c2, "kmem_cache_alloc_trace" },
	{ 0x2161655e, "of_device_get_match_data" },
	{ 0xf8940ddd, "of_property_read_variable_u32_array" },
	{ 0xf7425bed, "of_property_read_string" },
	{ 0x88e83bf4, "devm_kmalloc" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0x37a0cba, "kfree" },
	{ 0x40f44669, "cdev_del" },
	{ 0x322288d8, "device_destroy" },
	{ 0x452fc3ad, "_dev_info" },
	{ 0x5f754e5a, "memset" },
	{ 0x2cfde9a2, "warn_slowpath_fmt" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xc5850110, "printk" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "92CDB49E7B561F09342F4C0");
