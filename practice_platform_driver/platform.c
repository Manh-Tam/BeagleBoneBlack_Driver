#include <linux/kernel.h>
#include <linux/module.h>

/* Platform driver */
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>

#include <linux/gpio.h>

struct gpio_desc *gpiod;

static struct of_device_id my_of_match[] = 
{
    {
        .compatible = "tam,led0",
    },
    {}
};

static int my_probe(struct platform_device *pdev)
{
    const char *compatible;
    struct device *mdev = &pdev->dev;
    struct device_node *np= mdev->of_node;
    
    gpiod = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    gpiod_set_value(gpiod, 1);

    of_property_read_string(np, "compatible", &compatible);
    pr_info("compatible: \"%s\"\n", compatible);
    pr_info("My probed\n");
    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    pr_info("My removed\n");
    return 0;
}


static struct platform_driver my_device = 
{
    .probe = my_probe,
    .remove = my_remove,
    .driver = 
    {
        .name = "platform",
        .of_match_table = my_of_match,
    },
};

static int __init platform_init(void)
{
    pr_info("Hello from platform\n");
    return platform_driver_register(&my_device);
}

static void __exit platform_exit(void)
{
    platform_driver_unregister(&my_device);
    pr_info("Goodbye from platform\n");
}

module_init(platform_init);
module_exit(platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("Hello World");

