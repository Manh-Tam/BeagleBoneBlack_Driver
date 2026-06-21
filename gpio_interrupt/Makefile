obj-m += gpio_interrupt.o

KDIR := /home/tamle/Workspace/linux

PWD := $(shell pwd)

all:
	make -C $(KDIR) \
	ARCH=arm \
	CROSS_COMPILE=arm-linux-gnueabihf- \
	M=$(PWD) modules

	arm-linux-gnueabihf-gcc -static gpio_interrupt_app.c -o gpio_interrupt_app
	scp gpio_interrupt.ko gpio_interrupt_app debian@192.168.0.172:/home/debian/workspace/gpio_interrupt
clean:
	make -C $(KDIR) \
	ARCH=arm \
	CROSS_COMPILE=arm-linux-gnueabihf- \
	M=$(PWD) clean
	
	rm -rf gpio_interrupt_app