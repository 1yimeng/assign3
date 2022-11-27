#include <stdio.h> 
#include <string.h>   //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   //close 
#include <arpa/inet.h>    //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include <string>
#include <map>
#include <iostream>
#include <time.h>
#include <limits.h>
#include "tands.h"
#include "helper.h"
     
#define TRUE   1 
#define FALSE  0 

int main(int argc , char *argv[]) { 
    int port; 
    if (argv[1] != NULL) {
        port = atoi(argv[1]);
    }

    int opt = TRUE;  
    int master_socket , addrlen , new_socket, 
          max_clients = 30 , activity, i , valread , sd, jobNum = 0;  
    int client_socket[30];
    map<string, int> all_jobs;
    int max_sd;  
    struct sockaddr_in address;  
    bool first = true;
    double begin = 0, end = 0;
    struct timeval timeout;

    string file_name = "server.log";
    const char* cFile_name = file_name.c_str();
    FILE* pFile = fopen(cFile_name, "w");
        
    char message[1025];  //data buffer of 1K 
    
    timeout.tv_sec = 30; 
    timeout.tv_usec = 0;
    //set of socket descriptors 
    fd_set readfds;  
     
    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < max_clients; i++) {  
        client_socket[i] = 0;  
    }  
         
    //create a master socket 
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
     
    //set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
          sizeof(opt)) < 0 ) {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
     
    //type of socket created 
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons(port);  
         
    //bind the socket to port
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    fprintf(pFile, "Using port %d \n", port);  
         
    //try to specify maximum of 3 pending connections for the master socket 
    if (listen(master_socket, 3) < 0) {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
         
    //accept the incoming connection 
    addrlen = sizeof(address);   
         
    while(TRUE) {  
        //clear the socket set 
        FD_ZERO(&readfds);  
     
        //add master socket to set 
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
             
        //add child sockets to set 
        for ( i = 0 ; i < max_clients ; i++) {  
            //socket descriptor 
            sd = client_socket[i];  
                 
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &readfds);  
                 
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }  

        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , &timeout);  

        if ((activity < 0) && (errno!=EINTR)) {  
            fprintf(pFile, "select error");  
        }  

        if (activity == 0) {
            break;
        }
             
        //If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(master_socket, &readfds)) {  
            if ((new_socket = accept(master_socket, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
                 
            //add new socket to array of sockets 
            for (i = 0; i < max_clients; i++) {  
                //if position is empty 
                if( client_socket[i] == 0 ) {  
                    client_socket[i] = new_socket;  
                    break;  
                }  
            }  
        }  
             
        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++) {  
            sd = client_socket[i];  
                 
            if (FD_ISSET( sd , &readfds)) {  
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = recv(sd, (char*)&message, sizeof(message), 0)) == 0) {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                         
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd );  
                    client_socket[i] = 0; 
                }  
                     
                //Echo back the message that came in 
                else {   
                    //set the string terminating NULL byte on the end 
                    //of the data read  
                    message[valread] = '\0'; 
                    
                    // get time if first time doing the transaction
                    if (first) {
                        first = false;
                        begin = get_time();
                    }
                    
                    char hostname_pid[HOST_NAME_MAX+12];
                    int n;
                    sscanf(message, "%s %d", hostname_pid, &n);

                    string sHostname_pid = hostname_pid;

                    // record transaction numeber for each client
                    if (all_jobs.find(sHostname_pid) == all_jobs.end()) {
                        // first transaction ever
                        all_jobs[sHostname_pid] = 1;
                    } else {
                        all_jobs[sHostname_pid] += 1;
                    }
                    
                    jobNum += 1;
                    fprintf(pFile, "%10.2f: #%3d (T%d) from %s\n", get_time(), jobNum, n, hostname_pid);
                    
                    Trans(n);

                    string data = to_string(jobNum); 
                    strcpy(message, data.c_str());
                    send(sd, message, strlen(message), 0);
                    fprintf(pFile, "%10.2f: #%3d (Done) from %s\n", get_time(), jobNum, hostname_pid);

                    end = get_time();

                    // updating to 30 seconds to renew the countdown time 
                    timeout.tv_sec = 30; 
                }  
            }  
        }  
    }  

    close(master_socket);

    int total_job = 0;
    double sec; 
    fprintf(pFile, "Summary\n");

    // print job results from all clients 
    for (auto const& job : all_jobs) {
        total_job += job.second;
        fprintf(pFile, "%d transactions from %s\n", job.second, job.first.c_str());
    }

    sec = (double) total_job / (end-begin);
    fprintf(pFile, "%4.1f transactions/sec (%d/%.2f)\n", sec, total_job, end-begin);
    fclose(pFile);
    return 0;  
}  