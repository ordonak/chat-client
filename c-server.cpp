// Standard includes
#include <stdlib.h>
#include <signal.h>
#include <semaphore.h>
#include <iostream>
#include <fstream>
#include <queue>
#include <list>
#include <sstream>
#include <vector>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>

using namespace std;

// File includes
#include "network.cpp"

const int MAX_THREADS = 35;
const int EXIT_SIGNAL = 2;
const int OK_CODE = 1;
const int BAD_CODE = -1;
const char * SERV_PORT = "13369";

//
vector<int> SOCKET_VECTOR;


int main(int argc, char *argv[])
{

  // Bind Ctrl+C Signal to exitHandler()
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = exitHandler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  // Bind returnHandler() to return/exit event
  atexit(returnHandler);

  // Initialize thread pool
  initializeThreadPool();

  cerr << "Starting chat-server" << endl;
  while(true){
  }
}

void initializeThreadPool() {
  for(int i = 0; i < MAX_THREADS; i++) {
    pthread_t tid;
    threadInfo curr;
    curr.num = i;

    int rc = pthread_create(&tid, NULL, consumeRequest,
      (void *)(&curr));
    if (rc) {
      //log("ERROR in ThreadPool initialization");
      exit(BAD_CODE);
    }
    pthread_detach(tid);
  }
}