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

using namespace std;

#define PORT 8081
#define NODEPORT_BASE 8200
#define QUEUE_SIZE 10
#define BUFFER_SIZE 10240

#define FAIL -1
#define JOIN 1
#define LEAVE 0

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

void introduceNeighbors(int type, int idx, map<int, string> ips, map<int, int> states, int sock, struct sockaddr_in &serv_addr) {
	int nbIdx[4] = {-2, -1, 1, 2};
    char buffer[1024] = {0}; 
    int valread;
    printf("The new join node's ip : %s\n", (ips.find(idx)->second).c_str());
    // if(inet_pton(AF_INET, (ips.find(idx)->second).c_str(), &serv_addr.sin_addr)<=0)  
    // { 
    //     printf("\nInvalid address/ Address not supported \n"); 
    // } 
   
    // if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    // { 
    //     printf("\nConnection Failed \n"); 
    // }  

	string msg = "NEIGHBORS ";
	for(int i = 0; i < 4; i++) {
		int neighborIndex = idx + nbIdx[i];
		if(neighborIndex < 0) neighborIndex += 10;
		if(neighborIndex > 10) neighborIndex %= 10;
		if(neighborIndex == 0) neighborIndex = 10;
		string ip = ips.find(neighborIndex)->second;

		msg += to_string(i) + " " + to_string(neighborIndex) + " " + to_string(states.find(neighborIndex)->second) + " " + ip + " ";
	}
	printf("Sent %s\n", msg.c_str());
	send(sock, msg.c_str(), msg.length(), 0);
	printf("NEIGHBORS info sent to new join node. \n"); 
	
}

void updateStatus(int type, int idx, map<int, string> ips, int sock, struct sockaddr_in &serv_addr, map<int, int> states) {
	int nbIdx[4] = {-2, -1, 1, 2};

	for(int i = 0; i < 4; i++) {

		int neighborIndex = idx + nbIdx[i];
		if(neighborIndex < 0) neighborIndex += 10;
		if(neighborIndex > 10) neighborIndex %= 10;
		if(neighborIndex == 0) neighborIndex = 10;

		if((states.find(neighborIndex) -> second) != 1) continue;


		int sock = 0, valread; 
	    struct sockaddr_in serv_addr; 
	    char buffer[1024] = {0}; 
	    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	    { 
	        printf("\n Socket creation error \n"); 
	    } 
	   
	    serv_addr.sin_family = AF_INET; 
	    serv_addr.sin_port = htons(NODEPORT_BASE+neighborIndex); 



		if(inet_pton(AF_INET, (ips.find(neighborIndex) -> second).c_str(), &serv_addr.sin_addr)<=0) { 
	        printf("\nInvalid address/ Address not supported \n"); 
	    } 
	   
	    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
	        printf("\nConnection Failed \n"); 
	    }  
		string msg = "UPDATE " + to_string(i + 1) + " " + to_string(idx) + " " + to_string(type);
		
		printf("Send %s msg %s\n", to_string(idx).c_str(), msg.c_str());
		send(sock, msg.c_str(), msg.length(), 0);
		close(sock);
		sock = 0;
	}
}

int main(int arc, char const *argv[]) {
	map<int, int> states;
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
	    string finder = "";
	    
	    int idx = atoi(segment[1].c_str());
	    if(strcmp(segment[0].c_str(), "FAIL") == 0 && (states.find(idx) -> second) == JOIN) {
	    	type = FAIL;
	    	finder = segment[2];
	    	states.find(idx)->second = type;
	    	// updateStatus(type, idx, ips, new_server_fd, addr);
	    } else if(strcmp(segment[0].c_str(), "LEAVE") == 0 && (states.find(idx) -> second) == JOIN) {
	    	type = LEAVE;
	    	states.find(idx)->second = type;
	    	// updateStatus(type, idx, ips, new_server_fd, addr);
	    } else if(strcmp(segment[0].c_str(), "JOIN") == 0 && ((states.find(idx) -> second) == FAIL || (states.find(idx) -> second) == LEAVE)) {
	    	type = JOIN;
	    	
		    states.find(idx)->second = type;
	    	introduceNeighbors(type, idx, ips, states, new_server_fd, addr);
	    	updateStatus(type, idx, ips, new_server_fd, addr, states);
	    } else {
	    	printf("The msg is Invalid");
	    }

	    // updateStatus(type, idx, new_server_fd);
	    // states.insert(std::make_pair(idx, type));
	    map<int, int>::iterator iter;
		iter = states.begin();
		while(iter != states.end()) {
		    cout << iter->first << " : " << iter->second << endl;
		    iter++;
		}
	    

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

