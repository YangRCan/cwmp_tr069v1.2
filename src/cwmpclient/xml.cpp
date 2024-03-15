// /**
//  * @Copyright : Yangrongcan
//  */
// #include <iostream>
// #include <rapidxml.hpp>
// #include <rapidxml_utils.hpp>
// #include <rapidxml_print.hpp>
// #include "xml.h"

// using namespace rapidxml;

// void testRapidXml()
// {
//     xml_document<> doc;
//     xml_node<> *rot = doc.allocate_node(rapidxml::node_pi, doc.allocate_string("xml version='1.0' encoding='utf-8'"));
//     doc.append_node(rot);
//     xml_node<> *node = doc.allocate_node(node_element, "config", "information");
//     xml_node<> *color = doc.allocate_node(node_element, "color", NULL);
//     doc.append_node(node);
//     node->append_node(color);
//     color->append_node(doc.allocate_node(node_element, "red", "0.1"));
//     color->append_node(doc.allocate_node(node_element, "green", "0.1"));
//     color->append_node(doc.allocate_node(node_element, "blue", "0.1"));
//     color->append_node(doc.allocate_node(node_element, "alpha", "1.0"));

//     xml_node<> *size = doc.allocate_node(node_element, "size", NULL);
//     size->append_node(doc.allocate_node(node_element, "x", "640"));
//     size->append_node(doc.allocate_node(node_element, "y", "480"));
//     node->append_node(size);

//     xml_node<> *mode = doc.allocate_node(rapidxml::node_element, "mode", "screen mode");
//     mode->append_attribute(doc.allocate_attribute("fullscreen", "false"));
//     node->append_node(mode);

//     std::string text;
//     // rapidxml::print(std::back_inserter(text), doc, 0);

//     std::cout << text << std::endl;

//     std::ofstream out("config.xml");
//     out << doc;
// }
#include <iostream>
#include "tinyxml2.h"
using namespace std;
using namespace tinyxml2;

int createXML(const char* xmlPath)
{
    XMLDocument doc;
    if(3 != doc.LoadFile(xmlPath))
    {
        cout <<"file has been existed !"<<endl;
        return 0;
     }   

    //添加申明可以使用如下两行
    XMLDeclaration* declaration=doc.NewDeclaration();
    doc.InsertFirstChild(declaration);

    XMLElement* root=doc.NewElement("XMLUSER");
    doc.InsertEndChild(root);


    XMLElement* userNode = doc.NewElement("User");
    /*添加属性*/
    userNode->SetAttribute("Name","liangbaikai");
    userNode->SetAttribute("Password ","bbbbbb");
    root->InsertEndChild(userNode);

    return doc.SaveFile(xmlPath);
}