#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <pqxx/pqxx>
#include <string>
#include <ctime>

#define DATABASE "postgres"
#define USER "postgres"
#define PASSWORD "postgres"

class DatabaseConnectionError : public std::exception
{
public:    
    virtual const char* what() const noexcept {
       return "Cannot connect to the database.";
    }
};

class Database
{
public:
    Database();
    ~Database();

    pqxx::connection *connectToDatabase();
    void setup();
    void dropATable(pqxx::connection *, std::string tableName);
    void cleanTables(pqxx::connection *);
    void createTables(pqxx::connection *);

    pqxx::result query(pqxx::internal::basic_transaction &W, const std::string &q);
    int createNewOrder(pqxx::internal::basic_transaction &T, const std::string &status, 
    const std::string &symbol, double amount, double limit, const std::string &accountId);

    // pqxx::connection *getConnection();
};

#endif