#include <cstddef>

class RCPClient {
public:
  RCPClient();
  long Execute(long syscall);
private:
  void Connect(char *ip, int port);
  int sock_fd_;
};
