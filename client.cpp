#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <limits.h>
#include "tands.h"
#include "helper.h"

using namespace std;

int main(int argc, char *argv[]) {

    // we need 2 things: port number and ip address, in that order
    //grab the IP address and port number 
    char *serverIp = argv[2]; 
    int port;
    if (argv[1] != NULL) {
        port = atoi(argv[1]); 
    }

    //create a message buffer 
    char msg[1500]; 
    char input[1500];

    //setup a socket and connection tools 
    struct hostent* host = gethostbyname(serverIp); 
    sockaddr_in sendSockAddr;   
    bzero((char*)&sendSockAddr, sizeof(sendSockAddr)); 
    sendSockAddr.sin_family = AF_INET; 

    if (host != NULL) {
        sendSockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    }

    sendSockAddr.sin_port = htons(port);
    int clientSd = socket(AF_INET, SOCK_STREAM, 0);

    //try to connect...
    int status = connect(clientSd,
                         (sockaddr*) &sendSockAddr, sizeof(sendSockAddr));
    if(status < 0) {
        cout<<"Error connecting to socket!"<<endl;
        return -1;
    }

    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    pid_t pid = getpid();
    string hostname_pid = get_host_id(hostname, pid);

    string file_name = hostname_pid + ".log";
    const char* cFile_name = file_name.c_str();
    FILE* pFile = fopen(cFile_name, "w");

    fprintf(pFile, "Using port %d\n", port);
    fprintf(pFile, "Using server address %s\n", serverIp);
    fprintf(pFile, "Host %s\n", hostname_pid.c_str());

    struct timeval start1, end1;

    string data;
    int totalTrans = 0;
    string all;
    string strNum;
    int num;

    while(getline(cin, data)) {
        memset(&msg, 0, sizeof(msg));//clear the buffer
        memset(&input, 0, sizeof(input));//clear the buffer

        if (data.length() > 1) {
            strNum = data.substr(1, data.length()-1);
        }

        if (!strNum.empty()) {
            num = stoi(strNum);
        }

        if (data[0] == 'T') {
            all = hostname_pid + " " + strNum;
            strcpy(msg, all.c_str()); 
            strcpy(input, data.c_str());
            send(clientSd, (char*)&msg, strlen(msg), 0);
            fprintf(pFile, "%10.2f: Send (%s)\n", get_time(), input);

            memset(&msg, 0, sizeof(msg));//clear the buffer
            recv(clientSd, (char*)&msg, sizeof(msg), 0);

            // msg is message from client 
            fprintf(pFile, "%10.2f: Recv (%1s%s)\n", get_time(), "D", msg);
            totalTrans += 1;

        } else if (data[0] == 'S') {
            fprintf(pFile, "Sleep %3d units\n", num);
            Sleep(num);
        }
    }

    close(clientSd);
    fprintf(pFile, "Sent %d transactions\n", totalTrans);
    fclose(pFile);
    return 0;    
}
