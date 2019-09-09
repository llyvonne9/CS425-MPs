g++ -O3 -o /usr/local/mp1/client /usr/local/mp1/client.cpp -std=c++11 -pthread
g++ -O3 -o /usr/local/mp1/server /usr/local/mp1/server.cpp
pkill -f "server"
nohup /usr/local/mp1/server &