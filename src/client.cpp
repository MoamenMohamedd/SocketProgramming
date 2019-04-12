//============================================================================
// Name        : SocketClientFCB.cpp
// Author      : KizoFCB
// Version     : 1
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>


// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
//Choosing a specific port
#define PORT 8080

using namespace std;

int main(int argc, char const *argv[])
	{
		int sock = 0, valread;
		struct sockaddr_in serv_addr;

		//Initializing request message
		char *request = "GET img1.jpeg";

		//We will set our buffer to 4MB
		char buffer[4*1024*1024] = {0};

		//Initialize a socket using IPv4 and TCP
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
		{
			printf("\n Socket creation error \n");
			return -1;
		}

		memset(&serv_addr, '0', sizeof(serv_addr));

		//Using the server address with the predefined port
		serv_addr.sin_family = AF_INET; // @suppress("Field cannot be resolved")
		serv_addr.sin_port = htons(PORT); // @suppress("Field cannot be resolved")


		// Convert IPv4 and IPv6 addresses from text to binary form
		if(inet_pton(AF_INET, "192.168.1.63", &serv_addr.sin_addr)<=0) // @suppress("Function cannot be resolved") // @suppress("Field cannot be resolved")
		{
			printf("\nInvalid address/ Address not supported \n");
			return -1;
		}

		//Connecting the socket to the specified address
		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) // @suppress("Invalid arguments")
		{
			printf("\nConnection Failed \n");
			return -1;
		}

		//Sending the request with its length
		send(sock , request , strlen(request) , 0 );
		printf("Hello message sent\n");

		//Receiving the response from the server and printing it
		valread = read( sock , buffer, 4*1024*1024); // @suppress("Function cannot be resolved")
		printf("%s\n",buffer );


		return 0;
	}

