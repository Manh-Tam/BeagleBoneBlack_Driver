obj-m += mybutton.o

KDIR := /home/tamle/Workspace/linux

PWD := $(shell pwd)

all:
	make -C $(KDIR) \
	ARCH=arm \
	CROSS_COMPILE=arm-linux-gnueabihf- \
	M=$(PWD) modules
clean:
	make -C $(KDIR) \
	ARCH=arm \
	CROSS_COMPILE=arm-linux-gnueabihf- \
	M=$(PWD) clean