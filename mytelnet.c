#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void error(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  const char *hostname = argv[1];
  int port = atoi(argv[2]);

  // Create socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) error("Error creating socket");

  char *interface = "vnet1";
  // 将套接字绑定到特定网卡
  if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, interface,
                 strlen(interface)) < 0) {
    perror("setsockopt SO_BINDTODEVICE failed");
    close(sockfd);
    exit(1);
  }

  // Resolve hostname
  struct hostent *server = gethostbyname(hostname);
  if (server == NULL) {
    fprintf(stderr, "Error: No such host\n");
    exit(EXIT_FAILURE);
  }

  // Setup server address structure
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  server_addr.sin_port = htons(port);

  // Connect to server
  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("connect failed");
    close(sockfd);
    exit(1);
  }

  printf("Connected to %s on port %d using interface %s\n", hostname, port,
         interface);

  char buffer[BUFFER_SIZE];
  fd_set read_fds;

  while (1) {
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(sockfd, &read_fds);

    int max_fd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

    if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
      error("Error on select");

    // Check if there's input from stdin
    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      memset(buffer, 0, BUFFER_SIZE);
      if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;

      if (write(sockfd, buffer, strlen(buffer)) < 0)
        error("Error writing to socket");
    }

    // Check if there's data from the server
    if (FD_ISSET(sockfd, &read_fds)) {
      memset(buffer, 0, BUFFER_SIZE);
      int n = read(sockfd, buffer, BUFFER_SIZE - 1);
      if (n < 0)
        error("Error reading from socket");
      else if (n == 0) {
        printf("Server closed the connection\n");
        break;
      }

      printf("%s", buffer);
    }
  }

  close(sockfd);
  return 0;
}
