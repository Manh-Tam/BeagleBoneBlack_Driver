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
