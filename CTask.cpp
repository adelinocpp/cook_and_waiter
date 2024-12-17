#include "CTask.h"
#include <boost/range/algorithm/count.hpp>
// ----------------------------------------------------------------------------
std::string get_uuid() {
    static std::random_device dev;
    static std::mt19937 rng(dev());
    std::uniform_int_distribution<int> dist(0, 15);
    const char *v = "0123456789abcdef";
    const bool dash[] = { 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};
    std::string res;
    for (int i = 0; i < 16; i++) {
        if (dash[i]) 
            res += "-";
        res += v[dist(rng)];
        res += v[dist(rng)];
    }
    return res;
}
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
            // printf("CTask: %s",s.c_str());
            // printf("CTask: %ld.\n",boost::count(s,'\t'));
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
            // sscanf(s.c_str(), "%s;%s;%u;%d;%s;%s;%s;%s;%d;%s\n", uuid, 
            //     tag,&status,&queueP,schedT, initT,endT,comm,&pid,
            //     errorM);


            // printf("%s\t%s\t%u\t%d\t%s\t%s\t%s\t%s\t%d\t%s\n", uuid.c_str(), 
            //     tag.c_str(),status,queueP,schedT.c_str(), initT.c_str(),
            //     endT.c_str(),comm.c_str(),pid,errorM.c_str());
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
            // printf("Leu....\n");
            return 0;
        }
    }
    // printf("return....\n");
    return returnValue;
};
// ----------------------------------------------------------------------------
std::string CTask::getDataToFile(){
    char buffer[4096];
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
    char buffer[4096];
    sprintf(buffer,"{'uuid':'%s', 'tag': '%s','status':%u,'position':%d,"
    "'sched at':'%s','init at':'%s','end at':'%s','command':'%s','pid':%d,"
    "'error':'%s'}",
    this->UUID.c_str(), this->tag.c_str(),this->status,this->queuePosition,
    this->schedTime.c_str(),this->initTime.c_str(),this->endTime.c_str(),
    this->command.c_str(),this->pid,this->errorMessage.c_str());
    std::string returnStr = buffer;
    boost::trim_right(returnStr);
    return returnStr;
};
// ----------------------------------------------------------------------------
