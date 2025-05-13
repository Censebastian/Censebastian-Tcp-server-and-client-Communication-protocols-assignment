#ifndef SERVER_LIB_H
#define SERVER_LIB_H

#include <stdlib.h>
#include <poll.h>

#define STDIN 0
#define MAX_CLIENTS 32
#define UDP_PACKAGE_SIZE 1551

typedef struct {
    int cap;
    int no;
    void *data;
    size_t data_size;
} List;

typedef struct {
    int id;
    int sockfd;
    char connected;
    List *topics;
} Client;

typedef struct {
    char topic[51];
    char wildcard;
    char *data;
} Topic;

List *init_list(size_t data_size);
List *init_pfds(int tcp_sockfd, int udp_sockfd);

void add_to_list(List *list, void *new_obj);
int add_client(List *clients, List *pfds, int sockfd, int new_id);
Client *get_client_bysock(List *clients, int sockfd);
void disc_client(List *clients, int sockfd);
void remove_elem(List *list, void *elem, int start_idx);
int starts_with(char *input, char *start);
void subscribe(Client *client, char *topic);
void unsubscribe(Client *client, char *topic);
void print_topics(List *topics);
void print_clients(List *clients);
int contains(List *list, void *elem);
Topic create_topic(char *new_topic_str);
int is_subscribed(Client *client, char *topic);
void send_topic(List *clients, char *buf, int bytes_recv, struct sockaddr_in udp_client);

#endif //SERVER_LIB_H
