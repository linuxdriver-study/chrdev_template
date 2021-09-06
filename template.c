#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/types.h>
#include <linux/ide.h>

#define DEVICE_CNT      1
#define DEVICE_NAME     "led"

struct led_device_struct {
        int major;
        int minor;
        dev_t devid;
        struct cdev led_cdev;
        struct class *class;
        struct device *device;
};
static struct led_device_struct led_dev;

static int led_open(struct inode *inode, struct file *file);
static ssize_t led_write(struct file *file,
                        const char __user *user,
                        size_t count,
                        loff_t *loff);
static int led_release(struct inode *inode, struct file *file);

static struct file_operations ops = {
        .owner = THIS_MODULE,
        .open = led_open,
        .write = led_write,
        .release = led_release,
};

static int led_open(struct inode *inode, struct file *file)
{
        file->private_data = &led_dev;
        return 0;
}

static ssize_t led_write(struct file *file,
                        const char __user *user,
                        size_t count,
                        loff_t *loff)
{
        int ret = 0;
        unsigned char buf[1] = {0};
        struct led_device_struct *dev = file->private_data;

        ret = copy_from_user(buf, user, 1);
        if (ret != 0) {
                goto error;
        }

error:
        return ret;
}

static int led_release(struct inode *inode, struct file *file)
{
        file->private_data = NULL;
        return 0;
}

static int __init led_init(void)
{
        int ret = 0;
        if (led_dev.major) {
                led_dev.devid = MKDEV(led_dev.major, led_dev.minor);
                ret = register_chrdev_region(led_dev.devid, DEVICE_CNT, DEVICE_NAME);
        } else {
                ret = alloc_chrdev_region(&led_dev.devid, 0, DEVICE_CNT, DEVICE_NAME);
        }
        if (ret < 0) {
                printk("chrdev region error!\n");
                goto fail_chrdev_region;
        }
        led_dev.major = MAJOR(led_dev.devid);
        led_dev.minor = MINOR(led_dev.devid);
        printk("major:%d minor:%d\n", led_dev.major, led_dev.minor);

        cdev_init(&led_dev.led_cdev, &ops);
        ret = cdev_add(&led_dev.led_cdev, led_dev.devid, DEVICE_CNT);
        if (ret < 0) {
                printk("cdev add error!\n");
                goto fail_cdev_add;
        }
        led_dev.class = class_create(THIS_MODULE, DEVICE_NAME);
        if (IS_ERR(led_dev.class)) {
                printk("class create error!\n");
                ret = -EINVAL;
                goto fail_class_create;
        }
        led_dev.device = device_create(led_dev.class, NULL,
                                       led_dev.devid, NULL, DEVICE_NAME);
        if (IS_ERR(led_dev.device)) {
                printk("device create error!\n");
                ret = -EINVAL;
                goto fail_device_create;
        }
        goto success;
        
//fail_io_config:
        device_destroy(led_dev.class, led_dev.devid);
fail_device_create:
        class_destroy(led_dev.class);
fail_class_create:
        cdev_del(&led_dev.led_cdev);
fail_cdev_add:
        unregister_chrdev_region(led_dev.devid, DEVICE_CNT);
fail_chrdev_region:
success:
        return ret;
}

static void __exit led_exit(void)
{
        device_destroy(led_dev.class, led_dev.devid);
        class_destroy(led_dev.class);
        cdev_del(&led_dev.led_cdev);
        unregister_chrdev_region(led_dev.devid, DEVICE_CNT);
}

module_init(led_init);
module_exit(led_exit);
MODULE_AUTHOR("wanglei");
MODULE_LICENSE("GPL");
