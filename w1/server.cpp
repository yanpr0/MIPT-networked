#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include "socket_tools.h"

int main(int argc, const char **argv)
{
  const char *port = "2022";

  int sfd = create_dgram_socket(nullptr, port, nullptr);

  if (sfd == -1)
    return 1;
  printf("listening!\n");

  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd + 1, &readSet, NULL, NULL, &timeout);


    if (FD_ISSET(sfd, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      ssize_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, nullptr, nullptr);
      if (numBytes > 0)
        printf("%s\n", buffer); // assume that buffer is a string
    }
  }
  return 0;
}
