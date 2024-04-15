/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include <regex>

#include "tinyxml2.h"
#include "cwmpclient.h"
#include "cwmp.h"
#include "time_tool.h"
#include "parameter.h"
#include "faultCode.h"
#include "backup.h"
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

    int xml_get_rpc_methods(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_set_parameter_values(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_get_parameter_values(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_get_parameter_names(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_set_parameter_attributes(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_download(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_upload(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_factory_reset(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_reboot(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_get_parameter_attributes(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_schedule_inform(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_AddObject(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);
    int xml_DeleteObject(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out);

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
        { "ScheduleInform", xml_schedule_inform },
    };

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

    /**
     * 在ParameterList结构中，查询是否有重复的参数条目
     */
    int xml_check_duplicated_parameter(tx::XMLElement *tree)
    {
        tx::XMLElement *node = findElementBylabel(tree, "ParameterList");
        if (!node)
            return 0;
        node = findElementBylabel(node, "ParameterValueStruct");
        if (!node)
            return 0;

        // 遍历根节点下的所有节点
        for (tx::XMLElement *element = node; element; element = element->NextSiblingElement())
        {
            const char *name1 = element->FirstChildElement("Name")->GetText();
            if (!name1)
                continue;
            for (tx::XMLElement *n = element->NextSiblingElement(); n; n = n->NextSiblingElement())
            {
                const char *name2 = n->FirstChildElement("Name")->GetText();
                if (!strcmp(name1, name2))
                {
                    Log(NAME, L_NOTICE, "Fault in the param: %s , Fault code: 9003 <parameter duplicated>\n", name2);
                    return 1; // 有重复参数
                }
            }
        }
        return 0;
    }

    /**
     * 构造fault xml
     */
    tx::XMLElement *xml_create_generic_fault_message(tx::XMLElement *body, int code, tx::XMLDocument &doc)
    {
        tx::XMLElement *b, *t;
        b = doc.NewElement("soap_env:Fault");
        if (!b)
            return NULL;
        body->InsertEndChild(b);

        t = doc.NewElement("faultcode");
        if (!t)
            return NULL;
        t->SetText(fault_array[code].type);
        b->InsertEndChild(t);

        t = doc.NewElement("faultstring");
        if (!t)
            return NULL;
        t->SetText("CWMP fault");
        b->InsertEndChild(t);

        t = doc.NewElement("detail");
        if (!t)
            return NULL;
        b->InsertEndChild(t);

        b = doc.NewElement("cwmp:Fault"); // 返回的是这个指针
        if (!b)
            return NULL;
        t->InsertEndChild(b);

        t = doc.NewElement("FaultCode");
        if (!t)
            return NULL;
        t->SetText(fault_array[code].code);
        b->InsertEndChild(t);

        t = doc.NewElement("FaultString");
        if (!t)
            return NULL;
        t->SetText(fault_array[code].string);
        b->InsertEndChild(t);

        Log(NAME, L_NOTICE, "send Fault: %s: '%s'\n", fault_array[code].code, fault_array[code].string);
        return b;
    }

    /**
     * 构造SetParameterValue错误响应
     */
    int xml_create_set_parameter_value_fault_message(tx::XMLElement *body, int code, tx::XMLDocument &doc, std::list<param_info> &fp_list)
    {
        struct external_parameter *external_parameter;
        tx::XMLElement *b, *n, *t;
        int index;

        n = xml_create_generic_fault_message(body, code, doc);
        if (!n)
            return -1;

        for (auto it = fp_list.begin(); it != fp_list.end(); it++)
        {
            if (it->fault_code > 0)
            {
                b = doc.NewElement("SetParameterValuesFault");
                if (!b)
                    return -1;
                n->InsertEndChild(b);

                t = doc.NewElement("ParameterName");
                if (!t)
                    return -1;
                t->SetText(it->name);
                b->InsertEndChild(t);

                t = doc.NewElement("FaultCode");
                if (!t)
                    return -1;
                t->SetText(fault_array[it->fault_code].code);
                b->InsertEndChild(t);

                t = doc.NewElement("FaultString");
                if (!t)
                    return -1;
                t->SetText(fault_array[it->fault_code].string);
                b->InsertEndChild(t);
            }
            free(it->name);
        }
        return 0;
    }

    /**
     * GetRPCMethods
     */
    int xml_get_rpc_methods(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *b1, *b2, *method_list;

        b1 = findElementBylabel(tree_out, "soap_env:Body");
        if (!b1)
            return -1;

        b2 = doc_out.NewElement("cwmp:GetRPCMethodsResponse");
        if (!b2)
            return -1;
        b1->InsertEndChild(b2);

        method_list = doc_out.NewElement("MethodList");
        if (!method_list)
            return -1;
        b2->InsertEndChild(method_list);

        for (size_t i = 0; i < ARRAY_SIZE(rpc_methods); i++)
        {
            b1 = doc_out.NewElement("string");
            if (!b1)
                return -1;
            b1->SetText(rpc_methods[i].name.c_str());
            method_list->InsertEndChild(b1);
        }
        std::string attr_value = "xsd:string[" + std::to_string(ARRAY_SIZE(rpc_methods)) + "]";
        method_list->SetAttribute("soap_enc:arrayType", attr_value.c_str());

        Log(NAME, L_NOTICE, "send GetRPCMethodsResponse to the ACS\n");
        return 0;
    }

    /**
     * SetParameterValues
     */
    int xml_set_parameter_values(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        std::list<param_info> fp_list;
        int code = FAULT_9002;
        tx::XMLElement *body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (body_out == nullptr)
            return -1;

        if (xml_check_duplicated_parameter(body_in))
        {
            code = FAULT_9003;
            xml_create_set_parameter_value_fault_message(body_out, code, doc_out, fp_list);
            return 0;
        }

        tx::XMLElement *b = findElementBylabel(body_in, "ParameterValueStruct");
        const char *name = NULL, *value = NULL;
        bool exe_success = true;
        for (tx::XMLElement *n = b; n; n = n->NextSiblingElement())
        {
            name = n->FirstChildElement("Name")->GetText();
            value = n->FirstChildElement("Value")->GetText();
            if (name && value)
            {
                ExecuteResult rlt = setParameter(name, value, NOVERIFY); // 存在内存中，未写入文件
                if (rlt.fault_code)
                {
                    Log(NAME, L_NOTICE, "Fault in the param: %s , Fault code: %s\n", name, fault_array[rlt.fault_code].code);
                    code = FAULT_9003;
                    bool exe_success = false;
                    param_info fp;
                    fp.name = strdup(name);
                    fp.fault_code = rlt.fault_code;
                    fp_list.push_back(fp);
                }
            }
            name = value = NULL;
        }

        tx::XMLElement *pk = findElementBylabel(body_in, "ParameterKey");
        const char *param_key = pk->GetText();
        ExecuteResult rlt;
        if (!param_key)
            rlt = setParameter("Device.ManagementServer.ParameterKey", "", NOVERIFY);
        else
            rlt = setParameter("Device.ManagementServer.ParameterKey", param_key, NOVERIFY);
        if (rlt.fault_code)
        {
            code = FAULT_9003;
            bool exe_success = false;
            Log(NAME, L_NOTICE, "Fault in the param: Device.ManagementServer.ParameterKey , Fault code: %s\n", name, fault_array[rlt.fault_code].code);
        }
        rlt = save_data();
        if (exe_success == false || rlt.status == 0)
        {
            xml_create_set_parameter_value_fault_message(body_out, code, doc_out, fp_list);
            return 0;
        }
        tx::XMLElement *rsp = doc_out.NewElement("cwmp:SetParameterValuesResponse");
        if (!rsp)
            return -1;
        body_out->InsertEndChild(rsp);

        tx::XMLElement *status = doc_out.NewElement("Status");
        if (!b)
            return -1;
        rsp->InsertEndChild(status);
        status->SetText(std::to_string(rlt.status).c_str());

        Log(NAME, L_NOTICE, "send SetParameterValuesResponse to the ACS\n");
        return 0;
    }

    /**
     * GetParameterValues
     */
    int xml_get_parameter_values(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *n, *parameter_list, *b, *body_out, *t;
        int counter = 0, code = FAULT_9002;
        param_info *pf = NULL;
        param_info **pf_list = (param_info **)malloc(sizeof(param_info *));
        pf_list[0] = pf;
        std::list<param_info *> param_list;

        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (!body_out)
            return -1;

        ExecuteResult rlt;
        b = findElementBylabel(body_in, "string");
        for (tx::XMLElement *node = b; node; node = node->NextSiblingElement())
        {
            const char *name = node->GetText();
            rlt = getParameter(name, &pf_list);
            if (rlt.fault_code)
            {
                Log(NAME, L_NOTICE, "Fault in the param: %s , Fault code: %s\n", name, fault_array[rlt.fault_code].code);
                code = rlt.fault_code;
                xml_create_generic_fault_message(body_out, code, doc_out);
                return 0;
            }

            int len = 0;
            while (pf_list[len] != NULL)
            {
                param_list.push_back(pf_list[len]);
                len++;
            }
        }
        free(pf_list);

        n = doc_out.NewElement("cwmp:GetParameterValuesResponse");
        if (!n)
            return -1;
        body_out->InsertEndChild(n);
        parameter_list = doc_out.NewElement("ParameterList");
        if (!parameter_list)
            return -1;
        n->InsertEndChild(parameter_list);

        for (auto it = param_list.begin(); it != param_list.end(); it++)
        {
            n = doc_out.NewElement("ParameterValueStruct");
            if (!n)
                return -1;
            parameter_list->InsertEndChild(n);

            t = doc_out.NewElement("Name");
            if (!t)
                return -1;
            t->SetText((*it)->name);
            n->InsertEndChild(t);

            t = doc_out.NewElement("Value");
            if (!t)
                return -1;
            t->SetAttribute("xsi:type", (*it)->type);
            t->SetText((*it)->data ? (*it)->data : "");
            n->InsertEndChild(t);

            counter++; // 计数
        }

        std::string c = "cwmp:ParameterValueStruct[" + std::to_string(counter) + "]";
        parameter_list->SetAttribute("soap_enc:arrayType", c.c_str());

        Log(NAME, L_NOTICE, "send GetParameterValuesResponse to the ACS\n");
        return 0;
    }

    /**
     * GetParameterNames
     */
    int xml_get_parameter_names(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *n, *parameter_list, *body_out, *t;
        int counter = 0, code = FAULT_9002;
        const char *next_level = NULL;
        ParameterInfoStruct *parameterInfoStruct = NULL; // 结束标志
        ParameterInfoStruct **List = (ParameterInfoStruct **)malloc(sizeof(ParameterInfoStruct *));
        List[0] = parameterInfoStruct;

        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (!body_out)
            return -1;

        n = findElementBylabel(body_in, "NextLevel");
        next_level = n->GetText();
        n = findElementBylabel(body_in, "ParameterPath");
        ExecuteResult rlt;
        if (n->GetText())
        {
            rlt = getParameterNames(n->GetText(), next_level, &List);
        }
        else
        {
            rlt = getParameterNames(NULL, next_level, &List);
        }
        if (rlt.fault_code)
        {
            Log(NAME, L_NOTICE, "Fault in the param: %s , Fault code: %s\n", n->GetText() ? n->GetText() : "NULL", fault_array[rlt.fault_code].code);
            code = rlt.fault_code;
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }

        n = doc_out.NewElement("cwmp:GetParameterNamesResponse");
        if (!n)
            return -1;
        body_out->InsertEndChild(n);

        parameter_list = doc_out.NewElement("ParameterList");
        if (!parameter_list)
            return -1;
        n->InsertEndChild(parameter_list);

        while (List[counter] != NULL)
        {
            n = doc_out.NewElement("ParameterInfoStruct");
            if (!n)
                return -1;
            parameter_list->InsertEndChild(n);

            t = doc_out.NewElement("Name");
            if (!t)
                return -1;
            t->SetText(List[counter]->name);
            n->InsertEndChild(t);

            t = doc_out.NewElement("Writable");
            if (!t)
                return -1;
            int writable = List[counter]->writable; // 隐式类型转换
            t->SetText(std::to_string(writable).c_str());
            n->InsertEndChild(t);

            free(List[counter]->name);
            free(List[counter]);
            counter++;
        }
        free(List);

        std::string c = "cwmp:ParameterInfoStruct[" + std::to_string(counter) + "]";
        parameter_list->SetAttribute("soap_enc:arrayType", c.c_str());

        Log(NAME, L_NOTICE, "send GetParameterNamesResponse to the ACS\n");
        return 0;
    }

    /**
     * GetParameterAttributes
     */
    int xml_get_parameter_attributes(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *n, *parameter_list, *body_out, *t;
        int counter = 0, code = FAULT_9002;

        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (!body_out)
            return -1;

        n = findElementBylabel(body_in, "string");
        const char **param_arry = (const char **)malloc(sizeof(const char *));
        int length = 0;
        for (tx::XMLElement *node = n; node; node = node->NextSiblingElement("string"))
        {
            // 构造字符串数组
            param_arry = (const char **)realloc(param_arry, sizeof(const char *) * (length + 1));
            const char *param = strdup(n->GetText());
            param_arry[length] = param;
            length++;
        }

        ExecuteResult rlt;
        ParameterAttributeStruct **parameter_attributes = getParameterAttributes(param_arry, length, &rlt);
        if (rlt.fault_code)
        {
            code = rlt.fault_code;
            Log(NAME, L_NOTICE, "Fault in getting paramter attribute struct , Fault code: %s\n", fault_array[rlt.fault_code].code);
            xml_create_generic_fault_message(body_out, code, doc_out);

            // 释放空间
            for (size_t i = 0; i < length; i++)
                free(const_cast<char *>(param_arry[i]));
            free(param_arry);

            int index = 0;
            while (parameter_attributes && parameter_attributes[index])
            {
                int len = 0;
                while (parameter_attributes[index]->AccessList[len])
                {
                    free(parameter_attributes[index]->AccessList[len]);
                    len++;
                }
                free(parameter_attributes[index]->AccessList);
                free(parameter_attributes[index]->Name);
                free(parameter_attributes[index]);
                index++;
            }
            if (parameter_attributes)
                free(parameter_attributes);
            return 0;
        }

        n = doc_out.NewElement("cwmp:GetParameterAttributesResponse");
        if (!n)
            return -1;
        body_out->InsertEndChild(n);

        parameter_list = doc_out.NewElement("ParameterList");
        if (!parameter_list)
            return -1;
        n->InsertEndChild(parameter_list);

        int index = 0;
        while (parameter_attributes && parameter_attributes[index])
        {
            n = doc_out.NewElement("ParameterAttributeStruct");
            if (!n)
                return -1;
            parameter_list->InsertEndChild(n);

            t = doc_out.NewElement("Name");
            if (!t)
                return -1;
            t->SetText(parameter_attributes[index]->Name);
            n->InsertEndChild(t);

            t = doc_out.NewElement("Notification");
            if (!t)
                return -1;
            t->SetText(std::to_string(parameter_attributes[index]->Notification).c_str());
            n->InsertEndChild(t);

            t = doc_out.NewElement("AccessList");
            if (!t)
                return -1;
            n->InsertEndChild(t);

            int len = 0;
            while (parameter_attributes[index]->AccessList[len])
            {
                n = doc_out.NewElement("string");
                if (!n)
                    return -1;
                n->SetText(parameter_attributes[index]->AccessList[len]);
                t->InsertEndChild(n);
                free(parameter_attributes[index]->AccessList[len]);
                len++;
            }
            free(parameter_attributes[index]->AccessList);
            free(parameter_attributes[index]->Name);
            free(parameter_attributes[index]);
            index++;
        }
        if (parameter_attributes)
            free(parameter_attributes);

        std::string c = "cwmp:ParameterAttributeStruct[" + std::to_string(index) + "]";
        parameter_list->SetAttribute("soap_enc:arrayType", c.c_str());

        Log(NAME, L_NOTICE, "send GetParameterAttributesResponse to the ACS\n");
        return 0;
    }

    /**
     * SetParameterAttributes
     */
    int xml_set_parameter_attributes(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *body_out, *n, *b;
        int code = FAULT_9002;
        ExecuteResult rlt;
        bool NotificationChange = false, AccessListChange = false;
        ParameterAttributeStruct *param_attr_struct = new ParameterAttributeStruct;

        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (!body_out)
            return -1;

        n = findElementBylabel(body_in, "SetParameterAttributesStruct");
        for (tx::XMLElement *node = n; node; node = node->NextSiblingElement("SetParameterAttributesStruct"))
        {
            b = node->FirstChildElement("Name");
            if (!b)
                return -1;
            param_attr_struct->Name = const_cast<char *>(b->GetText());

            b = node->FirstChildElement("NotificationChange");
            if (!b)
                return -1;
            NotificationChange = strcmp(b->GetText(), "true") == 0 ? true : false;

            b = node->FirstChildElement("Notification");
            if (!b)
                return -1;
            param_attr_struct->Notification = atoi(b->GetText());

            b = node->FirstChildElement("AccessListChange");
            if (!b)
                return -1;
            AccessListChange = strcmp(b->GetText(), "true") == 0 ? true : false;

            param_attr_struct->AccessList = (char **)malloc(sizeof(char *));
            int numOfAccess = 0;
            n = findElementBylabel(n, "string");
            for (tx::XMLElement *target = n; target; target = target->NextSiblingElement("string"))
            {
                param_attr_struct->AccessList = (char **)realloc(param_attr_struct->AccessList, sizeof(char *) * (numOfAccess + 1));
                param_attr_struct->AccessList[numOfAccess] = strdup(target->GetText());
                numOfAccess++;
            }

            rlt = setParameterAttributes(param_attr_struct, NotificationChange, AccessListChange, numOfAccess);
            if (rlt.fault_code)
            {
                code = rlt.fault_code;
                Log(NAME, L_NOTICE, "Fault in the param: %s , Fault code: %s\n", param_attr_struct->Name, fault_array[rlt.fault_code].code);
                xml_create_generic_fault_message(body_out, code, doc_out);

                for (size_t i = 0; i < numOfAccess; i++)
                    free(param_attr_struct->AccessList[i]);
                free(param_attr_struct->AccessList);
                free(param_attr_struct->Name);
                return 0;
            }

            for (size_t i = 0; i < numOfAccess; i++)
                free(param_attr_struct->AccessList[i]);
            free(param_attr_struct->AccessList);
            free(param_attr_struct->Name);
        }

        rlt = save_data();
        if (rlt.fault_code || !rlt.status)
        {
            code = rlt.fault_code;
            Log(NAME, L_NOTICE, "Fault in setting parameter attributes, Fault code: %s\n", fault_array[rlt.fault_code].code);
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }

        b = doc_out.NewElement("cwmp:SetParameterAttributesResponse");
        if (!b)
            return -1;
        body_out->InsertEndChild(b);

        Log(NAME, L_NOTICE, "send SetParameterAttributesResponse to the ACS\n");
        return 0;
    }

    /**
     * Download
     */
    int xml_download(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *body_out, *n, *b, *t;
        std::string download_url, file_size, command_key, file_type, username, password, r;
        int delay = -1, code = FAULT_9002;

        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (!body_out)
            return -1;

        b = findElementBylabel(body_in, "CommandKey");
        if (!b)
            return -1;
        if (b->GetText())
            command_key = b->GetText();
        else
            command_key = "";

        b = findElementBylabel(body_in, "FileType");
        if (!b)
            return -1;
        if (b->GetText())
            file_type = b->GetText();
        else
            file_type = "";

        b = findElementBylabel(body_in, "URL");
        if (!b)
            return -1;
        if (b->GetText())
            download_url = b->GetText();
        else
            download_url = "";

        b = findElementBylabel(body_in, "Username");
        if (!b)
            return -1;
        if (b->GetText())
            username = b->GetText();
        else
            username = "";

        b = findElementBylabel(body_in, "Password");
        if (!b)
            return -1;
        if (b->GetText())
            password = b->GetText();
        else
            password = "";

        b = findElementBylabel(body_in, "FileSize");
        if (!b)
            return -1;
        if (b->GetText())
            file_size = b->GetText();
        else
            file_size = "0";

        b = findElementBylabel(body_in, "DelaySeconds");
        if (!b)
            return -1;
        if (b->GetText())
            delay = atoi(b->GetText());

        if (download_url.empty() || file_size.empty() || command_key.empty() || file_type.empty() || delay < 0)
        {
            code = FAULT_9003;
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }
        std::regex urlRegex("^(http|https)://.*");
        if (!std::regex_match(download_url, urlRegex))
        {
            code = FAULT_9003;
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }
        if (cwmp->get_download_count() >= MAX_DOWNLOAD)
        {
            code = FAULT_9004;
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }
        n = backup_add_download(command_key, delay, file_size, download_url, file_type, username, password);
        cwmp->cwmp_add_download(command_key, delay, file_size, download_url, file_type, username, password, n);

        t = doc_out.NewElement("cwmp:DownloadResponse");
        if (!t)
            return -1;
        body_out->InsertEndChild(t);

        b = doc_out.NewElement("Status");
        if (!b)
            return -1;
        b->SetText("1");
        t->InsertEndChild(b);

        b = doc_out.NewElement("StartTime");
        if (!b)
            return -1;
        b->SetText(UNKNOWN_TIME);
        t->InsertEndChild(b);

        b = doc_out.NewElement("CompleteTime");
        if (!b)
            return -1;
        b->SetText(UNKNOWN_TIME);
        t->InsertEndChild(b);

        Log(NAME, L_NOTICE, "send DownloadResponse to the ACS\n");
        return 0;
    }

    /**
     * Upload
     */
    int xml_upload(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *body_out, *n, *t, *b;
        std::string upload_url, command_key, file_type, username, password;
        int delay = -1, code = FAULT_9002;

        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (!body_out)
            return -1;

        b = findElementBylabel(body_in, "CommandKey");
        if (!b)
            return -1;
        if (b->GetText())
            command_key = b->GetText();
        else
            command_key = "";

        b = findElementBylabel(body_in, "FileType");
        if (!b)
            return -1;
        if (b->GetText())
            file_type = b->GetText();
        else
            file_type = "";

        b = findElementBylabel(body_in, "URL");
        if (!b)
            return -1;
        if (b->GetText())
            upload_url = b->GetText();
        else
            upload_url = "";

        b = findElementBylabel(body_in, "Username");
        if (!b)
            return -1;
        if (b->GetText())
            username = b->GetText();
        else
            username = "";

        b = findElementBylabel(body_in, "Password");
        if (!b)
            return -1;
        if (b->GetText())
            password = b->GetText();
        else
            password = "";

        b = findElementBylabel(body_in, "DelaySeconds");
        if (!b)
            return -1;
        if (b->GetText())
            delay = atoi(b->GetText());

        if (upload_url.empty() || command_key.empty() || file_type.empty() || username.empty() || password.empty() || delay < 0)
        {
            code = FAULT_9003;
            xml_create_generic_fault_message(body_out, code, doc_out);
            return -1;
        }
        std::regex urlRegex("^((https?|ftp|file)://)?([a-zA-Z0-9.-]+(\\.[a-zA-Z]{2,})+)(:\\d+)?(/\\S*)?$");
        if (!std::regex_match(upload_url, urlRegex))
        {
            code = FAULT_9003;
            xml_create_generic_fault_message(body_out, code, doc_out);
            return -1;
        }
        if (cwmp->get_upload_count() >= MAX_UPLOAD)
        {
            code = FAULT_9004;
            xml_create_generic_fault_message(body_out, code, doc_out);
            return -1;
        }
        n = backup_add_upload(command_key, delay, upload_url, file_type, username, password);
        cwmp->cwmp_add_upload(command_key, delay, upload_url, file_type, username, password, n);

        t = doc_out.NewElement("cwmp:UploadResponse");
        if (!t)
            return -1;
        body_out->InsertEndChild(t);

        b = doc_out.NewElement("Status");
        if (!b)
            return -1;
        b->SetText("1");
        t->InsertEndChild(b);

        b = doc_out.NewElement("StartTime");
        if (!b)
            return -1;
        b->SetText(UNKNOWN_TIME);
        t->InsertEndChild(b);

        b = doc_out.NewElement("CompleteTime");
        if (!b)
            return -1;
        b->SetText(UNKNOWN_TIME);
        t->InsertEndChild(b);

        Log(NAME, L_NOTICE, "send UploadResponse to the ACS\n");
        return 0;
    }

    /**
     * FactoryReset
     */
    int xml_factory_reset(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *body_out, *b;

        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (!body_out)
            return -1;

        b = doc_out.NewElement("cwmp:FactoryResetResponse");
        if (!b)
            return -1;
        body_out->InsertEndChild(b);

        cwmp->cwmp_add_handler_end_session(ENDS_FACTORY_RESET);

        Log(NAME, L_NOTICE, "send FactoryResetResponse to the ACS\n");
        return 0;
    }

    /**
     * Reboot
     */
    int xml_reboot(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *body_out, *b;
        std::string command_key;
        int code = FAULT_9002;

        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (!body_out)
            return -1;

        b = findElementBylabel(body_in, "CommandKey");
        if (!b)
        {
            code = FAULT_9003;
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }
        if (b->GetText())
            command_key = b->GetText();
        else
            command_key = "";

        b = doc_out.NewElement("cwmp:RebootResponse");
        if (!b)
            return -1;
        body_out->InsertEndChild(b);

        backup_add_event(EVENT_M_REBOOT, command_key, 0);
        cwmp->cwmp_add_handler_end_session(ENDS_REBOOT);

        Log(NAME, L_NOTICE, "send RebootResponse to the ACS\n");
        return 0;
    }

    /**
     * ScheduleInform
     */
    int xml_schedule_inform(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *body_out, *b;
        std::string command_key, delay_seconds;
        int delay = 0, code = FAULT_9002;

        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (!body_out)
            return -1;

        b = findElementBylabel(body_in, "CommandKey");
        if (!b)
        {
            code = FAULT_9003;
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }
        if (b->GetText())
            command_key = b->GetText();
        else
            command_key = "";

        b = findElementBylabel(body_in, "DelaySeconds");
        if (!b)
        {
            code = FAULT_9003;
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }
        if (b->GetText())
            delay = atoi(b->GetText());
        else
        {
            code = FAULT_9003;
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }

        cwmp->cwmp_add_scheduled_inform(command_key, delay);
        b = doc_out.NewElement("cwmp:ScheduleInformResponse");
        if (!b)
            return -1;
        body_out->InsertEndChild(b);

        Log(NAME, L_NOTICE, "send ScheduleInformResponse to the ACS\n");
        return 0;
    }

    /**
     * AddObject
     */
    int xml_AddObject(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *body_out, *b, *t;
        const char *object_name = NULL, *param_key = NULL;
        char *instance = NULL;
        int code = FAULT_9002;

        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (!body_out)
            return -1;

        b = findElementBylabel(body_in, "ObjectName");
        if (!b)
        {
            code = FAULT_9003;
            Log(NAME, L_NOTICE, "Fault in the param: %s, Fault code: %s\n", "", fault_array[code].code);
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }
        if (b->GetText())
            object_name = b->GetText();
        else
            object_name = "";

        b = findElementBylabel(body_in, "ParameterKey");
        if (!b)
        {
            code = FAULT_9003;
            Log(NAME, L_NOTICE, "Fault in the param: %s, Fault code: %s\n", object_name, fault_array[code].code);
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }
        if (b->GetText())
            param_key = b->GetText();
        else
            param_key = "";

        ExecuteResult rlt;
        rlt = addObject(object_name, &instance);
        if (rlt.fault_code)
        {
            code = rlt.fault_code;
            Log(NAME, L_NOTICE, "Fault in the param: %s, Fault code: %s\n", object_name, fault_array[code].code);
            xml_create_generic_fault_message(body_out, code, doc_out);
            free(instance);
            return 0;
        }
        if (!rlt.status || !instance)
        {
            code = FAULT_9002;
            Log(NAME, L_NOTICE, "Fault in the param: %s, Fault code: %s\n", object_name, fault_array[code].code);
            xml_create_generic_fault_message(body_out, code, doc_out);
            free(instance);
            return 0;
        }

        save_data();
        setParameter("Device.ManagementServer.ParameterKey", param_key, NOVERIFY);

        t = doc_out.NewElement("cwmp:AddObjectResponse");
        if (!t)
            return -1;
        body_out->InsertEndChild(t);

        b = doc_out.NewElement("InstanceNumber");
        if (!b)
            return -1;
        b->SetText(instance);
        t->InsertEndChild(b);

        b = doc_out.NewElement("Status");
        if (!b)
            return -1;
        b->SetText(std::to_string(rlt.status).c_str());
        t->InsertEndChild(b);

        free(instance);

        Log(NAME, L_NOTICE, "send AddObjectResponse to the ACS\n");
        return 0;
    }

    /**
     * DeleteObject
     */
    int xml_DeleteObject(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out, tx::XMLDocument &doc_out)
    {
        tx::XMLElement *body_out, *t, *b;
        const char *object_name = NULL, *param_key = NULL;
	    int code = FAULT_9002;

        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if(!body_out) return -1;

        b = findElementBylabel(body_in, "ObjectName");
        if(!b) {
            code = FAULT_9003;
            Log(NAME, L_NOTICE, "Fault in the param: %s, Fault code: %s\n", "", fault_array[code].code);
            xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }
        if(b->GetText()) object_name = b->GetText();
        else object_name = "";

        b = findElementBylabel(body_in, "ParameterKey");
        if(!b) {
            code = FAULT_9003;
            Log(NAME, L_NOTICE, "Fault in the param: %s, Fault code: %s\n", object_name, fault_array[code].code);
	        xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }
        if(b->GetText()) param_key = b->GetText();
        else param_key = "";

        ExecuteResult rlt = deleteObject(object_name);
        if(rlt.fault_code) {
            code = FAULT_9003;
            Log(NAME, L_NOTICE, "Fault in the param: %s, Fault code: %s\n", object_name, fault_array[code].code);
	        xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }
        if(!rlt.status) {
            code = FAULT_9002;
            Log(NAME, L_NOTICE, "Fault in the param: %s, Fault code: %s\n", object_name, fault_array[code].code);
	        xml_create_generic_fault_message(body_out, code, doc_out);
            return 0;
        }

        save_data();
        setParameter("Device.ManagementServer.ParameterKey", param_key, NOVERIFY);

        t = doc_out.NewElement("cwmp:DeleteObjectResponse");
        if(!t) return -1;
        body_out->InsertEndChild(t);

        b = doc_out.NewElement("Status");
        if(!b) return -1;
        t->SetText(std::to_string(rlt.status).c_str());
        t->InsertEndChild(b);

	    Log(NAME, L_NOTICE, "send DeleteObjectResponse to the ACS\n");
        return 0;
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
    param_info **inform_parameter = NULL;
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
    if (!tree)
        return -1;

    if (xml_add_cwmpid(tree))
        return -1; // 向传入的 XML 树中添加一个 cwmp:ID 元素的

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
    int fault = 0; // 用于表示错误状态

    tree = doc.RootElement();
    if (!tree)
        return -1;
    xml_free_ns(); // 释放 XML 命名空间相关的资源
    if (xml_recreate_namespace(tree))
        return -1; // 重新创建 XML 树的命名空间

    b = xml_find_node_by_env_type(tree, "Fault"); // 在 XML 树中查找具有特定Fault的节点
    if (b != nullptr)
    {
        b = findElementByValue(b, "8005");
        if (b)
        {
            fault = FAULT_ACS_8005; // 错误为8005
            return fault;
        }
        return fault;
    }

    xml_get_hold_request(tree); // 从 XML 树中获取特定节点的值并设置 cwmp->hold_requests 的布尔值
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

    auto handler_fault = [tree_out, &body_out, code, &doc_out, &msg_out]()
    {
        body_out = findElementBylabel(tree_out, "soap_env:Body");
        if (body_out == nullptr)
            return -1;
        // xml_create_generic_fault_message(body_out, code);//待实现
        tx::XMLPrinter printer;
        doc_out.Print(&printer);
        msg_out.assign(printer.CStr());
        std::cout << msg_out << std::endl; // 响应soap
        return 0;
    };

    doc_out.Parse(CWMP_RESPONSE_MESSAGE);
    tree_out = doc_out.RootElement(); // 加载 CWMP_RESPONSE_MESSAGE 并将结果存储在 tree_out 中。如果解析失败，跳转到 error 标签
    if (!tree_out)
        return -1;

    doc_int.Parse(msg_in.c_str());
    tree_in = doc_int.RootElement(); // 使用 mxmlLoadString 函数解析输入的 XML 消息（msg_in），并将结果存储在 tree_in 中
    if (!tree_in)
        return -1;

    xml_free_ns(); // 释放 XML 命名空间相关的资源
    if (xml_recreate_namespace(tree_in))
    { // 重新创建 XML 命名空间的函数, 失败返回-1, 成功返回0
        code = FAULT_9003;
        return handler_fault();
    }
    /* handle cwmp:ID */
    c = ns.cwmp + ":ID";

    b = findElementBylabel(tree_in, c.c_str());
    /* ACS没有发送ID参数，我们在没有它的情况下还是继续 */
    if (b)
    {
        if (b->GetText())
        {
            c.assign(b->GetText());

            b = findElementBylabel(tree_out, "cwmp:ID");
            if (!b)
                return -1;
            b->SetText(c.c_str()); // 与ACS请求体中的id保持一致
        }
    }

    b = xml_find_node_by_env_type(tree_in, "Body");
    if (!b)
    {
        code = FAULT_9003;
        return handler_fault();
    }

    b = b->FirstChildElement();
    if (!b)
    {
        code = FAULT_9003;
        return handler_fault();
    }

    c.assign(b->Value()); // 获取标签名称
    if (c.find(':') != std::string::npos)
    { // 存在冒号
        size_t ns_len = c.find(':');

        if (ns.cwmp.length() != ns_len)
        {
            code = FAULT_9003;
            return handler_fault();
        }

        if (c.find(ns.cwmp) == std::string::npos)
        {
            code = FAULT_9003;
            return handler_fault();
        }

        c = c.substr(ns_len + 1); // 拷贝冒号后面的标签
    }
    else
    {
        code = FAULT_9003;
        return handler_fault();
    }
    method = NULL;
    Log(NAME, L_NOTICE, "received %s method from the ACS\n", c.c_str());
    for (i = 0; i < ARRAY_SIZE(rpc_methods); i++)
    {
        if (c == rpc_methods[i].name)
        {
            method = &rpc_methods[i];
            break;
        }
    }
    if (method)
    {
        if (method->handler(b, tree_in, tree_out, doc_out))
            return -1;
    }
    else
    {
        code = FAULT_9000;
        return handler_fault();
    }
    tx::XMLPrinter printer;
    doc_out.Print(&printer);
    msg_out.assign(printer.CStr());
    std::cout << msg_out << std::endl; // 响应soap
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