#ifndef __CDEVICE_H
#define __CDEVICE_H

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

int cdevice_init(size_t device_count);   // 模块加载时的初始化函数
void cdevice_exit(size_t device_count);  // 模块卸载时的清理函数

#endif