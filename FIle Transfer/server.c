#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>     // socket close() defn

// Socket Specific Imports

#include <sys/types.h>  // for type definitions used int socket.h and in.h
#include <sys/socket.h> // all network socket related functions and structures (eg. sockaddr)

#include <netinet/in.h> // for sockaddr_in
#include <arpa/inet.h>   // for inet_ntoa

#define PORT 8888 
#define MESSAGE_SIZE 500
#define BUFFER_SIZE 1024

int startServer(struct sockaddr_in server_address) {
	int server_fd;
	// Create TCP Socket
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	server_address.sin_family = AF_INET; 
	server_address.sin_addr.s_addr = INADDR_ANY; 
	server_address.sin_port = htons( PORT ); 

	int flag = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
		perror("error in setsockopt");
		exit(1);
	}
	
	// Binding socket to the server_addr
	if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address))<0) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	}

	// Listening at Port
	if(listen(server_fd, 3) < 0) 
	{ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	} 
	printf("Listening on Port: %d...\n", PORT);
	return server_fd;
}

void fileTransfer(int sockfd) {
    int n;

    FILE *fd;
    char *filename = "sample50MB.txt";
    char buffer[MESSAGE_SIZE];

    bzero(buffer, MESSAGE_SIZE);

    fd = fopen(filename, "r");
	int bytes_send;

	int packet_cnt = 0;
    while(fgets(buffer, MESSAGE_SIZE, fd) != NULL) {
		packet_cnt++;
		bytes_send = send(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_send == -1) {
            perror("Failed to send file. Please try again.");
            exit(1);
        }
        bzero(buffer, MESSAGE_SIZE);
    }
	printf("Packets Send: %d\n", packet_cnt);
	// EOF
    strcpy(buffer, "EOF");
    send(sockfd, buffer, sizeof(buffer), 0);
    
}
void communication(int sockfd) {
	int n;
	char buffer[BUFFER_SIZE];
	char default_message[] = "Message Acknowledged";
	while(1) {
		// Recieve the Message from Client
		bzero(buffer, BUFFER_SIZE);
		n = recv(sockfd , buffer, BUFFER_SIZE, 0); 
		if(n<0) {
			printf("Reading Error!\n");
		}
		printf("Client: %s", buffer);

		// Check if it is to send file or not
		if (strncmp(buffer, "GivemeyourVideo", 15) == 0)
		{
			printf("File transfer started...\n");
			fileTransfer(sockfd);
			printf("File transfer finished\n");
		}
		else
		{
			if(strncmp(buffer, "Bye", 3) == 0) {
				close(sockfd);
				break;
			}
			bzero(buffer, BUFFER_SIZE);
			send(sockfd, default_message, strlen(default_message), 0); 
		}
	}
}

int main(int argc, char const *argv[]) 
{ 
	int server_fd, sockfd, n; 
	struct sockaddr_in server_address, client_address; 
	char buffer[BUFFER_SIZE] = {0}; 
	
	server_fd = startServer(server_address);

	int addrlen = sizeof(client_address); 
	if ((sockfd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*)&addrlen))<0) 
	{ 
		perror("accept"); 
		exit(EXIT_FAILURE); 
	} 
	
	// Communicate
	communication(sockfd);

	// Close Server
	close(server_fd);
	printf("Server Connection Closed.\n");
	return 0; 
} 

