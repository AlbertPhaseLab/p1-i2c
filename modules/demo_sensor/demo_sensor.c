// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>

#define REG_TEMP 0x00

struct demo_data {
    struct i2c_client *client;
};

static ssize_t temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct demo_data *data = dev_get_drvdata(dev);
    int val = i2c_smbus_read_word_data(data->client, REG_TEMP);

    if (val < 0)
        return val;

    return sysfs_emit(buf, "%d\n", val);
}
static DEVICE_ATTR_RO(temp);

static struct attribute *demo_attrs[] = {
    &dev_attr_temp.attr,
    NULL,
};
ATTRIBUTE_GROUPS(demo);

static int demo_probe(struct i2c_client *client)
{
    struct demo_data *data;
    int val;

    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;
    dev_set_drvdata(&client->dev, data);

    val = i2c_smbus_read_word_data(client, REG_TEMP);
    if (val < 0) {
        dev_err(&client->dev, "read failed (%d)\n", val);
        return val;
    }
    dev_info(&client->dev, "demo_sensor: reg 0x%02x = 0x%04x\n", REG_TEMP, val);
    return 0;
}

static void demo_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "demo_sensor: removed\n");
}

static const struct i2c_device_id demo_id[] = {
    { "demo_sensor" },
    { }
};
MODULE_DEVICE_TABLE(i2c, demo_id);

static struct i2c_driver demo_driver = {
    .driver = {
        .name = "demo_sensor",
        .dev_groups = demo_groups,
    },
    .probe    = demo_probe,
    .remove   = demo_remove,
    .id_table = demo_id,
};
module_i2c_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Week 2 day 2: expose sensor reading via sysfs");