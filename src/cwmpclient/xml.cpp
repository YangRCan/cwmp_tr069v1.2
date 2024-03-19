/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include "tinyxml2.h"

namespace tx = tinyxml2;

int createXML(const char *xmlPath)
{
    tx::XMLDocument doc;
    if (3 != doc.LoadFile(xmlPath))
    {
        std::cout << "file has been existed !" << std::endl;
        return 0;
    }

    // 添加申明可以使用如下两行
    tx::XMLDeclaration *declaration = doc.NewDeclaration();
    doc.InsertFirstChild(declaration);

    tx::XMLElement *root = doc.NewElement("XMLUSER");
    doc.InsertEndChild(root);

    tx::XMLElement *userNode = doc.NewElement("User");
    /*添加属性*/
    userNode->SetAttribute("Name", "liangbaikai");
    userNode->SetAttribute("Password ", "bbbbbb");
    root->InsertEndChild(userNode);

    return doc.SaveFile(xmlPath);
}