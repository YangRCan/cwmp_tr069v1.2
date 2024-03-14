/**
 * @Copyright : Yangrongcan
*/
#if !defined(_CWMP_XML_)
#define _CWMP_XML_

#define CWMP_INFORM_MESSAGE \
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"					\
"<soap_env:Envelope "															\
	"xmlns:soap_env=\"http://schemas.xmlsoap.org/soap/envelope/\" " 			\
	"xmlns:soap_enc=\"http://schemas.xmlsoap.org/soap/encoding/\" " 			\
	"xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "							\
	"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "					\
	"xmlns:cwmp=\"urn:dslforum-org:cwmp-1-2\">" 								\
		"<soap_env:Header>" 													\
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



#endif // _CWMP_XML_
