#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>

#include "blocking_queue.h"
#include "cdevice.h"
#include "vnet.h"
#include "vnic.h"

static const size_t vnet_count = 2;
static const size_t blocking_queue_capacity = 20;
static struct vnet instances[2];

struct vnet* get_vnet(size_t index) {
  return &instances[index];
}

static int __init vnet_init(void) {
  int i;
  char queue_name[20];
  cdevice_init(vnet_count);

  for (i = 0; i < vnet_count; ++i) {
    snprintf(queue_name, 20, "queue_%d", i);
    blocking_queue_init(&instances[i].blocking_queue, blocking_queue_capacity, queue_name);
    instances[i].vnic = vnic_create(&instances[i]);
  }

  printk(KERN_INFO "vnet init ok.\n");
  return 0;
}

static void __exit vnet_exit(void) {
  int i;
  cdevice_exit(vnet_count);

  for (i = 0; i < 2; ++i) {
    vnic_destroy(instances[i].vnic);
    blocking_queue_clean(&instances[i].blocking_queue);
  }
  printk(KERN_INFO "vnet exit\n");
}

module_init(vnet_init);
module_exit(vnet_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Baolong");
MODULE_DESCRIPTION(
    "A simple virtual network device driver for Linux Kernel 6.x");
