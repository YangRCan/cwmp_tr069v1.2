/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

int createXML(const char *xmlPath)
{
    XMLDocument doc;
    if (3 != doc.LoadFile(xmlPath))
    {
        cout << "file has been existed !" << endl;
        return 0;
    }

    // 添加申明可以使用如下两行
    XMLDeclaration *declaration = doc.NewDeclaration();
    doc.InsertFirstChild(declaration);

    XMLElement *root = doc.NewElement("XMLUSER");
    doc.InsertEndChild(root);

    XMLElement *userNode = doc.NewElement("User");
    /*添加属性*/
    userNode->SetAttribute("Name", "liangbaikai");
    userNode->SetAttribute("Password ", "bbbbbb");
    root->InsertEndChild(userNode);

    return doc.SaveFile(xmlPath);
}