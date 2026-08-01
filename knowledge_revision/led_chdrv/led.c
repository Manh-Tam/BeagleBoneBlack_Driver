#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>

#define LED_GPIO     67  /* P8_8 */

static dev_t led_num;
static struct cdev led;
static struct class *led_class;
static struct device *device;
static struct file_operations fops;

static int __init led_init(void);
static void __exit led_exit(void);
static int my_open(struct inode *inode, struct file *file);
static int my_release(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("led");

static struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int __init led_init(void)
{
    int ret = -1;
    printk("led_init: OK\n");
    ret = alloc_chrdev_region(&led_num, 0, 1, "mybutton");
    if (0 == ret)
    {
        led_class = class_create(THIS_MODULE, "led_class");
        if (IS_ERR(led_class))
        {
            ret = PTR_ERR(led_class);
        }
    }
    if (0 == ret)
    {
        cdev_init(&led, &fops);
        ret = cdev_add(&led, led_num, 1);
    }
    if (0 == ret)
    {
        device = device_create(led_class, NULL, led_num, NULL, "led");
        if (IS_ERR(device))
        {
            ret = PTR_ERR(device);
        }
    }

    /*initialize gpio*/
    if (ret == 0)
    {
        ret = !gpio_is_valid(LED_GPIO);
    }

    if (ret == 0)
    {
        ret = gpio_request(LED_GPIO, "led");
    }

    if (ret == 0)
    {
        ret = gpio_direction_output(LED_GPIO, 1);
    }

    return ret;
}

static void __exit led_exit(void)
{
    gpio_free(LED_GPIO);
    device_destroy(led_class, led_num);
    class_destroy(led_class);
    cdev_del(&led);
    unregister_chrdev_region(led_num, 1);
    printk("led_exit: OK\n");
}

static int my_open(struct inode *inode, struct file *file)
{
    printk("my_open: OK\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    printk("my_release: OK\n");
    return 0;
}

/*return value = 0: EOF*/
/*return value > 0: num of read bytes*/
/*return value < 0: error*/
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    printk("my_read: OK\n");
    return count;
}

/*return value < 0: error*/
/*return value >= 0: num of written bytes*/
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    int value;
    char kbuf[100];

    if (copy_from_user(kbuf, buf, count))
    {
        return -1;
    }
    printk("kbuf: %s", kbuf);

    value = gpio_get_value(LED_GPIO);
    if (value)
    {
        printk("gpio %d off", LED_GPIO);
        gpio_set_value(LED_GPIO, 0);
    }
    else
    {
        printk("gpio %d on", LED_GPIO);
        gpio_set_value(LED_GPIO, 1);
    }
    printk("my_write: OK\n");
    return count;
}

