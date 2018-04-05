/*
 * HarmonyHub API
 *
 * Source code subject to GNU GENERAL PUBLIC LICENSE version 3
 */

#ifndef _HarmonyHubAPI
#define _HarmonyHubAPI

#include <vector>
#include <string>
#include "jsoncpp/json.h"
#include "csocket.h"

class Action
{
public:
	std::string m_strCommand;
	std::string m_strName;
	std::string m_strLabel;
	std::string toString()
	{
		return m_strCommand;
	}
};


class Function
{
public:
	std::string m_strName;
	std::vector< Action > m_vecActions;
	std::string toString()
	{
		std::string ret=("\"");
	ret.append(m_strName);
	ret.append("\":[");
		std::vector<Action>::iterator it = m_vecActions.begin();
		std::vector<Action>::iterator ite = m_vecActions.end();
		for(; it != ite; ++it)
		{
	ret.append("\n\t");
	ret.append("\"");
		ret.append(it->toString());
	ret.append("\",");
		}
		ret=ret.substr(0, ret.size()-1);
		ret.append("],\n");
		return ret;
	}
};

class Device
{
public:
	std::string m_strID;
	std::string m_strLabel;
	std::string m_strManufacturer;
	std::string m_strModel;
	std::string m_strType;
	std::vector< Function > m_vecFunctions;

	std::string toString()
	{
	std::string ret = m_strType;
	ret.append("\":\"");
		ret.append(m_strLabel);
	ret.append("\"");
		ret.append(",\"ID\" :");
		ret.append(m_strID);
		ret.append(",\"");
		ret.append(m_strManufacturer);
	ret.append("\":\"");
		ret.append(m_strModel);
	ret.append("\"");
		ret.append(",\"Functions\": { \n ");
		std::vector<Function>::iterator it = m_vecFunctions.begin();
		std::vector<Function>::iterator ite = m_vecFunctions.end();
		for(; it != ite; ++it)
		{
			ret.append(it->toString());
		}
		ret=ret.substr(0, ret.size()-2);
		ret.append("}\n");
		return ret;
	}
};

class HarmonyHubAPI
{
	private:

	std::string errorString;
	std::string resultString;
	char databuffer[1000000];


	static const std::string base64_chars;
	static inline bool is_base64(unsigned char c);
	std::string base64_encode(char const* bytes_to_encode, unsigned int in_len);
	std::string base64_decode(std::string const& encoded_string);

	bool _parseAction(Json::Value *jAction, std::vector<Action>& vecDeviceActions, const std::string& strDeviceID);
	bool _parseFunction(Json::Value *jFunction, std::vector<Function>& vecDeviceFunctions, const std::string& strDeviceID);
	bool _parseControlGroup(Json::Value *jControlGroup, std::vector<Function>& vecDeviceFunctions, const std::string& strDeviceID);

	public:
	HarmonyHubAPI();
	~HarmonyHubAPI();

	bool parseConfiguration(const std::string& strConfiguration, std::map< std::string, std::string >& mapActivities, std::vector< Device >& vecDevices);

	bool harmonyWebServiceLogin(std::string strUserEmail, std::string strPassword, std::string& strAuthorizationToken );
	bool connectToHarmony(const std::string strHarmonyIPAddress, csocket& harmonyCommunicationcsocket);
	bool startCommunication(csocket* communicationcsocket, std::string strUserName, std::string strPassword);
	bool swapAuthorizationToken(csocket* authorizationcsocket, std::string& strAuthorizationToken);
	bool submitCommand(csocket* commandcsocket, std::string& strAuthorizationToken, std::string strCommand, std::string
			strCommandParameterPrimary, std::string strCommandParameterSecondary, std::string& resultString);

	const std::string ReadAuthorizationTokenFile();
	bool WriteAuthorizationTokenFile(const std::string& strAuthorizationToken);

	std::string getErrorString();


};

#endif
