#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include <linux/gpio.h>

#define LED_GPIO        67
#define MY_MAGIC        'L'
#define LED_ON          _IO(MY_MAGIC, 0)
#define LED_OFF         _IO(MY_MAGIC, 1)
#define LED_TOGGLE      _IO(MY_MAGIC, 2)

static dev_t dev_num;
static struct cdev led_chdrv;
static struct class *led_class;
static struct file_operations fops;
static int led_open(struct inode *inode, struct file *file);
static int led_release(struct inode *inode, struct file *file);
static ssize_t led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int __init led_init(void);
static void __exit led_exit(void);

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("LED IOCTL");

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .read = led_read,
    .write = led_write,
    .unlocked_ioctl = led_ioctl,
};


static int led_open(struct inode *inode, struct file *file)
{
    printk("led open\n");
    return 0;
}
static int led_release(struct inode *inode, struct file *file)
{
    printk("led released\n");
    return 0;
}

static ssize_t led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    printk("led read\n");
    return 0;
}

static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    printk("led write\n");
    return count;
}

static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    pr_info("Received command: %d\n", cmd);
	switch (cmd)
	{
    case LED_ON:
        gpio_set_value(LED_GPIO, 1);
        break;
    case LED_OFF:
        gpio_set_value(LED_GPIO, 0);
        break;
    case LED_TOGGLE:
        int value = gpio_get_value(LED_GPIO);
        if (value < 0)
        {
            return -EFAULT;
        }
        else
        {
            pr_info("toggle value: %d\n", value);
            gpio_set_value(LED_GPIO, !value);
        }
        break;
    default:
        return -EINVAL;
	}
	printk("led ioctl\n");
	return 0;
}

static int __init led_init(void)
{
    int ret = 0;
    printk("led init\n");
	ret = alloc_chrdev_region(&dev_num, 0, 1, "myled");
	if (ret)
	{
		pr_err("Failed to allocate device number\n");
		return ret;
	}
	cdev_init(&led_chdrv, &fops);
	ret = cdev_add(&led_chdrv, dev_num, 1);
	if (ret)
	{
		pr_err("Failed to add device\n");
		unregister_chrdev_region(dev_num, 1);
		return ret;
	}
	led_class = class_create(THIS_MODULE, "myled");
	if (NULL == led_class)
	{
		pr_err("Failed to create class\n");
		cdev_del(&led_chdrv);
		unregister_chrdev_region(dev_num, 1);
		ret = -1;
		return ret;
	}
	if (NULL == device_create(led_class, NULL, dev_num, NULL, "myled"))
	{
		pr_err("Failed to create device\n");
		class_destroy(led_class);
		cdev_del(&led_chdrv);
		unregister_chrdev_region(dev_num, 1);
		ret = -2;
		return ret;
	}
	if (!gpio_is_valid(LED_GPIO))
	{
		pr_err("Invalid GPIO %d\n", LED_GPIO);
		return -EINVAL;
	}
	ret = gpio_request(LED_GPIO, "led_chdrv");
	if (ret)
	{
		pr_err("Failed to request gpio\n");
		return ret;
	}
	ret = gpio_direction_output(LED_GPIO, 0);
	if (ret)
	{
		pr_err("Failed to set dirrection\n");
		return ret;
	}
	pr_info("My led initialized successfully\n");
    return ret;
}

static void __exit led_exit(void)
{
    printk("led exit\n");
    gpio_free(LED_GPIO);
	device_destroy(led_class, dev_num);
	class_destroy(led_class);
	cdev_del(&led_chdrv);
	unregister_chrdev_region(dev_num, 1);
}










