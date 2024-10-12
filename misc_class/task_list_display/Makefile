# Define variables
FNAME_C = task_list_detail
KDIR = /lib/modules/$(shell uname -r)/build
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

PWD := $(shell pwd)
obj-m := $(FNAME_C).o

# Debug mode (optional)
MYDEBUG = n
ifeq ($(MYDEBUG), y)
  ccflags-y += -DDEBUG -g -ggdb -Wall
else
  ccflags-y += -UDEBUG -Wall
endif

# Build the kernel module
all:
	@echo 'Building kernel module...'
	make -C $(KDIR) M=$(PWD) modules

# Install the kernel module
install:
	@echo 'Installing kernel module...'
	sudo make -C $(KDIR) M=$(PWD) modules_install
	sudo depmod

# Clean the build
clean:
	@echo 'Cleaning...'
	make -C $(KDIR) M=$(PWD) clean
	rm -f *~ *.dtb

# Help target
help:
	@echo 'Available targets:'
	@echo '  make          - Build the kernel module'
	@echo '  make install  - Install the kernel module'
	@echo '  make clean    - Clean the build'
	@echo '  make help     - Show this help message'
