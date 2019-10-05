#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h> 
#include <unistd.h> 
#include <fstream>
#include <algorithm>
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
using namespace std;

#define PORT 8080
#define QUEUE_SIZE 10
#define BUFFER_SIZE 10240

#define FAIL "FAIL"
#define JOIN "JOIN"

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

int main(int arc, char const *argv[]) {
	map<string, string> states;
	int server_fd, new_server_fd;
	struct sockaddr_in addr;
	string delimiter = "_";
	
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

	    string type = segment[0];
	    string idx = segment[1];
	    states.insert(std::make_pair(idx, type));
	    string finder = "";
	    if(strcmp(type.c_str(), FAIL) == 0) {
	    	finder = segment[2];
	    }

	    ofstream myfile;
    	myfile.open("log.txt", ios::app);
    	string res = "machine " + idx + " " + type + " at ";
    	auto time = std::chrono::system_clock::now();
    	time_t end_time = std::chrono::system_clock::to_time_t(time);
    	res += ctime(&end_time);
    	myfile << res;
    	myfile.close();


		
	}
	return 0;
}

