#ifndef __VNIC_H
#define __VNIC_H

#include <linux/types.h>

struct vnet;

struct vnic {
  struct vnet *vnet;
  struct net_device *net_device;
};

// 驱动加载时的初始化函数
struct vnic *vnic_create(struct vnet *vnet);

void vnic_destroy(struct vnic *vnic);

int vnic_rx_packet(struct vnic *vnic, const char __user *user_data, size_t data_len);

#endif
