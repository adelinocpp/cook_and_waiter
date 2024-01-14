#ifndef CTASKLIST_H
#define CTASKLIST_H
#include "CTask.h"
#include <vector>

/**
    enfileira tarefa: comando, tag
    desenfileira tarefa: tag, id
    lista tarefas: todas (estatus), hora
    adiar tarefa: tag, id, positions
*/
int checkTaskFile(char * filename);

class CTaskList{
    private:
        std::vector<CTask> listOfTasks;
        std::string fileName;
        bool isFileLog;
    public:
        inline CTaskList(){
            listOfTasks.clear();
            fileName = "";
            isFileLog = false;
            };
        inline ~CTaskList(){listOfTasks.clear();};
        bool readFileTask(std::string filename, bool isLog=false);
        bool giveIntegrity();
        int writeFileTask();
        int getIdxByTag(std::string tag);
        int getIdxByUUID(std::string uuid);
        std::string listTasks();
        bool queueTask(CTask qTask);
        bool dequeueTasks(unsigned int idx=0);
        bool moveTasks(unsigned int idx, unsigned int nPos=1);
        inline unsigned int size(){return this->listOfTasks.size();};
        inline CTask operator[](unsigned int idx) {return this->listOfTasks[idx];};
        inline void setTask(CTask newTask, unsigned int idx){this->listOfTasks[idx] = newTask;};
        std::string getListOfTask();
};

#endif //CTASKLIST_H