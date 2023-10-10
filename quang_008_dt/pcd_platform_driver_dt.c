#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include<linux/platform_device.h> 
#include "platform.h"
#include<linux/slab.h>
#include<linux/mod_devicetable.h>
#include<linux/of.h>
#include<linux/of_device.h>

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


enum pcdev_names
{
	PCDEVA1X = 0,
	PCDEVB1X,
	PCDEVC1X,
	PCDEVD1X
};

struct of_device_id	org_pcdev_dt_match[]=
{
	{ .compatible = "pcdev-A1x", .data = (void*)PCDEVA1X },
	{ .compatible = "pcdev-B1x", .data = (void*)PCDEVB1X },
	{ .compatible = "pcdev-C1x", .data = (void*)PCDEVC1X },
	{ .compatible = "pcdev-D1x", .data = (void*)PCDEVD1X },
	{}
};

struct device_config 
{
	int config_item1;
	int config_item2;
};

struct device_config pcdev_config[]=
{
	[PCDEVA1X] = {
		.config_item1 = 10,
		.config_item2 = 20
	},
	[PCDEVB1X] = {
		.config_item1 = 11,
		.config_item2 = 21
	},
	[PCDEVC1X] = {
		.config_item1 = 12,
		.config_item2 = 22
	},
	[PCDEVD1X] = {
		.config_item1 = 13,
		.config_item2 = 23
	}
};



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

    loff_t temp;

   struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;

   int max_size = pcdev_data->pdata.size;

	pr_info("lseek requested\n");
	pr_info("current file possition is %lld/n", filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if((offset > max_size) || (offset < 0))
				return -EINVAL;
			 filp->f_pos = offset;
			 break;
		case SEEK_CUR:
			 temp = filp->f_pos + offset;
			 if((temp > max_size)||(temp < 0))
				 return -EINVAL;
			 filp->f_pos = temp;
			 break;
		case SEEK_END:
			 temp = filp->f_pos + max_size;
			 if((temp > max_size)||(temp < 0))
				 return -EINVAL;
			 filp->f_pos = temp;
			 break;
		default:
			 return -EINVAL; 
	}
	pr_info("new value of file possition is %lld/n", filp->f_pos);

	return filp->f_pos;
	
}

/*read dont have inode to get device private data, hence device private data need to take over from open method*/
ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;

	int max_size = pcdev_data->pdata.size;

	pr_info("read requested for %zu bytes\n", count);
	pr_info("current file position = %lld\n", *f_pos);

	

	/*adjust the count*/
	if((*f_pos + count) > max_size)
	{
		count = max_size - *f_pos;	
	}

	/*copy to user*/
	if(copy_to_user(buff, pcdev_data->buffer + (*f_pos),count))//1st param: buffer to user, 2nd: source buffer, 3nd: number of byte
	{
		return EFAULT;
	}

	/*update the current file position*/
	*f_pos += count;
	pr_info("number of bytes has successfully read : %zu\n", count);
	pr_info("updated file position : %lld\n", *f_pos);/*long long int*/

	/*return number of byte which has successfully read*/
	return count;

	pr_info("read requested for %zu bytes\n",count);

	return 0;
}

ssize_t pcd_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{

	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->pdata.size;

	pr_info("write requested for %zu bytes\n",count);
	pr_info("current file position = %lld\n", *f_pos);

	/*adjust the count*/
	if((*f_pos + count) > max_size)
	{
		count = max_size - *f_pos;	
	}

	if (!count)
	{
		pr_err("no memory left\n");
		return -ENOMEM;
	}
	/*copy from user*/
	if(copy_from_user(pcdev_data->buffer,buff,count))//1st param: destination, 2nd: source buffer, 3nd: number of byte
	{
		return EFAULT;
	}

	/*update the current file position*/
	*f_pos += count;
	pr_info("number of bytes has successfully write : %zu\n", count);
	pr_info("updated file position : %lld\n", *f_pos);/*long long int*/

	/*return number of byte which has successfully write*/

	return count;

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
	int minor_n;
	int ret;
	struct pcdev_private_data *pcdev_data;

	pcdev_data = container_of(inode->i_cdev,struct pcdev_private_data, cdev);


	/*step 1 find out which device file to open by user attemp space*/
	minor_n = MINOR(inode->i_rdev);
	pr_info("minor access = %d \n",minor_n);
	
	/*step 2 get device private data structure*/
	/*to supply device private data to other methods of the drivers*/
	filp->private_data = pcdev_data;


	/*check permission */

	ret = check_permission(pcdev_data->pdata.perm, filp->f_mode);


	(!ret)?pr_info("open successful\n") : pr_info("open unsuccessful\n");

	return ret;
}


int pcd_release (struct inode *inode, struct file *filp)
{
	pr_info("close was successful\n");
	return 0;	
}



struct pcdev_platform_data* pcdev_get_platdata_from_dt(struct device *dev)
{
	struct pcdev_platform_data *pdata;

	pdata = devm_kzalloc(dev,sizeof(*pdata),GFP_KERNEL);

	if(!pdata)
	{
		/*can not return -ENOMEM since the return is of pointer type, we need to convert
		-ENOMEM in form of pointer*/
		dev_info(dev,"can not allocate memory\n");
		return ERR_PTR(-ENOMEM);
	}


	struct device_node *dev_node = dev->of_node;
	if(!dev_node)
	{
		/*this probe happen did not because of device tree node*/
		return NULL;
	} 

	if(of_property_read_string(dev->of_node,"org,device-serial-num", &pdata->serial_number))
	{
		dev_info(dev,"missing serial number properties\n");
		return ERR_PTR(-EINVAL);
	}

	if(of_property_read_u32(dev->of_node,"org,size", &pdata->size))
	{
		dev_info(dev,"missing size properties\n");
		return ERR_PTR(-EINVAL);
	}

	if(of_property_read_u32(dev->of_node,"org,perm", &pdata->perm))
	{
		dev_info(dev,"missing permission properties\n");
		return ERR_PTR(-EINVAL);
	}

	return pdata;
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
	
	/*when probe function call, if probe faile, really_probe function will free resource in case of error.
	really_probe -> your probe function
	*/
	int ret;

	struct pcdev_private_data *dev_data;
	
	struct pcdev_platform_data *pdata;

	const struct of_device_id* match;

	int driver_data;
	struct device *dev = &pdev->dev;

	dev_info(dev,"a device was detected\n");
#if 1
	/*match will always NULL if LINUX doesn't support device tree(CONFIG_OF is off)*/
	match = of_match_device(of_match_ptr(org_pcdev_dt_match), dev);

	if (match)/*probe happened because of device tree*/
	{
		driver_data = (int)match->data;
		/*first check if we get the device from dt*/

		pdata = pcdev_get_platdata_from_dt(dev);

		if(IS_ERR(pdata))
		{
			return PTR_ERR(pdata);
		}

	}

	else /*probe happened because of device setup code*/
	{
		/*check for device from device setup code*/

		/* 
		step 1. get the platform data, if  platform data not available, we can not proceed
		*/
		/*solution1
		pdata = (struct pcdev_platform_data*)pdev->dev.platform_data;
		*/
		/*solution2*/
		pdata = (struct pcdev_platform_data*)dev_get_platdata(dev);
		if (!pdata)
		{
			dev_info(dev,"no platform data available\n");
			ret = -EINVAL;
			return ret;
		}
		driver_data = pdev->id_entry->driver_data;
		dev_info(dev,"device id = %s, config1 = %d, config2 = %d \n",pdev->id_entry->name, pcdev_config[driver_data].config_item1,pcdev_config[pdev->id_entry->driver_data].config_item2);

	}

	/*
	step 2. Dynamically allocate memory for the device private data
		kzalloc flag  : GFP_KERNEL : allocate normal kernel ram, may sleep
			        GFP_ATOMIC : allocate will not sleep, ( use in emergency pool like interrupt)
	*/
	dev_data = kzalloc(sizeof(*dev_data),GFP_KERNEL);
	if (!dev_data)
	{
		dev_info(dev,"cannot allocate memory");
		ret = -ENOMEM;
		return ret;
	}


	/*copy driver data to platform_device, so we can access driver data (local) in another function */
	dev_set_drvdata(&pdev->dev, dev_data);

		 
	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

	dev_info(dev,"device serial number = %s/n",dev_data->pdata.serial_number);
	dev_info(dev,"device size = %d/n",dev_data->pdata.size);
	dev_info(dev,"device permission = %d/n",dev_data->pdata.perm);
	

	/*step3. Dynamically allocate memory for the device buffer using size information from the platform data*/
	dev_data->buffer = kzalloc(dev_data->pdata.size,GFP_KERNEL);
	if(!dev_data)
	{
		dev_info(dev,"can not allocate memory for dev_data buffer\n");
		ret = -ENOMEM;
		return ret;
	}

	/*step 4. get the device number*/
	dev_data->dev_num = pcdrv_data.device_num_base + pcdrv_data.total_devices;

	/*step 5. Do cdev init and cdev add*/
	dev_info(dev,"device number <major>:<minor> = %d:%d\n",MAJOR(dev_data->dev_num),MINOR(dev_data->dev_num));

	/*5.1. initilize the cdev structure with fops*/
	cdev_init(&dev_data->cdev, &pcd_fops);

	/*5.2 register cdev structure with VFS*/
	dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);	
	if(ret < 0 )
	{
		dev_info(dev,"cdev add fail");
		return ret;
	
	}
		

	/*step6 create device file for the detected platform device*/

	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, dev, dev_data->dev_num, NULL,"pcdev_%d",pcdrv_data.total_devices);
	if(IS_ERR(pcdrv_data.device_pcd))
	{
		dev_info(dev,"device creation fail\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		cdev_del(&dev_data->cdev);
		return ret;
	}

	
	
	pcdrv_data.total_devices+=1;


	dev_info(dev,"the probe was successfully, number of device = %d\n",pcdrv_data.total_devices);
	 
#endif

	return 0;

}

int pcd_platform_driver_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev,"device was removed\n");
	struct device *dev = &pdev->dev;
#if 1
	struct pcdev_private_data *dev_data = (struct pcdev_private_data*)dev_get_drvdata(&pdev->dev);
	
	/*remove a device that was create with device_create*/
	device_destroy(pcdrv_data.class_pcd,dev_data->dev_num);

	/*remove a cdev entry from the system*/
	 cdev_del(&dev_data->cdev);

	/*free the memory held by the device*/
	 kfree(dev_data->buffer);
	 kfree(dev_data);
	 dev_info(dev,"a device was removed\n");
	 pcdrv_data.total_devices-=1;
#endif
	return 0;

}


struct platform_device_id pcdevs_id[]=
{
	{
		.name = "pcdev-A1x",
		/*
		.driver_data is the field where driver need to update with device configuration
			the driver can extract the data on the probe function
		->drive_data store the index of configugarion array.
		*/
		.driver_data = PCDEVA1X
	},
	{
		.name = "pcdev-B1x",
		.driver_data = PCDEVB1X
	},
	{
		.name = "pcdev-C1x",
		.driver_data = PCDEVC1X
	},
	{
		.name = "pcdev-D1x",
		.driver_data = PCDEVD1X
	},
	{}//to let processor api know where is the end(this is indicator end of array)
} ;




struct platform_driver pcd_platform_driver = 
{
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	/*when id_table is used, .name is no longer need for device-driver matching
	  refer to platform_match function
	*/
	.driver ={
		.name = "pseudo-char-device",
		/*this is in case CONFIG_OF is off (mean system dont use dt)
		then .of_match_table should be null
		*/
		.of_match_table = of_match_ptr(org_pcdev_dt_match)
	},
	.id_table = pcdevs_id


};
/*we can not replace pr_info with dev_info in init function, because they dont have access for device*/

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


/*we can not replace pr_info with dev_info in init function, because they dont have access for device*/
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


