#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <linux/gpio.h>
#include <linux/uaccess.h>

static dev_t dev_num;
static struct cdev button;
static struct class *button_class;

#define BUTTON_GPIO     67  /* P8_8 */

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("Button opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("Button released\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    ssize_t ret = 0;
    char value = 0;
    size_t size = 0;
    
    value = gpio_get_value(BUTTON_GPIO) ? '1' : '0';
    size = min(sizeof(value), count); 
    ret = copy_to_user(buf, &value, size);
    if (ret)
    {
        pr_err("copy_to_user failed: %d\n", ret);
        return ret;
    }
    return count;
}

struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
};

/* two styles of handling return values: early return and classic if-else */
#if 0
static int __init button_init(void)
{
    int ret = 0;
    ret = alloc_chrdev_region(&dev_num, 0, 1, "mybutton");
    if (ret != 0)
    {
        pr_err("Failed to allocate character device region\n");
        return ret;
    }
    cdev_init(&button, &fops);
    ret = cdev_add(&button, dev_num, 1);
    if (ret != 0)
    {
        unregister_chrdev_region(dev_num, 1);
        pr_err("Failed to add device\n");
        return ret;
    }
    button_class = class_create(THIS_MODULE, "mybutton");
    if (IS_ERR(button_class))
    {
        cdev_del(&button);
        unregister_chrdev_region(dev_num, 1);
        ret = PTR_ERR(button_class);
        return ret;
    }
    struct device *button_device;
    button_device = device_create(button_class, NULL, dev_num, NULL, "BUTTON");
    if (IS_ERR(button_device))
    {
        ret = PTR_ERR(button_device);
        class_destroy(button_class);
        cdev_del(&button);
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }
    pr_info("Button gpio%d has been initalized\n", BUTTON_GPIO);
    return ret;
}
#else
static int __init button_init(void)
{
    int ret = 0;
    bool is_chrdev_region_allocated = false;
    bool is_cdev_added = false;
    bool is_class_created = false;
    bool is_device_created = false;
    bool is_gpio_requested = false;
    
    ret = alloc_chrdev_region(&dev_num, 0, 1, "mybutton");

    if (ret == 0)
    {
        is_chrdev_region_allocated = true;
        cdev_init(&button, &fops);
        ret = cdev_add(&button, dev_num, 1);
    }

    if (ret == 0)
    {
        is_cdev_added = true;
        button_class = class_create(THIS_MODULE, "mybutton");
        if (IS_ERR(button_class))
        {
            ret = PTR_ERR(button_class);
        }
    }

    if (ret == 0)
    {
        struct device *button_device;
        is_class_created = true;
        button_device = device_create(button_class, NULL, dev_num, NULL, "BUTTON");
        if (IS_ERR(button_device))
        {
            ret = PTR_ERR(button_device);
        }
    }

    if (ret == 0)
    {
        is_device_created = true;
        ret = !gpio_is_valid(BUTTON_GPIO);
    }

    if (ret == 0)
    {
        ret = gpio_request(BUTTON_GPIO, "button");
    }

    if (ret == 0)
    {
        is_gpio_requested = true;
        ret = gpio_direction_input(BUTTON_GPIO);
    }

    if (ret != 0)
    {
        pr_err("%s failed: %d\n", __func__, ret);
        if (is_gpio_requested)
        {
            gpio_free(BUTTON_GPIO);
        }
        if (is_device_created)
        {
            device_destroy(button_class, dev_num);
        }
        if (is_class_created)
        {
            class_destroy(button_class);
        }
        if (is_cdev_added)
        {
            cdev_del(&button);
        }
        if (is_chrdev_region_allocated)
        {
            unregister_chrdev_region(dev_num, 1);
        }
    }

    return ret;
}
#endif

static void __exit button_exit(void)
{
    device_destroy(button_class, dev_num);
    class_destroy(button_class);
    cdev_del(&button);
    unregister_chrdev_region(dev_num, 1);
    pr_info("Button gpio%d deinitialized\n", BUTTON_GPIO);
}

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("Driver for button");