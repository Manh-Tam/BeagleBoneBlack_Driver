#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>       /* Required for spi_device and spi_driver */
#include <linux/gpio/consumer.h> /* Modern GPIO API */
#include <linux/mod_devicetable.h>
#include <linux/of.h>

/* private device structure */
struct st7789_priv {
    struct spi_device *spi;
    struct gpio_desc *dc_gpio;
    struct gpio_desc *rst_gpio;
};

/* 1. Changed parameter type from platform_device to spi_device */
static int st7789_probe(struct spi_device *spi)
{
    pr_info("st7789 matching device found and initialized\n");
    return 0;
}

/* 2. Changed parameter type from platform_device to spi_device */
static int st7789_remove(struct spi_device *spi)
{
    pr_info("st7789 removed\n");
    return 0;
}

static const struct of_device_id my_of_match[] = 
{
    {
        .compatible = "custom,st7789v",
    },
    {}
};
MODULE_DEVICE_TABLE(of, my_of_match); /* Exports the table for matching functionality */

/* 3. Changed from platform_driver to spi_driver */
static struct spi_driver st7789_driver = 
{
    .probe = st7789_probe,
    .remove = st7789_remove,
    .driver = 
    {
        .name = "st7789driver",
        .of_match_table = my_of_match,
    },
};

/* 4. Use the specialized SPI registration helper macro */
module_spi_driver(st7789_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("PvsZ");