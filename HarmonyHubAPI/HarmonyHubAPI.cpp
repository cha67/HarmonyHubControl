/******************************************************************************
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <map>


#include "HarmonyHubAPI.h"
#include <sstream>

#define LOGITECH_AUTH_URL "https://svcs.myharmony.com/CompositeSecurityServices/Security.svc/json/GetUserAuthToken"
#define LOGITECH_AUTH_HOSTNAME "svcs.myharmony.com"
#define LOGITECH_AUTH_PATH "/CompositeSecurityServices/Security.svc/json/GetUserAuthToken"
#define HARMONY_COMMUNICATION_PORT 5222
#define HARMONY_HUB_AUTHORIZATION_TOKEN_FILENAME "HarmonyHub.AuthorizationToken"
#define CONNECTION_ID "12345678-1234-5678-1234-123456789012-1"


#ifndef _WIN32
#define sprintf_s(buffer, buffer_size, stringbuffer, ...) (sprintf(buffer, stringbuffer, __VA_ARGS__))
#endif

/*
 * Class construct
 */
HarmonyHubAPI::HarmonyHubAPI()
{
}


HarmonyHubAPI::~HarmonyHubAPI()
{
}



const std::string HarmonyHubAPI::base64_chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";


bool HarmonyHubAPI::is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}


std::string HarmonyHubAPI::base64_decode(std::string const& encoded_string) {
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i ==4) {
			for (i = 0; i <4; i++)
				char_array_4[i] = (char)base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = (char)base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

	return ret;
}


std::string HarmonyHubAPI::base64_encode(char const* bytes_to_encode, unsigned int in_len) {
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; (i <4) ; i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while((i++ < 3))
			ret += '=';

	}

	return ret;
}



const std::string HarmonyHubAPI::ReadAuthorizationTokenFile()
{
	std::string strAuthorizationToken;
	std::ifstream AuthorizationTokenFileStream (HARMONY_HUB_AUTHORIZATION_TOKEN_FILENAME);
	if (!AuthorizationTokenFileStream.is_open())
		return strAuthorizationToken;

	getline (AuthorizationTokenFileStream,strAuthorizationToken);
	AuthorizationTokenFileStream.close();
	return strAuthorizationToken;
}


bool HarmonyHubAPI::WriteAuthorizationTokenFile(const std::string& strAuthorizationToken)
{
	std::ofstream AuthorizationTokenFileStream;
	AuthorizationTokenFileStream.open(HARMONY_HUB_AUTHORIZATION_TOKEN_FILENAME);
	if(!AuthorizationTokenFileStream.is_open())
		return false;

	AuthorizationTokenFileStream << strAuthorizationToken;
	AuthorizationTokenFileStream.close();
	return true;
}


//  Logs into the Logitech Harmony web service
//  Returns a base64-encoded string containing a 48-byte Login Token in the third parameter
bool HarmonyHubAPI::harmonyWebServiceLogin(std::string strUserEmail, std::string strPassword, std::string& strAuthorizationToken )
{
	if(strUserEmail.empty() || strPassword.empty())
	{
		errorString = "harmonyWebServiceLogin : Empty email or password provided";
		return false;
	}


	// Build JSON request
	std::string strJSONText = "{\"email\":\"";
	strJSONText.append(strUserEmail.c_str());
	strJSONText.append("\",\"password\":\"");
	strJSONText.append(strPassword.c_str());
	strJSONText.append("\"}");

	std::string strHttpPayloadText;

	csocket authcsocket;
	authcsocket.connect("svcs.myharmony.com", 80);

	if (authcsocket.getState() != csocket::CONNECTED)
	{
		errorString = "harmonyWebServiceLogin : Unable to connect to Logitech server";
		return false;
	}

	char contentLength[32];
	sprintf_s( contentLength, 32, "%d", (int)strJSONText.length() );

	std::string strHttpRequestText;

	strHttpRequestText = "POST ";
	strHttpRequestText.append(LOGITECH_AUTH_URL);
	strHttpRequestText.append(" HTTP/1.1\r\nHost: ");
	strHttpRequestText.append(LOGITECH_AUTH_HOSTNAME);
	strHttpRequestText.append("\r\nAccept-Encoding: identity\r\nContent-Length: ");
	strHttpRequestText.append(contentLength);
	strHttpRequestText.append("\r\ncontent-type: application/json;charset=utf-8\r\n\r\n");

	authcsocket.write(strHttpRequestText.c_str(), strHttpRequestText.size());
	authcsocket.write(strJSONText.c_str(), strJSONText.length());

	memset(databuffer, 0, DATABUFFER_SIZE);
	authcsocket.read(databuffer, DATABUFFER_SIZE, false);
	strHttpPayloadText = databuffer;
/*
Expect: 0x00def280 "HTTP/1.1 200 OK Server: nginx/1.2.4 Date: Wed, 05 Feb 2014 17:52:13 GMT Content-Type: application/json; charset=utf-8 Content-Length: 127 Connection: keep-alive Cache-Control: private X-AspNet-Version: 4.0.30319 X-Powered-By: ASP.NET  {"GetUserAuthTokenResult":{"AccountId":0,"UserAuthToken":"KsRE6VVA3xrhtbqFbh0jWn8YTiweDeB\/b94Qeqf3ofWGM79zLSr62XQh8geJxw\/V"}}"
*/

	// Parse the login authorization token from the response
	std::string strAuthTokenTag = "UserAuthToken\":\"";
	size_t pos = (int)strHttpPayloadText.find(strAuthTokenTag);
	if(pos == std::string::npos)
	{
		errorString = "harmonyWebServiceLogin : Logitech web service response does not contain a login authorization token";
		return false;
	}

	strAuthorizationToken = strHttpPayloadText.substr(pos + strAuthTokenTag.length());
	pos = (int)strAuthorizationToken.find("\"}}");
	strAuthorizationToken = strAuthorizationToken.substr(0, pos);

	// Remove forward slashes
	strAuthorizationToken.erase(std::remove(strAuthorizationToken.begin(), strAuthorizationToken.end(), '\\'), strAuthorizationToken.end());
	return true;
}


bool HarmonyHubAPI::connectToHarmony(const std::string strHarmonyIPAddress, csocket& harmonyCommunicationcsocket)
{
	if (strHarmonyIPAddress.empty())
	{
		errorString = "connectToHarmony : Empty Harmony IP Address";
		return false;
	}

	harmonyCommunicationcsocket.connect(strHarmonyIPAddress.c_str(), HARMONY_COMMUNICATION_PORT);

	if (harmonyCommunicationcsocket.getState() != csocket::CONNECTED)
	{
		errorString = "connectToHarmony : Unable to connect to specified IP Address on Harmony communication Port";
		return false;
	}

	return true;
}


bool HarmonyHubAPI::startCommunication(csocket* communicationcsocket, std::string strUserName, std::string strPassword)
{
	if(communicationcsocket == NULL || strUserName.empty() || strPassword.empty())
	{
		errorString = "startCommunication : Invalid communication parameter(s) provided";
		return false;
	}

	// Start communication
	std::string data = "<stream:stream to='connect.logitech.com' xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' xml:lang='en' version='1.0'>";
	communicationcsocket->write(data.c_str(), static_cast<unsigned int>(data.length()));
	memset(databuffer, 0, DATABUFFER_SIZE);
	communicationcsocket->read(databuffer, DATABUFFER_SIZE, false);

	std::string strData = databuffer;
/*
Expect: <?xml version='1.0' encoding='iso-8859-1'?><stream:stream from='' id='XXXXXXXX' version='1.0' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams'><stream:features><mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'><mechanism>PLAIN</mechanism></mechanisms></stream:features>
*/

	data = "<auth xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" mechanism=\"PLAIN\">";
	std::string tmp = "\0";
	tmp.append(strUserName);
	tmp.append("\0");
	tmp.append(strPassword);
	data.append(base64_encode(tmp.c_str(), static_cast<unsigned int>(tmp.length())));
	data.append("</auth>");
	communicationcsocket->write(data.c_str(), static_cast<unsigned int>(data.length()));

	memset(databuffer, 0, DATABUFFER_SIZE);
	communicationcsocket->read(databuffer, DATABUFFER_SIZE, false);

	strData = databuffer; /* <- Expect: <success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/> */
	if(strData != "<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>")
	{
		errorString = "startCommunication : connection error";
		return false;
	}

	data = "<stream:stream to='connect.logitech.com' xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' xml:lang='en' version='1.0'>";
	communicationcsocket->write(data.c_str(), static_cast<unsigned int>(data.length()));

	memset(databuffer, 0, DATABUFFER_SIZE);
	communicationcsocket->read(databuffer, DATABUFFER_SIZE, false);

	strData = databuffer;
/*
Expect: <?xml version='1.0' encoding='iso-8859-1'?><stream:stream from='' id='057a30bd' version='1.0' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams'><stream:features><mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'><mechanism>PLAIN</mechanism></mechanisms></stream:features>
*/

	return true;
}


bool HarmonyHubAPI::swapAuthorizationToken(csocket* authorizationcsocket, std::string& strAuthorizationToken)
{
	if (authorizationcsocket == NULL || strAuthorizationToken.empty())
	{
		errorString = "swapAuthorizationToken : NULL csocket or empty authorization token provided";
		return false;
	}

	if (!startCommunication(authorizationcsocket, "guest", "gatorade."))
	{
		errorString = "swapAuthorizationToken : Communication failure";
		return false;
	}

	std::string strData;
	std::string sendData;

	// GENERATE A LOGIN ID REQUEST USING THE HARMONY ID AND LOGIN AUTHORIZATION TOKEN
	sendData = "<iq type=\"get\" id=\"";
	sendData.append(CONNECTION_ID);
	sendData.append("\"><oa xmlns=\"connect.logitech.com\" mime=\"vnd.logitech.connect/vnd.logitech.pair\">token=");
	sendData.append(strAuthorizationToken.c_str());
	sendData.append(":name=foo#iOS6.0.1#iPhone</oa></iq>");

	std::string strIdentityTokenTag = "identity=";
	size_t pos = std::string::npos;

	authorizationcsocket->write(sendData.c_str(), static_cast<unsigned int>(sendData.length()));

	memset(databuffer, 0, DATABUFFER_SIZE);
	authorizationcsocket->read(databuffer, DATABUFFER_SIZE, false);

	strData = databuffer;
/*
Expect: <iq/> ... <success xmlns= ... identity=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:status=succeeded ...
*/

	if (strData.find("<iq/>") != 0)
	{
		 errorString = "swapAuthorizationToken : Invalid Harmony response";
		 return false;
	}

	bool bIsDataReadable = false;
	authorizationcsocket->canRead(&bIsDataReadable, 0.3f);
	if (!bIsDataReadable && strData == "<iq/>")
	{
		bIsDataReadable = true;
	}

	while (bIsDataReadable)
	{
		memset(databuffer, 0, DATABUFFER_SIZE);
		authorizationcsocket->read(databuffer, DATABUFFER_SIZE, false);
		std::string strMoreData = std::string(databuffer);
		if (!strMoreData.empty())
		{
			strData.append(databuffer);
			authorizationcsocket->canRead(&bIsDataReadable, 0.3f);
		}
		else
			bIsDataReadable = false;
	}

	// Parse the session authorization token from the response
	pos = (int)strData.find(strIdentityTokenTag);
	if (pos == std::string::npos)
	{
		errorString = "swapAuthorizationToken : Logitech Harmony response does not contain a session authorization token";
		return false;
	}

	strAuthorizationToken = strData.substr(pos + strIdentityTokenTag.length());

	pos = (int)strAuthorizationToken.find(":status=succeeded");
	if (pos == std::string::npos)
	{
		errorString = "swapAuthorizationToken : Logitech Harmony response does not contain a valid session authorization token";
		return false;
	}

	strAuthorizationToken = strAuthorizationToken.substr(0, pos);
	return true;
}


bool HarmonyHubAPI::submitCommand(csocket* commandcsocket, std::string& strAuthorizationToken, std::string strCommand, std::string strCommandParameterPrimary, std::string strCommandParameterSecondary, std::string& resultString)
{
	if ((commandcsocket == NULL) || strAuthorizationToken.empty())
	{
		errorString = "submitCommand : NULL csocket or empty authorization token provided";
		return false;
	}

	std::string lstrCommand = strCommand;
	if (lstrCommand.empty())
	{
		// No command provided, return query for the current activity
		lstrCommand = "get_current_activity_id";
//		return true;
	}

	std::string strData;
	std::string sendData;

	sendData = "<iq type=\"get\" id=\"";
	sendData.append(CONNECTION_ID);
	sendData.append("\"><oa xmlns=\"connect.logitech.com\" mime=\"vnd.logitech.harmony/vnd.logitech.harmony.engine?");

	// Issue the provided command
	if (lstrCommand == "get_current_activity_id" || lstrCommand == "get_current_activity_id_raw")
	{
		sendData.append("getCurrentActivity\" /></iq>");
	}
	else if (lstrCommand == "get_config_raw")
	{
		sendData.append("config\"></oa></iq>");
	}
	else if (lstrCommand == "start_activity" || strCommand == "start_activity_raw")
	{
		sendData.append("startactivity\">activityId=");
		sendData.append(strCommandParameterPrimary.c_str());
		sendData.append(":timestamp=0</oa></iq>");
	}
	if ((lstrCommand == "issue_device_command") || (lstrCommand == "issue_device_command_raw"))
	{
		sendData.append("holdAction\">action={\"type\"::\"IRCommand\",\"deviceId\"::\"");
		sendData.append(strCommandParameterPrimary.c_str());
		sendData.append("\",\"command\"::\"");
		sendData.append(strCommandParameterSecondary.c_str());
		sendData.append("\"}:status=press</oa></iq>");
	}

	commandcsocket->write(sendData.c_str(), static_cast<unsigned int>(sendData.length()));

	memset(databuffer, 0, DATABUFFER_SIZE);
	commandcsocket->read(databuffer, DATABUFFER_SIZE, false);
	strData = databuffer;
/*
Expect: strData  == <iq/>
*/

	std::string iqTag = "<iq/>";
	size_t pos = strData.find(iqTag);

	if (pos != 0)
	{
		errorString = "submitCommand: Invalid Harmony response";
		return false;
	}

	// Harmony does not return success or failure after issuing a device command
	if ((lstrCommand == "issue_device_command") || (lstrCommand == "issue_device_command_raw"))
	{
		resultString = "";
		return true;
	}

	bool bIsDataReadable = false;
	commandcsocket->canRead(&bIsDataReadable, 0.6f);

	while (bIsDataReadable == true)
	{
		memset(databuffer, 0, DATABUFFER_SIZE);
		commandcsocket->read(databuffer, DATABUFFER_SIZE, false);
		std::string strMoreData = std::string(databuffer);
		if (!strMoreData.empty())
		{
			strData.append(databuffer);
			commandcsocket->canRead(&bIsDataReadable, 0.3f);
		}
		else
			bIsDataReadable = false;
	}

	resultString = strData;

	if (lstrCommand == "get_current_activity_id" || lstrCommand == "get_current_activity_id_raw")
	{
		size_t resultStartPos = resultString.find("result=");
		size_t resultEndPos = resultString.find("]]>");

		if (resultStartPos != std::string::npos && resultEndPos != std::string::npos)
		{
			resultString = resultString.substr(resultStartPos + 7, resultEndPos - resultStartPos - 7);
			resultString.insert(0, "{\"current activity\":");
			resultString.append("}");
		}

	}
	else if (lstrCommand == "get_config" || lstrCommand == "get_config_raw")
	{
/*
 * This is pointless code. Original code tries to get more data from HarmonyHub
 * but it is highly unlikely that there will be any. Also, because this code
 * never resets strData any data that might be received here can only confuse
 * the parser functions.
 *

		commandcsocket->canRead(&bIsDataReadable, 0.3f);

#ifndef WIN32
/*
 * This is really bad practice. Because of an incorrect implementation in csocket canRead()
 * the original programmer found it necessary to override its return here, causing the
 * application to stall for quite some time as timeout doesn't appear to be obeyed as well.
 *
		bIsDataReadable = true;
#endif

		while (bIsDataReadable)
		{
			memset(databuffer, 0, 1000000);
			commandcsocket->read(databuffer, 1000000, false);
			strData.append(databuffer);
			commandcsocket->canRead(&bIsDataReadable, 0.3f);
		}
*/
		pos = strData.find("![CDATA[{");
		if (pos != std::string::npos)
		{
			if (strCommand == "get_config")
				resultString = "Logitech Harmony Configuration : \n" + strData.substr(pos + 8);
			else
				resultString = strData.substr(pos + 8);
		}
	}
	else if (lstrCommand == "start_activity" || lstrCommand == "start_activity_raw")
	{
		resultString = "{\"current activity\":" + strCommandParameterPrimary + "}";
	}
	return true;
}


bool HarmonyHubAPI::_parseAction(Json::Value *jAction, std::vector<Action>& vecDeviceActions, const std::string& strDeviceID)
{
	Action a;
	a.m_strName = (*jAction)["name"].asString();
	a.m_strLabel = (*jAction)["label"].asString();

	std::string actionline = (*jAction)["action"].asString();
	std::stringstream ssaction;
	bool isescape = false;
	for (size_t i = 0; i < actionline.length(); i++)
	{
		if ((actionline[i] == '\\') && (!isescape))
			isescape = true;
		else
		{
			ssaction << actionline[i];
			isescape = false;
		}
	}

	Json::Value j_action;
	Json::Reader jReader;
	if (!jReader.parse(ssaction.str().c_str(), j_action))
		return false;

	a.m_strCommand = j_action["command"].asString();
	std::string commandDeviceID = j_action["deviceId"].asString();
	if (commandDeviceID != strDeviceID)
		return false;

	vecDeviceActions.push_back(a);
	return true;
}


bool HarmonyHubAPI::_parseFunction(Json::Value *jFunction, std::vector<Function>& vecDeviceFunctions, const std::string& strDeviceID)
{
	Function f;
	f.m_strName = (*jFunction)["name"].asString();

	size_t l = (*jFunction)["function"].size();
	for (size_t i = 0; i < l; ++i)
	{
		_parseAction(&(*jFunction)["function"][(int)(i)], f.m_vecActions, strDeviceID);
	}
	vecDeviceFunctions.push_back(f);
	return true;
}


bool HarmonyHubAPI::_parseControlGroup(Json::Value *jControlGroup, std::vector<Function>& vecDeviceFunctions, const std::string& strDeviceID)
{
	size_t l = jControlGroup->size();

	for (size_t i = 0; i < l; ++i)
	{
		if(!_parseFunction(&(*jControlGroup)[(int)(i)], vecDeviceFunctions, strDeviceID))
			return 1;
	}
	return true;
}



bool HarmonyHubAPI::parseConfiguration(const std::string& strConfiguration, std::map< std::string, std::string >& mapActivities, std::vector< Device >& vecDevices)
{
	Json::Value j_conf;
	Json::Reader jReader;
	if (!jReader.parse(strConfiguration.c_str(), j_conf))
		return false;

	size_t l = j_conf["activity"].size();
	for (size_t i = 0; i < l; ++i)
	{
		std::string strActivityLabel = j_conf["activity"][(int)(i)]["label"].asString();
		std::string strActivityID = j_conf["activity"][(int)(i)]["id"].asString();
		mapActivities.insert(std::map< std::string, std::string>::value_type(strActivityID, strActivityLabel));
	}

	// Search for devices and commands
	l = j_conf["device"].size();
	for (size_t i = 0; i < l; ++i)
	{
		Device d;

		d.m_strID = j_conf["device"][(int)(i)]["id"].asString();
		d.m_strLabel = j_conf["device"][(int)(i)]["label"].asString();
		d.m_strType = j_conf["device"][(int)(i)]["type"].asString();
		d.m_strManufacturer = j_conf["device"][(int)(i)]["manufacturer"].asString();
		d.m_strModel = j_conf["device"][(int)(i)]["model"].asString();

		// Parse Commands
		_parseControlGroup(&j_conf["device"][(int)(i)]["controlGroup"], d.m_vecFunctions, d.m_strID);

		vecDevices.push_back(d);
	}
	return true;
}



std::string HarmonyHubAPI::getErrorString()
{
	return errorString;
}
