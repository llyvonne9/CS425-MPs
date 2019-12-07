#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h> 
#include <fstream>
#include <algorithm>
#include <map>
#include <string>
#include <chrono>
#include <sstream>
#include <iostream>
#include <vector>
#include <set>
using namespace std;

#define PORT 8081
#define NODEPORT_BASE 8200
#define QUEUE_SIZE 10
#define BUFFER_SIZE 10240

#define FAIL -1
#define JOIN 1
#define LEAVE 0

set<int> memList;

//split a string to vector based on a delimiter
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

//get ips of all nodes
map<int, string> getIPs (string delimiter) {
	map<int, string> map;
	char szTest[100] = {0};
 
	FILE *fp = fopen("servers.txt", "r");
	if(NULL == fp)
	{
		printf("failed to open servers.txt\n");
		return map;
	}
 	int i = 0;	
	while(!feof(fp))
	{
		memset(szTest, 0, sizeof(szTest));
		fgets(szTest, sizeof(szTest) - 1, fp); // 包含了换行符
		vector<string> ips = split(szTest, " ");
		if(i < 10) map.insert(std::make_pair(atoi(ips[0].c_str()), ips[1]));
		i++;
	}
 
	fclose(fp);
 
	return map;
}

//introduce neighbors to a node when it joins
void introduceNeighbors(int type, int idx, map<int, string> ips, map<int, int> states, int sock, struct sockaddr_in &serv_addr, map<int, int> hb_states) {
	int nbIdx[4] = {-2, -1, 1, 2};
    char buffer[1024] = {0}; 
    int valread;
    printf("The new join node's ip : %s\n with %d heartbeat before \n", (ips.find(idx)->second).c_str(), hb_states.find(idx) -> second);

	string msg = "NEIGHBORS ";
	for(int i = 0; i < 4; i++) {
		int neighborIndex = idx + nbIdx[i];
		if(neighborIndex < 0) neighborIndex += 10;
		if(neighborIndex > 10) neighborIndex %= 10;
		if(neighborIndex == 0) neighborIndex = 10;
		string ip = ips.find(neighborIndex)->second;

		msg += to_string(i) + " " + to_string(neighborIndex) + " " + to_string(states.find(neighborIndex)->second) + " " + ip + " ";
	}

	string mem_list = "";
	for(int j = 1; j <= 10; j++) {
		int s = states.find(j) -> second;
		if(s == 1) {
			if(mem_list.length() == 0) mem_list += to_string(j);
			else mem_list += " " + to_string(j);
		}
	}

	msg += "MEMLIST " + mem_list + " " + to_string(hb_states.find(idx) -> second);

	printf("Sent JOIN msg %s\n", msg.c_str());
	send(sock, msg.c_str(), msg.length(), 0);
	printf("NEIGHBORS info sent to new join node. \n"); 
	
}

//If a node leave or join update its status in its neighbors' membershiplist
void updateStatus(int type, int idx, map<int, string> ips, int sock, struct sockaddr_in &serv_addr, map<int, int> states) {
	
	//neighbors of a node is its nearest nodes in the virtual ring, eg neighbors of 1 is 9, 10, 2, 3
	int nbIdx[4] = {-2, -1, 1, 2};

	for(int i = 0; i < 4; i++) {

		int neighborIndex = idx + nbIdx[i];
		if(neighborIndex < 0) neighborIndex += 10;
		if(neighborIndex > 10) neighborIndex %= 10;
		if(neighborIndex == 0) neighborIndex = 10;

		//if the neighbor is not in the network, do not send the update
		if((states.find(neighborIndex) -> second) != 1) continue;


		int sock = 0, valread; 
	    struct sockaddr_in serv_addr; 
	    char buffer[1024] = {0}; 
	    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	    { 
	        printf("\n Socket creation error \n"); 
	    } 
	   
	    serv_addr.sin_family = AF_INET; 
	    cout<<neighborIndex<<"\n";
	    serv_addr.sin_port = htons(NODEPORT_BASE+neighborIndex); 



		if(inet_pton(AF_INET, (ips.find(neighborIndex) -> second).c_str(), &serv_addr.sin_addr)<=0) { 
	        printf("\nInvalid address/ Address not supported \n"); 
	    } 
	   
	    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
	        printf("\nConnection Failed \n"); 
	    }  
	    

	    //Update + the update node is nth neighbor of its neighbor i + neighbor index + update to which type
		string msg = "UPDATE " + to_string(4 - i) + " " + to_string(idx) + " " + to_string(type);
		
		printf("Update %s change to %d. Sent its neighbor %s\n", to_string(idx).c_str(), type, to_string(neighborIndex).c_str());
		send(sock, msg.c_str(), msg.length(), 0);
		close(sock);
		sock = 0;
	}
}

int main(int arc, char const *argv[]) {
	map<int, int> states;
	map<int, int> heartbeat_status;
	int server_fd, new_server_fd;
	struct sockaddr_in addr;
	string delimiter = "_";
	map<int, string> ips = getIPs(" ");
	
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
	addr.sin_port = htons(PORT);

	//bind socket to the address
	if(::bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("[Error]: Fail to bind to address");
		exit(1);
	}

	for(int i = 1; i < 11; i++) {
		states.insert(std::make_pair(i, LEAVE));
		heartbeat_status.insert({ i, 0 }); 
	}

	string d = " ";
    getIPs(d);

	while(true){
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
		printf("\nThe INFO received is: %s\n", received_info);

	    vector<string> segment = split (received_info, delimiter);

	    int type = 0;
	    ;
	    
	    int idx = atoi(segment[1].c_str());
	    if(strcmp(segment[0].c_str(), "FAIL") == 0 && (states.find(idx) -> second) == JOIN) {
	    	type = FAIL;
	    	int failIdx = stoi(segment[2]);
	    	int hb_times = stoi(segment[3]);
	    	states.find(failIdx)->second = type;
	    	
	    	if((heartbeat_status.find(failIdx) -> second) < hb_times) {
	    		heartbeat_status.find(failIdx) -> second = hb_times;
	    	}
	    	// updateStatus(type, failIdx, ips, new_server_fd, addr, states);
	    } else if(strcmp(segment[0].c_str(), "LEAVE") == 0 && (states.find(idx) -> second) == JOIN) {
	    	type = LEAVE;
	    	states.find(idx)->second = type;
	    	updateStatus(type, idx, ips, new_server_fd, addr, states);
	    	string msg = "OK";
	    	send(new_server_fd, msg.c_str(), msg.length(), 0);
	    } else if(strcmp(segment[0].c_str(), "JOIN") == 0 && ((states.find(idx) -> second) == FAIL || (states.find(idx) -> second) == LEAVE)) {
	    	type = JOIN;
	    	
		    states.find(idx)->second = type;
	    	introduceNeighbors(type, idx, ips, states, new_server_fd, addr, heartbeat_status);
	    	updateStatus(type, idx, ips, new_server_fd, addr, states);
	    } else {
	    	// If user sends invalid command like join twice or leave when it is inactive
	    	printf("The msg is Invalid");
	    	string msg = "OK";
	    	send(new_server_fd, msg.c_str(), msg.length(), 0);
	    	continue;
	    }

	    // updateStatus(type, idx, new_server_fd);
	    // states.insert(std::make_pair(idx, type));
	    map<int, int>::iterator iter;
		iter = states.begin();
		while(iter != states.end()) {
		    cout << iter->first << " : " << iter->second << endl;
		    iter++;
		}
	    
		//Write down the status chance to logs
	    ofstream myfile;
    	myfile.open("log.txt", ios::app);
    	string res = "machine " + to_string(idx) + " " + to_string(type) + " at ";
    	auto time = std::chrono::system_clock::now();
    	time_t end_time = std::chrono::system_clock::to_time_t(time);
    	res += ctime(&end_time);
    	myfile << res;
    	myfile.close();
    	close(new_server_fd);
		
	}
	return 0;
}

