#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

int main()
{
    // Initialize Winsock — must be called before any socket API
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // SOCK_DGRAM = UDP
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Describe the destination: localhost port 9000
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr); // Localhot IP text → binary

    const char* msg = "Hello, UDP!";

    // sendto fires a single datagram; no connection needed beforehand
    int sent = sendto(sock, msg, static_cast<int>(strlen(msg)), 0,
                      reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
    printf("Sent %d bytes: \"%s\"\n", sent, msg);

    closesocket(sock);
    WSACleanup(); // Release Winsock resources
}
