#include "DirManager.h"
#include "global.h"
#include "gzip/compress.hpp"
#include "gzip/decompress.hpp"
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <windows.h>
#include <io.h>

void DirManager::get_files(string path)
{
    long hFile = 0;
    struct _finddata_t fileinfo;
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1){
        do{
            if (fileinfo.attrib &  _A_SUBDIR){
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0 && !map_ignore.count(fileinfo.name))
                    get_files(p.assign(path).append("\\").append(fileinfo.name));
            }
            else if(!map_ignore.count(fileinfo.name)){
                set_new.emplace(p.assign(path).append("\\").append(fileinfo.name).erase(0, 2));
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}

void DirManager::get_nodes(string path)
{
    long hFile = 0;
    struct _finddata_t fileinfo;
    if ((hFile = _findfirst(path.append("/*").c_str(), &fileinfo)) != -1){
        do{
            if (!(fileinfo.attrib &  _A_SUBDIR)){
                set_his.emplace(fileinfo.name);
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}

void DirManager::copy_file(const string & src, const string & dst)
{
	ifstream is(src, ios::binary);
	ofstream os(dst, ios::binary);
	os << is.rdbuf();
	is.close();
	os.close();
}

void DirManager::log(const string str)
{
	ofstream os(logfile, ios::app);
	os << str;
	os.close();
}

bool DirManager::fignore(const string & file)
{
	for(unordered_map<string, bool>::iterator it = map_ignore.begin(); it != map_ignore.end(); it++){
		if(file.find(it->first) != string::npos) return 1;
	}
	return 0;
}

void DirManager::init()
{
	// create directory
	if(CreateDirectory(savedir.c_str(), 0)){
		SetFileAttributes(savedir.c_str(), FILE_ATTRIBUTE_HIDDEN);
	}
	CreateDirectory(lsdir.c_str(), 0);
	// get date
	stringstream ss;
    time_t t = time(0);
    tm * ltm = localtime(&t);
    ss << put_time(ltm, "%Y-%m-%d %H:%M:%S");
	log("[start time] " + ss.str() + "\n");
	ss.str("");
	ss << put_time(ltm, "%y%m%d") << exp[ltm->tm_hour] << exp[ltm->tm_min] << exp[ltm->tm_sec]; 
	newhead = ss.str();
	// load ignore files
	map_ignore[savedir] = 1;
	fstream fs(ignfile, ios::in);
	string str;
	if(fs.is_open()){
		while(getline(fs, str)){
			if(str.length() > 0) map_ignore[str] = 1;
		}
	}
	else fs.open(ignfile, ios::out);
	fs.close();
	cout << "get new files..." << endl << endl;
	// get new files
	ss.str("");
	clock_t start = clock();
	get_files(".");
	for(set<string>::iterator it = set_new.begin(); it != set_new.end(); it++){
		ss << *it << endl;
	}
	str = ss.str();
	str = gzip::compress(str.c_str(), str.size());
	fs.open(newfile, ios::out | ios::binary);
	fs << str;
	ss.str("");
	ss << "#get " << set_new.size() << " newfiles with " << clock() - start << " ms." << endl;
	log(ss.str());
	fs.close();
	// load history
	get_nodes(lsdir);
	// load head
	fs.open(headfile, ios::in);
	if(fs.is_open() && set_his.size()){
		fs >> head;
		if(!set_his.count(head)) head = *set_his.rbegin();
		curr = head;
	}
	else update();
	fs.close();
}

void DirManager::diff()
{
	vec_del.clear();
	vec_new.clear();
	set<string> set_curr = set_new;
	
	string lsfile = lsdir + "/" + curr;
	fstream fs(lsfile, ios::in | ios::binary);
	stringstream ss;
	ss << fs.rdbuf();
	fs.close();
	string str = ss.str();
	ss.str(gzip::decompress(str.c_str(), str.size()));
	
	clock_t start = clock();
	while(getline(ss, str)){
		if(set_curr.count(str)) set_curr.erase(str);
		else vec_del.emplace_back(str);
	}
	ss.clear();
	ss.str("");
	ss << "#read files with " << clock() - start << " ms." << endl;
	log(ss.str());
	
	for(set<string>::iterator it = set_curr.begin(); it != set_curr.end(); it++){
		vec_new.emplace_back(*it);
	}
}

void DirManager::display_diff()
{
	for(int i = 0; i < vec_new.size(); i++) cout << "new: " << vec_new[i] << endl;
	for(int i = 0; i < vec_del.size(); i++){
		if(!fignore(vec_del[i])) cout << "delete: " << vec_del[i] << endl;
	}
	if(vec_new.size() | vec_del.size()) cout << endl;
}

void DirManager::display_his()
{
	int i = 0;
	for(set<string>::iterator it = set_his.begin(); it != set_his.end(); it++){
		cout << "[" << ++i << "] " << *it << endl;
	}
}

void DirManager::display_help()
{
	cout << "[command_list]" << endl
		 << "diff" << endl
		 << "update" << endl
		 << "curr" << endl
		 << "head" << endl
		 << "nodes" << endl
		 << "switch" << endl
		 << "erase" << endl
		 << "help" << endl
		 << "exit" << endl
		 << "*dos_commands" << endl;
}

bool DirManager::checkout(const string & node)
{
	string lsfile = lsdir + "/" + node;
	fstream fs(lsfile, ios::in);
	if(fs.is_open()){
		curr = node;
		fs.close();
		return 1;
	}
	else set_his.erase(node);
	return 0;
}

bool DirManager::erase(const string & node)
{
	if(node == newhead) return 0;
	string lsfile = lsdir + "/" + node;
	DeleteFile(lsfile.c_str());
	set_his.erase(node);
	if(set_his.size()){
		if(curr == node) curr = *set_his.rbegin();
		if(head == node) head = *set_his.rbegin();
	}
	else update();
	return 1;
}

bool DirManager::update()
{
	if(!updated){
		string lsfile = lsdir + "/" + newhead;
		copy_file(newfile, lsfile);
		curr = head = newhead;
		set_his.emplace(newhead);
		fstream fs(headfile, ios::out);
		fs << newhead;
		fs.close();
		vec_new.clear();
		vec_del.clear();
		updated = 1;
		return 1;
	}
	return 0;
}

void DirManager::run()
{
	cout << "# Input 'help' to get information #" << endl << endl;
	init();
	diff();
	display_diff();
	
	string line;
	while(true){
		cout << "/> ";
		getline(cin, line);
		stringstream ss(line);
		string cmd;
		ss >> cmd;
		if(cmd == "exit") break;
		else if(cmd == "update"){
			bool success = update();
			if(success) cout << "done." << endl << "Now at node " + curr + "." << endl;
			else cout << "Nothing to update." << endl;
		}
		else if(cmd == "diff"){
			display_diff();
		}
		else if(cmd == "switch"){
			string node;
			ss >> node;
			if(node == "head") node = head;
			if(curr != node){
				bool success = checkout(node);
				if(success){
					diff();
					cout << "Now at node " + node + "." << endl;
				}
				else cout << "No node " + node + "." << endl;
			}
		}
		else if(cmd == "curr"){
			cout << "Now at node " + curr + "." << endl;
		}
		else if(cmd == "head"){
			cout << head << endl;
		}
		else if(cmd == "nodes"){
			display_his();
		}
		else if(cmd == "erase"){
			string node;
			while(ss >> node){
				if(node == "curr") node = curr;
				else if(node == "head") node = head;
				bool success = erase(node);
				if(success){
					cout << "done. ";
					if(curr == newhead) cout << "Now at new head " + curr + "." << endl;
					else cout << "Now at node " + curr + "." << endl;
				}
				else cout << "ERROR: erase " << node << " failed." << endl;
			}
		}
		else if(cmd == "help"){
			display_help();
		}
		else system(line.c_str());
	}
}

void DirManager::save()
{
	//save history
	fstream fs(hisfile, ios::out);
	for(set<string>::iterator it = set_his.begin(); it != set_his.end(); it++){
		fs << *it << endl;
	}
	fs.close();
	//save head
	fs.open(headfile, ios::out);
	fs << head;
	fs.close();
}

DirManager::~DirManager()
{
	log("\n");
	DeleteFile(newfile.c_str());
	save();
}
