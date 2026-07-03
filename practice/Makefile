obj-m += my_module.o

KDIR = /home/tamle/linux

PWD = $(shell pwd)

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
