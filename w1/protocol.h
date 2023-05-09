#pragma once

#include <cstdint>

enum class ProtocolOpcode : char
{
    INIT,
    ACK,
    REQUEST,
    RESPONSE,
    KEEP_ALIVE,
    ERROR
};

struct Msg
{
  ProtocolOpcode op;
  std::uint64_t num;
};

