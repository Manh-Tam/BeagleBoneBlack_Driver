#include <linux/kernel.h>
#include <linux/module.h>

static int __init led_init(void)
{
    printk("led_init: OK\n");
    return 0;
}

static void __exit led_exit(void)
{
    printk("led_exit: OK\n");
}


module_init(led_init);
module_exit(led_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tam Le");
MODULE_DESCRIPTION("My LED");