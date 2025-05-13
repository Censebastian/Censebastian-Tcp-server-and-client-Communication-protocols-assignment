#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "server_lib.h"
#include "otcpp.h"

int my_atoi(char *str) {
    int n = 0;
    int len = strlen(str);

    while (len) {
        if (str[0] > '9' || str[0] < '0') { 
            return -1;
        }
        n *= 10;
        n += str[0] - '0';
        strcpy(str, str + 1);
        len--;
    }

    return n;
}

int atoip(char *ip_str, uint32_t *ip) {
    char *tok = strtok(ip_str, ".");

    while(tok) {
        *ip <<= 8;
        int seg = my_atoi(tok);
        if (seg == -1) {
            return -1;
        }
        *ip += seg & 255;
        tok = strtok(NULL, ".");
    }
    
    return 0;
}

void run_client(int sockfd) {
    struct pollfd poll_fds[2];
    char *base_msg = "client to server\n";
    char buf[1024];
    buf[1023] = '\0';
    Frame frame;
    frame.del = 0;
    frame.idx = 0;
    List *topics = init_list(sizeof(Topic));

    poll_fds[0].fd = STDIN;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = sockfd;
    poll_fds[1].events = POLLIN;

    while (1) {
        if (poll(poll_fds, 2, -1) < 0)
            printf("poll failed\n");

        if (poll_fds[0].revents == POLLIN) {
            if (fgets(buf, sizeof(buf), stdin) == NULL)
                printf("fgets eof error");
            buf[strlen(buf) - 1] = '\0';
            if (starts_with(buf, "subscribe ")) {
                Topic new_topic = create_topic(buf + strlen("subscribe "));
                if (!contains(topics, &new_topic)) {
                    add_to_list(topics, &new_topic);
                    printf("Subscribed to topic %s.\n", buf + strlen("subscribe "));
                }
            } else if (starts_with(buf, "unsubscribe ")) {
                Topic topic = create_topic(buf + strlen("unsubscribe "));
                if (contains(topics, &topic)) {
                    remove_elem(topics, &topic, 0);
                    printf("Unsubscribed from topic %s.\n", buf + strlen("unsubscribe "));
                }
            } else if (!strcmp(buf, "exit")) {
                return;
            }
            send(sockfd, (void *)buf, sizeof(buf), 0);
        }
        if (poll_fds[1].revents == POLLIN) {
            int recv_ret = recv(sockfd, buf, 1023, 0);
            if (!recv_ret) {
                printf("server ended connection\n");
                return;
            }
            reconstruct_msg(buf, &frame, recv_ret);
        }
    }

}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    if (argc != 4) {
        printf("enter [client_id], [server_ip], [server_port]\n");
        return 1;
    }

    int client_id = my_atoi(argv[1] + 1);
    uint32_t ip = 0;
    int rc = atoip(argv[2], &ip);
    int port = my_atoi(argv[3]);

    if (client_id == -1) {
        printf("invalid client_id\n");
    }
    if (rc == -1) {
        printf("invalid ip\n");
    }
    if (port == -1) {
        printf("invalid port\n");
    }
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    if (sockfd < 0) {
        printf("socket() error\n");
        return 1;
    }
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
        printf("setsockopt sockfd failed\n");
        return 1;
    }

    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(ip);
    
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rc < 0) {
        printf("connect failed\n");
        return 1;
    }

    send(sockfd, (void*)&client_id, sizeof(int), 0);
    run_client(sockfd);
    close(sockfd);
}
