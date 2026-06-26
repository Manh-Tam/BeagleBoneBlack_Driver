#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

static const struct of_device_id my_of_match[] = {
    {
        .compatible = "tam,my-led",
    },
    {

    }
};

static int my_probe(struct platform_device *pdev)
{
    int ret = -1;
    struct device_node *np;
    int led_gpio;
    pr_info("Device found!\n");
    np = pdev->dev.of_node;
    led_gpio = of_get_named_gpio(np, "led-gpios", 0);
    if (!gpio_is_valid(led_gpio))
    {
        pr_err("Invalid GPIO\n");
        return -EINVAL;
    }
    else
    {
        printk("GPIO num: %d\n", led_gpio);
        ret = gpio_request(led_gpio, "myled");
        if (ret)
        {
            pr_err("Failed to request gpio\n");
            return ret;
        }
        else
        {
            ret = gpio_direction_output(led_gpio, 0);
            if (ret)
            {
                pr_err("Failed to set gpio direction\n");
                gpio_free(led_gpio);
                return ret;
            }
            else
            {
                gpio_set_value(led_gpio, 1);
            }
        }
    }
    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    pr_info("Device removed\n");
    return 0;
}

static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "my_led",
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

