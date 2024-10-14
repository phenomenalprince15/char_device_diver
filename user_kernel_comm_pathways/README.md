# Various techniques

## /proc

- proc fs is that its content is not on a non-volatile disk. Its content is in RAM and volatile.
- files and dirs we see under /proc are pseudo files that have been setup by the kernel code for proc, and showing its file size as zero.

```
wiki@pi:~$ mount | grep -w proc
proc on /proc type proc (rw,nosuid,nodev,noexec,relatime)
systemd-1 on /proc/sys/fs/binfmt_misc type autofs (rw,relatime,fd=32,pgrp=1,timeout=0,minproto=5,maxproto=5,direct,pipe_ino=2495)
binfmt_misc on /proc/sys/fs/binfmt_misc type binfmt_misc (rw,nosuid,nodev,noexec,relatime)
wiki@pi:~$ ls -l /proc/
total 0
dr-xr-xr-x  9 root             root                           0 Jan  1  1970 1
dr-xr-xr-x  9 root             root                           0 Aug 27 14:30 100
dr-xr-xr-x  9 root             root                           0 Aug 27 14:30 101
dr-xr-xr-x  9 root             root                           0 Aug 27 14:30 102
dr-xr-xr-x  9 root             root                           0 Oct 12 09:23 1031
dr-xr-xr-x  9 root             root                           0 Aug 27 14:30 11
dr-xr-xr-x  9 root             root                           0 Aug 27 14:30 12
dr-xr-xr-x  9 root             root                           0 Aug 27 14:30 12
...
...
...
```

- Dir under /proc whose names are int represent processess currently alive on the system.
- The name of dir is PID of pricess and TGID of the process.
- /proc/pid/ contains info reg the processs.
- systemd or init always have PID as 1.
- as a root userr, we can even modify kernel parameters /proc/sys. it's called sysctl.
- sysctl utility can be used.

```
wiki@pi:~$ ls /proc/1
ls: cannot read symbolic link '/proc/1/cwd': Permission denied
ls: cannot read symbolic link '/proc/1/root': Permission denied
ls: cannot read symbolic link '/proc/1/exe': Permission denied
attr       clear_refs       cpuset   fd       latency    maps       mountstats  oom_score      projid_map  sessionid     stack   syscall         timerslack_ns
autogroup  cmdline          cwd      fdinfo   limits     mem        net         oom_score_adj  root        setgroups     stat    task            uid_map
auxv       comm             environ  gid_map  loginuid   mountinfo  ns          pagemap        sched       smaps         statm   timens_offsets  wchan
cgroup     coredump_filter  exe      io       map_files  mounts     oom_adj     personality    schedstat   smaps_rollup  status  timers
wiki@pi:~$ sudo ls /proc/1
[sudo] password for wiki: 
attr       clear_refs       cpuset   fd       latency    maps       mountstats  oom_score      projid_map  sessionid     stack   syscall         timerslack_ns
autogroup  cmdline          cwd      fdinfo   limits     mem        net         oom_score_adj  root        setgroups     stat    task            uid_map
auxv       comm             environ  gid_map  loginuid   mountinfo  ns          pagemap        sched       smaps         statm   timens_offsets  wchan
cgroup     coredump_filter  exe      io       map_files  mounts     oom_adj     personality    schedstat   smaps_rollup  status  timers
wiki@pi:~$ 
```

- procfs is off-bounds to driver authors. So don't use it.
- single_open() API, whenever this file is read, the proc_file system will call back our proc_show_debug_level() routine.
- Writing a simple proc fs program to handle debug_level (0 - no debug print messages, 1-medium, 2-all messages)

#### Misc procfs API
- We can create a symbolic or soft link within /proc by using proc_symlink() function.
- proc_create_single_data() API is used.
- Using this API eliminates the need of fops data structure.

```
struct proc_dir_entry *proc_create_single_data(const char *name, umode_t mode, struct proc_dir_entry *parent,
                                                int (*show) (struct seq_file *, void *), void *data);
```


## Interfacing with sysfs

- Sysfs tree encompasses the following :
1. Every bus present on the system (either virtual or pseudo bus as well)
2. Every device present on every bus.
3. Every device driver bound a device on a bus

- On system boot, and whenver a new device becomes visible, the driver core generates the required virtual files under sysfs tree.

```
wiki@pi:~/Linux-Kernel-Programming-Part-2/ch2$ ls /sys/
block  bus  class  dev  devices  firmware  fs  kernel  module  power
```

- One way to create a pseudo or virtual file under sysfs is via device_createa_file() API.
```
drivers/base/core.c
int device_Create_file(struct device *dev, const struct device_attribute *attr)

wiki@pi:~/Linux-Kernel-Programming-Part-2/ch2$ ls /sys/devices/platform/
 3ef642c0.nvram  'Fixed MDIO bus.0'   cam1_regulator   cpufreq-dt   fixedregulator_3v3   gpu      leds   power       regulator-sd-io-1v8   regulatory.0   serial8250      soc     uevent
 3ef64700.nvram   arm-pmu             cam_dummy_reg    emmc2bus     fixedregulator_5v0   kgdboc   phy    reg-dummy   regulator-sd-vcc      scb            snd-soc-dummy   timer   v3dbus

platform_device_register_simple() API ---> It returns a pointer to struct platform device.
we bring our platform device into existence by registering it to the (already existing) platform bus driver.

platform_device_unregister(sysfs_demo_platdev);
```

- we can create struct device in different ways, generic way is to setup and issue driver_create() API.
- Alternate means to create a sysfs file is bypassing the need for a device structure is to create a object and invoke sysfs_create_file() API.
- Here we prefer using platform device approach.



