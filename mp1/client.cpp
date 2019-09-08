// Client side by Socket programming 
#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <iostream>
#include <thread>
using namespace std;

#define PORT 8080 
#define numVM 1;

int init_socket_para(struct sockaddr_in &serv_addr){  
    
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 

    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    }
    return 0;
}

int connect_socket(int &sock, struct sockaddr_in &serv_addr){

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
      
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 

    return sock;
}
   
int main(int argc, char const *argv[]) 
{ 
    int sock = 0, valread; 
    struct sockaddr_in serv_addr;

    //Read parameters
    string cmd = "grep linux test.log";
    for (int i = 1; i < argc; i++) {
        if (((string) argv[i] == "-m")&&(((string) argv[i+1]).substr(0,4)=="grep")){
            cmd = (string) argv[++i];
        }
    }

    init_socket_para(serv_addr);
    connect_socket(sock, serv_addr);

    //const char *hello = "Hello from client"; 
    //send(sock , hello , strlen(hello) , 0 ); 
    send(sock, cmd.c_str(), cmd.length(), 0);
    printf("cmd sent\n"); 

    char buffer[1024] = {0}; 
    valread = read( sock , buffer, 1024); 
    printf("%s\n",buffer ); 
    return 0; 
} 

