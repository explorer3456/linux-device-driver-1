#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include <linux/platform_device.h>
#include<linux/slab.h>
#include<linux/mod_devicetable.h>
#include<linux/of.h>
#include<linux/of_device.h>

struct gpiodev_private_data
{
	char label[20];
};

struct gpiodrv_private_data
{
	int total_devices;
	struct class *class_gpio;
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jhkim");
MODULE_DESCRIPTION("A simple gpio sysfs driver");
