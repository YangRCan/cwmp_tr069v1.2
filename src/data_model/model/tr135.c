/**
 * @Copyright : Yangrongcan
 */
#include <stdio.h>

#include "tr181.h"
#include "tr135.h"
#include "parameter.h"

void init_tr135_object()
{
    addObjectToDataModel(concatenateStrings(ROOT, ".Services.STBService."), READONLY, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".Services.STBService.{i}."), READONLY, PresentObject, NULL);

}

void init_tr135_parameter()
{
    // .Services.STBService.
    addParameterToDataModel(concatenateStrings(ROOT, ".Services.STBService.STBServiceNumberOfEntries"), "", READONLY, Active_Notification, "unsignedInt", NULL);

    // .Services.STBService.{i}.
    addParameterToDataModel(concatenateStrings(ROOT, ".Services.STBService.{i}.Enable"), "", WRITABLE, Active_Notification, "boolean", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".Services.STBService.{i}.Alias"), "", WRITABLE, Active_Notification, "string(:64)", NULL);

    
}