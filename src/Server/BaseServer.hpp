#ifndef BASESERVER_H
#define BASESERVER_H

#include "BaseSocket.hpp"
#include "../DataBase/Database.hpp"
#include "../RapidXML/rapidxml_print.hpp"
#include "../RapidXML/rapidxml.hpp"
#include <pqxx/pqxx>
#include <exception>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <memory>

class FormatError : public std::exception
{
public:    
    virtual const char* what() const noexcept {
       return "The data received is of wrong format.";
    }
};

class FormatErrorLength : public std::exception
{
public:    
    virtual const char* what() const noexcept {
       return "The number in the first line does not match the length of the xml received.";
    }
};

class BaseServer
{
private:
    MySocket *sock;
    int backlog;
    int threadNum;
    Database db;
public:
    BaseServer(const char *_hostname, 
        const char *_port, int _backlog, int _threadNum);
    ~BaseServer();
    void setupServer(const char *_hostname, const char *_port);
    void launch();
    void scalabilityTest(int reqNum);
    void daemonize();
    void processRequest(MySocket *);
    std::string preprocess(std::string &rawData);
    void matchOrderTransaction(pqxx::connection *, int order_id);
    void executeSellOrder(pqxx::internal::basic_transaction &T, pqxx::result::const_iterator &it, double transactionAmount, double transactionPrice, double baseAmount);
    void executeBuyOrder(pqxx::internal::basic_transaction &T, pqxx::result::const_iterator &it, double transactionAmount, double transactionPrice, double baseAmount);
    void processCreate(pqxx::connection *, rapidxml::xml_document<> &response, rapidxml::xml_node<char> *node);
    void processTransaction(pqxx::connection *, rapidxml::xml_document<> &response, rapidxml::xml_node<char> *node);
    void processEachTransaction(pqxx::connection *, rapidxml::xml_document<> &response, rapidxml::xml_node<> *results, rapidxml::xml_node<> *child_node, std::string &accountId, bool accountExist);
    int processTransactionOrder(pqxx::connection *, rapidxml::xml_node<> *account, rapidxml::xml_node<> *results, rapidxml::xml_document<> &response, std::string &accountId, bool accountExist, 
    std::string &name, std::string &value, std::vector<std::string> &attrNames, std::vector<std::string> &attrValues, std::string &symbol, double amount, double limit);
    void processTransactionQuery(pqxx::connection *, rapidxml::xml_node<> *account, rapidxml::xml_node<> *results, rapidxml::xml_document<> &response, std::string &accountId, bool accountExist);
    void processTransactionCancel(pqxx::connection *, rapidxml::xml_node<> *account, rapidxml::xml_node<> *results, rapidxml::xml_document<> &response, std::string &accountId, bool accountExist);
    bool checkAccountExistenceTransaction(pqxx::connection *, std::string &accountId);
    int transactionOrderTransaction(pqxx::connection *, std::string &accountId, std::string &sym, double amount, double limit, std::string &name, std::string &value);
    void processCreateAccount(pqxx::connection *, rapidxml::xml_node<> *account, rapidxml::xml_node<> *results, rapidxml::xml_document<> &response);
    void processCreateSymbol(pqxx::connection *, rapidxml::xml_node<> *symbol, rapidxml::xml_node<> *results, rapidxml::xml_document<> &response);
    void createSymbolTransaction(pqxx::connection *, std::string &accountId, std::string &sym, double shares, std::string &name, std::string &value);
    void createAccountTransaction(pqxx::connection *, std::string &accountId, double balance, std::string &name, std::string &value);
    void getStatusTransaction(pqxx::connection *, const std::string &accountId, int orderId, pqxx::result &opened, pqxx::result &executed, pqxx::result &canceled);
    void processCancelTransaction(pqxx::connection *, const std::string &accountId, int orderId, pqxx::result &executed, pqxx::result &canceled);
    void addChildrenToNode(rapidxml::xml_document<> &response, rapidxml::xml_node<> *parent, const std::string &name, const std::string &value, std::vector<std::string> &attrNames, std::vector<std::string> &attrValues);

    // getter 
    int getBacklog() const;
    const MySocket *getSocket() const;

    // setter
    void setBacklog(int);
    void setSocket(MySocket *);
};


#endif /* BASESERVER_H */
