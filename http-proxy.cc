/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include "http-request.h"
#include "http-headers.h"

using namespace std;

const char* LISTENING_PORT = "4649";
const char* WELCOME_MSG = "This is our proxy server. Yoroshiku.\n";
int sockfd, new_fd;

void signal_handler(int sig)
{
	cout << "SIGINT received. Closing sockets." << endl;
	close(sockfd);
	close(new_fd);
	exit(0);
}

int main (int argc, char *argv[])
{
	signal(SIGINT, signal_handler);
	// command line parsing

	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	struct addrinfo hints, *res;
	// int sockfd, new_fd;

	// first, load up address structs with getaddrinfo():

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	getaddrinfo(NULL, LISTENING_PORT, &hints, &res);

	// make a socket, bind it, and listen on it:

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	bind(sockfd, res->ai_addr, res->ai_addrlen);
	listen(sockfd, 10);
	cout << "We are now listening on port " << LISTENING_PORT << ". We await connections." << endl;

	// now accept an incoming connection:

	addr_size = sizeof their_addr;

	char incoming[512];
	int bytesSent = 1;
	bool terminate = 0;

	HttpRequest req;

	while(!terminate)
	{
		cout << "Accepting..." << endl;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
		// ready to communicate on socket descriptor new_fd!
		if(new_fd != -1)
		{
			cout << "Received connection." << endl;
			send(new_fd, WELCOME_MSG, strlen(WELCOME_MSG), 0);

			while(bytesSent != 0)
			{
				memset(incoming, 0, sizeof incoming);
				bytesSent = recv(new_fd, incoming, sizeof incoming, 0);
				cout << "Received: " << incoming << endl;
				cout << "Number of bytes: " << bytesSent << endl;

				try
				{
					req.ParseRequest(incoming, bytesSent);
					cout << "\tHost: " << req.GetHost() << endl;
					cout << "\tHeader \"Header\": " << req.FindHeader("Header") << endl;
				}
				catch (ParseException& e)
				{
					cout << "Exception caught: " << e.what() << endl;
				}

				if(bytesSent == 2)
				{
					cout << "Empty line received. Closing connection." << endl;
					close(new_fd);
					terminate = 1;
					break;
				}
			}
		}
	}
	
	close(sockfd);
 	return 0;
}
