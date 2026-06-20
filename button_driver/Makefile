obj-m += mybutton.o

KDIR := /home/tamle/Workspace/linux

PWD := $(shell pwd)

all:
	make -C $(KDIR) \
	ARCH=arm \
	CROSS_COMPILE=arm-linux-gnueabihf- \
	M=$(PWD) modules

	arm-linux-gnueabihf-gcc -static button_app.c -o button_app
clean:
	make -C $(KDIR) \
	ARCH=arm \
	CROSS_COMPILE=arm-linux-gnueabihf- \
	M=$(PWD) clean

	rm -rf button_app