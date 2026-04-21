#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstring>
#include <random>
#include <ctime>
#include "protocol.h"

int main()
{
    // Initialize Winsock — must be called before any socket API
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // SOCK_DGRAM = UDP
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    uint32_t timeout = 500;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));

    // Describing the destination: localhost port 9000
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(9000);
    const char* ip_address = "127.0.0.1";
    // Convert IP address as text → binary, assign it to dest.sin_addr
    inet_pton(AF_INET, ip_address, &dest.sin_addr);
    int sender_len = sizeof(dest);

    // Initialize loop variables
    char msg[16]; // 16 bytes allows us to send "Hello, UDP!" (11) plus 0-100 (1-3) plus "\n" (1)
    PacketHeader header{}, ack_header{};
    uint8_t buf[MAX_PACKET_SIZE], ack_buf[MAX_PACKET_SIZE];
    uint8_t offset, ack_offset;
    uint16_t seq, len, ack_seq, ack_len;
    int buf_size;
    int sent, received;

    // Track packet count (sequence)
    uint16_t packet_count = 0;
    // Track retransmit attempts for each packet (resets per packet)
    const uint8_t RETRY_MAX = 3;
    uint8_t retries = 0;

    // Randomizer for packet drop simulation
    std::mt19937 rng(static_cast<uint32_t>(time(nullptr)));
    const float DROP_RATE = 0.1f;
    const float DUPE_RATE = 0.1f;
    std::uniform_real_distribution<float> rand_dist(0.0f, 1.0f);
    bool do_packet_drop = false;
    bool do_fake_dupe = false;

    // Stop at 100 packets sent
    while (packet_count < 100)
    {
        printf("-------\n");
        // Intentionally send duplicate of previous packet (edge case guard: initial packet)
        do_fake_dupe = packet_count > 0 && rand_dist(rng) < DUPE_RATE;
        if (do_fake_dupe)
        {
            /*
            * The if block skips constructing the new packet.
            * We also skip incrementing the packet_count at the end with `if (do_fake_dupe)`
            */ 
            printf("Sending duplicate packet\n");
        }
        else
        {
            // ###################### Packet construction ######################
            // Payload (string) to be sent
            snprintf(msg, sizeof(msg), "Hello, UDP!%d", packet_count);

            header.type = static_cast<uint8_t>(PacketType::PKT_DATA);
            header.sequence = packet_count;
            header.length = static_cast<uint16_t>(strlen(msg));

            // Buffer
            offset = 0;

            // Serializing each field of the packet header
            buf[offset] = header.type;      offset += 1;
            seq = htons(header.sequence);
            memcpy(buf + offset, &seq, 2);  offset += 2;
            len = htons(header.length);
            memcpy(buf + offset, &len, 2);  offset += 2;

            // Appending msg to the packet
            memcpy(buf + offset, msg, strlen(msg));
            buf_size = static_cast<int>(HEADER_SERIALIZED_SIZE + strlen(msg));
            // #################################################################

            // Moving on to next loop if we "dropped" packet
            do_packet_drop = rand_dist(rng) < DROP_RATE;
            if (do_packet_drop)
            {
                printf("Simulating packet drop\n");
                continue;
            }
        }

        // sendto fires a single datagram; no connection needed beforehand
        sent = sendto(sock, reinterpret_cast<char*>(buf), buf_size, 0,
            reinterpret_cast<sockaddr*>(&dest), sender_len);

        // Print bytes and message
        printf("Sent %d bytes; type - %d; seq - %d; len - %d\n",
            sent, header.type, header.sequence, header.length);
        printf("msg: \"%s\"\n", msg);

        // Listen for ACK response
        received = recvfrom(sock, reinterpret_cast<char*>(ack_buf), sizeof(ack_buf), 0,
            reinterpret_cast<sockaddr*>(&dest), &sender_len);

        // Timeout check
        while (received == SOCKET_ERROR) 
        {
            if (WSAGetLastError() == WSAETIMEDOUT) 
            {
                // If timed out, print timeout error message, then retry send
                printf("######\n");
                printf("Error: ACK timeout for seq %d\n", packet_count);

                // Retry send
                printf("Retrying send - retries: %d\n", retries + 1);
                sent = sendto(sock, reinterpret_cast<char*>(buf), buf_size, 0,
                    reinterpret_cast<sockaddr*>(&dest), sender_len);
                // Print bytes and message
                printf("Retry sent %d bytes; type - %d; seq - %d; len - %d\n",
                    sent, header.type, header.sequence, header.length);
                printf("msg: \"%s\"\n", msg);

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

        ack_offset = 0;
        ack_header.type = ack_buf[ack_offset];      ack_offset += 1;
        memcpy(&ack_seq, ack_buf + ack_offset, 2);  ack_offset += 2;
        memcpy(&ack_len, ack_buf + ack_offset, 2);  ack_offset += 2;
        ack_header.sequence = ntohs(ack_seq);
        ack_header.length = ntohs(ack_len);

        // Print information about received ACK packet
        printf("Received ACK %d bytes, type - %d, seq - %d, len - %d\n", 
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
        if (!do_fake_dupe) ++packet_count;
    }

    closesocket(sock);
    WSACleanup(); // Release Winsock resources

    return 0;
}
