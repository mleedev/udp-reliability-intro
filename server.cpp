#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include "protocol.h"

int main()
{
    // Initialize Winsock — must be called before any socket API
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // SOCK_DGRAM = UDP (no connection, no guarantees)
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Bind to port 9000 on all local interfaces so we can receive
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(9000); // htons converts host→network byte order
    addr.sin_addr.s_addr = INADDR_ANY; 
    bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    printf("Server listening on UDP port 9000...\n");

    uint8_t buf[MAX_PACKET_SIZE];
    sockaddr_in sender{};
    int senderLen = sizeof(sender);

    // recvfrom blocks until a datagram arrives; fills sender with the client's address
    int n = recvfrom(sock, reinterpret_cast<char*>(buf), sizeof(buf) - 1, 0,
                     reinterpret_cast<sockaddr*>(&sender), &senderLen);

    // Packet deserialization variables
    uint8_t offset = 0;
    PacketHeader header{};
    
    // Deserializing and interpreting packet
    header.type = buf[offset];      offset += 1;
    uint16_t seq, len;
    memcpy(&seq, buf + offset, 2);  offset += 2;
    memcpy(&len, buf + offset, 2);  offset += 2;
    header.sequence = ntohs(seq);
    header.length = ntohs(len);
    uint8_t msg[MAX_PAYLOAD_SIZE + 1];
    memcpy(msg, buf + offset, header.length);
    msg[header.length] = '\0';

    printf("Received %d bytes\n", n);
    printf("msg: \"%s\"\n", msg);
    printf("type: %d\n", header.type);
    printf("sequence: %d\n", header.sequence);
    printf("length: % d\n", header.length);

    closesocket(sock);
    WSACleanup(); // Release Winsock resources
}
