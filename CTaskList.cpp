#include "CTaskList.h"
#include <string>
#include <algorithm>
#include <stdio.h>
#include "CookFunctions.h"

//-----------------------------------------------------------------------------
int checkTaskFile(char * filename){
    FILE* fp = fopen(filename, "r");
    if (fp != NULL){
        printf("# [%s]: %s encontrado.\n",timeStamp(),filename);
        fflush(stdout);
        fclose(fp);
        return 0;
    } else {
        printf("# [%s]: %s não encontrado. Criando...\n",timeStamp(),filename);
        FILE* fp = fopen(filename, "w");
        if (fp != NULL){
            printf("# [%s]: %s criado.\n",timeStamp(),filename);
            fflush(stdout);
            fprintf(fp, "%s \n",CTASK_GUARD);
            fflush(stdout);
            fprintf(fp, "%s \n",CTASK_HEADER);
            fflush(stdout);
            fclose(fp);
            return 0;
        } else{
            printf("# [%s]: Ocorreu um problema e o arquivo %s não foi criado.\n",timeStamp(),filename);
            fflush(stdout);
            return 1;
        }
    }
}
//-----------------------------------------------------------------------------
bool CTaskList::loadFileTask(){
    CTask readTask;
    int taskRead;
    FILE* fp = fopen(this->fileName.c_str(), "r");
    try{
        if (fp == NULL)
            return false;
        char* line = NULL;
        size_t len = 0;
        while ((getline(&line, &len, fp)) != -1) {
            taskRead = readTask.readFileLine(line);
            if (taskRead == 1)
                continue;
            else
                this->listOfTasks.push_back(readTask);
        }
        fclose(fp);
        if (line)
            free(line);
        return true;
    } catch(...){
        if (fp != NULL)
            fclose(fp);
        return false;
    }
}
//-----------------------------------------------------------------------------
bool CTaskList::readFileTask(std::string filename, bool isLog){
    CTask readTask;
    int taskRead;
    FILE* fp = fopen(filename.c_str(), "r");
    try{
        if (fp == NULL)
            return false;
        char* line = NULL;
        size_t len = 0;
        while ((getline(&line, &len, fp)) != -1) {
            taskRead = readTask.readFileLine(line);
            if (taskRead == 1)
                continue;
            else
                this->listOfTasks.push_back(readTask);
        }
        fclose(fp);
        if (line)
            free(line);
        this->isFileLog = isLog;
        this->fileName = filename;
        return true;
    } catch(...){
        if (fp != NULL)
            fclose(fp);
        return false;
    }
}
//-----------------------------------------------------------------------------
bool CTaskList::giveIntegrity(){
    bool hasRunning = false;
    bool inOrder = true;
    unsigned int i, nRunning = 0;
    std::vector<int> readOrder;
    readOrder.clear();
    for (i = 0; i < this->listOfTasks.size(); i++){
        if (this->listOfTasks[i].getStatus() == CTASK_RUNNING){
            hasRunning = true;
            nRunning++;
            if (this->isFileLog){
                if (this->listOfTasks[i].getErrorMessage() == "-")
                    this->listOfTasks[i].setStatus(CTASK_FINISH);
                else
                    this->listOfTasks[i].setStatus(CTASK_FAIL);    
            } else
                this->listOfTasks[i].setStatus(CTASK_WAITING);
        }
        if (!(this->listOfTasks[i].getPosition() == (i+1)))
            inOrder = false;
    }
    if (!inOrder && !this->isFileLog){
        std::sort(this->listOfTasks.begin(), this->listOfTasks.end());
    }
    this->writeFileTask();
    return !(hasRunning || (!this->isFileLog && !inOrder));
}
//-----------------------------------------------------------------------------
int CTaskList::writeTaskLogFile(CTask logTask) {
    bool returnValue = 0;
    if (!this->isFileLog)
        return 1;
    FILE* pFile = fopen(this->fileName.c_str(), "a+");
    try{
        if (pFile!=NULL){
            fprintf(pFile, "%s \n",logTask.getDataToFile().c_str());
            fflush(stdout);
            fclose (pFile);
        }
    }catch(...){
        if (pFile != NULL)
            fclose(pFile);
        returnValue = 1;
    }
    return returnValue;
};
//-----------------------------------------------------------------------------
int CTaskList::writeFileTask(){  
    bool returnValue = 0;
    std::string strData, mode = "w";
    FILE* pFile = fopen(this->fileName.c_str(), mode.c_str());
    try{
        if (pFile!=NULL){
            // if (!this->isFileLog){
            fprintf(pFile, "%s \n",CTASK_GUARD);
            fflush(stdout);
            fprintf(pFile, "%s \n",CTASK_HEADER);
            fflush(stdout);
            // }
            for (unsigned int i = 0; i < this->listOfTasks.size(); i++){
                strData = this->listOfTasks[i].getDataToFile();
                fprintf(pFile, "%s \n",strData.c_str());
                fflush(stdout);
            }
        }
        fclose (pFile);
    }catch(...){
        if (pFile != NULL)
            fclose(pFile);
        returnValue = 1;
    }
    return returnValue;
}
//-----------------------------------------------------------------------------
int getIdxByTag(std::string tag, bool mfirst=true){
    // TODO: implement this function
    // Return de index(s) of task on vector by tag
    int returnValue = -1;
    return returnValue;
};
//-----------------------------------------------------------------------------
CTask CTaskList::getTaskByUUID(std::string uuid, int& idx){
    CTask returnValue, baseValue;
    bool loadFile = false;
    if (this->isFileLog)
        loadFile = this->loadFileTask();
    if (loadFile || !this->isFileLog)
        for(unsigned int i = 0; i < this->listOfTasks.size(); i++){
            baseValue = listOfTasks[i];
            if (uuid.compare(baseValue.getUUID()) == 0){
                returnValue = baseValue;
                idx = i;
                break;
            }
        }
    if (this->isFileLog)
        loadFile = this->freeMemoryFileTask();
    return returnValue;
};
//-----------------------------------------------------------------------------
std::string CTaskList::listTasks(){
    std::string strList = "";
    for (unsigned int i = 0; i < this->listOfTasks.size(); i++)
        strList +=  listOfTasks[i].getDataToFile();
    return strList;
}
;
//-----------------------------------------------------------------------------
bool CTaskList::queueTask(CTask qTask){
    bool returnValue = true;
    try{
        if (!this->isFileLog){
            unsigned int position = this->listOfTasks.size() + 1;
            qTask.setPosition(position);
        }
        this->listOfTasks.push_back(qTask);
    } catch(...){
        returnValue = false;
    }
    return returnValue;
};
//-----------------------------------------------------------------------------
bool CTaskList::dequeueTasks(unsigned int idx)
{
    bool returnValue = true;
    try{
        this->listOfTasks.erase(listOfTasks.begin()+idx);
        for (unsigned int i = 0; i < this->listOfTasks.size(); i++)
            this->listOfTasks[i].setPosition(i+1);
    } catch(...){
        returnValue = false;
    }
    return returnValue;
};
//-----------------------------------------------------------------------------
bool CTaskList::moveTasks(unsigned int idx, unsigned int nPos){
    bool returnValue = true;
    if ((idx + nPos) >= this->listOfTasks.size())
        nPos = this->listOfTasks.size() - idx -1;
    if ((idx < (this->listOfTasks.size()-1)) && (nPos > 0)){
        CTask tTask;
        try{
            for (unsigned int i = 0; i < nPos; i++){
                tTask = this->listOfTasks[idx+i+1];
                this->listOfTasks[idx+i+1] = this->listOfTasks[idx+i];
                this->listOfTasks[idx+i] = tTask;
                this->listOfTasks[idx+i+1].setPosition(idx+i+1);
                this->listOfTasks[idx+i].setPosition(idx+i);
            }
        } catch(...){
            returnValue = false;
        }
    } else{
        returnValue = false;
    }
    return returnValue;
};
//-----------------------------------------------------------------------------
std::string CTaskList::getListOfTask(){
    std::string rVector;
    std::string rawJson;
    unsigned int i;
    rVector = "[";
    for (i = 0; i < this->listOfTasks.size(); i++)
        rVector = rVector + listOfTasks[i].getDataAsJSON() + ",";
    if (rVector.size() > 1)
        rVector[rVector.size()-1] = ']';
    else
        rVector = rVector + "]";
    return rVector;
};
//-----------------------------------------------------------------------------