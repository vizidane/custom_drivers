#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include<linux/platform_device.h> 
#include "platform.h"
#include<linux/slab.h>

#define MEM_SIZE_MAX_PCDEV1 1024
#define MEM_SIZE_MAX_PCDEV2 512
#define MEM_SIZE_MAX_PCDEV3 1024
#define MEM_SIZE_MAX_PCDEV4 512
#define NO_OF_DEVICES 4
/*permission codes*/
#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR   0x11
#define MAX_DEVICES 10

/*
to see system trace : using strace
to write file from one to another:dd if=pcd_n.c of=/dev/pcdev_1 
				  dd write only 512 bytes at a time.
to combine both :
strace dd if=pcd_n.c of=/dev/pcdev_1 



*/

/*
device private data structure
device data will be created dynamically whenever probe function get call (platform device is detected)
*/

struct pcdev_private_data
{
	struct pcdev_platform_data pdata;
	char *buffer;
	dev_t dev_num;/*hold device number*/
	struct cdev cdev;
};

/*
driver private data structure
*/
struct pcdrv_private_data
{
	int total_devices;
	dev_t device_num_base; 
	struct class *class_pcd;
	struct device *device_pcd;

};	

struct pcdrv_private_data pcdrv_data;

#undef pr_fmt /*since pr_fmt already defined in prink.h, we want to overwrite it, so we need to undef the already defined.*/

#define pr_fmt(fmt) "%s :" fmt, __func__

/*not working macro
#undef container_of
#undef offset_of
#define offset_of(ELEMENT,TYPE) (&((TYPE*)0)->ELEMENT)
#define container_of(PTR,TYPE,ELEMENT) (TYPE*)(PTR - offset_of(ELEMENT,TYPE))
*/

#undef container_of
#define container_of(PTR,TYPE,ELEMENT) ((TYPE*)((char*)PTR - (char*)(&((TYPE*)0) -> ELEMENT )))




char device_buffer_pcdev1[MEM_SIZE_MAX_PCDEV1];
char device_buffer_pcdev2[MEM_SIZE_MAX_PCDEV2];
char device_buffer_pcdev3[MEM_SIZE_MAX_PCDEV3];
char device_buffer_pcdev4[MEM_SIZE_MAX_PCDEV4];









loff_t pcd_lseek (struct file *filp, loff_t offset, int whence)
{

     return 0;
	
}

/*read dont have inode to get device private data, hence device private data need to take over from open method*/
ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t pcd_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{

	return -ENOMEM;

}

/*
	user using macro open(path,flag) flag = O_RDONLY, O_WRONLY, O_RDWR
	those flag will be get from file->f_mode  (FMODE_READ)
*/
int check_permission(int dev_perm, int acc_mode)
{
	
	if ( RDWR == dev_perm )
		return 0;
	if ((dev_perm == RDONLY) &&( ( acc_mode & FMODE_READ ) && !(acc_mode & FMODE_WRITE) ))
		return 0;
	if ((dev_perm == WRONLY) && ( (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)))
		return 0;
	return -EPERM;	 
}

/*
this function must know which device file the use attemp to open. because we have 4 devices
every device have 1 inode. 
->can use inode to find the corresponding device.(dev_t i_rdev)
->find device private data structure, its very important because this is where all the device specification 
are kept.
	inode -> cdev
	since cdev is member of device_private_data structure, we can use cdev to gec address of 
	this private_data_structure
	->this can achieve by using container_of macro


*/

/*
once the device is open, the struct file kernel object is created, and the same kernel object is passed
to different operation method.
*/
int pcd_open(struct inode *inode, struct file *filp)
{
	return 0;
}


int pcd_release (struct inode *inode, struct file *filp)
{
	pr_info("close was successful\n");
	return 0;	
}



/*file operation*/
struct file_operations pcd_fops=
{
	.open  = pcd_open,
	.write = pcd_write,
	.read  = pcd_read,
	.llseek = pcd_lseek,
	.release = pcd_release,
	.owner = THIS_MODULE
};

int pcd_platform_driver_probe(struct platform_device *pdev)
{
	int ret;

	struct pcdev_private_data *dev_data;
	
	struct pcdev_platform_data *pdata;

	pr_info("a device was detected\n");

	/* 
	step 1. get the platform data, if  platform data not available, we can not proceed
	*/
	/*solution1
	pdata = (struct pcdev_platform_data*)pdev->dev.platform_data;
	*/
	/*solution2*/
	pdata = (struct pcdev_platform_data*)dev_get_platdata(&pdev->dev);
	if (!pdata)
	{
		pr_info("no platform data available\n");
		ret = -EINVAL;
		goto out;
	}
	/*
	step 2. Dynamically allocate memory for the device private data
		kzalloc flag  : GFP_KERNEL : allocate normal kernel ram, may sleep
			        GFP_ATOMIC : allocate will not sleep, ( use in emergency pool like interrupt)
	*/
	dev_data = kzalloc(sizeof(*dev_data),GFP_KERNEL);
	if (!dev_data)
	{
		pr_info("cannot allocate memory");
		ret = -ENOMEM;
		goto out;
	}


	/*copy driver data to platform_device, so we can access driver data (local) in another function */
	dev_set_drvdata(&pdev->dev, dev_data);

		 
	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

	pr_info("device serial number = %s/n",dev_data->pdata.serial_number);
	pr_info("device size = %d/n",dev_data->pdata.size);
	pr_info("device permission = %d/n",dev_data->pdata.perm);
	

	/*step3. Dynamically allocate memory for the device buffer using size information from the platform data*/
	dev_data->buffer = kzalloc(dev_data->pdata.size,GFP_KERNEL);
	if(!dev_data)
	{
		pr_info("can not allocate memory for dev_data buffer\n");
		ret = -ENOMEM;
		goto dev_data_free;
	}

	/*step 4. get the device number*/
	dev_data->dev_num = pcdrv_data.device_num_base + pdev->id;

	/*step 5. Do cdev init and cdev add*/
	pr_info("device number <major>:<minor> = %d:%d\n",MAJOR(dev_data->dev_num),MINOR(dev_data->dev_num));

	/*5.1. initilize the cdev structure with fops*/
	cdev_init(&dev_data->cdev, &pcd_fops);

	/*5.2 register cdev structure with VFS*/
	dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);	
	if(ret < 0 )
	{
		pr_info("cdev add fail");
		goto buffer_free;
	
	}
		

	/*step6 create device file for the detected platform device*/

	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, dev_data->dev_num, NULL,"pcdev_%d",pdev->id);
	if(IS_ERR(pcdrv_data.device_pcd))
	{
		pr_err("device creation fail\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		goto cdev_delete;
	}

	pcdrv_data.total_devices+=1;

	/*Error handling*/
	pr_info("the probe was successfully, number of device = %d\n",pcdrv_data.total_devices);
	 

	return 0;

cdev_delete:
	cdev_del(&dev_data->cdev);
buffer_free:
	kfree(dev_data->buffer);

dev_data_free:
	kfree(dev_data);

out:
	pr_info("device probe fail\n");
	return ret;
}

int pcd_platform_driver_remove(struct platform_device *pdev)
{
	struct pcdev_private_data *dev_data = (struct pcdev_private_data*)dev_get_drvdata(&pdev->dev);
	
	/*remove a device that was create with device_create*/
	device_destroy(pcdrv_data.class_pcd,dev_data->dev_num);

	/*remove a cdev entry from the system*/
	 cdev_del(&dev_data->cdev);

	/*free the memory held by the device*/
	 kfree(dev_data->buffer);
	 kfree(dev_data);
	 pr_info("a device was removed\n");
	 pcdrv_data.total_devices-=1;
	return 0;

}

struct platform_driver pcd_platform_driver = 
{
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.driver ={
		.name = "pseudo-char-device"
	}

};


static int __init pcd_platform_driver_init(void)
{
	int ret;
	/*1. dynamically allocate a device number for MAX_DEVICES*/

	ret = alloc_chrdev_region(&pcdrv_data.device_num_base, 0, MAX_DEVICES, "pcdevs");	
	if (ret < 0 )
	{
		pr_err("calloc chrdev fail\n");
		return ret;
	}

	/*2. Create device class under sys/class*/
	pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_err("class creation fail\n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
		return ret;
	}

	/*3. Regispcdrv_datater a platform device*/
	platform_driver_register(&pcd_platform_driver);
	pr_info("pcd platform driver load\n");
	return 0;

}



static void __exit pcd_platform_driver_cleanup(void)
{
	/*unregister platform driver*/
	platform_driver_unregister(&pcd_platform_driver);
	/*class destroy*/
	class_destroy(pcdrv_data.class_pcd);
	/*unregister chrdev_region*/
	unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);

	pr_info("pcd platform driver unload\n");

}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("QUANG");
MODULE_DESCRIPTION("a pseudo driver that control  n platform device");


