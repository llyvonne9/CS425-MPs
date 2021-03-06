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
#include <vector>
#include <limits.h>
#include <set>
#include <map>
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
    int port = PORT_HB;
    long check_time = 0;
    int id = -1;
    int status = 0;
    int sock = -1;
    int hb_times = 0;
};

int num_server = 0;
string cmd = "";
struct server_para *serverlist;
string log_name = "vm";

struct server_para myinfo;
struct server_para introducer;
struct server_para *neighbors;
int wait_time = 8000; //ms can use 80 for emulating msg loss
int heartbeat_time = wait_time/8;
int heartbeat_when_join = 0;
set<int> membership_list;
map<int, int> mem_hb_map;


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
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    struct addrinfo *addr = addrs;
    sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sock<0){
        err = errno; 
        close(sock);
        printf("\n Socket creation error \n"); 
        return -1;
    }
    if (connect(sock, addr->ai_addr, addr->ai_addrlen) < 0){ 
        printf("\nConnection Failed \n"); 
        return -1; 
    }
    
    freeaddrinfo(addrs);
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

int send_msg(string& msg, struct server_para server){
	int valread; 
	int sock;
    char recv_info[BUFFER_SIZE] = {0}; 

    struct sockaddr_in serv_addr; 	
	init_socket_para(serv_addr, server.addr.c_str(), server.port);
    if (connect_socket(sock, serv_addr)<0){server.status = -1; return -1;}
    cout<<"target= "<<server.addr<<":"<<server.port<<"\n";

    send(sock, msg.c_str(), msg.length(), 0);
    printf("cmd sent %s \n ", msg.c_str());

	valread = read(sock, recv_info, BUFFER_SIZE);
	recv_info[valread] = '\0';
	printf("\nThe info received is: %s\n", recv_info); //neighbor leave (join might be optional)

	msg = string(recv_info);
    close(sock);
	return 0;
}	

// send heartbeat to the neighbor with the address IP
int heartbeat(int idx){	//UDP send heartbeat to IP
	int server_fd, new_server_fd;
	struct sockaddr_in servaddr;//, cliaddr;
	
	int read_status;
    int addrlen = sizeof(servaddr); 

    int n_heartbeat = heartbeat_when_join;

	server_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(server_fd == 0) {
		perror("[Error]: Fail to create socket");
		exit(1);
	}

    memset(&servaddr, 0, sizeof(servaddr)); 

    // Filling server information
	servaddr.sin_family = AF_INET; //IPv4

	//keep listen to request

	//int maxN=1, cc [maxN], tmpc[maxN]; float msg_loss_rate = 0.3; srand( time( NULL ) ); //for msg loss emulation

	while(true){

		if (myinfo.status== 1 && neighbors[idx].status == 1){
			if(n_heartbeat == 0 && heartbeat_when_join != 0) n_heartbeat = heartbeat_when_join;
			//for msg loss emulation
			/*for (int i=0; i<maxN; i++){
			double r = ((double) rand() / (RAND_MAX)); //emulate msg loss
			if (cc[i]>0 && r>=msg_loss_rate) {cc[i]=0;}
			if (r<msg_loss_rate){cc[i]++;}
			if (cc[i]==2){
				string msg = "INFO_False alarm by "+to_string(myinfo.id)+" after "+to_string(n_heartbeat-tmpc[i]);
				send_msg(msg, introducer); cc[i]=0;tmpc[i]=n_heartbeat;
			}}*/
			
			servaddr.sin_addr.s_addr = inet_addr(neighbors[idx].addr.c_str()); 
			servaddr.sin_port = htons(PORT_HB+neighbors[idx].id);

			char received_info[BUFFER_SIZE] = {0}; 
			int n; socklen_t len = sizeof(servaddr);
	    	
			string hello = to_string(myinfo.id) + " " + to_string(n_heartbeat);
			// string hello = to_string(myinfo.id);

			string mem_list = "";
			// set<int>::iterator it;
			// for(it = membership_list.begin(); it!=membership_list.end(); it++)  {
			// 	mem_list += " " + to_string(*it) + " " + to_string(mem_hb_map.find(*));
			// }
			for(int i = 1; i < 11; i++) {
				int st = membership_list.find(i) != membership_list.end()? 1: -1;
				mem_list += " " + to_string(i) + " " + to_string(mem_hb_map.find(i)->second) + " " + to_string(st);
			}

			hello += " MEMLIST" + mem_list;

			// printf("heartbeat info: %s\n", hello.c_str());

			sendto(server_fd, hello.c_str(), hello.length(),  
	        	0, (const struct sockaddr *) &servaddr, 
	            len); 

			n_heartbeat++;
	    }
	    std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_time));
	}
	return 0;
}


// listen to neighbor's heartbeat
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
    servaddr.sin_port = htons(PORT_HB+myinfo.id); 
    servaddr.sin_addr.s_addr = INADDR_ANY;
      
    // Bind the socket with the server address 
    if ( ::bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) { 
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
			// printf("\nMonitor heartbeat from node : %s\n", buffer);
	    	char delim[] = " "; 
	    	string buffer_str = buffer;
	    	string del = " ";
	    	vector<string> v = split(buffer_str, del);
	    	// cout << "Received heartbeat info" << buffer << " " << v.size() << "\n";
	  //   	char *ptr = strtok(buffer, delim); 
			// int nbr_id = stoi(ptr);
			int nbr_id = stoi(v[0]);
			int nth_heartbeat = stoi(v[1]);
			mem_hb_map.find(nbr_id)->second = nth_heartbeat;
			for (int i=0; i<NUM_NBR; i++){	//use key or hash mapping in the future
				if (neighbors[i].id==nbr_id){
					//cout << "in this if \n";
					neighbors[i].check_time = std::chrono::duration_cast<std::chrono::milliseconds>(
						std::chrono::system_clock::now().time_since_epoch()).count();
					neighbors[i].hb_times = nth_heartbeat;
					break;
				}
			}
			// printf("Received heartbeat %s\n", buffer_str.c_str());
			set<int> tmp_mem_list;
			bool need_update = false;
			for (int i = 3; i < v.size(); i += 3) {
		        int cur_id = (stoi(v[i]));
		        int cur_hb = (stoi(v[i + 1]));
		        int cur_status = (stoi(v[i + 2]));
		        if( cur_hb > mem_hb_map.find(cur_id) -> second ) {
		        	mem_hb_map.find(cur_id)->second = cur_hb;
		        	if(cur_status == 1 && membership_list.find(cur_id) == membership_list.end()) {
		        		printf("Add %d after received a heartbeat", cur_id);
		        		membership_list.insert(cur_id);
		        		need_update = true;
		        	}
		        	if(cur_status == -1 && membership_list.find(cur_id) != membership_list.end()) {
		        		printf("Remove %d after received a heartbeat", cur_id);
		        		membership_list.erase(cur_id);
		        		need_update = true;
		        	}
		        }

		    }
		    // vector<int> diff_set(20);
		    // auto iter= set_symmetric_difference(membership_list.begin(), membership_list.end(), tmp_mem_list.begin(), tmp_mem_list.end(), diff_set.begin());
		    // diff_set.resize(iter-diff_set.begin());
		    // // printf("Diff set size %lu\n", diff_set.size());
		    // membership_list = tmp_mem_list;
		    // membership_list.insert(myinfo.id);
		    // for(int i = 0; i < NUM_NBR; i++) {
		    // 	// cout << neighbors[i].id;
		    // 	if(neighbors[i].status == 1) membership_list.insert(neighbors[i].id);
		    // 	else membership_list.erase(neighbors[i].id);
		    // }
		    if(need_update) {
		    	printf("UPDATE the membership list is: \n");
				set<int>::iterator it;
				for(it = membership_list.begin(); it!=membership_list.end(); it++)  {
					printf("Machine %d \n", *it);
				}
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
	} else {
		introducer.addr = "172.22.154.145";
		// introducer.addr = "127.0.0.1";
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

//send JOIN to introducer
int join(){	

	string msg = "JOIN_"+to_string(myinfo.id);
	send_msg(msg, introducer);

	printf("Msg after join %s\n", msg.c_str());

	string deli = " "; 
	vector<string> nbrs = split(msg, deli);
	// cout << "nb size " << to_string(nbrs.size()) << "\n";
	for(int i=0; i<NUM_NBR; i++){
			int nth = stoi(nbrs[NUM_NBR * i + 1]);
			neighbors[nth].id = stoi(nbrs[NUM_NBR * i + 2]);
			int status = stoi(nbrs[NUM_NBR * i + 3]);
			if (status == 1){
				neighbors[nth].check_time = (long) std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch()).count();
				cout<<neighbors[nth].check_time<<"\n";
			}
			neighbors[nth].status = status;
			neighbors[nth].addr = nbrs[NUM_NBR * i + 4];
			neighbors[nth].port = PORT_HB+neighbors[nth].id;
			
			cout<<nth<<" "<<neighbors[nth].id<<" "<< neighbors[nth].status <<" "<<neighbors[nth].addr<<"\n";
	}



	printf("After JOIN, the membership list including: \n");
	// for(int i = 0; i < NUM_NBR; i++) {
	// 	if(neighbors[i].status == 1) {
	// 		printf("Machine %d : ACTIVE\n", neighbors[i].id);
	// 	} else {
	// 		printf("Machine %d : INACTIVE\n", neighbors[i].id);
	// 	}
	// }
	membership_list.clear();

	for(int i = NUM_NBR * 4 + 2; i < nbrs.size() - 1; i++) {
		// if(stoi(nbrs[i]) == myinfo.id) continue;
		membership_list.insert(stoi(nbrs[i]));
		if(mem_hb_map.find(stoi(nbrs[i]))->second == -1) mem_hb_map.find(stoi(nbrs[i]))->second = 0;
		printf("Machine %s\n", nbrs[i].c_str());
	}

	printf("membershipList contains %lu machines\n", membership_list.size());

	heartbeat_when_join = stoi(nbrs[nbrs.size() - 1]);


	return 0;
}

//send leave to introducer
int leave(){ 
	
	string msg = "LEAVE_"+to_string(myinfo.id);
	send_msg(msg, introducer);

    return 0;
}

//listen to test command
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
	addr.sin_port = htons(PORT_TEST+myinfo.id);

	//bind socket to the address
	if(::bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
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
		string msg = "NOT OK";
		printf("\nThe order received is: %s\n", received_info);
		char delim[] = " ";
		char *ptr = strtok(received_info, delim); 
		if (strcmp(ptr,"TEST")==0){
			ptr = strtok(NULL, delim);
			if (strcmp(ptr,"JOIN")==0 && myinfo.status!=1){
				join();
				myinfo.status = 1;
				msg = "OK";
			}
			if (strcmp(ptr,"LEAVE")==0){
				myinfo.status = 0;
				leave();
				//close(introducer.sock);
				msg = "OK";
			}
			if (strcmp(ptr,"INFO")==0){
				//string msg = to_string(-1) + '\0';
				msg = "I'm "+to_string(myinfo.id)+". ";
			    for (int i=0;i<NUM_NBR;i++){ 
					//sprintf(msg, "neighbors[%d],id=%d,status=%d", i, neighbors[i].id,neighbors[i].status);
					msg += "neighbors[" +to_string(i)+ "],id=" +to_string(neighbors[i].id)+ ",status=" +to_string(neighbors[i].status)+"\n";
				}
			}
		}
		//if introducer sent update information about its neighbor
		if (strcmp(ptr,"UPDATE")==0){
			int nth = stoi(strtok(NULL, delim)) - 1;
			int id = stoi(strtok(NULL, delim));
			int status = stoi(strtok(NULL, delim));
			for (nth=0; nth<4; nth++){ if (id == neighbors[nth].id){break;} }
			if (status == 1 and neighbors[nth].status != 1){
				neighbors[nth].check_time = (long) std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch()).count();
				//cout<<neighbors[nth].check_time<<"\n";
			} else if (status == 0 and neighbors[nth].status == 1){ //Note introduce will not inform -1 (fail) in the current design
				neighbors[nth].check_time = 0;
			}
			neighbors[nth].status = status;
			neighbors[nth].id = id;
			printf("neighbor_node_id %d status change to %d\n", id, status);
			msg = "OK";

			if(status == 1) {
				membership_list.insert(id);
			} else {
				membership_list.erase(id);
			}
			mem_hb_map.find(id) -> second = mem_hb_map.find(id) -> second + 1;

			printf("UPDATE the membership list is: %lu \n", membership_list.size());
			set<int>::iterator it;
			for(it = membership_list.begin(); it!=membership_list.end(); it++)  {
				printf("Machine %d \n", *it);
			}
		}
		send(new_server_fd, msg.c_str(), msg.length(), 0);
		close(new_server_fd);
	}
	return 0;
}

//deal with all messages received from introducer //depleted
int intro_update(int sock){ 
    int valread; 

	while (true){
		if (myinfo.status == 1){
	    	char recv_info[BUFFER_SIZE] = {0}; 
			valread = read(sock, recv_info, BUFFER_SIZE);
	    	recv_info[valread] = '\0';
			printf("\nThe info received is: %s\n", recv_info); //neighbor leave (join might be optional)
			
			char delim[] = " ";
			char *ptr = strtok(recv_info, delim); 

			if (strcmp(ptr, "NEIGHBORS")==0){
				for(int i=0; i<NUM_NBR; i++){
					//printf("'%s'\n", ptr); 
					int nth = stoi(strtok(NULL, delim));
					neighbors[nth].id = stoi(strtok(NULL, delim));
					int status = stoi(strtok(NULL, delim));
					//if (status==1 && neighbors[nth].status!=1){
					neighbors[nth].check_time = std::chrono::duration_cast<std::chrono::milliseconds>(
							std::chrono::system_clock::now().time_since_epoch()).count();
					//}
					neighbors[nth].status = status;
					neighbors[nth].addr = (string) strtok(NULL, delim);
					cout<<nth<<" "<<status<<" "<<neighbors[nth].addr<<"\n";
				}
			}
			if (strcmp(ptr, "UPDATE")==0){
				int nth = stoi(strtok(NULL, delim));
				strtok(NULL, delim);
				int status = stoi(strtok(NULL, delim));
				neighbors[nth].status = status;

			}
		}
	}
	return 0;
}

int printMembershipList() {
	printf("UPDATE the membership list is: \n");
	set<int>::iterator it;
	for(it = membership_list.begin(); it!=membership_list.end(); it++)  {
		printf("Machine %d \n", *it);
	}
	return 0;
}

int main(int argc, char const *argv[]) {
	
	//Set id
	init_para(argc, argv);
	neighbors = new server_para[NUM_NBR];
	myinfo.status = 0; //status = leave until test says join

	//Connect to Introducer
    //int sock;
    int valread; 
    char recv_info[BUFFER_SIZE] = {0}; 
    
    //*** For texture Hostname
    //if (connect_by_host(sock, introducer, SOCK_STREAM)<0){
    //    introducer.status = -1;
    //    return -1;
    //}
    //cout<<"introducer"<<<<": "<<introducer.hostname<<":"<introducer.port<<"\n";
    for(int i = 0; i < 10; i++) {
    	if(i != myinfo.id) mem_hb_map.insert({i, -1});
    	else mem_hb_map.insert({i, myinfo.status});
    }

	//According the test to change myinfo status & accept intro info
	thread thread_test;
	thread_test = thread(test);
	// cout<<"fine1\n";

	//Send heartbeat to neighbors (UDP)
	thread thread_HBs[NUM_NBR];

    for (int i=0;i<NUM_NBR;i++){ 
		thread_HBs[i] = thread(heartbeat, i);
	}
	// cout<<"fine2\n";

	//Create a socket and listen to heartbeats from neighbors (UDP) by thread. Count timeout for each neighbor
    thread thread_monitor;
	thread_monitor = thread(monitor);
	// cout<<"fine3\n";

	long cur_time = 0;
	while(true){
		sleep(heartbeat_time/1000);
		if (myinfo.status==1){
			bool is_changed = true;
		    for (int i=0;i<NUM_NBR;i++){ 
				cur_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				if (neighbors[i].id != myinfo.id){

					//if the time since last heartbeat exceeded the wait time
					if (cur_time - neighbors[i].check_time > wait_time){ 
						if (neighbors[i].status == 1){
							cout<<cur_time<<" "<<neighbors[i].check_time<<" "<<cur_time - neighbors[i].check_time<<" "<<wait_time<<"\n";
							cout << "case 1";
							neighbors[i].status = -1;
							membership_list.erase(neighbors[i].id);
							mem_hb_map.find(neighbors[i].id) -> second = mem_hb_map.find(neighbors[i].id) -> second + 1;
							//string cmd = "LEAVE_"+id+"_"+i;
							char tmp[32] = {};
							sprintf(tmp, "FAIL_%d_%d_%d", myinfo.id, neighbors[i].id, neighbors[i].hb_times);
						    //send(introducer.sock, (const char *)tmp, strlen(tmp), 0);
							string msg = (string) tmp;
							send_msg(msg, introducer);
						    printf("cmd sent %s \n ", cmd.c_str());
						    is_changed = true;
						    printf("UPDATE the membership list (size: %lu) is: \n", membership_list.size());
							set<int>::iterator it;
							for(it = membership_list.begin(); it!=membership_list.end(); it++)  {
								printf("Machine %d \n", *it);
							}
						}
					} else{
						// If we hear heartbeat from an "inactive"node, restore its active status
						if (neighbors[i].status != 1) {
							cout << "case 2";
							neighbors[i].status = 1;
							membership_list.insert(neighbors[i].id);
							is_changed = true;
							printf("UPDATE the membership list is: \n");
							set<int>::iterator it;
							for(it = membership_list.begin(); it!=membership_list.end(); it++)  {
								printf("Machine %d \n", *it);
							}
							
						}
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
