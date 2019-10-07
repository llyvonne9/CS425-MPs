# CS425 MP1

##Deloyment:
	python deploy.py netid


## Compile

### Automatic Compile 
	run.sh

### Run introducer(Machine 01 in our MP) by introducer.cpp:
	./introducer
	

#### Run Normal Nodes by node.cpp:
	./node Id IpOfIntroducer
	e.g. ./node 1 172.22.154.145

### Run test.cpp:
	./test NodeId NodeIP ACTION(JOIN/LEAVE/INFO)
	e.g. ./test 2 172.22.152.145 JOIN -> let node 2 join the network
	
### Crash a Node
	Ctrl + C