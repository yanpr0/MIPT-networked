#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <enet/enet.h>
#include <iostream>

void send_server_addr(ENetHost* lobby, bool broadcast = true, ENetPeer* peer = nullptr)
{
  const char addr[] = "localhost\00010888";
  ENetPacket* packet = ::enet_packet_create(addr, sizeof(addr), ENET_PACKET_FLAG_RELIABLE);
  if (broadcast)
  {
    ::enet_host_broadcast(lobby, 0, packet);
  }
  else
  {
    ::enet_peer_send(peer, 0, packet);
  }
}

int main(int argc, const char **argv)
{
  if (::enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  std::atexit(::enet_deinitialize);

  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10887;

  ENetHost *lobby = ::enet_host_create(&address, 32, 1, 0, 0);

  if (!lobby)
  {
    std::printf("Cannot create ENet server\n");
    return 1;
  }

  const char startCmd[] = "start";
  bool gameStarted = false;
  while (true)
  {
    ENetEvent event;
    while (::enet_host_service(lobby, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        std::printf("Connected player %x:%u established\n", event.peer->address.host, event.peer->address.port);
        if (gameStarted)
        {
          send_server_addr(nullptr, false, event.peer);
        }
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        std::printf("Packet received '%s'\n", event.packet->data);
        if (event.packet->dataLength == sizeof(startCmd) - 1 && std::strcmp(startCmd, reinterpret_cast<char*>(event.packet->data)) == 0)
        {
          std::puts("Sending server address to players");
          gameStarted = true;
          send_server_addr(lobby);
        }
        ::enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
  }

  return 0;
}

