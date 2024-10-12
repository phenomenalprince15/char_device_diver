#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/uaccess.h>

static int stored_pid = -1; // to store the written PID

MODULE_AUTHOR("Prince");
MODULE_DESCRIPTION("Task List Display");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static ssize_t read_task_list_display(struct file *file, char __user *ubuf, 
                                        size_t len, loff_t *off) {
    struct pid *pid;
    struct task_struct *task;
    char *kbuf;
    int ret;

    if (stored_pid < 0) {
        pr_err("Invalid PID\n");
    }

    pid = find_get_pid(stored_pid);
    if (!pid) {
        pr_err("Task display: PID %d not found\n", stored_pid);
        return -ESRCH;
    }

    task = get_pid_task(pid, PIDTYPE_PID);
    if (!task) {
        pr_err("Task display: Task for PID %d not found\n", stored_pid);
        return -ESRCH;
    }

    kbuf = kvmalloc(256, GFP_KERNEL);
    if (!kbuf) {
        return -ENOMEM;
    }

    snprintf(kbuf, 256, "Task name: %s\n
            PID: %d\n State: %ld\n", task->comm, task->pid, task->state);
    
    ret = simple_read_from_buffer(ubuf, len, off, kbuf, strlen(kbuf));
    kfree(kbuf);

    return ret;
}

static const struct file_operations task_list_display_fops = {
    .read = read_task_list_display,
    .write = write_task_list_display,
    .llseek = no_llseek,
};

static struct miscdevice task_list_display = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "task_list_display_driver",
    .mode = 0666,
    .fops = &task_list_display_fops,
};

static int __init task_list_init(void) {
    int ret = 0;
    ret = misc_register(&task_list_display)
    if(ret) {
        pr_notice();
        return ret;
    }
}

static void __exit task_list_exit(void) {
    misc_deregister(&task_list_display);
    pr_info("Driver has been deregistered.\n");
}

module_init (task_list_init);
module_exit (task_list_exit);

