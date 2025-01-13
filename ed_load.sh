#!/bin/sh

export PATH=/sbin:/bin

# Use a pathname, as new modutils don't look in the current dir by default
sudo insmod vnet.ko
sudo ifconfig vnet0 192.168.5.1

#确保内核支持 IP 转发
#sudo sysctl -w net.ipv4.ip_forward=1
#验证 IP 转发是否启用：
#cat /proc/sys/net/ipv4/ip_forward
# 输出应为 1



sudo ip addr add 192.168.1.1/24 dev vnet0
sudo ip addr add 192.168.2.1/24 dev vnet1
sudo ip link set vnet0 up
sudo ip link set vnet1 up


sudo ifconfig vnet0 192.168.1.1/24 
sudo ifconfig vnet1 192.168.2.1/24 
sudo ip route add 192.168.2.0/24 via 192.168.1.1 dev vnet0
sudo ip route add 192.168.1.0/24 via 192.168.2.1 dev vnet1

ip link set dev vnet0 arp off
ip link set dev vnet1 arp off

sysctl -w net.ipv6.conf.vnet0.disable_ipv6=1
sysctl -w net.ipv6.conf.vnet1.disable_ipv6=1
cat /proc/sys/net/ipv6/conf/vnet0/disable_ipv6


tcpdump -i eth0 ether proto 0x88cc #检查是否有 LLDP 数据包
ip link set dev vnet0 multicast off #LLDP 使用多播以太网帧进行通信，禁用多播可能间接关闭 LLDP

