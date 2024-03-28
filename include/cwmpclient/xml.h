/**
 * @Copyright : Yangrongcan
*/
#if !defined(_CWMP_XML_)
#define _CWMP_XML_

#include <tinyxml2.h>

#define CWMP_INFORM_MESSAGE \
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"					\
"<soap_env:Envelope "															\
	"xmlns:soap_env=\"http://schemas.xmlsoap.org/soap/envelope/\" " 			\
	"xmlns:soap_enc=\"http://schemas.xmlsoap.org/soap/encoding/\" " 			\
	"xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "							\
	"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "					\
	"xmlns:cwmp=\"urn:dslforum-org:cwmp-1-2\">" 								\
	"<soap_env:Header>" 														\
		"<cwmp:ID soap_env:mustUnderstand=\"1\"/>"								\
	"</soap_env:Header>"														\
	"<soap_env:Body>"															\
	"<cwmp:Inform>" 															\
		"<DeviceId>"															\
			"<Manufacturer/>"													\
			"<OUI/>"															\
			"<ProductClass/>"													\
			"<SerialNumber/>"													\
		"</DeviceId>"															\
		"<Event soap_enc:arrayType=\"cwmp:EventStruct[0]\" />"					\
		"<MaxEnvelopes>1</MaxEnvelopes>"										\
		"<CurrentTime/>"														\
		"<RetryCount/>" 														\
		"<ParameterList soap_enc:arrayType=\"cwmp:ParameterValueStruct[0]\">"	\
		"</ParameterList>"														\
	"</cwmp:Inform>"															\
"</soap_env:Body>"																\
"</soap_env:Envelope>"


#define CWMP_RESPONSE_MESSAGE \
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"		\
"<soap_env:Envelope "												\
	"xmlns:soap_env=\"http://schemas.xmlsoap.org/soap/envelope/\" " \
	"xmlns:soap_enc=\"http://schemas.xmlsoap.org/soap/encoding/\" " \
	"xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "				\
	"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "		\
	"xmlns:cwmp=\"urn:dslforum-org:cwmp-1-2\">" 					\
	"<soap_env:Header>" 											\
		"<cwmp:ID soap_env:mustUnderstand=\"1\"/>"					\
	"</soap_env:Header>"											\
	"<soap_env:Body/>"												\
"</soap_env:Envelope>"

#define CWMP_GET_RPC_METHOD_MESSAGE \
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"		\
"<soap_env:Envelope "												\
	"xmlns:soap_env=\"http://schemas.xmlsoap.org/soap/envelope/\" " \
	"xmlns:soap_enc=\"http://schemas.xmlsoap.org/soap/encoding/\" " \
	"xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "				\
	"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "		\
	"xmlns:cwmp=\"urn:dslforum-org:cwmp-1-2\">" 					\
		"<soap_env:Header>" 										\
		"<cwmp:ID soap_env:mustUnderstand=\"1\"/>"					\
	"</soap_env:Header>"											\
	"<soap_env:Body>"												\
	"<cwmp:GetRPCMethods>"											\
	"</cwmp:GetRPCMethods>" 										\
"</soap_env:Body>"													\
"</soap_env:Envelope>"

#define CWMP_TRANSFER_COMPLETE_MESSAGE \
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"		\
"<soap_env:Envelope "												\
	"xmlns:soap_env=\"http://schemas.xmlsoap.org/soap/envelope/\" " \
	"xmlns:soap_enc=\"http://schemas.xmlsoap.org/soap/encoding/\" " \
	"xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "				\
	"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "		\
	"xmlns:cwmp=\"urn:dslforum-org:cwmp-1-2\">" 					\
		"<soap_env:Header>" 										\
		"<cwmp:ID soap_env:mustUnderstand=\"1\"/>"					\
	"</soap_env:Header>"											\
	"<soap_env:Body>"												\
	"<cwmp:TransferComplete>"										\
		"<CommandKey/>" 											\
		"<FaultStruct>" 											\
			"<FaultCode/>"											\
			"<FaultString/>"										\
		"</FaultStruct>"											\
		"<StartTime/>"												\
		"<CompleteTime/>"											\
	"</cwmp:TransferComplete>"										\
"</soap_env:Body>"													\
"</soap_env:Envelope>"

struct cwmp_namespaces
{
	std::string soap_env[8]; //某些ACS soap消息包含超过1个env
	std::string soap_enc;
	std::string xsd;
	std::string xsi;
	std::string cwmp;
};

/**
 * rpc方法结构体
*/
struct rpc_method {
	const std::string name;
	int (*handler)(tinyxml2::XMLElement *body_in, tinyxml2::XMLElement *tree_in, tinyxml2::XMLElement *tree_out);
};

int createXML(const char* xmlPath);

// void xml_exit(void);

int xml_prepare_inform_message(std::string &msg_out);
int xml_parse_inform_response_message(char *msg_in);
int xml_prepare_get_rpc_methods_message(char **msg_out);
int xml_parse_get_rpc_methods_response_message(char *msg_in);
int xml_handle_message(char *msg_in, char **msg_out);
int xml_get_index_fault(char *fault_code);

tinyxml2::XMLElement *xml_create_generic_fault_message(tinyxml2::XMLElement *body, int code);
int xml_add_cwmpid(tinyxml2::XMLElement *tree);
int xml_parse_transfer_complete_response_message(char *msg_in);
int xml_create_set_parameter_value_fault_message(tinyxml2::XMLElement *body, int code);

#endif // _CWMP_XML_
