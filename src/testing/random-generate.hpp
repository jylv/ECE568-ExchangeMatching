#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include "../RapidXML/rapidxml_print.hpp"
#include "../RapidXML/rapidxml.hpp"

using namespace std;
using namespace rapidxml;

void addChildrenToNode(xml_document<> &response, xml_node<> *parent, const string &name, const string &value, vector<string> &attrNames, vector<string> &attrValues) {
    char *node_name = response.allocate_string(name.c_str());
    char *node_value;
    if (value.size() != 0)
        node_value = response.allocate_string(value.c_str());
    else
        node_value = nullptr;
    xml_node<> *newNode = response.allocate_node(node_element, node_name, node_value);
    parent->append_node(newNode);

    for (size_t i = 0; i < attrNames.size(); ++i) {
        char *attr_name = response.allocate_string(attrNames[i].c_str());
        char *attr_value = response.allocate_string(attrValues[i].c_str());
        xml_attribute<> *newAttr = response.allocate_attribute(attr_name, attr_value);
        newNode->append_attribute(newAttr);
    }
}

string generateName(int len){
    stringstream attrName;
    for(int i=0;i<len;i++){
        int c=random()%26;
        char base=random()%2==0?'A':'a';
        char toAppend=c+base;
        attrName << toAppend; 
    }
    return attrName.str();    
}

string generateValue(int len){
    stringstream attrValue; 
    for(int i=0;i<len;i++){
        int c=random()%10;
        char toAppend=c+'0';
        attrValue << toAppend; 
    }
    return attrValue.str(); 
}


void generateAccount(xml_document<> &response,xml_node<> *results ){
    vector<string> attrNames = {"id", "balance"};
    vector<string> attrValues = {generateValue(random()%10), generateValue(random()%10)};
    addChildrenToNode(response, results, "account", "", attrNames, attrValues);
}

void generateSymbol(xml_document<> &response,xml_node<> *results ){
    vector<string> attrNames = {"sym"};
    vector<string> attrValues = {generateName(random()%10)};

    addChildrenToNode(response, results, "symbol", "", attrNames, attrValues); 
    attrNames = {"id"};
    attrValues = {generateValue(random()%10)};
    rapidxml::xml_node<> *n=results->last_node("symbol"); 
    addChildrenToNode(response, n, "account",generateValue(random()%10) , attrNames, attrValues);  
} 

string generateCreate(){
    xml_document<> response;
    vector<string> attrNames = {"id"};
    vector<string> attrValues = {generateValue(random()%10)};
    char *node_name = response.allocate_string("create"); 
    xml_node<> *results = response.allocate_node(node_element, node_name);
    response.append_node(results);
    int ran=random()%20;
    for(int i=0;i<ran;i++){
        if(random()%2==0)generateAccount(response,results);
        else generateSymbol(response,results);
    }

    // cout << "====================\n";
    stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << response;
    string result = ss.str();
    return to_string(result.size()) + "\n" + result;
}

void gennerateQuery(xml_document<> &response,xml_node<> *results){
   vector<string> attrNames = {"id"};
    vector<string> attrValues = {generateValue(random()%10)};
    addChildrenToNode(response, results, "query", "", attrNames, attrValues); 
}

void gennerateCancel(xml_document<> &response,xml_node<> *results){
   vector<string> attrNames = {"id"};
    vector<string> attrValues = {generateValue(random()%10)};
    addChildrenToNode(response, results, "cancel", "", attrNames, attrValues); 
}

void gennerateOrder(xml_document<> &response,xml_node<> *results){
   vector<string> attrNames = {"sym","amount","limit"};
    vector<string> attrValues = {generateName(random()%10),generateValue(random()%10),generateValue(random()%10)};
    addChildrenToNode(response, results, "order", "", attrNames, attrValues); 
}

string generateTransaction(){
    xml_document<> response;
    char *node_name = response.allocate_string("transactions");
    xml_node<> *results = response.allocate_node(node_element, node_name);
    char *attr_value = response.allocate_string("206");
    xml_attribute<> *newAttr = response.allocate_attribute("id", attr_value);
    results->append_attribute(newAttr);
    response.append_node(results);
    int ran=random()%20;
    for(int i=0;i<ran;i++){
        int a=random()%3;
        if(a==0)gennerateQuery(response,results);
        else if(a==1)gennerateCancel(response,results);
        else gennerateOrder(response,results); 
    }

    stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << response;
    string result = ss.str();
    return to_string(result.size()) + "\n" + result;
}
