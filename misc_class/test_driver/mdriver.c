#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>		// k[m|z]alloc(), k[z]free(), ...
#include <linux/mm.h>		// kvmalloc()
#include <linux/fs.h>		// the fops
#include <linux/sched.h>	// get_task_comm()

// copy_[to|from]_user()
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 11, 0)
#include <linux/uaccess.h>
#else
#include <asm/uaccess.h>
#endif

#define OURMODNAME   "miscdrv_rdwr"
MODULE_AUTHOR("Prince");
MODULE_DESCRIPTION("Test driver");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

struct driver_context {
    struct device *dev;
    int tx, rx, err, word;
    u32 config1, config2;
    u64 config3;
#define MAXBYTES    128
    char ourMessage[MAXBYTES];
};

static struct driver_context *context;

static int open_testdriver(struct inode *inode, struct file *fp) {
    struct device *dev = context->dev;
    char *buf = kzalloc(PATH_MAX, GFP_KERNEL);

    if(unlikely(!buf)) return -ENOMEM;

    dev_info(dev, " opening %s now, wrt open file: f_flags = 0x%x\n",
                file_path(fp, buf, PATH_MAX), fp->f_flags);
    kfree(buf);

    return nonseekable_open(inode, fp);
}

static ssize_t read_testdriver(struct file *fp, char __user *ubuf, size_t count, loff_t *off) {
    int ret = count;
    int messagelen = strnlen(context->ourMessage, MAXBYTES);

    struct device *dev = context->dev;
    char tasknm[TASK_COMM_LEN];

    dev_info(dev, "%s wants to read %zu bytes\n", get_task_comm(tasknm, current), count);

    ret = -EINVAL;
    if (count < MAXBYTES) {
        dev_warn(dev, "request # of bytes %zu is  < req. size %d , abort read\n", count, MAXBYTES);
        goto out_notok;
    }

    if (messagelen <= 0) {
        dev_warn(dev, "whoops, something's wrong, the 'secret' isn't"
			" available..; aborting read\n");
		goto out_notok;
    }

    ret = -EFAULT;
    if (copy_to_user(ubuf, context->ourMessage, messagelen)) {
        dev_warn(dev, "copy to user failed\n");
        goto out_notok;
    }

    ret = messagelen;

    context->tx += messagelen;
    dev_info(dev, " %d bytes read, returning... (stats: tx=%d, rx=%d)\n",
		messagelen, context->tx, context->rx);

out_notok:

    return ret;
}

static ssize_t write_testdriver(struct file *fp, const char __user *ubuf,
                                size_t count, loff_t *off) {

    int ret = count;
    void *kbuf = NULL;
    struct device *dev = context->dev;
    char tasknm[TASK_COMM_LEN];

    if(unlikely(count > MAXBYTES)) {
        dev_warn(dev, "count %zu exceeds max # of bytes allowed, "
			"aborting write\n", count);
		goto out_nomem;
    }
    dev_info(dev, "%s wants to write %zu bytes\n", get_task_comm(tasknm, current), count);

    ret = -ENOMEM;
    kbuf = kvmalloc(count, GFP_KERNEL);
	if (unlikely(!kbuf))
		goto out_nomem;
	memset(kbuf, 0, count);

    ret = -EFAULT;
	if (copy_from_user(kbuf, ubuf, count)) {
		dev_warn(dev, "copy_from_user() failed\n");
		goto out_cfu;
	}

    strscpy(context->ourMessage, kbuf, (count > MAXBYTES ? MAXBYTES : count));

    context->rx += count;

    ret = count;
    dev_info(dev, " %zu bytes written, returning... (stats: tx=%d, rx=%d)\n",
		count, context->tx, context->rx);
 out_cfu:
	kvfree(kbuf);
 out_nomem:
	return ret;
}

static int close_testdriver(struct inode *inode, struct file *fp) {
    struct device *dev = context->dev;
    char *buf = kzalloc(PATH_MAX, GFP_KERNEL);

    if(unlikely(!buf)) return -ENOMEM;

    dev_info(dev, " filename: \"%s\"\n", file_path(fp, buf, PATH_MAX));
    kfree(buf);

    return 0;
}

static const struct file_operations testdriver_fops = {
    .open = open_testdriver,
    .read = read_testdriver,
    .write = write_testdriver,
    .llseek = no_llseek,
    .release = close_testdriver,
};

static struct miscdevice testdriver = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "test_driver",
    .mode = 0666,
    .fops = &testdriver_fops,
};

static int __init testdriver_init(void) {
    
    int ret = 0;
    struct device *dev;

    ret = misc_register(&testdriver);
    if (ret) {
        pr_notice("%s: misc device reg. failed\n", OURMODNAME);
        return ret;
    }

    dev = testdriver.this_device;
    pr_info("Test driver with major #10 reg. minor# = %d\n" " dev node is /dev/%s\n", testdriver.minor, testdriver.name);

    context = devm_kzalloc(dev, sizeof(struct driver_context), GFP_KERNEL);
    if (unlikely(!context)) return -ENOMEM;

    context->dev = dev;
    strscpy(context->ourMessage, "Hello Prince", 13);
    dev_dbg(context->dev, "A sample print via dev_dbg(): driver is initialized\n");

    return 0;
}

static void __exit testdriver_exit(void) {
    misc_deregister(&testdriver);
    pr_info("Driver is de-registered.\n");
}

module_init (testdriver_init);
module_exit(testdriver_exit);

