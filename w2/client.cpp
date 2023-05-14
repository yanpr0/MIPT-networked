#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <enet/enet.h>
#include <iostream>
#include <string>
#include <sys/select.h>
#include <time.h>

void send_time_packet(ENetPeer* peer)
{
  struct timespec t = {};
  if (-1 == ::clock_gettime(CLOCK_REALTIME, &t))
  {
    std::perror("clock_gettime");
    return;
  }
  char* timeStr = ::ctime(&t.tv_sec);
  if (timeStr == nullptr)
  {
    std::perror("ctime");
    return;
  }

  ENetPacket* packet = ::enet_packet_create(timeStr, std::strlen(timeStr) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  ::enet_peer_send(peer, 0, packet);
}

int main(int argc, const char** argv)
{
  if (::enet_initialize() != 0)
  {
    std::printf("Cannot init ENet");
    return 1;
  }

  std::atexit(::enet_deinitialize);

  ENetHost* client = ::enet_host_create(nullptr, 2, 3, 0, 0);
  if (!client)
  {
    std::printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  ::enet_address_set_host(&address, "localhost");
  address.port = 10887;

  ENetPeer* lobbyPeer = ::enet_host_connect(client, &address, 1, 0);
  if (!lobbyPeer)
  {
    std::printf("Cannot connect to lobby");
    return 1;
  }
  address.port = 10888;
  ENetPeer* serverPeer = nullptr;

  std::string input;
  bool connected = false;
  std::uint32_t lastSendTime = ::enet_time_get();
  while (true)
  {
    if (!connected)
    {
      fd_set readSet;
      FD_ZERO(&readSet);
      FD_SET(STDIN_FILENO, &readSet);

      timeval timeout = { 0, 10000 }; // 10 ms
      ::select(STDIN_FILENO + 1, &readSet, NULL, NULL, &timeout);

      if (FD_ISSET(STDIN_FILENO, &readSet))
      {
        std::getline(std::cin, input);

        const char startCmd[] = "start";

        if (std::strcmp(input.c_str(), startCmd) == 0)
        {
          ENetPacket* packet = ::enet_packet_create(input.c_str(), input.size(), ENET_PACKET_FLAG_RELIABLE);
          ::enet_peer_send(lobbyPeer, 0, packet);
        }
      }
    }

    ENetEvent event;
    while (::enet_host_service(client, &event, connected ? 10 : 0) > 0)
    {
      switch (event.type)
      {
        case ENET_EVENT_TYPE_CONNECT:
          std::printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
          if (event.peer == serverPeer)
          {
            connected = true;
          }
          break;
        case ENET_EVENT_TYPE_RECEIVE:
          if (!connected && event.peer == lobbyPeer)
          {
            ::enet_address_set_host(&address, reinterpret_cast<char*>(event.packet->data));
            address.port = std::strtol(std::strchr(reinterpret_cast<char*>(event.packet->data), '\0') + 1, nullptr, 10);

            serverPeer = ::enet_host_connect(client, &address, 2, 0);
            if (!serverPeer)
            {
              std::printf("Cannot connect to server");
              return 1;
            }
          }
          else if (connected && event.peer == serverPeer)
          {
            std::printf("%s", event.packet->data);
          }
          ::enet_packet_destroy(event.packet);
          break;
        default:
          break;
      };
    }
    if (connected)
    {
      std::uint32_t curTime = ::enet_time_get();
      if (curTime - lastSendTime > 1000)
      {
        lastSendTime = curTime;
        send_time_packet(serverPeer);
      }
    }
  }
  return 0;
}

