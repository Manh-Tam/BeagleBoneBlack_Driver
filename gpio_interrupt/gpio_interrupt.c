#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

/* P8_8 */
#define GPIO_NUM        67 

static dev_t dev_num;
static struct cdev  cdev;
static struct class *my_class;
static int irq;
static bool is_button_pressed = false;
static wait_queue_head_t button_waitqueue;

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
    ssize_t ret = 0;
    char value = '1';
    size_t size = 0;
    size = min(sizeof(value), count);
    pr_info("Wating for button press ...\n");
    wait_event_interruptible(button_waitqueue, is_button_pressed == true);
    ret = copy_to_user(buf, &value, sizeof(value));
    if (ret != 0)
    {
        pr_err("copy_to_user failed: %d\n", ret);
        return -EFAULT;
    }
    is_button_pressed = false;
    return size;
}

static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
    static unsigned long last_interrupt = 0;
    if (jiffies_to_msecs(jiffies) >= last_interrupt + 20)
    {
        pr_info("Button pressed\n");
        is_button_pressed = true;
        wake_up_interruptible(&button_waitqueue);
        last_interrupt = jiffies_to_msecs(jiffies);
    }
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
    struct device *my_device = NULL;
    ret = alloc_chrdev_region(&dev_num, 0, 1, "dev_num");
    if (ret < 0)
    {
        pr_err("alloc_chrdev_region failed: %d\n", ret);
        return ret;
    }
    cdev_init(&cdev, &fops);
    ret = cdev_add(&cdev, dev_num, 1);
    if (ret < 0)
    {
        unregister_chrdev_region(dev_num, 1);
        pr_err("cdev_add failed: %d", ret);
        return ret;
    }
    
    my_class = class_create(THIS_MODULE, "my_class");
    if (IS_ERR(my_class))
    {
        ret = PTR_ERR(my_class);
        cdev_del(&cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("class_create failed: %d\n", ret);
        return ret;
    }
    my_device = device_create(my_class, NULL, dev_num, NULL, "gpio_interrupt");
    if (IS_ERR(my_device))
    {
        ret = PTR_ERR(my_device);
        class_destroy(my_class);
        cdev_del(&cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("device_create failed: %d\n", ret);
        return ret;
    }
    if (gpio_is_valid(GPIO_NUM))
    {
        ret = gpio_request(GPIO_NUM, "gpio67");
    }
    else
    {
        device_destroy(my_class, dev_num);
        class_destroy(my_class);
        cdev_del(&cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("gpio_is_valid failed\n");
        return -EINVAL;
    }
    if (ret < 0)
    {
        device_destroy(my_class, dev_num);
        class_destroy(my_class);
        cdev_del(&cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("gpio_request failed: %d\n", ret);
        return ret;
    }
    ret = gpio_direction_input(GPIO_NUM);
    if (ret < 0)
    {
        gpio_free(GPIO_NUM);
        device_destroy(my_class, dev_num);
        class_destroy(my_class);
        cdev_del(&cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("gpio_direction_input failed: %d\n", ret);
        return ret;
    }
    irq = gpio_to_irq(GPIO_NUM);
    if (irq < 0)
    {
        ret = irq;
        gpio_free(GPIO_NUM);
        device_destroy(my_class, dev_num);
        class_destroy(my_class);
        cdev_del(&cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("gpio_to_irq failed: %d\n", ret);
        return ret;
    }
    ret = request_irq(irq, button_irq_handler, IRQF_TRIGGER_FALLING, "button handler", NULL);
    if (ret < 0)
    {
        gpio_free(GPIO_NUM);
        device_destroy(my_class, dev_num);
        class_destroy(my_class);
        cdev_del(&cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("request_irq failed: %d\n", ret);
        return ret;
    }
    init_waitqueue_head(&button_waitqueue);
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