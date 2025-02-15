#include "CookFunctions.h"

// ----------------------------------------------------------------------------
// --- Retorna uma estampa de tempo para ser impressa na tela.
// --- Formato da estampa: YYYY-MM-DD-HH-MM-SS.mmm
char* timeStamp(){
    char* buffer = new char[30];
    char* bReturn = new char[80];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    strftime(buffer,30,"%Y-%m-%d-%T",localtime(&tv.tv_sec));
    sprintf(bReturn,"%s.%03d",buffer,(int)tv.tv_usec/1000);
    return bReturn;
}
//-----------------------------------------------------------------------------
std::string executeShellCommand(const char* cmd){
    char buffer[256];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe){
        //throw std::runtime_error("popen() failed!");
        printf("# [%s]: Falha ao chamar o comando popen()!\n",timeStamp());
        fflush(stdout);
    }
    try {
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    // remove new line character from result string
    int pos;
    while((pos=result.find('\n')) != std::string::npos)
        result.erase(pos);
    return result;
};
//-----------------------------------------------------------------------------
bool checkCommand(std::string strComm){
    printf("strComm: %s\n",strComm.c_str());
    std::string token = strComm.substr(0, strComm.find("|"));
    token = token.substr(0, token.find("&"));
    // printf("token.c_str() %s",token.c_str());
    // printf("token.find(python3) %lu\n",token.find("python3"));
    // printf("Check 1 %d\n",token.find(COMMAND_AUTH) != std::string::npos);
    // printf("Check 2 %d\n",token.find("python3") != std::string::npos);
    // printf("token.find(python3) %lu\n",token.find("python3"));
    return ((token.find("python3") != std::string::npos) || 
            (token.find("stress") != std::string::npos) );
    // return ((token.find(COMMAND_AUTH) != std::string::npos) &&
    //         (token.find("python3") != std::string::npos)) || 
    //         (token.find("stress") != std::string::npos);
};
//-----------------------------------------------------------------------------
int executeShellCommandPid(const char* cmd){
    /**
     * stress -c 4--vm 4 --vm-bytes 4096M -t 10s
    */
    std::string localCommand(cmd);
    // make a file name using the input command and the current time
    if (!checkCommand(localCommand))
        return -1;
    else{
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[256];    
        time (&rawtime);
        timeinfo = localtime(&rawtime); 
        strftime(buffer,sizeof(buffer),"%d-%m-%Y %I:%M:%S",timeinfo);
        std::string currentTime(buffer); 
        std::hash<std::string> hasher;
        std::string fileName = std::to_string(hasher(localCommand+currentTime)); 
        localCommand = localCommand + " & echo $! | grep -w -o -E '[0-9]*'>" + fileName + ".pid";
        // printf("localCommand 1: %s\n",localCommand.c_str());
        system(localCommand.c_str());
        localCommand = "cat " + fileName + ".pid";
        int pid = stoi(executeShellCommand(localCommand.c_str()));
        // printf("Process ID: %d.\n",pid);
        localCommand = "rm -rf " + fileName + ".pid";
        // printf("localCommand 2: %s\n",localCommand.c_str());
        system(localCommand.c_str());
        return pid;
    }
    return -1;
};
//-----------------------------------------------------------------------------
int is_pid_dir(const struct dirent *entry) {
    const char *p;
    for (p = entry->d_name; *p; p++) {
        if (!isdigit(*p))
            return 0;
    }
    return 1;
};
//-----------------------------------------------------------------------------
std::vector<int> getListPid() {
    DIR *procdir;
    FILE *fp;
    struct dirent *entry;
    char path[256 + 5 + 5]; // d_name + /proc + /stat
    int pid;
    std::vector<int> vecPID;
    std::mutex num_mutex;
    vecPID.clear();
    procdir = opendir("/proc");
    if (!procdir) {
        printf("# [%s]: Comando opendir falhou.\n",timeStamp());
        fflush(stdout);
        return vecPID;
    }
    // Iterate through all files and directories of /proc.
    num_mutex.lock(); 
    while ((entry = readdir(procdir))) {
        if (!is_pid_dir(entry))
            continue;
        // tenta abrir /proc/<PID>/stat.
        snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
        fp = fopen(path, "r");
        if (!fp) {
            printf("# [%s]: Falha ao abrir diretório %s.\n",timeStamp(),path);
            fflush(stdout);
            continue;
        }
        fscanf(fp, "%d ",&pid);
        vecPID.push_back(pid);
        fclose(fp);
    }
    num_mutex.unlock(); 
    closedir(procdir);
    return vecPID;
};
//-----------------------------------------------------------------------------
double getMemFreePercente() {
    unsigned long long memTotal, memAvaliable;
    double percent;
    FILE* file = fopen("/proc/meminfo", "r");
    char line[256];
    fgets(line, sizeof(line), file);
    sscanf(line, "MemTotal: %llu kB", &memTotal);
    fgets(line, sizeof(line), file);
    fgets(line, sizeof(line), file);
    sscanf(line, "MemAvailable: %llu kB", &memAvaliable);
    fclose(file);
    percent = 100*((double)memAvaliable)/memTotal;
    return percent; 
};
//-----------------------------------------------------------------------------
void initCpuPercent(unsigned long long &lastTotalUser, unsigned long long &lastTotalUserLow, 
                    unsigned long long &lastTotalSys, unsigned long long &lastTotalIdle){
    FILE* file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow,
        &lastTotalSys, &lastTotalIdle);
    fclose(file);
};
//-----------------------------------------------------------------------------
double getCpuPercent(unsigned long long &lastTotalUser, unsigned long long &lastTotalUserLow, 
                    unsigned long long &lastTotalSys, unsigned long long &lastTotalIdle){
    double percent;
    FILE* file;
    unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;
    file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow,
        &totalSys, &totalIdle);
    fclose(file);

    if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
        totalSys < lastTotalSys || totalIdle < lastTotalIdle){
        //Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else{
        total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
            (totalSys - lastTotalSys);
        percent = total;
        total += (totalIdle - lastTotalIdle);
        percent /= total;
        percent *= 100;
    }
    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;
    return percent;
};
//-----------------------------------------------------------------------------
