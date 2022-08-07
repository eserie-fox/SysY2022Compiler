#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <memory>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fstream>

const size_t MAXN = 1000;
char buf[MAXN];
const int port = 3456;

int main() {
  std::ofstream out("output.txt", std::ios::out);
  sockaddr_in local_addr;
  sockaddr_in remote_addr;
  memset(&local_addr, 0, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = INADDR_ANY;
  local_addr.sin_port = htons(port);

  int server_fd;
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return 1;
  }

  if (bind(server_fd, (sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
    perror("bind");
    return 2;
  }

  listen(server_fd, 50);
  socklen_t raddr_len;
  while (true) {
    int client_fd = accept(server_fd, (sockaddr *)&remote_addr, &raddr_len);
    if (client_fd < 0) {
      perror("accept");
      return 3;
    }
    printf("Accept connection from %s\n", inet_ntoa(remote_addr.sin_addr));
    size_t data_len;
    while ((data_len = recv(client_fd, buf, MAXN - 1, 0)) > 0) {
      buf[data_len] = 0;
      std::cout << buf;
      out << buf;
    }
    std::cout << std::flush;
    out << std::flush;
  }

  return 0;
}