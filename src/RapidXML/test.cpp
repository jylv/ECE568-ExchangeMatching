#include <iostream>
#include <fstream>
#include <vector>
#include "rapidxml_print.hpp"
#include "rapidxml.hpp"

using namespace std;
using namespace rapidxml;

xml_document<char> doc;
xml_node<char> *root_node = NULL;
   
int main(void)
{
    cout << "\nParsing my students data (sample.xml)....." << endl;
   
    // Read the sample.xml file
    ifstream theFile("sample.xml");
    vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
    buffer.push_back('\0');

    cout << buffer.size() << endl;
   
    // Parse the buffer
    doc.parse<0>(&buffer[0]);

    cout << doc;
   
    // Find out the root node
    root_node = doc.first_node("MyStudentsData");
   
    // Iterate over the student nodes
    for (xml_node<> *student_node = root_node->first_node(); student_node; student_node = student_node->next_sibling())
    {
        if (string(student_node->name()) == "test") {
            cout << student_node->name() << endl;
            try {
                cout << student_node->first_attribute("c")->value() << endl;
            }
            catch(const std::exception& e) {
                std::cerr << e.what() << '\n';
            }
            cout << student_node->first_attribute("b")->next_attribute("b")->value() << endl;
            continue;
        }
        cout << "\nStudent Type =   " << student_node->first_attribute("student_type")->value();
        cout << endl;
           
            // Interate over the Student Names
        for(xml_node<> *student_name_node = student_node->first_node("Name"); student_name_node; student_name_node = student_name_node->next_sibling())
        {
            cout << "Student Name =   " << student_name_node->value();
            cout << endl;
        }
        cout << endl;
    }
   
    return 0;
}