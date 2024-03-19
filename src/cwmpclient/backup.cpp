/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include <filesystem>
#include <fstream>

#include "tinyxml2.h"
#include "backup.h"
#include "cwmp.h"
#include "log.h"
#include "cwmpclient.h"

namespace tx = tinyxml2;
namespace fs = std::filesystem;

tx::XMLDocument backup_doucment;

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
    const char *event_num = NULL, *key = NULL;
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
                    key = NULL;
                }

                target = elem->FirstChildElement("event_method_id");
                if (target || target->GetText())
                    method_id = atoi(target->GetText());

                if (event_num)
                {
                    if (e = cwmp->cwmp_add_event(atoi(event_num), key, method_id, EVENT_NO_BACKUP))
                    {
                        e->backup_node = elem;
                    }
                    // cwmp->cwmp_add_inform_timer();
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
}

void backup_load_upload(void)
{
}

void backup_update_all_complete_time_transfer_complete(void)
{
}

/**
 * 向备份文件添加 event
*/
tx::XMLElement *backup_add_event(int code, const char *key, int method_id)
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

    if (key)
    {
        target = backup_doucment.NewElement("event_key");
        if (!target)
            return NULL;
        target->SetText(key);
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