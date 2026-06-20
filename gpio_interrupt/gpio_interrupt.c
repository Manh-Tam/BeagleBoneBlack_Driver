#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>

/* P8_8 */
#define GPIO_NUM        67 

static dev_t dev_num;
static struct cdev  cdev;
static struct class *my_class;
static int irq;

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("GPIO interrupt opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("GPIO interrupt release\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    pr_info("GPIO interrupt read\n");
    return 0;
}

static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
    pr_info("Button pressed\n");
    return 0;
}

struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
};

static int __init gpio_interrupt_init(void)
{
    int ret = -1;
    struct device *device = NULL;
    ret = alloc_chrdev_region(&dev_num, 0, 1, "dev_num");
    if (ret < 0)
    {
        pr_err("alloc_chdev_region failed: %d", ret);
        return ret;
    }
    cdev_init(&cdev, &fops);
    ret = cdev_add(&cdev, dev_num, 1);
    if (ret < 0)
    {
        pr_err("cdev_init failed: %d", ret);
        return ret;
    }
    
    my_class = class_create(THIS_MODULE, "my_class");
    if (IS_ERR(my_class))
    {
        ret = PTR_ERR(my_class);
        pr_err("class_create failed: %d\n", ret);
        return ret;
    }
    device = device_create(my_class, NULL, dev_num, NULL, "gpio_interrupt");
    if (IS_ERR(device))
    {
        ret = PTR_ERR(device);
        pr_err("device_create failed: %d\n", ret);
        return ret;
    }
    if (gpio_is_valid(GPIO_NUM))
    {
        ret = gpio_request(GPIO_NUM, "gpio67");
    }
    else
    {
        pr_err("gpio_is_valid failed\n");
        return -1;
    }
    if (ret < 0)
    {
        pr_err("gpio_request failed: %d\n", ret);
        return ret;
    }
    ret = gpio_direction_input(GPIO_NUM);
    if (ret < 0)
    {
        pr_err("gpio_direction_input failed: %d\n", ret);
        return ret;
    }
    irq = gpio_to_irq(GPIO_NUM);
    ret = request_irq(irq, button_irq_handler, IRQF_TRIGGER_FALLING, "button handler", NULL);
    pr_info("GPIO interrupt initialized\n");
    return 0;
}

static void __exit gpio_interrupt_exit(void)
{
    free_irq(irq, NULL);
    gpio_free(GPIO_NUM);
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("GPIO interrupt exitted");
}

module_init(gpio_interrupt_init);
module_exit(gpio_interrupt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("GPIO interrupt driver");