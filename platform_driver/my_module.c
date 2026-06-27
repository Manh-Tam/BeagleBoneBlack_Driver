#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>


struct led
{
    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    int led_gpio;
};

static struct led my_led;

static const struct of_device_id my_of_match[] = {
    {
        .compatible = "tam,my-led",
    },
    {

    }
};

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("my_open called\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("my_relase called\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    gpio_set_value(my_led.led_gpio, 0);
    pr_info("my_read called\n");
    return 0;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    gpio_set_value(my_led.led_gpio, 1);
    pr_info("my_write called\n");
    return count;
}

struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int my_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct device_node *np;
    pr_info("Device found!\n");
    np = pdev->dev.of_node;
    my_led.led_gpio = of_get_named_gpio(np, "led-gpios", 0);

    if (!gpio_is_valid(my_led.led_gpio))
    {
        pr_err("Invalid GPIO\n");
        ret = -EINVAL;
    }
    
    if (0 == ret)
    {
        printk("GPIO num: %d\n", my_led.led_gpio);
        ret = gpio_request(my_led.led_gpio, "myled");
    }

    if (0 == ret)
    {
        ret = gpio_direction_output(my_led.led_gpio, 0);
    }

    if (0 == ret)
    {
        ret = alloc_chrdev_region(&my_led.dev_num, 0, 1, "myled");
    }

    if (0 == ret)
    {
        cdev_init(&my_led.cdev, &fops);
        ret = cdev_add(&my_led.cdev, my_led.dev_num, 1);
    }

    if (0 == ret)
    {
        my_led.class = class_create(THIS_MODULE, "my_led");
    }

    if (0 == ret)
    {
        struct device *device = device_create(my_led.class, NULL, my_led.dev_num, NULL, "myled");
        if (IS_ERR(device))
        {
            ret = PTR_ERR(device);
        }
    }
    return ret;
}

static int my_remove(struct platform_device *pdev)
{
    gpio_set_value(my_led.led_gpio, 0);
    gpio_free(my_led.led_gpio);
    pr_info("Device removed\n");
    return 0;
}

static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "llll",
        .of_match_table = my_of_match,
    },
};

static int __init my_init(void)
{
    printk("My module initialized\n");
    return platform_driver_register(&my_driver);
}

static void __exit my_exit(void)
{
    platform_driver_unregister(&my_driver);
    printk("My module exited\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Manh Tam");
MODULE_DESCRIPTION("My module");

