# Nome do programa
PROJ_NAME = cook_and_waiter
# Bibliotecas para linkar
SOURCE_LIBS = -lstdc++ -lpthread
JSONCPP_INCLUDE_PATH = ./jsoncpp/
# Diretivas de compilacao
CPPFLAGS = -MMD -O3 -Wall -std=c++11 -fnon-call-exceptions # -Wreturn-stack-address
# compilador
CPP = clang++
# comando de remocao
RM = rm -rf *.o *.d

build: $(PROJ_NAME)

$(PROJ_NAME): main.o ctask.o ctasklist.o cookfunctions.o jsoncpp.o
	$(CPP) -o $(PROJ_NAME) main.o ctask.o ctasklist.o cookfunctions.o jsoncpp.o $(SOURCE_LIBS) 

main.o: main.cpp
	$(CPP) -o main.o main.cpp -c $(CPPFLAGS)
ctask.o: 
	$(CPP) -o ctask.o CTask.cpp -c $(CPPFLAGS)
ctasklist.o: ctask.o cookfunctions.o
	$(CPP) -o ctasklist.o CTaskList.cpp -c $(CPPFLAGS)
cookfunctions.o:
	$(CPP) -o cookfunctions.o CookFunctions.cpp -c $(CPPFLAGS)
jsoncpp.o: 
	$(CPP) -o jsoncpp.o $(JSONCPP_INCLUDE_PATH)jsoncpp.cpp -c $(CPPFLAGS) -I $(JSONCPP_INCLUDE_PATH)

.PHONY: clean
clean:
	$(RM)

clear all:
	$(RM) $(PROJ_NAME)
