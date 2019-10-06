#include <stdio.h> 
//#include <sys/time.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <netdb.h>
#include <thread>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <chrono>
using namespace std;
using namespace std::chrono;

#define BUFFER_SIZE 10240
#define QUEUE_SIZE 10
#define PORT_INTRO 8081
#define PORT_HB 8082
#define PORT_TEST 8083
#define NUM_NBR 4

//Server parameters to assign and to print
struct server_para {
    //char *addr;
    string addr = "127.0.0.1";
    string hostname = "local";
    int port = PORT_HB;
    long check_time = 0;
    int id = -1;
    int status = 1;
    int sock = -1;
};

struct server_para node;
string cmd;

//Connect using hotname. The sock will be used to send message
int connect_by_host(int &sock, server_para &server, int socktype){   
    struct addrinfo hints = {}, *addrs;
    hints.ai_family = AF_INET;
    hints.ai_socktype = socktype;
    hints.ai_protocol = IPPROTO_TCP;
    char port_str[16] = {};
    sprintf(port_str, "%d", server.port);
    const char *hostname = server.hostname.c_str();
    int err = getaddrinfo(hostname, port_str, &hints, &addrs);
    if (err != 0){
        //fprintf(stderr, "%s: %s\n", hostname, gai_strerror(err));
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    //for(struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next){
    struct addrinfo *addr = addrs;
        sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        // cout<< addr->ai_addr <<"\n";
        // cout<<"sock="<<sock<<"\n";
        if (sock<0){
            err = errno; 
            close(sock);
            printf("\n Socket creation error \n"); 
            //fprintf(stderr, "%s: %s\n", hostname, strerror(err));
            return -1;
        }
        if (connect(sock, addr->ai_addr, addr->ai_addrlen) < 0){ 
            printf("\nConnection Failed \n"); 
            return -1; 
        }
    //} 
    freeaddrinfo(addrs);
    return sock;
}

//Initialize network parameters with IPV4 adress
int init_socket_para(struct sockaddr_in &serv_addr, const char *addr, int port){  
    
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 
    //cout<<addr<<"\n";

    // Convert IPv4 addresses from text to binary form 
    if(inet_pton(AF_INET, addr, &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); //Why "localhost" not work?
        return -1; 
    }
    return 0;
}

//Connect to a server using IPV4 address
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

//Set parameters that read from command line and file
int init_para(int argc, char const *argv[]){
    if (argc != 3){
        printf("use paramer: ip port command(JOIN/LEAVE/INFO)");
    }

    node.addr = (string) argv[1];
    node.port = stoi(argv[2]);
    cmd = "TEST "+(string) argv[3];

    return 0;
}

int main(int argc, char const *argv[]) {
    //Set id
    init_para(argc, argv);

    //Connect to Introducer
    //int sock;
    int valread; 
    char recv_info[BUFFER_SIZE] = {0}; 

    struct sockaddr_in serv_addr;   
    init_socket_para(serv_addr, node.addr.c_str(), node.port);
    if (connect_socket(node.sock, serv_addr)<0){node.status = -1; return -1;}
    cout<<"node= "<<node.addr<<":"<<node.port<<"\n";

    send(node.sock, cmd.c_str(), cmd.length(), 0);
    printf("cmd sent %s \n ", cmd.c_str());

    valread = read(node.sock, recv_info, BUFFER_SIZE);
    printf("\nThe info received is: %s\n", recv_info); //neighbor status

    return 0;
}

