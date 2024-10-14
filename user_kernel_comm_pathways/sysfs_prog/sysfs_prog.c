#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>

// copy_[to|from]_user()
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 11, 0)
#include <linux/uaccess.h>
#else
#include <asm/uaccess.h>
#endif

/* We use a mutex lock */
static DEFINE_MUTEX(mtx);

static int debug_level;		/* 'off' (0) by default ... */
static u32 gpressure;		/* our dummy 'pressure' value */

static struct platform_device *sysfs_platform;	/* Device structure */

MODULE_AUTHOR("Prince");
MODULE_DESCRIPTION("Simple sysfs program");
/*
 * We *require* the module to be released under GPL license (as well) to please
 * several core driver routines (like sysfs_create_group,
 * platform_device_register_simple, etc which are exported to GPL only (using
 * the EXPORT_SYMBOL_GPL() macro))
 */
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

#define OURMODNAME		"sysfs_simple_intf"
#define SYSFS_FILE1		sysfs_debug_level
#define SYSFS_FILE2		sysfs_pgoff
#define SYSFS_FILE3		sysfs_pressure

/*
In both show and store methods - buffer is a kernel space buffer (don't try copy from and to user stuff)
Interface for exporting device attributes
struct device_attribute {
    struct attribute attr;
    ssize_t (*show)(struct device *dev, struct device_attribute *attr, char *buf);
    ssize_t (*store) (struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
}
*/
// 1
static ssize_t sysfs_pressure_show(struct device *dev, 
                    struct device_attribute *attr, char *buf) {
    int n;
    if (mutex_lock_interruptible(&mtx)) return -ERESTARTSYS;

    pr_debug("In the show method: pressure=%u\n", gpressure);
    n = snprintf(buf, 25, "%u", gpressure);
    mutex_unlock(&mtx);

    return n;
}

static DEVICE_ATTR_RO(sysfs_pressure);

// 2 - PAGE_OFFSET value ; sysfs entry point for "show"
static ssize_t sysfs_pgoff_show(struct device *dev, struct device_attribute *attr, char *buf) {
    int n;

    if (mutex_lock_interruptible(&mtx)) return -ERESTARTSYS;

    pr_debug("In the show method: PAGE_OFFSET=0x%px\n", (void *)PAGE_OFFSET);
    n = snprintf(buf, 25, "0x%px", (void*)PAGE_OFFSET);
    mutex_unlock(&mtx);

    return n;
}

static DEVICE_ATTR_RO(sysfs_pgoff);

#define DEBUG_LEVEL_MIN     0
#define DEBUG_LEVEL_MAX     2

// debug level
static ssize_t sysfs_debug_level_show(struct device *dev, struct device_attribute *attr, char *buf) {
    int n;
    if (mutex_lock_interruptible(&mtx)) return -ERESTARTSYS;
    
    pr_debug("In the show name %s, debug_level %d\n", dev->kobj.name, debug_level);
    n = snprintf(buf, 25, "%d\n", debug_level);
    mutex_unlock(&mtx);

    return n;
}

// debug level store to write
static ssize_t sysfs_debug_level_store(struct device *dev, struct device_attribute *attr, 
                    const char *buf, size_t count) {
    int ret = (int)count, prev_dbglevel;
    if (mutex_lock_interruptible(&mtx))
		return -ERESTARTSYS;

	prev_dbglevel = debug_level;
	pr_debug("In the 'store' method:\ncount=%zu, buf=0x%px count=%zu\n"
	    "Buffer contents: \"%.*s\"\n", count, buf, count, (int)count, buf);
	if (count == 0 || count > 12) {
		ret = -EINVAL;
		goto out;
	}

	ret = kstrtoint(buf, 0, &debug_level);	/* update it! */
	if (ret)
		goto out;
	if (debug_level < DEBUG_LEVEL_MIN || debug_level > DEBUG_LEVEL_MAX) {
		pr_info("trying to set invalid value (%d) for debug_level\n"
			" [allowed range: %d-%d]; resetting to previous (%d)\n",
			debug_level, DEBUG_LEVEL_MIN, DEBUG_LEVEL_MAX, prev_dbglevel);
		debug_level = prev_dbglevel;
		ret = -EFAULT;
		goto out;
	}

	ret = count;
 out:
	mutex_unlock(&mtx);
	return ret;
}

/*
DEVICE_ATTR{_RW|RO|WO}() macro instantiates a struct device attribute dev_attr_<name>
*/
static DEVICE_ATTR_RW(SYSFS_FILE1); /* It's show/store callsbacks are above */

/*
 * From <linux/device.h>:
DEVICE_ATTR{_RW} helper interfaces (linux/device.h):
--snip--
#define DEVICE_ATTR_RW(_name) \
    struct device_attribute dev_attr_##_name = __ATTR_RW(_name)
#define __ATTR_RW(_name) __ATTR(_name, 0644, _name##_show, _name##_store)
--snip--
and in <linux/sysfs.h>:
#define __ATTR(_name, _mode, _show, _store) {              \
	.attr = {.name = __stringify(_name),               \
		.mode = VERIFY_OCTAL_PERMISSIONS(_mode) }, \
	.show   = _show,                                   \
	.store  = _store,                                  \
}
 */
static int __init sysfs_prog_init(void) {

    int stat = 0;

    if (unlikely(!IS_ENABLED(CONFIG_SYSFS))) {
        pr_warn("sysfs unsupported, Aborting...\n");
        return -EINVAL;
    }

#define PLAT_NAME   "simple_sysfs_prog"

    /* 0. Register a (dummy) platform device; required as we need a
	 * struct device *dev pointer to create the sysfs file with
	 * the device_create_file() API:
	 *  struct platform_device *platform_device_register_simple(
	 *		const char *name, int id,
	 *		const struct resource *res, unsigned int num);
	 */

    sysfs_platform = platform_device_register_simple(PLAT_NAME, -1, NULL, 0);
    if (IS_ERR(sysfs_platform)) {
        stat = PTR_ERR(sysfs_platform);
        pr_info("Error %d registering our platform device.\n", stat);
        goto out1;
    }

    /*
        &dev_attr_SYSFS_FILE1 is actually instantiated via static DEVICE_ATTR_RW (SYSFS_FILE1);
        This DEVICE_ATTR{_RW|RO|WO} () macro instantiates a struct device attribure data strcuture.
        dev_attr_XX (name) macro becomes a struct device_attribute dev_attr_name data structure
    */
    // creating first sysfs file
    stat = device_create_file(&sysfs_platform->dev, &dev_attr_SYSFS_FILE1);
    if(stat) {
        pr_info("Device create file failed %d\n", stat);
        goto out2;
    }

    pr_debug("sysfs file (/sys/devices/platform/%s/%s) created\n", PLAT_NAME, __stringify(SYSFS_FILE1));

    // creating second sysfs file
    stat = device_create_file(&sysfs_platform->dev, &dev_attr_sysfs_pgoff);
    if (stat) {
        pr_info("device create file failed %d\n", stat);
        goto out3;
    }

    // create our third sysfile : sysfs_pressure
    gpressure = 25;
    stat = device_create_file(&sysfs_platform->dev, &dev_attr_sysfs_pressure);
    if(stat) {
        pr_info("device create file failed %d \n", stat);
        goto out4;
    }

    pr_debug("sysfs files created (/sys/devices/platform/%s/%s)", 
                PLAT_NAME, __stringify(SYSFS_FILE3));
    
    pr_info("Iniitialized\n");
    return 0;

out4:
    device_remove_file(&sysfs_platform->dev, &dev_attr_sysfs_pressure);
out3:
    device_remove_file(&sysfs_platform->dev, &dev_attr_sysfs_pgoff);
out2:
    platform_device_unregister(sysfs_platform);
out1:
    return stat;
}


static void __exit sysfs_prog_exit(void) {
    device_remove_file(&sysfs_platform->dev, &dev_attr_sysfs_pressure);
    device_remove_file(&sysfs_platform->dev, &dev_attr_sysfs_pgoff);
    device_remove_file(&sysfs_platform->dev, &dev_attr_SYSFS_FILE1);
    platform_device_unregister(sysfs_platform);
    pr_info("Removed.\n");
}

module_init(sysfs_prog_init);
module_exit(sysfs_prog_exit);
