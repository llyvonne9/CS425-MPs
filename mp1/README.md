# CS425 MP1

##Deloyment:
	python deploy.py netid


## Compile

### Automatic Compile 
	run.sh

### Run client - Query command:
	- ./client (-m cmd -s servers.txt -l log_prefix).
	- -m is optional, and cmd can only start by grep for now (exclude file).
	- -s is optional. If there is none, it only runs cmd locally.
	- -l is optional. If logs besides vmn.log needs to be tested, add para to specify the prefix of logs.

	#### Query example:
	./client -m "grep linux" -s servers.txt -l "machine."

### Run server 
	./server


## Unit Test
	
### Step1: Generate and Distributed Logs
	- Commad: python3 log_generator.py
	- Notes: You can customize logs inside the code. A file named pattern_frequncy.pkl is generated containing known groundtruth for unit tests.

### Step2: 
	- Command: python3 test.py
	- Notes: This should be put into same director where the log_generator.py is to retrieve the groudtruth for unit tests.


Server.txt: 
	List of server_addr and server_port. Format:
	(int)number of servers
	Server01_address Space_or_Tab Space_or_Tab Server01_hostname Server01_port
	Server02_address Space_or_Tab Space_or_Tab Server01_hostname Server02_port
	...