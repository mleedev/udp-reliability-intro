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
    uint32_t timeout = 2000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));

    // Describing the destination: localhost port 9000
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(9000);
    const char* ip_address = "127.0.0.1";
    // Convert IP address as text → binary, assign it to dest.sin_addr
    inet_pton(AF_INET, ip_address, &dest.sin_addr);
    int sender_len = sizeof(dest);



    // Send 500 packets
    uint16_t packet_count = 0;
    uint8_t retries = 0;
    while (packet_count < 100) 
    {
        // Payload (string) to be sent
        const char msg[16] = "Hello, UDP!";

        PacketHeader header{};
        header.type = static_cast<uint8_t>(PacketType::PKT_DATA);
        header.sequence = packet_count;
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
            reinterpret_cast<sockaddr*>(&dest), sender_len);

        // Print bytes and message
        printf("-------\n");
        printf("Sent %d bytes; type - %d; seq - %d; len - %d\n",
            sent, header.type, header.sequence, header.length);
        printf("msg: \"%s\"\n", msg);

        // Listen for ACK response
        uint8_t ack_buf[MAX_PACKET_SIZE];
        int received = recvfrom(sock, reinterpret_cast<char*>(ack_buf), sizeof(ack_buf), 0,
            reinterpret_cast<sockaddr*>(&dest), &sender_len);

        const uint8_t RETRY_MAX = 3;
        // Timeout check
        while (received == SOCKET_ERROR) 
        {
            if (WSAGetLastError() == WSAETIMEDOUT) 
            {
                // If timed out, print timeout error message, then retry send
                printf("###\n");
                printf("Error: ACK timeout for seq %d\n", packet_count);

                // Retry send
                printf("Retrying send\n");
                sent = sendto(sock, reinterpret_cast<char*>(buf), buf_size, 0,
                    reinterpret_cast<sockaddr*>(&dest), sender_len);
                // Print bytes and message
                printf("Retry sent %d bytes; type - %d; seq - %d; len - %d\n",
                    sent, header.type, header.sequence, header.length);
                printf("msg: \"%s\"\n", msg);
                printf("###");

                received = recvfrom(sock, reinterpret_cast<char*>(ack_buf), sizeof(ack_buf), 0,
                    reinterpret_cast<sockaddr*>(&dest), &sender_len);

                // Increment retry counter
                ++retries;

                // Need to exit gracefully if we reach retry maximum
                if (retries == RETRY_MAX) 
                {
                    printf("Max retries hit for seq %d\n", packet_count);
                    closesocket(sock);
                    WSACleanup();
                    return 1;
                }
            }
            else 
            {
                printf("Error: recvfrom failed with %d\n", WSAGetLastError());
                closesocket(sock);
                WSACleanup();
                return 1;
            }
        }
        // Reset retry counter for next loop
        retries = 0;

        PacketHeader ack_header{};
        uint8_t ack_offset = 0;
        ack_header.type = ack_buf[ack_offset];      ack_offset += 1;
        uint16_t ack_seq, ack_len;
        memcpy(&ack_seq, ack_buf + ack_offset, 2);  ack_offset += 2;
        memcpy(&ack_len, ack_buf + ack_offset, 2);  ack_offset += 2;
        ack_header.sequence = ntohs(ack_seq);
        ack_header.length = ntohs(ack_len);

        // Print information about received ACK packet
        printf("Received %d bytes; type - %d; seq - %d; len - %d\n", 
            received, ack_header.type, ack_header.sequence, ack_header.length);

        // If header.type isn't the ACK type then print an error (this shouldn't happen for this demo)
        if (ack_header.type == static_cast<uint8_t>(PacketType::PKT_DATA)) 
        {
            printf("Error: Received data packet while expecting ACK");
        }
        if (ack_header.sequence != header.sequence) 
        {
            printf("Error: Mismatching sequence in ACK header");
        }

        // Increment packet count, move to next loop iteration
        ++packet_count;
    }

    closesocket(sock);
    WSACleanup(); // Release Winsock resources

    return 0;
}
