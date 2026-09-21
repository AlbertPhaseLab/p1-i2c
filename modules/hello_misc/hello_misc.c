// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define BUF_SZ 128

static char *greeting = "hola";
module_param(greeting, charp, 0444);
MODULE_PARM_DESC(greeting, "Texto inicial del dispositivo");

static char buf[BUF_SZ];
static size_t buf_len;
static DEFINE_MUTEX(buf_lock);

static ssize_t hm_read(struct file *f, char __user *ubuf, size_t count, loff_t *ppos)
{
    ssize_t ret;

    mutex_lock(&buf_lock);
    ret = simple_read_from_buffer(ubuf, count, ppos, buf, buf_len);
    mutex_unlock(&buf_lock);
    return ret;
}

static ssize_t hm_write(struct file *f, const char __user *ubuf, size_t count, loff_t *ppos)
{
    if (count >= BUF_SZ)
        return -EINVAL;

    mutex_lock(&buf_lock);
    if (copy_from_user(buf, ubuf, count)) {
        mutex_unlock(&buf_lock);
        return -EFAULT;
    }
    buf_len = count;
    mutex_unlock(&buf_lock);
    return count;
}

static const struct file_operations hm_fops = {
    .owner = THIS_MODULE,
    .read  = hm_read,
    .write = hm_write,
};

static struct miscdevice hm_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "hello_misc",
    .fops  = &hm_fops,
};

static int __init hm_init(void)
{
    int ret;

    buf_len = scnprintf(buf, BUF_SZ, "%s\n", greeting);
    ret = misc_register(&hm_dev);
    if (ret) {
        pr_err("hello_misc: misc_register fallo (%d)\n", ret);
        return ret;
    }
    pr_info("hello_misc: /dev/%s listo\n", hm_dev.name);
    return 0;
}

static void __exit hm_exit(void)
{
    misc_deregister(&hm_dev);
    pr_info("hello_misc: descargado\n");
}

module_init(hm_init);
module_exit(hm_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Dia 3: misc device con parametro");
