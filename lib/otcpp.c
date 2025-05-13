#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "otcpp.h"

double mpow(int base, int exp) {
    double ret = 1.0;
    while (exp-- != 0)
        ret *= (double)base;
    return ret;
}

char *frame_msg(char *buf, uint16_t buf_len, struct sockaddr_in udp_client) {
    uint16_t len = EMPT_LEN + buf_len + sizeof(struct sockaddr_in);
    char *frame = malloc(len);
    frame[0] = FRAME_DEL;
    frame[len - 1] = FRAME_DEL;
    uint16_t *length = (uint16_t*)(frame + 1);
    *length = htons(len);

    memcpy(frame + 3, &udp_client, sizeof(struct sockaddr_in));
    memcpy(frame + 3 + sizeof(struct sockaddr_in), buf, buf_len);
    return frame;
}

int send_all(int sockfd, char *frame, uint16_t len) {
    uint16_t sent = 0;
    while(len > sent) {
        int n = send(sockfd, frame + sent, len - sent, 0);
        if (n < 0) {
            perror("sendall");
            return -1;
        }
        sent += n;
    }
    return sent;
}

void reconstruct_msg(char *recv_buf, Frame *frame, int recv_bytes) {
    for (int i = 0; i < recv_bytes; i++) {
        uint8_t curr_char = recv_buf[i];
        if (curr_char == FRAME_DEL)
            frame->del++;
        else
            frame->buf[frame->idx++] = curr_char;
        if (frame->del == 2) {
            frame->idx = 0;
            frame->del = 0;
            print_frame(frame->buf);
        }
    }
}

void print_frame(char *buf) {
    uint16_t len = ntohs(((uint16_t*)buf)[0]) - EMPT_LEN;
    struct sockaddr_in udp_sender = *((struct sockaddr_in*)(buf + 2));
    char udp_sender_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &udp_sender.sin_addr, udp_sender_ip, INET_ADDRSTRLEN);
    uint16_t udp_sender_port = ntohs(udp_sender.sin_port);

    char topic[51];
    topic[50] = '\0';
    strncpy(topic, buf + 2 + sizeof(struct sockaddr_in), 50);
    printf("%s:%u - %s - ", udp_sender_ip, udp_sender_port, topic);
    buf = buf + 50 + sizeof(struct sockaddr_in);

    switch (buf[2]) {
        case 0:
            int x = *((int*)(buf + 4));
            x = ntohl(x);
            x *= buf[3] ? -1 : 1;
            printf("INT - %d\n", x);
            break;
        case 1:
            float y = *((int*)(buf + 3));
            y = ntohs(y) / 100.0;
            printf("SHORT_REAL - %.2f\n", y);
            break;
        case 2:
            int z = *((int*)(buf + 4));
            uint8_t exp = *((int*)(buf + 8));
            z = ntohl(z);
            z *= buf[3] ? -1 : 1;
            printf("FLOAT - %.8g\n", (double)z / mpow(10, exp));
            break;
        case 3:
            buf[len - 50 - sizeof(struct sockaddr_in) - 1 + 3] = '\0';
            printf("STRING - %s\n", buf + 3);
            break;
    }
}
