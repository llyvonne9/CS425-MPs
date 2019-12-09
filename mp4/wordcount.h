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

//map<string, string> juice_helper(vector<string> files, string dir) {
map<string, int> juice_helper(vector<string> files, string dir) {	
	//map<string, string> counts; 
	map<string, int> counts; 
	cout << dir + "/" + files[0] << "\n";
	for(int i = 0; i < files.size(); i++) {
		int count = 0;
		ifstream fin;
		try{
			fin.open(dir + "/" + files[i]); 
		} catch (std::exception const &e) {
			cout<<"Maybe not SDFS_FILE_LIST file yet."<< dir + "/" + files[i] << "\n";
		}
		if(!fin) {
			cout << "While opening a file an error is encountered "<< dir + "/" + files[i] << endl;
			//return -1;
		}
	    const int LINE_LENGTH = 100; 
	    char str[LINE_LENGTH];  
	   	string word = split(files[i], "_")[2];
	    //while( fin.getline(str,LINE_LENGTH)) {
	   	// while(!fin.eof()){
	   	// 	fin.getline(str,LINE_LENGTH)
		   //  if (word == "1813"){
		   //  	cout<< "a 1813: "<< files[i]<< " " << stoi(str)<< "\n";
		   //  }  
	    // 	count += stoi(str);
	    // }
	    int a;
	    //while(!fin.eof()){
	    while(fin >> a){
			//fin >> a;
			count += a;
		}
	   	if(counts.find(word) != counts.end()) {
	   		//counts.find(word) -> second = to_string(stoi(counts.find(word) -> second) + count);
	   		//counts[word] += count;
	   		counts.find(word)->second = counts.find(word)->second + count;
		    if (word == "1813"){
		    	cout<< "b 1813: "<< files[i]<< " "  << count<< " "<< counts[word] << "\n";
		    }  
	   	} else {
	   		//counts.insert({word, to_string(count)});
	   		//counts.insert({word, count});
	   		counts.insert(std::make_pair(word, count));
		    if (word == "1813"){
		    	cout<< "c 1813: "<< files[i]<< " "  << count<< "\n";
		    }  
	   	}
	   	fin.close();
	}
	return counts;
}

// int main() {
// 	return 0;
// }