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
		set<string> set_new, set_his;
		string curr, head, newhead;
		bool updated = 0;
		
		DirManager(){}
		void init();
		void diff();
		void display_diff();
		void display_his();
		void display_help();
		bool checkout(const string & node);
		bool erase(const string & node);
		bool update();
		void copy_file(const string & src, const string & dst);
		void get_files(string path);
		void get_nodes(string path);
		bool fignore(const string & file);
		void log(const string str);
		void run();
		void save();
		~DirManager();
};

#endif
