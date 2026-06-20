#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <linux/gpio.h>
#include <linux/uaccess.h>

#include <linux/string.h>

#define LED_GPIO			60
#define RECEIVED_DATA_LEN	10

#define MY_MAGIC        'L'
#define LED_ON          _IO(MY_MAGIC, 0)
#define LED_OFF         _IO(MY_MAGIC, 1)
#define LED_TOGGLE      _IO(MY_MAGIC, 2)

static dev_t dev_num;
static struct cdev my_led;
static struct class *led_class;
static int led_state;

static int my_open(struct inode *inode, struct file *file)
{
	pr_info("Opened\n");
	return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
	pr_info("Released\n");
	return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	char data[2] = {0};
	pr_info("Read\n");
	if (*ppos > 0)
	{
		return 0;
	}
	data[0] = led_state? '1' : '0';
	data[1] = '\n';
	if (copy_to_user(buf, data, 2))
	{
		pr_err("Failed to copy to user\n");
		return -EFAULT;
	}
	*ppos += 2;
	return *ppos;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char kbuf[RECEIVED_DATA_LEN];
	if (count >= RECEIVED_DATA_LEN)
	{
		pr_err("Invalid data length\n");
		return -EFAULT;
	}
	if (copy_from_user(kbuf, buf, count))
	{
		pr_err("Failed to copy from user\n");
		return -EFAULT;
	}
	kbuf[count] = '\0';
	if (count > 0 && kbuf[count-1] == '\n')
		kbuf[count-1] = '\0';

	if (0 == strcmp(kbuf, "on"))
	{
		gpio_set_value(LED_GPIO, 1);
		led_state = 1;
	}
	else
	{
		gpio_set_value(LED_GPIO, 0);
		led_state = 0;
		return -EINVAL;
	}

	pr_info("Received: %s\n", kbuf);
	return count;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
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
	return 0;
}

static struct file_operations fops = 
{
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_release,
	.write = my_write,
	.read = my_read,
	.unlocked_ioctl = my_ioctl,
};

static int __init myled_init(void)
{
	int ret = 0;
	ret = alloc_chrdev_region(&dev_num, 0, 1, "myled");
	if (ret)
	{
		pr_err("Failed to allocate device number\n");
		return ret;
	}
	cdev_init(&my_led, &fops);
	ret = cdev_add(&my_led, dev_num, 1);
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
		cdev_del(&my_led);
		unregister_chrdev_region(dev_num, 1);
		ret = -1;
		return ret;
	}
	if (NULL == device_create(led_class, NULL, dev_num, NULL, "myled"))
	{
		pr_err("Failed to create device\n");
		class_destroy(led_class);
		cdev_del(&my_led);
		unregister_chrdev_region(dev_num, 1);
		ret = -2;
		return ret;
	}
	if (!gpio_is_valid(LED_GPIO))
	{
		pr_err("Invalid GPIO %d\n", LED_GPIO);
		return -EINVAL;
	}
	ret = gpio_request(LED_GPIO, "my_led");
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
	led_state = 0;
	pr_info("My led initialized successfully\n");
	return ret;
}

static void __exit myled_exit(void)
{
	device_destroy(led_class, dev_num);
	class_destroy(led_class);
	cdev_del(&my_led);
	unregister_chrdev_region(dev_num, 1);
	gpio_free(LED_GPIO);
	pr_info("exit my led\n");
}


module_init(myled_init);
module_exit(myled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("Hello");
