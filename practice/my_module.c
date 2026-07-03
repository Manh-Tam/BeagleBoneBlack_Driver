/* Kernel module */
#include <linux/kernel.h>
#include <linux/module.h>
/* Character driver */
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
/* Platform driver */
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>

/* Kernel module */
static int __init my_init(void);
static void __exit my_exit(void);

/* Chacter driver */
static dev_t dev_num;
static struct cdev cdev;
static struct class *class;

static int my_open(struct inode *inode, struct file *file);
static int my_release(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);

struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,  
};

/* Platform driver */
static int probe(struct platform_device *pdev);
static int remove(struct platform_device *pdev);

static const struct of_device_id my_of_match[] = 
{
    {
        .compatible = "tam,mybutton",
    },
};

static struct platform_driver my_driver = 
{
    .probe = probe, 
    .remove = remove,
    .driver = 
    {
        .name = "my_platform_driver",
        .of_match_table = my_of_match,
    },
};

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("Practice");

static int __init my_init(void)
{
    alloc_chrdev_region(&dev_num, 0, 1, "my_chr_driver");
    class = class_create(THIS_MODULE, "my_chr_class");
    cdev_init(&cdev, &fops);
    cdev_add(&cdev, dev_num, 1);
    device_create(class, NULL, dev_num, NULL, "my_device");
    pr_info("Hello from init\n");
    return platform_driver_register(&my_driver);
}

static void __exit my_exit(void)
{
    platform_driver_unregister(&my_driver);
    device_destroy(class, dev_num);
    cdev_del(&cdev);
    class_destroy(class);
    unregister_chrdev_region(dev_num, 1);
    pr_info("Goodbye from exit\n");
}

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("hello from my_open\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("goodbye from my_release\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    pr_info("my_read is called\n");
    return 0;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    pr_info("my_write is called\n");
    return count;
}

static int probe(struct platform_device *pdev)
{
    pr_info("Probe is called\n");
    return 0;
}

static int remove(struct platform_device *pdev)
{
    pr_info("Remove is called\n");
    return 0;
}