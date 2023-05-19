#include <cmath>
#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <utility>

static constexpr int sendIntervalMs = 40;
static constexpr int aiEntCount = 10;
static std::vector<Entity> entities;
static std::vector<std::pair<float, float>> aiTargets;
static std::map<uint16_t, ENetPeer*> controlledMap;
static std::vector<bool> collided;

void init_ai_ents()
{
  for (uint16_t i = 0; i < aiEntCount; ++i)
  {
    uint32_t color = 0x000000ff +
      ((rand() % 256) << 8) +
      ((rand() % 256) << 16) +
      ((rand() % 256) << 24);
    float x = -300 + (rand() % 600);
    float y = -300 + (rand() % 600);
    float radius = 10 + rand() % 20;
    Entity ent = {color, x, y, radius, i};
    entities.push_back(ent);
    aiTargets.emplace_back(-300 + (rand() % 600), -300 + (rand() % 600));
  }
}

void ai_ents_move()
{
  constexpr float speed = 70;
  for (uint16_t i = 0; i < aiEntCount; ++i)
  {
    float xDist = aiTargets[i].first - entities[i].x;
    float yDist = aiTargets[i].second - entities[i].y;
    float dt = sendIntervalMs / 1000.f;
    float ds = speed * dt;
    float dist = std::sqrt(xDist * xDist + yDist * yDist);
    if (dist < ds)
    {
      entities[i].x = aiTargets[i].first;
      entities[i].y = aiTargets[i].second;
      float x = -300 + (rand() % 600);
      float y = -300 + (rand() % 600);
      aiTargets[i] = {x, y};
    }
    else
    {
      entities[i].x += ds * xDist / dist;
      entities[i].y += ds * yDist / dist;
    }
  }
}

void proceed_collisions()
{
  auto dist = [](const Entity& a, const Entity& b)
  {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
  };

  collided.assign(entities.size(), false);

  for (auto& a : entities)
  {
    for (auto& b : entities)
    {
      if (a.r < b.r && dist(a, b) < a.r + b.r)
      {
        b.r = std::sqrt(b.r * b.r + a.r * a.r / 2.f); // taking half of mass
        a.r /= std::sqrt(2);
        if (a.r < 10.f)
        {
          a.r = 10.f + (rand() % 1000) / 1000.f; // prevent equal min sizes and no-eating
        }
        a.x = -300 + (rand() % 600);
        a.y = -300 + (rand() % 600);
        collided[a.eid] = true;
        collided[b.eid] = true;
      }
    }
  }
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
    maxEid = std::max(maxEid, e.eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = 0x000000ff +
                   ((rand() % 256) << 8) +
                   ((rand() % 256) << 16) +
                   ((rand() % 256) << 24);
  float x = -300 + (rand() % 600);
  float y = -300 + (rand() % 600);
  float radius = 10 + rand() % 20;
  Entity ent = {color, x, y, radius, newEid};
  entities.push_back(ent);

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f;
  deserialize_entity_state(packet, eid, x, y);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
    }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  atexit(enet_deinitialize);

  init_ai_ents();

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }

    ai_ents_move();
    proceed_collisions();

    for (const Entity &e : entities)
    {
      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        if (controlledMap[e.eid] != peer || collided[e.eid]) // nullptr's for ai
        {
          send_snapshot(peer, e.eid, e.x, e.y, e.r);
        }
      }
    }

    usleep(sendIntervalMs * 1000);
  }

  enet_host_destroy(server);

  return 0;
}


