#define _GNU_SOURCE
#include <arpa/inet.h>
#include <ctype.h>
#include <cstdint>
#include <netinet/ip.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "rcp_client.hpp"
#include "rcp_packet.hpp"
#define REQUEST 1;

const char DAEMON_IP[] = "127.0.0.1";
const int DAEMON_PORT = 9091;


RCPClient::RCPClient(){
}

void RCPClient::connect(char *ip, int port) {
  sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  struct in_addr server_in_addr = {
        .s_addr = 0,
    };
  inet_aton(ip, &server_in_addr);
  struct sockaddr_in server_sockaddr = {
        .sin_family = AF_INET,
        .sin_addr = server_in_addr,
        .sin_port = htons(port),
    };
  int connection = connect(sock_fd_, (const struct sockaddr*)&server_sockaddr, sizeof(struct sockaddr_in));
}

long RCPClient::Execute(long syscall) {
  connect(DAEMON_IP, DAEMON_PORT);
  RCPPacket packet_data;
  packet_data.type = REQUEST;
  packet_data.syscall_number = syscall;
  write(sock_fd_, &packet_data, sizeof(packet_data));
}
