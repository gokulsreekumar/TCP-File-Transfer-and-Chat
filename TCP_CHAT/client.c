#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // socket close() defn

#include <sys/socket.h>
#include <arpa/inet.h> // for inet_ntoa
#include <netinet/in.h>

#include <signal.h>
#include <errno.h>

#define BUFSIZE 1024
#define LO "127.0.0.1"
#define PORT 8080

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

char username[20], sender_name[20];

void trim_newline(char *arr, int length);
void connect_request(int *sockfd, struct sockaddr_in *server_addr);

void send_recv(int i, int sockfd, char name[32], fd_set *master);

void login(int sockfd) {

    memset(username,'\0',sizeof(username));

    printf("Please enter the username: ");
    scanf("%[^\n]",username);
    getchar();

    printf("\n-------------- Welcome to Chatroom -------------\n");

    // Send the username to server
    struct message send_message;

	send_message.type = 1;
	strcpy(send_message.name, username);

    send(sockfd, &send_message, sizeof(send_message), 0);
}

int main()
{
    int sockfd, i;
    struct sockaddr_in server_addr;

    char new_entry_message[45];

    connect_request(&sockfd, &server_addr);
    login(sockfd);


    fd_set master;
    fd_set read_fds;
    
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(0, &master); // STDIN
    FD_SET(sockfd, &master);

    int fdmax = sockfd;

    while (1) {

        read_fds = master;

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select failed");
            exit(1);
        }

        for (i = 0; i <= fdmax; i++)
            if (FD_ISSET(i, &read_fds))
                send_recv(i, sockfd, username, &master);

    }

    return 0;
}

void connect_request(int *sockfd, struct sockaddr_in *server_addr)
{
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket");
        exit(1);
    }

    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(PORT);
    server_addr->sin_addr.s_addr = inet_addr(LO);

    if (connect(*sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }
}

void send_recv(int i, int sockfd, char name[32], fd_set *master)
{

    struct message send_message, recv_message;
    int recv_size;

    if (i == 0)
    {
        fgets(send_message.data, BUFSIZE, stdin);

        if (strncmp(send_message.data, "tata", 4) == 0)
        {
            send_message.type = 2;
            strcpy(send_message.name, username);
            
            if( send(sockfd, &send_message, sizeof(send_message), 0) == -1)
				perror("Sending failed");

            FD_CLR(i, master);

            sprintf(send_message.data, "You have left the chat\n");

            close(sockfd);
            exit(0);
        }
        else {

            send_message.type = 0;
			strcpy(send_message.name, username);

            if(send(sockfd, &send_message, sizeof(send_message), 0) == -1)
			    perror("Sending failed");

        }
            
    } else {

        if(recv_size = recv(sockfd, &recv_message, sizeof(recv_message), 0) == 0) {
			perror("Connection closed");
			exit(0);
		}

        if (recv_message.type == 3) {

            printf("%s\n" , recv_message.data);

		}
		else if (recv_message.type == 2) {

            printf("%s left the room\n" , recv_message.name);

		}
        else if(recv_message.type == 1) {

            printf("%s has joined the chat\n" , recv_message.name);

        }
        else {

            printf("%s > %s" , recv_message.name, recv_message.data);

        }
		
		fflush(stdout);
    }
}
