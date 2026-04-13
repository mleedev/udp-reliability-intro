#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>

#pragma comment(lib, "Ws2_32.lib")

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

    char buf[512];
    sockaddr_in sender{};
    int senderLen = sizeof(sender);

    // recvfrom blocks until a datagram arrives; fills sender with the client's address
    int n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                     reinterpret_cast<sockaddr*>(&sender), &senderLen);
    buf[n] = '\0';
    printf("Received: \"%s\"\n", buf);

    closesocket(sock);
    WSACleanup(); // Release Winsock resources
}
