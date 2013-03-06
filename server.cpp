/**
This program acts as a server for a simple chat program

The program key aspects:
 -A map where the keys are equal to all the usernames logged in.  The values are the users current
 inbox.  A client makes a refresh request to the server and the server sends whats in their
 inbox and then clears there inbox
 		SEARCH,INSERT,DELETE = O(Log(n))
 		Space = O(n)
 	This was chosen because it will use far less size then a hash map which seemed overkill 
 	on this project because the amount of people logged in at once would have to be very
 	large to make log(n) not good enough
 -A vector maintaining the keys in for the map
 	This is used for delivering messages to each chat users inbox


**/
#include <string.h>
#include <iostream>
#include <fstream>
#include <sys/types.h> // size_t, ssize_t 
#include <sys/socket.h> // socket funcs 
#include <netinet/in.h> // sockaddr_in 
#include <arpa/inet.h> // htons, inet_pton 
#include <unistd.h>
#include <cmath>
#include <time.h>
#include <pthread.h>

const int MAXPENDING=5;
const int NUMBER_OF_NUMBERS = 3;
const int LEADERBOARD_SLOTS = 3;

using namespace std;
	//variables
	int PORT_NUMBER =10111;
	
	pthread_mutex_t lock;
	struct intWrap
	{
		int num;
	};
	struct leaderSlot
	{
		long rounds;  //202 if not filled
		string name;
	};
	leaderSlot leaderBoard[LEADERBOARD_SLOTS];
	//Function Declerations
	int createTCPSocket();
	int bindPort(short portNumber, int sockNumber);
	int setToListen(int sockNumber, int const MAXPENDING);
	int acceptConnection(int sockNumber);
	string recieveName(int clientSock, long size);
	void startGame(int sock);
	bool processGuess(int clientSock);
	long recieveNum(int clientSock);
	void sendMessage(string userNameStr, int sock);
	void sendLong(long guess, int sock);
	void* playGame(void* sockNum);
	void addToBoard(string name , long score);
	void sendLeader(int clientSock,leaderSlot board[]);
	void printLeaderboard(leaderSlot board[]);
	
int main(int argc, char *argv[])
{
	pthread_mutex_init( &lock, NULL);

if(argc!=2){
	cerr<<"Usage ./server PORT_NUMBER"<<endl;
}else{
	PORT_NUMBER = strtoul(argv[1],NULL,0); 
	for(unsigned i = 0 ; i < LEADERBOARD_SLOTS ; i++)
		{
			leaderBoard[i].rounds= (202);
			leaderBoard[i].name ="N/A";
		}
	printLeaderboard(leaderBoard);
	//Grab Socket Discripter
	int sock = createTCPSocket();
	//Bind Port
	int status = bindPort(PORT_NUMBER, sock);
	//Start loop for comunication
	while(true)
	{
		//Listen for connections
		status = setToListen(sock, MAXPENDING); 

	//accept requested connection and throw it to a thread
		pthread_t thread;
		pthread_attr_t attr;
		int threadExicutionStatus;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		intWrap wrap;
		wrap.num= acceptConnection(sock);
		threadExicutionStatus = pthread_create(&thread, NULL, playGame, (void *)&wrap);
		if (threadExicutionStatus)
		{
			printf("ERROR; return code from pthread_create() is %d\n", threadExicutionStatus);
			exit(-1);
		}

	}
}

}

//Prints out the leader boards
	void printLeaderboard(leaderSlot board[])
	{

		for(unsigned i = 0; i<  LEADERBOARD_SLOTS;i++)
		{
			if(board[i].rounds ==202){

			}else{
				cerr<<i+1<<": "<<board[i].name<< " in "<<board[i].rounds
				<<" rounds"<<endl;
			}
		}
	}
	// send the leader board
	void sendLeader(int clientSock,leaderSlot board[])
	{
		for(unsigned i = 0; i<  LEADERBOARD_SLOTS;i++)
		{
			sendLong(board[i].rounds, clientSock);
			sendMessage(board[i].name, clientSock);
		}
	}
	//Add new victory to leaderboard (also checks to see if they made it on the board)
	void addToBoard(string name , long score){

		pthread_mutex_lock(&lock);
		unsigned count = LEADERBOARD_SLOTS-1;
		leaderSlot newSlot;
		newSlot.name = name;
		newSlot.rounds = score;
		if(newSlot.rounds<leaderBoard[LEADERBOARD_SLOTS-1].rounds 
			|| leaderBoard[LEADERBOARD_SLOTS-1].rounds ==-1 )
			{
				leaderBoard[LEADERBOARD_SLOTS-1] = newSlot;

			while(count> 0 &&(
				leaderBoard[count].rounds < leaderBoard[count-1].rounds
				|| leaderBoard[count-1].rounds == -1))
				{

					newSlot = leaderBoard[count];

				leaderBoard[count] = leaderBoard[count-1];
				leaderBoard[count-1] = newSlot;
				count--;
				pthread_mutex_unlock(&lock);
				}
			}

	}


//void function for new game thread
void* playGame(void* sockNum){
	intWrap *sock;
	sock =(intWrap*)sockNum;
	startGame(sock->num);
}

//Start game with provided client socket
void startGame(int clientSock){
	string userName = "";
	srand(time(NULL));
	bool stillGuessing = true;
	
	long userNameSize;
	int roundsTaken = 1;
	//grab user name size
	userNameSize = recieveNum(clientSock);
	//Grab user name
	userName= recieveName(clientSock, userNameSize);
	cerr << userName << "Connected"<<endl;
	long targetNumber[NUMBER_OF_NUMBERS];
	long guesses[NUMBER_OF_NUMBERS];
	cerr<< "Random Numbers for player "<< userName <<": "<<endl;
	for(unsigned i = 0 ; i<NUMBER_OF_NUMBERS ; i++)
	{
		targetNumber[i] = (rand() % 200)+1; 
		cerr<<targetNumber[i]<<endl;
	}   

	
	while(stillGuessing)
	{

		for(unsigned i = 0 ; i < NUMBER_OF_NUMBERS ; i++)
		{
			guesses[i] = recieveNum(clientSock);
			cerr<< userName<<"'s guess number "<<i<<":" <<guesses[i]<<endl;
		}

		long under = 0;
		long over = 0;
		long correct = 0;

		for (unsigned a = 0 ; a < NUMBER_OF_NUMBERS ; a++)
		{
			
			if(guesses[a] == targetNumber[a])
			{
				correct++;
			}
			else if ( guesses[a] >targetNumber[a])
			{
				over++;
			}
			else
			{
				under++;
			}

		}

		sendLong(under, clientSock);
		sendLong(over, clientSock);
		sendLong(correct, clientSock);
		if(correct == NUMBER_OF_NUMBERS)
		{
			stillGuessing=false;
		}else{
			roundsTaken++;
		}

	}

	sendLong(roundsTaken, clientSock);
	addToBoard(userName, roundsTaken);
	cerr<<"Leaderboard:"<<endl;
	printLeaderboard(leaderBoard);
	sendLeader(clientSock, leaderBoard);
	close(clientSock);
	pthread_exit(NULL);

}

//Processes string and send its size then send the string
//returns true if the client needs to keep guessing

void sendMessage(string userNameStr, int sock){

//send name size
	sendLong(userNameStr.size()+1,sock);
	char userName[userNameStr.size()+1];
	strcpy(userName, userNameStr.c_str());
	int bytesSent = send(sock, (void *)&userName, userNameStr.size()+1, 0);
	if (bytesSent != userNameStr.size()+1) {
		cerr<<"Send fail";
		close(sock);
		pthread_exit(NULL);
	}
}
//Send a long to the provided client
void sendLong(long guess, int sock)
{

	long number = guess;
	number = htonl(number);
	int bytesSent = send(sock, (void *) &number, sizeof(long), 0);
	if (bytesSent != sizeof(long)) {
		cerr<<"Send fail";
		close(sock);
		pthread_exit(NULL);
	}


}

//Create TCP SOCKET
int createTCPSocket()
{
	//Grab Socket Discripter
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		cerr << "Error with socket" << endl;
		close(sock);
		pthread_exit(NULL);
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

return status;
}


int setToListen(int sockNumber, int const MAXPENDING)
{
	int status =listen(sockNumber, MAXPENDING); 
	if (status < 0) 
	{
		exit (-1);
	}
	return status;

}

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

//recieves a string from the provided socket with the provided string size
string recieveName(int clientSock, long size)
{

	int bytesLeft =size;
	char buffer[size];
	char *bp = buffer;
	while(bytesLeft>0){

		int bytesRecv = recv(clientSock,(void*) bp,bytesLeft,0);
		if(bytesRecv <=0) 
		{
			close(clientSock);
			pthread_exit(NULL);
		}
		bytesLeft = bytesLeft - bytesRecv;
		bp = bp +bytesRecv;
	}
	string name = string(buffer);
	return name;
}

// recieves a long integer from the provided socket

long recieveNum(int clientSock){

	int bytesLeft = sizeof(long);
	long numberRecieved ;
	char *bp = (char*) &numberRecieved;
	while(bytesLeft>0){

		int bytesRecv = recv(clientSock,(void*) bp,bytesLeft,0);
		
		if(bytesRecv <=0) 
		{
			close(clientSock);
			pthread_exit(NULL);
		}
		bytesLeft = bytesLeft - bytesRecv;
		bp = bp +bytesRecv;
	}


	numberRecieved = ntohl(numberRecieved);

	return numberRecieved;
}
