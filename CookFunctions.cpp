#include "CookFunctions.h"

//-----------------------------------------------------------------------------
std::string executeShellCommand(const char* cmd){
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) 
        //throw std::runtime_error("popen() failed!");
        printf("Falha ao chamar o comando popen()!\n");
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
    std::string token = strComm.substr(0, strComm.find("|"));
    token = token.substr(0, token.find("&"));
    return ((token.find(COMMAND_AUTH) != std::string::npos) &&
            (token.find("python3") != std::string::npos)) || 
            (token.find("stress") != std::string::npos);
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
        char buffer[80];    
        time (&rawtime);
        timeinfo = localtime(&rawtime); 
        strftime(buffer,sizeof(buffer),"%d-%m-%Y %I:%M:%S",timeinfo);
        std::string currentTime(buffer); 
        std::hash<std::string> hasher;
        std::string fileName = std::to_string(hasher(localCommand+currentTime)); 

        localCommand = localCommand + " & echo $! | grep -w -o -E '[0-9]*'>" + fileName + ".pid";
        system(localCommand.c_str());
        localCommand = "cat " + fileName + ".pid";
        int pid = stoi(executeShellCommand(localCommand.c_str()));
        localCommand = "rm -rf " + fileName + ".pid";
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
    vecPID.clear();
    procdir = opendir("/proc");
    if (!procdir) {
        perror("opendir failed");
        return vecPID;
    }
    // Iterate through all files and directories of /proc.
    while ((entry = readdir(procdir))) {
        // Skip anything that is not a PID directory.
        if (!is_pid_dir(entry))
            continue;
        // Try to open /proc/<PID>/stat.
        snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
        fp = fopen(path, "r");
        if (!fp) {
            perror(path);
            continue;
        }
        // Get PID, process name and number of faults.
        fscanf(fp, "%d ",&pid);
        // fscanf(fp, "%d %s %*c %*d %*d %*d %*d %*d %*u %*lu %*lu %lu",
        //     &pid, &path, &maj_faults
        // );
        vecPID.push_back(pid);
        // Pretty print.
        // printf("%5d %-20s: %lu\n", pid, path, maj_faults);
        fclose(fp);
    }
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
