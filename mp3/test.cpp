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
#include <map>
#include <vector>
#include <string>
using namespace std;
using namespace std::chrono;

#define BUFFER_SIZE 10240
#define QUEUE_SIZE 10
#define PORT_INTRO 8081
#define PORT_HB 8100
#define PORT_TEST 8200
#define NUM_NBR 4

//Server parameters to assign and to print
struct server_para {
    //char *addr;
    string addr = "127.0.0.1";
    string hostname = "local";
    int port = PORT_TEST;
    long check_time = 0;
    int id = -1;
    int status = 1;
    int sock = -1;
};

struct server_para node;
string cmd;
string id_ip_file = "servers.txt";
map<int, string> id_to_ip;

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

vector<string> split (string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

//Set parameters that read from command line and file
int init_para(int argc, char const *argv[]){
    if (argc != 3 && argc != 4){
        printf("use paramer: id (ip) command(JOIN/LEAVE/INFO/store/ls)");
    }
    node.port = PORT_TEST + stoi(argv[1]);
    // node.addr = (string) argv[2];
    // cmd = "TEST "+(string) argv[3];
    printf("argc %d\n", argc);
    if(argc < 4) {
        node.addr = id_to_ip.find(stoi(argv[1]))->second;
        cmd = "TEST "+ (string) argv[2];
    } else {
        node.addr = (string) argv[2];
        string action = (string) argv[3];
        if(strcmp(action.c_str(), "JOIN") == 0 || strcmp(action.c_str(), "LEAVE") == 0 || strcmp(action.c_str(), "INFO") == 0) cmd = "TEST "+ action;
        else if(strcmp(action.c_str(), "GET") == 0) {
            cmd = "TEST " + action + " "+ (string) argv[4] + " " + (string) argv[5];
        }else if(strcmp(action.c_str(), "PUT") == 0) {
            cmd = "TEST " + action + " "+ (string) argv[4];
        }else if(strcmp(action.c_str(), "DELETE") == 0) {
            cmd = "TEST " + action + " "+ (string) argv[4];
        } else if(strcmp(action.c_str(), "ls") == 0) {
            cmd = "TEST " + action + " "+ (string) argv[4];
        }
    }
    return 0;
}

int main(int argc, char const *argv[]) {

    ifstream fin(id_ip_file); 
    const int LINE_LENGTH = 100; 
    char str[LINE_LENGTH];  
    int line = 0;
    while( fin.getline(str,LINE_LENGTH) && line < 10) {    
        vector<string> v = split(str, " ");
        id_to_ip.insert({stoi(v[0]), v[1]});
        // printf("id ip %d %s\n", stoi(v[0]), v[1].c_str());
        line++;
    }
    // cout << "pass";
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

    close(node.sock);
    return 0;
}

