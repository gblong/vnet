# 定义模块名称
obj-m := vnet.o

# 列出源文件（不需要包含头文件）
#vnet-objs := vnet.o vnic.o cdevice.o blocking_queue.o
vnet-y := vnet_main.o vnic.o cdevice.o blocking_queue.o

# 内核源码路径
KDIR := /lib/modules/$(shell uname -r)/build

# 默认目标
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# 清理目标
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf echo_server mytelnet vnet_service capture_packet

service:
	gcc -o echo_server echo_server.c
	gcc mytelnet.c -o mytelnet
	gcc -o vnet_service vnet_service.c
	gcc -o capture_packet capture_packet.c -lpcap



