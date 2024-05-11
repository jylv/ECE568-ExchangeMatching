#include <iostream>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <string>
#include "Server/BaseSocket.hpp"
using namespace std;

string createXML = "665\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<create>\n\
  <account id=\"123456\" balance=\"1000.1\"/>\n\
  <account id=\"654321\" balance=\"1500.32\"/>\n\
  <account id=\"111111\" balance=\"5000.0\"/>\n\
  <symbol sym=\"MONSTER\">\n\
    <account id=\"123456\">1000</account>\n\
    <account id=\"654321\">100</account>\n\
    <account id=\"111111\">300</account>\n\
  </symbol>\n\
  <symbol sym=\"WITCHER\">\n\
    <account id=\"123456\">123.32</account>\n\
    <account id=\"654321\">322.23</account>\n\
    <account id=\"111111\">8778.6</account>\n\
  </symbol>\n\
  <symbol sym=\"ELDENRING\">\n\
    <account id=\"123456\">923.4</account>\n\
    <account id=\"654321\">10.54</account>\n\
    <account id=\"111111\">753.8</account>\n\
  </symbol>\n\
</create>\n";

string transactionOrderXML1 = "236\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<transactions id=\"123456\">\n\
  <order sym=\"MONSTER\" amount=\"10\" limit=\"12.2\"/>\n\
  <order sym=\"WITCHER\" amount=\"30\" limit=\"13.7\"/>\n\
  <order sym=\"ELDENRING\" amount=\"24.5\" limit=\"13.2\"/>\n\
</transactions>\n";

string transactionOrderXML2 = "241\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<transactions id=\"654321\">\n\
  <order sym=\"MONSTER\" amount=\"16.3\" limit=\"12.2\"/>\n\
  <order sym=\"WITCHER\" amount=\"3.24\" limit=\"13.7\"/>\n\
  <order sym=\"ELDENRING\" amount=\"-24.5\" limit=\"13.2\"/>\n\
</transactions>\n";

string transactionOrderXML3 = "239\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<transactions id=\"111111\">\n\
  <order sym=\"MONSTER\" amount=\"-45.3\" limit=\"12.2\"/>\n\
  <order sym=\"WITCHER\" amount=\"30\" limit=\"13.7\"/>\n\
  <order sym=\"ELDENRING\" amount=\"78.5\" limit=\"13.2\"/>\n\
</transactions>\n";

string transactionQueryXML = "226\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<transactions id=\"123456\">\n\
  <query id=\"0\"/>\n\
  <query id=\"1\"/>\n\
  <query id=\"2\"/>\n\
  <query id=\"3\"/>\n\
  <query id=\"4\"/>\n\
  <query id=\"5\"/>\n\
  <query id=\"6\"/>\n\
  <query id=\"7\"/>\n\
</transactions>\n";

string transactionCancelXML = "120\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<transactions id=\"123456\">\n\
  <cancel id=\"0\"/>\n\
  <cancel id=\"2\"/>\n\
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
    // cout << createXML.size() << endl;
    // cout << transactionOrderXML1.size() << endl;
    // cout << transactionOrderXML2.size() << endl;
    // cout << transactionOrderXML3.size() << endl;
    // cout << transactionQueryXML.size() << endl;
    // cout << transactionCancelXML.size() << endl;

    struct addrinfo host_info;

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    requestService(host_info, createXML);
    requestService(host_info, transactionOrderXML1);
    requestService(host_info, transactionOrderXML2);
    requestService(host_info, transactionOrderXML3);
    requestService(host_info, transactionCancelXML);
    requestService(host_info, transactionQueryXML);

    return 0;
}
