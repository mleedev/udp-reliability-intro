#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstring>
#include "protocol.h"

int main()
{
    // Initialize Winsock — must be called before any socket API
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // SOCK_DGRAM = UDP
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Describing the destination: localhost port 9000
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(9000);
    const char* ip_address = "127.0.0.1";
    // Convert IP address as text → binary, assign it to dest.sin_addr
    inet_pton(AF_INET, ip_address, &dest.sin_addr); 

    // Payload (string) to be sent
    const char* msg = "Hello, UDP!";

    // Packet header
    PacketHeader header{};
    header.type = static_cast<uint8_t>(PacketType::PKT_DATA);
    header.sequence = 0;
    header.length = static_cast<uint16_t>(strlen(msg));

    // Buffer
    uint8_t buf[MAX_PACKET_SIZE];
    uint8_t offset = 0;
    
    // Serializing each field of the packet header
    buf[offset] = header.type;              offset += 1;
    uint16_t seq = htons(header.sequence);
    memcpy(buf + offset, &seq, 2);          offset += 2;
    uint16_t len = htons(header.length);
    memcpy(buf + offset, &len, 2);          offset += 2;

    // Appending msg to the packet
    memcpy(buf + offset, msg, strlen(msg));

    // sendto fires a single datagram; no connection needed beforehand
    int buf_size = static_cast<int>(HEADER_SERIALIZED_SIZE + strlen(msg));
    int sent = sendto(sock, reinterpret_cast<char*>(buf), buf_size, 0,
                        reinterpret_cast<sockaddr*>(&dest), sizeof(dest));

    // Print bytes and message
    printf("Sent %d bytes: \"%s\"\n", sent, msg);

    closesocket(sock);
    WSACleanup(); // Release Winsock resources
}
