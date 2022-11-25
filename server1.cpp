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
#include <vector>
#include <time.h>
#include "tands.h"
#include "helper.h"
     
#define TRUE   1 
#define FALSE  0 

struct connect_info {
    int socket;
    char hostname_pid[20];
    int trans;
};

struct summary_info {
    char hostname_pid[20];
    int trans;
};

int main(int argc , char *argv[]) {  
    int port = atoi(argv[1]);
    int opt = TRUE;  
    int master_socket , addrlen , new_socket, 
          max_clients = 30 , activity, i , valread , sd, jobNum = 0;  
    connect_info client_socket[30];
    int max_sd;  
    struct sockaddr_in address;  
    vector<summary_info> summary;
    bool first = true;
    double begin = 0, end = 0;
    struct timeval timeout;
        
    char message[1025];  //data buffer of 1K 
    
    timeout.tv_sec = 100; 
    timeout.tv_usec = 0;
    //set of socket descriptors 
    fd_set readfds;  
     
    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < max_clients; i++)  
    {  
        client_socket[i].socket = 0;  
    }  
         
    //create a master socket 
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
     
    //set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
          sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
     
    //type of socket created 
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons(port);  
         
    //bind the socket to localhost port 8888 
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Using port %d \n", port);  
         
    //try to specify maximum of 3 pending connections for the master socket 
    if (listen(master_socket, 3) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
         
    //accept the incoming connection 
    addrlen = sizeof(address);   
         
    while(TRUE)  
    {  
        //clear the socket set 
        FD_ZERO(&readfds);  
     
        //add master socket to set 
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
             
        //add child sockets to set 
        for ( i = 0 ; i < max_clients ; i++)  
        {  
            //socket descriptor 
            sd = client_socket[i].socket;  
                 
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

        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  

        if (activity == 0) {
            break;
        }
             
        //If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(master_socket, &readfds))  
        {  
            if ((new_socket = accept(master_socket, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
             
            //inform user of socket number - used in send and receive commands 
            printf("New connection");  
                 
            //add new socket to array of sockets 
            for (i = 0; i < max_clients; i++)  
            {  
                //if position is empty 
                if( client_socket[i].socket == 0 )  
                {  
                    client_socket[i].socket = new_socket; 
                    client_socket[i].hostname_pid[0] = '0'; 
                    client_socket[i].trans = 0;
                    printf("Adding to list of sockets as %d\n" , i);  
                    break;  
                }  
            }  
        }  
             
        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i].socket;  
                 
            if (FD_ISSET( sd , &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = read( sd , message, 1024)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                         
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd );  
                    client_socket[i].socket = 0; 
                    summary_info end;
                    strcpy(end.hostname_pid, client_socket[i].hostname_pid);
                    end.trans = client_socket[i].trans;
                    summary.push_back(end);
                }  
                     
                //Echo back the message that came in 
                else 
                {   
                    //set the string terminating NULL byte on the end 
                    //of the data read  
                    message[valread] = '\0'; 
                    
                    if (first) {
                        first = false;
                        begin = get_time();
                    }

                    // getting the hostname and process id
                    if (client_socket[i].hostname_pid[0] == '0') {
                        strcpy(client_socket[i].hostname_pid, message);
                        continue;
                    }

                    client_socket[i].trans += 1;
                    jobNum += 1;
                    printf("%10.2f: #%3d (%4s) from %s\n", get_time(), jobNum, message, client_socket[i].hostname_pid);
                    
                    string data = to_string(jobNum); 
                    strcpy(message, data.c_str());
                    send(sd, message, strlen(message), 0);
                    printf("%10.2f: #%3d (Done) from %s\n", get_time(), jobNum, client_socket[i].hostname_pid);

                    end = get_time();
                    timeout.tv_sec = 5; 
                }  
            }  
        }  
    }  

    close(master_socket);

    int total_job;
    double sec;

    for (i = 0; i < max_clients; i++) { 
        if (client_socket[i].hostname_pid[0] != 0) {
            summary_info end;
            strcpy(end.hostname_pid, client_socket[i].hostname_pid);
            end.trans = client_socket[i].trans;
            summary.push_back(end);
        }
    }  

    printf("Summary\n");
    for (summary_info item : summary) {
        total_job += item.trans;
        printf("%d transactions from %s\n", item.trans, item.hostname_pid);
    }

    sec = (begin-end) / (double) total_job;
    printf("%.1f transactions/sec\n", sec);
    return 0;  
}  