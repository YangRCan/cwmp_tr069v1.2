/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include "tinyxml2.h"
#include "cwmpclient.h"
#include "cwmp.h"
#include "time_tool.h"
#include "parameter.h"
#include "xml.h"

namespace tx = tinyxml2;

namespace {
    //cwmp命名空间数据初始化
    const std::string soap_env_url = "http://schemas.xmlsoap.org/soap/envelope/";
    const std::string soap_enc_url = "http://schemas.xmlsoap.org/soap/encoding/";
    const std::string xsd_url = "http://www.w3.org/2001/XMLSchema";
    const std::string xsi_url = "http://www.w3.org/2001/XMLSchema-instance";
    const std::string cwmp_urls[] = {
            "urn:dslforum-org:cwmp-1-0", 
            "urn:dslforum-org:cwmp-1-1", 
            "urn:dslforum-org:cwmp-1-2", 
            };

    cwmp_namespaces ns;

    tx::XMLElement *xmlFindElementOpaque(tx::XMLElement *node, tx::XMLElement *top, const char *text, bool descend) {
        // 检查传入的参数是否为空
        if (!node || !top || !text) {
            return nullptr;
        }

        // 从当前节点开始遍历树
        if (descend) {
            node = node->FirstChildElement();
        } else {
            node = node->NextSiblingElement();
        }
        
        // 循环遍历节点，直到找到匹配的节点或遍历完整个树
        while (node != nullptr) {
            // 检查节点内容是否与传入的 text 匹配
            if (node->GetText() != nullptr && strcmp(node->GetText(), text) == 0) {
                return node; // 如果找到匹配节点，返回节点指针
            }

            // 根据 descend 参数决定是否向下遍历树或只在当前级别继续搜索
            if (descend) {
                node = node->FirstChildElement();
            } else {
                node = node->NextSiblingElement();
            }
        }

        return nullptr; // 如果没有找到匹配节点，返回 nullptr
    }

    /**
     * 在xml树中查找是否有标签值为 value 的标签
    */
    tx::XMLElement* findElementBylabel(tx::XMLElement* element, const char* label) {
        if (!element)
            return nullptr;
        if (strcmp(element->Value(), label) == 0)
            return element;
        for (tx::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
            tx::XMLElement* found = findElementBylabel(child, label);
            if (found)
                return found;
        }
        return nullptr;
    }

    /**
     * 在xml树中查找是否有标签值为 value 的标签
    */
    tx::XMLElement* findElementByValue(tx::XMLElement* element, const char* value) {
        if (!element)
            return nullptr;
        if (strcmp(element->GetText(), value) == 0)
            return element;
        for (tx::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
            tx::XMLElement* found = findElementByValue(child, value);
            if (found)
                return found;
        }
        return nullptr;
    }

    /**
     * 生成的XML可能是类似下面这样的结构（仅供参考，实际生成的XML结构会根据事件信息不同而有所差异）
    <Event>
        <EventStruct>
            <EventCode>EventCode1</EventCode>
            <CommandKey>CommandKey1</CommandKey>
        </EventStruct>
        <EventStruct>
            <EventCode>EventCode2</EventCode>
            <CommandKey>CommandKey2</CommandKey>
        </EventStruct>
        <!-- 更多事件结构 -->
    </Event>
    */
    int xml_prepare_events_inform(tx::XMLDocument &doc)
    {
        tx::XMLElement *node, *b1, *b2;
        tx::XMLElement *tree = doc.RootElement();
        int n = 0;
        // 获取Event标签
        b1 = findElementBylabel(tree, "Event");
        if (!b1) return -1;
        // 遍历事件列表
        std::list<event*> events = cwmp->get_event_list();
        for(auto it = events.begin(); it != events.end(); it++) {
            node = doc.NewElement("EventStruct");
            if (!node) return -1;

            b2 = doc.NewElement("EventCode");
            if (!b2) return -1;
            b2->SetText(event_code_array[(*it)->code].code.c_str());
            node->InsertEndChild(b2);

            b2 = doc.NewElement("CommandKey");
            if (!b2) return -1;
            if (!(*it)->key.empty()) {
                b2->SetText((*it)->key.c_str());
            }
            node->InsertEndChild(b2);

            b1->InsertEndChild(node);
            n++;
        }
        // 修改属性值
        if (n) {
            std::string c = "cwmp:EventStruct[" + std::to_string(n) + "]";
            b1->SetAttribute("soap_enc:arrayType", c.c_str());
        }

        return 0;
    }

    /**
     * 检索 notifications 链表，插入要上报的参数
    */
    int xml_prepare_notifications_inform(tx::XMLElement *parameter_list, int *counter, tx::XMLDocument &doc)
    {
        tx::XMLElement *b, *n;
        std::list<notification *> notifications = cwmp->get_notifications();

        for(auto it = notifications.begin(); it != notifications.end(); it++) {
            b = findElementByValue(parameter_list, (*it)->parameter.c_str());
            if (b) continue;//已存在要上报的参数
            
            n = doc.NewElement("ParameterValueStruct");
            if (!n) return -1;
            parameter_list->InsertEndChild(n);

            b = doc.NewElement("Name");
            if (!b) return -1;
            b->SetText((*it)->parameter.c_str());

            b = doc.NewElement("Value");
            if (!b) return -1;
            b->SetAttribute("xsi:type", (*it)->type.c_str());
            b->SetText((*it)->value.c_str());

            (*counter)++;
        }
        return 0;
    }

    int xml_get_rpc_methods(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    int xml_set_parameter_values(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    int xml_get_parameter_values(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out){
        return 0;
    }

    int xml_get_parameter_names(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    int xml_set_parameter_attributes(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    int xml_download(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    int xml_upload(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    int xml_factory_reset(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    int xml_reboot(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    int xml_get_parameter_attributes(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    int xml_schedule_inform(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    int xml_AddObject(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    int xml_DeleteObject(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        return 0;
    }

    // 支持的RPC方法
    const rpc_method rpc_methods[] = {
        { "GetRPCMethods", xml_get_rpc_methods },
        { "SetParameterValues", xml_set_parameter_values },
        { "GetParameterValues", xml_get_parameter_values },
        { "GetParameterNames", xml_get_parameter_names },
        { "GetParameterAttributes", xml_get_parameter_attributes },
        { "SetParameterAttributes", xml_set_parameter_attributes },
        { "AddObject", xml_AddObject },
        { "DeleteObject", xml_DeleteObject },
        { "Download", xml_download },
        { "Upload", xml_upload },
        { "Reboot", xml_reboot },
        { "FactoryReset", xml_factory_reset },
        // { "ScheduleInform", xml_schedule_inform },
    };

    void xml_do_inform() {

    }
}

/**
 * 准备inform消息中的xml数据
*/
int xml_prepare_inform_message(std::string &msg_out)
{
    tx::XMLDocument doc;
	tx::XMLElement *tree, *b, *n, *parameter_list;
	std::string c;
	int counter = 0;
    deviceInfo di = cwmp->get_device_info();

    doc.Parse(CWMP_INFORM_MESSAGE);
    tree = doc.RootElement();
	if (!tree) return -1;

	if(xml_add_cwmpid(tree)) return -1;

    b = findElementBylabel(tree, "RetryCount");
	if (!b) return -1;
	b->SetText(cwmp->get_retry_count());

    b = findElementBylabel(tree, "Manufacturer");
	if (!b) return -1;
    b->SetText(di.manufacturer.c_str());

    b = findElementBylabel(tree, "OUI");
	if (!b) return -1;
    b->SetText(di.oui.c_str());

    b = findElementBylabel(tree, "ProductClass");
	if (!b) return -1;
    b->SetText(di.product_class.c_str());

    b = findElementBylabel(tree, "SerialNumber");
	if (!b) return -1;
    b->SetText(di.serial_number.c_str());
    
    // 修改Event子树内容
	if (xml_prepare_events_inform(doc))
		return -1;

    b = findElementBylabel(tree, "CurrentTime");
	if (!b) return -1;
    b->SetText(get_time());

    // 获取需要上报的参数
    InformParameter **inform_parameter = NULL;
    inform_parameter = getInformParameter();
    parameter_list = findElementBylabel(tree, "ParameterList");
	if (!parameter_list) return -1;
    for (size_t i = 0; inform_parameter[i] != NULL; i++)
    {
        n = doc.NewElement("ParameterValueStruct");
		if (!n) return -1;
        parameter_list->InsertEndChild(n);
        
        b = doc.NewElement("Name");
		if (!b) return -1;
        b->SetText(inform_parameter[i]->name);
        n->InsertEndChild(b);

        b = doc.NewElement("Value");
		if (!b) return -1;
        b->SetText(inform_parameter[i]->data ? inform_parameter[i]->data : "");
        b->SetAttribute("xsi:type", inform_parameter[i]->type ? inform_parameter[i]->type : "string");
        n->InsertEndChild(b);

		counter++;
        free(inform_parameter[i]->name);
        free(inform_parameter[i]->type);
        free(inform_parameter[i]->data);
        free(inform_parameter[i]);
    }
    free(inform_parameter);

	if (xml_prepare_notifications_inform(parameter_list, &counter, doc))
		return -1;

    c = "cwmp:ParameterValueStruct[" + std::to_string(counter) + "]";

    parameter_list->SetAttribute("soap_enc:arrayType", c.c_str());

    tx::XMLPrinter printer;
    doc.Print(&printer);
    msg_out.assign(printer.CStr());
    std::cout << msg_out << std::endl;

	return 0;
}





/**
 * 向传入的 XML 树中添加一个 cwmp:ID 元素的
*/
int xml_add_cwmpid(tx::XMLElement *tree)
{
	tx::XMLElement *b = tree->FirstChildElement("soap_env:Header")->FirstChildElement("cwmp:ID");
    if (!b) return -1;//失败
	static unsigned int id = 0;//使其在函数调用之间保持其值，保持递增
	char buf[16];
	snprintf(buf, sizeof(buf), "%u", ++id);
    b->SetText(buf);
	return 0;//完成
}