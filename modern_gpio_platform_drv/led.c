#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/of_gpio.h>

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>


#define LED_GPIO        60

static dev_t dev_num;
static struct cdev cdev;
static struct class *class;

static struct gpio_desc *gpiod;

static int gpio_num;
static struct gpio_desc *gpiod;


static const struct of_device_id my_of_match[] = 
{
    {
        .compatible = "tam,led1",
    },
    {}
};

static int my_probe(struct platform_device *pdev)
{
    struct device_node *np;
    np = pdev->dev.of_node;
    gpiod = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    gpiod_set_value(gpiod, 1);
    pr_info("my_probe1 called\n");
    return 0;
};

static int my_remove(struct platform_device *pdev)
{
    gpiod_set_value(gpiod, 0);
    gpio_free(gpio_num);
    pr_info("my_remove called\n");
    return 0;
};

static struct platform_driver my_driver = 
{
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "led driver",
        .of_match_table = my_of_match,
    },
};

static int led_open(struct inode *inode, struct file *file)
{
    pr_info("led opened\n");
    return 0;
}

static int led_release(struct inode *inode, struct file *file)
{
    pr_info("led released\n");
    return 0;
}

static ssize_t led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    gpio_set_value(LED_GPIO, 1);
    return 0;
}

static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    gpio_set_value(LED_GPIO, 0);
    return count;
}

struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .read = led_read,
    .write = led_write,
};

// static int __init led_init(void)
// {
//     alloc_chrdev_region(&dev_num, 0, 1, "my led");
//     class = class_create(THIS_MODULE, "my_led");
//     cdev_init(&cdev, &fops);
//     cdev_add(&cdev, dev_num, 1);
//     device_create(class, NULL, dev_num, NULL, "my_led");
//     gpio_request(LED_GPIO, "my_gpio");
//     gpio_direction_output(LED_GPIO, 0);
//     gpiod = gpiod_get(&pdev->dev, "reset", GPIOD_OUT_LOW);
//     pr_info("My led initialized\n");
//     return 0;
// }

// static void __exit led_exit(void)
// {
//     device_destroy(class, dev_num);
//     cdev_del(&cdev);
//     class_destroy(class);
//     unregister_chrdev_region(dev_num, 1);
//     pr_info("My led deinitialized\n");
// }

static int __init led_init(void)
{
    return platform_driver_register(&my_driver);
}

static void __exit led_exit(void)
{
    platform_driver_unregister(&my_driver);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("Led Driver");