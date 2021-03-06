#include <iostream>
#include <string>
using namespace std;

namespace task1{
   #include "wordcount.h"
}
// 第二个命名空间
namespace task2{
   #include "buildingelevation.h"
}


vector<string> split (string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

set<string> maple(string exe, string contents, string prefix, int maple_task_num, int current_index, string dir) {
	map<string, string> maple_map;
	if(strcmp(exe.c_str(), "wordcount") == 0)
		maple_map = task1::maple_helper(contents); 
	else 
		maple_map = task2::maple_helper(contents); 
	
	set<string> names;
	for (std::map<string, string>::iterator it=maple_map.begin(); it!=maple_map.end(); ++it) {
		string word = it->first;
		string count = it -> second;
		std::hash<std::string> str_hash;
		int h = str_hash(word) % maple_task_num;
		ofstream outfile;
		string name = prefix + "_" + to_string(h) + "_" + word + "_" + to_string(current_index);
		outfile.open(dir + name, std::ios_base::app);
		outfile << count + "\n"; 
		names.insert(name);
		// printf("Result: %s %s\n", word.c_str(), count.c_str());
		outfile.close();
	}
	
	return names;
}

int juice(string exe, vector<string> files, string output, string dir, string dir_maple) {
	//map<string, string> juice_map;
	map<string, int> juice_map;
	if(strcmp(exe.c_str(), "wordcount") == 0)
		juice_map = task1::juice_helper(files, dir); 
	else 
		juice_map = task2::juice_helper(files, dir); 
	ofstream outfile;
	cout << dir_maple + "/" + output <<"\n";
	outfile.open(dir_maple + "/" + output, std::ios_base::app);
	//for (std::map<string, string>::iterator it=juice_map.begin(); it!=juice_map.end(); ++it) {
	for (std::map<string, int>::iterator it=juice_map.begin(); it!=juice_map.end(); ++it) {
		outfile << (it -> first) + " " + to_string(it -> second) + "\n"; 
	}
	
	outfile.close();
	
	return 0;
}

// int main() {
// 	return 0;
// }