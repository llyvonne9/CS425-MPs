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

int num_server = 0;
string cmd = "";
struct server_para *serverlist;
string log_name = "vm";

struct server_para myinfo;
struct server_para introducer;
struct server_para *neighbors;
int wait_time = 8000; //ms
int heartbeat_time = wait_time/4;


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

int heartbeat(string IP){	//UDP send heartbeat to IP
	int server_fd, new_server_fd;
	struct sockaddr_in servaddr;//, cliaddr;
	
	int read_status;
    int addrlen = sizeof(servaddr); 

    //struct timeval tp;
	//gettimeofday(&tp, NULL);
	//neighbors[nbr_id].check_time = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get millisecond

	server_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(server_fd == 0) {
		perror("[Error]: Fail to create socket");
		exit(1);
	}

    memset(&servaddr, 0, sizeof(servaddr)); 
    //memset(&cliaddr, 0, sizeof(cliaddr)); 

    // Filling server information
	servaddr.sin_family = AF_INET; //IPv4
	servaddr.sin_addr.s_addr = inet_addr(IP.c_str()); 
	servaddr.sin_port = htons(PORT_HB);

	//bind socket to the address
	//if(bind(server_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
	//	perror("[Error]: Fail to bind to address");
	//	exit(1);
	//}

	int n_heartbeat = 0;
	//keep listen to request
	while(true){
		if (myinfo.status==1){
			char received_info[BUFFER_SIZE] = {0}; 
			int n; socklen_t len = sizeof(servaddr);
	    	//n = recvfrom(server_fd, (char *)buffer, MAXLINE,  
	        //        MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
	        //        &len); 
	    	//received_info[n] = '\0'; 
			//printf("\nThe order received is: %s\n", received_info);
			//int nbr_id = stoi(received_info);
			//neighbors[nbr_id].check_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

			char* hello; sprintf(hello, "%d", myinfo.id);
			sendto(server_fd, (const char*) hello, strlen(hello),  
	        	MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
	            len); 
	    	printf("Heartbeat %d sent\n", n_heartbeat++);
	    }
	    std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_time));
	}
	return 0;
}

int monitor(){ //UDP monitor heartbeat
	int sockfd; 
    struct sockaddr_in	servaddr; 
  
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT_HB); 
    servaddr.sin_addr.s_addr = INADDR_ANY;
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    int n; socklen_t len = sizeof(servaddr);
	while(true){
		char buffer[BUFFER_SIZE] = {0}; 
	    n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE,  
	                MSG_WAITALL, (struct sockaddr *) &servaddr, 
	                &len); 
	    buffer[n] = '\0'; 

	    if (myinfo.status == 1){	//update only when I'm alive
			printf("\nThe order received is: %s\n", buffer);
			int nbr_id = stoi(buffer);
			for (int i=0; i++; i<NUM_NBR){	//use key or hash mapping in the future
				if (neighbors[i].id==nbr_id){
					neighbors[i].check_time = std::chrono::duration_cast<std::chrono::milliseconds>(
						std::chrono::system_clock::now().time_since_epoch()).count();
				}
				break;
			}
		}

	}  
    close(sockfd); 
    return 0; 
}

//Set parameters that read from command line and file
int init_para(int argc, char const *argv[]){
	introducer.port = PORT_INTRO;
	if (argc > 2){
		introducer.addr = (string) argv[2];
	}
	if (argc > 1){
		try {
    		myinfo.id = std::stoi(argv[1]);
		} catch (std::exception const &e) {
    	// This could not be parsed into a number so an exception is thrown.
    	// atoi() would return 0, which is less helpful if it could be a valid value.
		}
	} else {
		cout << "Need one parameter: id";
		return -1;
	}
	return 0;
}

int join(){	//send JOIN to introducer, receie NEIGHBORS
	int valread; 
    char recv_info[BUFFER_SIZE] = {0}; 

    string cmd = "JOIN_"+myinfo.id;
    send(introducer.sock, cmd.c_str(), cmd.length(), 0);
    printf("cmd sent %s \n ", cmd.c_str());

    valread = read(introducer.sock, recv_info, BUFFER_SIZE);
	printf("\nThe info received is: %s\n", recv_info); //neighbor list

	char delim[] = " ";
	char *ptr = strtok(recv_info, delim);
	for(int i=0; i++; i<NUM_NBR){
		//printf("'%s'\n", ptr); 
		int nth = stoi(strtok(NULL, delim));
		neighbors[nth].id = stoi(strtok(NULL, delim));
		neighbors[nth].status = stoi(strtok(NULL, delim));
		neighbors[nth].addr = (string) strtok(NULL, delim);
	}
	return 0;
}

int leave(){ //send leave to introducer
	int valread; 
    char recv_info[BUFFER_SIZE] = {0}; 

    string cmd = "LEAVE_"+myinfo.id;
    send(introducer.sock, cmd.c_str(), cmd.length(), 0);
    printf("cmd sent %s \n ", cmd.c_str());
    return 0;
}

int test(){
	int server_fd;
	struct sockaddr_in addr;
	
	string result;
	int read_received_message;
    int addrlen = sizeof(addr); 

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd == 0) {
		perror("[Error]: Fail to create socket");
		exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT_TEST);

	//bind socket to the address
	if(bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("[Error]: Fail to bind to address");
		exit(1);
	}

	//keep listen to request
	while(true){
		char received_info[BUFFER_SIZE] = {0}; 
		if(listen(server_fd, QUEUE_SIZE) < 0) {
			perror("[Error]: Fail to listen to incoming connections");
			exit(1);
		}
		int new_server_fd = accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addrlen);
		if(new_server_fd < 0) {
			perror("[Error]: Fail to accept to incoming connections");
			exit(1);
		} 

		//read the request
		read_received_message = read(new_server_fd, received_info, BUFFER_SIZE);
		string msg;
		printf("\nThe order received is: %s\n", received_info);
		if (strcmp(received_info,"JOIN")==0){
			join();
			myinfo.status = 1;
			msg = "OK";
		}
		if (strcmp(received_info,"LEAVE")==0){
			leave();
			myinfo.status = 1;
			close(introducer.sock);
			msg = "OK";
		}
		if (strcmp(received_info,"INFO")==0){
			//string msg = to_string(-1) + '\0';
			msg = "myinfo.id\n";
		    for (int i=0;i<NUM_NBR;i++){ 
				//sprintf(msg, "neighbors[%d],id=%d,status=%d", i, neighbors[i].id,neighbors[i].status);
				msg += "neighbors[" +to_string(i)+ "],id=" +to_string(neighbors[i].id)+ ",status=" +to_string(neighbors[i].status);
			}
		}
		send(new_server_fd, msg.c_str(), msg.length(), 0);
		close(new_server_fd);
	}
	return 0;
}

int intro_update(int sock){
    int valread; 

	while (true){
    	char recv_info[BUFFER_SIZE] = {0}; 
		valread = read(sock, recv_info, BUFFER_SIZE);
		printf("\nThe info received is: %s\n", recv_info); //neighbor leave (join might be optional)
		char delim[] = " ";
		char *ptr = strtok(recv_info, delim); 
		if (strcmp(ptr, "UPDATE")==0){
			neighbors[stoi(strtok(NULL, delim))].status = stoi(strtok(NULL, delim));
		}
	}
	return 0;
}

int main(int argc, char const *argv[]) {
	//Set id
	init_para(argc, argv);
	neighbors = new server_para[num_server];
	myinfo.status = 0; //until test says join

	//Connect to Introducer
    //int sock;
    int valread; 
    char recv_info[BUFFER_SIZE] = {0}; 

    struct sockaddr_in serv_addr; 	
	init_socket_para(serv_addr, introducer.addr.c_str(), introducer.port);
    if (connect_socket(introducer.sock, serv_addr)<0){introducer.status = -1; return -1;}
    cout<<"introducer= "<<introducer.addr<<":"<<introducer.port<<"\n";
    
    //*** For texture Hostname
    //if (connect_by_host(sock, introducer, SOCK_STREAM)<0){
    //    introducer.status = -1;
    //    return -1;
    //}
    //cout<<"introducer"<<<<": "<<introducer.hostname<<":"<introducer.port<<"\n";


	//According the test to change myinfo status
	thread thread_test;
	thread_test = thread(test);
	cout<<"fine1\n";

	//Send heartbeat to neighbors (UDP)
	thread thread_HBs[NUM_NBR];
    for (int i=0;i<num_server;i++){ 
		thread_HBs[i] = thread(heartbeat, neighbors[i].addr);
	}
	cout<<"fine2\n";

	//Create a socket and listen to heartbeats from neighbors (UDP) by thread. Count timeout for each neighbor
    thread thread_monitor;
	thread_monitor = thread(monitor);
	cout<<"fine3\n";

	//thread thread_intro_update;
	//thread_intro_update = thread(intro_update, introducer.sock);
	//cout<<"fine4\n";

	long cur_time = 0;
	while(true){
		sleep(heartbeat_time/1000);
		if (myinfo.status==1){
			bool is_changed = true;
		    for (int i=0;i<NUM_NBR;i++){ 
				cur_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				//cout<<cur_time<<"\n";
				if (cur_time - neighbors[i].check_time > wait_time){ 
					if (neighbors[i].status == 1){
						neighbors[i].status = 0;
						//string cmd = "LEAVE_"+id+"_"+i;
						char *tmp;
						sprintf(tmp,"FAIL_%d_%d",myinfo.id,neighbors[i].id);
					    send(introducer.sock, (const char *)tmp, strlen(tmp), 0);
					    printf("cmd sent %s \n ", cmd.c_str());
					    is_changed = true;
					}
				} else{
					if (neighbors[i].status == 0) {
						neighbors[i].status = 1;
						is_changed = true;
					}
				}
			}
		    ofstream myfile;
			myfile.open ("nbr_state.txt");
			myfile<<"myinfo.id"<<"\n";
		    for (int i=0;i<NUM_NBR;i++){ 
		    	//fprintf(myfile, "neighbors[%d],id=%d,status=%d", i, neighbors[i].id,neighbors[i].status);
				myfile << i <<" "<<neighbors[i].id<<" "<<neighbors[i].status<<"\n";
			}
			myfile.close();
		}
	}
	close(introducer.sock);
	return 0;
}
