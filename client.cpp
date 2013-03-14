//This program acts as a client for a simple chat program
#define name client
#include <cstdio>
#include <string.h>
#include <netdb.h>
#include <string.h>
#include <list>
#include <vector>
#include <iostream>
#include <sstream>
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
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <ncurses.h>

using namespace std;


struct connInfo{
    int sock;
};


//function declarations
int createTCPSocket();
int bindPort(short portNumber, int sockNumber);
int acceptConnection(int sockNumber);
string recieveMessage(int clientSock);
void sendMessage(string message, int sock);
void sendLong(long guess, int sock);
long recieveNum(int clientSock);
bool parseMessage(string mess);
vector<string> parseUsers(string newUsers);
string getUser();
string createMessage(string toSend);
void sendToCurrentUser(string message);
void toggleUpdate(bool expression);
void login();
void addMessage(string user, string msg,string from);
void addUser(string name);
void removeUser(string oldUser);
void displayUserList(int pRow,int pCol, int selected);
void printConvo(string user);
void renderCommandLine(int x, int y);


//THREAD FUNCTIONS
void createConn();
void * connThread(void * info);

//Global var
string currentConvo="0";
static connInfo cInfo;
static char *IPAddr = "10.124.72.20";
static unsigned short PORT_NUMBER = 10105;
static int LEADERBOARD_SLOTS = 3;
static string userName;
string sharedMess="";
string sendToUser="";
string fromUser = "";
string inputPrompt = "";
string commandLine;
static bool updated = false;
vector<pair<string, string> > userList;
bool loggedIn = false;
bool waiting = true;
int mode = 0;
int const LIST = 0;
int const CONVO = 1;
//Synchronization variables
pthread_mutex_t MESSAGE_LOCK;


int main(int argc, char *argv[])
{

    if(argc!=3)
        cerr<<"Usage ./client IP_ADDRESS PORT_NUMBER"<<endl;
    else
    {
        //initCurses(userWindow);
        //userWind = create_newwin(LINES, 15, 0, 0);
        //chatWind = create_newwin(LINES, (COLS-16), 0, 15);
        //mvwprintw(userWind, 0, 0, "asdasd");
        IPAddr = argv[1]; 
        PORT_NUMBER = strtoul(argv[2],NULL,0);
        cInfo.sock = createTCPSocket();
        createConn();
        int keyPressed;
        bool inChat = true;
        //send name size and name 

        //initializing nCurses
        initscr();
        cbreak();
        nodelay(stdscr,TRUE);
        keypad(stdscr, TRUE);

        wchar_t f1checker;
        
        int row,col;
        scrollok(stdscr,true);
        login();

        getUser();
        toggleUpdate(true);
        noecho();
        while(inChat)
        {
            
            if ((keyPressed = getch()) == ERR) {
             //user hasn't responded
                if(updated)
                {
                    clear();
                    getmaxyx(stdscr,row,col);       /* get the number of rows and columns */
                    char *display = (char*)sharedMess.c_str();
                    printw(display);
                    if(mode == CONVO)
                    {
                        printConvo(currentConvo);
                        printw(string(userName+":").c_str());
                    }
                    toggleUpdate(false);
                    sharedMess = "";
                }
            }else
            {
                    //pull up userList if F1 pressed
                if(keyPressed==KEY_F(1))
                {
                    getUser();
                    toggleUpdate(true);
                    commandLine.clear();
                }else if ( keyPressed == KEY_BACKSPACE)  // detect backspace
                {
                    if(commandLine.size()>=1)
                    {
                        commandLine.erase(commandLine.size() - 1);
                        clear();
                        if(mode == CONVO)
                        {
                        printConvo(currentConvo);
                        printw(string(userName + ":" + commandLine).c_str());
                        }else
                        {
                            printw(commandLine.c_str());
                        }
                    }

                }
                else if(keyPressed != '\n')
                {
                    commandLine += keyPressed;
                    clear();
                    if(mode == CONVO)
                    {
                        printConvo(currentConvo);
                        printw(string(userName + ":" + commandLine).c_str());
                    }else
                    {
                        printw(commandLine.c_str());
                    }
                }
                else 
                {
                    sendToCurrentUser(commandLine);
                    commandLine.clear();
                }
            }
        }
        //Close connection
        close(cInfo.sock);
        endwin();
    }
}


void convoMode()
{


}

void renderCommandLine(int x, int y)
{

}

void sendToCurrentUser(string message)
{
    string msg = createMessage(message);
    string fromSigniture ="<from>"+userName+"</from>";
    string toSigniture = "<sendto>"+currentConvo+"</sendto>";
    sendMessage(toSigniture+fromSigniture+msg, cInfo.sock);
    addMessage(currentConvo,message, userName);
    toggleUpdate(true);

}

void login()
{
    char keyPressed;
    string temp;
    string returnMess;
    printw("Please log in:");
       while(!loggedIn){
                if ((keyPressed = getch()) == ERR){

                }
                else{
                    if(keyPressed != '\n')
                        temp+= keyPressed;
                    else{
                        userName=temp;
                        fromUser="<from>"+userName+"</from>";
                        sendMessage("<login>"+userName, cInfo.sock);
                        while(waiting)
                        {
                            
                        }
                        clear();
                        printw(string("Sorry the username \""+temp+"\" has been taken\n").c_str());
                        temp.clear();
                        printw("Please log in with a different name:");
                    }

                }
            }
    clear();

}

void createConn()
{   pthread_t connID;

    int rc = pthread_create(&connID, NULL, connThread, 
        NULL);
    if(rc){
        //cerr << "ERROR with CONN thread." << endl;
        exit(-1);
    }
}

void * connThread(void * info){
    //Establish Connection
    //Convert dotted decimal address to int unsigned long servIP;
    unsigned long servIP;
    int status = inet_pton(AF_INET, IPAddr, &servIP); 

    if (status <= 0) exit(-1);
    // Set the fields
    struct sockaddr_in servAddr; 
    servAddr.sin_family = AF_INET; // always AF_INET 
    servAddr.sin_addr.s_addr = servIP; 
    servAddr.sin_port = htons(PORT_NUMBER);
    //connect
    status = connect(cInfo.sock, (struct sockaddr *) &servAddr, sizeof(servAddr));
    if (status < 0) 
    {
        //cerr << "Error with connect" << endl;
        exit (-1);
    }
    while(true){
        string mess = recieveMessage(cInfo.sock);
        bool gotRealData = parseMessage(mess);
        //cerr << "New messages have come-in(input just r to refresh.)" << endl;
        if(gotRealData)
        {
           toggleUpdate(true);
        }
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

void toggleUpdate(bool expression)
    {
        pthread_mutex_lock(&MESSAGE_LOCK);
            updated = expression;
        pthread_mutex_unlock(&MESSAGE_LOCK);

    }


        //Accept connection with given sock number
int acceptConnection(int sockNumber)
{
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int clientSock = accept(sockNumber,(struct sockaddr *)
        &clientAddr, &addrLen);
    if (clientSock < 0)
        exit(-1);

    return clientSock;
}

    //Recieve a string of the given size with the given sock
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
        exit(-1);
    }
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

bool parseMessage(string mess)
{
    if(mess.find("<logon>")!= string::npos ) //User is logging in
    {
        int userStart = mess.find("<logon>")+strlen("<logon>");
        int userEnd = mess.find("</logon>");
        string extractedUser =  mess.substr(userStart,userEnd-userStart);

       addUser(extractedUser);
        return true;

    }else if(mess.find("<logoff>")!= string::npos) //User is logging off
    {
        int userStart = mess.find("<logoff>")+strlen("<logoff>");
        int userEnd = mess.find("</logoff>");
        string extractedUser =  mess.substr(userStart,userEnd-userStart);
        removeUser(extractedUser);
        return true;
    }
    else if(mess.find("<confirm>")!= string::npos) //User is logging off
    {
        loggedIn = true;
        waiting = false;
        return true;
    }
    else if(mess.find("<deny>")!= string::npos)
    {
        loggedIn = false;
        waiting = false;
        return true;
    }

    else if (mess.find("<from>")!= string::npos){
        int authorStart = mess.find("<from>")+strlen("<from>");
        int authorEnd = mess.find("</from>");

        int msgStart = mess.find("<msg>")+strlen("<msg>");
        int msgEnd = mess.find("</msg>");

        string author = mess.substr(authorStart,(authorEnd-authorStart));
        string sentMess = mess.substr(msgStart, (msgEnd-msgStart));
       addMessage(author,sentMess,author);
        return true;
    }else
    {
        return false;
    }


}

    void addUser(string newUser)
    {
        
        bool addToList = true;
         if(newUser.compare(userName) == 0)
            {

                 addToList = false;
                //if you are this user do nothing?
            }
        for(int i = 0; i < userList.size(); i++)
        {
           if(newUser.compare( userList[i].first)==0)
             {
                //if the user is already in our list of conversations
                addToList = false;

            }
         }
         if(addToList){
          userList.push_back(pair<string,string>(newUser,""));

        }

    }

    void addMessage(string user, string msg, string from)
    {

        for(int i = 0; i < userList.size(); i++)
        {
            if(user.compare(userName)==0)
            {
                //if you are this user do nothing?
             }else if(user.compare(userList[i].first)==0)
             {
                //append message to user string
                userList[i].second.append(from+":"+msg + "\n");

             }else{

                //do nothing?  user doesnt exists
            }

        }

    }

    void printConvo(string user)
    {
  
  
        for(int i = 0; i < userList.size(); i++)
        {
            if(user.compare(userList[i].first)==0)
            {
                //append message to user string
                printw(userList[i].second.c_str());

            }

        }


    }

    void removeUser(string oldUser)
    {
        int index;
        bool found = false;
        for(int i = 0; i < userList.size(); i++)
        {
            if(oldUser.compare(userName)==0)
            {
                //if you are this user do nothing?
             }else if(oldUser.compare(userList[i].first)==0)
             {
                //if the user is already in our list of conversations
               index = i;
                found = true;


             }else{

                //userList.push_back(pair<string,string>(newUser,""));
            }

         }

         if(found)
         {
             userList.erase(userList.begin() + index);
         }
    }


string createMessage(string toSend){
    return("<msg>"+toSend+"</msg>");
}

string getUser()
{
    displayUserList(1, 1,0);
    int keyPressed;
    string message = "";
    int i = 0;
    while(true)
    {

        if ((keyPressed = getch()) == ERR) 
        {
            if(updated)
            {
                clear();
                displayUserList(1, 1,i);
                toggleUpdate(false);

            }
   
        }
        else
        {
            if(keyPressed == KEY_DOWN)
            {
                if((i+1)>userList.size()-1)
                {

                }else
                {
                    clear();
                    i++;
                    displayUserList(1, 1,i);
                } 
                
            }else if(keyPressed == KEY_UP)
            {
                
                if((i-1)<0 )
                    {

                    }else
                    {
                        clear();
                        i--;
                        displayUserList(1, 1,i);
                    }
                
            }

            else if(keyPressed != '\n')
            {

            }
            else
            {
                if(userList.size()>0){
                clear();

                currentConvo = userList[i].first;
                mode = CONVO;
                inputPrompt =string("To-User:"+ userList[atoi(message.c_str())].first+":");
                printw(inputPrompt.c_str());
                return string("<sendto>"+userList[atoi(message.c_str())].first+"</sendto>");
            }
            }
        }

    }
}

void displayUserList(int pRow, int pCol, int selected)
{
 
    int row, col;
    
    getmaxyx(stdscr,row,col);
    int divide = col/3;
    clear();
    stringstream str; 
    int r=2;
   
    mvprintw(r++,divide-6,string("  [People Online]").c_str());
    mvprintw(r++,divide-6,string("(Use The Arrow Keys)").c_str());
    for(int i = 0; i < userList.size(); i++)
    {
        if(i!=selected)
        {
        mvprintw(r++,divide,str.str().c_str());
        printw(string(userList[i].first).c_str());
        }
        else
        {   mvprintw(r++,divide-3,string("-->").c_str());
            printw(str.str().c_str());
            printw(string(userList[i].first).c_str());
            printw(string("<--").c_str());
        }
    }

    mvprintw(row-1,col-1,string("").c_str());

}
