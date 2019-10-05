cd /home/deploy;
mkdir mp1;
mv *.cpp *.txt *.log mp1/;
cd mp1;
g++ -o client client.cpp -std=c++11 -pthread;
g++ -o server server.cpp -std=c++11;
pkill -f "server"; nohup /home/deploy/mp1/server &