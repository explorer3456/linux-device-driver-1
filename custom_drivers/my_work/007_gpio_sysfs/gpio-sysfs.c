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

struct gpiodrv_private_data gpio_drv_data;

static int __init gpio_sysfs_init(void)
{
	gpio_drv_data.class_gpio = class_create(THIS_MODULE, "bone_gpios");
	if (IS_ERR(gpio_drv_data.class_gpio)) {
		pr_err("Error in creating class \n");
		return PTR_ERR(gpio_drv_data.class_gpio);
	}
	pr_info("module load success\n");

	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jhkim");
MODULE_DESCRIPTION("A simple gpio sysfs driver");
