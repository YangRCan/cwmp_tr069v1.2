/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include <filesystem>
#include <fstream>
#include <ctime>

#include "tinyxml2.h"
#include "backup.h"
#include "config.h"
#include "cwmp.h"
#include "log.h"
#include "faultCode.h"
#include "cwmpclient.h"
#include "time_tool.h"
#include "xml.h"

namespace tx = tinyxml2;
namespace fs = std::filesystem;

tx::XMLDocument backup_doucment;

/**
 * 在xml树中查找是否有标签值为 label 的标签
 */
static tx::XMLElement *backup_findElementBylabel(tx::XMLElement *element, const char *label)
{
    if (!element)
        return nullptr;
    if (strcmp(element->Value(), label) == 0)
        return element;
    for (tx::XMLElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        tx::XMLElement *found = backup_findElementBylabel(child, label);
        if (found)
            return found;
    }
    return nullptr;
}

/**
 * 创建根标签 backup_file，及其子标签 cwmp
 * return: 返回cwmp标签元素
 */
tx::XMLElement *backup_tree_init(void)
{
    tx::XMLElement *root = backup_doucment.NewElement("backup_file");
    if (!root)
        return nullptr;
    backup_doucment.InsertFirstChild(root);
    tx::XMLElement *cwmpElement = backup_doucment.NewElement("cwmp");
    if (!cwmpElement)
        return nullptr;
    root->InsertEndChild(cwmpElement);
    return cwmpElement;
}

/**
 * 将备份文件中的数据读入到程序中
 */
void backup_init(void)
{
    fs::path folderPath = BACKUP_DIR;
    if (!fs::exists(folderPath))
    {
        if (!fs::create_directories(folderPath))
        {
            Log(NAME, L_DEBUG, "Failed to create directory: %s.", BACKUP_DIR);
            return;
        }
    }

    fs::path filePath = BACKUP_FILE;
    if (!fs::exists(filePath))
    {
        std::ofstream outFile(filePath);
        outFile.close();
        Log(NAME, L_INFO, "Created file: %s.", BACKUP_FILE);
    }

    // 以只读方式打开备份文件
    std::ifstream inFile(filePath);
    if (!inFile.is_open())
    {
        Log(NAME, L_DEBUG, "Failed to open file for reading: %s.", BACKUP_FILE);
        return;
    }
    if (backup_doucment.LoadFile(filePath.string().c_str()) != tx::XML_SUCCESS)
    {
        Log(NAME, L_DEBUG, "Failed to load XML file: %s.", BACKUP_FILE);
        return;
    }

    // 加载到程序中
    backup_load_event();
    backup_load_download();
    backup_load_upload();
    backup_update_all_complete_time_transfer_complete();
}

/**
 * 从backup_document中迭代查找event节点，并添加到events, 设置一个Inform定时器
 */
void backup_load_event(void)
{
    tx::XMLElement *root = backup_doucment.RootElement();
    tx::XMLElement *target;
    const char *event_num = NULL;
    std::string key;
    int method_id = 0;
    event *e;

    if (root)
    {
        tx::XMLElement *cwmp_node = root->FirstChildElement("cwmp");
        if (cwmp_node)
        {
            // 遍历 cwmpNode 的所有直接子节点
            for (tx::XMLElement *elem = cwmp_node->FirstChildElement("event"); elem != nullptr; elem = elem->NextSiblingElement("event"))
            {
                // 处理事件节点，这里可以根据需要进行操作
                target = elem->FirstChildElement("event_number");
                if (!target || !(target->GetText()))
                    Log(NAME, L_DEBUG, "Error: event_number tag not found or no value.");
                event_num = target->GetText();

                target = elem->FirstChildElement("event_key");
                if (target && target->GetText())
                {
                    key = target->GetText();
                }
                else
                {
                    key = "";
                }

                target = elem->FirstChildElement("event_method_id");
                if (target && target->GetText())
                    method_id = atoi(target->GetText());

                if (event_num)
                {
                    e = cwmp->cwmp_add_event(atoi(event_num), key, method_id, EVENT_NO_BACKUP);
                    if (e != NULL)
                    {
                        e->backup_node = elem;
                    }
                    cwmp->cwmp_add_inform_timer(1000);
                }
            }
        }
        else
        {
            Log(NAME, L_DEBUG, "Failed to find the cwmp subtree.");
        }
    }
    else
    {
        Log(NAME, L_DEBUG, "Failed to find root element.");
    }
}

/**
 * 从backup_document中迭代查找download节点，并添加到downloads链表
 */
void backup_load_download(void)
{
    tx::XMLElement *root = backup_doucment.RootElement();
    tx::XMLElement *target;
    int delay = 0;
    unsigned int t;
    std::string command_key, download_url, username, password, file_size, time_execute, file_type;

    if (root)
    {
        tx::XMLElement *cwmp_node = root->FirstChildElement("cwmp");
        if (cwmp_node)
        {
            for (tx::XMLElement *b = cwmp_node->FirstChildElement("download"); b; b = b->NextSiblingElement("download"))
            {

                target = b->FirstChildElement("command_key");
                if (!target)
                    return;
                if (target->GetText())
                    command_key = target->GetText();
                else
                    command_key = ""; // 没有就给空

                target = b->FirstChildElement("url");
                if (!target)
                    return;
                if (target->GetText())
                    download_url = target->GetText();
                else
                    download_url = "";

                target = b->FirstChildElement("username");
                if (!target)
                    return;
                if (target->GetText())
                    username = target->GetText();
                else
                    username = "";

                target = b->FirstChildElement("password");
                if (!target)
                    return;
                if (target->GetText())
                    password = target->GetText();
                else
                    password = "";

                target = b->FirstChildElement("file_size");
                if (!target)
                    return;
                if (target->GetText())
                    file_size = target->GetText();
                else
                    file_size = "";

                target = b->FirstChildElement("time_execute");
                if (!target)
                    return;
                if (target->GetText())
                {
                    time_execute = target->GetText();
                    unsigned long value = std::stoul(time_execute);
                    t = static_cast<unsigned int>(value);
                    delay = t - time(NULL);
                }

                target = b->FirstChildElement("file_type");
                if (!target)
                    return;
                if (target->GetText())
                    file_type = target->GetText();
                else
                    file_type = "";

                cwmp->cwmp_add_download(command_key, delay, file_size, download_url, file_type, username, password, b);
            }
        }
        else
        {
            Log(NAME, L_DEBUG, "Failed to find the cwmp subtree.");
        }
    }
    else
    {
        Log(NAME, L_DEBUG, "Failed to find root element.");
    }
}

/**
 * 从backup_document中迭代查找upload节点，并添加到uploads链表
 */
void backup_load_upload(void)
{
    tx::XMLElement *root = backup_doucment.RootElement();
    tx::XMLElement *target;
    int delay = 0;
    unsigned int t;
    std::string command_key, upload_url, username, password, file_type;

    if (root)
    {
        tx::XMLElement *cwmp_node = root->FirstChildElement("cwmp");
        if (cwmp_node)
        {
            for (tx::XMLElement *b = cwmp_node->FirstChildElement("upload"); b; b = b->NextSiblingElement("upload"))
            {
                target = b->FirstChildElement("command_key");
                if (!target)
                    return;
                if (target->GetText())
                    command_key = target->GetText();
                else
                    command_key = "";

                target = b->FirstChildElement("url");
                if (!target)
                    return;
                if (target->GetText())
                    upload_url = target->GetText();
                else
                    upload_url = "";

                target = b->FirstChildElement("username");
                if (!target)
                    return;
                if (target->GetText())
                    username = target->GetText();
                else
                    username = "";

                target = b->FirstChildElement("password");
                if (!target)
                    return;
                if (target->GetText())
                    password = target->GetText();
                else
                    password = "";

                target = b->FirstChildElement("time_execute");
                if (!target)
                    return;
                if (target->GetText())
                {
                    std::string time_execute = target->GetText();
                    unsigned long value = std::stoul(time_execute);
                    t = static_cast<unsigned int>(value);
                    delay = t - time(NULL);
                }

                target = b->FirstChildElement("file_type");
                if (!target)
                    return;
                if (target->GetText())
                    file_type = target->GetText();
                else
                    file_type = "";

                cwmp->cwmp_add_upload(command_key, delay, upload_url, file_type, username, password, b);
            }
        }
        else
        {
            Log(NAME, L_DEBUG, "Failed to find the cwmp subtree.");
        }
    }
    else
    {
        Log(NAME, L_DEBUG, "Failed to find root element.");
    }
}

/**
 * 在backup_document中查找transfer_complete节点，修改其中的complete_time的值
 */
void backup_update_all_complete_time_transfer_complete(void)
{
    tx::XMLElement *target;
    tx::XMLElement *root = backup_doucment.RootElement();
    if (root)
    {
        tx::XMLElement *cwmp_node = root->FirstChildElement("cwmp");
        if (cwmp_node)
        {
            for (tx::XMLElement *elem = cwmp_node->FirstChildElement("transfer_complete"); elem != nullptr; elem = elem->NextSiblingElement("transfer_complete"))
            {
                target = elem->FirstChildElement("complete_time");
                if (!target)
                    return;
                if (target->GetText() && strcmp(target->GetText(), UNKNOWN_TIME) != 0)
                    continue;
                target->SetText(get_time());
            }
            if (backup_doucment.SaveFile(BACKUP_FILE) != tx::XML_SUCCESS)
            {
                Log(NAME, L_DEBUG, "Failed to save backup.xml file.");
            }
        }
        else
        {
            Log(NAME, L_DEBUG, "Failed to find the cwmp subtree.");
        }
    }
    else
    {
        Log(NAME, L_DEBUG, "Failed to find root element.");
    }
}

/**
 * 检查是否已经存在于 ACS URL 相关的节点，并且检查其中的内容是否与配置中的 ACS URL 一致
 */
void backup_check_acs_url(void)
{
    tx::XMLElement *cwmp_node = nullptr;
    tx::XMLElement *root = backup_doucment.RootElement();
    if(root) cwmp_node = root->FirstChildElement("cwmp");
    if (!cwmp_node)
    {
        backup_add_acsurl(config->acs->url.c_str()); // 更新备份文件中添加 ACS URL
        return;
    }
    tx::XMLElement *b = cwmp_node->FirstChildElement("acs_url");
    std::string url;
    if (!b || url.assign(b->GetText()) != config->acs->url)
    {
        backup_add_acsurl(config->acs->url.c_str()); // 更新备份文件中添加 ACS URL
    }
}

/**
 * 检查是否已存在 software_version 相关节点，并检查其中的内容是否与配置中的 software_version 一致,
 * 若不一致，重构一个 "software_version" 的新节点，并更新配置文件中的值
 */
void backup_check_software_version(void)
{
    tx::XMLElement *data;
    tx::XMLElement *root = backup_doucment.RootElement();
    tx::XMLElement *cwmp_node = root->FirstChildElement("cwmp");
    if (!cwmp_node)
        cwmp_node = backup_tree_init();

    data = cwmp_node->FirstChildElement("software_version");
    if (data)
    { // 存在
        std::string software_version;
        if (config->device->software_version != software_version.assign(data->GetText()))
        {
            cwmp->cwmp_add_event(EVENT_VALUE_CHANGE, "", 0, EVENT_NO_BACKUP);
        }
        cwmp_node->DeleteChild(data);
    }
    data = backup_doucment.NewElement("software_version");
    if (!data)
        return;
    cwmp_node->InsertEndChild(data);
    data->SetText(config->device->software_version.c_str());
    // 保存更改后的备份文件
    if (backup_doucment.SaveFile(BACKUP_FILE) != tx::XML_SUCCESS)
    {
        Log(NAME, L_DEBUG, "Failed to save backup.xml file.");
    }
    cwmp->cwmp_add_inform_timer(1000);
}

/**
 * 在 xml 文件中查找第一个 transfer_complete 并返回
 * 没有则为 nullptr
*/
tx::XMLElement *backup_check_transfer_complete(void)
{
    tx::XMLElement *root = backup_doucment.RootElement();
    return backup_findElementBylabel(root, "transfer_complete");
}

/**
 * 删除传入的节点
 */
int backup_remove_node(tx::XMLElement *node)
{
    if (!node)
    {
        Log(NAME, L_DEBUG, "Node pointer is NULL.\n");
        return 1;
    }
    tx::XMLNode *parent = node->Parent();
    if (parent)
    {
        parent->DeleteChild(node);
    }
    else
    {
        Log(NAME, L_DEBUG, "Failed to remove node: Parent node not found.\n");
        return 1; // 失败
    }
    // 保存修改后的 XML 文件
    if (backup_doucment.SaveFile(BACKUP_FILE) != tx::XML_SUCCESS)
    {
        Log(NAME, L_DEBUG, "Failed to save backup.xml file.");
        return 1;
    }
    return 0;
}

/**
 * 向备份文件中添加 ACS URL
 */
void backup_add_acsurl(const char *acs_url)
{
    // 声明了两个指向 mxml_node_t 类型的指针变量 data 和 b，用于处理 XML 节点
    tx::XMLElement *data, *b;

    cwmp->cwmp_clean();
    backup_doucment.DeleteChildren();
    b = backup_tree_init();
    if (!b)
        return;
    data = backup_doucment.NewElement("acs_url");
    if (!data)
        return;
    data->SetText(config->acs->url.c_str());
    b->InsertEndChild(data);

    // 保存更改后的备份文件
    if (backup_doucment.SaveFile(BACKUP_FILE) != tx::XML_SUCCESS)
    {
        Log(NAME, L_DEBUG, "Failed to save backup.xml file.");
    }

    cwmp->cwmp_add_event(EVENT_BOOTSTRAP, "", 0, EVENT_BACKUP);//添加开机启动事件
    cwmp->cwmp_add_inform_timer(1000);
}

/**
 * 向备份文件添加 event
 */
tx::XMLElement *backup_add_event(int code, std::string key, int method_id)
{
    tx::XMLElement *event_node, *target, *cwmp_node;
    tx::XMLElement *root = backup_doucment.RootElement();

    cwmp_node = root->FirstChildElement("cwmp");
    if (!cwmp_node)
        return NULL;
    event_node = backup_doucment.NewElement("event");
    if (!event_node)
        return NULL;
    cwmp_node->InsertEndChild(event_node);

    target = backup_doucment.NewElement("event_number");
    if (!target)
        return NULL;
    target->SetText(code);
    event_node->InsertEndChild(target);

    if (!key.empty())
    {
        target = backup_doucment.NewElement("event_key");
        if (!target)
            return NULL;
        target->SetText(key.c_str());
        event_node->InsertEndChild(target);
    }

    if (method_id)
    {
        target = backup_doucment.NewElement("event_method_id");
        if (!target)
            return NULL;
        target->SetText(method_id);
        event_node->InsertEndChild(target);
    }

    // 保存更改后的备份文件
    if (backup_doucment.SaveFile(BACKUP_FILE) != tx::XML_SUCCESS)
    {
        Log(NAME, L_DEBUG, "Failed to save backup.xml file.");
    }

    return event_node;
}

/**
 * 在 cwmp 标签下添加 download 标签及其子标签
*/
tx::XMLElement* backup_add_download(std::string key, int delay, std::string file_size, std::string download_url, std::string file_type, std::string username, std::string password)
{
    tx::XMLElement *tree, *cwmp_node, *download_node, *n;
    tx::XMLElement *root = backup_doucment.RootElement();
    unsigned int timestamp = delay + static_cast<unsigned int>(time(NULL));
    std::string time_execute = std::to_string(timestamp);
    cwmp_node = root->FirstChildElement("cwmp");
    if (!cwmp_node)
        return NULL;
    download_node = backup_doucment.NewElement("download");
    if (!download_node)
        return NULL;
    cwmp_node->InsertEndChild(download_node);
    
    n = backup_doucment.NewElement("command_key");
    if(!n) return NULL;
    n->SetText(key.c_str());
    download_node->InsertEndChild(n);

    n = backup_doucment.NewElement("file_type");
    if(!n) return NULL;
    n->SetText(file_type.c_str());
    download_node->InsertEndChild(n);

    n = backup_doucment.NewElement("url");
    if(!n) return NULL;
    n->SetText(download_url.c_str());
    download_node->InsertEndChild(n);

    n = backup_doucment.NewElement("username");
    if(!n) return NULL;
    n->SetText(username.c_str());
    download_node->InsertEndChild(n);

    n = backup_doucment.NewElement("password");
    if(!n) return NULL;
    n->SetText(password.c_str());
    download_node->InsertEndChild(n);

    n = backup_doucment.NewElement("file_size");
    if(!n) return NULL;
    n->SetText(file_size.c_str());
    download_node->InsertEndChild(n);

    n = backup_doucment.NewElement("time_execute");
    if(!n) return NULL;
    n->SetText(time_execute.c_str());
    download_node->InsertEndChild(n);

    if (backup_doucment.SaveFile(BACKUP_FILE) != tx::XML_SUCCESS)
    {
        Log(NAME, L_DEBUG, "Failed to save backup.xml file.");
    }
    return download_node;
}

/**
 * 在 cwmp 标签下添加 upload 标签及其子标签
*/
tinyxml2::XMLElement* backup_add_upload(std::string key, int delay, std::string upload_url, std::string file_type, std::string username, std::string password)
{
    tx::XMLElement *tree, *cwmp_node, *upload_node, *n;
    tx::XMLElement *root = backup_doucment.RootElement();
    unsigned int timestamp = delay + static_cast<unsigned int>(time(NULL));
    std::string time_execute = std::to_string(timestamp);
    cwmp_node = root->FirstChildElement("cwmp");
    if (!cwmp_node)
        return NULL;
    upload_node = backup_doucment.NewElement("upload");
    if (!upload_node)
        return NULL;
    cwmp_node->InsertEndChild(upload_node);
    
    n = backup_doucment.NewElement("command_key");
    if(!n) return NULL;
    n->SetText(key.c_str());
    upload_node->InsertEndChild(n);

    n = backup_doucment.NewElement("file_type");
    if(!n) return NULL;
    n->SetText(file_type.c_str());
    upload_node->InsertEndChild(n);

    n = backup_doucment.NewElement("url");
    if(!n) return NULL;
    n->SetText(upload_url.c_str());
    upload_node->InsertEndChild(n);

    n = backup_doucment.NewElement("username");
    if(!n) return NULL;
    n->SetText(username.c_str());
    upload_node->InsertEndChild(n);

    n = backup_doucment.NewElement("password");
    if(!n) return NULL;
    n->SetText(password.c_str());
    upload_node->InsertEndChild(n);

    n = backup_doucment.NewElement("time_execute");
    if(!n) return NULL;
    n->SetText(time_execute.c_str());
    upload_node->InsertEndChild(n);

    if (backup_doucment.SaveFile(BACKUP_FILE) != tx::XML_SUCCESS)
    {
        Log(NAME, L_DEBUG, "Failed to save backup.xml file.");
    }
    return upload_node;
}

/**
 * 在 cwmp 标签下添加 transfer_complete 标签及其子标签
 */
tx::XMLElement *backup_add_transfer_complete(std::string command_key, int fault_code, std::string start_time, int method_id)
{
    tx::XMLElement *transfer_complete, *target, *cwmp_node;
    tx::XMLElement *root = backup_doucment.RootElement();
    cwmp_node = root->FirstChildElement("cwmp");
    if (!cwmp_node)
        return NULL;
    transfer_complete = backup_doucment.NewElement("transfer_complete");
    if (!transfer_complete)
        return NULL;
    cwmp_node->InsertEndChild(transfer_complete);

    target = backup_doucment.NewElement("command_key");
    if (!target)
        return NULL;
    target->SetText(command_key.c_str());
    transfer_complete->InsertEndChild(target);

    target = backup_doucment.NewElement("fault_code");
    if (!target)
        return NULL;
    target->SetText(fault_array[fault_code].code);
    transfer_complete->InsertEndChild(target);

    target = backup_doucment.NewElement("fault_string");
    if (!target)
        return NULL;
    target->SetText(fault_array[fault_code].string);
    transfer_complete->InsertEndChild(target);

    target = backup_doucment.NewElement("start_time");
    if (!target)
        return NULL;
    target->SetText(start_time.c_str());
    transfer_complete->InsertEndChild(target);

    target = backup_doucment.NewElement("complete_time");
    if (!target)
        return NULL;
    target->SetText(UNKNOWN_TIME);
    transfer_complete->InsertEndChild(target);

    target = backup_doucment.NewElement("method_id");
    if (!target)
        return NULL;
    target->SetText(std::to_string(method_id).c_str());
    transfer_complete->InsertEndChild(target);

    // 保存更改后的备份文件
    if (backup_doucment.SaveFile(BACKUP_FILE) != tx::XML_SUCCESS)
    {
        Log(NAME, L_DEBUG, "Failed to save backup.xml file.");
    }

    return transfer_complete;
}

/**
 * 传输出错，更新备份节点中的故障信息
 */
int backup_update_fault_transfer_complete(tx::XMLElement *node, int fault_code)
{
    tx::XMLElement *target;
    target = node->FirstChildElement("fault_code");
    if (!target)
        return -1;
    target->SetText(fault_array[fault_code].code);

    target = node->FirstChildElement("fault_string");
    if (!target)
        return -1;
    target->SetText(fault_array[fault_code].string);

    // 保存更改后的备份文件
    if (backup_doucment.SaveFile(BACKUP_FILE) != tx::XML_SUCCESS)
    {
        Log(NAME, L_DEBUG, "Failed to save backup.xml file.");
    }
    return 0;
}

/**
 * 传输并且应用成功，更新备份节点 transfer_complete 中的完成时间
 */
int backup_update_complete_time_transfer_complete(tx::XMLElement *node)
{
    tx::XMLElement *target = node->FirstChildElement("complete_time");
    if (!target)
        return -1;
    target->SetText(get_time());
    if (backup_doucment.SaveFile(BACKUP_FILE) != tx::XML_SUCCESS)
    {
        Log(NAME, L_DEBUG, "Failed to save backup.xml file.");
    }
    return 0;
}

/**
 * 从传入的 XML 节点中提取特定的信息，并将这些信息保存到一个新的 XML 节点中
*/
int backup_extract_transfer_complete(tx::XMLElement *node, std::string &msg_out, int *method_id)
{
    tx::XMLDocument doc;
	tx::XMLElement *tree, *b, *n;
	const char *val;
    
    doc.Parse(CWMP_TRANSFER_COMPLETE_MESSAGE);
    tree = doc.RootElement();
	if (!tree) return -1;

	if(xml_add_cwmpid(tree)) return -1;

    b = backup_findElementBylabel(node, "command_key");
	if (!b) return -1;
    n = backup_findElementBylabel(tree, "CommandKey");
	if (!n) return -1;
	if (b->GetText()) { //检查 XML 节点 b 的子节点是否存在、其类型是否为 MXML_OPAQUE 且值不为空
		val = b->GetText();
		n->SetText(val);//创建一个新的 mxml_node_t 节点 n，将经过处理的值设置为这个新节点的值
	}
	else
		n->SetText("");

    b = backup_findElementBylabel(node, "fault_code");
	if (!b) return -1;
    n = backup_findElementBylabel(tree, "FaultCode");
	if (!n) return -1;
    n->SetText(b->GetText());

    b = backup_findElementBylabel(node, "fault_string");
	if (!b) return -1;
	if (b->GetText()) {
        n = backup_findElementBylabel(tree, "FaultString");
		if (!n) return -1;
        val = b->GetText();
        n->SetText(val);
	}

    b = backup_findElementBylabel(node, "start_time");
	if (!b) return -1;
    n = backup_findElementBylabel(tree, "StartTime");
	if (!n) return -1;
    n->SetText(b->GetText());

    b = backup_findElementBylabel(node, "complete_time");
	if (!b) return -1;
    n = backup_findElementBylabel(tree, "CompleteTime");
	if (!n) return -1;
    n->SetText(b->GetText());

    b = backup_findElementBylabel(node, "method_id");
	if (!b) return -1;
	*method_id = atoi(b->GetText());

    tx::XMLPrinter printer;
    doc.Print(&printer);
    msg_out.assign(printer.CStr());
    std::cout << msg_out << std::endl;
    
	return 0; //返回0表示成功
}

