#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define DEVICE_NAME "signal_demo"

// 全局变量
static int major;
static DECLARE_WAIT_QUEUE_HEAD(wq);  // 等待队列
static int data_ready = 0;           // 数据是否准备好

// 设备读函数
static ssize_t signal_demo_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
  int nonblock = 0;
  DECLARE_WAITQUEUE(wait, current);

  printk(KERN_INFO "Waiting for data...\n");

  // 阻塞等待数据，支持信号中断
  //   if (wait_event_interruptible(wq, data_ready)) {
  //     printk(KERN_INFO "Read interrupted by signal, ret:\n", -ERESTARTSYS);
  //     return -ERESTARTSYS;  // 返回系统调用中断错误
  //   }

  add_wait_queue(&wq, &wait);
  for (;;) {
    set_current_state(TASK_INTERRUPTIBLE);
    if (data_ready) break;
    nonblock = file->f_flags & O_NONBLOCK;
    if (nonblock) break;
    if (signal_pending(current)) break;
    schedule();
  }

  set_current_state(TASK_RUNNING);
  remove_wait_queue(&wq, &wait);

  if (nonblock) {
    printk(KERN_INFO "read with O_NONBLOCK");
    if (!data_ready) {
      return -EAGAIN;
    }
  } else if (!data_ready) {
    printk(KERN_INFO "signal interrupt happened");
    return -ERESTARTNOINTR;  //  应用程序依然会阻塞在read不返回
    // return -EINTR;  // 应用程序read返回 Interrupted system call
    // return -ERESTARTSYS;  // 应用程序依然会阻塞在read不返回
  }

  data_ready = 0;
  if (copy_to_user(buf, "Hello, World!", 13)) {
    //应用程序read返回 bad address
    return -EFAULT;
  }

  printk(KERN_INFO "Data sent to user space.\n");
  return 13;  // 返回写入用户空间的字节数
}

// 写函数，用于模拟数据准备好
static ssize_t signal_demo_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
  data_ready = 1;              // 标记数据已准备好
  wake_up_interruptible(&wq);  // 唤醒等待队列
  printk(KERN_INFO "Data is ready.\n");
  return count;
}

// 文件操作定义
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = signal_demo_read,
    .write = signal_demo_write,
};

// 模块加载
static int __init signal_demo_init(void) {
  major = register_chrdev(0, DEVICE_NAME, &fops);
  if (major < 0) {
    printk(KERN_ALERT "Failed to register device.\n");
    return major;
  }

  printk(KERN_INFO "Signal demo device registered, major: %d\n", major);
  return 0;
}

// 模块卸载
static void __exit signal_demo_exit(void) {
  unregister_chrdev(major, DEVICE_NAME);
  printk(KERN_INFO "Signal demo device unregistered.\n");
}

module_init(signal_demo_init);
module_exit(signal_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Signal Example");
MODULE_DESCRIPTION("A simple Linux driver to demonstrate signal handling.");
