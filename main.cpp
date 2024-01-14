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
#include <sys/time.h>
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

/*
std::string getEnvVar( std::string const & key ) const
{
    char * val = getenv( key.c_str() );
    return val == NULL ? std::string("") : std::string(val);
}

*/
// ----------------------------------------------------------------------------
char* timeStamp(){
    char* buffer = new char[30];
    char* bReturn = new char[80];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    strftime(buffer,30,"%m-%d-%Y-%T",localtime(&tv.tv_sec));

    sprintf(bReturn,"%s.%03d",buffer,(int)tv.tv_usec/1000);
    return bReturn;
}
// ----------------------------------------------------------------------------
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
        printf("Primeira Mensagem.\n");
    }
    if (getListOfTasks && hasBody){
        printf("Solicita lista de tarefas.\n");

        rawJson = buffer.substr(posLeftBlacket,posRightBlacket-posLeftBlacket+1);
        rawJsonLength = static_cast<int>(rawJson.length());
        jsonOK = reader->parse(rawJson.c_str(), rawJson.c_str() + rawJsonLength, &body, &err);
        
        response["lista"] = listOfTasks.getListOfTask();
        json_file = Json::writeString(builder, response);
        if (jsonOK)
            printf("%s.\n",json_file.c_str());
        else
            printf("Problema com json: %s.\n",err.c_str());
        return json_file;
    }
    if (postQueue && hasBody){
        printf("Adiciona an fila.\n");
        rawJson = buffer.substr(posLeftBlacket,posRightBlacket-posLeftBlacket+1);
        rawJsonLength = static_cast<int>(rawJson.length());
        jsonOK = reader->parse(rawJson.c_str(), rawJson.c_str() + rawJsonLength, &body, &err);
        if (jsonOK){
            if (body["token"] == COMMAND_AUTH){
                CTask recieveTask;
                recieveTask.setCommand(body["command"].asString());
                recieveTask.setTag(body["tag"].asString());
                recieveTask.setSchedTime(timeStamp());
                listOfTasks.queueTask(recieveTask);
                if (listOfTasks.writeFileTask() == 0){
                    // 
                    response["mensagem"] = "Comando executado";
                    response["uuid"] = recieveTask.getUUID();
                }
            }
            else{
                response["mensagem"] = "Comando não executado";
            }
            printf("%s.\n",json_file.c_str());
        }
        else
            printf("Problema com json: %s.\n",err.c_str());
        json_file = Json::writeString(builder, response);
        return json_file;
        // jsonOK = readerJSON.parse( buffer.substr(posLeftBlacket,posRightBlacket-posLeftBlacket).c_str(), body, false );
        // printf("%s.\n",body.asCString());
    }
    if (postDequeue && hasBody){
        printf("Remove da fila.\n");
        // jsonOK = readerJSON.parse( buffer.substr(posLeftBlacket,posRightBlacket-posLeftBlacket).c_str(), body, false );
        // printf("%s.\n",body.asCString());
    }
    if (postPostpone && hasBody){
        printf("Adia na fila.\n");
        // jsonOK = readerJSON.parse( buffer.substr(posLeftBlacket,posRightBlacket-posLeftBlacket).c_str(), body, false );
        // printf("%s.\n",body.asCString());
    }
    // nlohmann::json ex1 = nlohmann::json::parse(buffer.substr(posLeftBlacket,posRightBlacket-posLeftBlacket));
    // printf("%s.\n",ex1.dump().c_str());
    // std::string retKey = "";
    // if (pos != std::string::npos)
    //     retKey = buffer.substr(pos+9,64); 
    return buffer;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void *getRunParam(void*){
    unsigned int nPeriods = 0, i;
    const int time_mult = (int)LOAD_TIME_MEAN/LOAD_BASE;
    double cpu, men, sumCPU = 0, sumMem = 0;
    bool cPIDrunning = false;
    CTask tTask;
    while (1) {
        nPeriods++;
        cpu = getCpuPercent(lastTotalUser, lastTotalUserLow, 
                            lastTotalSys, lastTotalIdle);
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
             printf("File com PID: %d, running: %d.\n",PIDrunning,cPIDrunning);
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
        printf("Falha ao iniciar socket.");  
        exit(EXIT_FAILURE);  
    }  
    if( setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
            sizeof(opt)) < 0 ) {  
        printf("Erro ao realizar o setsockopt.");  
        exit(EXIT_FAILURE);  
    }  
    // Definições so socket criado
    address.sin_family = AF_INET; // AF_LOCAL  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  
    // Vincula (bind) o socket a porta PORT (12142)
    if (bind(main_socket, (struct sockaddr *)&address, sizeof(address))<0)  {  
        printf("Falha no bind.");  
        exit(EXIT_FAILURE);  
    }  
    while (1) {
        // Inicio da abertura socket
        printf("\033[A\33[2K\r");
        printf("[%s] Escutando porta %d pela thread.\n", timeStamp(), PORT); 
        listen(main_socket,3);
        new_socket = accept(main_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0)
            printf("ERROR on accept.\n");
        // --- INÍCIO: Seção crítica
        g_num_mutex.lock(); 
        // close(main_socket);
        bzero(buffer,1024);
        int n = read(new_socket,buffer,1024);
        if (n < 0) 
            printf("Erro na leitura de dados do socket.\n");
        // 
        // g_num_mutex.lock(); 
        jsonMessage = solveMessage(buffer);
        // g_num_mutex.unlock(); 
        //string jsonMessage = "{\"status\":\"ok\",\"pid\":\"1\",\"user\":\"1\"}";        
        string reply = "HTTP/1.1 200 OK\nContent-Type: json\nContent-Length: " +
                        to_string(jsonMessage.size()) + 
                        "\nAccept-Ranges: bytes\nConnection: close\n\n" + jsonMessage;
        n = send(new_socket, reply.c_str(), strlen(reply.c_str()), 0);
        if( n > 0 ) {  
            printf("Mensagem devolvida com sucesso.\n");  
        }  
        shutdown(new_socket,SHUT_RDWR);
        close(new_socket);
        // --- FIM: Seção crítica
        g_num_mutex.unlock(); 
        //sleep_until(system_clock::now() + milliseconds(1000));
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
        printf("#: Arquivos de registros OK.\n");
    }
    else{
        printf("Falha na carga e leitura dos arquivos de registros. Order file %d; Log file: %d.\n",(ckOrderFile == 0),(ckOrderFileLog == 0));
        return 1;
    }
    // Faz a leitura do arquivo de entrada
    ckOrderList = listOfTasks.readFileTask((char*)ORDER_FILE);
    ckOrderLog = logListOfTasks.readFileTask((char*)ORDER_FILE_LOG,true);
    if (!ckOrderList || !ckOrderLog){
        printf("Falha na associação dos arquivos de registros. Order file %d; Log file: %d.\n",ckOrderList,ckOrderLog);
        return 1;
    }
    // Verifica integridade dos dados e se existia tarefa rodando
    PIDs = getListPid(); 
    printf("#: Integridade lista %s.\n",listOfTasks.giveIntegrity() ? "ok": "falhou");
    printf("#: Integridade log %s.\n",logListOfTasks.giveIntegrity() ? "ok": "falhou");
    printf("#: N° de processo %zu.\n",PIDs.size());
    sleep_until(system_clock::now() + milliseconds(5000));
    // +++ Após esta etapa precisa habilitar as seções críticas +++++++++++++++
    //  Thread de monitoramento
    pthread_create(&inputTimer, NULL, getRunParam, NULL); //Error here
    // Período de latência para ajste das medidas de CPU e memória
    printf("#: CPU: %5.2f, Men: %5.2f\n",percentCPU,percentFreeMemory);
    printf("#: Escutando...\n");
    //  Thread de escuta
    pthread_create(&inputListen, NULL, getListenParam, NULL); //Error here
    
    while(1){
        printf("CPU: %4.1f (%4.1f), memoria %4.1f (%4.1f), Tasks: %u.\n",percentCPU,(float)MAX_CPU_PERCENT,percentFreeMemory,(float)MIN_MEM_PERCENT,listOfTasks.size());
        if ((percentCPU < (float)MAX_CPU_PERCENT) && (percentFreeMemory > (float)MIN_MEM_PERCENT) && 
            isIdle && (listOfTasks.size() > 0)){
            
            // --- INÍCIO: Seção crítica
            g_num_mutex.lock(); 
            tTask = listOfTasks[0];
            // std::string runNohupCommand = "nohup " + tTask.getCommand() + " 2>&1 &";
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
            sleep_until(system_clock::now() + milliseconds(1000));
    }
    return 0;
}
