#ifndef DIRMANAGER_H
#define DIRMANAGER_H

#include <iostream>
#include <unordered_map>
#include <set>
#include <vector>
using namespace std;

class DirManager{
	public:
		unordered_map<string, bool> map_ignore;
		vector<string> vec_del, vec_new;
		
		DirManager(){}
		void init();
		void diff();
		void copy_file(const string & src, const string & dst);
		void get_files(string path, set<string> & set_new);
		string get_date();
		bool fignore(string & file);
		void log(const string str);
		void run();
		~DirManager();
};

#endif
