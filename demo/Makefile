# 定义模块名称
obj-m := wait_event.o


# 内核源码路径
KDIR := /lib/modules/$(shell uname -r)/build

# 默认目标
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# 清理目标
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

service:
	gcc -o signal_test signal_test.c



