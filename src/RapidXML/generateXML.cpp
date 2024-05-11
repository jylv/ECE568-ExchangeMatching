#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include "rapidxml_print.hpp"
#include "rapidxml.hpp"

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

int main(int argc, char const *argv[]) {
    xml_document<> response;

    char *node_name = response.allocate_string("results"); 
    xml_node<> *results = response.allocate_node(node_element, node_name);
	response.append_node(results);

    vector<string> attrNames = {"id", "sym"};
    vector<string> attrValues = {"003", "ssjkd"};
    addChildrenToNode(response, results, "error", "error msg", attrNames, attrValues);

    attrNames = {"id23", "sy323m"};
    attrValues = {"03403", "sskdjnjkd"};
    addChildrenToNode(response, results, "created", "", attrNames, attrValues);

    cout << response;

    cout << "====================\n";
    stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << response;
    cout << ss.str() << endl;
    // char *node_value = doc.allocate_string("Google"); 
    // xml_node<> *node = doc.allocate_node(node_element, "a", node_value);
    // // cout << node->name() << ' ' << node->value() << endl;
    // doc.append_node(node);
    // xml_attribute<> *attr = doc.allocate_attribute("href", "google.com");
    // node->append_attribute(attr);

    // xml_node<> *inner_node = doc.allocate_node(node_element, "b", "Facebook");
    // node->append_node(inner_node);

    // cout << doc;
    return 0;
}
