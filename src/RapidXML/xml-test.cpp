#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "RapidXML/rapidxml_print.hpp"
#include "RapidXML/rapidxml.hpp"

using namespace std;
using namespace rapidxml;

int main(int argc, char const *argv[]) {
    xml_document<char> doc;
    xml_node<char> *root_node = NULL;

    // Read the sample.xml file
    ifstream theFile("sample2.xml");
    vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
    buffer.push_back('\0');

    // Parse the buffer
    doc.parse<0>(&buffer[0]);

    cout << *doc.first_node();
    cout << doc.first_node()->value() << endl;

    // string s = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<MyStudentsData>value</MyStudentsData>";
    // cout << s.size() << endl;
    return 0;
}