#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/uaccess.h>

#define BUF_SIZE 100

MODULE_AUTHOR("Prince");
MODULE_DESCRIPTION("Misc Driver for read/write");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

// Function to check if the string contains only valid numeric characters.
static int validate_num(const char *str) {
    int i = 0;
    int has_dot = 0;
    
    if (str[i] == '-') // Allow negative numbers
        i++;

    for (; str[i] != '\0'; i++) {
        if (str[i] == '.') {
            if (has_dot) return 0; // Only one dot allowed
            has_dot = 1;
        } else if (str[i] < '0' || str[i] > '9') {
            return 0; // Not a numeric character
        }
    }
    
    return 1;
}

// Fahrenheit to Celsius conversion function (using integer arithmetic)
static int fahrenheit_to_celsius(int fahrenheit_scaled) {
    // Conversion formula: C = (F - 32) * 5/9
    // Multiply everything by 10 for precision (fixed-point arithmetic)
    int celsius_scaled = (fahrenheit_scaled - 320) * 5 / 9;
    return celsius_scaled;
}


static ssize_t write_temp_driver(struct file *fp, const char __user *ubuf,
                size_t len, loff_t *off) {
    char *kbuf = NULL;
    int f_int, c_int;

    if (len > BUF_SIZE) {
        pr_warn("Len %zu exceeds max # of len allowed\n", len);
        return -EINVAL;
    }

    kbuf = kvmalloc(len+1, GFP_KERNEL);
    if (unlikely(!kbuf)) {
        pr_err("Memory allocation failed.\n");
        return -ENOMEM;
    }
    memset(kbuf, 0, len+1);

    if (copy_from_user(kbuf, ubuf, len)) {
        pr_warn("copy from user failed.\n");
        kvfree(kbuf);
        return -EFAULT;
    }

    kbuf[len] = '\0';

    // echo 100 > /dev/temp_driver ---> It's needed as "100" not "100\n"
    if (len > 0 && kbuf[len-1] == '\n') kbuf[len-1] = '\0'; // remove newline if present

    pr_info("Received input: %s\n", kbuf); // Debug log

    if (!validate_num(kbuf)) {
        pr_info("Invalid input.\n");
        kvfree(kbuf);
        return -EINVAL;
    }

    if (kstrtoint(kbuf, 10, &f_int) != 0) {
        pr_warn("Failed to convert string to integer.\n");
        kvfree(kbuf); // Free allocated memory on conversion failure
        return -EINVAL;
    }

    c_int = (f_int - 320) * 5 / 9;

    pr_info("Temperature in Celsius: %d.%d C\n", c_int / 10, c_int % 10);
    kvfree(kbuf);

    return len;
}

static const struct file_operations temp_driver_fops = {
    // .open = open_temp_driver,
    // .read = read_temp_driver,
    .write = write_temp_driver,
    // .release = close_temp_driver
    .llseek = no_llseek,
};

static struct miscdevice temp_driver = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "temp_driver",
    .mode = 0666,
    .fops = &temp_driver_fops,
};

static int __init temp_init(void) {
    int ret = 0;
    struct device *dev;
    ret = misc_register(&temp_driver);
    if (ret) {
        pr_notice();
        return ret;
    }

    dev = temp_driver.this_device;
    pr_info("Temp driver with major #10 registered, minor = %d\n"
            "dev node is /dev/%s\n", temp_driver.minor, temp_driver.name);

    return 0;
}

static void __exit temp_exit(void) {
    misc_deregister(&temp_driver);
    pr_info("Driver has been deregistered.\n");
}

module_init (temp_init);
module_exit(temp_exit);

