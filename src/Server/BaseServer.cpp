#include "BaseServer.hpp"
#include "../DataBase/Database.hpp"
#include "../Utils/utils.hpp"
#include <string>
#include <vector>
#include <iomanip>
#include <sys/time.h>
#include <sstream>
#include <cmath>
#include <mutex>
#include <thread>
#include <tbb/task.h>
#include <tbb/task_group.h>
#include <tbb/task_scheduler_init.h>
#include <boost/thread.hpp>
#include <fcntl.h>
#include <chrono>

using namespace std;
using namespace rapidxml;
using namespace pqxx;
using namespace tbb;
using namespace std::chrono;

#define MAX_LIMIT 65536

BaseServer::BaseServer(const char *_hostname, 
    const char *_port, int _backlog, int _threadNum) 
    : sock(nullptr), backlog(_backlog), threadNum(_threadNum) {	
	// create, bind, and listen to a socket
	setupServer(_hostname, _port);
}

void BaseServer::setupServer(const char *hostname, const char *port) {
	// create, bind, and listen to a socket
	struct addrinfo host_info;

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

	sock = new MySocket(host_info, hostname, port);
	setSocket(sock);
	sock->bindSocket();
	sock->listenSocket(backlog);
}

BaseServer::~BaseServer() {
    delete sock;
}

void BaseServer::scalabilityTest(int reqNum) {
	tbb::task_scheduler_init init(threadNum);
	task_group group;

	int reqCount = 0;
	static int64_t totalMilliSeconds = 0;
	std::chrono::_V2::system_clock::time_point start;
	std::chrono::_V2::system_clock::time_point stop;
    while (1) {
        if (reqCount == reqNum) {
			group.wait();
			stop = high_resolution_clock::now();
			auto duration = duration_cast<milliseconds>(stop - start);
			totalMilliSeconds = duration.count();
			std::cout << "Processed " << reqNum << " requests in " << totalMilliSeconds / 1000.0 << " seconds\n";
			std::cout << "Throughput is " << reqNum * 1.0 / (totalMilliSeconds / 1000.0) << "req/s\n";
			break;
		}
		
		std::cout << "\n=============Waiting for connection=============\n";
        MySocket *client_sock = sock->acceptConnection();
		if (reqCount == 0) {
			start = high_resolution_clock::now();
		}
		++reqCount;
		
		// thread pool
		group.run([=]{processRequest(client_sock);});

        // thread newThread(&BaseServer::processRequest, this, client_sock);
        // newThread.detach();
		std::cout << "=======================Done=======================\n";
    }
}

void BaseServer::launch() {
	// daemonize();
	// initialize thread pool
	tbb::task_scheduler_init init(threadNum);
	task_group group;

    while (1) {
		std::cout << "\n=============Waiting for connection=============\n";
        MySocket *client_sock = sock->acceptConnection();
		
		// thread pool
		group.run([=]{processRequest(client_sock);});

		std::cout << "=======================Done=======================\n";
    }
	group.wait();
}

void BaseServer::processRequest(MySocket *client_sock) {
	// std::cout << boost::this_thread::get_id() << endl;
	// connect to database, need a new connection for each thread
	connection *C = db.connectToDatabase();

	string rawData = client_sock->receiveData();
	// response xml
	xml_document<> response;

	if (rawData.size() == 0) {
		std::cout << "The other side close connection/ Something went wrong.\n";
		return;
	} 

	try {
		// raw data pre-processing
		string xml = preprocess(rawData);

		// create xml object
		xml_document<char> doc;
		xml_node<char> *root_node = NULL;
		doc.parse<0>(const_cast<char*>(xml.c_str()));
		// std::cout << "======Received XML========:\n";
		// std::cout << doc;
		// std::cout << "======Received XML========\n";

		std::cout << "========Processing XML========\n";
		// check the top-level node, which can only be <create> or <transactions>
		root_node = doc.first_node();
		if (strcmp(root_node->name(), "create") == 0) {
			processCreate(C, response, root_node);
		} else if (strcmp(root_node->name(), "transactions") == 0) {
			processTransaction(C, response, root_node);
		} else {
			std::cout << "The top-level node can only be <create> or <transactions>\n";
			throw FormatError();
		}

		// send back the xml response
		stringstream ss;
		ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		ss << response;
		client_sock->sendData(ss.str());
	}
	catch(const std::exception& e) {
		std::cerr << e.what() << '\n';
	}
	std::cout << "========Processing XML Done========\n";

	C->disconnect();
	delete C;
	delete client_sock;
}

void BaseServer::processCreate(connection * C, xml_document<> &response, xml_node<char> *node) {
	char *node_name = response.allocate_string("results"); 
    xml_node<> *results = response.allocate_node(node_element, node_name);
	response.append_node(results);

	// iterate through all the children nodes
	// the creation should be processed in order
	for(xml_node<> *child_node = node->first_node(); child_node; child_node = child_node->next_sibling()) {
		if (strcmp(child_node->name(), "account") == 0) {
			processCreateAccount(C, child_node, results, response);
		} else if (strcmp(child_node->name(), "symbol") == 0) {
			processCreateSymbol(C, child_node, results, response);
		} else {
			std::cout << "The children of create can only be account or symbol\n";
			throw FormatError();
		}
	}
}

void BaseServer::processCreateAccount(connection *C, xml_node<> *account, xml_node<> *results, xml_document<> &response) {
	string name;
	string value;
	vector<string> attrNames;
	vector<string> attrValues;
	
	double balance;

	rapidxml::xml_attribute<char> *attr = account->first_attribute("id");
	if (!attr) {
		std::cout << "Account should contain attribute id\n";
		throw FormatError();
	}

	string accountId = attr->value();
	// account id length is truncated to 40 chars
	accountId = accountId.substr(0, 40);
	attrNames.push_back("id");
	attrValues.push_back(accountId);
	// std::cout << "start processing account: " << accountId << endl;
	// invalid account number, write error in results
	if (!isValidAccountNum(accountId)) {
		name = "error";
		value = "Either the id attribute not exist or the account id is invalid.";
	} else {
		attr = account->first_attribute("balance");
		if (!attr) {
			std::cout << "Account should contain attribute balance\n";
			throw FormatError();
		}
		if (!isValidPrice(attr->value(), true, &balance)) {
			name = "error";
			value = "Either the balance attribute not exist or the balance is not a valid non-negative fractional.";
		}
	}
	
	// if both account id and balance are valid, start transaction
	if (name != "error") {
		bool retry = true;
		while (retry) {
			try {
				createAccountTransaction(C, accountId, balance, name, value);
				retry = false;
			}
			catch (const pqxx::serialization_failure &e) {
				name = "";
				value = "";
				std::cout << "Serialization failure!!!\n";
			}
		}
	}

	addChildrenToNode(response, results, name, value, attrNames, attrValues);
	// std::cout << "end processing account: " << accountId << endl;
}

void BaseServer::createAccountTransaction(connection *C, string &accountId, double balance, string &name, string &value) {
	/* Create a transactional object. */
    transaction<serializable, read_write> T(*C);

	// check if the account id exists
	stringstream ss;
	ss << "select count(*) from account where account_id = '" << accountId << "';";
	
	// sql query
	result r;
	r = db.query(T, ss.str());

	// if existed already, error
	if (r.begin()[0].as<int>() != 0) {
		name = "error";
		value = "The account already exists.";
	} else {
		// insert into database
		ss = stringstream();
		ss << "INSERT INTO account VALUES ('"<< accountId << "', "<< fixed << balance << ");";
		db.query(T, ss.str());

		name = "created";
	}
	T.commit();
}

void BaseServer::addChildrenToNode(xml_document<> &response, xml_node<> *parent, const string &name, const string &value, vector<string> &attrNames, vector<string> &attrValues) {
	char *node_name = response.allocate_string(name.c_str());
	char *node_value;
	if (value.size() == 0) 
		node_value = nullptr;
	else
		node_value = response.allocate_string(value.c_str());
	xml_node<> *newNode = response.allocate_node(node_element, node_name, node_value);
	parent->append_node(newNode);

	for (size_t i = 0; i < attrNames.size(); ++i) {
		char *attr_name = response.allocate_string(attrNames[i].c_str());
		char *attr_value = response.allocate_string(attrValues[i].c_str());
		xml_attribute<> *newAttr = response.allocate_attribute(attr_name, attr_value);
    	newNode->append_attribute(newAttr);
	}
}

void BaseServer::processCreateSymbol(connection *C, xml_node<> *symbol, xml_node<> *results, xml_document<> &response) {
	string name;
	string value;
	vector<string> attrNames;
	vector<string> attrValues;

	rapidxml::xml_attribute<char> *attr = symbol->first_attribute("sym");
	if (!attr) {
		std::cout << "Symbol should contain attribute sym\n";
		throw FormatError();
	}
	// fetch symbol
	string sym = attr->value();
	sym = sym.substr(0, 40);
	attrNames.push_back("sym");
	attrValues.push_back(sym);
	// std::cout << "start processing symbol: " << sym << endl;
	// invalid symbol, write error in results
	if (!isValidSymbol(sym)) {
		name = "error";
		value = "The symbol contains characters other than alphabets or numbers.";
	} 
	
	for (xml_node<> *child_node = symbol->first_node(); child_node; child_node = child_node->next_sibling()) {
		// child node should be account
		if (strcmp(child_node->name(), "account") != 0) {
			std::cout << "The child node of create symbol should only be account\n";
			throw FormatError();
		}
		
		// fetch account id
		string accountId;
		attr = child_node->first_attribute("id");
		if (!attr) {
			std::cout << "In create symbol, account should contain attribute id\n";
			throw FormatError();
		}
		accountId = attr->value();
		accountId = accountId.substr(0, 40);
		attrNames.push_back("id");
		attrValues.push_back(accountId);

		// if symbol is of wrong format already
		if (name == "error") {
			addChildrenToNode(response, results, name, value, attrNames, attrValues);
			attrNames.pop_back();
			attrValues.pop_back();
			continue;
		}

		// check if NUM is valid
		if (!attr) {
			std::cout << "In create symbol, the num of share should be valid\n";
			throw FormatError();
		}
		double shares = 0;
		if (!isValidPrice(child_node->value(), true, &shares)) {
			name = "error";
			value = "The shares is not a valid non-negative fracitonal.";
		} else {
			// start transaction of creating symbol
			bool retry = true;
			while (retry) {
				try {
					createSymbolTransaction(C, accountId, sym, shares, name, value);
					retry = false;
				}
				catch (const pqxx::serialization_failure &e) {
					name = "";
					value = "";
					std::cout << "Serialization failure!!!\n";
				}
			}
		}

		// write to response xml
		addChildrenToNode(response, results, name, value, attrNames, attrValues);

		// restore condition
		name = string();
		value = string();
		attrNames.pop_back();
		attrValues.pop_back();
	}
	// std::cout << "end processing symbol: " << sym << endl;
}

void BaseServer::createSymbolTransaction(connection *C, string &accountId, string &sym, double shares, string &name, string &value) {
	/* Create a transactional object. */
	transaction<serializable, read_write> T(*C);

	// check if the account id exists
	stringstream ss;
	ss << "select * from account where account_id = '" << accountId << "';";
	result r = db.query(T, ss.str());
	// if not existed, error
	if (r.size() == 0) {
		name = "error";
		value = "The account id not exist.";
	} else {
		// if the account exists
		ss = stringstream();
		ss << "select amount from position where symbol = '" << sym
		<< "' and account_id = '" << accountId << "';";
		r = db.query(T, ss.str());
		// if symbol for this account already exists
		// update shares in the database
		if (r.size() == 1) {
			double amount = r.begin()[0].as<double>();
			amount += shares;

			ss = stringstream();
			ss << "update position set amount = " << fixed << amount << " where symbol = '" << sym
			<< "' and account_id = '" << accountId << "';";
			db.query(T, ss.str());
		} 
		// create a new position entry
		else {
			ss = stringstream();
			ss << "INSERT INTO position VALUES ('" << accountId << "', '" << 
			sym << "', " << fixed << shares << ");";
			db.query(T, ss.str());
		}
		name = "created";
	}
	T.commit();
}

bool BaseServer::checkAccountExistenceTransaction(connection *C, string &accountId) {
	/* Create a transactional object. */
	transaction<serializable, read_write> T(*C);

	// check if the account id exists
	stringstream ss;
	ss << "select * from account where account_id = '" << accountId << "';";
	result r = db.query(T, ss.str());
	T.commit();
	if (r.size() == 0) {
		return false;
	} 
	return true;
}

void BaseServer::executeSellOrder(pqxx::internal::basic_transaction &T, pqxx::result::const_iterator &it, double transactionAmount, double transactionPrice, double baseAmount) {
	stringstream ss;
	ss = stringstream();
	ss << "select balance from account where account_id = '" << it[7].as<string>() << "';";
	result temp = db.query(T, ss.str());
	double orig = temp.begin()[0].as<double>();
	
	// change seller's account balance
	ss << "update account set balance = " << orig + transactionAmount * transactionPrice << " where account_id = '" << it[7].as<string>() << "';";
	db.query(T, ss.str());
	// add a new executed order
	ss = stringstream();
	ss << "insert into orders values (default, " << it[1].as<int>() << ", 'executed', '" << it[3].as<string>() << "', " <<
	-transactionAmount << ", " << transactionPrice << ", " << time(nullptr) << ", '" << it[7].as<string>() << "');";
	db.query(T, ss.str());
	// if sell amount is 0, delete the original order
	if (fabs(it[4].as<double>() + transactionAmount + baseAmount) < 1e-5) {
		ss = stringstream();
		ss << "delete from orders where id = " << it[0].as<int>() << ";";
		db.query(T, ss.str());
	}
	// if is not 0, update the original order instead of deleting
	else {
		ss = stringstream();
		ss << "update orders set amount = "<< it[4].as<double>() + transactionAmount + baseAmount << " where id = " << it[0].as<int>() << ";";
		db.query(T, ss.str());
	}
}

void BaseServer::executeBuyOrder(pqxx::internal::basic_transaction &T, pqxx::result::const_iterator &it, double transactionAmount, double transactionPrice, double baseAmount) {
	stringstream ss;
	
	ss = stringstream();
	ss << "select amount from position where account_id = '" << it[7].as<string>() << "' and symbol = '" << it[3].as<string>() << "';";
	result temp = db.query(T, ss.str());
	double orig = temp.begin()[0].as<double>();

	ss = stringstream();
	// change buyer's number of shares
	ss << "update position set amount = " << orig + transactionAmount << " where account_id = '" << 
	it[7].as<string>() << "' and symbol = '" << it[3].as<string>() << "';";
	db.query(T, ss.str());

	// refund the buyer with the money that deducted greater than it should be when it is first placed
	if (fabs(transactionPrice - it[5].as<double>()) > 1e-5) {
		ss << "update account set balance = balance + " << transactionAmount * fabs(transactionPrice - it[5].as<double>()) << " where account_id = '" << 
		it[7].as<string>() << "';";
		db.query(T, ss.str());
	}
	// add a new executed order
	ss = stringstream();
	ss << "insert into orders values (default, " << it[1].as<int>() << ", 'executed', '" << it[3].as<string>() << "', " <<
	transactionAmount << ", " << transactionPrice << ", " << time(nullptr) << ", '" << it[7].as<string>() << "');";
	db.query(T, ss.str());
	// if buy amount is 0, delete the original order
	if (fabs(it[4].as<double>() - transactionAmount - baseAmount) < 1e-5) {
		ss = stringstream();
		ss << "delete from orders where id = " << it[0].as<int>() << ";";
		db.query(T, ss.str());
	} 
	// if is not 0, update the original order instead of deleting
	else {
		ss = stringstream();
		ss << "update orders set amount = "<< it[4].as<double>() - transactionAmount - baseAmount << " where id = " << it[0].as<int>() << ";";
		db.query(T, ss.str());
	}
}

void BaseServer::matchOrderTransaction(connection *C, int order_id) {
	/* Create a transactional object. */
	transaction<serializable, read_write> T(*C);
	stringstream ss;

	// get the order to match
	ss << "select * from orders where status = 'open' and order_id = " << order_id << ";";
	result currOrder = db.query(T, ss.str());
	if (currOrder.size() == 0)
		return;
	double amount = currOrder.begin()[4].as<double>();
	string symbol = currOrder.begin()[3].as<string>();
	double limit = currOrder.begin()[5].as<double>();
	string accountId = currOrder.begin()[7].as<string>();
	result::const_iterator currOrderIt = currOrder.begin();

	// if this is a buy order
	if (amount >= 0) {
		// look for the sell orders
		ss = stringstream();
		ss << "select * from orders where status = 'open' and symbol = '" << symbol << "' and amount < 0 and price < " << 
		limit << " and account_id != '" << accountId << "' order by price, time;";
		result sells = db.query(T, ss.str());
		result::const_iterator it = sells.begin();

		// execute both buy and sell orders
		double baseAmount = 0;
		while (amount >= 1e-5 && it != sells.end()) {
			// decide the amount and price of this transaction
			double transactionAmount = min(amount, fabs(it[4].as<double>()));
			double transactionPrice = it[5].as<double>();

			executeSellOrder(T, it, transactionAmount, transactionPrice, 0);

			// ss = stringstream();
			// ss << "select amount from position where account_id = '" << currOrderIt[7].as<string>() << "' and symbol = '" << currOrderIt[3].as<string>() << "';";
			// result temp = db.query(T, ss.str());
			// std::cout << temp.begin()[0].as<double>() << endl;

			executeBuyOrder(T, currOrderIt, transactionAmount, transactionPrice, baseAmount);

			amount -= transactionAmount;
			baseAmount += transactionAmount;
			++it;

			// ss = stringstream();
			// ss << "select amount from position where account_id = '" << currOrderIt[7].as<string>() << "' and symbol = '" << currOrderIt[3].as<string>() << "';";
			// temp = db.query(T, ss.str());
			// std::cout << temp.begin()[0].as<double>() << endl;
		}
	}
	// if this is a sell order
	else {
		// look for the buy orders
		ss = stringstream();
		ss << "select * from orders where status = 'open' and symbol = '" << symbol << "' and amount >= 0 and price > " << 
		limit << " and account_id != '" << accountId << "' order by price desc, time;";
		result buys = db.query(T, ss.str());
		result::const_iterator it = buys.begin();

		// execute both buy and sell orders
		double baseAmount = 0;
		while (fabs(amount) >= 1e-5 && it != buys.end()) {
			// decide the amount and price of this transaction
			double transactionAmount = min(fabs(amount), it[4].as<double>());
			double transactionPrice = it[5].as<double>();

			executeBuyOrder(T, it, transactionAmount, transactionPrice, 0);
			executeSellOrder(T, currOrderIt, transactionAmount, transactionPrice, baseAmount);

			amount += transactionAmount;
			baseAmount += transactionAmount;
			++it;
		}
	}
	T.commit();
}

void BaseServer::processEachTransaction(connection *C, xml_document<> &response, xml_node<> *results, xml_node<> *child_node, string &accountId, bool accountExist) {
	if (strcmp(child_node->name(), "order") == 0) {
		string name;
		string value;
		vector<string> attrNames;
		vector<string> attrValues;

		string sym;
		double amount;
		double limit;
		// fetch symbol
		rapidxml::xml_attribute<char> *attr = child_node->first_attribute("sym");
		if (!attr) {
			std::cout << "In transaction, order should contain attribute sym\n";
			throw FormatError();
		}
		sym = attr->value();
		sym = sym.substr(0, 40);
		attrNames.push_back("sym");
		attrValues.push_back(sym);
		// std::cout << "processing order with symbol: " << sym << endl;

		// fetch amount
		attr = child_node->first_attribute("amount");
		if (!attr) {
			std::cout << "In transaction, order should contain attribute amount\n";
			throw FormatError();
		}
		if (!isValidPrice(attr->value(), false, &amount)) {
			name = "error";
			value = "The amount is not a valid fractional.";
		} 
		// if the amount is too close to zero, reject this order
		else if (fabs(amount) < 1e-5) {
			name = "error";
			value = "The amount is too close to 0 which does not make any sense.";
		}
		attrNames.push_back("amount");
		attrValues.push_back(attr->value());

		// fetch limit
		attr = child_node->first_attribute("limit");
		if (!attr) {
			std::cout << "In transaction, order should contain attribute limit\n";
			throw FormatError();
		}
		if (!isValidPrice(attr->value(), true, &limit)) {
			name = "error";
			value = "The limit is not a valid non-negative fractional.";
		}
		attrNames.push_back("limit");
		attrValues.push_back(attr->value());

		// atomically place order
		int order_id = processTransactionOrder(C, child_node, results, response, accountId, accountExist, 
		name, value, attrNames, attrValues, sym, amount, limit);
		
		// atomically match order
		if (order_id != -1) {
			bool retry = true;
			while (retry) {
				try {
					matchOrderTransaction(C, order_id);
					retry = false;
				}
				catch (const pqxx::serialization_failure &e) {
					std::cout << "Serialization failure!!! Retrying...\n";
				}
			}
		}
	} else if (strcmp(child_node->name(), "query") == 0) {
		// std::cout << "processing query" << endl;
		processTransactionQuery(C, child_node, results, response, accountId, accountExist);
	} else if (strcmp(child_node->name(), "cancel") == 0) {
		// std::cout << "processing cancel" << endl;
		processTransactionCancel(C, child_node, results, response, accountId, accountExist);
	} else {
		std::cout << "The child of transaction should only be order/query/cancel\n";
		throw FormatError();
	}
	// std::cout << "processing done" << endl;
}

void BaseServer::processTransaction(connection *C, xml_document<> &response, xml_node<char> *node) {
	char *node_name = response.allocate_string("results"); 
	xml_node<> *results = response.allocate_node(node_element, node_name);
	response.append_node(results);
	
	rapidxml::xml_attribute<char> *attr = node->first_attribute("id");
	if (!attr) {
		std::cout << "Transaction contain attribute id\n";
		throw FormatError();
	}
	string accountId = attr->value();

	bool accountExist;
	bool retry = true;
	while (retry) {
		try {
			accountExist = checkAccountExistenceTransaction(C, accountId);
			retry = false;
		}
		catch (const pqxx::serialization_failure &e) {
			std::cout << "Serialization failure!!! Retrying\n";
		}
	}

	// iterate through all the children nodes
	for(xml_node<> *child_node = node->first_node(); child_node; child_node = child_node->next_sibling()) {
		// spawn a new thread for each transaction
		processEachTransaction(C, response, results, child_node, accountId, accountExist);
	}
}

// place order
int BaseServer::processTransactionOrder(connection *C, xml_node<> *account, xml_node<> *results, xml_document<> &response, string &accountId, bool accountExist, 
string &name, string &value, vector<string> &attrNames, vector<string> &attrValues, string &sym, double amount, double limit) {
	int order_id = -1;
	if (name != "error") {
		if (accountExist) {
			// atomically place order transaction
			bool retry = true;
			while (retry) {
				try {
					order_id = transactionOrderTransaction(C, accountId, sym, amount, limit, name, value);
					retry = false;
				}
				catch (const pqxx::serialization_failure &e) {
					name = "";
					value = "";
					std::cout << "Serialization failure!!! Retrying...\n";
				}
			}
			if (order_id != -1) {
				attrNames.push_back("id");
				attrValues.push_back(to_string(order_id));
			}
		} else {
			name = "error";
			value = "Invalid account id.";
		}
	}

	addChildrenToNode(response, results, name, value, attrNames, attrValues);
	return order_id;
}

void BaseServer::processTransactionQuery(connection *C, xml_node<> *query, xml_node<> *results, xml_document<> &response, string &accountId, bool accountExist) {
	string name = "status";
	string value;
	vector<string> attrNames;
	vector<string> attrValues;
	
	int order_id;

	// fetch transaction id
	rapidxml::xml_attribute<char> *attr = query->first_attribute("id");
	if (!attr) {
		std::cout << "Query should contain attribute id\n";
		throw FormatError();
	}
	if (!accountExist) {
		name = "error";
		value = "The account does not exist.";
	} else if (!isValidInteger(attr->value(), true, &order_id)) {
		name = "error";
		value = "The transaction id is not a valid positive integer.";
	} 
	attrNames.push_back("id");
	attrValues.push_back(attr->value());
	addChildrenToNode(response, results, name, value, attrNames, attrValues);

	// if everything's fine for now
	if (name != "error") {
		rapidxml::xml_node<char> *statusNode = results->last_node("status");

		// get open, executed, and canceled orders
		result opened;
		result executed;
		result canceled;
		bool retry = true;
		while (retry) {
			try {
				getStatusTransaction(C, accountId, order_id, opened, executed, canceled);
				retry = false;
			}
			catch (const pqxx::serialization_failure &e) {
				std::cout << "Serialization failure!!! Retrying...\n";
			}
		}
		// note that there must be at least one zero between opened and canceled
		assert(opened.size() == 0 || canceled.size() == 0);

		// if the transaction id does not belong to this account
		if (opened.size() == 0 && executed.size() == 0 && canceled.size() == 0) {
			name = "error";
			value = "This transaction does not belong to this account.";
			results->remove_node(statusNode);
			addChildrenToNode(response, results, name, value, attrNames, attrValues);
			return;
		}

		// write to result xml
		name = "open";
		attrNames = {"shares"};
		attrValues = vector<string>();
		for (result::const_iterator c = opened.begin(); c != opened.end(); ++c) {
			attrValues.push_back(to_string(c[0].as<double>()));
			addChildrenToNode(response, statusNode, name, value, attrNames, attrValues);
			attrValues.pop_back();
		}

		name = "canceled";
		attrNames = {"shares", "time"};
		for (result::const_iterator c = canceled.begin(); c != canceled.end(); ++c) {
			attrValues = vector<string>();
			attrValues.push_back(to_string(c[0].as<double>()));
			attrValues.push_back(to_string(c[1].as<int>()));
			addChildrenToNode(response, statusNode, name, value, attrNames, attrValues);
		}

		name = "executed";
		attrNames = {"shares", "price", "time"};
		for (result::const_iterator c = executed.begin(); c != executed.end(); ++c) {
			attrValues = vector<string>();
			attrValues.push_back(to_string(c[0].as<double>()));
			attrValues.push_back(to_string(c[1].as<double>()));
			attrValues.push_back(to_string(c[2].as<int>()));
			addChildrenToNode(response, statusNode, name, value, attrNames, attrValues);
		}
	}
}

void BaseServer::processTransactionCancel(connection *C, xml_node<> *cancel, xml_node<> *results, xml_document<> &response, string &accountId, bool accountExist) {
	string name = "canceled";
	string value;
	vector<string> attrNames;
	vector<string> attrValues;
	
	int order_id;

	// fetch transaction id
	rapidxml::xml_attribute<char> *attr = cancel->first_attribute("id");
	if (!attr) {
		std::cout << "Query should contain attribute id\n";
		throw FormatError();
	}
	if (!accountExist) {
		name = "error";
		value = "The account does not exist.";
	} else if (!isValidInteger(attr->value(), true, &order_id)) {
		name = "error";
		value = "The transaction id is not a valid positive integer.";
	} 
	attrNames.push_back("id");
	attrValues.push_back(attr->value());
	addChildrenToNode(response, results, name, value, attrNames, attrValues);

	// if everything's fine for now
	if (name != "error") {
		rapidxml::xml_node<char> *cancelNode = results->last_node("canceled");

		// cancel open part of the order
		// refund the buyer or return the share to the seller
		// get executed, and canceled orders after cancellation
		result executed;
		result canceled;
		bool retry = true;
		while (retry) {
			try {
				processCancelTransaction(C, accountId, order_id, executed, canceled);
				retry = false;
			}
			catch (const pqxx::serialization_failure &e) {
				std::cout << "Serialization failure!!! Retrying...\n";
			}
		}
		assert(canceled.size() <= 1);

		// if the transaction id does not belong to this account
		if (executed.size() == 0 && canceled.size() == 0) {
			name = "error";
			value = "This transaction does not belong to this account.";
			results->remove_node(cancelNode);
			addChildrenToNode(response, results, name, value, attrNames, attrValues);
			return;
		}

		// write to result xml
		name = "canceled";
		attrNames = {"shares", "time"};
		for (result::const_iterator c = canceled.begin(); c != canceled.end(); ++c) {
			attrValues = vector<string>();
			attrValues.push_back(to_string(c[0].as<double>()));
			attrValues.push_back(to_string(c[1].as<int>()));
			addChildrenToNode(response, cancelNode, name, value, attrNames, attrValues);
		}

		name = "executed";
		attrNames = {"shares", "price", "time"};
		for (result::const_iterator c = executed.begin(); c != executed.end(); ++c) {
			attrValues = vector<string>();
			attrValues.push_back(to_string(c[0].as<double>()));
			attrValues.push_back(to_string(c[1].as<double>()));
			attrValues.push_back(to_string(c[2].as<int>()));
			addChildrenToNode(response, cancelNode, name, value, attrNames, attrValues);
		}
	}
}

// cancel and refund atomically
void BaseServer::processCancelTransaction(connection *C, const string &accountId, int orderId, result &executed, result &canceled) {
	/* Create a transactional object. */
	transaction<serializable, read_write> T(*C);

	// get open orders and cancel them
	// first check if the order is buying or selling
	stringstream ss;
	ss << "select symbol, amount, price from orders where status = 'open' and account_id = '" << accountId << "' and order_id = " << orderId << ";";
	result opened = db.query(T, ss.str());
	if (opened.size() != 0) {
		string symbol = opened.begin()[0].as<string>();
		double amount = opened.begin()[1].as<double>();
		double price = opened.begin()[2].as<double>();
		// if the open order is buying
		if (amount >= 0) {
			// refund to buyer's account
			ss = stringstream();
			ss << "update account set balance = balance + " << fixed << amount * price << " where account_id = '" << accountId << "';";
			db.query(T, ss.str());
		}
		// if the open order is selling
		else {
			// restore the shares of seller's position
			ss = stringstream();
			ss << "update position set amount = amount + " << fixed << fabs(amount) << " where account_id = '" << accountId << "' and symbol = '" << symbol << "';";
			db.query(T, ss.str());
		}

		// change open order to canceled
		time_t timeStamp = time(nullptr);
		ss = stringstream();
		ss << "update orders set status = 'canceled', time = " << timeStamp << " where account_id = '"
		<< accountId << "' and order_id = " << orderId << " and status = 'open';";
		db.query(T, ss.str());
	}

	// get executed orders
	ss = stringstream();
	ss << "select amount, price, time from orders where status = 'executed' and account_id = '" << accountId << "' and order_id = " << orderId << ";";
	executed = db.query(T, ss.str());
	// get canceled orders
	ss = stringstream();
	ss << "select amount, time from orders where status = 'canceled' and account_id = '" << accountId << "' and order_id = " << orderId << ";";
	canceled = db.query(T, ss.str());

	T.commit();
}

// get results of open, canceled, and executed orders of specified account and order
void BaseServer::getStatusTransaction(connection *C, const string &accountId, int orderId, result &opened, result &executed, result &canceled) {
	/* Create a transactional object. */
	transaction<serializable, read_write> T(*C);

	// get open orders
	stringstream ss;
	ss << "select amount from orders where status = 'open' and account_id = '" << accountId << "' and order_id = " << orderId << ";";
	opened = db.query(T, ss.str());
	// get executed orders
	ss = stringstream();
	ss << "select amount, price, time from orders where status = 'executed' and account_id = '" << accountId << "' and order_id = " << orderId << ";";
	executed = db.query(T, ss.str());
	// get canceled orders
	ss = stringstream();
	ss << "select amount, time from orders where status = 'canceled' and account_id = '" << accountId << "' and order_id = " << orderId << ";";
	canceled = db.query(T, ss.str());

	T.commit();
}

// place order
int BaseServer::transactionOrderTransaction(connection *C, string &accountId, string &sym, double amount, double limit, string &name, string &value) {
	/* Create a transactional object. */
	transaction<serializable, read_write> T(*C);

	int order_id = -1;
	// if this is a buy order
	// atomically open the order and reduce the balance
	if (amount >= 0) {
		// check if the account balance is sufficient for this order
		stringstream ss;
		ss << "select balance from account where account_id = '" << accountId << "';";
		result r = db.query(T, ss.str());
		double balance = r.begin()[0].as<double>();
		// if is sufficient
		if (balance > limit * amount) {
			balance -= limit * amount;
			// update the database: change balance
			ss = stringstream();
			ss << "update account set balance = " << fixed << balance << " where account_id = '" << accountId << "';";
			db.query(T, ss.str());

			// update the database: create a new order
			order_id = db.createNewOrder(T, "open", sym, amount, limit, accountId);

			name = "opened";
		} 
		// if not sufficient
		else {
			name = "error";
			value = "The account balance is not sufficient for this order.";
		}
	}
	// if this is a sell order
	// atomically open the order and reduce the shares
	else {
		// check if the account own the symbol
		stringstream ss;
		ss << "select amount from position where account_id = '" << accountId << "' and symbol = '" << sym << "';";
		result r = db.query(T, ss.str());
		if (r.size() == 0) {
			name = "error";
			value = "This account does not own any shares of this symbol, namely a short sale.";
		} else {
			double ownedAmount = r.begin()[0].as<double>();
			// if the owned shares is not enough
			if (ownedAmount < fabs(amount)) {
				name = "error";
				value = "For this account, shares of this symbol is not enough for sell.";
			} else {
				ownedAmount += amount;
				// update the database: change shares
				ss = stringstream();
				ss << "update position set amount = " << fixed << ownedAmount << 
				" where account_id = '" << accountId << "' and symbol = '" << sym << "';";
				db.query(T, ss.str());

				// update the database: create a new order
				order_id = db.createNewOrder(T, "open", sym, amount, limit, accountId);

				name = "opened";
			}
		}
	}
	T.commit();
	return order_id;
}

string BaseServer::preprocess(string &rawData) {
	// extract the first line
	size_t pos = rawData.find('\n');
	if (pos == string::npos || !isValidNumber(rawData.substr(0, pos))) {
		// std::cout << "First Line should be a valid number: " << rawData << endl;
		throw FormatError();
	} 
	int xmlLength = stoi(rawData.substr(0, pos));
	string xml = rawData.substr(pos + 1, min((int)rawData.size() - (int)pos - 1, xmlLength));
	if ((int)xml.size() != xmlLength) {
		std::cout << "The length doesn't match\n";
		std::cout << "xml size: " << xml.size() << " first line number:  " << stoi(rawData.substr(0, pos)) << endl;
		throw FormatErrorLength();
	}
	return xml;
}

void BaseServer::daemonize(){
    // fork, create a child process
    pid_t pid = fork();
    // exit the parent process, guaranteed not to be a process group leader
    if (pid != 0) {
        exit(EXIT_SUCCESS);
    } else if (pid == -1) {
        cerr << "During daemonize: First Fork failure\n";
        exit(EXIT_FAILURE);
    }
    // working on the child process
    // create a new session with no controlling tty
    pid_t sid = setsid();
    if (sid == -1) {
        cerr << "During daemonize: Create new session failure\n";
        exit(EXIT_FAILURE);
    }
    // point stdin/stdout/stderr to it
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    
    // change working directory to root
    chdir("/");
    // clear umask
    umask(0);
    // fork again
    pid = fork();
    // exit the parent process, guaranteed not to be a session leader
    if (pid != 0) {
        exit(EXIT_SUCCESS);
    } else if (pid == -1) {
        cerr << "During daemonize: Second Fork failure\n";
        exit(EXIT_FAILURE);
    }
}

// getter functions
int BaseServer::getBacklog() const {
    return backlog;
}
const MySocket *BaseServer::getSocket() const {
    return sock;
}

// setter functions
void BaseServer::setBacklog(int b) {
    backlog = b;
}
void BaseServer::setSocket(MySocket *s) {
    sock = s;
}