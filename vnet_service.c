#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 2000

int open_dev(char *file_name) {
  int fd;
  fd = open(file_name, O_RDWR);
  if (fd < 0) {
    perror("Open cdevice error.\n");
    exit(1);
  }

  // 设置文件描述符为非阻塞模式
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl get failed");
    close(fd);
    exit(1);
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl set failed");
    close(fd);
    exit(1);
  }

  return fd;
}

void read_and_write(int rfd, int wfd) {
  int nread = 0;
  int nwrite = 0;
  char buffer[BUFFER_SIZE];
  if ((nread = read(rfd, buffer, BUFFER_SIZE)) > 0) {
    if ((nwrite = write(wfd, buffer, nread)) > 0) {
      // printf("Transmission successful: %d bytes transfered.\n", nwrite);
      return;
    }
  }
  // perror("read nothing.\n");
}

int main() {
  int sfd = open_dev("/dev/cdevice0");
  int cfd = open_dev("/dev/cdevice1");

  while (1) {
    read_and_write(cfd, sfd);
    usleep(10000);
    read_and_write(sfd, cfd);
    usleep(10000);
  }

  close(sfd);
  close(cfd);

  return EXIT_SUCCESS;
}
