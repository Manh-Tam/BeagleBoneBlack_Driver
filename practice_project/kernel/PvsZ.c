#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>       /* Required for spi_device and spi_driver */
#include <linux/gpio/consumer.h> /* Modern GPIO API */
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

/* private device structure */
struct st7789_priv {
    struct spi_device *spi;
    struct gpio_desc *dc_gpio;
    struct gpio_desc *rst_gpio;
};

#define COMMAND     0
#define DATA        1

static void lcd_command(struct spi_device *spi, u8 command)
{
    spi_write(spi, &command, 1);
}

static void lcd_data(struct spi_device *spi, u8 data)
{
    spi_write(spi, &data, 1);
}

/* 1. Changed parameter type from platform_device to spi_device */
static int st7789_probe(struct spi_device *spi)
{
    struct st7789_priv *priv;
    struct device *dev = &spi->dev;
    int i = 0;
    u8 data[1000];
    spi->mode = SPI_MODE_0;             /* ST7789 operates nicely on Mode 3 */
    spi->bits_per_word = 8;
    spi->max_speed_hz = 4000000;       /* 24 MHz */
    if (spi_setup(spi) < 0) {
        dev_err(dev, "SPI setup failed\n");
        return -EINVAL;
    }
    dev_info(dev, "ST7789: Matching hardware node found!");
    /*allocate memory for driver's private data*/
    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
    {
        return -ENOMEM;
    }
    priv->spi = spi;
    spi_set_drvdata(spi, priv);
    priv->dc_gpio = devm_gpiod_get(dev, "dc", GPIOD_OUT_LOW);
    if (IS_ERR(priv->dc_gpio)) {
        dev_err(dev, "ST7789: Failed to acquire DC GPIO. Error: %ld\n", PTR_ERR(priv->dc_gpio));
        return PTR_ERR(priv->dc_gpio);
    }

    priv->rst_gpio = devm_gpiod_get(dev, "rst", GPIOD_OUT_LOW);
    if (IS_ERR(priv->rst_gpio)) {
        dev_err(dev, "ST7789: Failed to acquire RST GPIO. Error: %ld\n", PTR_ERR(priv->rst_gpio));
        return PTR_ERR(priv->rst_gpio);
    }
    dev_info(dev, "ST7789: Hardware configuration successful!\n");

    /*1. Physical hardware Reset */
    gpiod_set_value(priv->rst_gpio, 1);
    msleep(10);      /*sleep for 10 ms*/
    gpiod_set_value(priv->rst_gpio, 0);
    msleep(20);     /*pull down 20 ms*/
    gpiod_set_value(priv->rst_gpio, 1);
    msleep(120);     /*stable delay */

    /*2. Software Reset */
    /*set internal registers to their default state*/
    gpiod_set_value(priv->dc_gpio, COMMAND);
    lcd_command(priv->spi, 0x01);
    msleep(150);

    /*3. Wake up*/
    /*Wake up charge pumps and oscillators*/
    gpiod_set_value(priv->dc_gpio, COMMAND);
    lcd_command(priv->spi, 0x11);
    msleep(120);

    /*4. Interface Pixel Format*/
    /*0x3A COLMOD*/
    /*0x55 use RGB565*/
    gpiod_set_value(priv->dc_gpio, COMMAND);
    lcd_command(priv->spi, 0x3A);
    gpiod_set_value(priv->dc_gpio, DATA);
    lcd_data(priv->spi, 0x55);

    /*5. Memory Access Control*/
    /*MADCTL*/
    /*Top to bottom, Left to right layout*/
    gpiod_set_value(priv->dc_gpio, COMMAND);
    lcd_command(priv->spi, 0x36);
    gpiod_set_value(priv->dc_gpio, DATA);
    lcd_data(priv->spi, 0x00);

    /*6. Turn display on*/
    gpiod_set_value(priv->dc_gpio, COMMAND);
    lcd_command(priv->spi, 0x20);  /*Normal display mode*/
    gpiod_set_value(priv->dc_gpio, COMMAND);
    lcd_command(priv->spi, 0x29);  /*Main screen ON*/
    msleep(20);

    pr_info("Initialization completed\n");
    
    gpiod_set_value(priv->dc_gpio, COMMAND);
    lcd_command(spi, 0x2A);
    gpiod_set_value(priv->dc_gpio, DATA);
    data[0] = 0;
    data[1] = 100;
    data[2] = 0;
    data[3] = 200;
    spi_write(spi, &data, 4);


    /*7. set data to display RAM*/
    // RAMWR
    gpiod_set_value(priv->dc_gpio, COMMAND);
    lcd_command(spi, 0x2C);

    gpiod_set_value(priv->dc_gpio, DATA);
    for (i = 0; i < 1000; i++)
    {
        data[i++] = 0xF8;
        data[i] = 0x1F;
    }
    for (i = 0; i < 20; i++)
        spi_write(spi, &data, 1000);

    // for (i = 0; i < 6; i++) {
    //     gpiod_set_value(priv->dc_gpio, COMMAND);
    //     if (i % 2 == 0) {
    //         lcd_command(priv->spi, 0x21); /* INVON: Enter Inverted Color Mode */
    //     } else {
    //         lcd_command(priv->spi, 0x20); /* INVOFF: Exit Inverted Color Mode */
    //     }
    //     msleep(500); /* Flash color state every half second */
    // }
    
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