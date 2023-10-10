#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>

#define DEV_MEM_SIZE 512

#undef pr_fmt /*since pr_fmt already defined in prink.h, we want to overwrite it, so we need to undef the already defined.*/

#define pr_fmt(fmt) "%s :" fmt, __func__

dev_t device_number; //hold the device number



char device_buffer[DEV_MEM_SIZE];

/*cdev variable*/
struct cdev pcd_cdev;

loff_t pcd_lseek (struct file *filp, loff_t offset, int whence)
{

        loff_t temp;
	pr_info("lseek requested\n");
	pr_info("current file possition is %lld/n", filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if((offset > DEV_MEM_SIZE) || (offset < 0))
				return -EINVAL;
			 filp->f_pos = offset;
			 break;
		case SEEK_CUR:
			 temp = filp->f_pos + offset;
			 if((temp > DEV_MEM_SIZE)||(temp < 0))
				 return -EINVAL;
			 filp->f_pos = temp;
			 break;
		case SEEK_END:
			 temp = filp->f_pos + DEV_MEM_SIZE;
			 if((temp > DEV_MEM_SIZE)||(temp < 0))
				 return -EINVAL;
			 filp->f_pos = temp;
			 break;
		default:
			 return -EINVAL; 
	}
	pr_info("new value of file possition is %lld/n", filp->f_pos);
	return filp->f_pos;
	
	
}

ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("read requested for %zu bytes\n", count);
	pr_info("current file position = %lld\n", *f_pos);

	/*adjust the count*/
	if((*f_pos + count) > DEV_MEM_SIZE)
	{
		count = DEV_MEM_SIZE - *f_pos;	
	}

	/*copy to user*/
	if(copy_to_user(buff, &device_buffer[*f_pos],count))//1st param: buffer to user, 2nd: source buffer, 3nd: number of byte
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
	pr_info("write requested for %zu bytes\n",count);
	pr_info("current file position = %lld\n", *f_pos);

	/*adjust the count*/
	if((*f_pos + count) > DEV_MEM_SIZE)
	{
		count = DEV_MEM_SIZE - *f_pos;	
	}

	if (!count)
	{
		return -ENOMEM;
	}
	/*copy from user*/
	if(copy_from_user(&device_buffer[*f_pos],buff,count))//1st param: destination, 2nd: source buffer, 3nd: number of byte
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

int pcd_open(struct inode *inode, struct file *filp)
{
	pr_info("open was successful\n");
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

struct class *class_pcd;
struct device *device_pcd;

static int __init pcd_driver_init(void)
{
	/*1.dynamic allocate the device number*/
	alloc_chrdev_region(&device_number, 0, 1, "pcd_devices");	
	pr_info("device number <major>:<minor> = %d:%d\n",MAJOR(device_number),MINOR(device_number));
	/*2. initilize the cdev structure with fops*/
	cdev_init(&pcd_cdev, &pcd_fops);

	/*3.register cdev structure with VFS*/
	pcd_cdev.owner = THIS_MODULE;
	cdev_add(&pcd_cdev, device_number, 1);	

	/*4.create dvice class under /sys/class*/
	class_pcd = class_create(THIS_MODULE, "pcd_class");

	/*5. populate the sysfs with device information*/
	device_pcd = device_create(class_pcd, NULL, device_number, NULL,"pcd");

	pr_info("module init was successful\n");

	return 0;
}

static void __exit pcd_driver_cleanup(void)
{
	
}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("QUANG");
MODULE_DESCRIPTION("a pseudo character device driver");


