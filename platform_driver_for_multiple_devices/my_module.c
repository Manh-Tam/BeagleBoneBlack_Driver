#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#include <linux/slab.h>   // For kzalloc and kfree

static int id = 0;
static struct class *class = NULL;
static dev_t first_dev;


struct my_dev_data 
{
    dev_t dev_num;
    struct cdev dev;
};

static int my_open(struct inode *inode, struct file *file)
{
    struct my_dev_data *p_led;
    p_led = container_of(inode->i_cdev,
                   struct my_dev_data,
                   dev);
    file->private_data = p_led;
    pr_info("my open called\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("my release called\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    pr_info("my read called\n");
    return 0;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    pr_info("my write called\n");
    return count;
}

static struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int my_probe(struct platform_device *p_dev)
{
    int ret = 0;
    struct device *my_device = NULL;
    struct my_dev_data *p_led;
    
    p_led = kzalloc(sizeof(*p_led), GFP_KERNEL);

    if (NULL == p_led)
    {
        ret = -ENOMEM;
    }

    if (0 == ret)
    {
        cdev_init(&p_led->dev, &fops);
        p_led->dev_num = MKDEV(MAJOR(first_dev), id);
        ret = cdev_add(&p_led->dev, p_led->dev_num, 1);
    }
    
    if (0 == ret)
    {
        my_device = device_create(class, NULL, p_led->dev_num, NULL, "led%d", id);
        id++;
        if (IS_ERR(my_device))
        {
            pr_err("here\n");
            ret = PTR_ERR(my_device);
        }
    }

    if (0 == ret)
    {
        pr_info("My module initialized\n");
    }

    platform_set_drvdata(p_dev, p_led);
    pr_info("my_probe called\n");
    return 0;
}

static int my_remove(struct platform_device *p_dev)
{
    struct my_dev_data *p_led;
    p_led = platform_get_drvdata(p_dev);

    device_destroy(class, p_led->dev_num);
    cdev_del(&p_led->dev);
    pr_info("my_remove called\n");
    return 0;
}

static const struct of_device_id my_of_match[] = 
{
    {
        .compatible = "tam,led0",
    },
    {
        .compatible = "tam,led1",
    },
    {}
};

static struct platform_driver my_driver = 
{
    .probe = my_probe,
    .remove = my_remove,
    .driver = 
    {
        .name = "my_platform_driver",
        .of_match_table = my_of_match,
    }
};

#if 0
static int __init my_init(void)
{
    int ret = 0;
    struct device *my_device = NULL;
    bool chrdev_region_allocated = false;
    bool cdev_added = false;
    bool class_created = false;

    ret = alloc_chrdev_region(&p_led->dev_num, 0, 1, "my module");

    if (0 == ret)
    {
        chrdev_region_allocated = true;
        cdev_init(&p_led->dev, &fops);
        ret = cdev_add(&p_led->dev, p_led->dev_num, 1);
    }
    
    if (0 == ret)
    {
        cdev_added = true;
        p_led->class = class_create(THIS_MODULE, "my_class");
        if (IS_ERR(p_led->class))
        {
            ret = PTR_ERR(p_led->class);
        }
    }
    
    if (0 == ret)
    {
        class_created = true;
        my_device = device_create(p_led->class, NULL, p_led->dev_num, NULL, "my_dev");
        if (IS_ERR(my_device))
        {
            ret = PTR_ERR(my_device);
        }
    }

    if (0 != ret)
    {
        if (class_created)
        {
            class_destroy(p_led->class);
        }
        
        if (cdev_added)
        {
            cdev_del(&p_led->dev);
        }

        if (chrdev_region_allocated)
        {
            unregister_chrdev_region(p_led->dev_num, 1);
        }
        
    }
    else
    {
        pr_info("My module initialized\n");
    }

    return 0;
}

static void __exit my_exit(void)
{
    device_destroy(p_led->class, p_led->dev_num);
    class_destroy(p_led->class);
    cdev_del(&p_led->dev);
    unregister_chrdev_region(p_led->dev_num, 1);
    pr_info("My module exited\n");
}
#endif

static int __init my_init(void)
{
    int ret = 0;
    /* allocate a range of 32 devices */
    ret = alloc_chrdev_region(&first_dev, 0, 32, "my module");
    if (0 == ret)
    {
        class = class_create(THIS_MODULE, "myled");
        if (IS_ERR(class))
        {
            ret = PTR_ERR(class);
        }
    }
    return platform_driver_register(&my_driver);
}

static void __exit my_exit(void)
{
    platform_driver_unregister(&my_driver);
    class_destroy(class);
    unregister_chrdev_region(first_dev, 32);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("My kernel module");