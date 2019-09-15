cd /home/deploy/mp1;
g++ -o client client.cpp -std=c++11 -pthread;
g++ -o server server.cpp -std=c++11;
nohup /home/deploy/mp1/server &