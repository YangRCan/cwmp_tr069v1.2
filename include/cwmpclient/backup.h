/**
 * @Copyright : Yangrongcan
 */

#if !defined(_CWMP_BACKUP_)
#define _CWMP_BACKUP_

#include "tinyxml2.h"
#define BACKUP_DIR "./backup"
#define BACKUP_FILE BACKUP_DIR"/backup.xml"

void backup_init(void);
void backup_load_event(void);
void backup_load_download(void);
void backup_load_upload(void);
void backup_update_all_complete_time_transfer_complete(void);

tinyxml2::XMLElement* backup_add_event(int code, const char *key, int method_id);

#endif // _CWMP_BACKUP_
