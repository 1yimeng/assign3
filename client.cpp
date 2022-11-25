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
    int port = atoi(argv[1]); 

    //create a message buffer 
    char msg[1500]; 
    char input[1500];

    //setup a socket and connection tools 
    struct hostent* host = gethostbyname(serverIp); 
    sockaddr_in sendSockAddr;   
    bzero((char*)&sendSockAddr, sizeof(sendSockAddr)); 
    sendSockAddr.sin_family = AF_INET; 
    sendSockAddr.sin_addr.s_addr = 
        inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
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
    cout << "Using port " << port << endl;
    cout << "Using server address " << serverIp << endl;
    string hostname_pid = get_host_id(hostname, pid);
    cout << "Host " << hostname_pid << endl;

    int bytesRead, bytesWritten = 0;
    struct timeval start1, end1;

    string data;
    int totalTrans = 0;

    while(getline(cin, data)) {
        memset(&msg, 0, sizeof(msg));//clear the buffer
        memset(&input, 0, sizeof(input));//clear the buffer

        string strNum = data.substr(1, data.length()-1);
        int num = stoi(strNum);

        if (data[0] == 'T') {
            string all = hostname_pid + " " + strNum;
            strcpy(msg, all.c_str()); 
            strcpy(input, data.c_str());
            send(clientSd, (char*)&msg, strlen(msg), 0);
            printf("%10.2f: Send (%s)\n", get_time(), input);

            memset(&msg, 0, sizeof(msg));//clear the buffer
            recv(clientSd, (char*)&msg, sizeof(msg), 0);

            // msg is message from client 
            printf("%10.2f: Recv (%1s%s)\n", get_time(), "D", msg);
            totalTrans += 1;

        } else if (data[0] == 'S') {
            printf("Sleep %3d units\n", num);
            // Sleep(num);
        }
    }

    close(clientSd);
    printf("Sent %d transactions\n", totalTrans);

    return 0;    
}
