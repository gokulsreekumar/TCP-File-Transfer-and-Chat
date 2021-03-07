#include <stdio.h>
#include <string.h>
#include <unistd.h> // socket close() defn
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h> // for inet_ntoa

#include <sys/time.h>

#define PORT 8888	   // PORT to connect to
#define LO "127.0.0.1" // LOOPBACK ADDRESS FOR CONNECTING TO LOCAL HOST
#define MESSAGE_SIZE 500
#define MB_IN_BYTES 1048576

double value (struct timeval a) {
    double val = a.tv_sec + a.tv_usec*1e-6;
    return val;
}

void fileTransfer(int sockfd, struct timeval begin)
{
	double n;

	FILE *fp;
	char *filename = "file_RCVD.txt";
	char buffer[MESSAGE_SIZE];
	fp = fopen(filename, "w");

    FILE *fp2;
    char *filename2 = "rates.txt";
	fp2 = fopen(filename2, "w");
   	
	// Transmission Data
	double time_taken, data_received, transmission_rate, tick = 0.01;
	struct timeval curr, prev;
	double curr_val;

	//initialize prev;
    prev.tv_sec = 0;
    prev.tv_usec = 0;

	while (1)
	{
		n = recv(sockfd, buffer, MESSAGE_SIZE, 0);

		gettimeofday(&curr, NULL);
		curr.tv_sec = curr.tv_sec - begin.tv_sec;
		curr.tv_usec = curr.tv_usec - begin.tv_usec;

		// Elapsed Time
		long seconds = curr.tv_sec - prev.tv_sec;
        long microseconds = curr.tv_usec - prev.tv_usec;
        time_taken = seconds + microseconds*1e-6;

		transmission_rate = (n/MB_IN_BYTES) / (time_taken);
        curr_val = value(curr);

		// Updating Transmission rate
		while (tick < curr_val) {
            fprintf(fp2, "%f %f\n", tick, transmission_rate);
            printf("%f %f\n", tick, transmission_rate);
            tick += 0.01; 
        }

		if (n < 0)
		{
			perror("read error");
			close(sockfd);
			exit(1);
		}
		else if (n == 0)
		{
			printf("server closed connection\n");
			close(sockfd);
			exit(1);
		}

		if ((strcmp(buffer, "EOF")) == 0)
		{
			break;
		}

		fprintf(fp, "%s", buffer);

   		prev.tv_sec = curr.tv_sec;
        prev.tv_usec = curr.tv_usec;
		
		bzero(buffer, MESSAGE_SIZE);
	}

}

void communication(int sockfd)
{
	int n;
	char buffer[1024] = {0};
	while (1)
	{
		bzero(buffer, 1024);
		printf("Client's Message: ");
		fgets(buffer, 1024, stdin);
		if (strncmp(buffer, "GivemeyourVideo", 15) == 0)
		{
			n = send(sockfd, buffer, 1024, 0);
			struct timeval begin;
    		gettimeofday(&begin, NULL);
			printf("file transfer...\n");
			fileTransfer(sockfd, begin);
			printf("file transfer finished!\n");
		}
		else
		{
			// Send the Message to Server
			n = send(sockfd, buffer, 1024, 0);
			if (n < 0)
			{
				printf("Reading Error!\n");
			}

			if (strncmp(buffer, "Bye", 3) == 0)
			{
				printf("Client closed connection\n");
				close(sockfd);
				break;
			}
			bzero(buffer, 1024);
			// Print Message from Server
			n = recv(sockfd, buffer, 1024, 0);
			printf("Server: %s\n", buffer);
			bzero(buffer, 1024);
		}
	}
}

int main(int argc, char const *argv[])
{
	int sockfd = 0, n;
	struct sockaddr_in serv_addr;
	char buffer[1024] = {0};

	// Create Socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	// Make Address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, LO, &serv_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	// Connect()
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed \n");
		return -1;
	}

	// Communication
	communication(sockfd);
	return 0;
}
