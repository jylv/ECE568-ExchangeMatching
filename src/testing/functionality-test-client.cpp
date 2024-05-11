#include <iostream>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <string>
#include "../Server/BaseSocket.hpp"
using namespace std;

string createXML = "258\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<create>\n\
  <account id=\"123456\" balance=\"100000000\"/>\n\
  <account id=\"654321\" balance=\"1500\"/>\n\
  <symbol sym=\"MONSTER\">\n\
    <account id=\"123456\">20</account>\n\
    <account id=\"654321\">5000</account>\n\
  </symbol>\n\
</create>\n";

string transactionOrderXML1 = "232\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<transactions id=\"123456\">\n\
  <order sym=\"MONSTER\" amount=\"300\" limit=\"125\"/>\n\
  <order sym=\"MONSTER\" amount=\"200\" limit=\"127\"/>\n\
  <order sym=\"MONSTER\" amount=\"400\" limit=\"123\"/>\n\
</transactions>\n";

string transactionOrderXML2 = "235\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<transactions id=\"654321\">\n\
  <order sym=\"MONSTER\" amount=\"-100\" limit=\"130\"/>\n\
  <order sym=\"MONSTER\" amount=\"-500\" limit=\"128\"/>\n\
  <order sym=\"MONSTER\" amount=\"-200\" limit=\"140\"/>\n\
</transactions>\n";

string transactionOrderXML3 = "132\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<transactions id=\"123456\">\n\
  <order sym=\"MONSTER\" amount=\"550\" limit=\"132\"/>\n\
</transactions>\n";

string transactionQueryXML1 = "154\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<transactions id=\"123456\">\n\
  <query id=\"0\"/>\n\
  <query id=\"1\"/>\n\
  <query id=\"2\"/>\n\
  <query id=\"6\"/>\n\
</transactions>\n";

string transactionQueryXML2 = "136\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<transactions id=\"654321\">\n\
  <query id=\"3\"/>\n\
  <query id=\"4\"/>\n\
  <query id=\"5\"/>\n\
</transactions>\n";

void requestService(struct addrinfo &host_info, string &req) {
    try {
        MySocket sock(host_info, "vcm-25032.vm.duke.edu", "12345");
        sock.connectSocket();

        std::cout << "=============Sending=============\n";
        sock.sendData(req);
        std::cout << "=============Receiving=============\n";
        cout << sock.receiveData() << endl;
    } catch (const exception &e) {
        cerr << e.what() << endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const *argv[]) {
    struct addrinfo host_info;

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    requestService(host_info, createXML);
    requestService(host_info, transactionOrderXML1);
    requestService(host_info, transactionOrderXML2);
    requestService(host_info, transactionOrderXML3);
    requestService(host_info, transactionQueryXML1);
    requestService(host_info, transactionQueryXML2);

    return 0;
}
