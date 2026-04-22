#pragma once
#include <cstdint>

struct PacketHeader
{
	uint8_t  type;
	uint16_t sequence;
	uint16_t length;
};

enum class PacketType : uint8_t
{
	PKT_DATA = 0,
	PKT_ACK = 1,
	PKT_END = 2,
};

inline constexpr size_t HEADER_SERIALIZED_SIZE = 5; // 1 + 2 + 2
inline constexpr size_t MAX_PACKET_SIZE = 508; // Safe even with max IP header size on minimum-MTU IPv4 path
inline constexpr size_t MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SERIALIZED_SIZE;