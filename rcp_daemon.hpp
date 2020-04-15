#include <cstddef>
#include <sys/epoll.h>

class RCPDaemon {
public:
  RCPDaemon(size_t init_port);
  void ServeForever();
private:
  void Serve();
  void AcceptClient();
  void InitEpoll();
  int sock_fd_;
  int epoll_fd_;
  size_t port_;
  struct epoll_event events_[4096];
};
