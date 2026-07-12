# BeagleBoneBlack_Driver
Hi everyone, welcome to my repository. I am eager to learn embedded Linux, so I am making these series of journals to track my progress. I think they can provide you with insights into your Linux journey. Thank you.
6/19/2026
Add a gpio driver to control P912 aka GPIO60 on the BBB.
6/20/2026
Add a button driver to control P8_8 aka GPIO67 on the BBB.
6/21/2026
Add a gpio interrupt, blocking read, poll/select

6/25/2026
modify device trees, learn platform drivers to control hardware with device tree and avoid hardcoded hardware.

6/27/2026
successfully added a platform driver.
use platform driver + character driver to control led

6/28/2026
add a platform driver for multiple devices

6/29/2026
spend time reviewing what I've learned.
kernel module -> device drivers -> platform drivers

6/30/2026
Use modern gpio APIs for platform drivers to control a GPIO.
This approach hides the GPIO hardware number from users.

7/1/2026
Add a device tree interrupt
Control the interrupt through a platform driver.

7/2/2026
Learned sysfs

7/3/2026
review my knowledge

7/11/2026
add driver for ST7789 and XPT2046 at application layer

7/12/2026
Start final project to create a simplified PvsZ game using the touch screen
1. Create a device tree for the hardware
2. Create a spi client driver to control the hardware at kernel space
3. Write an application at user space
-> handle graphics, create plants, create zombies, movements
-> handle collisions
-> handle the logic

integrate spi into my custom device tree overlay.
face serious issues related to device tree
-> base device tree loaded first
-> universal capre loaded next, occupy P9_22
-> my dto loaded in the end => can't use P9_22
-> do research on Google + AI -> find out a way to disable P9_22 under /ocp node