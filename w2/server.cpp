#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <enet/enet.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

std::unordered_map<ENetPeer*, std::pair<std::uint64_t, std::string>> players;

void broadcast_time_packet(ENetHost* server)
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

  std::string msg = "Server time: ";
  msg += timeStr;

  ENetPacket* packet = ::enet_packet_create(msg.c_str(), msg.size() + 1, ENET_PACKET_FLAG_RELIABLE);
  ::enet_host_broadcast(server, 0, packet);
}

void broadcast_ping_packet(ENetHost* server)
{
  std::string pings;

  for (const auto& [peer, player] : players)
  {
    pings += "ping of {" + std::to_string(player.first) + ", " + player.second + "}: " + std::to_string(peer->roundTripTime) + " ms\n";
  }

  ENetPacket* packet = ::enet_packet_create(pings.c_str(), pings.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  ::enet_host_broadcast(server, 1, packet);
}

std::string generate_nick(std::size_t len)
{
  std::string nick(len, '\0');

  for (auto& c : nick)
  {
    c = 'a' + std::rand() % ('z' - 'a' + 1);
  }

  return nick;
}

void init_new_player(ENetPeer* player)
{
  std::string nick = generate_nick(5);

  std::string msg = "New player: {" + std::to_string(players.size()) + ", " + nick + "}\n";
  std::string player_list = "Currently playing:\n";

  for (const auto& [peer, player] : players)
  {
    player_list += std::to_string(player.first) + ' ' + player.second + '\n';
    ENetPacket* packet = ::enet_packet_create(msg.c_str(), msg.size() + 1, ENET_PACKET_FLAG_RELIABLE);
    ::enet_peer_send(peer, 0, packet);
  }

  ENetPacket* packet = ::enet_packet_create(player_list.c_str(), player_list.size() + 1, ENET_PACKET_FLAG_RELIABLE);
  ::enet_peer_send(player, 0, packet);

  players.emplace(player, std::pair{static_cast<std::uint64_t>(players.size()), std::move(nick)});
}

int main(int argc, const char **argv)
{
  if (::enet_initialize() != 0)
  {
    std::printf("Cannot init ENet");
    return 1;
  }
  std::atexit(::enet_deinitialize);

  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10888;

  ENetHost *server = ::enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    std::printf("Cannot create ENet server\n");
    return 1;
  }

  std::uint32_t lastTimeSend = ::enet_time_get();
  std::uint32_t lastPingSend = lastTimeSend;
  while (true)
  {
    ENetEvent event;
    while (::enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
        case ENET_EVENT_TYPE_CONNECT:
          std::printf("Connected player %x:%u\n", event.peer->address.host, event.peer->address.port);
          init_new_player(event.peer);
          break;
        case ENET_EVENT_TYPE_RECEIVE:
          std::printf("Time of {%" PRIu64 ", %s}: %s", players[event.peer].first, players[event.peer].second.c_str(), event.packet->data);
          ::enet_packet_destroy(event.packet);
          break;
        default:
          break;
      };
    }
    std::uint32_t curTime = ::enet_time_get();
    if (curTime - lastTimeSend > 1000)
    {
      lastTimeSend = curTime;
      broadcast_time_packet(server);
    }
    if (curTime - lastPingSend > 2000)
    {
      lastPingSend = curTime;
      broadcast_ping_packet(server);
    }
  }

  ::enet_host_destroy(server);

  atexit(::enet_deinitialize);
  return 0;
}

