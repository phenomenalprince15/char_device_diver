#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#else
#include <asm/uaccess.h>
#endif

#define OURMODNAME      "debugfs_prog"

MODULE_AUTHOR("Prince");
MODULE_DESCRIPTION("debugfs learning");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

/* Module parameters */
static int cause_an_oops;
module_param(cause_an_oops, int, 0644);
MODULE_PARM_DESC(cause_an_oops,
"Setting this to 1 can cause a kernel bug, an Oops; if 1, we do NOT perform"
" required cleanup! so, after removal, any op on the debugfs files will cause"
" an Oops! (default is 0, no bug)");

static struct dentry *gparent;
/* Mutex lock */
DEFINE_MUTEX(mtx);

struct driver_context {
    int tx, rx, err, myword, power;
    u32 config1;
    u32 config2;
    u64 config3;
#define MAXBYTES        128
    char message[MAXBYTES];
};
static struct driver_context *gdriver_context;
static int debug_level;     // off/0 by default

static ssize_t debugfs_show_driver_context(struct file *fp, char __user *ubuf, 
            size_t count, loff_t *fpos) {

    struct driver_context *data = (struct driver_context *)fp->f_inode->i_private;

#define MAXUPASS        256
    char locbuf[MAXUPASS];

    if (mutex_lock_interruptible(&mtx)) return -ERESTARTSYS;

    data->config3 = jiffies;
    snprintf(locbuf, MAXUPASS - 1, 
            "prodname:%s\n"
            "tx:%d, rx:%d, err:%d, myword:%d,power:%d\n"
            "config1:0x%x, config2:0x%x, config3:0x%llx (%llu)\n"
            "our message:%s\n",
            OURMODNAME,
            data->tx, data->rx, data->err, data->myword, data->power,
            data->config1, data->config2, data->config3, data->config3, data->message);


    mutex_unlock(&mtx);
    return simple_read_from_buffer(ubuf, MAXUPASS-1, fpos, locbuf, strlen(locbuf));
}

static const struct file_operations debugfs_driver_context_fops = {
    .read = debugfs_show_driver_context,
};

static struct driver_context *alloc_init_driver_context(void) {
    struct driver_context *driver = NULL;

    driver = kzalloc(sizeof(struct driver_context), GFP_KERNEL);
    if (!driver) {
        return ERR_PTR(-ENOMEM);
    }

    driver->config1 = 0x0;
    driver->config2 = 0x48524a5f;
    driver->config3 = jiffies;
    driver->power = 1;
    // strncpy(driver->message, "Hello driver", 13);
    strncpy(driver->message, "Hello driver", sizeof(driver->message) - 1);
    driver->message[sizeof(driver->message) - 1] = '\0';

    pr_info("Allocated and init the driver context data structure.\n");
    return driver;
}

static int __init debugfs_prog_init(void) {
    
    int stat = 0;
    struct dentry *file1, *file2;

    if (!IS_ENABLED(CONFIG_DEBUG_FS)) {
        pr_warn("debugfs unsupported.\n");
        return -EINVAL;
    }

    /*
        Create a dir under the debugfs mount point
        struct dentry *debugfs_create_dir(const char *name, struct dentry *parent)
create a directory in the debugfs filesystem
    */
    gparent = debugfs_create_dir(OURMODNAME, NULL);
    if (!gparent) {
        pr_info("debugfs creation failed.\n");
        stat = PTR_ERR(gparent);
        goto out_fail_1;
    }

    /*
        Allocate and initialize out driver context data structure
    */
   gdriver_context = alloc_init_driver_context();
   if (IS_ERR(gdriver_context)) {
        pr_info("driver context allocation failed.\n");
        stat = PTR_ERR(gdriver_context);
        goto out_fail_2;
   }

   /*
        Generic debugfs + passing a pointer to a data structure
        struct dentry *debugfs_create_file(const char *name, umode_t mode, struct dentry *parent, 
        void *data, const struct file_operations *fops)¶
   */
#define DEBUGFS_FILE1       "debugfs_show_driver_context"
    file1 = debugfs_create_file(DEBUGFS_FILE1, 0440, gparent, (void *)gdriver_context, 
                &debugfs_driver_context_fops);

    if (!file1) {
        pr_info("debugfs create file failed.\n");
        stat = PTR_ERR(file1);
        goto out_fail_3;
    }
    pr_debug("debugfs file 1 mount point: /%s/%s created\n", OURMODNAME, DEBUGFS_FILE1);

    /*
        Create the debugfs file for debug level global
    */
#define DEBUGFS_FILE2         "debugfs_debug_level"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)  // ref commit 2b07021a94
    debugfs_create_u32(DEBUGFS_FILE2, 0644, gparent, &debug_level);
#else
    debugfs_create_u32(DEBUGFS_FILE2, 0644, gparent, &debug_level);
    // if (!file2) {
    //     pr_info("debugfs_create_u32 failed, aborting...\n");
    //     stat = PTR_ERR(file2);
    //     goto out_fail_3;
    // }   
#endif

    pr_debug("debugfs file 2 mount point: /%s/%s created\n", OURMODNAME, DEBUGFS_FILE2);

    pr_info("Initialized %s\n", cause_an_oops == 1 ? "On" : "Off");

    return 0;

out_fail_3:
    kfree(gdriver_context);
out_fail_2:
    debugfs_remove_recursive(gparent);
out_fail_1:
    return stat;
}

static void __exit debugfs_prog_exit(void) {
    kfree(gdriver_context);
    if (!cause_an_oops) {
        debugfs_remove_recursive(gparent);
    }
    pr_info("Removed.\n");
}

module_init(debugfs_prog_init);
module_exit(debugfs_prog_exit);

