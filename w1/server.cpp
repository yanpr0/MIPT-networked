#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <utility>

#include "protocol.h"
#include "socket_tools.h"


class Fib
{
public:
  // O(log(n))
  std::pair<std::uint64_t, bool> operator()(std::uint64_t n)
  {
    bool overflow = n >= overflow_index_;

    if (auto val = cache_.find(n); val != cache_.end())
    {
      return {val->second, overflow};
    }

    std::uint64_t init[2][2] =
    {
      {1, 1},
      {1, 0}
    };

    std::uint64_t val[2][2] =
    {
      {1, 1},
      {1, 0}
    };

    while (n > 0)
    {
      if (n & 1)
      {
        std::uint64_t tmp[2][2] = {};
        tmp[0][0] = val[0][0] * init[0][0] + val[0][1] * init[1][0];
        tmp[0][1] = val[0][0] * init[0][1] + val[0][1] * init[1][1];
        tmp[1][0] = val[1][0] * init[0][0] + val[1][1] * init[1][0];
        tmp[1][1] = val[1][0] * init[0][1] + val[1][1] * init[1][1];
        std::memcpy(val, tmp, sizeof(tmp));
      }

      std::uint64_t tmp[2][2] = {};
      tmp[0][0] = init[0][0] * init[0][0] + init[0][1] * init[1][0];
      tmp[0][1] = init[0][0] * init[0][1] + init[0][1] * init[1][1];
      tmp[1][0] = init[1][0] * init[0][0] + init[1][1] * init[1][0];
      tmp[1][1] = init[1][0] * init[0][1] + init[1][1] * init[1][1];
      std::memcpy(init, tmp, sizeof(tmp));

      n >>= 1;
    }

    if (cache_.size() == max_cache_size_)
    {
      cache_.erase(cache_.begin());
    }
    cache_.emplace(n, val[1][1]);

    return {val[1][1], overflow};
  }

private:
  static constexpr std::size_t max_cache_size_ = 1024;
  static constexpr std::uint64_t overflow_index_ = 94;

  std::unordered_map<std::uint64_t, std::uint64_t> cache_;
};


int main(int argc, const char **argv)
{
  const char *port = "2023";

  int sfd = create_dgram_socket(nullptr, port, nullptr);

  if (sfd == -1)
  {
    std::fputs("Fatal: cannot create a socket\n", stderr);
    return 1;
  }

  std::puts("listening!");

  struct sockaddr_storage addr = {};
  socklen_t addrLen = 0;

  Fib fib;

  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    ::select(sfd + 1, &readSet, NULL, NULL, &timeout);


    if (FD_ISSET(sfd, &readSet))
    {
      Msg msg = {};

      addrLen = sizeof(addr);
      ssize_t numBytes = ::recvfrom(sfd, &msg, sizeof(msg), 0, reinterpret_cast<sockaddr*>(&addr), &addrLen);

      if (numBytes == -1)
      {
        std::perror("Error: recvfrom");
        continue;
      }

      switch (msg.op)
      {
        case ProtocolOpcode::INIT:
        {
          ProtocolOpcode op = ProtocolOpcode::ACK;
          if (sizeof(op) != ::sendto(sfd, &op, sizeof(op), 0, reinterpret_cast<sockaddr*>(&addr), addrLen))
          {
            std::perror("Error: sendto");
          }
          else
          {
            std::fputs("Connected client\n", stderr);
          }
          break;
        }
        case ProtocolOpcode::REQUEST:
        {
          auto [num, err] = fib(msg.num);
          msg.op = err ? ProtocolOpcode::ERROR : ProtocolOpcode::RESPONSE;
          msg.num = num;
          if (sizeof(msg) != ::sendto(sfd, &msg, sizeof(msg), 0, reinterpret_cast<sockaddr*>(&addr), addrLen))
          {
            std::perror("Error: response sending error: sendto");
          }
          break;
        }
        case ProtocolOpcode::KEEP_ALIVE:
        {
          std::fputs("KEEP_ALIVE\n", stderr);
          break;
        }
        default:
        {
          std::fputs("Error: wrong message opcode\n", stderr);
        }
      }
    }
  }

  ::close(sfd);
  return 0;
}

