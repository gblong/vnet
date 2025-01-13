#!/bin/bash


sudo ifconfig vnet0 192.168.1.1
sudo ifconfig vnet1 192.168.2.2

sudo ip link set dev vnet0 arp off
sudo ip link set dev vnet1 arp off

sudo ip link set dev vnet0 multicast off
sudo ip link set dev vnet1 multicast off

sudo sysctl -w net.ipv6.conf.vnet0.disable_ipv6=1
sudo sysctl -w net.ipv6.conf.vnet1.disable_ipv6=1


