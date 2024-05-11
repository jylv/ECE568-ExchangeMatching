CPPFLAGS = -Wall -Werror -pedantic -pthread -std=c++17 -ggdb3
EXTRAFLAGS = -lpqxx -lpq -ltbb -lboost_thread
TARGETS = functionality-test-client scalability-test-client

all: $(TARGETS)

functionality-test-client: functionality-test-client.o ../Server/BaseSocket.o
	g++ $(CPPFLAGS) -o functionality-test-client functionality-test-client.o ../Server/BaseSocket.o $(EXTRAFLAGS)

functionality-test-client.o: functionality-test-client.cpp ../Server/BaseSocket.hpp
	g++ $(CPPFLAGS) -c functionality-test-client.cpp $(EXTRAFLAGS)

scalability-test-client: scalability-test-client.o ../Server/BaseSocket.o ../Utils/utils.o random-generate2.o
	g++ $(CPPFLAGS) -o scalability-test-client scalability-test-client.o ../Server/BaseSocket.o ../Utils/utils.o random-generate2.o $(EXTRAFLAGS)

scalability-test-client.o: scalability-test-client.cpp random-generate2.hpp ../Server/BaseSocket.hpp ../Utils/utils.hpp
	g++ $(CPPFLAGS) -c scalability-test-client.cpp $(EXTRAFLAGS)

random-generate2.o: random-generate2.cpp random-generate2.hpp
	g++ $(CPPFLAGS) -c random-generate2.cpp $(EXTRAFLAGS)

.PHONY clean:
	rm -f *~ *.o functionality-test-client scalability-test-client