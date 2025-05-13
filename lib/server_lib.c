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

#include "server_lib.h"
#include "otcpp.h"

List *init_list(size_t data_size) {
    List *list = malloc(sizeof(List));
    list->cap = MAX_CLIENTS;
    list->no = 0;
    list->data_size = data_size;
    list->data = malloc(list->data_size * list->cap);
    return list;
}

List *init_pfds(int tcp_sockfd, int udp_sockfd) {
    List *list = init_list(sizeof(struct pollfd));
    list->no = 3;

    ((struct pollfd*)list->data)[0].fd = STDIN;
    ((struct pollfd*)list->data)[0].events = POLLIN;
    ((struct pollfd*)list->data)[1].fd = tcp_sockfd;
    ((struct pollfd*)list->data)[1].events = POLLIN;
    ((struct pollfd*)list->data)[2].fd = udp_sockfd;
    ((struct pollfd*)list->data)[2].events = POLLIN;

    return list;
}

void add_to_list(List *list, void *new_elem) {
    if (list->cap == list->no) {
        list->cap *= 2;
        list->data = realloc(list->data, list->data_size * list->cap);
    }
    memcpy(list->data + list->no++ * list->data_size, new_elem, list->data_size);
}

int add_client(List *clients, List *pfds, int sockfd, int new_id) {
    for (int i = 0; i < clients->no; i++)
        if (((Client*)clients->data)[i].id == new_id) {
            if (((Client*)clients->data)[i].connected)
                return 0;
            ((Client*)clients->data)[i].connected = 1;
            ((Client*)clients->data)[i].sockfd = sockfd;
            struct pollfd poll_fd;
            poll_fd.fd = sockfd;
            poll_fd.events = POLLIN;
            add_to_list(pfds, &poll_fd);
            return 1;
        }
    Client new_client;
    memset(&new_client, 0, sizeof(Client));
    new_client.id = new_id;
    new_client.sockfd = sockfd;
    new_client.connected = 1;
    new_client.topics = init_list(sizeof(Topic));
    add_to_list(clients, &new_client);

    struct pollfd poll_fd;
    memset(&poll_fd, 0, sizeof(struct pollfd));
    poll_fd.fd = sockfd;
    poll_fd.events = POLLIN;
    add_to_list(pfds, &poll_fd);
    return 1;
}

Client *get_client_bysock(List *clients, int sockfd) {
    for (int i = 0; i < clients->no; i++) {
        if (((Client*)clients->data)[i].sockfd == sockfd)
            return (Client*)(clients->data + i * clients->data_size);
    }
}

void disc_client(List *clients, int sockfd) {
    for (int i = 0; i < clients->no; i++)
        if (((Client*)clients->data)[i].sockfd == sockfd) {
            ((Client*)clients->data)[i].connected = 0;
            ((Client*)clients->data)[i].sockfd = -1;
            printf("Client C%d disconnected.\n", ((Client*)clients->data)[i].id);
        }
}

void remove_elem(List *list, void *elem, int start_idx) {
    for (int i = start_idx; i < list->no; i++)
        if (!memcmp((char*)list->data + i * list->data_size, elem, list->data_size)) {
            list->no--;
            memcpy((char*)list->data + i * list->data_size,
                   (char*)list->data + list->no * list->data_size, list->data_size);
            return;
        }
}

int starts_with(char *input, char *start) {
    while (*start) {
        if (*input++ != *start++) return 0;
    }
    return 1;
}

void subscribe(Client *client, char *topic) {
    for (int i = 0; i < client->topics->no; i++) {
        if (!strcmp(((Topic*)client->topics->data)[i].topic, topic))
            return;
    }
    Topic new_topic = create_topic(topic);
    add_to_list(client->topics, &new_topic);
}

void unsubscribe(Client *client, char *topic) {
    for (int i = 0; i < client->topics->no; i++) {
        if (!strcmp(((Topic*)client->topics->data)[i].topic, topic))
            remove_elem(client->topics, (char*)client->topics->data + i, 0);
    }
}

void print_topics(List *topics) {
    for (int i = 0; i < topics->no; i++)
        printf("topic: %s\n", ((Topic*)topics->data)[i].topic);
}

void print_clients(List *clients) {
    for (int i = 0; i < clients->no; i++) {
        printf("cid: %d\n", ((Client*)clients->data)[i].id);
    }
}

int contains(List *list, void *elem) {
    for (int i = 0; i < list->no; i++) {
        if (!memcmp((char*)list->data + i * list->data_size, elem, list->data_size))
            return 1;
    }
    return 0;
}

Topic create_topic(char *new_topic_str) {
    Topic new_topic;
    memset(&new_topic, 0, sizeof(Topic));
    strcpy(new_topic.topic, new_topic_str);
    new_topic.wildcard = (strchr(new_topic_str, '*') == NULL && strchr(new_topic_str, '+') == NULL) ? 0 : 1;
    new_topic.data = NULL;
    return new_topic;
}

int is_subscribed(Client *client, char *topic) {
    for (int i = 0; i < client->topics->no; i++)
        if (!strcmp(((Topic*)client->topics->data)[i].topic, topic))
            return 1;
    return 0;
}

void send_topic(List *clients, char *buf, int bytes_recv, struct sockaddr_in udp_client) {
    char topic[51];
    topic[50] = '\0';
    strncpy(topic, buf, 50);

    char *frame = frame_msg(buf, bytes_recv, udp_client);
    
    for (int i = 0; i < clients->no; i++)
        if (is_subscribed(((Client*)clients->data) + i, topic)) { // wildcard goes here 
            int sent = send_all(((Client*)clients->data)[i].sockfd, frame, EMPT_LEN + bytes_recv + sizeof(struct sockaddr_in));
         }
    free(frame);
}
