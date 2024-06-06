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

struct of_device_id gpio_device_match[] =
{
	{
		.compatible = "org,bone-gpio-sysfs"
	},
	{}
};

int gpio_sysfs_probe(struct platform_device *pdev)
{
	return 0;
}

int gpio_sysfs_remove(struct platform_device *pdev)
{
	return 0;
}

struct platform_driver gpiosysfs_platform_driver = 
{
	.probe = gpio_sysfs_probe,
	.remove = gpio_sysfs_remove,
	.driver = {
		.name = "bone-gpio-sysfs",
		.of_match_table = of_match_ptr(gpio_device_match),
	},

};

static int __init gpio_sysfs_init(void)
{
	gpio_drv_data.class_gpio = class_create(THIS_MODULE, "bone_gpios");
	if (IS_ERR(gpio_drv_data.class_gpio)) {
		pr_err("Error in creating class \n");
		return PTR_ERR(gpio_drv_data.class_gpio);
	}

	platform_driver_register(&gpiosysfs_platform_driver);

	pr_info("module load success\n");

	return 0;
}

void __exit gpio_sysfs_exit(void)
{
	platform_driver_unregister(&gpiosysfs_platform_driver);
	class_destroy(gpio_drv_data.class_gpio);
}

module_init(gpio_sysfs_init);
module_exit(gpio_sysfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jhkim");
MODULE_DESCRIPTION("A simple gpio sysfs driver");
