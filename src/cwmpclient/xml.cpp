/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include "tinyxml2.h"
#include "cwmpclient.h"
#include "cwmp.h"
#include "time_tool.h"
#include "parameter.h"

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
    int xml_prepare_events_inform(tx::XMLElement *tree)
    {
        tx::XMLElement *node, *b1, *b2;
        int n = 0;
        // 获取Event标签
        b1 = tree->FirstChildElement("cwmp:Inform")->FirstChildElement("Event");
        if (!b1) return -1;
        // 遍历事件列表
        for(auto it = cwmp->get_event_list().begin(); it != cwmp->get_event_list().end(); it++) {
            node = tx::XMLDocument::NewElement("EventStruct");
            if (!node) return -1;

            b2 = tx::XMLDocument::NewElement("EventCode");
            if (!b2) return -1;
            b2->SetText(event_code_array[(*it)->code].code.c_str());
            node->InsertEndChild(b2);

            b2 = tx::XMLDocument::NewElement("CommandKey");
            if (!b2) return -1;
            if ((*it)->key) {
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

    int xml_get_rpc_methods(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {

    }

    int xml_set_parameter_values(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {

    }

    int xml_get_parameter_values(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out){
        
    }

    int xml_get_parameter_names(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {

    }

    int xml_set_parameter_attributes(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        
    }

    int xml_download(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {

    }

    int xml_upload(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {

    }

    int xml_factory_reset(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {

    }

    int xml_reboot(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {
        
    }

    int xml_get_parameter_attributes(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {

    }

    int xml_schedule_inform(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {

    }

    int xml_AddObject(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {

    }

    int xml_DeleteObject(tx::XMLElement *body_in, tx::XMLElement *tree_in, tx::XMLElement *tree_out) {

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

    doc.Parse(CWMP_INFORM_MESSAGE);
    tree = doc.RootElement();
	if (!tree) return -1;

	if(xml_add_cwmpid(tree)) return -1;

    b = tree->FirstChildElement("cwmp:Inform")->FirstChildElement("RetryCount");
	if (!b) return -1;
	b->SetText(cwmp->get_retry_count());

    b = tree->FirstChildElement("cwmp:Inform")->FirstChildElement("DeviceId")->FirstChildElement("Manufacturer");
	if (!b) return -1;
    b->SetText(cwmp->deviceInfo.manufacturer.c_str());

    b = tree->FirstChildElement("cwmp:Inform")->FirstChildElement("DeviceId")->FirstChildElement("OUI");
	if (!b) return -1;
    b->SetText(cwmp->deviceInfo.oui.c_str());

    b = tree->FirstChildElement("cwmp:Inform")->FirstChildElement("DeviceId")->FirstChildElement("ProductClass");
	if (!b) return -1;
    b->SetText(cwmp->deviceInfo.product_class.c_str());

    b = tree->FirstChildElement("cwmp:Inform")->FirstChildElement("DeviceId")->FirstChildElement("SerialNumber");
	if (!b) return -1;
    b->SetText(cwmp->deviceInfo.serial_number.c_str());
    
    // 修改Event子树内容
	if (xml_prepare_events_inform(tree))
		return -1;

    b = tree->FirstChildElement("cwmp:Inform")->FirstChildElement("CurrentTime");
	if (!b) return -1;
	b = mxmlNewOpaque(b, mix_get_time());
    b->SetText(get_time());

    // 获取需要上报的参数
    getInformParameter();
	parameter_list = tree->FirstChildElement("cwmp:Inform")->FirstChildElement("ParameterList");
	if (!parameter_list) return -1;
	while (external_list_parameter.next != &external_list_parameter) {

		n = mxmlNewElement(parameter_list, "ParameterValueStruct");
		if (!n) return -1;

		b = mxmlNewElement(n, "Name");
		if (!b) return -1;

		b = mxmlNewOpaque(b, external_parameter->name);
		if (!b) return -1;

		b = mxmlNewElement(n, "Value");
		if (!b) return -1;

		mxmlElementSetAttr(b, "xsi:type", external_parameter->type);
		b = mxmlNewOpaque(b, external_parameter->data ? external_parameter->data : "");
		if (!b) return -1;

		counter++;
	}

	if (xml_prepare_notifications_inform(parameter_list, &counter))
		return -1;

	if (asprintf(&c, "cwmp:ParameterValueStruct[%d]", counter) == -1)
		return -1;

	mxmlElementSetAttr(parameter_list, "soap_enc:arrayType", c);
	FREE(c);

	*msg_out = mxmlSaveAllocString(tree, xml_format_cb);

	mxmlDelete(tree);
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