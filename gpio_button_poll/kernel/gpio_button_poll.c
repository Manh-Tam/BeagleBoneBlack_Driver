#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#define KBUF_SIZE       100
#define GPIO_IN         67
#define GPIO_OUT        60

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;

static int irq;
static wait_queue_head_t button_wait;
static bool button_pressed;

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("gpio button poll opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("gpio button poll released\n");
    return 0;
}

/* Return value */
/* > 0: read size*/
/* = 0: EOF */
/* < 0: error */
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char value;
    gpio_set_value(GPIO_OUT, 0);
    button_pressed = false;
    value = '1';
    if (copy_to_user(buf, &value, sizeof(value)))
    {
        pr_err("copy to user\n");
        return -EINVAL;
    }
    pr_info("gpio button poll read\n");
    return sizeof(value);
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf[KBUF_SIZE];
    if (count >= KBUF_SIZE)
    {
        pr_err("invalid write size\n");
        return -EINVAL;
    }
    if (copy_from_user(kbuf, buf, count))
    {
        pr_err("copy_to_user failed\n");
        return -EFAULT;
    }
    kbuf[count] = '\0';
    gpio_set_value(GPIO_OUT, 1);
    pr_info("gpio button poll written: %s\n", kbuf);
    return count;
}

static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
    pr_info("Button pressed\n");
    button_pressed = true;
    wake_up_interruptible(&button_wait);
    return IRQ_HANDLED;
}

/*
    Function: my_poll
    Arguments:
    file: 
    wait: 
    Return value:
    POLLIN:  indicates data available for reading
    POLLOUT: indicates data available for writing
    POLLERR: indicates error
    POLLUP:  indicates device disconnected
*/
static __poll_t my_poll(struct file *file, poll_table *wait)
{
    pr_info("gpio button poll polling\n");
    poll_wait(file, &button_wait, wait);
    if (button_pressed)
    {
        return POLLIN;
    }
    return 0;
}

static struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .poll = my_poll,
};

static int __init gpio_button_poll_init(void)
{
    int ret = 0;
    struct device *my_device = NULL;
    bool is_chrdev_region_allocated = false;
    bool is_cdev_added = false;
    bool is_class_created = false;
    // bool is_device_created = false;
    ret = alloc_chrdev_region(&dev_num, 0, 1, "my_device");
    if (ret == 0)
    {
        is_chrdev_region_allocated = true;
        cdev_init(&my_cdev, &fops);
        ret = cdev_add(&my_cdev, dev_num, 1);
    }
    if (ret == 0)
    {
        is_cdev_added = true;
        my_class = class_create(THIS_MODULE, "GBP_class");
        if (IS_ERR(my_class))
        {
            ret = PTR_ERR(my_class);
        }
    }
    if (ret == 0)
    {
        is_class_created = true;
        my_device = device_create(my_class, NULL, dev_num, NULL, "gpio_button_poll");
        if (IS_ERR(my_device))
        {
            ret = PTR_ERR(my_device);
        }
    }

    if (ret != 0)
    {
        if (is_class_created)
        {
            class_destroy(my_class);
        }
        if (is_cdev_added)
        {
            cdev_del(&my_cdev);
        }
        if (is_chrdev_region_allocated)
        {
            unregister_chrdev_region(dev_num, 1);
        }
        pr_err("gpio button poll init failed\n");
        return ret;
    }
    gpio_request(GPIO_IN, "button_gpio");
    gpio_direction_input(GPIO_IN);
    irq = gpio_to_irq(GPIO_IN);
    ret = request_irq(irq, button_irq_handler, IRQF_TRIGGER_FALLING, "button handler", NULL);
    gpio_request(GPIO_OUT, "led_gpio");
    gpio_direction_output(GPIO_OUT, 0);
    init_waitqueue_head(&button_wait);
    button_pressed = false;
    pr_info("gpio button poll initialized successfully\n");
    return 0;
}

static void __exit gpio_button_poll_exit(void)
{
    free_irq(irq, NULL);
    gpio_free(GPIO_OUT);
    gpio_free(GPIO_IN);
	device_destroy(my_class, dev_num);
	class_destroy(my_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev_num, 1);
    pr_info("gpio button poll exitted\n");
}

module_init(gpio_button_poll_init);
module_exit(gpio_button_poll_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("gpio button poll driver");