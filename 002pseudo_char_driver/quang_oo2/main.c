#include<linux/module.h>
#include<linux/fs.h>

#define DEV_MEM_SIZE 512

dev_t device_number //hold the device number

char device_buffer[DEV_MEM_SIZE]

static int __init pcd_driver_init(void)
{
	/*dynamic allocate the device number*/
	alloc_chrdev_region(&device_number, 0, 1, "pcd");
	return 0;
}

static void __exit pcd_driver_cleanup(void)
{
	
}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);
