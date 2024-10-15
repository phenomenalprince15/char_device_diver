# debugfs

```
wiki@pi:~/test/debugfs_prrog$ uname -r
6.8.0-1012-raspi
wiki@pi:~/test/debugfs_prrog$ mount | grep -w debugfs
debugfs on /sys/kernel/debug type debugfs (rw,nosuid,nodev,noexec,relatime)
wiki@pi:~/test/debugfs_prrog$ sudo ls /sys/kernel/debug
[sudo] password for wiki: 
accel  bluetooth        debug_enabled     dma_buf    dynamic_debug       hid        kvm           mmc0  pinctrl   regmap     sleep_time                swiotlb  vchiq
asoc   cec              devfreq           dma_pools  extfrag             i2c        lru_gen       mmc1  pm_genpd  regulator  spi-bcm2835-fe204000.spi  tracing  vcsm-cma
bdi    clear_warn_once  device_component  dmaengine  fault_around_bytes  ieee80211  lru_gen_full  opp   pwm       sched      split_huge_pages          usb
block  clk              devices_deferred  dri        gpio                kprobes    memblock      phy   ras       slab       stackdepot                vc-mem
wiki@pi:~/test/debugfs_prrog$ ls /proc/config.gz
ls: cannot access '/proc/config.gz': No such file or directory
wiki@pi:~/test/debugfs_prrog$ ls /boot/config-$(uname -r)
/boot/config-6.8.0-1012-raspi
wiki@pi:~/test/debugfs_prrog$ grep CONFIG_DEBUG_FS /boot/config-$(uname -r)
CONFIG_DEBUG_FS=y
CONFIG_DEBUG_FS_ALLOW_ALL=y
# CONFIG_DEBUG_FS_DISALLOW_MOUNT is not set
# CONFIG_DEBUG_FS_ALLOW_NONE is not set
wiki@pi:~/test/debugfs_prrog$ mount | grep -w debugfs
debugfs on /sys/kernel/debug type debugfs (rw,nosuid,nodev,noexec,relatime)
wiki@pi:~/test/debugfs_prrog$
```

- https://www.kernel.org/doc/html/latest/filesystems/api-summary.html#the-debugfs-filesystem

```
struct dentry *debugfs_lookup(const char *name, struct dentry *parent); // lookup an existing debugfs file
struct dentry *debugfs_create_file(const char *name, umode_t mode, 
            struct dentry *parent, void *data, const struct file_operations *fops) // create a debugfs file
and many other functionalities are offered.
It has no rules in particular.
```



