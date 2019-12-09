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
#include <sys/poll.h>
#include <cstddef>
#include <ctime>
#include <future>
#include <dirent.h>
#include <cstdio>
#include "maple_juice.h"
using namespace std;
using namespace std::chrono;

#define BUFFER_SIZE 10240
#define QUEUE_SIZE 10
#define PORT_INTRO 8081
#define PORT_MASTER 8090
#define PORT_HB 8100
#define PORT_TEST 8200
#define PORT_FILE 8300
#define PORT_MAPLE 8400
#define NUM_NBR 4
#define REPLICA 2
#define DIR_SDFS "DIR_SDFS"
#define DIR_TEMP "DIR_TEMP"
#define MIN_UPDATE_DURATION 60000
#define RESPONDE_TIMEOUT 20
#define SDFS_FILE_LIST "sdfs_file_list"

#define DEBUG_J true

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
    int mapleport = PORT_HB; 
    //int is_master = 0;
};

struct file_para{
	string name;
	set<int> nodes;
	long timestamp;
};

int master_id = 2;
int master_count = 0;

int num_server = 0;
string cmd = "";
struct server_para *serverlist;
string log_name = "vm";

struct server_para myinfo;
struct server_para introducer;
struct server_para master_server;
struct server_para *neighbors;
int wait_time = 500; //ms can use 80 for emulating msg loss
int heartbeat_time = wait_time/8;
int heartbeat_when_join = 0;
set<int> membership_list;
map<int, int> mem_hb_map;
set<string> sdfs_file_set;
map<int, set<string>> files_per_node;
map<string, file_para> file_map;
int next_id = 1;
int next_mj_id = 1;
long maple_start_time = 0;
bool is_mapling = false;
bool ready_to_juice = false;
bool is_juicing = false;
map<int, int> maple_idxs_machines;
map<int, int> maple_machines_idxs;
set<int> maple_finish_set;
set<int> juice_finish_set;
string maple_msg;
string juice_msg;
int maple_machine_num = 0;
int delete_intermediate = 0;
string output_file = "";
string prefix = "";
bool is_restore_snapshot = false;

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

// vector<string> split (string s, string delimiter) {
//     size_t pos_start = 0, pos_end, delim_len = delimiter.length();
//     string token;
//     vector<string> res;

//     while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
//         token = s.substr (pos_start, pos_end - pos_start);
//         pos_start = pos_end + delim_len;
//         res.push_back (token);
//     }

//     res.push_back (s.substr (pos_start));
//     return res;
// }

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

bool check_is_machine_alive(int id) {
	return membership_list.find(id) != membership_list.end();
}

bool check_file_exists(string file_name) {
	if(file_map.find(file_name) == file_map.end()) return false;
	else return true;
}

int send_msg(string& msg, struct server_para server){
	int valread; 
	int sock;
    char recv_info[BUFFER_SIZE] = {0}; 

    struct sockaddr_in serv_addr; 	
	init_socket_para(serv_addr, server.addr.c_str(), server.port);
    if (connect_socket(sock, serv_addr)<0){server.status = -1; return -1;}
    // cout<<"target= "<<server.addr<<":"<<server.port<<"\n";

    send(sock, msg.c_str(), msg.length(), 0);
    // printf("send to %d a cmd %s \n ", server.id, msg.c_str());

	//valread = read(sock, recv_info, BUFFER_SIZE);
	//recv_info[valread] = '\0';
	// printf("\nThe info received from %d is: %s\n", server.id, recv_info); //neighbor leave (join might be optional)
	//msg = string(recv_info);
	msg = "";
	while ((valread = read(sock, recv_info, BUFFER_SIZE)) > 0){ 
		 if (valread < BUFFER_SIZE-1){
            recv_info[valread] = '\0';
        }
        msg += recv_info;
    }

    close(sock);
	return 0;
}

int send_msg_map_reduce(string& msg, struct server_para server){
	int valread; 
	int sock;
    char recv_info[BUFFER_SIZE] = {0}; 

    struct sockaddr_in serv_addr; 	
	init_socket_para(serv_addr, server.addr.c_str(), server.mapleport);
    if (connect_socket(sock, serv_addr)<0){server.status = -1; return -1;}
    cout<<"target= "<<server.addr<<":"<<server.port<<"\n";

    send(sock, msg.c_str(), msg.length(), 0);
    printf("send to %d a cmd %s \n ", server.id, msg.c_str());

	valread = read(sock, recv_info, BUFFER_SIZE);
	recv_info[valread] = '\0';
	// printf("\nThe info received from %d is: %s\n", server.id, recv_info); //neighbor leave (join might be optional)

	msg = string(recv_info);
    close(sock);
	return 0;
}	

int add_file2node(string file_name, int id){
	if (file_name == "" || (file_name == " ")){
		return 0;
	}
	if(check_file_exists(file_name)) {
		file_map[file_name].nodes.insert(id);
	} else {
		file_para fp;
		fp.name = file_name;
		fp.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		set<int> set;
		set.insert(id);
		fp.nodes = set;
		// cout<< "first time in file_map"<<"\n";
		file_map.insert({fp.name, fp});
	}
	// cout << "machine " << id << " " << file_name<<" "<<"add file to node. okk\n";
	files_per_node[id].insert(file_name);
	return 0;
}

int re_replica(int id) {	//make sure only master calls this function
	long time1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	//set<string> file_names = files_per_node.find(id) -> second;
	set<string> file_names = files_per_node[id];
	set<string>::iterator it;
	for(it = file_names.begin(); it!=file_names.end(); it++)  {
		string file_name = *it;
		set<int> nodes = file_map[file_name].nodes;
		nodes.erase(id);
		int src_id = *nodes.begin(), trg_id=next_id;
		set<int>::iterator it;
		while (membership_list.find(trg_id) == membership_list.end() || nodes.find(trg_id) != nodes.end()){
			trg_id++;
			if (trg_id==10) {trg_id=0;}
			if (trg_id == next_id){
				cout<<"No place to store the file.";
				return -1;
			}
		}
		cout<<trg_id << "\n";
		next_id = trg_id;
		//string msg = "COPY "+ file_name + " TO "+ to_string(trg_id);
		add_file2node(file_name, trg_id);
		string msg = "SEND_DUPICATE " + file_name + " " + to_string(trg_id);
		nodes.insert(trg_id);
		file_map[file_name].nodes = nodes;
		cout<< "send "+to_string(src_id)+" a msg: "+msg + "\n";
		send_msg(msg, serverlist[src_id-1]);
	}
	long time2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	cout<<"Replicate time:"<<time2-time1<<"\n";
	return 0;
}

int master_init() {
	for(int i = 1; i < 11; i++) {
		if(membership_list.find(i) != membership_list.end()) {
			string msg = "COLLECT_SDFS";
			send_msg(msg, serverlist[i - 1]);

			vector<string> files = split(msg, " ");
			set<string> set;
			files_per_node.insert({i, set});
			for(int j = 0; j < files.size(); j++) {
				add_file2node(files[j], i);
			}
		}
	}

	string replicas = "";
	map<string, file_para>::iterator it;
	for(it = file_map.begin(); it!=file_map.end(); it++)  {
		set<int> nodes = (it -> second).nodes;
		while(nodes.size() < REPLICA) {
			for(int i = 1; i < 11; i++) {
				if(membership_list.find(i) != membership_list.end() && nodes.find(i) == nodes.end()) {
					printf("file_name:%s num of nodes:%d\n", (it->first).c_str(), (int)nodes.size());
					int sender = *nodes.begin(); 
					string msg = "SEND_DUPICATE " + it->first + " " + to_string(i);
					send_msg(msg, serverlist[sender-1]);
					if(strcmp(msg.c_str(), "OK") == 0) {
						nodes.insert(i);
						add_file2node(it->first, i);
						break;
					}
				}
			}
		}
	}

	master_server = serverlist[myinfo.id - 1];
	master_server.port = PORT_MASTER + master_server.id - 1;
	master_count++;
	cout<<"I'm the new master\n";
	return 0;


}

int node_quit_proc(int id){
	membership_list.erase(id);
	if (id==master_id){
		bool am_I_master = true;
		for (auto it: membership_list){
			if (it < myinfo.id){
				am_I_master = false;
			}
		}
		if (am_I_master){
			master_id = myinfo.id;

			master_init();
		}
	}
	printf("fail_id:%d, master_id:%d, myinfo.id:%d\n", id, master_id, myinfo.id);
	if (master_id == myinfo.id){ //I'm the master
		if (!DEBUG_J) {
			re_replica(id);
		}

		if(is_mapling && maple_machines_idxs.find(id) != maple_machines_idxs.end() && maple_finish_set.find(maple_machines_idxs.find(id) -> second) == maple_finish_set.end()) {
			while(membership_list.find(next_mj_id) == membership_list.end() || maple_machines_idxs.find(next_mj_id) != maple_machines_idxs.end()) {
				next_mj_id++;
				if(next_mj_id != 10) next_mj_id = next_mj_id % 10;
			}
			int failed_maple_index = maple_machines_idxs.find(id) -> second;
			maple_machines_idxs.erase(id);
			maple_machines_idxs.insert({next_mj_id, failed_maple_index});
			maple_idxs_machines.find(failed_maple_index) -> second = next_mj_id;
			string tmp = maple_msg + " " + to_string(failed_maple_index);
			send_msg_map_reduce(tmp, serverlist[next_mj_id - 1]);
		}

		if(is_juicing && maple_machines_idxs.find(id) != maple_machines_idxs.end() && juice_finish_set.find(maple_machines_idxs.find(id) -> second) == juice_finish_set.end()) {
			while(membership_list.find(next_mj_id) == membership_list.end() || maple_machines_idxs.find(next_mj_id) != maple_machines_idxs.end()) {
				next_mj_id++;
				if(next_mj_id != 10) next_mj_id = next_mj_id % 10;
			}
			int failed_maple_index = maple_machines_idxs.find(id) -> second;
			maple_machines_idxs.erase(id);
			maple_machines_idxs.insert({next_mj_id, failed_maple_index});
			maple_idxs_machines.find(failed_maple_index) -> second = next_mj_id;
			string tmp = juice_msg + " " +  to_string(failed_maple_index);
			send_msg_map_reduce(tmp, serverlist[next_mj_id - 1]);
		}
	}
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

			hello += " "+ to_string(master_id)+ " "+ to_string(master_count); //consensus about the current master

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
			int new_master_id =  stoi(v[v.size()-2]);
			int new_master_count =  stoi(v[v.size()-1]);
			//for (int i = 3; i < v.size(); i += 3) {
			for (int i = 3; i < v.size()-2; i += 3) {
		        int cur_id = (stoi(v[i]));
		        int cur_hb = (stoi(v[i + 1]));
		        int cur_status = (stoi(v[i + 2]));

		        if( cur_hb > mem_hb_map.find(cur_id) -> second ) {

		        	if (new_master_id != master_id && new_master_count > master_count){
		        		master_id = new_master_id;
		        		master_count = new_master_count;
		        		printf("new master is %d\n", master_id);
		        		master_server = serverlist[master_id - 1];
		        		master_server.port = PORT_MASTER + master_server.id - 1;
		        	}

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

		        		node_quit_proc(cur_id);
		        	}
		        }

		    }

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

int save_sdfs_file_list() {
	ofstream myfile;
	myfile.open (SDFS_FILE_LIST);
	set<string>::iterator it;
	for(it = sdfs_file_set.begin(); it!=sdfs_file_set.end(); it++)  {
		//printf("%s\n", (*it).c_str());
		myfile << (*it).c_str() << "\n";
	}
	myfile.close();
	return 0;	
}

int read_sdfs_file_list() {
	ifstream myfile;
	try{
		myfile.open (SDFS_FILE_LIST);
	} catch (std::exception const &e) {
		cout<<"Maybe not SDFS_FILE_LIST file yet.";
	}

	if(!myfile) {
		cout << "While opening a file an error is encountered" << endl;
		return -1;
	}
	sdfs_file_set.clear();
	while(!myfile.eof()){
		string a;
		myfile >> a;
		sdfs_file_set.insert(a);
	}
	myfile.close();
	return 0;	
}

//Set parameters that read from command line and file
int init_para(int argc, char const *argv[]){
	introducer.port = PORT_INTRO;
	if (argc >4){
		is_restore_snapshot = strcmp(argv[4], "1") == 0;
	} else {is_restore_snapshot = false;}
	if (argc >3){
		master_id = stoi(argv[3]);
	} else { master_id = 2;}
	if (argc > 2){
		introducer.addr = (string) argv[2];
	} else {
		// introducer.addr = "172.22.154.145";
		introducer.addr = "127.0.0.1";
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

	//create local dir
	try {
	    string dir = DIR_SDFS + to_string(myinfo.id);
		string cmd = "mkdir "+ dir;
		popen(cmd.c_str(), "r");
	} catch (std::exception const &e) {

	}
	try{
		string dir = DIR_TEMP + to_string(myinfo.id);
		string cmd = "mkdir "+ dir;
		popen(cmd.c_str(), "r");
	} catch (std::exception const &e) {}

	if (is_restore_snapshot) 
		read_sdfs_file_list();
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

string get_filenames_by_prefix (string pre) {
	string msg = "";
	for (map<string, file_para>::iterator it = file_map.begin(); it!=file_map.end(); ++it) {
    	string name = it->first;
    	//if(name.find(pre) ==  std::string::npos) {
    	// cout<<name.substr(0,pre.length()) << " " << "pre" << " "<< to_string(name.substr(0,pre.length()) == pre);
    	if(name.substr(0,pre.length()) == pre) {
    		// continue;
    		if(msg.length() == 0) {msg += name;}
    		else msg += " " + name;
    		cout << name << "!!!\n";
    	}
  //   	set<int>::iterator it2;
		// for(it2 = file_map[name].nodes.begin(); it2!=file_map[name].nodes.end(); it2++)  {
		// 	if(membership_list.find(*it2) != membership_list.end()) {
		// 		name += "_" + to_string(*it2);
		// 		break;
		// 	}
		// }

    	// if(msg.length() == 0) {msg += name;printf("get_filenames_by_prefix %s\n", name.c_str());}
    	// else msg += " " + name;
    	

	}
	return msg;
}


int send_file(string file_name, int sock){
	FILE *fp = fopen(file_name.c_str(), "rb");
	char buffer[BUFFER_SIZE];
	int length = 0, total_len = 0, n_sent=0;
	if(fp == NULL) {
		printf("File %s not found.\n", file_name.c_str());
	} else {
		bzero(buffer, BUFFER_SIZE);
		while((length = fread(buffer, sizeof(char), BUFFER_SIZE-1, fp)) > 0) {
			if((n_sent = send(sock, buffer, length, 0)) < 0) {
				printf("Send %s failed\n", file_name.c_str());
				break;
			}
			total_len += n_sent;
			bzero(buffer, BUFFER_SIZE);
		}
		// printf("Transfer Successfully. Total bytes: %d\n\n", total_len);
	}
	fclose(fp);
	return 0;
}

//get file action from another server with the channal sock
int get_file(string sdfs_name, int sock, bool isLocal){

	string dir = DIR_SDFS + to_string(myinfo.id);
	string file_name = isLocal? sdfs_name: dir + "/" + sdfs_name;
	FILE *fp = fopen((file_name).c_str(), "wb");
	char buffer[BUFFER_SIZE];
	int length = 0, total_len = 0;

	if(fp == NULL) {
		printf("File %s not found.\n", sdfs_name.c_str());
	} else {
		bzero(buffer, BUFFER_SIZE);
		while ((length = recv(sock , buffer, BUFFER_SIZE - 1, 0)) > 0){ 
	        fwrite(buffer, sizeof(char), length, fp);
	        bzero(buffer, BUFFER_SIZE);
	        total_len += length;
	    }
		// printf("%d bytes received. Transfer Successfully. \n\n", total_len);
	}
	fclose(fp);

    return 0;

}

//machine get file sdfs_filename from master and store as local_filename
int get(string sdfs_filename, string local_filename) {
	// to master
	printf("[FILE SERVER] get %s to local as %s\n", sdfs_filename.c_str(), local_filename.c_str());
	string msg = "GET_SDFS "+ sdfs_filename;
	send_msg(msg, master_server);
	if (msg.length() == 0){
		printf("Master server tells me no such file.");
		return -1;
	}
	int id = stoi(msg); 
	

	//to node

	msg = "GET " + sdfs_filename;

	int valread; 
	int sock;
    char recv_info[BUFFER_SIZE] = {0}; 

    struct sockaddr_in serv_addr; 	
	init_socket_para(serv_addr, serverlist[id - 1].addr.c_str(), serverlist[id - 1].port);
    if (connect_socket(sock, serv_addr)<0 || membership_list.find(id) == membership_list.end()){return -1;}

    send(sock, msg.c_str(), msg.length(), 0);
    // printf("cmd sent %s \n ", msg.c_str());

    get_file(local_filename, sock, true);

    close(sock);
    return 0;

}

//get files with the name *_
vector<string> get_(string sdfs_filename_prefix, int current_index) {
	// to master
	vector<string> names;
	string msg = "GET_SDFS_NAMES "+ to_string(myinfo.id) + " " + sdfs_filename_prefix;
	send_msg(msg, master_server);

	if (msg.length() == 0){
		printf("get_\n");
		printf("Master server tells me no such file.");
		return names;
	}

	names = split(msg, " "); 

	for (int i = 0; i < names.size(); i++) {
		get(names[i], DIR_TEMP + to_string(myinfo.id) + "/" +names[i]);
		// if(stoi(split(names[i], "_")[3]) == current_index) continue;
		/*msg = "GET " + split(names[i], "_")[0];
		int id = stoi(split(names[i], "_")[1]);

		int valread; 
		int sock;
	    char recv_info[BUFFER_SIZE] = {0}; 

	    struct sockaddr_in serv_addr; 	
		init_socket_para(serv_addr, serverlist[id - 1].addr.c_str(), serverlist[id - 1].port);
		//TODO: if the node fails after master returned the server
	    if (connect_socket(sock, serv_addr)<0 || membership_list.find(id) == membership_list.end()){return names;}

	    send(sock, msg.c_str(), msg.length(), 0);
	    printf("cmd sent %s \n ", msg.c_str());

	    get_file(names[i], sock, true);
	    close(sock);
		*/
	}

    return names;

}

int combine_results(string output) {
	printf("[Master] combine starts.\n");
	ofstream outfile;
  	outfile.open (output, ios::app);
	for(int i = 0; i < maple_machine_num; i++) {
		string tmp_file = DIR_TEMP+to_string(myinfo.id)+"/juiceoutput_" + to_string(i);
		get("juiceoutput_" + to_string(i), tmp_file); 
		
		string line;
		ifstream intermediate_output (tmp_file);
		if (intermediate_output.is_open()) {
		    while ( getline (intermediate_output,line) ) {
		    	outfile << line;
		    	printf("%s !!!\n", line.c_str());
		    }
		    
		}
		intermediate_output.close();
	}
	outfile.close();
	printf("[Master] combine ends.\n");
	return 0;
}

int delete_file(string file_name) {
	//if(remove(file_name) != 0)
    //Delete the local copy
	if (sdfs_file_set.find(file_name) != sdfs_file_set.end()){
		sdfs_file_set.erase(file_name);
    	printf("File %s is deleted successfully. \n", file_name.c_str());
	} else {
		perror( "Error deleting file" );
		return -1;
	}
	return 1;
}

// send a long msg to an built socket, WITHOUT closing the socket
int send_msg_to_sock(string &msg, int sock){
	int total_sent = 0;
	int n_sent = 0;
	//send the msg segment by segment
	while (total_sent < msg.length()){
		int n_bytes = msg.length() - total_sent < BUFFER_SIZE - 1? msg.length() - total_sent: BUFFER_SIZE - 1;
		n_sent = send(sock, msg.c_str() + total_sent, n_bytes, 0);
		total_sent += n_sent;
		//printf("n_sent=%d\n", n_sent);
	}
	//close(new_server_fd);
	return 0;
}

int master() {
	//struct
	// map<string, file_para> file_map;
	int server_fd, new_server_fd;
	struct sockaddr_in addr;
	string delimiter = "_";
	
	int read_received_message;
    int addrlen = sizeof(addr); 

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd == 0) {
		perror("[Error]: Fail to create socket");
		exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT_MASTER + myinfo.id - 1);

	//bind socket to the address
	if(::bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("[Error]: Fail to bind to address");
		exit(1);
	}

	while(true) {
		if(master_id == myinfo.id && myinfo.status == 1) {
			
			char received_info[BUFFER_SIZE] = {0}; 
			if(listen(server_fd, QUEUE_SIZE) < 0) {
				perror("[Error]: Fail to listen to incoming connections");
				exit(1);
			}
			new_server_fd = accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addrlen);
			if(new_server_fd < 0) {
				perror("[Error]: Fail to accept to incoming connections");
				exit(1);
			}
			read_received_message = read(new_server_fd, received_info, BUFFER_SIZE);
			printf("\n[MASTER] The order received is: %s\n", received_info);
			string received_str = received_info;
			vector<string> received_info_vec = split(received_str, " ");
			string msg = "";
			if(strcmp(received_info_vec[0].c_str(), "GET_SDFS") == 0) {
				string file_name = received_info_vec[1];
				string msg = "";

				if(!check_file_exists(file_name)) {
					msg = "";
					printf("[MASTER GET_SDFS] The file does not exit.\n");
				} else {

					// msg = to_string(*file_map[file_name].nodes.begin());
					set<int>::iterator it;
					for(it = file_map[file_name].nodes.begin(); it!=file_map[file_name].nodes.end(); it++)  {
						if(membership_list.find(*it) != membership_list.end()) {
							msg = to_string(*it);
							break;
						}
					}
					// printf("[MASTER GET_SDFS] master to node. msg: %s\n", msg.c_str());
				}
				send(new_server_fd, msg.c_str(), msg.length(), 0);
				close(new_server_fd);

			} else if(strcmp(received_info_vec[0].c_str(), "PUT_SDFS") == 0) {
				// printf("PUT_SDFS\n");
				string file_name = received_info_vec[1];
				if(!check_file_exists(file_name)) {
					// printf("File does not exist\n");
					//set<int> nodes;
					for(int i = 0; i < REPLICA; i++) {
						while(membership_list.find(next_id) == membership_list.end() || (file_map.find(file_name)->second).nodes.find(next_id) != (file_map.find(file_name)->second).nodes.end()) {
							next_id++;
							if(next_id != 10) next_id = next_id % 10;
						}
						//nodes.insert(next_id);
						add_file2node(file_name, next_id);
						
						if(i > 0) msg += " " + to_string(next_id);
						else msg = to_string(next_id);
						// printf("Replica %d\n", next_id);
						next_id++;
						if(next_id != 10) next_id = next_id % 10;
					}
					// printf("[Master] Put to node %s\n", msg.c_str());
					
					send(new_server_fd, msg.c_str(), msg.length(), 0);
					close(new_server_fd);
				}  else {
					//confirmation about update
					//update
					printf("[MASTER PUT] File exists. \n");
					long cur_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					long confirmed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					string update_confirm_msg = "LESS1MIN";
					if((cur_time - file_map[file_name].timestamp) < MIN_UPDATE_DURATION ) {
						printf("Less than 1 min. Wait user to confirm \n");
						send(new_server_fd, update_confirm_msg.c_str(), update_confirm_msg.length(), 0);
						struct pollfd fd;
						int ret;
						fd.fd = new_server_fd; //  socket handler 
						fd.events = POLLIN;
						ret = poll(&fd, 1, RESPONDE_TIMEOUT * 1000); // 30 second for timeout
						char confirm_info[BUFFER_SIZE] = {0};
						switch (ret) {
						    case -1:
						        // Error
						    	printf("case 0\n");
						        break;
						    case 0:
						        // Timeout
						    	printf("Timeout. Update denied. \n");
								recv(new_server_fd,confirm_info,sizeof(confirm_info), 0);
								update_confirm_msg = "TIMEOUT";
						        break;
						    default:
						        recv(new_server_fd,confirm_info,sizeof(confirm_info), 0); // get  data
						        update_confirm_msg = confirm_info;
						        printf("Master received confimation %s\n", update_confirm_msg.c_str());
						        break;
						}


					} 

					if(strcmp(update_confirm_msg.c_str(), "YES") == 0) {
						string replicas = "";
						set<int>::iterator it;
						for(it = file_map[file_name].nodes.begin(); it!=file_map[file_name].nodes.end(); it++)  {
							if(replicas.length() == 0) replicas += to_string(*it);
							else replicas += " " + to_string(*it);
						}
						printf("[MASTER Update] Master sent msg: %s\n", replicas.c_str());
						msg = replicas;
						
					} else {
						
						printf("[MASTER Update] master denied with msg: %s\n", update_confirm_msg.c_str());
						msg = "NO";
					}

					send(new_server_fd, msg.c_str(), msg.length(), 0);
					// printf("Master sent %s\n", msg.c_str());
					close(new_server_fd);
					
				}

			} else if(strcmp(received_info_vec[0].c_str(), "DELETE_SDFS") == 0 
					|| strcmp(received_info_vec[0].c_str(), "LS_SDFS") == 0) {
				string file_name = received_info_vec[1];
				if(!check_file_exists(file_name)) {

					msg = "";
				} else {
					string replicas = "";
					set<int>::iterator it;
					for(it = file_map[file_name].nodes.begin(); it!=file_map[file_name].nodes.end(); it++)  {
						printf("%d\n", *it);
						if(replicas.length() == 0) replicas += to_string(*it);
						else replicas += " " + to_string(*it);
					}
					msg = replicas;

					if(strcmp(received_info_vec[0].c_str(), "DELETE_SDFS") == 0) {
						std::map<string,file_para>::iterator map_it;
						map_it=file_map.find(file_name);
		  				file_map.erase (map_it);
					}
					
				}
				send(new_server_fd, msg.c_str(), msg.length(), 0);
				close(new_server_fd);
				
			}else if(strcmp(received_info_vec[0].c_str(), "GET_SDFS_NAMES") == 0) {
				//received_info_vec[1] is from which machine id
				string file_name = received_info_vec[2];
				msg = get_filenames_by_prefix(file_name);
				send_msg_to_sock(msg, new_server_fd);
				//send(new_server_fd, msg.c_str(), msg.length(), 0);
				close(new_server_fd);
			}else if (strcmp(received_info_vec[0].c_str(), "M_SAVE_SNAPSHOT")==0){
				for (auto i : membership_list){
					msg = "F_SAVE_SNAPSHOT";
					send_msg(msg, serverlist[i-1]);
					if (msg != "OK"){
						break;
					}
				}
				send(new_server_fd, msg.c_str(), msg.length(), 0);
				close(new_server_fd);

			}else if (strcmp(received_info_vec[0].c_str(), "M_LOAD_SNAPSHOT")==0){
				for (auto i : membership_list){
					msg = "F_LOAD_SNAPSHOT";
					send_msg(msg, serverlist[i-1]);
					if (msg != "OK"){
						break;
					}
				}
				//msg = "OK"; msg would be modified as "OK" from file_server return
				send(new_server_fd, msg.c_str(), msg.length(), 0);
				close(new_server_fd);
				
			}else if(strcmp(received_info_vec[0].c_str(), "MAPLE_SDFS") == 0) {
				printf("[MASTER] MAPLE_SDFS\n");
				maple_finish_set.clear();

				msg = "OK";
				send(new_server_fd, msg.c_str(), msg.length(), 0);
				close(new_server_fd);
				
				maple_msg = "";
				string exe = received_info_vec[1];
			    maple_machine_num = stoi(received_info_vec[2]);
			    prefix = received_info_vec[3];
		        string input_file = received_info_vec[4];

			    maple_start_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			    is_mapling = true;
			    maple_msg = "MAPLE_EXE " + exe + " " + to_string(maple_machine_num) + " " + prefix + " " + input_file ;
			    // printf("maple_msg : %s\n", maple_msg.c_str());
			    for(int i = 0; i < maple_machine_num; i++) {
			        while(membership_list.find(next_mj_id) == membership_list.end()) {
						next_mj_id++;
						if(next_mj_id != 10) next_mj_id = next_mj_id % 10;
					}
					string tmp = maple_msg + " " + to_string(i);
					// printf("Master to machine %d to maple msg: %s\n", next_mj_id, tmp.c_str());
					send_msg_map_reduce(tmp, serverlist[next_mj_id - 1]);

					maple_idxs_machines.insert({i, next_mj_id});
					maple_machines_idxs.insert({next_mj_id, i});
					next_mj_id++;

			    }
					
			        
			}  else if(strcmp(received_info_vec[0].c_str(), "MAPLE_FINISH") == 0) {
				int m_id = stoi(received_info_vec[1]);
				maple_finish_set.insert(m_id);

				if(maple_finish_set.size() >= maple_machine_num) {
					ready_to_juice = true;
					is_mapling = false;
					printf("[MASTER] Ready to reduce.");

				}
				string msg = "OK, good job!";
				send(new_server_fd, msg.c_str(), msg.length(), 0);
				close(new_server_fd);
			} else if(strcmp(received_info_vec[0].c_str(), "JUICE_SDFS") == 0) {
				// std::string("JUICE_SDFS ") + para_exe + " "+ para_num + " "+ para_prefix + " "+ para_output + " "+ para_delete;
				juice_finish_set.clear();
				string para_exe = received_info_vec[1];
				string para_num = received_info_vec[2];
				string para_prefix = received_info_vec[3];
			    output_file = received_info_vec[4];
			    delete_intermediate = stoi(received_info_vec[5]);

			    juice_msg = "JUICE_EXE " + para_exe + " " + para_prefix + " " + output_file + " " + to_string(maple_machine_num) + " " + to_string(delete_intermediate);

			    if(maple_idxs_machines.size() != 0) {
			    	set<int>::iterator it;
				    for(int i = 0; i < maple_machine_num; i++) {
				        int juice_id = maple_idxs_machines.find(i) ->second;
				        string tmp = juice_msg + " " + to_string(i);
						send_msg_map_reduce(tmp, serverlist[juice_id - 1]);
				    }
			    } else {
			    	for(int i = 0; i < maple_machine_num; i++) {
				        while(membership_list.find(next_mj_id) == membership_list.end()) {
							next_mj_id++;
							if(next_mj_id != 10) next_mj_id = next_mj_id % 10;
						}
						string tmp = maple_msg + " " + to_string(i);
						// printf("Master to machine %d to maple msg: %s\n", next_mj_id, tmp.c_str());
						send_msg_map_reduce(tmp, serverlist[next_mj_id - 1]);

						maple_idxs_machines.insert({i, next_mj_id});
						maple_machines_idxs.insert({next_mj_id, i});
						next_mj_id++;

				    }

			    }
			    
			    close(new_server_fd);
					
			        
			}  else if(strcmp(received_info_vec[0].c_str(), "JUICE_FINISH") == 0) {
				int j_id = stoi(received_info_vec[1]);
				juice_finish_set.insert(j_id);

				if(juice_finish_set.size() >= maple_machine_num) {
					combine_results(output_file);

					cout << "JOICE finished";

					is_juicing = false;
					if(delete_intermediate == 1) {
						//delete maple output
						string file_names = get_filenames_by_prefix(prefix);
						vector<string> files = split(file_names, " ");
						for(int i = 0; i < files.size(); i++) 
							delete_file(files[i]);

							//delete reduce output
						file_names = get_filenames_by_prefix("juiceoutput_");
						files = split(file_names, " ");
						for(int i = 0; i < files.size(); i++) 
							delete_file(files[i]);
					}

				}
				close(new_server_fd);

			}
			
		}

	}
}




int send_dup(string file_name, int id) {
	int valread; 
	int sock;
	char recv_info[BUFFER_SIZE] = {0}; 

	struct sockaddr_in serv_addr; 	
	init_socket_para(serv_addr, serverlist[id - 1].addr.c_str(), serverlist[id - 1].port);
	if (connect_socket(sock, serv_addr)<0 || membership_list.find(id) == membership_list.end()){
	    return -1;
	}

	string msg = "RECV_DUPICATE " + file_name;
	send(sock, msg.c_str(), msg.length(), 0);
	printf("cmd sent %s \n ", msg.c_str());

	char buffer[BUFFER_SIZE] = {0}; 

	if (read(sock , buffer, BUFFER_SIZE) < 0){
		perror("file put init error");
		return -1;
	}
	string dir_sdfs = DIR_SDFS + to_string(myinfo.id);
	int total_sent = send_file(dir_sdfs +"/" + file_name, sock);

	close(sock);
	return 0;
}


static std::string getAnswer()
{    
    std::string answer;
    std::cin >> answer;
    return answer;
}

int put(string local_file, string target_file) {
	// to master
	string msg = "PUT_SDFS "+target_file;
	// printf("[FILE SERVER] put %s to SDFS as %s\n", local_file.c_str(), target_file.c_str());

	int valread; 
	int sock_confim;
    char recv_info[BUFFER_SIZE] = {0}; 

    struct sockaddr_in serv_addr; 	
    // printf("Master id %d\n", master_id);
    // printf("Master %s %d\n", master_server.addr.c_str(), master_server.port);
	init_socket_para(serv_addr, master_server.addr.c_str(), master_server.port);
    if (connect_socket(sock_confim, serv_addr)<0){master_server.status = -1; return -1;}

    send(sock_confim, msg.c_str(), msg.length(), 0);
    // printf("cmd sent %s \n ", msg.c_str());

	valread = read(sock_confim, recv_info, BUFFER_SIZE);
	recv_info[valread] = '\0';
	// printf("\nThe info received from master is: %s\n", recv_info); //neighbor leave (join might be optional)

	msg = string(recv_info);
	// printf("(Put) Node received info %s\n", msg.c_str());

	char delim[] = " ";
	// char *dup = strdup(msg.c_str());
	char *ptr = strtok(recv_info, delim); 
	// free(dup);
	if (strcmp(ptr, "LESS1MIN") == 0){
		string YESNO = "";
		cout << "The update is less than one min. Do you want to continue?(YES/NO)"<< std::endl << std::flush;

		struct pollfd poller;
		poller.fd = STDIN_FILENO;
		poller.events = POLLIN;
		poller.revents = 0;

		int rc = poll(&poller, 1, RESPONDE_TIMEOUT * 1000 + 10);  // Poll one descriptor with a five second timeout
		if (rc < 0)
		    perror("select");
		else if (rc == 0)
		{
		    // Timeout
		    printf("Timeout\n");
		}
		else
		{
			cin >> YESNO;
		    // There is input to be read on standard input
		}
		send(sock_confim, YESNO.c_str(), YESNO.length(), 0);
		if(YESNO.length() == 0) printf("Timeout. PUT denied. \n");
		printf("User's response %s\n", YESNO.c_str());
		if (YESNO == "NO"){	//either timeout or cin is NO.
			printf("User withdraw update. \n");
			close(sock_confim);
			return 0;
		} else if(YESNO == "") {
			printf("User didn't response. Timeout. \n");
			close(sock_confim);
			return 0;
		} else{
			// ptr = strtok(NULL, delim); 
			char recv_info2[BUFFER_SIZE] = {0}; 
			read(sock_confim, recv_info, BUFFER_SIZE);
			msg = "";
			msg = recv_info;
			printf("[FILE SERVER] User confirm action %s\n", msg.c_str());
			
		}
	} 
	
	close(sock_confim);

	int put_count = 0;
	
	// ids = [stoi(ptr), stoi(strtok(NULL,delim)), stoi(strtok(NULL,delim))]; //3 other copies, revise this later
	vector<string> ids = split(msg, " ");
	//to node
	msg = "PUT " + target_file;

	for (int i=0; i<REPLICA; i++){
		// printf("Put to %s\n", ids[i].c_str());
		int valread; 
		int sock;
	    char recv_info[BUFFER_SIZE] = {0}; 

	    struct sockaddr_in serv_addr; 	
		init_socket_para(serv_addr, serverlist[stoi(ids[i]) - 1].addr.c_str(), serverlist[stoi(ids[i]) - 1].port);
	    if (connect_socket(sock, serv_addr)<0 || membership_list.find(stoi(ids[i])) == membership_list.end()){
	    	return -1;
	    }

	    send(sock, msg.c_str(), msg.length(), 0);
	    // printf("cmd sent %s \n ", msg.c_str());

		char buffer[BUFFER_SIZE] = {0};
		// printf("(PUT) send file\n"); 

		valread = read(sock , buffer, BUFFER_SIZE) < 0;

		// printf("(PUT)received %s before sending file\n", buffer);
		// string dir_sdfs = DIR_SDFS + to_string(myinfo.id);
		int total_sent = send_file(local_file, sock);
		put_count++;

	    close(sock);
	}
	
	// myfile.close();
	return put_count;
}


int ls(string file_name) {
	printf("[node to master]ls:\n");
	string msg = "LS_SDFS "+file_name;
	printf("msg: %s\n", msg.c_str());
	printf("Master server: %d %d\n", master_server.id, master_server.port);
	send_msg(msg, master_server);
	vector<string> nodes = split(msg, " ");
	if(msg.size() == 0 || nodes.size() == 0 ) {
		printf("%s is not in the system. \n", file_name.c_str());
		return 0;
	}
	printf("Machines store %s are: \n", file_name.c_str());
	for(int i = 0; i < nodes.size(); i++) {
		printf("Machine %s\n", nodes[i].c_str());
	}
	return 0;
}


int store() {
	set<string>::iterator it;
	printf("Result of store: \n");
	for(it = sdfs_file_set.begin(); it!=sdfs_file_set.end(); it++)  {
		printf("%s\n", (*it).c_str());
	}
	printf("\n");
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
		printf("\n[TEST] The order received is: %s\n", received_info);
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
			if(strcmp(ptr, "GET") == 0) {
				long get_start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				ptr = strtok(NULL, delim);
				string sdfs_file = (string) ptr;
				ptr = strtok(NULL, delim);
				string local_file = (string) ptr;
				get(sdfs_file, local_file);
				msg = "OK";
				long get_end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				printf("GET used time %lu\n", (get_end - get_start));
			}
			if(strcmp(ptr, "DELETE") == 0) {
				ptr = strtok(NULL, delim);
				string target_file = (string) ptr;
				//delete_file(target_file); //delete local copy => This can be done when server notices it.
				
				//tell master to delete file
				msg = "DELETE_SDFS " + target_file;
				send_msg(msg, master_server);
				if(msg.length() == 0) printf("%s does not exist in the system.\n", target_file.c_str());
				else {
					string cmd = "DELETE " + target_file;
					vector<string> nodes = split(msg, " ");
					int ok_count = 0;
					for(int i = 0; i < nodes.size(); i++) {
						int id = stoi(nodes[i]);
						cmd = "DELETE " + target_file;
						if(id != myinfo.id) {
							send_msg(cmd, serverlist[id - 1]);
							if(strcmp(cmd.c_str(), "OK") == 0) ok_count++;
						} else {
							string dir = DIR_SDFS + to_string(myinfo.id);
							string cmd = "rm "+ dir + "/" + target_file;
							popen(cmd.c_str(), "r");
							sdfs_file_set.erase(target_file);
							ok_count++;
						}
						
					}
					printf("Deleted %d copies\n", ok_count);
					if(ok_count == REPLICA) msg = "OK";
					else msg = "NOT OK";
				}

			}
			if(strcmp(ptr, "PUT") == 0) {
				long put_start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				ptr = strtok(NULL, delim);
				string local_file = (string) ptr;
				printf("%s\n", local_file.c_str());
				ptr = strtok(NULL, delim);
				string sdfs_file = (string) ptr;
				printf("%s\n", sdfs_file.c_str());
				string sfds_dir = DIR_SDFS + to_string(myinfo.id);
				FILE *fp = fopen(local_file.c_str(), "rb");
				if (fp == NULL) {msg = "NOT OK"; printf("File not found\n");}
				else{
					int put_count = put(local_file, sdfs_file);

					if(put_count == REPLICA) {
						msg = "OK";
						printf("PUT successfully\n");
					}
					else {
						msg = "NOT OK";
						printf("PUT fail\n");
					}

					long put_end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					printf("Put used %lu\n", (put_end - put_start));
				}
			} 

			if(strcmp(ptr, "STORE") == 0) {
				store();
				msg = "OK";
			} 

			if(strcmp(ptr, "SAVE_SNAPSHOT") == 0){
				msg = "M_SAVE_SNAPSHOT";
				send_msg(msg, master_server);
				//msg would be modified as "OK" from Master
			}
			if(strcmp(ptr, "LOAD_SNAPSHOT") == 0){
				msg = "M_LOAD_SNAPSHOT";
				send_msg(msg, master_server);
			}

			if(strcmp(ptr, "LS") == 0) {
				printf("node TEST LS\n");
				ptr = strtok(NULL, delim);
				string file_name = (string) ptr;
				ls(file_name);
				msg = "OK";
			}

			if(strcmp(ptr, "MAPLE") == 0) {
				ptr = strtok(NULL, delim);
				string para_exe = (string) ptr;
				ptr = strtok(NULL, delim);
				string para_num = (string) ptr;
				ptr = strtok(NULL, delim);
				string para_prefix = (string) ptr;
				ptr = strtok(NULL, delim);
				string para_input = (string) ptr;
				string tmp = std::string("MAPLE_SDFS ") + para_exe + " "+ para_num + " "+ para_prefix + " "+ para_input;
				printf("[MAPLE] The msg to master is: %s\n", tmp.c_str());
				send_msg(tmp, master_server);
				msg = "OK";
				printf("server sent OK\n");
			}

			if(strcmp(ptr, "JUICE") == 0) {
				ptr = strtok(NULL, delim);
				string para_exe = (string) ptr;
				ptr = strtok(NULL, delim);
				string para_num = (string) ptr;
				ptr = strtok(NULL, delim);
				string para_prefix = (string) ptr;
				ptr = strtok(NULL, delim);
				string para_output = (string) ptr;
				ptr = strtok(NULL, delim);
				string para_delete = (string) ptr;
				string tmp = received_info;
				string tmp2 = std::string("JUICE_SDFS ") + para_exe + " "+ para_num + " "+ para_prefix + " "+ para_output + " "+ para_delete;
				send_msg(tmp2, master_server);
				msg = "OK";
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
				node_quit_proc(id);
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

int send_file_names(int sock) {
	string msg = "";
	set<string>::iterator it;
	for(it = sdfs_file_set.begin(); it!=sdfs_file_set.end(); it++)  {
		if(msg.length() == 0) msg += *it;
		else msg += " " + *it;
	}
	cout<<msg<<"\n";
	send(sock, msg.c_str(), msg.length(), 0);
	return 0;
}

int file_server() {
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
	addr.sin_port = htons(PORT_FILE + myinfo.id);

	//bind socket to the address
	if(::bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("[Error]: Fail to bind to address");
		exit(1);
	}

	

	while(true){
		int new_server_fd;
		char received_info[BUFFER_SIZE] = {0}; 
		if(listen(server_fd, QUEUE_SIZE) < 0) {
			perror("[Error]: Fail to listen to incoming connections");
			exit(1);
		}
		new_server_fd = accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addrlen);
		if(new_server_fd < 0) {
			perror("[Error]: Fail to accept to incoming connections");
			exit(1);
		}
		//read the request
		read_received_message = read(new_server_fd, received_info, BUFFER_SIZE);
		printf("[FILE SERVER]The order received is: %s\n", received_info);
		vector<string> received_vector = split(received_info, " ");
		string file_name = (received_vector.size()>1)? received_vector[1]: "WHATSUP";

		if(strcmp(received_vector[0].c_str(),"DELETE")==0) {
			//delete the file
			string dir = DIR_SDFS + to_string(myinfo.id);
			string cmd = "rm "+ dir + "/" + file_name;
			popen(cmd.c_str(), "r");
			string msg = "OK";
			send(new_server_fd, msg.c_str(), msg.length(), 0);
			sdfs_file_set.erase(file_name);
			printf("%s is deleted successfully\n\n", file_name.c_str());
		} else if(strcmp(received_vector[0].c_str(),"GET")==0) {
			string msg = "OK";
			send(new_server_fd, msg.c_str(), msg.length(), 0);
			string dir = DIR_SDFS + to_string(myinfo.id);
			send_file(dir + "/" + file_name, new_server_fd);
		} else if(strcmp(received_vector[0].c_str(),"PUT")==0) {
			string msg = "OK";
			send(new_server_fd, msg.c_str(), msg.length(), 0);
			get_file(received_vector[1], new_server_fd, false);
    		sdfs_file_set.insert(received_vector[1]);
		} else if(strcmp(received_vector[0].c_str(),"SEND_DUPICATE")==0) {
			send_dup(received_vector[1], stoi(received_vector[2]));
			string msg = "OK";
			send(new_server_fd, msg.c_str(), msg.length(), 0);
		} else if(strcmp(received_vector[0].c_str(),"RECV_DUPICATE")==0) {
			string msg = "OK";
			send(new_server_fd, msg.c_str(), msg.length(), 0);
			sdfs_file_set.insert(received_vector[1]);
			get_file(received_vector[1], new_server_fd,false);
		} else if(strcmp(received_vector[0].c_str(),"COLLECT_SDFS")==0) {
			send_file_names(new_server_fd);
			cout<<"file names sent\n";
		} else if(strcmp(received_vector[0].c_str(),"F_SAVE_SNAPSHOT")==0) {
			save_sdfs_file_list();
			string msg = "OK";
			send(new_server_fd, msg.c_str(), msg.length(), 0);
		} else if(strcmp(received_vector[0].c_str(),"F_LOAD_SNAPSHOT")==0) {
			read_sdfs_file_list();
			string msg = "OK";
			send(new_server_fd, msg.c_str(), msg.length(), 0);
		} 

		close(new_server_fd);
	}
	
	return 0;
}

int map_reduce() {
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
	addr.sin_port = htons(PORT_MAPLE + myinfo.id);

	//bind socket to the address
	if(::bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("[Error]: Fail to bind to address");
		exit(1);
	}

	

	while(true){
		sleep(heartbeat_time/1000);
		int new_server_fd;
		char received_info[BUFFER_SIZE] = {0}; 
		if(listen(server_fd, QUEUE_SIZE) < 0) {
			perror("[Error]: Fail to listen to incoming connections");
			exit(1);
		}
		new_server_fd = accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addrlen);
		if(new_server_fd < 0) {
			perror("[Error]: Fail to accept to incoming connections");
			exit(1);
		}
		//read the request
		read_received_message = read(new_server_fd, received_info, BUFFER_SIZE);
		printf("[MapReduce] The order received is: %s\n", received_info);
		vector<string> received_vector = split(received_info, " ");

		if(strcmp(received_vector[0].c_str(),"MAPLE_EXE")==0) {
			string exeFile = received_vector[1];
			int maple_task_num = stoi(received_vector[2]);
			string maple_prefix = received_vector[3];
			string input = received_vector[4];
			int current_id = stoi(received_vector[5]);
			string dir = DIR_TEMP + to_string(myinfo.id) +'/';
			
			string msg = "Maybe OK";
			send(new_server_fd, msg.c_str(), msg.length(), 0);
			close(new_server_fd);

			printf("try get file to local for maple\n");
			//if(sdfs_file_set.find(input) == sdfs_file_set.end()) 
				get(input, dir+input);
			printf("finished get\n");

			int line = 0;
			ifstream input_file;
			input_file.open(dir + input,ios::in); //open a file to perform read operation using file object
			set<string> files;
			
		   	if (input_file.is_open()){   //checking whether the file is open
		   		printf("file %s is open to maple\n", input.c_str());

		      	string tp;
		    	string ten_lines = "";
		    	while(getline(input_file, tp)){ //read data from file object and put it into string.
		      		if(line % 10 == 0 && (line / 10) % maple_task_num == current_id && line / 10 != 0) {
		      			set<string> part_files = maple(exeFile, ten_lines, maple_prefix, maple_task_num, current_id, dir);
		      			set<string> tmp;
		      			std::set_union(part_files.begin(), part_files.end(), files.begin(), files.end(), std::inserter(tmp, tmp.begin()));
		      			files = tmp;
		      			ten_lines = "";
		      		}
		        	if( (line / 10) % maple_task_num == current_id ) {
		         		ten_lines += tp;
		        	}
		        	line++;

		      	}

		      	//maple_output_targetmachineid_key_machineid

		    	//find all local files with the prefix maple_output_
				// DIR *dp;
		  //   	struct dirent *dirp;
		    	
		  //   	if((dp = opendir((dir).c_str())) == NULL) {
		  //     		cout << "Error(" << errno << ") opening " << (dir) << endl;
		  //     		return errno;
		  //   	}
		  //  		while ((dirp = readdir(dp)) != NULL) {
		  //   		string name = dirp->d_name;
		  //   		// if(name.find(maple_prefix) != std::string::npos && stoi(split(name, "_")[1]) != current_id) {
		  //   		if(name.find(maple_prefix) != std::string::npos) {
		  //   			// printf("%s\n", dirp->d_name);
				// 		files.push_back(string(dirp->d_name));
				// 	}
		  //   	}
		  //   	closedir(dp);


		    	printf("MAPLE FINISHED YEAH.\n");

		    	//put all maple results into SDFS
		    	for(string file: files) {
		    		
		    		put(dir + file, file);
		    	}

		    	printf("MAPLE PUT FINISHED YEAH.\n");

		    	string tmp = "MAPLE_FINISH " + to_string(current_id);
		    	send_msg(tmp, master_server);
		    	input_file.close(); //close the file object.
		   } 
			
		} else if(strcmp(received_vector[0].c_str(),"JUICE_EXE")==0) {
			string dir = DIR_SDFS + to_string(myinfo.id);
			string msg = "Maybe OK";
			send(new_server_fd, msg.c_str(), msg.length(), 0);
			close(new_server_fd);
 			// "JUICE_EXE " + para_exe + " " + para_prefix + " " + output_file + to_string(maple_machine_num);

			string exeFile = received_vector[1];
			string juice_prefix = received_vector[2];
			string output = received_vector[3];
			int maple_task_num = stoi(received_vector[4]);
			int delete_or_not = stoi(received_vector[5]);
			int current_id = stoi(received_vector[6]);

			vector<string> files = get_(juice_prefix + "_" + received_vector[5], current_id);

			juice(exeFile, files, "juiceoutput_" + to_string(current_id), dir, DIR_TEMP + to_string(myinfo.id));

			put(DIR_TEMP + to_string(myinfo.id) + '/' + "juiceoutput_" + to_string(current_id) ,  "juiceoutput_" + to_string(current_id));
			string tmp = "JUICE_FINISH " + to_string(current_id);
			send_msg(tmp, master_server);

			printf("JUICE FINISHED YEAH!\n");

			if(delete_or_not == 1) {
				// remove the dir_tmp folder
				printf("Start to delete. \n");
				string delete_command = std::string("rm -r ") + DIR_TEMP;
				try {
					string cmd = "mkdir "+ dir;
					popen(cmd.c_str(), "r");
				} catch (std::exception const &e) {

				}


				// remove the files in the folder dir_sdfs
				DIR *dp;
		    	struct dirent *dirp;
		    	vector<string> files;
		    	if((dp = opendir((DIR_SDFS + to_string(myinfo.id) + "/").c_str())) == NULL) {
		      		cout << "Error(" << errno << ") opening " << (dir + "/") << endl;
		      		return errno;
		    	}
		   		while ((dirp = readdir(dp)) != NULL) {
		    		string name = dirp->d_name;
		    		// if(name.find(maple_prefix) != std::string::npos && stoi(split(name, "_")[1]) != current_id) {
		    		if(name.find(prefix) != std::string::npos || name.find("juiceoutput_") != std::string::npos) {
		    			if (remove((DIR_SDFS + to_string(myinfo.id) + "/" + name).c_str()) != 0)
							perror("File deletion failed");
						else
							cout << "File deleted successfully";
					}
		    	}
		    	closedir(dp);


			}
			
		}
		string msg = "OK";
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

int getServers() {
	ifstream fin("servers.txt"); 
    const int LINE_LENGTH = 100; 
    char str[LINE_LENGTH];  
    int line = 0;
    while( fin.getline(str,LINE_LENGTH) && line < 10) {  

        vector<string> v = split(str, " ");
        server_para sp;

        sp.id = stoi(v[0]);
        sp.addr = v[1];
        sp.hostname = v[2];
        sp.port = PORT_FILE + stoi(v[0]);
        sp.mapleport = PORT_MAPLE + stoi(v[0]);
        // printf("id ip %d %s\n", stoi(v[0]), v[1].c_str());
        serverlist[line] = sp;
        line++;
    }
    return 0;
}

int main(int argc, char const *argv[]) {
	
	//Set id
	init_para(argc, argv);
	neighbors = new server_para[NUM_NBR];
	serverlist = new server_para[10];
	myinfo.status = 0; //status = leave until test says join

	//Connect to Introducer
    //int sock;
    int valread; 
    char recv_info[BUFFER_SIZE] = {0}; 

    getServers();

    // master_server = serverlist[master_id];
    // if (myinfo.id == master_id) {master_init();}
    //master_server = serverlist[master_server.id - 1];

    master_server = serverlist[master_id-1];
    if (myinfo.id == master_id) {master_init();}
    master_server.port = PORT_MASTER + master_server.id - 1;
    printf("Master is Machine %d\n", master_server.id);
    printf("Node Id: %d\n", myinfo.id);
    
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

	//start master server in a thread
	thread thread_master;
	thread_master = thread(master);

	//start file server in a thread
	thread thread_fileserver;
	thread_fileserver = thread(file_server);

	thread thread_map_reduce;
	thread_map_reduce = thread(map_reduce);

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
							//if(myinfo.id == master_id) {
							//	re_replica(neighbors[i].id);
							//}
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

							node_quit_proc(neighbors[i].id);
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
