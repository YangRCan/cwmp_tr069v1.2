/**
 * @Copyright : Yangrongcan
 */
#include <stdio.h>

#include "tr181.h"
#include "parameter.h"

void init_tr181_object()
{
    char *path;
    // DeviceInfo
    path = concatenateStrings(ROOT, ".DeviceInfo.");                                                            // 子路径与根拼接, concatenateStrings申请的path内存在addObjectToDataModel中释放
    addObjectToDataModel(path, READONLY, PresentObject, NULL);                                                  // 在数据模型创建该Object路径
    addObjectToDataModel(concatenateStrings(ROOT, ".DeviceInfo.MemoryStatus."), READONLY, PresentObject, NULL); // 两种写法均可

    // ManagementServer
    addObjectToDataModel(concatenateStrings(ROOT, ".ManagementServer."), READONLY, PresentObject, NULL);

    // DHCPv4
    addObjectToDataModel(concatenateStrings(ROOT, ".DHCPv4."), READONLY, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server."), READONLY, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Pool."), WRITABLE, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Pool.{i}."), WRITABLE, PresentObject, NULL);

    // IP
    addObjectToDataModel(concatenateStrings(ROOT, ".IP."), READONLY, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".IP.Interface."), WRITABLE, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}."), WRITABLE, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.IPv4Address."), WRITABLE, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.IPv4Address.{i}."), WRITABLE, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Stats."), READONLY, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics."), READONLY, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics.IPPing."), READONLY, PresentObject, NULL);

    // WiFi
    addObjectToDataModel(concatenateStrings(ROOT, ".WiFi."), READONLY, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".WiFi.Radio."), READONLY, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".WiFi.Radio.{i}."), READONLY, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".WiFi.SSID."), WRITABLE, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".WiFi.SSID.{i}."), WRITABLE, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".WiFi.AccessPoint."), WRITABLE, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".WiFi.AccessPoint.{i}."), WRITABLE, PresentObject, NULL);
    addObjectToDataModel(concatenateStrings(ROOT, ".WiFi.AccessPoint.{i}.Security."), READONLY, PresentObject, NULL);
}

void init_tr181_parameter()
{
    // DeviceInfo
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.SpecVersion"), READONLY, Active_Notification, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.ProvisioningCode"), READONLY, Active_Notification, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.Manufacturer"), READONLY, Active_Notification, "string(:64)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.ManufacturerOUI"), READONLY, Active_Notification, "string(6)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.ProductClass"), READONLY, Active_Notification, "string(:64)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.SerialNumber"), READONLY, Active_Notification, "string(:64)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.HardwareVersion"), READONLY, Active_Notification, "string(:64)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.SoftwareVersion"), READONLY, Active_Notification, "string(:64)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.IP"), READONLY, Active_Notification, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.UpTime"), READONLY, Notification_Off, "unsignedInt", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.DeviceLog"), READONLY, Notification_Off, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.MemoryStatus.Total"), READONLY, Notification_Off, "unsignedInt", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DeviceInfo.MemoryStatus.Free"), READONLY, Notification_Off, "unsignedInt", NULL);

    // ManagementServer
    addParameterToDataModel(concatenateStrings(ROOT, ".ManagementServer.URL"), WRITABLE, Notification_Off, "string(:256)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".ManagementServer.Username"), WRITABLE, Notification_Off, "string(:256)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".ManagementServer.Password"), WRITABLE, Notification_Off, "string(:256)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".ManagementServer.PeriodicInformEnable"), WRITABLE, Notification_Off, "boolean", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".ManagementServer.PeriodicInformInterval"), WRITABLE, Notification_Off, "unsignedInt(1:)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".ManagementServer.PeriodicInformTime"), WRITABLE, Notification_Off, "dateTime", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".ManagementServer.ConnectionRequestURL"), READONLY, Notification_Off, "string(:256)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".ManagementServer.ConnectionRequestUsername"), WRITABLE, Notification_Off, "string(:256)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".ManagementServer.ConnectionRequestPassword"), WRITABLE, Notification_Off, "string(:256)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".ManagementServer.ParameterKey"), READONLY, Active_Notification, "string(:32)", NULL);

    // DHCPv4
    addParameterToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Enable"), WRITABLE, Notification_Off, "boolean", NULL);

    addParameterToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Pool.{i}.Enable"), WRITABLE, Notification_Off, "boolean", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Pool.{i}.Status"), READONLY, Notification_Off, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Pool.{i}.Interface"), WRITABLE, Notification_Off, "string(:256)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Pool.{i}.MinAddress"), WRITABLE, Notification_Off, "string(:45)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Pool.{i}.MaxAddress"), WRITABLE, Notification_Off, "string(:45)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Pool.{i}.SubnetMask"), WRITABLE, Notification_Off, "string(:45)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Pool.{i}.DNSServers"), WRITABLE, Notification_Off, "string(:45)[:4]", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Pool.{i}.IPRouters"), WRITABLE, Notification_Off, "string(:45)[:4]", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".DHCPv4.Server.Pool.{i}.LeaseTime"), WRITABLE, Notification_Off, "int(-1:)", NULL);

    // IP
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Enable"), WRITABLE, Notification_Off, "boolean", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Name"), READONLY, Notification_Off, "string(:64)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Type"), READONLY, Notification_Off, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.IPv4AddressNumberOfEntries"), READONLY, Notification_Off, "unsignedInt", NULL);

    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.IPv4Address.{i}.IPAddress"), WRITABLE, Notification_Off, "string(:45)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.IPv4Address.{i}.AddressingType"), READONLY, Notification_Off, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.IPv4Address.{i}.Enable"), WRITABLE, Notification_Off, "boolean", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.IPv4Address.{i}.SubnetMask"), WRITABLE, Notification_Off, "string(:45)", NULL);

    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Stats.BytesSent"), READONLY, Notification_Off, "unsignedLong", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Stats.BytesReceived"), READONLY, Notification_Off, "unsignedLong", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Stats.PacketsSent"), READONLY, Notification_Off, "unsignedLong", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Stats.PacketsReceived"), READONLY, Notification_Off, "unsignedLong", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Stats.ErrorsSent"), READONLY, Notification_Off, "unsignedInt", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Stats.ErrorsReceived"), READONLY, Notification_Off, "unsignedInt", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Stats.DiscardPacketsSent"), READONLY, Notification_Off, "unsignedInt", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Interface.{i}.Stats.DiscardPacketsReceived"), READONLY, Notification_Off, "unsignedInt", NULL);

    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics.IPPing.DiagnosticsState"), WRITABLE, Notification_Off, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics.IPPing.Host"), WRITABLE, Notification_Off, "string(:256)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics.IPPing.NumberOfRepetitions"), WRITABLE, Notification_Off, "unsignedInt(1:)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics.IPPing.Timeout"), WRITABLE, Notification_Off, "unsignedInt(1:)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics.IPPing.DataBlockSize"), WRITABLE, Notification_Off, "unsignedInt(1:65535)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics.IPPing.SuccessCount"), READONLY, Notification_Off, "unsignedInt", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics.IPPing.FailureCount"), READONLY, Notification_Off, "unsignedInt", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics.IPPing.AverageResponseTime"), READONLY, Notification_Off, "unsignedInt", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics.IPPing.MinimumResponseTime"), READONLY, Notification_Off, "unsignedInt", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".IP.Diagnostics.IPPing.MaximumResponseTime"), READONLY, Notification_Off, "unsignedInt", NULL);

    // WIFI
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.Radio.{i}.Enable"), WRITABLE, Notification_Off, "boolean", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.Radio.{i}.Status"), READONLY, Notification_Off, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.Radio.{i}.Name"), READONLY, Notification_Off, "string(:64)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.Radio.{i}.SupportedFrequencyBands"), READONLY, Notification_Off, "string[]", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.Radio.{i}.OperatingFrequencyBand"), WRITABLE, Notification_Off, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.Radio.{i}.OperatingStandards"), WRITABLE, Notification_Off, "string[]", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.Radio.{i}.ChannelsInUse"), READONLY, Notification_Off, "string[](:1024)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.Radio.{i}.Channel"), WRITABLE, Notification_Off, "unsignedInt(1:255)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.Radio.{i}.AutoChannelSupported"), READONLY, Notification_Off, "boolean", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.Radio.{i}.AutoChannelEnable"), WRITABLE, Notification_Off, "boolean", NULL);

    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.SSID.{i}.Enable"), WRITABLE, Notification_Off, "boolean", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.SSID.{i}.Status"), READONLY, Notification_Off, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.SSID.{i}.Name"), READONLY, Notification_Off, "string(:64)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.SSID.{i}.LowerLayers"), WRITABLE, Notification_Off, "string[](:1024)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.SSID.{i}.SSID"), WRITABLE, Notification_Off, "string(:32)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.SSID.{i}.X_IPInterface"), WRITABLE, Notification_Off, "string", NULL);

    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.AccessPoint.{i}.Enable"), WRITABLE, Notification_Off, "boolean", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.AccessPoint.{i}.Status"), READONLY, Notification_Off, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.AccessPoint.{i}.SSIDReference"), WRITABLE, Notification_Off, "string(:256)", NULL);

    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.AccessPoint.{i}.Security.ModesSupported"), READONLY, Notification_Off, "string[]", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.AccessPoint.{i}.Security.ModeEnabled"), WRITABLE, Notification_Off, "string", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.AccessPoint.{i}.Security.WEPKey"), WRITABLE, Notification_Off, "hexBinary(5,13)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.AccessPoint.{i}.Security.PreSharedKey"), WRITABLE, Notification_Off, "hexBinary(:32)", NULL);
    addParameterToDataModel(concatenateStrings(ROOT, ".WiFi.AccessPoint.{i}.Security.KeyPassphrase"), WRITABLE, Notification_Off, "string(:63)", NULL);
}