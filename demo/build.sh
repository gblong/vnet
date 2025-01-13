set -x
make CC=/usr/bin/gcc-9
sudo rmmod wait_event
sudo insmod wait_event.ko
