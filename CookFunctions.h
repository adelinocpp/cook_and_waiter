#ifndef COOK_FUNCTIONS
#define COOK_FUNCTIONS

#include <vector>
#include <string>
#include <dirent.h>
#include <chrono>

#define LOAD_BASE  5 //  milliseconds
#define LOAD_TIME_MEAN  40 //  milliseconds
#define MAX_CPU_PERCENT 20
#define MIN_MEM_PERCENT 75
#define COMMAND_AUTH "qJT28XHm5ra8Ce4C"

//-----------------------------------------------------------------------------
std::string executeShellCommand(const char* cmd);
bool checkCommand(std::string strComm);
//-----------------------------------------------------------------------------
int executeShellCommandPid(const char* cmd);
int is_pid_dir(const struct dirent *entry);
std::vector<int> getListPid();
//-----------------------------------------------------------------------------
double getMemFreePercente();
void initCpuPercent(unsigned long long &lastTotalUser, unsigned long long &lastTotalUserLow, 
                    unsigned long long &lastTotalSys, unsigned long long &lastTotalIdle);
double getCpuPercent(unsigned long long &lastTotalUser, unsigned long long &lastTotalUserLow, 
                    unsigned long long &lastTotalSys, unsigned long long &lastTotalIdle);
//-----------------------------------------------------------------------------


#endif //COOK_FUNCTIONS