#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>

#include <cinttypes>
#include <cstdint>
#include <cstdio>

#include "protocol.h"
#include "socket_tools.h"


int main(int argc, const char **argv)
{
  int tfd = ::timerfd_create(CLOCK_MONOTONIC, 0);
  if (tfd == -1)
  {
    std::fputs("Fatal: cannot create a timer\n", stderr);
    return 1;
  }

  const char *port = "2023";

  addrinfo resAddrInfo;
  int sfd = create_dgram_socket("localhost", port, &resAddrInfo);

  if (sfd == -1)
  {
    std::fputs("Fatal: cannot create a socket\n", stderr);
    return 1;
  }

  ProtocolOpcode op = ProtocolOpcode::INIT;
  if (sizeof(op) != ::sendto(sfd, &op, sizeof(op), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen))
  {
    std::fputs("Fatal: initialization failed\n", stderr);
    std::perror("sendto");
    return 1;
  }

  ::fcntl(sfd, F_SETFL, ::fcntl(sfd, F_GETFL) & ~O_NONBLOCK);
  auto res = ::recv(sfd, &op, sizeof(op), 0);
  ::fcntl(sfd, F_SETFL, ::fcntl(sfd, F_GETFL) | O_NONBLOCK);

  if (res != 1 || op != ProtocolOpcode::ACK)
  {
    std::fputs("Fatal: initialization failed\n", stderr);
    if (res == -1)
    {
      std::perror("recv");
    }
    return 1;
  }

  if (::isatty(STDOUT_FILENO))
  {
    std::puts("Initialization complete, input Fibonacci number index, get its value");
  }

  //send keep-alive every 1 sec
  struct itimerspec ts = {
    .it_interval = {1, 0},
    .it_value = {1, 0},
  };

  ::timerfd_settime(tfd, 0, &ts, NULL);

  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(0, &readSet);
    FD_SET(tfd, &readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    ::select(sfd + 1, &readSet, NULL, NULL, &timeout);

    Msg msg = {};

    if (FD_ISSET(sfd, &readSet))
    {
      auto n = ::recv(sfd, &msg, sizeof(msg), 0);

      if (n == -1)
      {
        std::perror("Error: read");
      }
      else if (n != sizeof(msg))
      {
        std::fputs("Error: wrong message size\n", stderr);
      }
      else
      {
        switch (msg.op)
        {
          case ProtocolOpcode::RESPONSE:
          {
            std::printf("%" PRIu64 "\n", msg.num);
            break;
          }
          case ProtocolOpcode::ERROR:
          {
            std::fputs("Warning: uint64 overflow\n", stderr);
            std::printf("%" PRIu64 "\n", msg.num);
            break;
          }
          default:
          {
            std::fputs("Error: wrong message opcode\n", stderr);
          }
        }
      }
    }

    if (FD_ISSET(tfd, &readSet))
    {
      std::uint64_t expCnt = 0;
      ::read(tfd, &expCnt, sizeof(expCnt));

      ProtocolOpcode op = ProtocolOpcode::KEEP_ALIVE;
      if (sizeof(op) != ::sendto(sfd, &op, sizeof(op), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen))
      {
        std::perror("Error: keep-alive sending error: sendto");
      }

    }

    if (FD_ISSET(0, &readSet))
    {
      msg.op = ProtocolOpcode::REQUEST;
      if (std::scanf("%" SCNu64, &msg.num) == 0)
      {
        std::fputs("Error: not a number\n", stderr);
        return 1;
      }

      if (sizeof(msg) != ::sendto(sfd, &msg, sizeof(msg), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen))
      {
        std::perror("Error: request sending error: sendto");
      }
    }
  }

  ::close(sfd);
  ::close(tfd);
  return 0;
}

