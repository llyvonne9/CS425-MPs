#include <iostream>
#include <string>
using namespace std;


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

map<string, string> maple_helper(string contents) {
	map<string, string> count; 
	vector<string> words = split(contents, " ");
	for(int i = 0; i < words.size(); i++) {
		string word = words[i];
		if(word.length() == 0) continue;
		if(count.find(word) != count.end()) {
			count.find(word) -> second = to_string(stoi(count.find(word) -> second) + 1);
		} else {
			count.insert({word, to_string(1)});
		}
	}
	
	return count;
}

map<string, string> juice_helper(vector<string> files, string dir) {
	int count = 0;
	map<string, string> counts; 
	cout << dir + "/" + files[0] << "\n";
	for(int i = 0; i < files.size(); i++) {
		ifstream fin(dir + "/" + files[i]); 
	    const int LINE_LENGTH = 100; 
	    char str[LINE_LENGTH];  
	    while( fin.getline(str,LINE_LENGTH)) {  
	    	count += stoi(str);
	    }
	   	string word = split(files[i], "_")[2];
	   	if(counts.find(word) != counts.end()) {
	   		counts.find(word) -> second = to_string(stoi(counts.find(word) -> second) + count);
	   	} else {
	   		counts.insert({word, to_string(count)});
	   	}
	   	fin.close();
	}
	return counts;
}

// int main() {
// 	return 0;
// }