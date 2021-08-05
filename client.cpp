/* 
 * File:   client.cpp
 * Author: Alexandre Heideker
 *
 * Created on March 24 2017, 14:04
 */

#include <stack>
#include <cstdlib>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <thread>
#include <unistd.h>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <cstring>
#include <thread>
#include <iostream>
#include <vector>
#include<bits/stdc++.h>

#include "rest.cpp"
#include "swissknife.h"

#define SEM_WAIT mt.lock();
#define SEM_POST mt.unlock();
#define SEM_JOB_WAIT mtJob.lock();
#define SEM_JOB_POST mtJob.unlock();
#define PING_INTERVAL 10
#define QTD_WORKERS 100
using namespace std;

int run(char*, char*, int);
int RndI(int, int);
void Randomize(void);
void SleepExp(float);
float getTimeExp(float);
void Worker(int);
bool readSetup();
bool readSetupFromCL(int argc, char *argv[]);
bool parseVar(string token, string value);
void dumpVar();
double getNow(void);
string getAllTargets();
string getRndTarget();

float alfa;
float rate;
std::vector <std::string>  target;
string tag;
string FileName;
int id;
int nFiles = 100;
int ExpTime;
int ExpStart;
bool _debugMode;
int qtdWorkers = 10;
std::mutex mt;
std::mutex mtJob;
stack<int> Jobs;
stack<int> ThrStack;
stack<int> ExecStack;
bool doWork = true;
bool comma = false;
bool logSql = false;
bool https = false;
string proxy = "";
ofstream logOut;

int main(int argc, char *argv[]) {
    Randomize();
    readSetup();
    if (argc>1) {
        if (!readSetupFromCL(argc, argv)) return -1;
    }
    if (_debugMode) dumpVar();
    char f[30];
    sprintf(f, "client%04u.txt", id);
    FileName = f;
    logOut.open(FileName.c_str(), ios_base::out);
    if (logSql) logOut << "insert into client (exp, tStamp, delay, erro) values " << endl;
    if (_debugMode>0) cout << "Create log file: " << FileName << endl;
    std::thread * th_id = new std::thread[qtdWorkers];
 
    if (_debugMode>0) cout << "Start...." << endl;

    alfa = alfa / qtdWorkers;
    sleep(20);
    for (int i=0; i<qtdWorkers; i++){
        if (_debugMode>0) cout << "\r" << "Startting worker " << i << std::flush;
        th_id[i] = thread(Worker, i);
    }
    if (_debugMode>0) cout << "...OK" << endl;

    for (int i = 0; i < qtdWorkers; i++)
        th_id[i].join();
    logOut.close();
    return 0;
}


void Worker(int n) {
    ExpStart = (unsigned) time(NULL);
    unsigned t0 = (unsigned) time(NULL);
    float alfaT = alfa * .1;
    unsigned t;
    unsigned t1 = (ExpTime * .1);
    unsigned t2 = (ExpTime * .9);

    float alfa0 = alfa * .1;
    float alfa2 = alfa;
    int i = 0;
    float timeExp = getTimeExp(alfa0);
    float step = ((.9 * alfa) / (.8 * ExpTime));

    double ti;
    while ((ExpStart + ExpTime - (unsigned) timeExp) >= (unsigned) time(NULL)) {
        SleepExp(timeExp);
        unsigned tInicio =  (unsigned) time(NULL) ;
        
        int n = RndI(1, nFiles);
        Rest rest;
        string url = ((https)? "https://" : "http://") + getRndTarget() + "/" + to_string(n) + ".jpg";
        if (_debugMode) cout << "wGet " << url << endl;
        Rest::wget_status st = rest.wget(url, proxy);
        if (!st.ok) {
            ti = 0;
        } else {
            ti = (st.time / st.size) * 1048576; // (time per megabyte)
        }
        //write statistics
        SEM_WAIT
        if (_debugMode) cout << "Writing statistics..." << endl;
        //ofstream log;
        //log.open(FileName.c_str(), ios_base::app);
        if (logSql) {
            logOut << ((comma)? ",":"") << " ('" << tag << "', " << tInicio 
                << ", " << ti << ", " << ((st.ok)? "0" : "1" ) << ")" << std::flush;
            comma = true;
        } else {
            logOut << id << ";" << tag << ";" << tInicio << ";" << i << ";"
                << ti << ";" << ((st.ok)? "0" : "1" )<< endl;
        }
        //log.close();
        SEM_POST

        t = (unsigned) time(NULL) - t0;
        if (t < t1) {
            alfaT = alfa0;
        } else if (t > t2) {
            alfaT = alfa2;
        } else {
            alfaT = alfa0 + (t * step) ;
        }
        i++;
        timeExp = getTimeExp(alfaT);
    }
    if (_debugMode>0) cout << "\rWorker " << n << " good bye!    " << std::flush;
}


int run(char* comando, char* saida, int max) {
    FILE *fp;
    fp = popen(comando, "r");
    if (fp == NULL) //erro
        return -1;
    fgets(saida, max - 1, fp);
    pclose(fp);
    return 0;
}

void Randomize(void) {
    srand(time(NULL));
}

int RndI(int min, int max) {
    int k;
    double d;
    d = (double) rand() / ((double) RAND_MAX + 1);
    k = d * (max - min + 1);
    return min + k;
}

void SleepExp(float e) {
    int s;
    long ns;
    float resto;
    resto = (float) e - (int) e;
    s = (int) e;
    ns = (long) (resto * 1000000000L);
    struct timespec req = {0};
    req.tv_sec = s;
    req.tv_nsec = ns;
    nanosleep(&req, (struct timespec *)NULL);    
    //nanosleep((struct timespec[]) {{s, ns}}, NULL);
    return;
}

float getTimeExp(float alf) {
    float r;
    float e;
    r = (float) rand() / ((float) RAND_MAX + 1);
    e = -1 * log(1 - r) / alf;
    return e;
}

bool parseVar(string token, string value){
    transform(token.begin(), token.end(), token.begin(), ::tolower);
    if (token == "debugmode") {
        _debugMode = stoi(value);
    } else
    if (token == "logsql") {
        logSql = true;
    } else
    if (token == "https") {
        https = true;
    } else
    if (token == "alfa") {
        alfa = stof(value);
    } else
    if (token == "rate") {
        rate = stof(value);
    } else
    if (token == "target") {
        target = splitString(trim(value), ',');
    } else
    if (token == "tag") {
        tag = value;
    } else
    if (token == "proxy") {
        proxy = value;
    } else
    if (token == "id") {
        id = stoi(value);
    } else
    if (token == "nfiles") {
        nFiles = stoi(value);
    } else
    if (token == "exptime") {
        ExpTime = stoi(value);
    } else
    if (token == "qtdworkers") {
        qtdWorkers = stoi(value);
    } else
    if (token == "help") {
        cout << "Usage: \n \t --debugMode=<int>\n\t--alfa=<float>\n\t--proxy=<host:port>\n\t--target=<host,host,host>\n\t--tag=<exp.tag>\n\t--id=<int>\n\t--nFiles=<int>\n\t--ExpTime=<int>\n\t--qtdWorkers=<int>\n\t--logSQL\n\t--https\n\t--help (this help)" << endl;
        return false;
    } else
    {
        cout << "Invalid argument: Token=" << token << (value != "" ? "  Value=" : "") << value << endl;
        cout << "Usage: \n \t --debugMode=<int>\n\t--alfa=<float>\n\t--proxy=<host:port>\n\t--target=<host,host,host>\n\t--tag=<exp.tag>\n\t--id=<int>\n\t--nFiles=<int>\n\t--ExpTime=<int>\n\t--qtdWorkers=<int>\n\t--logSQL\n\t--https\n\t--help (this help)" << endl;
        return false;
    }
    return true;
}

bool readSetup(){
    bool error = false;
    ifstream File;
    string line;
    File.open ("setup.conf");
    if (File.is_open()) {
        string token;
        string value;
        while (!File.eof() && !error){
            getline(File, line);
            if (line[0] != '#' && line!="") {
                token = trim(line.substr(0, line.find("=")));
                if (line.find("=") != string::npos) {
                    value = trim(line.substr(line.find("=")+1, line.length()-1));
                } else {
                    value = "";
                }
                error = !parseVar(token, value);
            }
        }
        File.close();
        return !error;
    } else {
        return false;
    }
}

bool readSetupFromCL(int argc, char *argv[]){
    int i;
    bool error = false;
    string token = "";
    string value;
    for (i=1; i<argc; i++) {
        string line(argv[i]);
        if (line[0] == '-' && line[1] == '-') {
            token = trim(line.substr(2, line.find("=")-2));
            if (line.find("=") != string::npos) {
                value = trim(line.substr(line.find("=")+1, line.length()-1));
            } else {
                value = "";
            }
            error = !parseVar(token, value);
        } else {
            error = true;
        }
        if (error) break;
    }
    if (error) {
        return false;
    } else {
        return true;
    }
}

void dumpVar() {
    cout << "*** DEBUG Mode *** Dumping variables:" << endl;
    cout << "Alfa:\t" << alfa << endl;
    cout << "Target:\t" <<  getAllTargets() << endl;
    cout << "Proxy:\t" <<  proxy << endl;
    cout << "Experiment Tag:\t" <<  tag << endl;
    cout << "Experiment Id:\t" <<  id << endl;
    cout << "Experiment Time:\t" <<  ExpTime << endl;
    cout << "Number of Threads (workers):\t" <<  qtdWorkers << endl;
}

double getNow(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double) (tv.tv_sec) + (double)(tv.tv_usec) / 1000000;
}

string getAllTargets(){
    string s;
    for (auto p: target)
        s += (p + " ");
    return s;
}
string getRndTarget(){
    if (target.size()==1) 
        return target[0];
    return target[RndI(0,target.size()-1)];
}

