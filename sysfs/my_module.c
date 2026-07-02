#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>

struct my_device {
    struct gpio_desc *led;
};

static int probe(struct platform_device *p_dev);
static int remove(struct platform_device *p_dev);
static ssize_t led_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t led_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR_RW(led);

static struct of_device_id my_of_match[] = 
{
    {
        .compatible = "tam,led1",
    },
    {}
};

static struct platform_driver my_driver = 
{
    .probe = probe,
    .remove = remove,
    .driver = 
    {
        .name = "my platform driver",
        .of_match_table = my_of_match,
    },
};

static int __init my_init(void)
{
    return platform_driver_register(&my_driver);
}
static void __exit my_exit(void)
{
    platform_driver_unregister(&my_driver);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("My kernel module");


static int probe(struct platform_device *pdev)
{
    int ret = 0;
    ret = device_create_file(&pdev->dev,
                         &dev_attr_led);
    printk("size of ssize_t: %d\n", sizeof(ssize_t));
    printk("called from probe\n");
    return 0;
}

static int remove(struct platform_device *pdev)
{
    device_remove_file(&pdev->dev,
                   &dev_attr_led);
    printk("called from remove\n");
    return 0;
}

static ssize_t led_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    printk("led+show\n");
    return 1;
}

static ssize_t led_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    printk("led+store\n");
    return 1;
}
