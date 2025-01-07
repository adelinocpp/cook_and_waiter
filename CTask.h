#ifndef CTASK_H
#define CTASK_H
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <boost/algorithm/string.hpp>
#include <random>

#include "CookFunctions.h"

// ID - tag - hora agendamento - hora inicio - estatus - posicao - comando - hora fim
#define CTASK_WAITING 120420
#define CTASK_RUNNING 120421
#define CTASK_FINISH  120422
#define CTASK_FAIL    120423
// #define CTASK_HEADER "# UUID;Tag;Status;Position;Sheduled;Start;End;Command;PID;Error"
#define CTASK_HEADER "# UUID\tTag\tStatus\tPosition\tSheduled\tStart\tEnd\tCommand\tPID\tError"
#define CTASK_GUARD "# Lista de processos na fila."

// ----------------------------------------------------------------------------
std::string get_uuid();
// ----------------------------------------------------------------------------
class CTask{
    private:
        std::string UUID;
        std::string tag;
        std::string schedTime;
        std::string initTime;
        unsigned int status;
        std::string endTime;
        int queuePosition;
        std::string command;
        int pid;
        std::string errorMessage;
    public:
        CTask(){
            UUID = get_uuid();
            tag = "-";
            status = CTASK_WAITING;
            queuePosition = -1;
            schedTime = "-";
            initTime = "-";
            endTime = "-";
            command = "-";
            pid = -1;
            errorMessage = "-";
        };
        ~CTask(){};
        int readFileLine(char * fileLine);
        inline std::string getUUID(){ return this->UUID;};
        inline int getStatus(){ return this->status;};
        inline void setStatus(unsigned int newStatus){this->status = newStatus;};
        inline int getPosition(){ return this->queuePosition;};
        inline void setPosition(int newPosition){this->queuePosition = newPosition;};
        inline std::string getErrorMessage(){return this->errorMessage;};
        inline std::string getCommand(){return this->command;};
        bool operator<(const CTask &sTask) const { return queuePosition < sTask.queuePosition; };
        std::string getDataToFile();
        std::string getDataAsJSON();
        inline void setSchedTime(std::string newTime) {this->schedTime = newTime;};
        inline void setInitTime(std::string newTime) {this->initTime = newTime;};
        inline void setEndTime(std::string newTime) {this->endTime = newTime;};
        inline void setPID(int newPID){this->pid = newPID;};
        inline void setError(std::string newError){this->errorMessage = newError;};
        inline void setCommand(std::string newCommand){this->command = newCommand;};
        inline void setTag(std::string newTag){this->tag = newTag;};
        std::string translate_status();
        inline void appendUuidOnCommand(){
            if (this->command.find("python3") != std::string::npos)
                this->command = this->command + " " + this->UUID;
        };
};

#endif // CTASK_H
