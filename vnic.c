#include "vnic.h"

#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>

#include "blocking_queue.h"
#include "vnet.h"

#define DRIVER_NAME "vnet"

// 打开网卡
static int vnic_open(struct net_device *dev) {
  netif_start_queue(dev);  // 启动传输队列
  printk(KERN_INFO "%s: vnic opened\n", dev->name);
  return 0;
}

// 停止网卡
static int vnic_stop(struct net_device *dev) {
  netif_stop_queue(dev);  // 停止传输队列
  printk(KERN_INFO "%s: vnic stopped\n", dev->name);
  return 0;
}

// 接口回调，发送数据包
static netdev_tx_t vnic_start_xmit(struct sk_buff *skb,
                                   struct net_device *dev) {
  struct vnic *vnic;
  int ret = -1;
  /* check the ip packet length,it must more then 34 octets */
  if (skb->len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
    printk(KERN_WARNING "Bad packet! It's size is less then 34!\n");
  }

  vnic = netdev_priv(dev);
  ret = blocking_queue_put(&vnic->vnet->blocking_queue, skb);
  if (ret == 1) {
    // 更新统计信息
    dev->stats.tx_packets++;
    dev->stats.tx_bytes += skb->len;
    printk(KERN_INFO
           "%s: Transmitting packet of length %u to blocking queue.\n",
           dev->name, skb->len);
  } else {
    dev->stats.tx_dropped++;
    dev_kfree_skb(skb);
    printk(KERN_WARNING "blocking queue is full, packet droped.\n");
  }

  return NETDEV_TX_OK;
}

// 定义网卡的操作函数
static const struct net_device_ops vnic_netdev_ops = {
    .ndo_open = vnic_open,
    .ndo_stop = vnic_stop,
    .ndo_start_xmit = vnic_start_xmit,
};

// receive ethnet frame pointered by user_data
int vnic_rx_packet(struct vnic *vnic, const char __user *user_data,
                   size_t data_len) {
  struct net_device *dev;
  struct sk_buff *skb;

  // 分配 sk_buff
  dev = vnic->net_device;
  skb = netdev_alloc_skb(dev, 2 + data_len);
  if (!skb) {
    printk(KERN_ERR "%s: Failed to allocate sk_buff\n", dev->name);
    dev->stats.rx_dropped++;
    return -1;
  }

  skb_reserve(skb, 2);  // aimed to align ether header to 16 bytes
  skb_put(skb, data_len);

  if (copy_from_user(skb->data, user_data, data_len)) {
    printk(KERN_ERR "Failed to copy data from user space\n");
    return -2;
  }

  // 解封以太包
  skb->protocol = eth_type_trans(skb, dev);
  skb->dev = dev;  // Indicate the packet is received
  skb->ip_summed = CHECKSUM_UNNECESSARY;

  if (skb->protocol != 0x8) {
    dev_kfree_skb(skb);
    dev->stats.rx_dropped++;
    // printk(KERN_INFO "drop broadcast packet.\n");
    return 0;
  }
  skb->protocol = 0x0800;

  // 提交到网络栈, enqueued in the softirq
  // netif_rx enqueues the packet for later processing in a softirq context,
  // netif_receive_skb processes the packet immediately in the current context,
  // depending on the protocol, the packet is further processed, possibly
  // leading to routing, forwarding, or delivery to a socket.
  // if (netif_rx(skb) != NET_RX_SUCCESS) {
  if (netif_receive_skb(skb) != NET_RX_SUCCESS) {
    printk(KERN_WARNING "%s: Packet dropped by networking stack\n", dev->name);
    return 0;
  }

  // 更新统计信息
  dev->stats.rx_packets++;
  dev->stats.rx_bytes += data_len;
  printk(KERN_INFO
         "%s: submitted packet to tcp/ip layer, length: %ld, ethertype: %x\n",
         dev->name, data_len, skb->protocol);
  return 1;
}

// 驱动加载时的初始化函数
struct vnic *vnic_create(struct vnet *vnet) {
  int ret;
  struct vnic *vnic;

  // 分配并初始化网卡
  struct net_device *net_dev = alloc_etherdev(sizeof(struct vnic));
  if (!net_dev) {
    printk(KERN_ERR "Failed to allocate net_device\n");
    return NULL;
  }
  net_dev->netdev_ops = &vnic_netdev_ops;  // 设置操作函数
  strcpy(net_dev->name, "vnet%d");         // 设置网卡名称
  eth_hw_addr_random(net_dev);             // 设置随机 MAC 地址
  // memcpy(net_dev->dev_addr, "\x00\x11\x22\x33\x44\x55", ETH_ALEN);  // MAC
  // address

  vnic = netdev_priv(net_dev);
  vnic->vnet = vnet;
  vnic->net_device = net_dev;

  // 注册网卡
  ret = register_netdev(net_dev);
  if (ret) {
    printk(KERN_ERR "Failed to register net_device: %s\n", net_dev->name);
    free_netdev(net_dev);
    return NULL;
  }

  printk(KERN_INFO "vnet device registered: %s\n", net_dev->name);
  return vnic;
}

void vnic_destroy(struct vnic *vnic) {
  struct net_device *net_dev = vnic->net_device;
  unregister_netdev(net_dev);
  free_netdev(net_dev);
  printk(KERN_INFO "vnic instance unregistered: %s\n", net_dev->name);
}
