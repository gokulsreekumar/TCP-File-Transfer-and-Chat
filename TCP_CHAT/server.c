#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 8080
#define BUFSIZE 1024

#define MAX_NUMBER_CLIENTS 1024
char name_directory[MAX_NUMBER_CLIENTS][32];

// Message Structure
// 0 - Message
// 1 - name
// 2 - end
// 3 - namelist
struct message
{
    int type;
    char name[32];
    char data[BUFSIZE];
};

void startServer(int *sockfd, struct sockaddr_in *server_addr);
void acceptConnection(fd_set *master, int *fdmax, int sockfd, struct sockaddr_in *client_addr);
void send_recv(int i, fd_set *master, int sockfd, int fdmax);

int main()
{
    fd_set master, read_fds;

    int fdmax, i;
    int sockfd = 0;

    struct sockaddr_in server_addr, client_addr;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    startServer(&sockfd, &server_addr);

    FD_SET(sockfd, &master);
    fdmax = sockfd;

    memset(name_directory, '\0', sizeof(name_directory));

    while (1)
    {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }
        for (i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == sockfd)
                    acceptConnection(&master, &fdmax, sockfd, &client_addr);
                else
                    send_recv(i, &master, sockfd, fdmax);
            }
        }
    }
    return 0;
}

void startServer(int *sockfd, struct sockaddr_in *server_addr)
{
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket");
        exit(1);
    }

    // Make Address
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(PORT);
    server_addr->sin_addr.s_addr = INADDR_ANY;

    // Options
    int flag = 1;
	if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
		perror("error in setsockopt");
		exit(1);
	}

    // Bind
    if (bind(*sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Unable to bind");
        exit(1);
    }
    printf("TCP Server Bound to Port: %d\n", PORT);

    // Listen
    if (listen(*sockfd, 10) == -1)
    {
        perror("listen");
        exit(1);
    }
    printf("TCP Server Waiting for Client Requests...\n");
    fflush(stdout);
}

void acceptConnection(fd_set *master, int *fdmax, int sockfd, struct sockaddr_in *client_addr)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int newsockfd;

    if ((newsockfd = accept(sockfd, (struct sockaddr *)client_addr, &addrlen)) == -1)
    {
        perror("accept");
        exit(1);
    }
    else
    {
        FD_SET(newsockfd, master);
        if (newsockfd > *fdmax)
            *fdmax = newsockfd;
        printf("New connection at %s : %d \n", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    }
}


void broadcast(int i, int sockfd, int size, struct message recv_message, fd_set *master, int fdmax)
{
    for(int j = 0; j <= fdmax; j++)
        if (FD_ISSET(j, master)){
            if (j != sockfd && j != i) {
                if (send(j, &recv_message, size, 0) == -1) {
                    perror("Sending failed");
                }
            }
        }
}


void send_recv(int i, fd_set *master, int sockfd, int fdmax)
{
    int recv_size, j;
    struct message recv_message, send_message;
    char recv_buf[BUFSIZE];

    if ((recv_size = recv(i, &recv_message, sizeof(struct message), 0)) <= 0) {

        // Remove the Client
        memset(name_directory[i], '\0', sizeof(name_directory[i]));

        close(i);
        FD_CLR(i, master);
    }
    else
    {
        broadcast(i, sockfd, recv_size, recv_message, master, fdmax);

        if (recv_message.type == 2)
        {

            printf("%s left the chat\n", recv_message.name);

        }
        else if (recv_message.type == 1)
        {

            strcpy(name_directory[i], recv_message.name);

            printf("%s has joined the chat\n" , recv_message.name);

            // sending namelist
            send_message.type = 3;
			for(j = 0; j <= fdmax; j++) {

				if (j != sockfd && j != i && name_directory[j][0] != '\0') {

					sprintf(send_message.data, "%s is online", name_directory[j]);

					if (send(i, &send_message, sizeof(send_message), 0) == -1) {
						perror("Sending failed");
					}

				}				
			}

        }
        else if (recv_message.type == 0)
        {
            printf("%s > %s", recv_message.name, recv_message.data);
        }
        bzero(recv_buf, BUFSIZE);
    }
}
