#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>


static dev_t dev_num;
static struct cdev button;
static struct class *button_class;

#define BUTTON_GPIO     38


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
    pr_info("Button read\n");
    return 0;
}

struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
};

static int __init button_init(void)
{
    int ret = 0;
    alloc_chrdev_region(&dev_num, 0, 1, "mybutton");
    cdev_init(&button, &fops);
    cdev_add(&button, dev_num, 1);
    button_class = class_create(THIS_MODULE, "mybutton");
    device_create(button_class, NULL, dev_num, NULL, "BUTTON");
    pr_info("Button gpio%d has been initalized\n", BUTTON_GPIO);
    return ret;
}

static void __exit button_exit(void)
{
    device_destroy(button_class, dev_num);
    class_destroy(button_class);
    cdev_del(&button);
    unregister_chrdev_region(dev_num, 1);
    pr_info("Button gpio%d has been deinitialized\n", BUTTON_GPIO);
}

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("Driver for button");