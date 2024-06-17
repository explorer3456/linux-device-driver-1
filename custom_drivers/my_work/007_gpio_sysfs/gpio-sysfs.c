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
#include <linux/gpio/consumer.h>

struct gpiodev_private_data
{
	char label[20];
	struct gpio_desc *desc;
};

struct gpiodrv_private_data
{
	int total_devices;
	struct class *class_gpio;
	struct device **dev;
};

struct gpiodrv_private_data gpio_drv_data;


struct of_device_id gpio_device_match[] =
{
	{
		.compatible = "org,bone-gpio-sysfs"
	},
	{}
};

static direction_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct gpiodev_private_data *dev_data;
	int dir;
	char *direction;

	dev_data = dev_get_drvdata(dev);

	dir = gpiod_get_direction(dev_data->desc);

	if (dir < 0)
		return dir;

	direction = (dir == 0) ? "out" : "in"; 

	return sprintf(buf, "%s\n", direction);
}

static direction_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct gpiodev_private_data *dev_data;
	int ret;

	dev_data = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "in")){
		ret = gpiod_direction_input(dev_data->desc);
	} else if (sysfs_streq(buf, "out")) {
		ret = gpiod_direction_output(dev_data->desc, 0);
	} else
		ret = -EINVAL;

	return ret ? ret : count;
}

static value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	int value;

	value = gpiod_get_value(dev_data->desc);

	return sprintf(buf, "%d\n", value);
}

static value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	int ret;
	long value;

	ret = kstrtol(buf, 0, &value);

	if (ret)
		return ret;

	gpiod_set_value(dev_data->desc, value);

	return count;
}

static ssize_t label_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", dev_data->label);
}



static DEVICE_ATTR_RW(direction);
static DEVICE_ATTR_RW(value);
static DEVICE_ATTR_RO(label);

static struct attribute *gpio_attrs[] =
{
	&dev_attr_direction.attr,
	&dev_attr_value.attr,
	&dev_attr_label.attr,
	NULL
};

static struct attribute_group gpio_attr_group = 
{
	.attrs = gpio_attrs
};

static const struct attribute_group *gpio_attr_groups[] = 
{
	&gpio_attr_group,
	NULL
};

int gpio_sysfs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node * parent = pdev->dev.of_node;
	struct device_node * child = NULL;
	struct gpiodev_private_data *dev_data;

	const char *name;
	int i = 0;
	int ret;

	gpio_drv_data.total_devices = of_get_child_count(parent);

	if (gpio_drv_data.total_devices == 0) {
		dev_err(dev, "No devices\n");
		return -EINVAL;
	}

	gpio_drv_data.dev = devm_kzalloc(dev, sizeof(struct device *) * gpio_drv_data.total_devices, \
			GFP_KERNEL);

	for_each_available_child_of_node(parent, child)
	{
		dev_data = devm_kzalloc(dev, sizeof(*dev_data), GFP_KERNEL);

		if (!dev_data) {
			dev_err(dev, "Cannot allocate memory\n");
			return -ENOMEM;
		}

		if (of_property_read_string(child, "label", &name))
		{
			dev_warn(dev, "Missing label information");
			snprintf(dev_data->label, sizeof(dev_data->label), "unkngpio%d", i);
		} else {
			strcpy(dev_data->label, name);
			dev_info(dev, "gpio label = %s\n", dev_data->label);
		}

		dev_data->desc = devm_fwnode_get_gpiod_from_child(dev, "bone", &child->fwnode, \
				GPIOD_ASIS, dev_data->label);

		if (IS_ERR(dev_data->desc)) {
			ret = PTR_ERR(dev_data->desc);
			if (ret == -ENOENT) 
				dev_err(dev, "No GPIO has been assigned\n");

			return ret;
		}

		ret = gpiod_direction_output(dev_data->desc, 0);
		if (ret) {
			dev_err(dev, "gpio direction output failed\n");
			return ret;
		}

		gpio_drv_data.dev[i] = device_create_with_groups(gpio_drv_data.class_gpio, dev, 0,\
				dev_data, gpio_attr_groups, dev_data->label);

		if (IS_ERR(gpio_drv_data.dev[i])) {
			dev_err(dev, "Error in device create\n");
			return PTR_ERR(gpio_drv_data.dev[i]);
		}

		i++;

	}

	return 0;
}

int gpio_sysfs_remove(struct platform_device *pdev)
{
	int i;

	/*
	   we should test using device_destroy with gpio_class next time.
	 */
	dev_info(&pdev->dev, "removing devices\n");
	for (i=0; i< gpio_drv_data.total_devices; i++) {
		device_unregister(gpio_drv_data.dev[i]);
	}

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
