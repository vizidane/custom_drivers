#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>

#define MEM_SIZE_MAX_PCDEV1 1024
#define MEM_SIZE_MAX_PCDEV2 512
#define MEM_SIZE_MAX_PCDEV3 1024
#define MEM_SIZE_MAX_PCDEV4 512
#define NO_OF_DEVICES 4
/*permission codes*/
#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR   0x11

/*
to see system trace : using strace
to write file from one to another:dd if=pcd_n.c of=/dev/pcdev_1 
				  dd write only 512 bytes at a time.
to combine both :
strace dd if=pcd_n.c of=/dev/pcdev_1 



*/


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


/*private data structure*/
struct pcdev_private_data
{
	char *buffer;
	unsigned size;
	const char *serial_number;
	int perm;
	struct cdev cdev;
};

/*driver prvate data structure*/
struct pcdrv_private_data
{
	int total_devices;
	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
	dev_t device_number; //hold the device number
	struct class *class_pcd;
	struct device *device_pcd;
};

struct pcdrv_private_data pcdrv_data=
{
	.total_devices = NO_OF_DEVICES,
	.pcdev_data = 
	{
		[0] = {
			.buffer = device_buffer_pcdev1,
			.size = MEM_SIZE_MAX_PCDEV1,
			.serial_number = "PCDEV1-123",
			.perm = RDONLY/*RDONLY*/

		},
		[1] = {
			.buffer = device_buffer_pcdev2,
			.size = MEM_SIZE_MAX_PCDEV2,
			.serial_number = "PCDEV2-123",
			.perm = WRONLY/*WRONLY*/
		
		},
		[2] = {
			.buffer = device_buffer_pcdev3,
			.size = MEM_SIZE_MAX_PCDEV3,
			.serial_number = "PCDEV3-123",
			.perm = RDWR/*RDWR*/
		
		},
		[3] = {
			.buffer = device_buffer_pcdev4,
			.size = MEM_SIZE_MAX_PCDEV4,
			.serial_number = "PCDEV4-123",
			.perm = RDWR/*RDWR*/
		
		},

	}
};

/*cdev variable*/
struct cdev pcd_cdev;



loff_t pcd_lseek (struct file *filp, loff_t offset, int whence)
{

        loff_t temp;

        struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;

        int max_size = pcdev_data->size;

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

	int max_size = pcdev_data->size;

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

	int max_size = pcdev_data->size;

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

	ret = check_permission(pcdev_data->perm, filp->f_mode);


	(!ret)?pr_info("open successful\n") : pr_info("open unsuccessful\n");

	return ret;
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


static int __init pcd_driver_init(void)
{

	int ret;
	int i;
	/*1.dynamic allocate the device number*/
	ret = alloc_chrdev_region(&pcdrv_data.device_number, 0, NO_OF_DEVICES, "pcdevs");	
	if (ret < 0 )
	{
		pr_err("calloc chrdev fail\n");
		goto out;
	}

	/*4.create device class under /sys/class*/
	pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_err("class creation fail\n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		goto unreg_chardev;
	}

	for(i = 0; i < NO_OF_DEVICES; i++)
	{	
		pr_info("device number <major>:<minor> = %d:%d\n",MAJOR(pcdrv_data.device_number + i),MINOR(pcdrv_data.device_number + i));

		/*2. initilize the cdev structure with fops*/
		cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);

		/*3.register cdev structure with VFS*/
		pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
		ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number + i, 1);	
		if(ret < 0 )
			goto cdev_del;


		/*5. populate the sysfs with device information*/
		pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, pcdrv_data.device_number + i, NULL,"pcdev_%d",i+1);
		if(IS_ERR(pcdrv_data.device_pcd))
		{
			pr_err("device creation fail\n");
			ret = PTR_ERR(pcdrv_data.device_pcd);
			goto class_del;
		}

	}

	

	pr_info("module init was successful\n");

	return 0;

cdev_del:
class_del:
	for(;i >= 0; --i)
	{
		device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);
unreg_chardev:
	unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);
out:
	return ret;


}

static void __exit pcd_driver_cleanup(void)
{
	int i;
	for ( i = 0; i < NO_OF_DEVICES; i++)
	{
		device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);
	unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
	pr_info("device unload");

}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("QUANG");
MODULE_DESCRIPTION("a pseudo driver that control  n device");


