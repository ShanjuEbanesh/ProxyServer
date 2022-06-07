/* A proxy Server to access data from servers using http protocol can handle only one client at a time.*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#define MAXLEN 4096			//maximum length of the input buffer

int parse(char * client_req, char ** url, char ** port_number){		//parsing function
	
	int itr = 0;
	char * host = "Host: ";
	
	while(client_req[itr++] == ' ');	//removing initial spaces if any

	while(client_req[itr++] != ' ');	//skipping the method part
	
	while(client_req[itr++] == ' ');	//skipping spaces after method part
	
	itr--;
	int start = itr;
	while(client_req[itr+1] != 0){
		if((client_req[itr] == '/' && client_req[itr + 1] != '/' && client_req[itr - 1] != '/') || client_req[itr] == ' '){		//skipping http:// or https://
			break;
		}
		itr++;
	}

	while(client_req[itr] != 0){			//overrighting the http:// or https:// part in the http header
		client_req[start++] = client_req[itr++];
	}
	client_req[start] = '\0';

	itr = -1;
	while(client_req[++itr] != 0){		//finding the Host part in the http header
		if(client_req[itr] == '\n'){
			itr++;
			int flag = 0;
			while(client_req[itr++] == host[flag++]);		
			if(flag == 7){
				break;
			}
		}
	}
	
	int url_len = 0;
	start = itr - 1;
	while(client_req[itr] != '\n' && client_req[itr] != ' '){		//finding the url length for dynamic allocation
		url_len++;
		itr++;
	} 

	printf("Size of the url is : %d\n", url_len);
	*url = (char*)malloc((url_len) * sizeof(char));				//allocating memory for the url
	itr = 0;
	while(client_req[start + itr] != '\n' && client_req[start + itr] != ':' && itr < url_len){		//copying the url from the header into the url variable
		(*url)[itr] = client_req[start + itr];
		itr++;
	}
	(*url)[itr] = '\0';
	if(client_req[start + itr] == ':')			//if there is a port number copy it to the port_number variable
	{
		itr++;
		int i = 0;
		while(client_req[start + itr] != '\n' && client_req[start + itr] != ' ' && i < 6){
			(*port_number)[i] = client_req[start + itr];
			i++;itr++;
		}
		i--;
		(*port_number)[i] = '\0';
	}
	
	return 0;

}

int print_error(int error_code){				//printing the errors

	switch(error_code){
	case 1:
		fprintf(stderr, "Error at getaddrinfo\n");
		break;
	case 2:
		fprintf(stderr, "Error at socket\n");
		break;
	case 3:
		fprintf(stderr, "Error ar setsockopt: Port already being used\n");
 		break;
 	case 4:
 		fprintf(stderr, "Error at bind\n");
		break;
	case 5:
		fprintf(stderr, "Error at listen\n");
		break;
	case 6:
		fprintf(stderr, "Error at accept\n");
		break;
	default:
		fprintf(stderr, "No such error code found\n");
	}
}

int read_input(int source_desc, char ** client_inp){	//getting inputs from the source_desc and store in the client_inp variavle

	char buffer[MAXLEN];
	int inp_length = 1;
	int rec_length;
	*client_inp = (char *)malloc(sizeof(char) * MAXLEN);	//allocating memory to the client_inp variable
	memset(*client_inp, 0, sizeof(*client_inp));

	while(1){									//loop for handling the overflow of data from the recv buffer
		memset(buffer, 0, sizeof(buffer));
		if((rec_length = recv(source_desc, buffer, MAXLEN, 0)) < MAXLEN){
			if(rec_length > 0){
				inp_length += rec_length + 1;
				*client_inp = (char *)realloc(*client_inp, inp_length * sizeof(char));
				strcat(*client_inp, buffer);
			}
			return 0;
		}
		else{
			inp_length += rec_length + 1;
			*client_inp = (char *)realloc(*client_inp, inp_length * sizeof(char));
			strcat(*client_inp, buffer);
		}
	}
}

int isnum(char val){
	if(val >= '0' && val <= '9')	return 1;
	return 0;
}

int check_url(char *url){			//checks if the url is an IP address or URL
	
	if(isnum(url[0])){
		return 0;
	}
	else
		return 1;
}

unsigned short port_to_num(char * port){		//converts the port number from string to short integer

	unsigned short ans = 0;
	int itr = 0;
	while(port[itr] != '\0'){
		ans = ans*10 + (port[itr]-'0');
		itr++;
	}
	return ans;
}

//handle_request function creates a connection with the req_url and access the req_page from the server and sends the data to the client
int handle_request(int *client_desc, char * client_req){	
	
	struct addrinfo *destaddr;
	int server_desc;
	
	char *port_number = (char*)malloc(6 * sizeof(char));		//initializing port number to 80(as default)
	port_number[0] = '8';
	port_number[1] = '0';
	port_number[2] = '\0';

	printf("The input from the client is: \n%s\n", client_req);
	char *url;
	parse(client_req, &url, &port_number);			//calling the parsing function to seperate the url and port number

	printf("The HTTP header after parsing is: \n%s\n", client_req);
	printf("The requested url from the client is: %s\n", url);
	printf("The port number being used is: %s\n", port_number);

	if(check_url(url)){				//checking if the URL is a URL or an IP address
		int status;
		if((status = getaddrinfo(url, port_number, NULL, &destaddr)) != 0)		//getting address info of the given url
		{	
			fprintf(stderr, "Error at getaddrinfo: %s\n", gai_strerror(status));
			exit(0);
		}
	
		server_desc = socket(destaddr->ai_family, destaddr->ai_socktype, destaddr->ai_protocol);;
		
		
		connect(server_desc, destaddr->ai_addr, destaddr->ai_addrlen);
	}
	else{						//if the URL is an IP address
		struct sockaddr_in my_addr;

		server_desc = socket(AF_INET, SOCK_STREAM, 0);

		my_addr.sin_family = AF_INET;			//Ipv4 is used
		int port = port_to_num(port_number);	//string to short integer 
		my_addr.sin_port = htons(port);     	//short integer to network byte order
		my_addr.sin_addr.s_addr = inet_addr(url);	//storing the ip address
		memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
		connect(server_desc, (struct sockaddr *)(&my_addr), sizeof(my_addr));	//connecting to the server
	}

	printf("Connected to the server\n");
	if(send(server_desc, client_req, strlen(client_req), 0) < strlen(client_req))		//sending the formatted HTTP header
		printf("sending unsuccessful\n");

	printf("The data received from the server: \n");
	
	int rec_length;
	char *buffer = (char *)malloc(MAXLEN * sizeof(char));			//buffer to store the data being received from the server
	int count = 0;
	while(1){
		if((rec_length = recv(server_desc, buffer, MAXLEN, 0)) < MAXLEN){
			if(rec_length > 0){
				send(*client_desc, buffer, rec_length, 0);
				printf("%d: %s\n", ++count, buffer);
			}
			close(server_desc);
			close(*client_desc);
			printf("HTTP Request processed and client socket closed\n\n\n");
			return 0;
		}
		else{
			send(*client_desc, buffer, rec_length, 0);
			printf("%d: %s\n", ++count, buffer);
		}
	}
	
	free(buffer);				//freeing all the allocated memory
	freeaddrinfo(destaddr);
	free(port_number);

	return 0;	
	
}

int create_proxysock(int *proxy_desc){			//function to create a socket for the local host itself
	struct addrinfo hostaddr, *resaddr;
	int status;
	

	memset(&hostaddr, 0, sizeof(hostaddr));
	hostaddr.ai_family = AF_UNSPEC;
	hostaddr.ai_socktype = SOCK_STREAM;
	hostaddr.ai_flags = AI_PASSIVE;
	
	if((status = getaddrinfo(NULL, "10003", &hostaddr, &resaddr)) != 0)	{		//getting the Ip address and other details of the local host
		return 1;
	}	
	
	if((*proxy_desc = socket(resaddr -> ai_family, resaddr -> ai_socktype, resaddr -> ai_protocol)) < 0){	//creating a socket with the IP of the local host of the system and the port number 10003
		return 2;
	}
	
	int yes=1;
	if (setsockopt(*proxy_desc ,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) == -1) {
 		return 3;
	}
	
	if(bind(*proxy_desc, resaddr -> ai_addr, resaddr -> ai_addrlen) == -1){//binding the port with the ip address
		return 4;
	}

	freeaddrinfo(resaddr);

	return 0;
}


int main(){

	int host_desc;
	socklen_t addr_size;
	int client_desc;
	struct sockaddr_storage recvaddr;
	
	create_proxysock(&host_desc);		//creating a socket for the local host and binding its IP with the same 
	
	
	if(listen(host_desc, 1024) == -1){	//listening for incoming clients
		fprintf(stderr, "Error at listen\n");
		return 1;
	}
	int flag = 0;
	
	while(1){
	
		addr_size = sizeof(recvaddr);
   		if((client_desc = accept(host_desc, (struct sockaddr *)&recvaddr, &addr_size)) == -1){	//accepting client connections
   			fprintf(stderr, "Error at accept\n");
   			return 5;
   		}
    	
   		char * client_req;
   		
   		read_input(client_desc, &client_req);	//receiving the HTTP request from the client  		

		handle_request(&client_desc, client_req);	//handling the input received from the client

		free(client_req);			//free the memory after handling the input
	}
	
	close(host_desc);			//closing the host after handling the requests from clients		
	return 0;

}
