#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/wait.h>

#define BUTTON_GPIO     66  /* P8_7 */

static dev_t button_num;
static struct cdev button;
static struct class *button_class;
static struct device *device;
static struct file_operations fops;
static wait_queue_head_t button_wait_queue;
static bool button_pressed;
int irq = -1;


static int __init button_init(void);
static void __exit button_exit(void);
static int button_open(struct inode *inode, struct file *file);
static int button_release(struct inode *inode, struct file *file);
static ssize_t button_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t button_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static irqreturn_t button_irq_handler(int irq, void *dev_id);

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("button");

static struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = button_open,
    .release = button_release,
    .read = button_read,
    .write = button_write,
};

static int __init button_init(void)
{
    int ret = -1;
    printk("button_init: OK\n");
    ret = alloc_chrdev_region(&button_num, 0, 1, "mybutton");
    if (0 == ret)
    {
        button_class = class_create(THIS_MODULE, "button_class");
        if (IS_ERR(button_class))
        {
            ret = PTR_ERR(button_class);
        }
    }
    if (0 == ret)
    {
        cdev_init(&button, &fops);
        ret = cdev_add(&button, button_num, 1);
    }
    if (0 == ret)
    {
        device = device_create(button_class, NULL, button_num, NULL, "button");
        if (IS_ERR(device))
        {
            ret = PTR_ERR(device);
        }
    }

    /*initialize gpio*/
    if (ret == 0)
    {
        ret = !gpio_is_valid(BUTTON_GPIO);
    }

    if (ret == 0)
    {
        ret = gpio_request(BUTTON_GPIO, "button");
    }

    if (ret == 0)
    {
        ret = gpio_direction_input(BUTTON_GPIO);
    }
    
    if (ret == 0)
    {
        irq = gpio_to_irq(BUTTON_GPIO);
        ret = request_irq(irq, button_irq_handler, IRQF_TRIGGER_FALLING, "button handler", NULL);
    }

    init_waitqueue_head(&button_wait_queue);
    button_pressed = false;
    return ret;
}

static void __exit button_exit(void)
{
    free_irq(irq, NULL);
    gpio_free(BUTTON_GPIO);
    device_destroy(button_class, button_num);
    class_destroy(button_class);
    cdev_del(&button);
    unregister_chrdev_region(button_num, 1);
    printk("button_exit: OK\n");
}

static int button_open(struct inode *inode, struct file *file)
{
    printk("button_open: OK\n");
    return 0;
}

static int button_release(struct inode *inode, struct file *file)
{
    printk("button_release: OK\n");
    return 0;
}

/*return value = 0: EOF*/
/*return value > 0: num of read bytes*/
/*return value < 0: error*/
static ssize_t button_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int value;
    printk("button_read: OK\n");
    wait_event_interruptible(button_wait_queue, button_pressed == true);
    button_pressed = false;
    value = gpio_get_value(BUTTON_GPIO);
    if (copy_to_user(buf, &value, sizeof(value)))
    {
        return -1;
    }
    return 0;
}

/*return value < 0: error*/
/*return value >= 0: num of written bytes*/
static ssize_t button_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    printk("button_write: OK\n");
    return count;
}

static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
    static unsigned long timeout = 0;
    if (time_after(jiffies, timeout))
    {
        timeout = jiffies + msecs_to_jiffies(20);
        button_pressed = true;
        wake_up_interruptible(&button_wait_queue);
        printk("Button pressed\n");
    }
    return 0;
}
