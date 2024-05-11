CPPFLAGS = -Wall -Werror -pedantic -pthread -std=c++17 -ggdb3
EXTRAFLAGS = -lpqxx -lpq -ltbb -lboost_thread
TARGETS = server scalability-test-server

all: $(TARGETS)

server: server.o Server/BaseServer.o Server/BaseSocket.o Utils/utils.o DataBase/Database.o
	g++ $(CPPFLAGS) -o server server.o Server/BaseServer.o Server/BaseSocket.o Utils/utils.o DataBase/Database.o $(EXTRAFLAGS)

server.o: server.cpp Utils/utils.hpp
	g++ $(CPPFLAGS) -c server.cpp $(EXTRAFLAGS)

scalability-test-server: scalability-test-server.o Server/BaseServer.o Server/BaseSocket.o Utils/utils.o DataBase/Database.o
	g++ $(CPPFLAGS) -o scalability-test-server scalability-test-server.o Server/BaseServer.o Server/BaseSocket.o Utils/utils.o DataBase/Database.o $(EXTRAFLAGS)

scalability-test-server.o: scalability-test-server.cpp Utils/utils.hpp
	g++ $(CPPFLAGS) -c scalability-test-server.cpp $(EXTRAFLAGS)

.PHONY clean:
	rm -f *~ *.o ./Utils/*.o ./Server/*.o ./DataBase/*.o server scalability-test-server