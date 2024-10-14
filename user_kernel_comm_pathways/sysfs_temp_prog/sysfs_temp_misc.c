/*
Extend one of the misc device drivers we wrote in chapter 1 (temp or task display)
setup two sysfs files and their read/write callbacks and test them on userspace.
Let's go with temp.
*/
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
MODULE_DESCRIPTION("Misc Driver for read/write using sysfs");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

#define temp      sysfs_temp

static int temp_value;  // Temperature value

// Function to check if the string contains only valid numeric characters.
static int validate_num(const char *str) {
    int i = 0;
    if (str[i] == '-') // Allow negative numbers
        i++;
    
    for (; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 0; // Not a numeric character
        }
    }
    
    return 1;
}

static int fahrenheit_to_celsius(int f) {
    return (f - 32) * 5 / 9;
}

static ssize_t sysfs_temp_show(struct device *dev, struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", temp_value);
}

static ssize_t sysfs_temp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    // char kbuf[BUF_SIZE];
    // int f_int;

    // if (count > BUF_SIZE - 1) {
    //     pr_warn("Input exceeds buffer size\n");
    //     return -EINVAL;
    // }

    // if (copy_from_user(kbuf, buf, count)) {
    //     return -EINVAL;
    // }
    // kbuf[count] = '\0';

    // if (validate_num(kbuf) == 0) {
    //     pr_info("Invalid input\n");
    //     return -EINVAL;
    // }

    // if (kstrtoint(kbuf, 10, &f_int) != 0) {
    //     pr_warn("Failed to convert string to integer\n");
    //     return -EINVAL;
    // }

    // temp_value = fahrenheit_to_celsius(f_int);
    // pr_info("Temp in celsius: %d\n", temp_value);
    // return count;

    int f_int, prev_temp_value;
    
    // Store the previous temperature value
    prev_temp_value = temp_value;

    // Check for invalid input
    if (count == 0 || count > BUF_SIZE - 1) {
        pr_warn("Input exceeds buffer size or is zero\n");
        return -EINVAL;
    }

    // Null-terminate the buffer
    if (buf[count - 1] != '\n') {
        pr_warn("Input is not properly null-terminated\n");
        return -EINVAL;
    }

    // Convert string to integer directly from the buffer
    if (kstrtoint(buf, 10, &f_int) != 0) {
        pr_warn("Failed to convert string to integer\n");
        return -EINVAL;
    }

    // Validate the new temperature value
    // if (f_int < TEMP_MIN || f_int > TEMP_MAX) {
    //     pr_info("Trying to set invalid temperature value (%d)\n"
    //             " [allowed range: %d-%d]; resetting to previous value (%d)\n",
    //             f_int, TEMP_MIN, TEMP_MAX, prev_temp_value);
    //     temp_value = prev_temp_value;  // Revert to previous value
    //     return -EFAULT;  // Indicate an error
    // }

    // Update the temperature value in Celsius
    temp_value = fahrenheit_to_celsius(f_int);
    pr_info("Temp in Celsius: %d\n", temp_value);
    return count;
}

// Use DEVICE_ATTR_RW for read/write access
static DEVICE_ATTR_RW(temp);

static const struct file_operations temp_driver_fops = {
    .write = NULL,
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

    ret = misc_register(&temp_driver);
    if (ret) {
        pr_err("Failed to register misc device.\n");
        return ret;
    }

    // Create sysfs file
    ret = device_create_file(temp_driver.this_device, &dev_attr_temp);
    if (ret) {
        pr_err("Failed to create sysfs.\n");
        misc_deregister(&temp_driver);
        return ret;
    }

    pr_info("Temp driver registered as /dev/%s\n", temp_driver.name);

    return 0;
}

static void __exit temp_exit(void) {
    device_remove_file(temp_driver.this_device, &dev_attr_temp);
    misc_deregister(&temp_driver);
    pr_info("Temp driver has been deregistered.\n");
}

module_init(temp_init);
module_exit(temp_exit);


