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

#include <tr1/unordered_map>
#include <string.h>
#include <netdb.h>
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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <vector>
#define PORT "3000"   // port we're listening on

    using namespace std;
void messageParser(string message, int i);
string recieveMessage(int clientSock);
void sendLong(long guess, int sock);
void sendMessage(string userNameStr, int sock);
 long recieveNum(int clientSock);
 bool isclosed(int socket);
     tr1::unordered_map <string, int> activeUsers;
    vector<string> activeUserKeys;


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{



    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            cerr<<"rro 1"<<endl;
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("ChatServer: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else if(!isclosed(i)) {
                    // handle data from a client
                    string message = recieveMessage(i);
                    cerr<<"text recieved:"<< message<<endl;
                    messageParser(message , i);
                    // if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                    //     // got error or connection closed by client
                    //     if (nbytes == 0) {
                    //         // connection closed
                    //         printf("ChatServer: socket %d hung up\n", i);
                    //     } else {
                    //         perror("recv");
                    //     }
                    //     close(i); // bye!
                    //     FD_CLR(i, &master); // remove from master set
                    // } else {
                    //     // we got some data from a client
                    //     for(j = 0; j <= fdmax; j++) {
                    //         // send to everyone!
                    //         if (FD_ISSET(j, &master)) {
                    //             // except the listener and ourselves
                    //             if (j != listener && j != i) {
                    //                 //long temp = sizeOf(buff);
                    //                 sendLong(sizeof(buf),j);
                    //                 if (send(j, buf, nbytes, 0) == -1) {
                    //                     perror("send");
                    //                 }
                    //             }
                    //         }
                    //     }
                    // }
                }else{
                    printf("ChatServer: socket %d hung up\n", i);
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set

                } 
                // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}

string recieveMessage(int clientSock)
{
    
    long size = recieveNum(clientSock);
    int bytesLeft = size;
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
    string name = string(buffer,size);
    return name;
}

void sendMessage(string message, int sock){

    //send name size
    sendLong(message.size(),sock);
    //send name
    char userName[message.size()];
    strcpy(userName, message.c_str());
    int bytesSent = send(sock, (void *)&userName, message.size(), 0);
    if (bytesSent != message.size()) {
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


void messageParser(string message,int i)
{
    //Check if user is client is trying to register a new user

    if(message.find("<login>") != std::string::npos)
    {
       string extractedUserName =  message.substr(message.find("<login>")+strlen("<login>"),string::npos);
       tr1::unordered_map<string,int>::const_iterator findResult = activeUsers.find (extractedUserName);

       if(findResult != activeUsers.end())
        {
            sendMessage("Sorry That User Name is Taken", i);

        }else{
       activeUsers.insert(std::make_pair<std::string,int>(extractedUserName,i)); 
       activeUserKeys.push_back(extractedUserName);

        sendMessage("you are now logged in as: " + extractedUserName, i);
    }
    }else {
sendMessage(message, i);
    }
}

bool isclosed(int sock) {
  fd_set rfd;
  FD_ZERO(&rfd);
  FD_SET(sock, &rfd);
  timeval tv = { 0 };
  select(sock+1, &rfd, 0, 0, &tv);
  if (!FD_ISSET(sock, &rfd))
    return false;
  int n = 0;
  ioctl(sock, FIONREAD, &n);
  return n == 0;
}

