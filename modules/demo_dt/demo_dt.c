// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static int demo_dt_probe(struct platform_device *pdev)
{
    u32 id = 0;
    int ret;

    ret = of_property_read_u32(pdev->dev.of_node, "sensor-id", &id);
    if (ret)
        dev_warn(&pdev->dev, "sensor-id not found (%d), using default\n", ret);
    
    dev_info(&pdev->dev, "demo_dt: probed via Device Tree, sensor-id=%u\n", id);
    return 0;
}

static const struct of_device_id demo_dt_of_match[] = {
    { .compatible = "acme,demo-therm"},
    {}
};
MODULE_DEVICE_TABLE(of, demo_dt_of_match);

static struct platform_driver demo_dt_driver = {
    .probe = demo_dt_probe,
    .driver = {
        .name = "demo_dt",
        .of_match_table = demo_dt_of_match,
    },
};
module_platform_driver(demo_dt_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Week 3 day 1: platform driver auto-probed via Device Tree");