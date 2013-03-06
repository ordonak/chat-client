//This program acts as a client for a simple chat program
#define name client
#include <string.h>
#include <iostream>
#include <fstream>
#include <sys/types.h> // size_t, ssize_t 
#include <sys/socket.h> // socket funcs 
#include <netinet/in.h> // sockaddr_in 
#include <arpa/inet.h> // htons, inet_pton 
#include <unistd.h>

using namespace std;

//function declerations

int createTCPSocket();
int bindPort(short portNumber, int sockNumber);

int acceptConnection(int sockNumber);
string recieveMessage(int clientSock, long nameSize);
void sendLong(long guess, int sock);
void sendMessage(string userNameStr, int sock);
char *IPAddr = "10.124.72.20";
unsigned short PORT_NUMBER = 10105;
int LEADERBOARD_SLOTS = 3;
long recieveNum(int clientSock);
string userName = "";
int main(int argc, char *argv[])
{
	
	if(argc!=3){
		cerr<<"Usage ./client IP_ADDRESS PORT_NUMBER"<<endl;
	}else
	{
		IPAddr = argv[1]; 
		PORT_NUMBER = strtoul(argv[2],NULL,0); 
		int sock = createTCPSocket();

	//Establish Connection
	// Convert dotted decimal address to int unsigned long servIP;
		unsigned long servIP;
		int status = inet_pton(AF_INET, IPAddr, &servIP); 
		if (status <= 0) exit(-1);
		
	// Set the fields
		struct sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET; // always AF_INET 
	servAddr.sin_addr.s_addr = servIP; 
	servAddr.sin_port = htons(PORT_NUMBER);

	//connect
	status = connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr));
	if (status < 0) 
	{
		cerr << "Error with connect" << endl;
		exit (-1);
	}



//send name size and name 


	string userNameStr;
	cerr << "Please enter username: ";
	cin >> userNameStr;
	cerr << endl;
	cerr << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
	cerr<<"It worked!!!!!!!!!"<<endl<<"YAY!"<<endl;

	sendMessage(userNameStr, sock);

	bool inChat = true;

	while(inChat)
	{
		/**
		User types in what they want to do
		R = Refresh Page
		M = Send new Message
		Q = Quit
		**/




	} 



//Close connection
	close(sock);
}
}



	//sends the size and then a string provided of that size
void sendMessage(string message, int sock){


	//send name size
	sendLong(message.size()+1,sock);

	//send name
	char userName[userName.size()+1];
	strcpy(userName, message.c_str());
	int bytesSent = send(sock, (void *)&userName, message.size()+1, 0);
	if (bytesSent != message.size()+1) {
		cerr<<"Send fail";
		exit(-1);}
	}

	// sends a long integer
	void sendLong(long guess, int sock)
	{

		long number = guess;

		number = htonl(number);

		int bytesSent = send(sock, (void *) &number, sizeof(long), 0);
		if (bytesSent != sizeof(long)) 
		{
			cerr<<"Send fail";
			exit(-1);
		}


	}
		//create Socket
	int createTCPSocket()
	{
	//Grab Socket Discripter
		int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock < 0) {
			cerr << "Error with socket" << endl;
			exit (-1);
		}
		return sock;
	}

	int bindPort(short portNumber , int sockNumber)
	{
	unsigned short servPort = portNumber; //port range 10100-10199
	struct sockaddr_in servAddr;
	// Set the fields
	// INADDR_ANY is a wildcard for any IP address 
	servAddr.sin_family = AF_INET; // always AF_INET 
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(servPort);
	//bind port
	int status = bind(sockNumber, (struct sockaddr *) &servAddr, sizeof(servAddr));
	if (status < 0) {cerr<<"Port Taken";	
	exit (-1); }
}

		//Accept connection with given sock number
int acceptConnection(int sockNumber){
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	int clientSock = accept(sockNumber,(struct sockaddr *)
		&clientAddr, &addrLen);
	if (clientSock < 0) 
	{
		exit(-1);

	}
	return clientSock;
}

	//Recieve a string of the given size with the given sock
string recieveMessage(int clientSock, long size)
{

	int bytesLeft =size;
	char buffer[size];
	char *bp = buffer;
	while(bytesLeft>0){

		int bytesRecv = recv(clientSock,(void*) bp,bytesLeft,0);
		if(bytesRecv <=0) 
		{
			exit(-1);
		}
		bytesLeft = bytesLeft - bytesRecv;
		bp = bp +bytesRecv;
	}
//cout<<buffer<<endl;
	string name = string(buffer);
	return name;
}
//Recieve a long integer
long recieveNum(int clientSock){

	int bytesLeft = sizeof(long);
	long numberRecieved ;
	char *bp = (char*) &numberRecieved;
	while(bytesLeft>0){

		int bytesRecv = recv(clientSock,(void*) bp,bytesLeft,0);
		
		if(bytesRecv <=0) 
		{

			exit(-1);
		}
		bytesLeft = bytesLeft - bytesRecv;
		bp = bp +bytesRecv;
	}


	numberRecieved = ntohl(numberRecieved);

	return numberRecieved;
}

