/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include "tinyxml2.h"
#include "cwmpclient.h"
#include "cwmp.h"
#include "time_tool.h"
#include "parameter.h"
#include "faultCode.h"
#include "log.h"
#include "xml.h"

namespace tx = tinyxml2;

namespace
{
    // cwmp命名空间数据初始化
    const std::string soap_env_url = "http://schemas.xmlsoap.org/soap/envelope/";
    const std::string soap_enc_url = "http://schemas.xmlsoap.org/soap/encoding/";
    const std::string xsd_url = "http://www.w3.org/2001/XMLSchema";
    const std::string xsi_url = "http://www.w3.org/2001/XMLSchema-instance";
    const std::string cwmp_urls[] = {
        "urn:dslforum-org:cwmp-1-0",
        "urn:dslforum-org:cwmp-1-1",
        "urn:dslforum-org:cwmp-1-2",
        ""};

    cwmp_namespaces ns;

    /**
     * 用于释放 XML 命名空间相关的资源
     * 加上 inline 关键字的函数在编译时可能会被直接嵌入到调用它的地方，而不是像普通函数那样被单独地编译成独立的代码块
     * inline 关键字只是建议性的，编译器并不一定会完全按照指示来处理。过度使用 inline 也可能造成代码体积增大，因为函数的实现会被复制到每个调用的地方，增加了代码的重复性
     */
    inline void xml_free_ns(void)
    {
        int i = 0;
        ns.soap_enc.clear();
        ns.xsd.clear();
        ns.xsi.clear();
        ns.cwmp.clear();
        for (i = 0; i < ARRAY_SIZE(ns.soap_env) && !ns.soap_env[i].empty(); i++)
        { // 循环遍历 ns.soap_env 数组, ARRAY_SIZE 计算了数组中元素的个数
            ns.soap_env[i].clear();
        }
    }

    /**
     * 在xml树中查找是否有标签值为 label 的标签
     */
    tx::XMLElement *findElementBylabel(tx::XMLElement *element, const char *label)
    {
        if (!element)
            return nullptr;
        if (strcmp(element->Value(), label) == 0)
            return element;
        for (tx::XMLElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
        {
            tx::XMLElement *found = findElementBylabel(child, label);
            if (found)
                return found;
        }
        return nullptr;
    }

    /**
     * 在xml树中查找是否有标签值为 value 的标签
     */
    tx::XMLElement *findElementByValue(tx::XMLElement *element, const char *value)
    {
        if (!element)
            return nullptr;
        if (strcmp(element->GetText(), value) == 0)
            return element;
        for (tx::XMLElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
        {
            tx::XMLElement *found = findElementByValue(child, value);
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
        if (!b1)
            return -1;
        // 遍历事件列表
        std::list<event *> events = cwmp->get_event_list();
        for (auto it = events.begin(); it != events.end(); it++)
        {
            node = doc.NewElement("EventStruct");
            if (!node)
                return -1;

            b2 = doc.NewElement("EventCode");
            if (!b2)
                return -1;
            b2->SetText(event_code_array[(*it)->code].code.c_str());
            node->InsertEndChild(b2);

            b2 = doc.NewElement("CommandKey");
            if (!b2)
                return -1;
            if (!(*it)->key.empty())
            {
                b2->SetText((*it)->key.c_str());
            }
            node->InsertEndChild(b2);

            b1->InsertEndChild(node);
            n++;
        }
        // 修改属性值
        if (n)
        {
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

        for (auto it = notifications.begin(); it != notifications.end(); it++)
        {
            b = findElementByValue(parameter_list, (*it)->parameter.c_str());
            if (b)
                continue; // 已存在要上报的参数

            n = doc.NewElement("ParameterValueStruct");
            if (!n)
                return -1;
            parameter_list->InsertEndChild(n);

            b = doc.NewElement("Name");
            if (!b)
                return -1;
            b->SetText((*it)->parameter.c_str());

            b = doc.NewElement("Value");
            if (!b)
                return -1;
            b->SetAttribute("xsi:type", (*it)->type.c_str());
            b->SetText((*it)->value.c_str());

            (*counter)++;
        }
        return 0;
    }

    /**
     * 从 XML 元素节点的属性中获取特定值的属性名，并将结果存储在提供的数组中
     */
    int xml_get_attrname_array(tx::XMLElement *node, std::string value, std::string (&name_arr)[8], int size)
    {
        int j = 0;

        // 检查输入参数的有效性。如果节点为空，节点类型不是MXML_ELEMENT，或者 value 为空，则直接返回 -1，表示出现了错误
        if (!node || value.empty())
            return (-1);
        for (const tx::XMLAttribute *attr = node->FirstAttribute(); attr; attr = attr->Next())
        {
            // 检查当前属性的值是否与输入的 value 值相匹配，并且该属性名的第六个字符是 ':'
            if (!strcmp(attr->Value(), value.c_str()) && *(attr->Name() + 5) == ':')
            {
                name_arr[j++].assign(attr->Name() + 6); // 使用 strdup 复制该属性名（从第七个字符开始）到 name_arr 数组中，并增加 j 的值
            }
            if (j >= size)
                break; // 如果存储到 name_arr 数组中的属性数量已经达到了 size 的限制，就结束循环
        }
        // 如果成功存储了至少一个属性名，则返回 0；如果没有存储任何属性名，则返回 -1，表示未找到匹配的属性
        return (j ? 0 : -1);
    }

    /**
     * 在 XML 树中查找具有特定env类型的节点
     */
    tx::XMLElement *xml_find_node_by_env_type(tx::XMLElement *tree_in, const std::string bname)
    {
        tx::XMLElement *b;
        // 使用 for 循环遍历数组 ns.soap_env
        for (int i = 0; i < ARRAY_SIZE(ns.soap_env) && !ns.soap_env[i].empty(); i++)
        {
            std::string c = ns.soap_env[i] + ":" + bname;
            b = findElementBylabel(tree_in, c.c_str());
            if (b != nullptr)
                return b; // 如果找到节点，返回该节点
        }
        return nullptr; // 未找到
    }

    /**
     * 获取 xml 的命名空间名称
     */
    int xml_recreate_namespace(tx::XMLElement *tree)
    {
        std::string attributeValue;
        const char *c;

        if (ns.cwmp.empty())
        { // 检查特定命名空间是否为 NULL，如果是，则在节点中查找特定属性值来重新设置该命名空间
            for (size_t i = 0; !cwmp_urls[i].empty(); i++)
            {
                for (const tx::XMLAttribute *attribute = tree->FirstAttribute(); attribute; attribute = attribute->Next())
                {
                    if (attributeValue.assign(attribute->Value()) == cwmp_urls[i])
                    {
                        c = attribute->Name();
                        break;
                    }
                }
                if (c && *(c + 5) == ':')
                {
                    ns.cwmp.assign(c + 6);
                    break;
                }
            }
        }

        if (ns.soap_env[0].empty())
        { // 在节点中查找特定属性值来设置 ns.soap_env 数组
            xml_get_attrname_array(tree, soap_env_url, ns.soap_env, ARRAY_SIZE(ns.soap_env));
        }

        // 类似地，在节点中查找特定属性值来设置 ns.soap_enc、ns.xsd、ns.xsi
        if (ns.soap_enc.empty())
        {
            for (const tx::XMLAttribute *attribute = tree->FirstAttribute(); attribute; attribute = attribute->Next())
            {
                if (attributeValue.assign(attribute->Value()) == soap_enc_url)
                {
                    c = attribute->Name();
                    break;
                }
            }
            if (c && (*(c + 5) == ':'))
            {
                ns.soap_enc.assign((c + 6));
            }
        }

        if (ns.xsd.empty())
        {
            for (const tx::XMLAttribute *attribute = tree->FirstAttribute(); attribute; attribute = attribute->Next())
            {
                if (attributeValue.assign(attribute->Value()) == xsd_url)
                {
                    c = attribute->Name();
                    break;
                }
            }
            if (c && (*(c + 5) == ':'))
            {
                ns.xsd.assign((c + 6));
            }
        }

        if (ns.xsi.empty())
        {
            for (const tx::XMLAttribute *attribute = tree->FirstAttribute(); attribute; attribute = attribute->Next())
            {
                if (attributeValue.assign(attribute->Value()) == xsi_url)
                {
                    c = attribute->Name();
                    break;
                }
            }
            if (c && (*(c + 5) == ':'))
            {
                ns.xsi.assign((c + 6));
            }
        }

        for (tx::XMLElement *child = tree->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
        {
            xml_recreate_namespace(child); // 递归所有节点
        }

        if ((!ns.soap_env[0].empty()) && (!ns.cwmp.empty())) // 最后，检查是否成功创建了所有需要的命名空间
            return 0;

        return -1;
    }

    /**
     * 用于从 XML 树中获取特定节点的值并设置 cwmp->hold_requests 的布尔值
     * 查看ACS发送的soap数据xml头部中携带的HoldRequests标签或者NoMoreRequests标签的值
     * <cwmp:HoldRequests soap:mustUnderstand="1">0</cwmp:HoldRequests>
     */
    void xml_get_hold_request(tx::XMLElement *tree)
    {
        tx::XMLElement *b;
        std::string c;

        cwmp->set_hold_requests(false); // 将 cwmp 结构体中的 hold_requests 成员初始化为 false

        c = ns.cwmp + ":" + "NoMoreRequests";
        b = findElementBylabel(tree, c.c_str());
        if (b != nullptr)
        {
            // b = b->FirstChildElement();
            if (b->GetText()) // 如果子节点的值存在
                cwmp->set_hold_requests((atoi(b->GetText())) ? true : false);
        }

        // 同上
        c = ns.cwmp + ":" + "HoldRequests";
        b = findElementBylabel(tree, c.c_str());
        if (b != nullptr)
        {
            // b = b->FirstChildElement();
            if (b->GetText())
                cwmp->set_hold_requests((atoi(b->GetText())) ? true : false);
        }
    }

    int xml_get_rpc_methods(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_set_parameter_values(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_get_parameter_values(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_get_parameter_names(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_set_parameter_attributes(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_download(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_upload(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_factory_reset(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_reboot(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_get_parameter_attributes(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_schedule_inform(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_AddObject(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    int xml_DeleteObject(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out)
    {
        return 0;
    }

    // 支持的RPC方法
    const rpc_method rpc_methods[] = {
        {"GetRPCMethods", xml_get_rpc_methods},
        {"SetParameterValues", xml_set_parameter_values},
        {"GetParameterValues", xml_get_parameter_values},
        {"GetParameterNames", xml_get_parameter_names},
        {"GetParameterAttributes", xml_get_parameter_attributes},
        {"SetParameterAttributes", xml_set_parameter_attributes},
        {"AddObject", xml_AddObject},
        {"DeleteObject", xml_DeleteObject},
        {"Download", xml_download},
        {"Upload", xml_upload},
        {"Reboot", xml_reboot},
        {"FactoryReset", xml_factory_reset},
        // { "ScheduleInform", xml_schedule_inform },
    };

    void xml_do_inform()
    {
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
    if (!tree)
        return -1;

    if (xml_add_cwmpid(tree))
        return -1;

    b = findElementBylabel(tree, "RetryCount");
    if (!b)
        return -1;
    b->SetText(cwmp->get_retry_count());

    b = findElementBylabel(tree, "Manufacturer");
    if (!b)
        return -1;
    b->SetText(di.manufacturer.c_str());

    b = findElementBylabel(tree, "OUI");
    if (!b)
        return -1;
    b->SetText(di.oui.c_str());

    b = findElementBylabel(tree, "ProductClass");
    if (!b)
        return -1;
    b->SetText(di.product_class.c_str());

    b = findElementBylabel(tree, "SerialNumber");
    if (!b)
        return -1;
    b->SetText(di.serial_number.c_str());

    // 修改Event子树内容
    if (xml_prepare_events_inform(doc))
        return -1;

    b = findElementBylabel(tree, "CurrentTime");
    if (!b)
        return -1;
    b->SetText(get_time());

    // 获取需要上报的参数
    InformParameter **inform_parameter = NULL;
    inform_parameter = getInformParameter();
    parameter_list = findElementBylabel(tree, "ParameterList");
    if (!parameter_list)
        return -1;
    for (size_t i = 0; inform_parameter[i] != NULL; i++)
    {
        n = doc.NewElement("ParameterValueStruct");
        if (!n)
            return -1;
        parameter_list->InsertEndChild(n);

        b = doc.NewElement("Name");
        if (!b)
            return -1;
        b->SetText(inform_parameter[i]->name);
        n->InsertEndChild(b);

        b = doc.NewElement("Value");
        if (!b)
            return -1;
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
 * 解析ACS对Inform的响应
 */
int xml_parse_inform_response_message(std::string msg_in)
{
    tx::XMLDocument doc;
    tx::XMLElement *tree, *b;
    char *c;
    int fault = 0;
    doc.Parse(msg_in.c_str());
    tree = doc.RootElement();
    if (!tree)
        return -1;
    // 对全局变量 ns 进行重建
    xml_free_ns(); // 释放 XML 命名空间相关的资源
    if (xml_recreate_namespace(tree))
        return -1;
    // 查找名为 "Fault" 的节点
    b = xml_find_node_by_env_type(tree, "Fault");
    if (b != nullptr)
    {
        // 如果找到 "Fault" 节点，继续查找子节点 "8005"
        b = findElementByValue(b, "8005");
        if (b)
        {
            // 如果找到 "8005" 节点，则表示出现了特定错误
            fault = FAULT_ACS_8005;
            return fault;
        }
        return -1; // 未找到 "8005" 节点，错误处理
    }
    // 获取 hold requests 信息
    xml_get_hold_request(tree); // 用于从 XML 树中获取特定节点的值并设置 cwmp->hold_requests 的布尔值
    // 查找名为 "MaxEnvelopes" 的节点
    b = findElementBylabel(tree, "MaxEnvelopes");
    if (b == nullptr || !(b->GetText()))
        return -1;
    return fault;
}

/**
 * 准备get_rpc消息中的xml数据
 */
int xml_prepare_get_rpc_methods_message(std::string &msg_out)
{
    tx::XMLDocument doc;
    tx::XMLElement *tree;

    doc.Parse(CWMP_GET_RPC_METHOD_MESSAGE);
	tree = doc.RootElement();
	if (!tree) return -1;

	if(xml_add_cwmpid(tree)) return -1;//向传入的 XML 树中添加一个 cwmp:ID 元素的

    tx::XMLPrinter printer;
    doc.Print(&printer);
    msg_out.assign(printer.CStr());
    std::cout << msg_out << std::endl;

	return 0;
}

/**
 * 解析ACS对get_rpc的响应
*/
int xml_parse_get_rpc_methods_response_message(std::string msg_in)
{
    tx::XMLDocument doc;
    doc.Parse(msg_in.c_str());
    tx::XMLElement *tree, *b;
	int fault = 0;//用于表示错误状态

	tree = doc.RootElement();
	if (!tree) return -1;
    xml_free_ns(); // 释放 XML 命名空间相关的资源
	if(xml_recreate_namespace(tree)) return -1;//重新创建 XML 树的命名空间

	b = xml_find_node_by_env_type(tree, "Fault");//在 XML 树中查找具有特定Fault的节点
	if (b != nullptr) {
        b = findElementByValue(b, "8005");
		if (b) {
			fault = FAULT_ACS_8005;//错误为8005
			return fault;
		}
		return fault;
	}

	xml_get_hold_request(tree);//从 XML 树中获取特定节点的值并设置 cwmp->hold_requests 的布尔值
	return fault;
}

/**
 * 解下ACS对transfer_complete的响应
 */
int xml_parse_transfer_complete_response_message(std::string msg_in)
{
    tx::XMLDocument doc;
    doc.Parse(msg_in.c_str());
    tx::XMLElement *tree, *b;
    char *c;
    int fault = 0;

    tree = doc.RootElement();
    if (!tree)
        return -1;
    xml_free_ns(); // 释放 XML 命名空间相关的资源
    if (xml_recreate_namespace(tree))
        return -1; // 重新获取 XML 命名空间
    // 在XML树中查找 Fault 节点。如果找到了 Fault 节点，则进入条件判断
    b = xml_find_node_by_env_type(tree, "Fault");
    if (b)
    {
        // 在 Fault 节点中继续查找特定值为 "8005" 的节点。如果找到了，表示发现了ACS返回的特定错误代码
        b = findElementByValue(b, "8005");
        if (b)
        {
            fault = FAULT_ACS_8005;
            return fault;
        }
        return fault;
    }
    // 用于从 XML 树中获取特定节点的值并设置 cwmp->hold_requests 的布尔值
    xml_get_hold_request(tree);

    return fault;
}

/**
 * 解析ACS发送过来的请求
*/
int xml_handle_message(std::string msg_in, std::string &msg_out)
{
    tx::XMLDocument doc_int, doc_out;
    tx::XMLElement *tree_in = NULL, *tree_out = NULL, *b, *body_out;
	const rpc_method *method;
	int i, code = FAULT_9002;
	std::string c;

    auto handler_fault = [tree_out, &body_out, code, &doc_out, &msg_out](){
        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (body_out == nullptr) return -1;
        // xml_create_generic_fault_message(body_out, code);//待实现
        tx::XMLPrinter printer;
        doc_out.Print(&printer);
        msg_out.assign(printer.CStr());
        std::cout << msg_out << std::endl;//响应soap
        return 0;
    };

    doc_out.Parse(CWMP_RESPONSE_MESSAGE);
	tree_out = doc_out.RootElement();//加载 CWMP_RESPONSE_MESSAGE 并将结果存储在 tree_out 中。如果解析失败，跳转到 error 标签
	if (!tree_out) return -1;

    doc_int.Parse(msg_in.c_str());
	tree_in = doc_int.RootElement();//使用 mxmlLoadString 函数解析输入的 XML 消息（msg_in），并将结果存储在 tree_in 中
	if (!tree_in) return -1;

    xml_free_ns(); // 释放 XML 命名空间相关的资源
	if(xml_recreate_namespace(tree_in)) {//重新创建 XML 命名空间的函数, 失败返回-1, 成功返回0
		code = FAULT_9003;
		return handler_fault();
	}
	/* handle cwmp:ID */
    c = ns.cwmp + ":ID";

    b = findElementBylabel(tree_in, c.c_str());
	/* ACS没有发送ID参数，我们在没有它的情况下还是继续 */
	if (b) {
        if (b->GetText()) {
            c.assign(b->GetText());

            b = findElementBylabel(tree_out, "cwmp:ID");
            if (!b) return -1;
            b->SetText(c.c_str());//与ACS请求体中的id保持一致
        }
    }

	b = xml_find_node_by_env_type(tree_in, "Body");
	if (!b) {
		code = FAULT_9003;
		return handler_fault();
	}

    b = b->FirstChildElement();
    if(!b) {
        code = FAULT_9003;
        return handler_fault();
    }

    c.assign(b->Value());//获取标签名称
	if (c.find(':') != std::string::npos) {//存在冒号
		size_t ns_len = c.find(':');

		if (ns.cwmp.length() != ns_len) {
			code = FAULT_9003;
			return handler_fault();
		}

		if (c.find(ns.cwmp) == std::string::npos) {
			code = FAULT_9003;
			return handler_fault();
		}

        c = c.substr(ns_len + 1);//拷贝冒号后面的标签
	} else {
		code = FAULT_9003;
		return handler_fault();
	}
	method = NULL;
	Log(NAME, L_NOTICE, "received %s method from the ACS\n", c.c_str());
	for (i = 0; i < ARRAY_SIZE(rpc_methods); i++) {
		if (c == rpc_methods[i].name) {
			method = &rpc_methods[i];
			break;
		}
	}
	if (method) {
		if (method->handler(b, tree_in, tree_out)) return -1;
	}
	else {
		code = FAULT_9000;
		return handler_fault();
	}
    tx::XMLPrinter printer;
    doc_out.Print(&printer);
    msg_out.assign(printer.CStr());
    std::cout << msg_out << std::endl;//响应soap
	return 0;
}

/**
 * 退出，清除全局变量 ns 的内容
 */
void xml_exit(void)
{
    xml_free_ns();
}

/**
 * 向传入的 XML 树中添加一个 cwmp:ID 元素的
 */
int xml_add_cwmpid(tx::XMLElement *tree)
{
    tx::XMLElement *b = tree->FirstChildElement("soap_env:Header")->FirstChildElement("cwmp:ID");
    if (!b)
        return -1;              // 失败
    static unsigned int id = 0; // 使其在函数调用之间保持其值，保持递增
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", ++id);
    b->SetText(buf);
    return 0; // 完成
}