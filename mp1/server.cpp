#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h> 
#include <unistd.h> 
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace std;

#define PORT 8080
#define QUEUE_SIZE 10
#define BUFFER_SIZE 10240

// get the result given a grep query
string getResult(const char *cmd) {
	char res[BUFFER_SIZE];
	string result = "";
	int rc = 0;
	FILE *fp = popen(cmd, "r");
	if(NULL == fp) {
		perror("popen failed");
		exit(1);
	}
	while(fgets(res, sizeof(res), fp) != NULL) {
		// combine all results
		result += res;
	}
	rc = pclose(fp);
	if(rc == -1) {
		perror("Close failed");
	}
	return result;
}

int main(int arc, char const *argv[]) {
	int server_fd, new_server_fd;
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
	addr.sin_port = htons(PORT);

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
		new_server_fd = accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addrlen);
		if(new_server_fd < 0) {
			perror("[Error]: Fail to accept to incoming connections");
			exit(1);
		}

		//read the request
		read_received_message = read(new_server_fd, received_info, BUFFER_SIZE);
		printf("\nThe order received is: %s\n", received_info);
		result = getResult(received_info);
		
		// if the result does not existing
		if(result.length() == 0) {
			cout << "No Result Found \n" << endl;
			string error_msg = to_string(-1) + '\0';
			send(new_server_fd, error_msg.c_str(), error_msg.length(), 0);
			close(new_server_fd);
		} else {
			// printf("Query result is: %s\n", result.c_str());
			ofstream myfile;
			myfile.open ("result_sent.txt");
			myfile << result;
			myfile.close();

			int total_sent = 0;
			printf("result length=%lu\n",result.length());
			int n_sent = 0;
			//send the msg segment by segment
			while (total_sent < result.length()){
				int n_bytes = result.length() - total_sent < BUFFER_SIZE - 1? result.length() - total_sent: BUFFER_SIZE - 1;
				n_sent = send(new_server_fd, result.c_str() + total_sent, n_bytes, 0);
				total_sent += n_sent;
				printf("n_sent=%d\n", n_sent);
			}
			close(new_server_fd);
			//print statistics to facilitate test
			printf("done. total_sent=%d\n", total_sent);
			cout << "Total " << count(result.begin(), result.end(), '\n') << " lines are retrieved" << std::endl;
		}

		
	}
	return 0;
}

