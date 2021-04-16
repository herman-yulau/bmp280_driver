#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/uaccess.h>

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "Herman Yulau" );
MODULE_VERSION( "0.1");

static int major = 0;
module_param(major, int, S_IRUGO);

static char *buffer = "This is bmp280 module!\n"; //buffer

ssize_t bmp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    int len = strlen(buffer);
    printk( KERN_INFO "=== read : %d\n", count);   
    if (count < len) return -EINVAL;
    if (*ppos != 0) {
        printk( KERN_INFO "=== read return : 0\n"); // EOF
        return 0;
    }
    if (copy_to_user(buf, buffer, len)) return -EINVAL;
    *ppos = len;
    printk( KERN_INFO "=== read return : %d\n", len);
    return len;
}

static const struct file_operations bmp_fops = {
    .owner = THIS_MODULE,
    .read = bmp_read,
};

#define DEVICE_FIRST 0
#define DEVICE_COUNT 1
#define MODENAME "bmp280_driver"

static struct cdev bmp_dev;
static struct class *devclass;

static int __init bmp280_init(void) {
    int ret, i;
    dev_t dev;
    printk(buffer);
    if (major != 0) {
        dev = MKDEV(major, DEVICE_FIRST);
        ret = register_chrdev_region(dev, DEVICE_COUNT, MODENAME);
    } else {
        ret = alloc_chrdev_region(&dev, DEVICE_FIRST, DEVICE_COUNT, MODENAME);
        major = MAJOR(dev);
    }
    if (ret < 0) {
        printk( KERN_ERR "=== Can not register char device region\n");
        goto err;
    }
    cdev_init(&bmp_dev, &bmp_fops);
    bmp_dev.owner = THIS_MODULE;
    ret = cdev_add(&bmp_dev, dev, DEVICE_COUNT);
    if (ret < 0) {
        unregister_chrdev_region( MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
        printk( KERN_ERR "=== Can not add char device\n" );
        goto err;
    }
    devclass = class_create( THIS_MODULE, "dyn_class" ); /* struct class* */
    for( i = 0; i < DEVICE_COUNT; i++ ) {
#define DEVNAME "bmp280"
        dev = MKDEV( major, DEVICE_FIRST + i );
        device_create(devclass, NULL, dev, NULL, "%s_%d", DEVNAME, i );
    }
    printk( KERN_INFO "======== module installed %d:[%d-%d] ===========\n", MAJOR( dev ), DEVICE_FIRST, MINOR( dev ) ); 

err:
    return ret;
}

static void __exit bmp280_release(void) {
    dev_t dev;
    int i;
    for( i = 0; i < DEVICE_COUNT; i++ ) {
        dev = MKDEV( major, DEVICE_FIRST + i );
        device_destroy( devclass, dev );
    }
    class_destroy( devclass );
    cdev_del( &bmp_dev );
    unregister_chrdev_region( MKDEV( major, DEVICE_FIRST ), DEVICE_COUNT );
    printk( "bmp280 module was removed!\n" );
}

module_init( bmp280_init );
module_exit( bmp280_release );

