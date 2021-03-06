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

	static const std::string base64_chars;
	static inline bool is_base64(unsigned char c);
	std::string base64_encode(char const* bytes_to_encode, unsigned int in_len);
	std::string base64_decode(std::string const& encoded_string);

	bool _parseAction(Json::Value *jAction, std::vector<Action>& vecDeviceActions, const std::string& strDeviceID);
	bool _parseFunction(Json::Value *jFunction, std::vector<Function>& vecDeviceFunctions, const std::string& strDeviceID);
	bool _parseControlGroup(Json::Value *jControlGroup, std::vector<Function>& vecDeviceFunctions, const std::string& strDeviceID);
	bool CheckIqGood(const std::string& strData);


	public:
	HarmonyHubAPI();
	~HarmonyHubAPI();

	bool ParseConfiguration(const std::string& strConfiguration, std::map< std::string, std::string >& mapActivities, std::vector< Device >& vecDevices);

	bool HarmonyWebServiceLogin(std::string strUserEmail, std::string strPassword, std::string& strAuthorizationToken );
	bool ConnectToHarmony(const std::string strHarmonyIPAddress, csocket& harmonyCommunicationcsocket);
	bool StartCommunication(csocket* communicationcsocket, std::string strUserName, std::string strPassword);
	bool CloseCommunication(csocket* communicationcsocket);
	bool SwapAuthorizationToken(csocket* authorizationcsocket, std::string& strAuthorizationToken);
	bool SubmitCommand(csocket* commandcsocket, std::string& strAuthorizationToken, std::string strCommand, std::string
			strCommandParameterPrimary, std::string strCommandParameterSecondary, std::string& resultString);

	const std::string ReadAuthorizationTokenFile();
	bool WriteAuthorizationTokenFile(const std::string& strAuthorizationToken);
	bool SendPing(csocket* commandcsocket, std::string& strAuthorizationToken);
	std::string ReadData(csocket* commandcsocket);
	std::string ReadData(csocket* commandcsocket, float waitTime);

	std::string GetErrorString();

};

#endif
