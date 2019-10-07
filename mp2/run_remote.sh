cd /home/deploy/mp2;
g++ -o node node.cpp -std=c++11 -pthread;
g++ -o introducer introducer.cpp -std=c++11 -pthread;
g++ -o test test.cpp -std=c++11 -pthread;
