// ----------------------------------------------------------------------------
// --- Created by Adelino Silva on 24/12/2023.
// --- mail: adelinocpp@yahoo.com, adelinocpp@gmail.com
// ----------------------------------------------------------------------------
// sudo apt install clang
// sudo apt install libc++-dev
// -> sudo apt install g++-12
// sudo apt-get install libboost-dev

#include <stdlib.h>
#include <thread>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <ctype.h>
#include <vector>
#include "CTask.h" 
#include "CTaskList.h"
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_ntoa
#include "CookFunctions.h"
// #include <sys/time.h>
#include <time.h>
#include "jsoncpp/json/json.h"

#define ORDER_FILE "cook_order.txt"
#define ORDER_FILE_LOG "cook_order_log.txt"
#define PORT 12142

using namespace std;
using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono; // nanoseconds, system_clock, seconds

// --- Variaveis compartilhadas -----------------------------------------------
static unsigned long long lastTotalUser, lastTotalUserLow, 
                            lastTotalSys, lastTotalIdle;
double percentCPU, percentFreeMemory;
int PIDrunning = -1;
bool isIdle = true;
CTaskList listOfTasks;
CTaskList logListOfTasks;
vector<int> PIDs;
std::mutex g_num_mutex, set_error_mutex;

//-----------------------------------------------------------------------------
bool checkBodyJson(string buffer, JSONCPP_STRING& err,Json::Value& body){
    std::string rawJson;
    Json::CharReaderBuilder builderJSON;
    const std::unique_ptr<Json::CharReader> reader(builderJSON.newCharReader());
    int posLeftBlacket = buffer.find("{") ;
    int posRightBlacket = buffer.find("}");
    int rawJsonLength;
    rawJson = buffer.substr(posLeftBlacket,posRightBlacket-posLeftBlacket+1);
    rawJsonLength = static_cast<int>(rawJson.length());
    return reader->parse(rawJson.c_str(), rawJson.c_str() + rawJsonLength, &body, &err);
}
//-----------------------------------------------------------------------------
std::string getFirstCall(string srtMessage="Primeira Mensagem"){
    Json::Value response;
    Json::StreamWriterBuilder builder;
    std::string json_file;
    printf("# [%s]: %s.\n",timeStamp(),srtMessage.c_str());
    fflush(stdout);
    response["mensagem"] = "Comando não executado";
    json_file= Json::writeString(builder, response);
    return json_file; 
}
//-----------------------------------------------------------------------------
std::string getListOfTasks(string buffer,bool finish=false){
    Json::Value body,response;
    Json::CharReaderBuilder builderJSON;
    JSONCPP_STRING err;
    Json::StreamWriterBuilder builder;
    std::string json_file;
    printf("# [%s]: Solicita lista de tarefas.\n",timeStamp());
    fflush(stdout);
    if (checkBodyJson(buffer,err,body)){
        if (finish){
            if(logListOfTasks.loadFileTask()){
                response["lista"] = logListOfTasks.getListOfTask();
                response["mensagem"] = "Lista de tarefas já executadas.";
                logListOfTasks.freeMemoryFileTask();
            } else
                response["mensagem"] = "Falha ao carregar lista de tarefas executadas.";
        }
        else{
            response["lista"] = listOfTasks.getListOfTask();
            response["mensagem"] = "Lista de tarefas em execução.";
        }
    }else{
        printf("# [%s]: Problema com json: %s.\n",timeStamp(),err.c_str());
        fflush(stdout);
    }
    json_file = Json::writeString(builder, response);
    return json_file;
};
//-----------------------------------------------------------------------------
std::string postQueue(string buffer){
    Json::Value body, response;
    JSONCPP_STRING err;
    Json::StreamWriterBuilder builder;
    std::string json_file;
    printf("# [%s]: Adiciona an fila.\n",timeStamp());
    fflush(stdout);
    if (checkBodyJson(buffer,err,body)){
        if (body["token"] == COMMAND_AUTH){
            CTask recieveTask;
            recieveTask.setCommand(body["command"].asString());
            recieveTask.setTag(body["tag"].asString());
            recieveTask.setSchedTime(timeStamp());
            listOfTasks.queueTask(recieveTask);
            if (listOfTasks.writeFileTask() == 0){
                response["mensagem"] = "Comando executado";
                response["uuid"] = recieveTask.getUUID();
            }
        } else{
            response["mensagem"] = "Comando não executado";
        }
    } else{
        printf("# [%s]: Problema com json: %s.\n",timeStamp(),err.c_str());
        fflush(stdout);
    }
    // --------
    json_file = Json::writeString(builder, response);
    return json_file;
}
// ----------------------------------------------------------------------------
std::string getTasksByID(string buffer){
    Json::Value body, response;
    Json::CharReaderBuilder builderJSON;
    JSONCPP_STRING err;
    Json::StreamWriterBuilder builder;
    std::string json_file;
    printf("# [%s]: Solicita tarefa por UUID.\n",timeStamp());
    fflush(stdout);
    if (checkBodyJson(buffer,err,body)){
        CTask findTask;
        unsigned int idx;
        std::string sendUUID = body["uuid"].asCString();
        // --- Try on List of Tasks
        findTask = listOfTasks.getTaskByUUID(sendUUID,idx);
        // --- Try on LOG List of Tasks
        if(findTask.getCommand().compare("-") == 0)
            findTask = logListOfTasks.getTaskByUUID(sendUUID,idx);
        if(findTask.getCommand().compare("-") == 0)
            response["tarefa"] = "{}";
        else{
            response["tarefa"] = findTask.getDataAsJSON();
            response["messagem"] = "Tarefa não existe na lista de solicitações.";
        }
    }else{
        printf("# [%s]: Problema com json: %s.\n",timeStamp(),err.c_str());
        fflush(stdout);
    }
    json_file = Json::writeString(builder, response);
    return json_file;
}
// ----------------------------------------------------------------------------
std::string setErrorTasksByID(string buffer){
    Json::Value body, response;
    Json::CharReaderBuilder builderJSON;
    JSONCPP_STRING err;
    Json::StreamWriterBuilder builder;
    std::string json_file;
    printf("# [%s]: Seta mensagem de erro por UUID.\n",timeStamp());
    fflush(stdout);
    if (checkBodyJson(buffer,err,body)){
        CTask findTask;
        std::string sendUUID = body["uuid"].asCString();
        unsigned int idx;
        // --- INÍCIO: Seção crítica
        set_error_mutex.lock(); 
        findTask = listOfTasks.getTaskByUUID(sendUUID,idx);
        findTask.setError(body["error"].asCString());
        listOfTasks.setTask(findTask,idx);
        set_error_mutex.unlock(); 
        // --- FIM: Seção crítica
        response["sucess"] = "1";
    } else{
        printf("# [%s]: Problema com json: %s.\n",timeStamp(),err.c_str());
        response["sucess"] = "0";
        fflush(stdout);
    }
    json_file = Json::writeString(builder, response);
    return json_file;
}
// ----------------------------------------------------------------------------
// --- Funcao para resolver as mensagens aceitas pelo servidor
// --- Funcao muito repetitiva, precisa melhorar
// --- Requisições:
//      GET: e.g.  http://127.0.0.1:12142/?command=stress -c 4 --vm 4 --vm-bytes 4096M -t 10s (do nothing)
//      GET: e.g.  http://127.0.0.1:12142/list_tasks (ok)
//      POST: e.g.  http://127.0.0.1:12142/queue (ok)
//      POST: e.g.  http://127.0.0.1:12142/dequeue (not ready)
//      POST: e.g.  http://127.0.0.1:12142/postpone (not ready)
string solveMessage(string buffer){
    bool getFirst = buffer.find("GET /?command=") != -1;
    bool callGetListOfTasks = buffer.find("GET /list_tasks HTTP/1.1") != -1;
    bool callGetListOfFinishTasks = buffer.find("GET /list_finish_tasks HTTP/1.1") != -1;
    bool callGetTasksByID = buffer.find("GET /get_task_by_id HTTP/1.1") != -1;
    bool callSetErrorTasksByID = buffer.find("GET /set_error_task_by_id HTTP/1.1") != -1;
    bool callPostQueue = buffer.find("POST /queue HTTP/1.1") != -1;
    bool postDequeue = buffer.find("POST /dequeue HTTP/1.1") != -1;
    bool postPostpone = buffer.find("POST /postpone HTTP/1.1") != -1;
    bool hasBody  = ((buffer.find("{") != -1) && (buffer.find("}") != -1));
    std::string json_file;
    // TODO: Implementar as demais requisições
    // TODO: modo verbose
    if (getFirst){
        return getFirstCall();
    } else if (callGetListOfTasks && hasBody){
        return getListOfTasks(buffer);
    } else if (callGetListOfFinishTasks && hasBody){
        return getListOfTasks(buffer,true);
    } else if (callGetTasksByID && hasBody){
        return getTasksByID(buffer);
    } else if (callSetErrorTasksByID && hasBody){
        return setErrorTasksByID(buffer);
    } else if (callPostQueue && hasBody){
        return postQueue(buffer);
    } else if (postDequeue && hasBody){
        return getFirstCall("Remove da fila");
    } else if (postPostpone && hasBody){
        return getFirstCall("Adia na fila");
    } else{
        return getFirstCall("Comando não existe");
    }
    return json_file;
}
//-----------------------------------------------------------------------------
void *getRunParam(void*){
    unsigned int nPeriods = 0, i;
    const int time_mult = (int)LOAD_TIME_MEAN/LOAD_BASE;
    double cpu, men, sumCPU = 0, sumMem = 0;
    bool cPIDrunning = false;
    CTask tTask;
    while (1) {
        nPeriods++;
        cpu = getCpuPercent(lastTotalUser, lastTotalUserLow, lastTotalSys, 
                            lastTotalIdle);
        men = getMemFreePercente();
        sumCPU += cpu;
        sumMem += men;
        if (nPeriods == time_mult){
            percentCPU  = sumCPU/nPeriods;
            percentFreeMemory  = sumMem/nPeriods;
            nPeriods = 0;
            sumCPU = 0;
            sumMem = 0;
        }
        if (PIDrunning > 0){
            PIDs = getListPid();
            cPIDrunning = false;
            for (i = 0; i < PIDs.size(); i++)
                if (PIDs[i] == PIDrunning){
                    cPIDrunning = true;
                    break;
                }
        }
        if ((PIDrunning > 0) && !cPIDrunning){
            printf("# [%s]: File com PID: %d, running: %d.\n",timeStamp(),PIDrunning,cPIDrunning);
            fflush(stdout);
            // --- INÍCIO: Seção crítica
            g_num_mutex.lock(); 
            tTask = listOfTasks[0];
            tTask.setEndTime(timeStamp());
            tTask.setStatus(CTASK_FINISH);
            tTask.setPosition(-1);
            listOfTasks.dequeueTasks(0);
            listOfTasks.writeFileTask();
            logListOfTasks.writeTaskLogFile(tTask);
            PIDrunning = -1;
            isIdle = true;
            // --- FIM: Seção crítica
            g_num_mutex.unlock(); 
        }
        std::this_thread::sleep_until(system_clock::now() + milliseconds(LOAD_BASE));
    }
    pthread_exit(NULL);
}
//-----------------------------------------------------------------------------
void *getListenParam(void*){
    int opt = true;  
    int main_socket , addrlen , new_socket;   
    struct sockaddr_in address; 
    char buffer[1024];  
    std::string jsonMessage;
    // Criação do socket principal
    if( (main_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {  
        printf("# [%s]: Falha ao iniciar socket.",timeStamp());  
        fflush(stdout);
        exit(EXIT_FAILURE);  
    }  
    if( setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
            sizeof(opt)) < 0 ) {  
        printf("# [%s]: Erro ao realizar o setsockopt.",timeStamp());  
        fflush(stdout);
        exit(EXIT_FAILURE);  
    }  
    // Definições so socket criado
    address.sin_family = AF_INET; // AF_LOCAL  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  
    // Vincula (bind) o socket a porta PORT (12142)
    if (bind(main_socket, (struct sockaddr *)&address, sizeof(address))<0)  {  
        printf("# [%s]: Falha no bind.\n",timeStamp());  
        fflush(stdout);
        exit(EXIT_FAILURE);  
    }  
    while (1) {
        // Inicio da abertura socket
        // printf("\033[A\33[2K\r");
        printf("# [%s]: ---------------------------------------------------------------\n",timeStamp());
        fflush(stdout);
        printf("# [%s]: Escutando porta %d pela thread.\n", timeStamp(), PORT); 
        fflush(stdout);
        listen(main_socket,3);
        new_socket = accept(main_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0){
            printf("# [%s]: ERROR on accept.\n",timeStamp());
            fflush(stdout);
            break;
        }
        // --- INÍCIO: Seção crítica
        g_num_mutex.lock();
        bzero(buffer,1024);
        int n = read(new_socket,buffer,1024);
        if (n < 0){ 
            printf("# [%s]: Erro na leitura de dados do socket.\n",timeStamp());
            fflush(stdout);
            g_num_mutex.unlock(); 
            // printf("Mutex unlock.\n");
            break;
        }
        jsonMessage = solveMessage(buffer);
        string reply = "HTTP/1.1 200 OK\nContent-Type: json\nContent-Length: " +
                        to_string(jsonMessage.size()) + 
                        "\nAccept-Ranges: bytes\nConnection: close\n\n" + jsonMessage;
        n = send(new_socket, reply.c_str(), strlen(reply.c_str()), 0);
        if( n > 0 ) {  
            printf("# [%s]: Mensagem devolvida com sucesso.\n",timeStamp());  
            fflush(stdout);
        }  
        shutdown(new_socket,SHUT_RDWR);
        close(new_socket);
        // --- FIM: Seção crítica
        g_num_mutex.unlock(); 
    }
    pthread_exit(NULL);
}
//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {  
    int ckOrderFile, ckOrderFileLog;
    bool ckOrderList, ckOrderLog;
    initCpuPercent(lastTotalUser, lastTotalUserLow, 
                            lastTotalSys, lastTotalIdle);
    std::string logDir = "/var/www/cook_and_waiter/";
    std::string strOrderFileFullPath = logDir + std::string(ORDER_FILE);
    std::string strOrderFileLogFullPath = logDir + std::string(ORDER_FILE_LOG);
    
    printf("# [%s]: Arquivo: %s.\n",timeStamp(),strOrderFileFullPath.c_str()); 
    fflush(stdout);
    printf("# [%s]: Arquivo: %s.\n",timeStamp(),strOrderFileLogFullPath.c_str()); 
    fflush(stdout);
    CTask tTask;
    pthread_t inputTimer, inputListen;
    isIdle = true;
    // Verifica Arquivo
    ckOrderFile = checkTaskFile((char*) strOrderFileFullPath.c_str());
    ckOrderFileLog = checkTaskFile((char*) strOrderFileLogFullPath.c_str());
    if ((ckOrderFile == 0) && (ckOrderFileLog == 0)){
        printf("# [%s]: Arquivos de registros OK.\n",timeStamp());
        fflush(stdout);
    }
    else{
        printf("# [%s]: Falha na carga e leitura dos arquivos de registros. "
            "Order file %d; Log file: %d.\n",timeStamp(),(ckOrderFile == 0),
            (ckOrderFileLog == 0));
        fflush(stdout);
        return 1;
    }
    // --- Faz a leitura do arquivo de entrada --------------------------------
    ckOrderList = listOfTasks.readFileTask((char*) strOrderFileFullPath.c_str());
    ckOrderLog = logListOfTasks.readFileTask((char*) strOrderFileLogFullPath.c_str(),true);
    if (!ckOrderList || !ckOrderLog){
        printf("# [%s]: Falha na associação dos arquivos de registros. Order "
        "file %d; Log file: %d.\n",timeStamp(),ckOrderList,ckOrderLog);
        fflush(stdout);
        return 1;
    }
    // Verifica integridade dos dados e se existia tarefa rodando
    PIDs = getListPid(); 
    printf("# [%s]: Integridade lista %s (%d).\n",timeStamp(),
            listOfTasks.giveIntegrity() ? "ok": "falhou",listOfTasks.size());
    fflush(stdout);            
    printf("# [%s]: Integridade log %s (%d).\n",timeStamp(),
            logListOfTasks.giveIntegrity() ? "ok": "falhou",logListOfTasks.size());
    fflush(stdout);
    // --- Libera arquivo de log da memória -----------------------------------
    logListOfTasks.freeMemoryFileTask();
    printf("# [%s]: N° de processo %zu.\n",timeStamp(),PIDs.size());
    fflush(stdout);
    //  Thread de monitoramento
    pthread_create(&inputTimer, NULL, getRunParam, NULL); //Error here
    std::this_thread::sleep_until(system_clock::now() + milliseconds(1000));
    // Período de latência para ajste das medidas de CPU e memória
    printf("# [%s]: CPU: %5.2f %%, Men: %5.2f %% \n",timeStamp(),percentCPU,percentFreeMemory);
    fflush(stdout);
    printf("# [%s]: Escutando...\n",timeStamp());
    fflush(stdout);
    //  Thread de escuta
    pthread_create(&inputListen, NULL, getListenParam, NULL); //Error here
    
    while(1){
        // printf("# [%s]: CPU: %4.1f (%4.1f), memoria %4.1f (%4.1f), Tasks: %u, Idle %d.\n",
        //     timeStamp(),percentCPU,(float)MAX_CPU_PERCENT,percentFreeMemory,
        //     (float)MIN_MEM_PERCENT,listOfTasks.size(),isIdle);
        // printf("# [%s]: CPU: %d, memoria %d, Tasks: %u, Idle %d.\n",
        //     timeStamp(),(percentCPU < (float)MAX_CPU_PERCENT),(percentFreeMemory > (float)MIN_MEM_PERCENT),
        //     (listOfTasks.size() > 0),isIdle);
        if ((percentCPU < (float)MAX_CPU_PERCENT) && (percentFreeMemory > (float)MIN_MEM_PERCENT) && 
            isIdle && (listOfTasks.size() > 0)){
            // --- INÍCIO: Seção crítica
            g_num_mutex.lock(); 
            tTask = listOfTasks[0];
            tTask.appendUuidOnCommand();
            PIDrunning = executeShellCommandPid(tTask.getCommand().c_str());
            if (PIDrunning > 0){
                // Executando com sucesso
                isIdle = false;
                tTask.setInitTime(timeStamp());
                tTask.setPID(PIDrunning);
                tTask.setStatus(CTASK_RUNNING);
                listOfTasks.setTask(tTask,0);
                listOfTasks.writeFileTask();   
            }
            else{
                // Falha de execução
                tTask.setInitTime(timeStamp());
                tTask.setEndTime(timeStamp());
                tTask.setPID(-1);
                tTask.setStatus(CTASK_FAIL);
                tTask.setPosition(-1);
                tTask.setError("Falha de execução");
                listOfTasks.dequeueTasks(0);
                listOfTasks.writeFileTask();
                logListOfTasks.writeTaskLogFile(tTask);
                isIdle = true;
            }
            // --- FIM: Seção crítica
            g_num_mutex.unlock();
        }
        else{
            std::this_thread::sleep_until(system_clock::now() + milliseconds(50));
        }
    }
    return 0;
}
