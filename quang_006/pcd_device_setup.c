#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"

#undef pr_fmt /*since pr_fmt already defined in prink.h, we want to overwrite it, so we need to undef the already defined.*/

#define pr_fmt(fmt) "%s :" fmt, __func__

/*
	to test:
	cd /sys/devices/platform/
*/


/*
	device private data in platform device context is device platform data


*/

void pcdev_release(struct device *dev)
{
	pr_info("device release\n");
}

/*create 4 platform data*/
struct pcdev_platform_data pcdev_pdata[] =
{
	[0]={
		.size = 512,
		.perm = RDWR,
		.serial_number = "PCDEVABC1111"
	},
	[1]={
		.size = 1024,
		.perm = RDWR, 
		.serial_number = "PCDEVXYZ2222"
	},
	[2]={
		.size = 128,
		.perm = RDONLY,
		.serial_number = "PCDEVABC3333"
	},
	[3]={
		.size = 32,
		.perm = WRONLY,
		.serial_number = "PCDEVXYZ4444"
	}
};

/*
	create platform device
	for device, we need to implement the release function
		release function:
			The release function to free the device after al references have gone away
			(this is applicable if you using some dynamic allocate memory)
			This should be set by the allocator of the device


*/
struct platform_device platform_pcdev_1 = 
{
	.name = "pcdev-A1x",
	.id = 0,
	.dev = {
		.platform_data = &pcdev_pdata[0],
		.release = pcdev_release
	}
};

struct platform_device platform_pcdev_2 =
{
	.name = "pcdev-B1x",
	.id = 1,
	.dev = {
		.platform_data = &pcdev_pdata[1],
		.release = pcdev_release
	}
};

struct platform_device platform_pcdev_3 = 
{
	.name = "pcdev-C1x",
	.id = 2,
	.dev = {
		.platform_data = &pcdev_pdata[2],
		.release = pcdev_release
	}
};

struct platform_device platform_pcdev_4 =
{
	.name = "pcdev-D1x",
	.id = 3,
	.dev = {
		.platform_data = &pcdev_pdata[3],
		.release = pcdev_release
	}
};

struct platform_device *platform_pcdevs[]=
{
	&platform_pcdev_1,
	&platform_pcdev_2,
	&platform_pcdev_3,
	&platform_pcdev_4

};

static int __init pcdev_platform_init(void)
{
	/*register platform device*/
	/*solution1*/
	//platform_device_register(&platform_pcdev_1);
	//platform_device_register(&platform_pcdev_2);
	/*solution2 - using platform_add_devices macro*/
	platform_add_devices(platform_pcdevs,ARRAY_SIZE(platform_pcdevs));
	pr_info("device inserted\n");


	 return 0;

}

static void __exit pcdev_platform_exit(void)
{
	platform_device_unregister(&platform_pcdev_1);
	platform_device_unregister(&platform_pcdev_2);
	platform_device_unregister(&platform_pcdev_3);
	platform_device_unregister(&platform_pcdev_4);
	pr_info("device_removed\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("QUANG");
MODULE_DESCRIPTION("module that register platform device");