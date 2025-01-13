#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int fd;

// 信号处理函数
void handle_signal(int sig) { printf("Signal %d received.\n", sig); }

int main() {
  char buffer[100];

  // 设置信号处理
  signal(SIGINT, handle_signal);

  // 打开设备文件
  fd = open("/dev/signal_demo", O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    perror("Failed to open device");
    return 1;
  }

  printf("Reading from device. Press Ctrl+C to send SIGINT.\n");

  // 调用 read 函数（阻塞）
  int n = 0;
  n = read(fd, buffer, sizeof(buffer));
  printf("read return: %d\n", n);
  if (n < 0) {
    perror("Read interrupted");
  } else {
    printf("Data from device: %s\n", buffer);
  }

  sleep(1);
  close(fd);
  return 0;
}
