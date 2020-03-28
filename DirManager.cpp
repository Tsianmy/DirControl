#include "DirManager.h"
#include "global.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono> 
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
            if(fileinfo.attrib &  _A_SUBDIR){
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0 && !map_ignore.count(fileinfo.name))
                    get_files(p.assign(path).append("\\").append(fileinfo.name));
            }
            else if(!map_ignore.count(fileinfo.name)){
                set_new.insert(p.assign(path).append("\\").append(fileinfo.name).erase(0, 2));
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}

void DirManager::copy_file(const string & src, const string & dst)
{
	ifstream is(src);
	ofstream os(dst);
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

bool DirManager::fignore(string & file)
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
    time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    tm * ltm = localtime(&t);
    ss << put_time(ltm, "%Y-%m-%d %H:%M:%S");
	log("[start time] " + ss.str() + "\n");
	ss.str("");
	ss << put_time(ltm, "%y%m%d%H%M%S");
	newhead = ss.str();
	// load ignore files
	map_ignore[savedir] = 1;
	fstream fs(ignfile, ios::in);
	string file;
	if(fs.is_open()){
		while(getline(fs, file)){
			if(file.length() > 0) map_ignore[file] = 1;
		}
	}
	else fs.open(ignfile, ios::out);
	fs.close();
	// get new files
	ss.str("");
	fs.open(newfile, ios::out);
	clock_t start = clock();
	get_files(".");
	for(set<string>::iterator it = set_new.begin(); it != set_new.end(); it++){
		fs << *it << endl;
	}
	ss << "#get " << set_new.size() << " newfiles with " << clock() - start << " ms." << endl;
	log(ss.str());
	fs.close();
	// load history
	fs.open(hisfile, ios::in);
	if(fs.is_open()){
		while(getline(fs, file)) set_his.insert(file);
	}
	fs.close();
	// load head
	fs.open(headfile, ios::in);
	if(fs.is_open()){
		fs >> head;
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
	
	string lsfile = lsdir + "/" + curr + ".txt";
	fstream fs(lsfile, ios::in);
	stringstream ss;
	clock_t start = clock();
	string file;
	while(getline(fs, file)){
		if(set_curr.count(file)) set_curr.erase(file);
		else vec_del.push_back(file);
	}
	ss << "#read files with " << clock() - start << " ms." << endl;
	log(ss.str());
	fs.close();
	
	for(set<string>::iterator it = set_curr.begin(); it != set_curr.end(); it++){
		vec_new.push_back(*it);
	}
}

void DirManager::display_diff()
{
	for(int i = 0; i < vec_new.size(); i++) cout << "new: " << vec_new[i] << endl;
	for(int i = 0; i < vec_del.size(); i++){
		if(!fignore(vec_del[i])) cout << "delete: " << vec_del[i] << endl;
	}
	cout << endl;
}

void DirManager::display_his()
{
	int i = 0;
	for(set<string>::iterator it = set_his.begin(); it != set_his.end(); it++){
		cout << "[" << ++i << "] " << *it << endl;
	}
}

bool DirManager::checkout(const string & branch)
{
	string lsfile = lsdir + "/" + branch + ".txt";
	fstream fs(lsfile, ios::in);
	if(fs.is_open()){
		curr = branch;
		return 1;
	}
	else set_his.erase(branch);
	return 0;
}

bool DirManager::update()
{
	if(!updated){
		string lsfile = lsdir + "/" + newhead + ".txt";
		copy_file(newfile, lsfile);
		curr = head = newhead;
		set_his.insert(newhead);
		fstream fs(headfile, ios::out);
		fs << head;
		fs.close();
		updated = 1;
		return 1;
	}
	return 0;
}

void DirManager::run()
{
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
			if(success) cout << "done." << endl << "Now at branch " + curr + "." << endl;
			else cout << "Nothing to update." << endl;
		}
		else if(cmd == "diff"){
			display_diff();
		}
		else if(cmd == "switch"){
			string branch;
			ss >> branch;
			bool success = checkout(branch);
			if(success){
				diff();
				cout << "Now at branch " + branch + "." << endl;
			}
			else cout << "No branch " + branch + "." << endl;
		}
		else if(cmd == "history"){
			display_his();
		}
		else system(line.c_str());
	}
}

void DirManager::save_his()
{
	fstream fs(hisfile, ios::out);
	for(set<string>::iterator it = set_his.begin(); it != set_his.end(); it++){
		fs << *it << endl;
	}
	fs.close();
}

DirManager::~DirManager()
{
	log("\n");
	DeleteFile(newfile.c_str());
	save_his();
}
