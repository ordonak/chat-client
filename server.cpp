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
    
//function declarations
void messageParser(string message, int i);
string recieveMessage(int clientSock);
void sendLong(long guess, int sock);
void sendMessage(string userNameStr, int sock);
long recieveNum(int clientSock);
bool isclosed(int socket);
void showUsers();
void sendListOfUsers();
void sendLogon(string extractedUserName);
void logoutUser(int fd);
tr1::unordered_map <string, int> activeUsers;
tr1::unordered_map <int, string> activeLogins;
vector<string> activeUserKeys;
void sendLogoff(string extractedUserName);
void *get_in_addr(struct sockaddr *sa);

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
                    showUsers();
               
                }else{
                    printf("ChatServer: socket %d hung up\n", i);
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                        logoutUser(i);
                        showUsers();

                } 
                // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
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

        int userStart = message.find("<login>")+strlen("<login>");
        int userEnd = message.find("</login>");
       string extractedUserName =  message.substr(userStart,userEnd-userStart);
       tr1::unordered_map<string,int>::const_iterator activeUsersFindResult = activeUsers.find (extractedUserName);
       if(activeUsersFindResult != activeUsers.end())
        {
           sendMessage("<from>SYSTEM</from><deny>",i);

        }else if(activeLogins.find(i) == activeLogins.end() )  //Check if user is logged in
        {
        activeUsers.insert(std::make_pair<std::string,int>(extractedUserName,i)); 
        activeLogins.insert(std::make_pair<int,string>(i,extractedUserName)); 
        activeUserKeys.push_back(extractedUserName);
        cerr<<"<from>SYSTEM</from><confirm>"<<endl;
        sendMessage("<from>SYSTEM</from><confirm>", i);

        sendListOfUsers();
        }
        else
        {
        sendMessage("<from>SYSTEM</from><deny>" , i);
        }
    }else if ((message.find("<sendto>") != string::npos) ) //Check if they are trying to send a message
    {
        //Find mark ups start and end positions
        int recipeintStart = message.find("<sendto>")+strlen("<sendto>");
        int recipeintEnd = message.find("</sendto>");
        int authorStart = message.find("<from>")+strlen("<from>");
        int authortEnd = message.find("</from>");
        int msgStart = message.find("<msg>");
        int msgEnd = message.find("</msg>"+strlen("</msg>"));

        //Parse our data given the markup start and end points
       string recipeint =  message.substr(recipeintStart,(recipeintEnd-recipeintStart));
       string author = message.substr(authorStart,(authortEnd-authorStart));
       string msg = message.substr(msgStart,(msgEnd-msgStart));
       
       //Check if user exists and grab their socket number
        tr1::unordered_map<string,int>::const_iterator recipientSocketIter = activeUsers.find(recipeint);
        if(recipientSocketIter != activeUsers.end()){
        int recipeintSocket = (*recipientSocketIter).second; 
        cerr<<"Sending message to:"<<recipeint<<" From:"<<author<<endl;
       sendMessage(message, recipeintSocket);
        }else
        {
            sendMessage("<sendto>" + author + "</sendto><from>SYSTEM</from><msg>Sorry we couldn't find that user :(</msg>", i);
        }
//<sendto>Russell</sendto><from>Ken</from><msg>Yo it worked bitch!</msg>
    }else
    {
        cerr<<message<<endl<<"User was not logged in"<<endl;
    //sendMessage(message, i);
    }
}

void sendListOfUsers()
{
    //cerr<<"Users Currently Logged On:"<<endl;

 
    for (int count = 0 ; count < activeUserKeys.size() ; count++)
    {
    tr1::unordered_map<string,int>::const_iterator recipientSocketIter = activeUsers.find(activeUserKeys[count]);
        if(recipientSocketIter != activeUsers.end())
        {
            string author = (*recipientSocketIter).first;
            int recipeintSocket = (*recipientSocketIter).second; 

             for (int count = 0 ; count < activeUserKeys.size() ; count++)
            {
                cerr<<"SENDING:"<<"<logon>" << activeUserKeys[count] <<"</logon> to:"<<author<<endl;
                 sendMessage("<logon>" + activeUserKeys[count] + "</logon>", recipeintSocket);
            }

           
        }else
        {
        cerr<<"Error Deliverying List Of Users"<<endl;
        }
    }
}

void sendLogon(string name)
{
        //cerr<<"Users Currently Logged On:"<<endl;
    string userListMsg = "<logon>";
    userListMsg.append(name);
    userListMsg.append("</logon>");
    //cerr<<userListMsg;
    for (int count = 0 ; count < activeUserKeys.size() ; count++)
    {
    tr1::unordered_map<string,int>::const_iterator recipientSocketIter = activeUsers.find(activeUserKeys[count]);
        if(recipientSocketIter != activeUsers.end())
        {
            string author = (*recipientSocketIter).first;
            int recipeintSocket = (*recipientSocketIter).second; 
            sendMessage(userListMsg, recipeintSocket);
        }else
        {
        cerr<<"Error Deliverying List Of Users"<<endl;
        }
    }


}

void sendLogonOff(string name)
{
        //cerr<<"Users Currently Logged On:"<<endl;
    string userListMsg = "<logoff>";
    userListMsg.append(name);
    userListMsg.append("</logoff>");
    //cerr<<userListMsg;
    for (int count = 0 ; count < activeUserKeys.size() ; count++)
    {
    tr1::unordered_map<string,int>::const_iterator recipientSocketIter = activeUsers.find(activeUserKeys[count]);
        if(recipientSocketIter != activeUsers.end())
        {
            string author = (*recipientSocketIter).first;
            int recipeintSocket = (*recipientSocketIter).second; 
            sendMessage(userListMsg, recipeintSocket);
        }else
        {
        cerr<<"Error Deliverying List Of Users"<<endl;
        }
    }


}

void logoutUser(int fd)
{

       // tr1::unordered_map <string, int> activeUsers;
    //tr1::unordered_map <int, string> activeLogins;
    //vector<string> activeUserKeys;
    string userName;
    tr1::unordered_map<int,string>::const_iterator activeLoginsFindResult = activeLogins.find (fd);
    if(activeLoginsFindResult != activeLogins.end())
    {
        userName = (*activeLoginsFindResult).second; 
        cerr<<userName<<"Is LOGGING OFF!"<<endl;
        activeLogins.erase(fd);

        

          tr1::unordered_map<string,int>::const_iterator activeUsersFindResult = activeUsers.find (userName);
    if(activeUsersFindResult != activeUsers.end())
    {
        activeUsers.erase(userName);
        sendLogonOff(userName);
    }
    int userNameIndex;
    for(int count = 0 ; count < activeUserKeys.size() ; count++)
    {
        if(activeUserKeys[count].compare(userName)==0)
        {
            userNameIndex = count;

        }
    }


    activeUserKeys.erase(activeUserKeys.begin() + userNameIndex);

    }

  
}

void showUsers()
{
    cerr<<"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nUsers Currently Logged On:"<<endl;
    for (int count = 0 ; count<activeUserKeys.size();count++)
        cerr<<activeUserKeys[count]<<endl;
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
