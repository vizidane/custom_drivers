obj-m := pcd.o
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
#set KERN_DIR to linux source location 
KERN_DIR = ~/workspace/ldd/source/linux/
HOST_KERN_DIR = /lib/modules/$(shell uname -r)/build/
TARGET_KERN_DIR = /home/tranviquang/workspace/ldd/source/linux/
BUILD_TARGET_DIR=$(TARGET_KERN_DIR)
#BUILD_TARGET_DIR=$(HOST_KERN_DIR)

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(BUILD_TARGET_DIR) M=$(PWD) modules
clean:
	make -C $(HOST_KERN_DIR) M=$(PWD) clean
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(BUILD_TARGET_DIR) M=$(PWD) clean
help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(BUILD_TARGET_DIR) M=$(PWD) help
host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules
