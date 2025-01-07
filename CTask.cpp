#include "CTask.h"
#include <boost/range/algorithm/count.hpp>
// ----------------------------------------------------------------------------
std::string get_uuid() {
    static std::random_device dev;
    static std::mt19937 rng(dev());
    std::uniform_int_distribution<int> dist(0, 15);
    std::uniform_int_distribution<int> distC(8, 11);
    const char *v = "0123456789abcdef";
    const bool dash[] = { 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};
    std::string res;
    for (int i = 0; i < 16; i++) {
        if (dash[i]) 
            res += "-";
        if (i == 6){
            res += v[4];
            res += v[distC(rng)];
        } else if(i == 8){
            res += v[distC(rng)];
            res += v[dist(rng)];
        } else{
            res += v[dist(rng)];
            res += v[dist(rng)];
        }
    }
    return res;
}
// ----------------------------------------------------------------------------
std::string CTask::translate_status()
{    /*
    #define CTASK_WAITING 120420
    #define CTASK_RUNNING 120421
    #define CTASK_FINISH  120422
    #define CTASK_FAIL    120423
    */
    if (this->status == CTASK_WAITING){
        return "WAITING";
    } else if (this->status == CTASK_RUNNING){
        return "RUNNING";
    } else if (this->status == CTASK_FINISH){
        return "FINISH";
    } else if (this->status == CTASK_FAIL){
        return "FAIL";
    }
    return "UNDEFINED"; 
};
// ----------------------------------------------------------------------------
int CTask::readFileLine(char * fileLine){
    std::string s = fileLine;
    int returnValue = 1;
    int FimIdx;
    if (s.length() > 0){
        char firstChar = s.c_str()[0];
        if (firstChar == '#')
            return returnValue;
        else{
            if (!(boost::count(s,'\t') == 9))
                return returnValue;
            // char uuid[1024], tag[1024], schedT[1024], initT[1024], endT[1024], comm[1024], errorM[1024];
            std::string uuid, tag, schedT, initT, endT, comm, errorM;
            unsigned int status;
            int queueP, pid;
            // printf("%s\n",s.c_str());
            FimIdx = s.find('\t');
            uuid = s.substr(0,0 + FimIdx);
            s = s.substr(FimIdx+1);
            FimIdx = s.find('\t');
            tag = s.substr(0,FimIdx);
            s = s.substr(FimIdx+1);
            FimIdx = s.find('\t');
            status = stoi(s.substr(0,FimIdx));
            s = s.substr(FimIdx+1);
            FimIdx = s.find('\t');
            queueP = stoi(s.substr(0,FimIdx));
            s = s.substr(FimIdx+1);
            FimIdx = s.find('\t');
            schedT = s.substr(0,FimIdx);
            s = s.substr(FimIdx+1);
            FimIdx = s.find('\t');
            initT = s.substr(0,FimIdx);
            s = s.substr(FimIdx+1);
            FimIdx = s.find('\t');
            endT = s.substr(0,FimIdx);
            s = s.substr(FimIdx+1);
            FimIdx = s.find('\t');
            comm = s.substr(0,FimIdx);
            s = s.substr(FimIdx+1);
            FimIdx = s.find('\t');
            pid = stoi(s.substr(0,FimIdx));
            s = s.substr(FimIdx+1);
            FimIdx = s.find('\n');
            errorM = s.substr(0,FimIdx);
            this->UUID = uuid;
            this->tag = tag;
            this->status = status;
            this->queuePosition = queueP;
            this->schedTime = schedT;
            this->initTime = initT;
            this->endTime = endT;
            this->command = comm;
            this->pid = pid;
            this->errorMessage = errorM;
            returnValue = 0;
            return returnValue;
        }
    }
    return returnValue;
};
// ----------------------------------------------------------------------------
std::string CTask::getDataToFile(){
    char buffer[8192];
    sprintf(buffer, "%s\t%s\t%u\t%d\t%s\t%s\t%s\t%s\t%d\t%s\n", this->UUID.c_str(),
    this->tag.c_str(),this->status,this->queuePosition,this->schedTime.c_str(),
    this->initTime.c_str(),this->endTime.c_str(),this->command.c_str(),this->pid,
    this->errorMessage.c_str());
    std::string returnStr = buffer;
    boost::trim_right(returnStr);
    return returnStr;
}
// ----------------------------------------------------------------------------
std::string  CTask::getDataAsJSON(){    
    char buffer[8192];
    sprintf(buffer,"{'uuid':'%s', 'tag': '%s','status':%s,'position':%d,"
    "'sched at':'%s','init at':'%s','end at':'%s','command':'%s','pid':%d,"
    "'error':'%s'}",
    this->UUID.c_str(), this->tag.c_str(),this->translate_status().c_str(),this->queuePosition,
    this->schedTime.c_str(),this->initTime.c_str(),this->endTime.c_str(),
    this->command.c_str(),this->pid,this->errorMessage.c_str());
    std::string returnStr = buffer;
    boost::trim_right(returnStr);
    return returnStr;
};
// ----------------------------------------------------------------------------
