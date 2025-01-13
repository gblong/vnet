#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080         // 服务端端口号
#define BIND_IP "192.168.1.1" // 特定的绑定 IP 地址
#define BUFFER_SIZE 1024  // 数据缓冲区大小

int main() {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  char buffer[BUFFER_SIZE];
  socklen_t addr_len = sizeof(client_addr);

  // 1. 创建套接字
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }
  // 将套接字绑定到特定网卡
   char *interface = "vnet0";
   if (setsockopt(server_fd, SOL_SOCKET, SO_BINDTODEVICE, interface,
                  strlen(interface)) < 0) {
     perror("setsockopt SO_BINDTODEVICE failed");
     close(server_fd);
     exit(1);
   }
   printf("Socket created successfully.\n");


   // Set the SO_REUSEADDR option
   int optval = 1;
   if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
	   perror("setsockopt for reuseaddr failed");
	   close(server_fd);
	   exit(EXIT_FAILURE);
   }

   // 2. 配置服务端地址
   server_addr.sin_family = AF_INET;          // IPv4
   //server_addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有地址
   server_addr.sin_addr.s_addr = inet_addr(BIND_IP); // 绑定到特定的 IP
   server_addr.sin_port = htons(PORT);        // 转换端口为网络字节序


   // 3. 绑定地址和端口
   if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
		   0) {
	   perror("Bind failed");
	   close(server_fd);
	   exit(EXIT_FAILURE);
   }
   printf("Bind to port %d successfully.\n", PORT);

   // 4. 监听连接
   if (listen(server_fd, 50) < 0) {
	   perror("Listen failed");
	   close(server_fd);
	   exit(EXIT_FAILURE);
   }
   printf("Listening for connections...\n");

   // 5. 主服务循环
   while (1) {
	   printf("Waiting for a connection...\n");

	   // 接受客户端连接
	   client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
	   if (client_fd < 0) {
		   perror("Accept failed");
		   close(server_fd);
		   exit(EXIT_FAILURE);
	   }
	   printf("Connection established with client: %s:%d\n",
			   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	   // 处理客户端数据
	   while (1) {
		   memset(buffer, 0, BUFFER_SIZE);
		   ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
		   if (bytes_received < 0) {
			   perror("Receive failed");
			   break;
		   } else if (bytes_received == 0) {
			   printf("Client disconnected.\n");
			   break;
		   }

		   printf("Received: %s", buffer);

		   // 回显数据给客户端
		   ssize_t bytes_sent = send(client_fd, buffer, bytes_received, 0);
		   if (bytes_sent < 0) {
			   perror("Send failed");
			   break;
		   }
	   }

	   // 关闭客户端套接字
	   close(client_fd);
	   printf("Client connection closed.\n");
   }

   // 关闭服务端套接字
   close(server_fd);
   printf("Server shut down.\n");

   return 0;
}
