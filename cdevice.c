#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "blocking_queue.h"
#include "vnet.h"
#include "vnic.h"

#define DEVICE_NAME "cdevice"

extern struct vnet *get_vnet(size_t index);

struct cdevice {
	struct vnet* vnet;
	int minor;
};

static int major;
static struct cdev my_cdev;

// 可以打开设备多次
static int cdevice_open(struct inode *inode, struct file *file) {
  int minor = iminor(inode);  // Get the minor number
  file->private_data = get_vnet(minor);
  printk(KERN_INFO "Opened cdevice with minor number: %d\n", minor);
  return 0;
}

// 释放设备
static int cdevice_release(struct inode *inode, struct file *file) {
  int minor = iminor(inode);
  printk(KERN_INFO "Release cdevice with minor number: %d\n", minor);
  return 0;
}

// 从设备读取数据
static ssize_t cdevice_read(struct file *file, char __user *user_buffer,
                            size_t count, loff_t *offset) {
  size_t data_len = 0;
  int  nonblock = file->f_flags & O_NONBLOCK;
  struct vnet *vnet = file->private_data;
  struct blocking_queue* blocking_queue = &vnet->blocking_queue;
  struct sk_buff *skb = blocking_queue_get(blocking_queue, nonblock);

  if (skb != NULL) {
    if (copy_to_user(user_buffer, skb->data, skb->len)) {
      return -EFAULT;
    }
    data_len = skb->len;
    dev_kfree_skb(skb);
    printk(KERN_INFO "cdevice %d: read %zu bytes\n", iminor(file->f_inode), data_len);
  }

  return data_len;
}

// 写入数据到设备
static ssize_t cdevice_write(struct file *file, const char __user *user_buffer,
                             size_t count, loff_t *offset) {
  struct vnet *vnet;
  vnet = file->private_data;
  if (vnic_rx_packet(vnet->vnic, user_buffer, count) > 0) {
  	printk(KERN_INFO "cdevice %d: write: %zu bytes\n", iminor(file->f_inode), count);
  }


  return count;
}

// 文件操作定义
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = cdevice_open,
    .release = cdevice_release,
    .read = cdevice_read,
    .write = cdevice_write,
};

// 模块加载时的初始化函数
int cdevice_init(size_t device_count) {
  // 分配主设备号
  dev_t dev;
  int ret = alloc_chrdev_region(&dev, 0, device_count, DEVICE_NAME);
  if (ret) {
    printk(KERN_ERR "cdevice: Failed to allocate major number\n");
    return ret;
  }

  major = MAJOR(dev);

  // 初始化字符设备
  cdev_init(&my_cdev, &fops);
  my_cdev.owner = THIS_MODULE;

  // 添加字符设备
  ret = cdev_add(&my_cdev, dev, device_count);
  if (ret) {
    printk(KERN_ERR "cdevice: Failed to add cdev\n");
    unregister_chrdev_region(dev, device_count);
    return ret;
  }

  printk(KERN_INFO "cdevice: Module loaded, major number %d\n", major);
  return 0;
}

// 模块卸载时的清理函数
void cdevice_exit(size_t device_count) {
  dev_t dev = MKDEV(major, 0);

  cdev_del(&my_cdev);
  unregister_chrdev_region(dev, device_count);

  printk(KERN_INFO "cdevice: Module unloaded\n");
}
