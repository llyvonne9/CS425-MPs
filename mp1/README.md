# CS425 MP1

Deloyment:
	python deploy.py

(Local compile: run.sh)
(Run Server locally: ./server)

Server.txt: 
	List of server_addr and server_port. Format:
	(int)number of servers
	Server01_address Space_or_Tab Server01_port
	Server02_address Space_or_Tab Server02_port
	...

Run client - Query command:
	./client -m cmd (-s servers.txt)
	-m is necessary, and cmd can only start by grep for now
	-s is optional. If there is none, it only runs cmd locally

Query example:
	./client -m "grep linux test.log" -s servers.txt
