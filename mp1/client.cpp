// Client side by Socket programming 
#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <iostream>
#include <thread>
#include <fstream>
using namespace std;

#define BUFFER_SIZE 10240

struct server_para {
    //char *addr;
    string addr = "127.0.0.1";
    int port = 8080;
};

int num_server = 0;
string cmd = "";
struct server_para *serverlist;

int init_socket_para(struct sockaddr_in &serv_addr, const char *addr, int port){  
    
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 
    //cout<<addr<<"\n";

    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, addr, &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); //Why "localhost" not work?
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

//Set parameters
int init_para(int argc, char const *argv[]){
    for (int i = 1; i < argc; i++) {
        if ((string) argv[i] == "-m"){
            if ((argc>i+1) && (((string) argv[i+1]).substr(0,4)=="grep")){
                cmd = (string) argv[++i];
            } else {
                printf("Error: cmd not supported. Use -m \"grep ...\"");
                return -1;
            }
        } else if ((string) argv[i] == "-s"){
            FILE *f;
            try {
                //ifstream input(argv[i+1])
                //string line;
                //getline( input, line )
                //num_server = atoi(line.c_str());
                f = fopen(argv[++i], "r");
                char line[1024];
                fgets(line, 1024, f);
                num_server = atoi(line);
                cout<<"Number of servers="<<num_server<<"\n";
                serverlist = new server_para[num_server];
                char delim[] = " \t";
                for (int i=0; i<num_server; i++){
                    fgets(line, 1024, f);
                    serverlist[i].addr = strtok(line, delim);
                    serverlist[i].port = atoi(strtok(NULL, delim));
                }
            } catch (...){
                fclose(f);
                throw;
            }
            fclose(f);
        }
    }
    //No command is given
    if (cmd == ""){
        //printf("Error: -m is required. Use -m \"grep ...\"");
        //return -1;
        cmd = "grep -E '^[0-9]*[a-z]{5}' ";
    }
    //No server is specified => do only local search
    if (num_server ==0){
        num_server = 1;
        serverlist = new server_para[num_server];
        serverlist[0].addr = "127.0.0.1";
        serverlist[0].port = 8080;
    }
    return 0;
}

void run_cmd(int i){
    struct sockaddr_in serv_addr;      
    int sock;
    int valread; 
    cout<<"Server "<<i<<":"<<serverlist[i].addr<<":"<<serverlist[i].port<<"\n";
    init_socket_para(serv_addr, serverlist[i].addr.c_str(), serverlist[i].port);
    connect_socket(sock, serv_addr);

    //const char *hello = "Hello from client"; 
    //send(sock , hello , strlen(hello) , 0 ); 
    printf("cmd before:%s\n",cmd.c_str());
    send(sock, (cmd + "vm" + to_string(i + 1) + ".log").c_str(), cmd.length(), 0);
    printf("cmd sent %s \n ", cmd.c_str()); 

    char buffer[BUFFER_SIZE] = {0}; 
    string res = "";

    ofstream myfile;
    myfile.open ("result_received_" + to_string(i + 1) +".txt");
    while ((valread = recv(sock , buffer, BUFFER_SIZE - 1, 0)) > 0){ 
        printf("read=%d bits from server-%d\n",valread, i);
        //printf("%s",buffer ); 
        if (valread < BUFFER_SIZE-1){
            char sbuff[valread];
            memcpy(sbuff, &buffer[0], valread - 1);
            sbuff[valread - 1] = '\0';
            res += sbuff;
            break;
        }

        res += buffer;
    }
    myfile << res;
    myfile.close();

    // printf("server-%d's msg: %s\n",i, res.c_str());
}
   
int main(int argc, char const *argv[]) 
{ 
    //int *sock;
    //struct sockaddr_in *serv_addr;

    //Read parameters, set num_server, serverlist, cmd
    init_para(argc, argv);
    thread threads[num_server];

    for (int i=0;i<num_server;i++){  
        threads[i] = thread(run_cmd, i);
        threads[i].join();
    }

    return 0; 
} 

