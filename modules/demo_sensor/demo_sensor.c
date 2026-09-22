// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>

#define REG_TEMP 0x00

static int demo_probe(struct i2c_client *client)
{
    int val = i2c_smbus_read_word_data(client, REG_TEMP);

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
    .driver = { .name = "demo_sensor" },
    .probe  = demo_probe,
    .remove = demo_remove,
    .id_table = demo_id,
};
module_i2c_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Week 2: minimal I2C driver reading a simulated register");