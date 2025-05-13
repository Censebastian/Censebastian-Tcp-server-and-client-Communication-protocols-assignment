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

void recv_udp(int udp_sockfd, List *clients) {
    char buf[UDP_PACKAGE_SIZE];
    struct sockaddr_in udp_client;
    socklen_t addr_len = sizeof(udp_client);
    int udp_recv = recvfrom(udp_sockfd, buf, UDP_PACKAGE_SIZE, 0,
                            (struct sockaddr *)&udp_client, &addr_len);
    send_topic(clients, buf, udp_recv, udp_client);
}

void add_new_client(List *clients, List *pfds, int tcp_sockfd) {
    struct sockaddr_in client_addr;
    socklen_t addrsize = sizeof(struct sockaddr);

    int new_sockfd = accept(tcp_sockfd, (struct sockaddr*)&client_addr, &addrsize);
    if (new_sockfd < 0)
        printf("accept failed\n");

    int enable = 1;
    if (setsockopt(new_sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
        printf("setsockopt new_sockfd failed\n");
        return;
    }

    int new_id = -1;
    recv(new_sockfd, (void*)&new_id, sizeof(int), 0); // possible problem

    if (add_client(clients, pfds, new_sockfd, new_id)) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        unsigned short client_port;
        printf("New client C%d connected from %s:%d.\n",
               new_id, client_ip, client_port);
        return;
    }
    printf("Client C%d already connected.\n", new_id);
    close(new_sockfd);
}

void receive_on_port(int tcp_sockfd, int udp_sockfd) {
    List *pfds = init_pfds(tcp_sockfd, udp_sockfd);
    List *clients = init_list(sizeof(Client));
    struct pollfd *poll_fds= (struct pollfd*)pfds->data;
    
    if (listen(tcp_sockfd, MAX_CLIENTS / 2) < 0)
        printf("listen failed\n");

    while (1) {
        char buf[1024];
        if (poll(poll_fds, pfds->no, -1) < 0)
            printf("poll failed\n");

        for (int i = 0; i < pfds->no; i++) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == STDIN) {
                    if (fgets(buf, sizeof(buf), stdin) == NULL)
                        printf("fgets eof error");
                    buf[strlen(buf) - 1] = '\0';
                    if (!strcmp(buf, "exit"))
                        return;
                } else if (poll_fds[i].fd == tcp_sockfd) { // new tcp client
                    add_new_client(clients, pfds, tcp_sockfd);
                } else if (poll_fds[i].fd == udp_sockfd) {
                    recv_udp(udp_sockfd, clients);
                } else {
                    int recv_ret = recv(poll_fds[i].fd, buf, 1024, 0);
                    if (starts_with(buf, "subscribe ") && strlen(buf + strlen("subscribe ")) != 0) {
                        subscribe(get_client_bysock(clients, poll_fds[i].fd), buf + strlen("subscribe "));
                    } else if (starts_with(buf, "unsubscribe ") && strlen(buf + strlen("unsubscribe ")) != 0) {
                        unsubscribe(get_client_bysock(clients, poll_fds[i].fd), buf + strlen("unsubscribe "));
                    }
                    if (!recv_ret) {
                        disc_client(clients, poll_fds[i].fd);
                        close(poll_fds[i].fd);
                        remove_elem(pfds, poll_fds + i, 3);
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    if (argc != 2) {
        printf("provide a port\n");
        return -1; }

    uint16_t port;
    if (sscanf(argv[1], "%hu", &port) != 1) {
        printf("invalid port\n");
        return -1;
    }
    
    int tcp_sockfd;
    int udp_sockfd;
    struct sockaddr_in servaddr;
    
    if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("couldn't create udp socket\n");
        return -1;
    }
    if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("couldn't create udp socket\n");
        return -1;
    }
    
    
    int enable = 1;
    if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        printf("setsockopt tcp_sockfd failed\n");
        return -1;
    }
    if (setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        printf("setsockopt udp_sockfd failed\n");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);
   
    if (bind(tcp_sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        printf("tcp bind failed \n");
    if (bind(udp_sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        printf("udp bind failed \n");

    receive_on_port(tcp_sockfd, udp_sockfd);
}
