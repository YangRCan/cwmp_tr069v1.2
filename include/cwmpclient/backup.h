/**
 * @Copyright : Yangrongcan
 */

#if !defined(_CWMP_BACKUP_)
#define _CWMP_BACKUP_

#include "tinyxml2.h"
#define BACKUP_DIR "./backup"
#define BACKUP_FILE BACKUP_DIR"/backup.xml"

static tinyxml2::XMLElement *backup_findElementBylabel(tinyxml2::XMLElement *element, const char *label);

tinyxml2::XMLElement* backup_tree_init(void);
void backup_init(void);
void backup_load_event(void);
void backup_load_download(void);
void backup_load_upload(void);
void backup_update_all_complete_time_transfer_complete(void);

void backup_check_acs_url(void);
void backup_check_software_version(void);
tinyxml2::XMLElement *backup_check_transfer_complete(void);

int backup_remove_node(tinyxml2::XMLElement *node);

void backup_add_acsurl(const char *acs_url);
tinyxml2::XMLElement* backup_add_event(int code, std::string key, int method_id);
tinyxml2::XMLElement* backup_add_download(std::string key, int delay, std::string file_size, std::string download_url, std::string file_type, std::string username, std::string password);
tinyxml2::XMLElement* backup_add_upload(std::string key, int delay, std::string upload_url, std::string file_type, std::string username, std::string password);
tinyxml2::XMLElement* backup_add_transfer_complete(std::string command_key, int fault_code, std::string start_time, int method_id);


int backup_update_fault_transfer_complete(tinyxml2::XMLElement *node, int fault_code);
int backup_update_complete_time_transfer_complete(tinyxml2::XMLElement *node);
int backup_extract_transfer_complete(tinyxml2::XMLElement *node, std::string &msg_out, int *method_id);

#endif // _CWMP_BACKUP_
