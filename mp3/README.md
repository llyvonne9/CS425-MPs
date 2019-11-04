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
	e.g. ./node 1 (172.22.154.145)

### Run test.cpp:
	NODE ACTIONS./test NodeId ACTION(JOIN/LEAVE/INFO)
		e.g. ./test 2 JOIN -> let node 2 join the network
		
	FILE ACTIONS
	PUT: ./test NodeId PUT localname sdfsname
	GET: ./test NodeId GET sdfsname localname
	DELETE: ./test NodeId DELETE filename
	LS: ./test NodeId LS filename
	STORE: ./test NodeId STORE


	
### Crash a Node
	Ctrl + C