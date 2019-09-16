// Client side by Socket programming 
#include <stdio.h> 
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
using namespace std;
using namespace std::chrono;

#define BUFFER_SIZE 10240

//Server parameters to assign and to print
struct server_para {
    //char *addr;
    string addr = "127.0.0.1";
    string hostname = "local";
    int port = 8080;
};

int num_server = 0;
string cmd = "";
struct server_para *serverlist;
string log_name = "vm";

//Connect using hotname. The sock will be used to send message
int connect_by_host(int &sock, server_para &server){   
    struct addrinfo hints = {}, *addrs;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
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
    cmd = "";
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
                    serverlist[i].hostname = strtok(NULL, delim);
                    serverlist[i].port = atoi(strtok(NULL, delim));
                }
            } catch (...){
                fclose(f);
                throw;
            }
            fclose(f);
        } else if ((string) argv[i] == "-l"){
            log_name = (string) argv[++i];
        }
    }
    //No command is given
    if (cmd == ""){
        //printf("Error: -m is required. Use -m \"grep ...\"");
        //return -1;
        cmd = "grep -E '^[0-9]*[a-z]{5}'";
        // cmd = "grep '[A-Za-z]+[0-9]*'";
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

//This is the main procedure to run on Server i
int run_cmd(int i, int lines[]){
    struct sockaddr_in serv_addr;      
    int sock;
    int valread; 
    //*** For IP address
    //init_socket_para(serv_addr, serverlist[i].addr.c_str(), serverlist[i].port);
    //if (connect_socket(sock, serv_addr)<0){return -1;}
    //cout<<"Server "<<i<<":"<<serverlist[i].addr<<":"<<serverlist[i].port<<"\n";

    //*** For texture Hostname
    if (connect_by_host(sock, serverlist[i])<0){
        return -1;
    }
    cout<<"Server "<<i<<":"<<serverlist[i].hostname<<":"<<serverlist[i].port<<"\n";

    //const char *hello = "Hello from client"; 
    //send(sock , hello , strlen(hello) , 0 ); 
    string cur_cmd = cmd + " " + log_name + to_string(i + 1) + ".log";
    send(sock, cur_cmd.c_str(), cur_cmd.length(), 0);
    printf("cmd sent %s \n ", cur_cmd.c_str()); 

    char buffer[BUFFER_SIZE] = {0}; 
    string res = "";
    string res_file_name = "result_received_" + to_string(i + 1) +".txt";
    remove(res_file_name.c_str());
    ofstream myfile;
    myfile.open ("result_received_" + to_string(i + 1) +".txt", ios::app);
    while ((valread = recv(sock , buffer, BUFFER_SIZE - 1, 0)) > 0){ 
        // printf("read=%d bits from server-%d\n",valread, i); 
        // if (valread < BUFFER_SIZE-1){
        //     char sbuff[valread];
        //     memcpy(sbuff, &buffer[0], valread - 1);
        //     sbuff[valread - 1] = '\0';
        //     res += sbuff;
        //     break;
        // }
        if (valread < BUFFER_SIZE-1){
            buffer[valread] = '\0';
        }
        res += buffer;
        
    }
    myfile << res;
    printf("Total received bytes: %lu", res.length());
    lines[i] = 0;
    if(strcmp(res.c_str(), "-1") == 0) {
        cout << "\nNo Result Found " << endl;
    } else {
        cout << "\nTotal " << count(res.begin(), res.end(), '\n') << " lines are retrieved" << std::endl;
        lines[i] = count(res.begin(), res.end(), '\n');
    }

    cout << "End of VM "<< to_string(i + 1) << endl;

    myfile.close();
    return 0;

    // printf("server-%d's msg: %s\n",i, res.c_str());
}
   
//Here we use threads to query each server individually
int main(int argc, char const *argv[]) 
{ 
    //milliseconds ms1 = duration_cast< milliseconds >(system_clock::now().time_since_epoch().count);
    long ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    //int *sock;
    //struct sockaddr_in *serv_addr;

    //Read parameters, set num_server, serverlist, cmd
    init_para(argc, argv);
    thread threads[num_server];
    int lines[num_server];

    for (int i=0;i<num_server;i++){  
        threads[i] = thread(run_cmd, i, &lines[0]);
    }
    for (int i=0;i<num_server;i++){  
        threads[i].join();
    }

    long ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    printf("======================== The result statistics: ======================== \n ");

    printf("For query %s: \n", cmd.c_str());

    for (int i=0;i<num_server;i++){  
        if(lines[i] != 0) {
            cout << "Machine vm " << serverlist[i].hostname << " has " << lines[i] << " lines of results. Stored in file result_received_" << to_string(i + 1) << ".txt" << endl;
        } else{
            cout << "Machine vm " << serverlist[i].hostname << " has no results for this search." << endl;
        } 
    }       

    cout<<"time elapse = "<<ms2-ms1<<" ms\n";

    return 0; 
} 

