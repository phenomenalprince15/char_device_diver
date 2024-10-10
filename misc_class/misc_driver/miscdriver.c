#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>           /* fops, file data structures */
#include <linux/slab.h>


MODULE_AUTHOR("Prince");
MODULE_DESCRIPTION("Simple misc char driver");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

/*
open_driver_misc function
this open method ill get invoked by the kernel VFS
when the device file is opened we print some info.
POSIX standard requires open() to return the fd (file descriptor) on success
no_llseek tells the kenel that our device is not seek-able.

struct inode: inode object represents an object within the file system.
This describes how the VFS can manipulate an inode in your filesystem.

struct file: It represents a file

*/
static int open_driver_misc(struct inode *inode, struct file *fp) {
    char *buf = kzalloc(PATH_MAX, GFP_KERNEL);

    if(unlikely(!buf)) return -ENOMEM;
    // PRINT_CTX();            /* displays process context info (or atomic) */

    pr_info("Opening \"%s\" nowl wrt open file: f_flags = 0x%x\n",
            file_path(fp, buf, PATH_MAX), fp->f_flags);

    kfree(buf);
    return nonseekable_open(inode, fp);
}

/*
read and write method of driver take over the system calls.
read() and write() as per POSIX return no. of bytes read or written on success, 0 on EOF
-1 or -ve errno on failure
*/
static ssize_t read_driver_misc(struct file *fp, char __user *ubuf, size_t count, loff_t *off) {
    pr_info("to read %zd bytes\n", count);
    return count;
}

static ssize_t write_driver_misc(struct file *fp, const char __user *ubuf,
                                    size_t count, loff_t *off) {
    pr_info("to write %zd bytes\n", count);
    return count;
}

static int close_driver_misc(struct inode *inode, struct file *fp) {
    char *buf = kzalloc(PATH_MAX, GFP_KERNEL);

    if(unlikely(!buf)) return -ENOMEM;

    pr_info("Closing \"%s\"\n", file_path(fp, buf, PATH_MAX));
    kfree(buf);

    return 0;
}

static const struct file_operations driver_miscdev_fops = {
    .open = open_driver_misc,           /* these are all functions we need to define */
    .read = read_driver_misc,
    .write = write_driver_misc,
    .release = close_driver_misc,
    .llseek = no_llseek,
};

/*
miscdevice is a structure present in linux included file as above.
Creating a miscdevice structure driver_miscdev
*/
static struct miscdevice driver_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,        /* kernel  dynamically assigns a free minor number */
    .name = "driver_miscdrv",
    .mode = 0666,
    .fops = &driver_miscdev_fops,          /* connect to this driver's functionality, it's a function that calls all file operations that e need to feines */
};

static int __init miscdriver_init(void) {
    int ret;
    struct device *dev;

    ret = misc_register(&driver_miscdev);       /* registering misc driver */
    if (ret != 0) {
        pr_notice("misc device registeration failed...\n");
        return ret;
    }

    /* Get the device pointer for this device created */
    dev = driver_miscdev.this_device;
    pr_info("LLKD misc driver (major # 10) registered, minor# = %d,"
		" dev node is /dev/%s\n", driver_miscdev.minor, driver_miscdev.name);

    dev_info(dev, "sample dev_info(): minor = %d\n", driver_miscdev.minor);

    return(0); // success
}

static void __exit miscdriver_exit(void)
{
	misc_deregister(&driver_miscdev);
	pr_info("LLKD misc driver deregistered, bye\n");
}

module_init(miscdriver_init);
module_exit(miscdriver_exit);
