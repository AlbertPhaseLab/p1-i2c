// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/hwmon.h>
#include <linux/of.h>

#define REG_TEMP 0x00

struct demo_data {
    struct i2c_client *client;
};

static int demo_read(struct device *dev, enum hwmon_sensor_types type,
                      u32 attr, int channel, long *val)
{
    struct demo_data *data = dev_get_drvdata(dev);
    int raw;

    if (type != hwmon_temp || attr != hwmon_temp_input)
        return -EOPNOTSUPP;

    raw = i2c_smbus_read_word_data(data->client, REG_TEMP);
    if (raw < 0) {
        dev_dbg(dev, "failed to read temperature register: %d\n", raw);
        return raw;
    }

    /* raw are units of 0.0625 C (typical format of sensors such as TMP102);
     * hwmon expects values in millidegrees Celsius */
    *val = raw * 625 / 10;
    return 0;
}

static umode_t demo_is_visible(const void *data, enum hwmon_sensor_types type,
                                u32 attr, int channel)
{
    if (type == hwmon_temp && attr == hwmon_temp_input)
        return 0444;
    return 0;
}

static const struct hwmon_channel_info *demo_info[] = {
    HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT),
    NULL
};

static const struct hwmon_ops demo_ops = {
    .is_visible = demo_is_visible,
    .read       = demo_read,
};

static const struct hwmon_chip_info demo_chip_info = {
    .ops  = &demo_ops,
    .info = demo_info,
};

static int demo_probe(struct i2c_client *client)
{
    struct demo_data *data;
    struct device *hwmon_dev;

    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;

    hwmon_dev = devm_hwmon_device_register_with_info(&client->dev, "demo_sensor",
                                                       data, &demo_chip_info, NULL);
    if (IS_ERR(hwmon_dev))
        return dev_err_probe(&client->dev, PTR_ERR(hwmon_dev),
                              "failed to register hwmon device\n");

    dev_info(&client->dev, "demo_sensor: registered via hwmon\n");
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

/*
 * of_match_table is provided for real hardware with a Device Tree-described
 * I2C controller (e.g. a board's i2c1 node). It cannot be exercised
 * end-to-end in this QEMU `virt` + i2c-stub setup, since i2c-stub has no
 * Device Tree representation. See demo_dt.c (week 3, day 1) for a verified
 * auto-probe demo using the platform bus.
 */
static const struct of_device_id demo_of_match[] = {
    { .compatible = "acme,demo-sensor" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static struct i2c_driver demo_driver = {
    .driver = {
        .name           = "demo_sensor",
        .of_match_table = demo_of_match,
    },
    .probe    = demo_probe,
    .remove   = demo_remove,
    .id_table = demo_id,
};
module_i2c_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Week 3 day 2: I2C driver with Device Tree matching");
