// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/hwmon.h>
#include <linux/of.h>
#include <linux/regmap.h>

#define REG_TEMP 0x00

struct demo_data {
    struct regmap *regmap;
};

static const struct regmap_config demo_regmap_config = {
    .reg_bits = 8,
    .val_bits = 16,
    .val_format_endian = REGMAP_ENDIAN_LITTLE,
};

static int demo_read(struct device *dev, enum hwmon_sensor_types type,
                      u32 attr, int channel, long *val)
{
    struct demo_data *data = dev_get_drvdata(dev);
    unsigned int raw;
    int ret;

    if (type != hwmon_temp || attr != hwmon_temp_input)
        return -EOPNOTSUPP;

    ret = regmap_read(data->regmap, REG_TEMP, &raw);
    if (ret)
        return ret;

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

    data->regmap = devm_regmap_init_i2c(client, &demo_regmap_config);
    if (IS_ERR(data->regmap))
        return dev_err_probe(&client->dev, PTR_ERR(data->regmap),
                              "failed to init regmap\n");

    hwmon_dev = devm_hwmon_device_register_with_info(&client->dev, "demo_sensor_regmap",
                                                       data, &demo_chip_info, NULL);
    if (IS_ERR(hwmon_dev))
        return dev_err_probe(&client->dev, PTR_ERR(hwmon_dev),
                              "failed to register hwmon device\n");

    dev_info(&client->dev, "demo_sensor_regmap: registered via hwmon (regmap)\n");
    return 0;
}

static void demo_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "demo_sensor_regmap: removed\n");
}

static const struct i2c_device_id demo_id[] = {
    { "demo_sensor_regmap" },
    { }
};
MODULE_DEVICE_TABLE(i2c, demo_id);

static const struct of_device_id demo_of_match[] = {
    { .compatible = "acme,demo-sensor-regmap" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static struct i2c_driver demo_driver = {
    .driver = {
        .name           = "demo_sensor_regmap",
        .of_match_table = demo_of_match,
    },
    .probe    = demo_probe,
    .remove   = demo_remove,
    .id_table = demo_id,
};
module_i2c_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Week 3 day 3: I2C driver using regmap (experimental)");
