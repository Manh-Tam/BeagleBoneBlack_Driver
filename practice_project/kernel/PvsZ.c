#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>       /* Required for spi_device and spi_driver */
#include <linux/gpio/consumer.h> /* Modern GPIO API */
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/of_device.h>       // of_match_device

#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/poll.h>           // poll()
#include <linux/wait.h>           // wake_up() and wait()

static dev_t dev_num;
static struct class *class;

static wait_queue_head_t button_wait;
static bool lcd_touched = false;

/* private device structure */
struct st7789_priv {
    struct cdev cdev;
    struct spi_device *spi;
    struct gpio_desc *dc_gpio;
    struct gpio_desc *rst_gpio;
};

#define COMMAND     0
#define DATA        1

static irqreturn_t touch_irq_thread(int irq, void *dev_id)
{
    pr_info("touch_irq_thread\n");
    return IRQ_HANDLED;
}

static irqreturn_t touch_irq_handler(int irq, void *dev_id)
{
    // Schedule threaded handler

    unsigned long current_time = jiffies_to_msecs(jiffies);
    static unsigned long last_time = 0;
    unsigned long elapsed_time = 20;
    if (current_time - last_time >= 100)
    {
        last_time = current_time;
        lcd_touched = true;
        wake_up_interruptible(&button_wait);
    }
    return IRQ_WAKE_THREAD; 
}

static void lcd_command(struct spi_device *spi, u8 command)
{
    spi_write(spi, &command, 1);
}

static void lcd_data(struct spi_device *spi, u8 data)
{
    spi_write(spi, &data, 1);
}

static int my_open(struct inode *inode, struct file *file)
{
    struct st7789_priv *p_st7789;
    p_st7789 = container_of(inode->i_cdev, struct st7789_priv, cdev);
    file->private_data = p_st7789;
    pr_info("My open was called\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("My release was called\n");
    return 0;
}

static int spi_read_bytes(struct spi_device *spi, u8 *tx_buf, u8 *rx_buf, size_t len)
{
    struct spi_transfer t = {
        .tx_buf = tx_buf,     // No data to send
        .rx_buf = rx_buf,   // Buffer to store received data
        .len    = len,      // Number of bytes to read
    };
    struct spi_message m;

    spi_message_init(&m);
    spi_message_add_tail(&t, &m);

    return spi_sync(spi, &m); // Blocking call
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct st7789_priv *p_st7789;
    // u16 raw_adc;
    // uint8_t tx[3] = { 0x00, 0x00, 0x00 };
    u8 rx[2] = { 0x00, 0x00};
    
    p_st7789 = file->private_data;
    pr_info("My read was called\n");
    // tx[0] = 0x90;
    // devm_gpiod_set(p_st7789->cs1, 0);
    // raw_adc = spi_read_bytes(p_st7789->spi, tx, rx, 3);
    // devm_gpiod_set(p_st7789->cs1, 1);
    // dev_info(&p_st7789->spi->dev, "Read bytes: %*ph\n", (int)sizeof(rx), rx);
    // tx[0] = 0xD0;
    // devm_gpiod_set(p_st7789->cs1, 0);
    // raw_adc = spi_read_bytes(p_st7789->spi, tx, rx, 3);
    // devm_gpiod_set(p_st7789->cs1, 1);
    // dev_info(&p_st7789->spi->dev, "Read bytes: %*ph\n", (int)sizeof(rx), rx);
    char value = 'a';
    u8 cmd = 0xD0;
    pr_info("my_read called from poll()\n");
    spi_write_then_read(p_st7789->spi, &cmd, 1, rx, 2);
    lcd_touched = false;
    if (copy_to_user(buf, rx, sizeof(rx)))
    {
        pr_err("copy to user\n");
        return -EINVAL;
    }
    return 0;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    int i = 0;
    size_t cnt = count;
    u8 data[800];
    struct st7789_priv *p_st7789;
    *ppos = 0;
    // size_t num = (count + 799) / 800;

    if (count <= 0)
        return 0;

    p_st7789 = file->private_data;

    gpiod_set_value(p_st7789->dc_gpio, COMMAND);
    lcd_command(p_st7789->spi, 0x2A);
    gpiod_set_value(p_st7789->dc_gpio, DATA);
    data[0] = 0;
    data[1] = 0;   /* X start */
    data[2] = 0x01;
    data[3] = 0x3F; /* X end*/
    spi_write(p_st7789->spi, &data, 4);

    gpiod_set_value(p_st7789->dc_gpio, COMMAND);
    lcd_command(p_st7789->spi, 0x2B);
    gpiod_set_value(p_st7789->dc_gpio, DATA);
    data[0] = 0;
    data[1] = 0;     /* Y start */
    data[2] = 0;
    data[3] = 0xEF;     /* Y end */
    spi_write(p_st7789->spi, &data, 4);

    // gpiod_set_value(p_st7789->dc_gpio, COMMAND);
    // lcd_command(p_st7789->spi, 0x2C);
    // gpiod_set_value(p_st7789->dc_gpio, DATA);
    // memset(data, 0x0E, 200);
    // for (i = 0; i < 1536/2; i++)
    // {
    //     spi_write(p_st7789->spi, &data, 200);
    // }

    pr_info("the number of received bytes: %d\n", count);

    // spi_write(p_st7789->spi, &buf, count);
    // for (i = 0; i < count; i++)
    // {
    //     if (copy_from_user(data, (buf + i * 800), 800))
    //         return -EFAULT;
    //     // for (i = 0; i < 10; i++)
    //     //     pr_info("%d ", data[i]);
    //     // pr_info("\n");
    //     spi_write(p_st7789->spi, &data, 800);
    // }

    // gpiod_set_value(p_st7789->dc_gpio, COMMAND);
    // lcd_command(p_st7789->spi, 0x2A);
    // gpiod_set_value(p_st7789->dc_gpio, DATA);
    // data[0] = 0;
    // data[1] = x;   /* X start */
    // data[2] = 0;
    // data[3] = x + 39; /* X end*/
    // spi_write(p_st7789->spi, &data, 4);

    // gpiod_set_value(p_st7789->dc_gpio, COMMAND);
    // lcd_command(p_st7789->spi, 0x2B);
    // gpiod_set_value(p_st7789->dc_gpio, DATA);
    // data[0] = 0;
    // data[1] = 0;     /* Y start */
    // data[2] = 0;
    // data[3] = 39;     /* Y end */
    // spi_write(p_st7789->spi, &data, 4);

    gpiod_set_value(p_st7789->dc_gpio, COMMAND);
    lcd_command(p_st7789->spi, 0x2C);
    gpiod_set_value(p_st7789->dc_gpio, DATA);
    i = 0;
    cnt = count;
    while (cnt > 0)
    {
        if (cnt > 800)
        {
            if (copy_from_user(data, (buf + i * 800), 800))
                return -EFAULT;
            *ppos += 800;
        }
        else
        {
            if (copy_from_user(data, (buf + i * 800), cnt))
                return -EFAULT;
            *ppos += cnt;
        }
        i++;
        cnt -= 800;
        spi_write(p_st7789->spi, &data, 800);
    }
    pr_info("\n");
    // spi_write(p_st7789->spi, &data, 800);

    pr_info("My spi write was called\n");
    return *ppos;
}

static __poll_t my_poll(struct file* file, poll_table *wait)
{
    pr_info("my poll called\n");
    poll_wait(file, &button_wait, wait);
    if (lcd_touched)
    {
        return POLLIN;
    }
    return 0;
}

struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .poll = my_poll,
};

static const struct of_device_id my_of_match[];

/* 1. Changed parameter type from platform_device to spi_device */
static int st7789_probe(struct spi_device *spi)
{
    int ret = 0;
    struct st7789_priv *priv;
    struct device *dev = &spi->dev;
    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    priv->spi = spi;
    #if 0
    // int i = 0, j = 0;
    // u8 data[1000];
    spi->mode = SPI_MODE_0;             /* ST7789 operates nicely on Mode 0 */
    spi->bits_per_word = 8;
    spi->max_speed_hz = 24000000;       /* 24 MHz */
    if (spi_setup(spi) < 0) {
        dev_err(dev, "SPI setup failed\n");
        return -EINVAL;
    }
    dev_info(dev, "ST7789: Matching hardware node found!");
    /*allocate memory for driver's private data*/
    if (!priv)
    {
        return -ENOMEM;
    }
    priv->spi = spi;
    
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
    lcd_data(priv->spi, 0xa0);

    /*6. Turn display on*/
    gpiod_set_value(priv->dc_gpio, COMMAND);
    lcd_command(priv->spi, 0x20);  /*Normal display mode*/
    gpiod_set_value(priv->dc_gpio, COMMAND);
    lcd_command(priv->spi, 0x29);  /*Main screen ON*/
    msleep(20);
    #endif

    // gpiod_set_value(priv->dc_gpio, COMMAND);
    // lcd_command(spi, 0x2A);
    // gpiod_set_value(priv->dc_gpio, DATA);
    // data[0] = 0;
    // data[1] = 160;
    // data[2] = 0;
    // data[3] = 200;
    // spi_write(spi, &data, 4);

    // gpiod_set_value(priv->dc_gpio, COMMAND);
    // lcd_command(spi, 0x2B);
    // gpiod_set_value(priv->dc_gpio, DATA);
    // data[0] = 0;
    // data[1] = 200;
    // data[2] = 0;
    // data[3] = 240;
    // spi_write(spi, &data, 4);

    /*7. set data to display RAM*/
    // RAMWR
    // gpiod_set_value(priv->dc_gpio, COMMAND);
    // lcd_command(spi, 0x2C);

    // gpiod_set_value(priv->dc_gpio, DATA);
    // data[0] = 0xF8;
    // // data[0] = 0x12;
    // data[1] = 0x1F;
    // // for (i = 0; i < 76800; i++)
    // for (i = 0; i < 6; i++)
    // {
    //     gpiod_set_value(priv->dc_gpio, COMMAND);
    //     lcd_command(spi, 0x2A);
    //     gpiod_set_value(priv->dc_gpio, DATA);
    //     data[0] = 0;
    //     data[1] = i * 40;   /* X start */
    //     data[2] = 0;
    //     data[3] = ((i + 1) * 40) - 1; /* X end*/
    //     spi_write(spi, &data, 4);
    //     for (j = 0; j < 8; j++)
    //     {
    //         gpiod_set_value(priv->dc_gpio, COMMAND);
    //         lcd_command(spi, 0x2B);
    //         gpiod_set_value(priv->dc_gpio, DATA);
    //         u16 y_s = (j * 40);
    //         u16 y_e = ((j + 1) * 40) - 1;
    //         data[0] = y_s >> 8;
    //         data[1] = y_s & 0xff;     /* Y start */
    //         data[2] = y_e >> 8;
    //         data[3] = y_e & 0xff;     /* Y end */
    //         spi_write(spi, &data, 4);

    //         gpiod_set_value(priv->dc_gpio, COMMAND);
    //         lcd_command(spi, 0x2C);

    //         gpiod_set_value(priv->dc_gpio, DATA);
    //         if (j % 2 == 0)
    //         {
    //             if (i % 2 == 0)
    //                 memset(data, 0x00, 800);
    //             else
    //                 memset(data, 0x77, 800);

    //             spi_write(spi, &data, 800);
    //             spi_write(spi, &data, 800);
    //             spi_write(spi, &data, 800);
    //             spi_write(spi, &data, 800);
    //         }
    //         else
    //         {
    //             if (i % 2 == 0)
    //                 memset(data, 0x77, 800);
    //             else
    //                 memset(data, 0x00, 800);
    //             spi_write(spi, &data, 800);
    //             spi_write(spi, &data, 800);
    //             spi_write(spi, &data, 800);
    //             spi_write(spi, &data, 800);
    //         }
    //     }
    // }
            
    #if 1
    ret = alloc_chrdev_region(&dev_num, 0, 1, "st7789");
    if (ret)
    {
        return ret;
    }
    class = class_create(THIS_MODULE, "st7789");
    if (IS_ERR(class))
    {
        ret = PTR_ERR(class);
        return ret;
    }
    cdev_init(&priv->cdev, &fops);
    ret = cdev_add(&priv->cdev, dev_num, 1);
    if (ret)
    {
        return ret;
    }
    device_create(class, NULL, dev_num, NULL, "st7789");
    spi_set_drvdata(spi, priv);
    #endif
    const struct of_device_id *match;
    match = of_match_device(my_of_match, &spi->dev);
    switch((uintptr_t)match->data)
    {
        case 0:
        {
            pr_info("display\n");
            break;
        }
        case 1:
        {
            int ret = 0;
            int i = 0;
            u8 cmd = 0x90;
            u8 rx[2];
            struct gpio_desc *pendown;

            pr_info("touch\n");
            spi->mode = SPI_MODE_0;             /* xpt2046 operates nicely on Mode 0 */
            spi->bits_per_word = 8;
            spi->max_speed_hz = 4000000;       /* 4 MHz */
            if (spi_setup(spi) < 0) {
                dev_err(&spi->dev, "SPI setup failed\n");
                return -EINVAL;
            }
            for (i = 0; i < 10; i++)
            {
                ret = spi_write_then_read(spi, &cmd, 1, rx, 2);
                if (ret)
                {
                    return -EINVAL;
                }
                else
                {
                    pr_info("rx: 0x%x%x", rx[0], rx[1]);
                }
                msleep(10);
            }
            /*register interrupt*/
            pendown = devm_gpiod_get(&spi->dev, "tirq", GPIOD_IN);
            if (IS_ERR(pendown))
            {
                return PTR_ERR(pendown);
            }
            pr_info("get tirq\n");
            ret = devm_request_threaded_irq(&spi->dev,
                                            spi->irq,
                                            touch_irq_handler,
                                            touch_irq_thread,
                                            IRQF_TRIGGER_FALLING,
                                            dev_name(&spi->dev),
                                            NULL);
            if (ret)
            {
                pr_err("irq failed\n");
                return ret;
            }
            init_waitqueue_head(&button_wait);
            lcd_touched = false;
            break;
        }
    }
    pr_info("Initialization completed\n");
    return 0;
}

/* 2. Changed parameter type from platform_device to spi_device */
static int st7789_remove(struct spi_device *spi)
{
    struct st7789_priv *p_dev;
    p_dev = spi_get_drvdata(spi);
    device_destroy(class, dev_num);
    cdev_del(&p_dev->cdev);
    class_destroy(class);
    unregister_chrdev_region(dev_num, 1);
    pr_info("st7789 removed\n");
    return 0;
}

static const struct of_device_id my_of_match[] = 
{
    // {
    //     .compatible = "custom,st7789v",
    //     .data = (void*)0,
    // },
    {
        .compatible = "custom,xpt2046",
        .data = (void*)1,
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