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

/* Support multiple devices */
#include <linux/slab.h>     // kzalloc

/* GPIO APIs */
#include <linux/gpio.h>     
#include <linux/gpio/consumer.h>    // gpio_get_value

#include <linux/of_device.h>       // of_match_device

/* Kernel module */
static int __init my_init(void);
static void __exit my_exit(void);

/* Chacter driver */
static dev_t first_dev;
static struct class *class;
static int id = 0;

enum dev_type
{
    DEV_LED,
    DEV_BUTTON,
};

struct my_device
{
    struct cdev cdev;
    dev_t dev_num;
    struct gpio_desc *gpiod;
    enum dev_type type;
};

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
        .data = (void*)DEV_BUTTON,
    },
    {
        .compatible = "tam,led0",
        .data = (void*)DEV_LED,
    },
    {
        .compatible = "tam,led1",
        .data = (void*)DEV_LED,
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
    alloc_chrdev_region(&first_dev, 0, 32, "my_chr_driver");
    class = class_create(THIS_MODULE, "my_chr_class");

    pr_info("Hello from init\n");
    return platform_driver_register(&my_driver);
}

static void __exit my_exit(void)
{
    platform_driver_unregister(&my_driver);
    class_destroy(class);
    unregister_chrdev_region(first_dev, 1);
    pr_info("Goodbye from exit\n");
}

static int my_open(struct inode *inode, struct file *file)
{
    struct my_device *mdev;
    mdev = container_of(inode->i_cdev, struct my_device, cdev);
    file->private_data = mdev;
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
    struct my_device *mdev;
    int status = -1;
    mdev = file->private_data;
    pr_info("read data addr: %p\n", mdev);
    status = gpiod_get_value(mdev->gpiod);
    pr_info("Status of gpio: %d\n", status);
    pr_info("my_read is called\n");
    return 0;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    int direction = -1;
    struct my_device *mdev;
    mdev = file->private_data;
    direction = gpiod_get_direction(mdev->gpiod);
    if (0 == direction)
    {
        pr_info("the pin is gpio input\n");
    }
    else if (1 == direction)
    {
        pr_info("the pin is gpio output\n");
    }
    else
    {
        pr_info("Error: gpio get diretion\n");
    }
    // pr_info("my_write is called\n");
    return count;
}

static int probe(struct platform_device *pdev)
{
    struct my_device *mdev;
    struct device_node *np;
    const struct of_device_id *match;
    
    np = pdev->dev.of_node;
    pr_info("node name: %s\n", np->name);
    mdev = kzalloc(sizeof(*mdev), GFP_KERNEL);
    cdev_init(&mdev->cdev, &fops);
    mdev->dev_num = MKDEV(MAJOR(first_dev), id);
    cdev_add(&mdev->cdev, mdev->dev_num, 1);
    device_create(class, NULL, mdev->dev_num, NULL, "my_device%d", id);

    match = of_match_device(my_of_match, &pdev->dev);
    if (NULL == match)
    {
        return -ENODEV;
    }

    switch ((uintptr_t)match->data)
    {
        case DEV_LED:
        {
            mdev->gpiod = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_HIGH);
            if (IS_ERR(mdev->gpiod))
            {
                pr_err("Error get button\n");
                return PTR_ERR(mdev->gpiod);
            }
            pr_info("led%d initialized\n", id);
            break;
        }
        case DEV_BUTTON:
        {
            mdev->gpiod = devm_gpiod_get(&pdev->dev, "button", GPIOD_IN);
            if (IS_ERR(mdev->gpiod))
            {
                pr_err("Error get button\n");
                return PTR_ERR(mdev->gpiod);
            }
            pr_info("button%d initialized\n", id);
            break;
        }
        default:
            pr_err("Error get device\n");
            return -EINVAL;
    }

    platform_set_drvdata(pdev, mdev);
    pr_info("Probe is called %d\n", id);
    id++;
    return 0;
}

static int remove(struct platform_device *pdev)
{
    struct my_device *mdev;

    mdev = platform_get_drvdata(pdev);
    device_destroy(class, mdev->dev_num);
    cdev_del(&mdev->cdev);
    pr_info("Remove is called\n");
    return 0;
}