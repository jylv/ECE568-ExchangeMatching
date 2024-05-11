#ifndef RANDOM_GENERATE_HPP
#define RANDOM_GENERATE_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <boost/thread.hpp>
#include "../RapidXML/rapidxml_print.hpp"
#include "../RapidXML/rapidxml.hpp"

void addChildrenToNode(rapidxml::xml_document<> &response, rapidxml::xml_node<> *parent, const std::string &name, const std::string &value, std::vector<std::string> &attrNames, std::vector<std::string> &attrValues);

std::string generateName(int len);

std::string generateValue(int len);

void generateAccount(rapidxml::xml_document<> &response, rapidxml::xml_node<> *results);

void generateSymbol(rapidxml::xml_document<> &response, rapidxml::xml_node<> *results);

std::string generateCreate();

void gennerateQuery(rapidxml::xml_document<> &response, rapidxml::xml_node<> *results);

void gennerateCancel(rapidxml::xml_document<> &response, rapidxml::xml_node<> *results);

void gennerateOrder(rapidxml::xml_document<> &response, rapidxml::xml_node<> *results);

std::string generateTransaction();
#endif