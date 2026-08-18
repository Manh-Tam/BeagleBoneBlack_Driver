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

static dev_t first_dev_num;
static struct class *class;

static wait_queue_head_t button_wait;
static bool lcd_touched = false;
static bool class_created = false;

/* private device structure */
struct lcd_priv {
    struct cdev cdev;
    dev_t dev_num;
    struct spi_device *spi;
    struct gpio_desc *dc_gpio;
    struct gpio_desc *rst_gpio;
};

struct touch_priv {
    struct cdev cdev;
    dev_t dev_num;
    struct spi_device *spi;
    struct gpio_desc *irq_gpio;
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
    unsigned long elapsed_time = 100;
    if (current_time - last_time >= elapsed_time)
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
    struct lcd_priv *p_lcd;
    p_lcd = container_of(inode->i_cdev, struct lcd_priv, cdev);
    file->private_data = p_lcd;
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
    struct lcd_priv *p_lcd;
    // u16 raw_adc;
    // uint8_t tx[3] = { 0x00, 0x00, 0x00 };
    u8 rx[4] = { 0x00, 0x00};
    u8 cmd = 0x00;
    
    p_lcd = file->private_data;
    pr_info("My read was called\n");
    // tx[0] = 0x90;
    // devm_gpiod_set(p_lcd->cs1, 0);
    // raw_adc = spi_read_bytes(p_lcd->spi, tx, rx, 3);
    // devm_gpiod_set(p_lcd->cs1, 1);
    // dev_info(&p_lcd->spi->dev, "Read bytes: %*ph\n", (int)sizeof(rx), rx);
    // tx[0] = 0xD0;
    // devm_gpiod_set(p_lcd->cs1, 0);
    // raw_adc = spi_read_bytes(p_lcd->spi, tx, rx, 3);
    // devm_gpiod_set(p_lcd->cs1, 1);
    // dev_info(&p_lcd->spi->dev, "Read bytes: %*ph\n", (int)sizeof(rx), rx);
    cmd = 0xD0;
    pr_info("get x coordinate\n");
    spi_write_then_read(p_lcd->spi, &cmd, 1, rx, 2);
    cmd = 0x90;
    pr_info("get y coordinate\n");
    spi_write_then_read(p_lcd->spi, &cmd, 1, &rx[2], 2);
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
    struct lcd_priv *p_lcd;
    *ppos = 0;
    // size_t num = (count + 799) / 800;

    if (count <= 0)
        return 0;

    p_lcd = file->private_data;

    gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    lcd_command(p_lcd->spi, 0x2A);
    gpiod_set_value(p_lcd->dc_gpio, DATA);
    data[0] = 0;
    data[1] = 0;   /* X start */
    data[2] = 0x01;
    data[3] = 0x3F; /* X end*/
    spi_write(p_lcd->spi, &data, 4);

    gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    lcd_command(p_lcd->spi, 0x2B);
    gpiod_set_value(p_lcd->dc_gpio, DATA);
    data[0] = 0;
    data[1] = 0;     /* Y start */
    data[2] = 0;
    data[3] = 0xEF;     /* Y end */
    spi_write(p_lcd->spi, &data, 4);

    // gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    // lcd_command(p_lcd->spi, 0x2C);
    // gpiod_set_value(p_lcd->dc_gpio, DATA);
    // memset(data, 0x0E, 200);
    // for (i = 0; i < 1536/2; i++)
    // {
    //     spi_write(p_lcd->spi, &data, 200);
    // }

    pr_info("the number of received bytes: %d\n", count);

    // spi_write(p_lcd->spi, &buf, count);
    // for (i = 0; i < count; i++)
    // {
    //     if (copy_from_user(data, (buf + i * 800), 800))
    //         return -EFAULT;
    //     // for (i = 0; i < 10; i++)
    //     //     pr_info("%d ", data[i]);
    //     // pr_info("\n");
    //     spi_write(p_lcd->spi, &data, 800);
    // }

    // gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    // lcd_command(p_lcd->spi, 0x2A);
    // gpiod_set_value(p_lcd->dc_gpio, DATA);
    // data[0] = 0;
    // data[1] = x;   /* X start */
    // data[2] = 0;
    // data[3] = x + 39; /* X end*/
    // spi_write(p_lcd->spi, &data, 4);

    // gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    // lcd_command(p_lcd->spi, 0x2B);
    // gpiod_set_value(p_lcd->dc_gpio, DATA);
    // data[0] = 0;
    // data[1] = 0;     /* Y start */
    // data[2] = 0;
    // data[3] = 39;     /* Y end */
    // spi_write(p_lcd->spi, &data, 4);

    gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    lcd_command(p_lcd->spi, 0x2C);
    gpiod_set_value(p_lcd->dc_gpio, DATA);
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
        spi_write(p_lcd->spi, &data, 800);
    }
    pr_info("\n");
    // spi_write(p_lcd->spi, &data, 800);

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

static int lcd_init(struct spi_device *spi)
{
    int ret = 0;
    struct device *my_device = NULL;
    struct lcd_priv *p_lcd;
    struct device *p_dev;
    
    p_dev = &spi->dev;
    p_lcd = devm_kzalloc(p_dev, sizeof(*p_lcd), GFP_KERNEL);

    /*1. init character driver for touch*/
    cdev_init(&p_lcd->cdev, &fops);
    p_lcd->dev_num = MKDEV(MAJOR(first_dev_num), 0);
    ret = cdev_add(&p_lcd->cdev, p_lcd->dev_num, 1);
    if (ret)
    {
        return ret;
    }
    my_device = device_create(class, NULL, p_lcd->dev_num, NULL, "lcd");
    if (IS_ERR(my_device))
    {
        pr_err("failed to create my device\n");
        ret = PTR_ERR(my_device);
    }

    /*2. init spi for st7789*/
    p_lcd->spi = spi;
    spi->mode = SPI_MODE_0;             /* st7789 operates nicely on Mode 0 */
    spi->bits_per_word = 8;
    spi->max_speed_hz = 24000000;       /* 4 MHz */
    if (spi_setup(spi) < 0) {
        dev_err(&spi->dev, "SPI setup failed\n");
        return -EINVAL;
    }
    pr_info("spi: ok\n");
    p_lcd->dc_gpio = devm_gpiod_get(p_dev, "dc", GPIOD_OUT_LOW);
    if (IS_ERR(p_lcd->dc_gpio)) {
        dev_err(p_dev, "ST7789: Failed to acquire DC GPIO. Error: %ld\n", PTR_ERR(p_lcd->dc_gpio));
        return PTR_ERR(p_lcd->dc_gpio);
    }
    pr_info("dc: ok\n");
    p_lcd->rst_gpio = devm_gpiod_get(p_dev, "rst", GPIOD_OUT_LOW);
    if (IS_ERR(p_lcd->rst_gpio)) {
        dev_err(p_dev, "ST7789: Failed to acquire RST GPIO. Error: %ld\n", PTR_ERR(p_lcd->rst_gpio));
        return PTR_ERR(p_lcd->rst_gpio);
    }
    pr_info("rst: ok\n");

    /*3. Init lcd*/
    /*3.1. Physical hardware Reset */
    gpiod_set_value(p_lcd->rst_gpio, 1);
    msleep(10);      /*sleep for 10 ms*/
    gpiod_set_value(p_lcd->rst_gpio, 0);
    msleep(20);     /*pull down 20 ms*/
    gpiod_set_value(p_lcd->rst_gpio, 1);
    msleep(120);     /*stable delay */

    /*3.2. Software Reset */
    /*set internal registers to their default state*/
    gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    lcd_command(p_lcd->spi, 0x01);
    msleep(150);

    /*3.3. Wake up*/
    /*Wake up charge pumps and oscillators*/
    gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    lcd_command(p_lcd->spi, 0x11);
    msleep(120);

    /*3.4. Interface Pixel Format*/
    /*0x3A COLMOD*/
    /*0x55 use RGB565*/
    gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    lcd_command(p_lcd->spi, 0x3A);
    gpiod_set_value(p_lcd->dc_gpio, DATA);
    lcd_data(p_lcd->spi, 0x55);

    /*3.5. Memory Access Control*/
    /*MADCTL*/
    /*Top to bottom, Left to right layout*/
    gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    lcd_command(p_lcd->spi, 0x36);
    gpiod_set_value(p_lcd->dc_gpio, DATA);
    lcd_data(p_lcd->spi, 0x60);

    /*3.6. Turn display on*/
    gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    lcd_command(p_lcd->spi, 0x20);  /*Normal display mode*/
    gpiod_set_value(p_lcd->dc_gpio, COMMAND);
    lcd_command(p_lcd->spi, 0x29);  /*Main screen ON*/
    msleep(20);
    pr_info("lcd: ok\n");
    /*4. store driver data*/
    spi_set_drvdata(spi, p_lcd);
    return ret;
}

static int touch_init(struct spi_device *spi)
{
    int ret = 0;
    struct touch_priv *p_touch;
    struct device *my_device = NULL;
    struct device *p_dev;

    p_dev = &spi->dev;
    p_touch = devm_kzalloc(p_dev, sizeof(*p_touch), GFP_KERNEL);
    
    /*1. init character driver for touch*/
    cdev_init(&p_touch->cdev, &fops);
    p_touch->dev_num = MKDEV(MAJOR(first_dev_num), 1);
    ret = cdev_add(&p_touch->cdev, p_touch->dev_num, 1);
    if (ret)
    {
        return ret;
    }
    my_device = device_create(class, NULL, p_touch->dev_num, NULL, "touch");
    if (IS_ERR(my_device))
    {
        pr_err("failed to create my device\n");
        ret = PTR_ERR(my_device);
    }
    
    /*2. init xpt2046 */
    p_touch->spi = spi;
    spi->mode = SPI_MODE_0;             /* xpt2046 operates nicely on Mode 0 */
    spi->bits_per_word = 8;
    spi->max_speed_hz = 2000000;       /* 2 MHz */
    
    if (spi_setup(spi) < 0) {
        dev_err(&spi->dev, "SPI setup failed\n");
        return -EINVAL;
    }
    /*3. register interrupt*/
    p_touch->irq_gpio = devm_gpiod_get(p_dev, "tirq", GPIOD_IN);
    if (IS_ERR(p_touch->irq_gpio))
    {
        return PTR_ERR(p_touch->irq_gpio);
    }
    pr_info("get tirq\n");
    ret = devm_request_threaded_irq(p_dev,
                                    spi->irq,
                                    touch_irq_handler,
                                    touch_irq_thread,
                                    IRQF_TRIGGER_FALLING,
                                    dev_name(p_dev),
                                    NULL);
    if (ret)
    {
        pr_err("irq failed\n");
        return ret;
    }
    init_waitqueue_head(&button_wait);
    lcd_touched = false;

    spi_set_drvdata(spi, p_touch);

    return ret;
}

static int st7789_probe(struct spi_device *spi)
{
    int ret = 0;
    const struct of_device_id *match;
    /*1. Create class for display and touch*/
    if (false == class_created)
    {
        class_created = true;
        ret = alloc_chrdev_region(&first_dev_num, 0, 32, "touch_lcd");
        if (ret)
        {
            return ret;
        }
        class = class_create(THIS_MODULE, "touch_lcd");
        if (IS_ERR(class))
        {
            ret = PTR_ERR(class);
            return ret;
        }
    }

    /*2. Create character driver for each device */
    match = of_match_device(my_of_match, &spi->dev);
    switch((uintptr_t)match->data)
    {
        case 0:
        {
            pr_info("[INFO] Initializing display\n");
            lcd_init(spi);
            pr_info("[INFO] Display initialized\n");
            break;
        }
        case 1:
        {
            pr_info("[INFO] Initializing touch\n");
            touch_init(spi);
            pr_info("[INFO] Touch initialized\n");
            break;
        }
    }
    
    pr_info("[INFO] Initialization completed\n");
    return 0;
}

static int st7789_remove(struct spi_device *spi)
{
    struct lcd_priv *p_dev;
    const struct of_device_id *match;
    match = of_match_device(my_of_match, &spi->dev);
    /*1. Remove each device and remove their class in the end*/
    switch((uintptr_t)match->data)
    {
        case 0:
        {
            p_dev = spi_get_drvdata(spi);
            device_destroy(class, p_dev->dev_num);
            cdev_del(&p_dev->cdev);
            pr_info("[INFO] Device display removed\n");
            break;
        }
        case 1:
        {
            p_dev = spi_get_drvdata(spi);
            device_destroy(class, p_dev->dev_num);
            cdev_del(&p_dev->cdev);
            pr_info("[INFO] Device touch removed\n");
            class_destroy(class);
            unregister_chrdev_region(first_dev_num, 32);
            pr_info("[INFO] Class touch_lcd removed\n");
            break;
        }
    }

    return 0;
}

static const struct of_device_id my_of_match[] = 
{
    {
        .compatible = "custom,st7789v",
        .data = (void*)0,
    },
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