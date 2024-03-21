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
#include "cwmpclient.h"

namespace tx = tinyxml2;
namespace fs = std::filesystem;

tx::XMLDocument backup_doucment;

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
 * 从backup_document中迭代查找event节点，并添加到event, 设置一个Inform定时器
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
                    if (e = cwmp->cwmp_add_event(atoi(event_num), key, method_id, EVENT_NO_BACKUP))
                    {
                        e->backup_node = elem;
                    }
                    cwmp->cwmp_add_inform_timer();
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

void backup_load_download(void)
{
    tx::XMLElement *root = backup_doucment.RootElement();
    tx::XMLElement *target;
	int delay = 0;
	unsigned int t;
    std::string command_key, download_url, username, password, file_size, time_execute, file_type;

    if(root) {
	    tx::XMLElement *cwmp_node = root->FirstChildElement("cwmp");
        if(cwmp_node) {
            for(tx::XMLElement *b = cwmp_node->FirstChildElement("download"); b; b = b->NextSiblingElement("download")) {

                target = b->FirstChildElement("command_key");
                if (!target) return;
                if (target->GetText()) command_key = target->GetText();
                command_key = "";//没有就给空

                target = b->FirstChildElement("url");
                if (!target) return;
                if (target->GetText()) download_url = target->GetText();
                download_url = "";

                target = b->FirstChildElement("username");
                if (!target) return;
                if (target->GetText()) username = target->GetText();
                username = "";

                target = b->FirstChildElement("password");
                if(!target) return;
                if(target->GetText()) password = target->GetText();
                password = "";

                target = b->FirstChildElement("file_size");
                if(!target) return;
                if(target->GetText()) file_size = target->GetText();
                file_size = "";

                target = b->FirstChildElement("time_execute");
                if(!target) return;
                if(target->GetText()) {
                    time_execute = target->GetText();
                    unsigned long value = std::stoul(time_execute);
                    t = static_cast<unsigned int>(value);
                    delay = t - time(NULL);
                }

                target = b->FirstChildElement("file_type");
                if(!target) return;
                if(target->GetText()) file_type = target->GetText();
                file_type = "";

                cwmp->cwmp_add_download(command_key, delay, file_size, download_url, file_size, username, password, b);
            }
        }else {
            Log(NAME, L_DEBUG, "Failed to find the cwmp subtree.");
        }
    } else {
        Log(NAME, L_DEBUG, "Failed to find root element.");
    }
}

void backup_load_upload(void)
{
}

void backup_update_all_complete_time_transfer_complete(void)
{
}

/**
 * 检查是否已经存在于 ACS URL 相关的节点，并且检查其中的内容是否与配置中的 ACS URL 一致
 */
void backup_check_acs_url(void)
{
    tx::XMLElement *b = backup_doucment.FirstChildElement("acs_url");
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
    if(!cwmp_node)
        cwmp_node = backup_tree_init();

    data = cwmp_node->FirstChildElement("software_version");
    if(data) {//存在
        std::string software_version;
        if(config->device->software_version == software_version.assign(data->GetText())) {
            cwmp->cwmp_add_event(EVENT_VALUE_CHANGE, "", 0, EVENT_NO_BACKUP);
        }
        cwmp_node->DeleteChild(data);
    }
    data = backup_doucment.NewElement("software_version");
    if(!data) return;
    cwmp_node->InsertEndChild(data);
    data->SetText(config->device->software_version.c_str());
    // 保存更改后的备份文件
    if (backup_doucment.SaveFile(BACKUP_FILE) != tx::XML_SUCCESS)
    {
        Log(NAME, L_DEBUG, "Failed to save backup.xml file.");
    }
    cwmp->cwmp_add_inform_timer();
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

    cwmp->cwmp_add_event(EVENT_BOOTSTRAP, "", 0, EVENT_BACKUP);
    cwmp->cwmp_add_inform_timer();
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
