#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h> 
#include <unistd.h> 
#include <iostream>

#define PORT 8080
#define QUEUE_SIZE 10
#define MAXLINE 1024

using namespace std;
string getResult(const char *cmd) {
	// char* res1;
	char res[MAXLINE];
	string result = "";
	int rc = 0;
	FILE *fp = popen(cmd, "r");
	if(NULL == fp) {
		perror("popen failed");
		exit(1);
	}
	while(fgets(res, sizeof(res), fp) != NULL) {
		result += res;
		printf("Command %s's output: %s\n", cmd, res);
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
	char received_info[MAXLINE] = {0}; 
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

	if(bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("[Error]: Fail to bind to address");
		exit(1);
	}


	while(true){
		if(listen(server_fd, QUEUE_SIZE) < 0) {
			perror("[Error]: Fail to listen to incoming connections");
			exit(1);
		}
		new_server_fd = accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addrlen);
		if(new_server_fd < 0) {
			perror("[Error]: Fail to accept to incoming connections");
			exit(1);
		}

		read_received_message = read(new_server_fd, received_info, MAXLINE);
		printf("The order received is: %s\n", received_info);
		// "grep 'linux' test.txt"
		result = getResult(received_info);
		printf("Query result is: %s\n", result.c_str());
		send(new_server_fd, result.c_str(), MAXLINE, 0);
	}
	return 0;
}

