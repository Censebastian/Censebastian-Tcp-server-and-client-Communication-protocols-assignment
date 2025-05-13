#ifndef OTCPP_H
#define OTCPP_H

#include <stdlib.h>

#define MAX_LEN 1501
#define EMPT_LEN 4
#define FRAME_DEL 228

typedef struct {
    int del;
    int idx;
    char buf[MAX_LEN + 2];
} Frame;

char *frame_msg(char *buf, uint16_t buf_len, struct sockaddr_in udp_client);
int send_all(int sockfd, char *frame, uint16_t len);
void reconstruct_msg(char *recv_buf, Frame *frame, int recv_bytes);
void print_frame(char *buf);
double mpow(int base, int exp);

#endif 
