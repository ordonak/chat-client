//This program acts as a client for a simple chat program
#define name client
#include <cstdlib>
#include <cstdio>
#include <string.h>
#include <netdb.h>
#include <string.h>
#include <list>
#include <vector>
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
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <ncurses.h>

using namespace std;

//Window Structs
typedef struct _win_border_struct {
    chtype  ls, rs, ts, bs, 
    tl, tr, bl, br;
}WIN_BORDER;

typedef struct _WIN_struct {

    int startx, starty;
    int height, width;
    WIN_BORDER border;
}WIN;

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
string parseMessage(string mess, bool& userL);
vector<string> parseUsers(string newUsers);
string createMessage(string message);

//Ncurses functions
void initCurses(WIN userWindow);
void init_win_params(WIN *p_win);
void print_win_params(WIN *p_win);
void create_box(WIN *p_win, bool flag);
WINDOW *create_newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW *local_win);
void update_scr(WINDOW * userWindow);


//THREAD FUNCTIONS

void createConn();
void * uiThread(void * info);
void * connThread(void * info);


static connInfo cInfo;
static char *IPAddr = "10.124.72.20";
static unsigned short PORT_NUMBER = 10105;
static int LEADERBOARD_SLOTS = 3;
static string userName = "";
string sharedMess="";
static bool updated = false;
vector<string> userList;

//Synchronization variables
pthread_mutex_t MESSAGE_LOCK;


int main(int argc, char *argv[])
{
    WIN userWindow;
    WINDOW* userWind;
    WINDOW* chatWind;

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
        
        //send name size and name 
        bool inChat = true;
        bool loggedIn = false;
        while(inChat)
        {
            if(!loggedIn){
                cerr << "Please log in with your username: ";
                getline(std::cin,userName);
                sendMessage("<login>"+userName);
            }
            cerr << "Send a message:" << endl;
            string message;
            getline(std::cin,message);
            if(message != "r"){
                sendMessage(message, cInfo.sock);
            }
            if(updated){
                cerr << sharedMess <<endl;
                pthread_mutex_lock(&MESSAGE_LOCK);
                updated = false;
                pthread_mutex_unlock(&MESSAGE_LOCK);
                sharedMess = "";
            }
        }
        //Close connection
        close(cInfo.sock);
    }
}

void createConn()
{
    pthread_t connID;

    int rc = pthread_create(&connID, NULL, connThread, 
        NULL);
    if(rc){
        cerr << "ERROR with CONN thread." << endl;
        exit(-1);
    }
}

void * connThread(void * info){
    //Establish Connection
    //Convert dotted decimal address to int unsigned long servIP;
    unsigned long servIP;
    int status = inet_pton(AF_INET, IPAddr, &servIP); 
    
    if (status <= 0) exit(-1);
    bool uList = false;
    // Set the fields
    struct sockaddr_in servAddr; 
    servAddr.sin_family = AF_INET; // always AF_INET 
    servAddr.sin_addr.s_addr = servIP; 
    servAddr.sin_port = htons(PORT_NUMBER);
    //connect
    status = connect(cInfo.sock, (struct sockaddr *) &servAddr, sizeof(servAddr));
    if (status < 0) 
    {
        cerr << "Error with connect" << endl;
        exit (-1);
    }
    while(true){
        string mess = recieveMessage(cInfo.sock);
        sharedMess = parseMessage(mess, uList);
        cerr << "New messages have come-in(input just r to refresh.)" << endl;
        if(uList){
            userList = parseUsers(sharedMess);
            uList = new bool;
        }
        else{
            pthread_mutex_lock(&MESSAGE_LOCK);
            updated = true;
            pthread_mutex_unlock(&MESSAGE_LOCK);
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

string parseMessage(string mess, bool& userL){
    if(mess.find("<userList>")!= string::npos){
    userL = true;
    string userList = mess.substr(mess.find("<userList>")
        +mess.find(strlen("</userList>")));
    
        return userList;
    }

    else{
        int authorStart = mess.find("<from>")+strlen("<from>");
        int authorEnd = mess.find("</from>");

        int msgStart = mess.find("<msg>")+strlen("<msg>");
        int msgEnd = mess.find("</msg>");

        cerr << "Whole message: " << mess << endl;
        string author = mess.substr(authorStart,(authorEnd-authorStart));
        string sentMess = mess.substr(msgStart, (msgEnd-msgStart));
        cerr<< endl << author<<": " << sentMess << endl;
        return(author + ": " + sentMess);
    }
}


vector<string> parseUsers(string newUsers){
    vector<string> userList;
    char *str =  new char[newUsers.length()+1];
    strcpy(str, newUsers.c_str());
    char *tok;
    tok = strtok(str,"\n");
    while(tok != NULL)
    {
        userList.push_back(string(tok));
        tok = strtok(str, "\n");
    }
    return userList;
}

string createMessage(string message)
{
    int recipStart = mess.find("@")+strlen("@");
    int recipEnd = mess.find(" ");

    int msgStart = message.find(" ")+1;

    string author = message.substr(recipStart,(recipEnd-recipStart));
    string msg = message.subStr(msgStart,string::npos);

    string toSend= "<sendto>"+author+"</sendto>"
        +"<from>"+userName+"</from>"+"<msg>"+msg+"</msg>";

    return toSend;
}


void initCurses(WIN userWindow)
{
    initscr();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);

    //init_win_params(&userWindow);
    //print_win_params(&userWindow);
    attron(COLOR_PAIR(1));
    //create_box(&userWindow, true);
    refresh();
}

void init_win_params(WIN *p_win)
{
    p_win->height = LINES;
    p_win->width = 20;
    p_win->starty = 0;  
    p_win->startx = 0;
    p_win->border.ls = '|';
    p_win->border.rs = '|';
    p_win->border.ts = '-';
    p_win->border.bs = '-';
    p_win->border.tl = '+';
    p_win->border.tr = '+';
    p_win->border.bl = '+';
    p_win->border.br = '+';

}
void print_win_params(WIN *p_win)
{
#ifdef _DEBUG
    mvprintw(25, 0, "%d %d %d %d", p_win->startx, p_win->starty, 
        p_win->width, p_win->height);
    refresh();
#endif
}
void create_box(WIN *p_win, bool flag)
{   int i, j;
    int x, y, w, h;

    x = p_win->startx;
    y = p_win->starty;
    w = p_win->width;
    h = p_win->height;

    if(flag == TRUE)
        {   mvaddch(y, x, p_win->border.tl);
            mvaddch(y, x + w, p_win->border.tr);
            mvaddch(y + h, x, p_win->border.bl);
            mvaddch(y + h, x + w, p_win->border.br);
            mvhline(y, x + 1, p_win->border.ts, w - 1);
            mvhline(y + h, x + 1, p_win->border.bs, w - 1);
            mvvline(y + 1, x, p_win->border.ls, h - 1);
            mvvline(y + 1, x + w, p_win->border.rs, h - 1);

        }
        else
            for(j = y; j <= y + h; ++j)
                for(i = x; i <= x + w; ++i)
                    mvaddch(j, i, ' ');
                
                refresh();

            }

            void update_scr(WINDOW* userWindow)
            {
                mvwprintw(userWindow, 2,2, "Users");
                refresh();
            }

            WINDOW *create_newwin(int height, int width, int starty, int startx)
            {   WINDOW *local_win;

                local_win = newwin(height, width, starty, startx);
    box(local_win, 0 , 0);      /* 0, 0 gives default characters 
                     * for the vertical and horizontal
                     * lines            */
    wrefresh(local_win);        /* Show that box        */

                     return local_win;
                    }

                    void destroy_win(WINDOW *local_win)
                    {   
    /* box(local_win, ' ', ' '); : This won't produce the desired
     * result of erasing the window. It will leave it's four corners 
     * and so an ugly remnant of window. 
     */
     wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
    /* The parameters taken are 
     * 1. win: the window on which to operate
     * 2. ls: character to be used for the left side of the window 
     * 3. rs: character to be used for the right side of the window 
     * 4. ts: character to be used for the top side of the window 
     * 5. bs: character to be used for the bottom side of the window 
     * 6. tl: character to be used for the top left corner of the window 
     * 7. tr: character to be used for the top right corner of the window 
     * 8. bl: character to be used for the bottom left corner of the window 
     * 9. br: character to be used for the bottom right corner of the window
     */
     wrefresh(local_win);
     delwin(local_win);
}
