// ----------------------------------------------------------------------------
// --- Created by Adelino Silva on 24/12/2023.
// --- mail: adelinocpp@yahoo.com, adelinocpp@gmail.com
// ----------------------------------------------------------------------------
// sudo apt install clang
// sudo apt install libc++-dev
// -> sudo apt install g++-12

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
bool isIdle = false;
CTaskList listOfTasks;
CTaskList logListOfTasks;
vector<int> PIDs;
std::mutex g_num_mutex;


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
    bool getListOfTasks = buffer.find("GET /list_tasks HTTP/1.1") != -1;
    bool postQueue = buffer.find("POST /queue HTTP/1.1") != -1;
    bool postDequeue = buffer.find("POST /dequeue HTTP/1.1") != -1;
    bool postPostpone = buffer.find("POST /postpone HTTP/1.1") != -1;
    int posLeftBlacket = buffer.find("{") ;
    int posRightBlacket = buffer.find("}");
    bool hasBody  = ((posLeftBlacket != -1) && (posRightBlacket != -1));

    Json::Value body, response;
    Json::CharReaderBuilder builderJSON;
    const std::unique_ptr<Json::CharReader> reader(builderJSON.newCharReader());
    std::string rawJson;
    JSONCPP_STRING err;
    bool jsonOK;
    int rawJsonLength;
    Json::StreamWriterBuilder builder;
    std::string json_file;
    // TODO: Implementar as demais requisições
    // TODO: modo verbose
    if (getFirst){
        printf("# [%s]: Primeira Mensagem.\n",timeStamp());
        response["mensagem"] = "Comando não executado";
        json_file = Json::writeString(builder, response);
    } else if (getListOfTasks && hasBody){
        printf("# [%s]: Solicita lista de tarefas.\n",timeStamp());
        // --- TODO: emcapsular estas 3 linhas
        rawJson = buffer.substr(posLeftBlacket,posRightBlacket-posLeftBlacket+1);
        rawJsonLength = static_cast<int>(rawJson.length());
        jsonOK = reader->parse(rawJson.c_str(), rawJson.c_str() + rawJsonLength, &body, &err);
        // --- TODO: encapsular o tratamento 
        if (jsonOK){
            response["lista"] = listOfTasks.getListOfTask();
        }else
            printf("# [%s]: Problema com json: %s.\n",timeStamp(),err.c_str());
        // --------
        json_file = Json::writeString(builder, response);
        return json_file;
    } else if (postQueue && hasBody){
        printf("# [%s]: Adiciona an fila.\n",timeStamp());
        // --- TODO: emcapsular estas 3 linhas
        rawJson = buffer.substr(posLeftBlacket,posRightBlacket-posLeftBlacket+1);
        rawJsonLength = static_cast<int>(rawJson.length());
        jsonOK = reader->parse(rawJson.c_str(), rawJson.c_str() + rawJsonLength, &body, &err);
        // --- TODO: encapsular o tratamento 
        if (jsonOK){
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
        } else
            printf("# [%s]: Problema com json: %s.\n",timeStamp(),err.c_str());
        // --------
        json_file = Json::writeString(builder, response);
        return json_file;
    } else if (postDequeue && hasBody){
        printf("# [%s]: Remove da fila.\n",timeStamp());
        response["mensagem"] = "Comando não executado";
        json_file = Json::writeString(builder, response);
    } else if (postPostpone && hasBody){
        printf("# [%s]: Adia na fila.\n",timeStamp());
        response["mensagem"] = "Comando não executado";
        json_file = Json::writeString(builder, response);
    } else{
        response["mensagem"] = "Comando não executado";
        json_file = Json::writeString(builder, response);
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
            // --- INÍCIO: Seção crítica
            g_num_mutex.lock(); 
            tTask = listOfTasks[0];
            tTask.setEndTime(timeStamp());
            tTask.setStatus(CTASK_FINISH);
            tTask.setPosition(-1);
            logListOfTasks.queueTask(tTask);
            listOfTasks.dequeueTasks(0);
            listOfTasks.writeFileTask();
            logListOfTasks.writeFileTask();
            PIDrunning = -1;
            isIdle = true;
            // --- FIM: Seção crítica
            g_num_mutex.unlock(); 
        }
        sleep_until(system_clock::now() + milliseconds(LOAD_BASE));
    }
    pthread_exit(NULL);
}
//-----------------------------------------------------------------------------
void *getListenParam(void*){
    int opt = true;  
    int main_socket , addrlen , new_socket;   
    struct sockaddr_in address; 
    char buffer[1025];  
    std::string jsonMessage;
    // Criação do socket principal
    if( (main_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {  
        printf("# [%s]: Falha ao iniciar socket.",timeStamp());  
        exit(EXIT_FAILURE);  
    }  
    if( setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
            sizeof(opt)) < 0 ) {  
        printf("# [%s]: Erro ao realizar o setsockopt.",timeStamp());  
        exit(EXIT_FAILURE);  
    }  
    // Definições so socket criado
    address.sin_family = AF_INET; // AF_LOCAL  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  
    // Vincula (bind) o socket a porta PORT (12142)
    if (bind(main_socket, (struct sockaddr *)&address, sizeof(address))<0)  {  
        printf("# [%s]: Falha no bind.",timeStamp());  
        exit(EXIT_FAILURE);  
    }  
    while (1) {
        // Inicio da abertura socket
        printf("\033[A\33[2K\r");
        printf("# [%s]: Escutando porta %d pela thread.\n", timeStamp(), PORT); 
        listen(main_socket,3);
        new_socket = accept(main_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0){
            printf("# [%s]: ERROR on accept.\n",timeStamp());
            break;
        }
        // --- INÍCIO: Seção crítica
        g_num_mutex.lock(); 
        bzero(buffer,1024);
        int n = read(new_socket,buffer,1024);
        if (n < 0){ 
            printf("# [%s]: Erro na leitura de dados do socket.\n",timeStamp());
            g_num_mutex.unlock(); 
            break;
        }
        jsonMessage = solveMessage(buffer);
        string reply = "HTTP/1.1 200 OK\nContent-Type: json\nContent-Length: " +
                        to_string(jsonMessage.size()) + 
                        "\nAccept-Ranges: bytes\nConnection: close\n\n" + jsonMessage;
        n = send(new_socket, reply.c_str(), strlen(reply.c_str()), 0);
        if( n > 0 ) {  
            printf("# [%s]: Mensagem devolvida com sucesso.\n",timeStamp());  
        }  
        shutdown(new_socket,SHUT_RDWR);
        close(new_socket);
        // --- FIM: Seção crítica
        g_num_mutex.unlock(); 
    }
    pthread_exit(NULL);
}
//-----------------------------------------------------------------------------
int main(int argc , char *argv[]) {  
    int ckOrderFile, ckOrderFileLog;
    bool ckOrderList, ckOrderLog;
    initCpuPercent(lastTotalUser, lastTotalUserLow, 
                            lastTotalSys, lastTotalIdle);
    CTask tTask;
    pthread_t inputTimer, inputListen;
    isIdle = true;
    // Verifica Arquivo
    ckOrderFile = checkTaskFile((char*)ORDER_FILE);
    ckOrderFileLog = checkTaskFile((char*)ORDER_FILE_LOG);
    if ((ckOrderFile == 0) && (ckOrderFileLog == 0)){
        printf("# [%s]: Arquivos de registros OK.\n",timeStamp());
    }
    else{
        printf("# [%s]: Falha na carga e leitura dos arquivos de registros. "
            "Order file %d; Log file: %d.\n",timeStamp(),(ckOrderFile == 0),
            (ckOrderFileLog == 0));
        return 1;
    }
    // --- Faz a leitura do arquivo de entrada --------------------------------
    ckOrderList = listOfTasks.readFileTask((char*)ORDER_FILE);
    ckOrderLog = logListOfTasks.readFileTask((char*)ORDER_FILE_LOG,true);
    if (!ckOrderList || !ckOrderLog){
        printf("# [%s]: Falha na associação dos arquivos de registros. Order "
        "file %d; Log file: %d.\n",timeStamp(),ckOrderList,ckOrderLog);
        return 1;
    }
    // Verifica integridade dos dados e se existia tarefa rodando
    PIDs = getListPid(); 
    printf("# [%s]: Integridade lista %s.\n",timeStamp(),
            listOfTasks.giveIntegrity() ? "ok": "falhou");
    printf("# [%s]: Integridade log %s.\n",timeStamp(),
            logListOfTasks.giveIntegrity() ? "ok": "falhou");
    printf("# [%s]: N° de processo %zu.\n",timeStamp(),PIDs.size());
    //  Thread de monitoramento
    pthread_create(&inputTimer, NULL, getRunParam, NULL); //Error here
    sleep_until(system_clock::now() + milliseconds(1000));
    // Período de latência para ajste das medidas de CPU e memória
    printf("# [%s]: CPU: %5.2f, Men: %5.2f\n",timeStamp(),percentCPU,percentFreeMemory);
    printf("# [%s]: Escutando...\n",timeStamp());
    //  Thread de escuta
    pthread_create(&inputListen, NULL, getListenParam, NULL); //Error here
    
    while(1){
        // printf("CPU: %4.1f (%4.1f), memoria %4.1f (%4.1f), Tasks: %u.\n",percentCPU,(float)MAX_CPU_PERCENT,percentFreeMemory,(float)MIN_MEM_PERCENT,listOfTasks.size());
        if ((percentCPU < (float)MAX_CPU_PERCENT) && (percentFreeMemory > (float)MIN_MEM_PERCENT) && 
            isIdle && (listOfTasks.size() > 0)){
            // --- INÍCIO: Seção crítica
            g_num_mutex.lock(); 
            tTask = listOfTasks[0];
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
                logListOfTasks.queueTask(tTask);
                logListOfTasks.writeFileTask();
                isIdle = true;
            }
            // --- FIM: Seção crítica
            g_num_mutex.unlock();
        }
        else
            sleep_until(system_clock::now() + milliseconds(50));
    }
    return 0;
}
