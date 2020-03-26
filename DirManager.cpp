#include "DirManager.h"
#include "global.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <windows.h>
#include <io.h>

void DirManager::get_files(string path, set<string> & set_new)
{
    long hFile = 0;
    struct _finddata_t fileinfo;
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1){
        do{
            if ((fileinfo.attrib &  _A_SUBDIR)){
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0 && !map_ignore.count(fileinfo.name))
                    get_files(p.assign(path).append("\\").append(fileinfo.name), set_new);
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
	DeleteFile(dst.c_str());
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

string DirManager::get_date()
{
    time_t nowtime = time(0);
    char date[64];
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", localtime(&nowtime));
    return date;
}

bool DirManager::fignore(string & file)
{
	for(unordered_map<string, bool>::iterator it = map_ignore.begin(); it != map_ignore.end(); it++){
		if(file.find(file) != string::npos) return 1;
	}
	return 0;
}

void DirManager::init()
{
	if(CreateDirectory(savedir.c_str(), 0)){
		SetFileAttributes(savedir.c_str(), FILE_ATTRIBUTE_HIDDEN);
	}
	map_ignore[savedir] = 1;
	fstream fs(ignfile, ios::in);
	string file;
	if(fs.is_open()){
		while(getline(fs, file)) map_ignore[file] = 1;
	}
	fs.close();
	log("[start time] " + get_date() + "\n");
}

void DirManager::diff()
{
	vec_del.clear();
	vec_new.clear();
	
	stringstream ss;
	set<string> set_new;
	fstream fs(newfile, ios::out);
	clock_t start = clock();
	get_files(".", set_new);
	for(set<string>::iterator it = set_new.begin(); it != set_new.end(); it++){
		fs << *it << endl;
	}
	ss << "#get " << set_new.size() << " newfiles with " << clock() - start << " ms." << endl;
	log(ss.str());
	fs.close();
	
	fs.open(ini, ios::in);
	if(!fs.is_open()){
		copy_file(newfile, ini);
		cout << "Create new ini file." << endl;
	}
	else{
		ss.str("");
		start = clock();
		string str;
		while(getline(fs, str)){
			if(set_new.count(str)) set_new.erase(str);
			else vec_del.push_back(str);
		}
		ss << "#read inifiles with " << clock() - start << " ms." << endl;
		log(ss.str());
		fs.close();
		
		for(set<string>::iterator it = set_new.begin(); it != set_new.end(); it++){
			vec_new.push_back(*it);
		}
		
		for(int i = 0; i < vec_new.size(); i++) cout << "new: " << vec_new[i] << endl;
		for(int i = 0; i < vec_del.size(); i++){
			if(!fignore(vec_del[i])) cout << "delete: " << vec_del[i] << endl;
		}
		cout << endl;
	}
}

void DirManager::run()
{
	init();
	diff();
	
	string cmd;
	while(true){
		cout << "/> ";
		getline(cin, cmd);
		if(cmd == "exit") break;
		else if(cmd == "update"){
			if(vec_new.size() + vec_del.size() > 0) copy_file(newfile, ini);
			cout << "done." << endl;
		}
		else system(cmd.c_str());
	}
}

DirManager::~DirManager()
{
	DeleteFile(newfile.c_str());
	log("\n");
}
