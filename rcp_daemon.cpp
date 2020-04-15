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
#include <pthread.h>

#include "rcp_daemon.hpp"
#include "rcp_packet.hpp"

#define REQUEST 1;
#define REPLY 2;
#define CALLBACK 3;

struct thread_arg {
  int caller_fd;
  long syscall;
  size_t parrent_port;
  pthread_t thread_id;
};

struct thread_result{
  int caller_fd;
  long syscall_result;
};



void *thread_routine(void *arg)
{
  thread_arg *data_str = (thread_arg*) arg;
  long result = syscall(data_str->syscall);
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct in_addr server_in_addr = {
        .s_addr = 0,
    };
  inet_aton("127.0.0.1", &server_in_addr);
  struct sockaddr_in server_sockaddr = {
        .sin_family = AF_INET,
        .sin_addr = server_in_addr,
        .sin_port = htons(data_str->parrent_port),
    };
  int connection = connect(sock_fd, (const struct sockaddr*)&server_sockaddr, sizeof(struct sockaddr_in));
  thread_result *return_data = (thread_result*) malloc(sizeof(thread_result));
  return_data->caller_fd = data->caller_fd;
  return_data->syscall_result = result;
  RCPPacket callback_data;
  callback_data.type = CALLBACK;
  callback_data.thread_id = (long long) data_str->thread_id;
  write(sock_fd, &callback_data, sizeof(callback_data));
  free(arg);
  return (void*) return_data;
}

RCPDaemon::RCPDaemon(size_t port) : port_(port) {
}

void RCPDaemon::ServeForever()
{
  sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  int flags = fcntl(sock_fd_, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(sock_fd_, F_SETFL, flags);
  struct in_addr server_in_addr = {
        .s_addr = 0,
    };
  inet_aton("127.0.0.1", &server_in_addr);
  struct sockaddr_in server_sockaddr = {
        .sin_family = AF_INET,
        .sin_addr = server_in_addr,
        .sin_port = htons(port_),
    };
  bind(sock_fd_, (const struct sockaddr*)&server_sockaddr, sizeof(struct sockaddr_in));
  listen(sock_fd_, 1);
  InitEpoll();
  Serve();
  shutdown(sock_fd_, 2);
  close(sock_fd_);
}

void RCPDaemon::InitEpoll() {
  epoll_fd_ = epoll_create1(0);
  struct epoll_event server_event;
  server_event.events = EPOLLIN;
  server_event.data.fd = sock_fd_;
  epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sock_fd_, &server_event);
}

void RCPDaemon::AcceptClient() {
  int client_fd = accept(sock_fd, NULL, NULL);
  int flags = fcntl(client_fd, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(client_fd, F_SETFL, flags);
  struct epoll_event tmp_ev;
  client_event.events = EPOLLIN;
  client_event.data.fd = client_fd;
  epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &client_event);
}

void RCPDaemon::Serve() {
  while (true) {
    to_process = epoll_wait(epoll_fd, events, 4096, -1);
    for (int i = 0; i < to_process; ++i) {
      if (events_[i].data.fd != sock_fd) {
          RCPPacket received_packet;
          int data_size = read(events[i].data.fd, &received_packet, sizeof(received_packet));
          if (received_packet.type == CALLBACK) {
              thread_result* thread_data;
              pthread_join((pthread_t)received_packet.data, (void**)&thread_data);
              RCPPacket return_data;
              return_data.type = REPLY;
              return_data.result = thread_data->syscall_result;
              write(thread_data->caller_fd, &return_data, sizeof(return_data));
              free(thread_data);
          }
          if (received_packet.type == REQUEST) {
              thread_arg* routine_data = (thread_arg*) malloc (sizeof(routine_data));
              routine_data->caller_fd = events_[i].data.fd;
              routine_data->syscall = received_packet.syscall_number;
              routine_data->parrent_port = port_;
              pthread_create(&(routine_data->thread_id), NULL, thread_routine, (void*) routine_data);
          }
      } else {
          AcceptClient();
      }
    }
  }
}
