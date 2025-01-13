#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "mychardev"
#define BUFFER_SIZE 1024
#define DEVICE_COUNT 3

static int major = 511;
static struct cdev my_cdev;
static char *device_buffer;

// 打开设备
static int mychardev_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "mychardev: Device opened\n");
    return 0;
}

// 释放设备
static int mychardev_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "mychardev: Device closed\n");
    return 0;
}

// 从设备读取数据
static ssize_t mychardev_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset) {
    size_t bytes_to_read;

    // 检查是否到达文件结尾
    if (*offset >= BUFFER_SIZE) {
        return 0;
    }

    // 计算实际读取字节数
    bytes_to_read = min(count, (size_t)(BUFFER_SIZE - *offset));

    // 复制数据到用户空间
    if (copy_to_user(user_buffer, device_buffer + *offset, bytes_to_read)) {
        return -EFAULT;
    }

    *offset += bytes_to_read;
    printk(KERN_INFO "mychardev: Read %zu bytes\n", bytes_to_read);
    return bytes_to_read;
}

// 写入数据到设备
static ssize_t mychardev_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset) {
    size_t bytes_to_write;

    // 检查是否超出缓冲区大小
    if (*offset >= BUFFER_SIZE) {
        return -ENOMEM;
    }

    // 计算实际写入字节数
    bytes_to_write = min(count, (size_t)(BUFFER_SIZE - *offset));

    // 从用户空间复制数据到设备缓冲区
    if (copy_from_user(device_buffer + *offset, user_buffer, bytes_to_write)) {
        return -EFAULT;
    }

    *offset += bytes_to_write;
    printk(KERN_INFO "mychardev: Wrote %zu bytes\n", bytes_to_write);
    return bytes_to_write;
}

// 文件操作定义
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = mychardev_open,
    .read = mychardev_read,
    .write = mychardev_write,
    .release = mychardev_release,
};

// 模块加载时的初始化函数
static int __init mychardev_init(void) {
    // 分配主设备号
    dev_t dev = MKDEV(major, 0);
    int ret = -1;
    /*
    int ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret) {
        printk(KERN_ERR "mychardev: Failed to allocate major number\n");
        return ret;
    }
    major = MAJOR(dev);
    */

    // 初始化字符设备
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    // 添加字符设备
    ret = cdev_add(&my_cdev, dev, DEVICE_COUNT);
    if (ret) {
        printk(KERN_ERR "mychardev: Failed to add cdev\n");
        unregister_chrdev_region(dev, DEVICE_COUNT);
        return ret;
    }

    // 分配设备缓冲区
    device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev, DEVICE_COUNT);
        return -ENOMEM;
    }
    memset(device_buffer, 0, BUFFER_SIZE);

    printk(KERN_INFO "mychardev: Module loaded, major number %d\n", major);
    return 0;
}

// 模块卸载时的清理函数
static void __exit mychardev_exit(void) {
    dev_t dev = MKDEV(major, 0);

    kfree(device_buffer);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, 1);

    printk(KERN_INFO "mychardev: Module unloaded\n");
}

module_init(mychardev_init);
module_exit(mychardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple character device driver for Linux Kernel 6.x");

