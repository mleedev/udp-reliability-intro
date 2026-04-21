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

    // Sender address
    sockaddr_in sender{};
    int senderLen = sizeof(sender);

    // Initialize memory for loop variables: buffer, message, headers, trackers, etc
    uint8_t buf[MAX_PACKET_SIZE], ack_buf[MAX_PACKET_SIZE];
    uint8_t msg[MAX_PAYLOAD_SIZE + 1];
    PacketHeader header{}, ack_header{};
    uint8_t offset = 0, ack_offset = 0;
    uint16_t seq, len, ack_seq, ack_len;
    int received, sent;

    // Track latest sequence number for duplicate suppression
    uint16_t last_seq = UINT16_MAX;

    // Listen and receive packet loop
    while (true) {
        // recvfrom blocks until a datagram arrives; fills sender with the client's address
        received = recvfrom(sock, reinterpret_cast<char*>(buf), sizeof(buf) - 1, 0,
            reinterpret_cast<sockaddr*>(&sender), &senderLen);

        // Packet deserialization variables
        offset = 0;

        // Deserializing and interpreting packet
        header.type = buf[offset];      offset += 1;
        memcpy(&seq, buf + offset, 2);  offset += 2;
        memcpy(&len, buf + offset, 2);  offset += 2;
        header.sequence = ntohs(seq);
        header.length = ntohs(len);

        memcpy(msg, buf + offset, header.length);
        msg[header.length] = '\0';

        // If the received seq is not equal to the expected seq
        if (header.sequence != last_seq + 1)
        {
            // Duplicate suppression, check against last received sequence
            if (last_seq == header.sequence)
            {
                /*
                * Duplicate detected, sender didn't get our last ACK, resend it.
                * Note: ACK variable values are being used from previous loop.
                */
                sent = sendto(sock, reinterpret_cast<char*>(ack_buf), ack_offset, 0,
                    reinterpret_cast<sockaddr*>(&sender), senderLen);

                // Print resposne ACK packet info
                printf("Sent %d bytes; type - %d; seq - %d; len - %d\n",
                    sent, ack_header.type, ack_header.sequence, ack_header.length);

            }
            else
            {
                printf("Received out-of-order packet, dropping it. Expected seq %d, received seq %d\n", 
                    last_seq + 1, header.sequence);
            }
            // Proceed to next loop without printing received packet (we are dropping it)
        }
        else
        {
            // Printing packet information
            printf("------\n");
            printf("Received %d bytes; type - %d; seq - %d; len - %d\n",
                received, header.type, header.sequence, header.length);
            printf("msg: \"%s\"\n", msg);

            // Track latest sequence for duplicate suppression
            last_seq = header.sequence;

            // If we received a data packet, send an ACK back
            if (header.type == static_cast<uint8_t>(PacketType::PKT_DATA))
            {
                // Visualizing packet header for ACK response packet
                ack_header.type = static_cast<uint8_t>(PacketType::PKT_ACK);
                ack_header.sequence = header.sequence;
                ack_header.length = 0;

                // Serializing ACK response packet header
                ack_offset = 0;
                ack_buf[ack_offset] = ack_header.type;          ack_offset += 1;
                ack_seq = htons(ack_header.sequence);
                memcpy(ack_buf + ack_offset, &ack_seq, 2);      ack_offset += 2;
                ack_len = htons(ack_header.length);
                memcpy(ack_buf + ack_offset, &ack_len, 2);      ack_offset += 2;
                sent = sendto(sock, reinterpret_cast<char*>(ack_buf), ack_offset, 0,
                    reinterpret_cast<sockaddr*>(&sender), senderLen);

                // Print resposne ACK packet info
                printf("Sent %d bytes; type - %d; seq - %d; len - %d\n",
                    sent, ack_header.type, ack_header.sequence, ack_header.length);
            }
        }
    }

    closesocket(sock);
    WSACleanup(); // Release Winsock resources

    return 0;
}
