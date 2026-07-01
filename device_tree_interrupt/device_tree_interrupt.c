#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>

#include <linux/interrupt.h>
#include <linux/gpio.h>

static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
    pr_info("Button pressed\n");
    return IRQ_HANDLED;
}

static const struct of_device_id my_of_match[] = 
{
    {
        .compatible = "tam,mybutton",
    },
    {}
};

static int my_probe(struct platform_device *pdev)
{
    struct gpio_desc *button;
    int irq = 0;
    int ret = 0;
    button = devm_gpiod_get(&pdev->dev,
                        "button",
                        GPIOD_IN);
    irq = gpiod_to_irq(button);
    printk("irq: %d\n", irq);
    ret = devm_request_irq(&pdev->dev, irq, button_irq_handler, IRQF_TRIGGER_FALLING, "button", NULL);
    printk("ret: %d\n", ret);
    pr_info("my_probe called\n");
    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    pr_info("my_remove called\n");
    return 0;
}

static struct platform_driver my_driver = 
{
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "dt int",
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
MODULE_DESCRIPTION("Device Tree Interrupt");
