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

//#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include "HarmonyHubAPI/csocket.h"
#include "HarmonyHubAPI/HarmonyHubAPI.h"


using namespace std;


HarmonyHubAPI hclient;

void log(const std::string message, bool bQuiet)
{
	if (bQuiet)
	{
		if (!hclient.getErrorString().empty())
		{
			cout << "{\n   \"status\":\"error\",\n   \"message\":\"" << hclient.getErrorString() << "\"\n}\n";
		}
		else if (message.find("FAILURE") != std::string::npos)
		{
			cout << "{\n   \"status\":\"error\",\n   \"message\":\"general failure\"\n}\n";
		}
		return;
	}

	cout << message << endl;
}


int usage(int returnstate)
{
	const char *suffix = "";
#if WIN32
	suffix = ".exe";
#endif
	printf("Syntax:\n");
	printf("HarmonyHubControl%s [harmony_ip] [command]\n", suffix);
	printf("    where command can be any of the following:\n");
	printf("        list_activities\n");
	printf("        list_activities_raw\n");
	printf("        get_current_activity_id\n");
	printf("        get_current_activity_id_raw\n");
	printf("        start_activity <ID>\n");
	printf("        start_activity_raw <ID>\n");
	printf("        list_devices\n");
	printf("        list_devices_raw\n");
	printf("        list_device_commands <deviceId>\n");
	printf("        list_device_commands_raw <deviceId>\n");
	printf("        issue_device_command <deviceId> <command> [...]\n");
	printf("        issue_device_command_raw <deviceId> <command> [...]\n");
	printf("        get_config\n");
	printf("        get_config_raw\n");
	printf("\n");
	return returnstate;
}


int main(int argc, char * argv[])
{
	hclient = HarmonyHubAPI();

	ifstream myFile("HarmonyHub.AuthorizationToken");
	if (myFile.fail()) {
		ofstream auth("HarmonyHub.AuthorizationToken");
		auth << "KsRE6VVA3xrhtbqFbh0jWn8YTiweDeB/b94Qeqf3ofWGM79zLSr62XQh8geJxw/V";
	}

	if (argc < 2)
	{
		return usage(0);
	}

	std::string strUserEmail = "guest@x.com";
	std::string strUserPassword = "guest";
	std::string strHarmonyIP = argv[1];
	std::string strCommand;
	std::vector<std::string> strCommandParameters;

	// User requested an action to be performed
	if(argc >= 3)
	{
		strCommand = argv[2];

		if (strCommand == "turn_off")
		{
			strCommand = "start_activity";
			strCommandParameters[0] = "-1";
		}
		else if (
			(strCommand == "list_activities") ||
			(strCommand == "list_activities_raw") ||
			(strCommand == "get_current_activity_id") ||
			(strCommand == "get_current_activity_id_raw") ||
			(strCommand == "list_devices") ||
			(strCommand == "list_devices_raw") ||
			(strCommand == "get_config") ||
			(strCommand == "get_config_raw")
		    )
		{
			// all fine here
		}
		else if (
			(strCommand == "start_activity") ||
			(strCommand == "start_activity_raw") ||
			(strCommand == "list_device_commands") ||
			(strCommand == "list_device_commands_raw")
		    )
		{
			// requires a parameter
			if (argc < 4)
				return usage(1);

			strCommandParameters.push_back(argv[3]);
		}
		else if (
			(strCommand == "issue_device_command") ||
			(strCommand == "issue_device_command_raw")
			)
		{
			// requires at least 2 parameters
			if (argc < 5)
				return usage(1);

			strCommandParameters.push_back(argv[3]);
			strCommandParameters.push_back(argv[4]);
			if(argc >= 6)
			{
				for (int i = 5; i < argc; i++)
				{
					strCommandParameters.push_back(argv[i]);
				}
			}
		}
		else
		{
			// unknown command
			return usage(1);
		}
	}

	bool bQuietMode = ((!strCommand.empty()) && (strCommand.find("_raw") != std::string::npos));


 	// Read the token
	std::string strAuthorizationToken = hclient.ReadAuthorizationTokenFile();

	//printf("\nLogin Authorization Token is: %s\n\n", strAuthorizationToken.c_str());

	bool bAuthorizationComplete = false;

	if (strAuthorizationToken.length() > 0)
	{
		csocket authorizationcsocket;
		if(!hclient.connectToHarmony(strHarmonyIP, authorizationcsocket))
		{
			log("HARMONY COMMUNICATION LOGIN    : FAILURE", bQuietMode);
			cerr << "ERROR : " << hclient.getErrorString() << endl;
			return 1;
		}

		if(hclient.swapAuthorizationToken(&authorizationcsocket, strAuthorizationToken))
		{
			// Authorization Token found in the file worked.
			// Bypass authorization through Logitech's servers.
			log("LOGITECH WEB SERVICE LOGIN     : BYPASSED", bQuietMode);
			bAuthorizationComplete = true;
		}
	}


	if (!bAuthorizationComplete)
	{
		// Log into the Logitech Web Service to retrieve the login authorization token
		if(hclient.harmonyWebServiceLogin(strUserEmail, strUserPassword, strAuthorizationToken) == 1)
		{
			log("LOGITECH WEB SERVICE LOGIN     : FAILURE", bQuietMode);
			cerr << "ERROR : " << hclient.getErrorString() << endl;
			return 1;
		}
		log("LOGITECH WEB SERVICE LOGIN     : SUCCESS", bQuietMode);

		//printf("\nLogin Authorization Token is: %s\n\n", strAuthorizationToken.c_str());

		// Write the Authorization Token to an Authorization Token file to bypass this step
		// on future sessions
		hclient.WriteAuthorizationTokenFile(strAuthorizationToken);

		// Log into the harmony hub to convert the login authorization token for a
		// session authorization token

		csocket authorizationcsocket;
		if(!hclient.connectToHarmony(strHarmonyIP, authorizationcsocket))
		{
			log("HARMONY COMMUNICATION LOGIN    : FAILURE", bQuietMode);
			cerr << "ERROR : " << hclient.getErrorString() << endl;
			return 1;
		}

		if(!hclient.swapAuthorizationToken(&authorizationcsocket, strAuthorizationToken))
		{
			log("HARMONY COMMUNICATION LOGIN    : FAILURE", bQuietMode);
			cerr << "ERROR : " << hclient.getErrorString() << endl;
			return 1;
		}
	}

	log("HARMONY COMMUNICATION LOGIN    : SUCCESS", bQuietMode);


	//printf("\nSession Authorization Token is: %s\n\n", strAuthorizationToken.c_str());

	// We've successfully obtained our session authorization token from the harmony hub
	// using the login authorization token we received earlier from the Logitech web service.
	// Now, disconnect from the harmony and reconnect using the mangled session token
	// as our username and password to issue a command.


	csocket commandcsocket;
	if(!hclient.connectToHarmony(strHarmonyIP, commandcsocket))
	{
		log("HARMONY COMMAND SUBMISSION     : FAILURE", bQuietMode);
		cerr << "ERROR : " << hclient.getErrorString() << endl;
		return 1;
	}

	std::string strUserName = strAuthorizationToken;
	//strUserName.append("@connect.logitech.com/gatorade.");
	std::string strPassword = strAuthorizationToken;

	if(!hclient.startCommunication(&commandcsocket, strUserName, strPassword))
	{
		cerr << "ERROR : Communication failure" << endl;
		return 1;
	}

	std::string lstrCommand = strCommand;
	if (strCommand == "list_activities"      || strCommand == "list_activities_raw"      ||
	    strCommand == "list_devices"         || strCommand == "list_devices_raw"         ||
	    strCommand == "list_commands"        || strCommand == "list_commands_raw"        ||
	    strCommand == "list_device_commands" || strCommand == "list_device_commands_raw" ||
	    strCommand == "get_config"
	)
	{
		lstrCommand = "get_config_raw";
	}

	std::string resultString;

	// fill up to minimum required size
	while (strCommandParameters.size() < 2)
	{
		strCommandParameters.push_back("");
	}

	// run this in a loop to allow sending multiple device commands in sequence
	// (e.g. channel select with multiple digits)
	for (size_t i = 1; i < strCommandParameters.size(); i++)
	{
		if(!hclient.submitCommand(&commandcsocket, strAuthorizationToken, lstrCommand, strCommandParameters[0], strCommandParameters[i], resultString))
		{
			log("HARMONY COMMAND SUBMISSION     : FAILURE", bQuietMode);
			cerr << "ERROR : " << hclient.getErrorString() << endl;
			return 1;
		}
	}	

	log("HARMONY COMMAND SUBMISSION     : SUCCESS", bQuietMode);

	Json::Value j_result;
	Json::Reader jReader;
	jReader.parse(resultString.c_str(), j_result);

	if (lstrCommand.empty() || (lstrCommand.find("activity") != std::string::npos))
	{
		std::ofstream current("current.json");
		current << j_result.toStyledString();
	}

	if (bQuietMode && resultString.empty())
	{
		cout << "{\n   \"status\":\"ok\"\n}\n";
		return 0;
	}

	if (lstrCommand == "get_config_raw")
	{
		ofstream config("config.json");
		config << j_result.toStyledString();

		std::string prettyheader="";

		std::map< std::string, std::string> mapActivities;
		std::vector< Device > vecDevices;

		if (!hclient.parseConfiguration(resultString, mapActivities, vecDevices))
		{
			log("PARSE ACTIVITIES AND DEVICES   : FAILURE", bQuietMode);
			cerr << "ERROR : " << hclient.getErrorString() << endl;
			return 1;
		}

		if (strCommand == "list_activities" || strCommand == "list_activities_raw")
		{
			resultString = "{";
			if(strCommand == "list_activities")
				prettyheader = "Activities Available via Harmony : \n\n";

			std::map< std::string, std::string>::iterator it = mapActivities.begin();
			std::map< std::string, std::string>::iterator ite = mapActivities.end();
			for (; it != ite; ++it)
			{
				resultString.append("\"");
				resultString.append(it->second);
				resultString.append("\":\"");
				resultString.append(it->first);
				resultString.append("\",");
			}
			resultString=resultString.substr(0, resultString.size()-1);
			resultString.append("}");
			ofstream activities("activities.json");
			jReader.parse(resultString.c_str(), j_result);
			activities << j_result.toStyledString();
		}

		if (strCommand == "list_devices" || strCommand == "list_devices_raw")
		{
			resultString ="{";
			if (strCommand == "list_devices")
				prettyheader = "Devices Controllable via Harmony : \n\n";

			std::vector< Device >::iterator it = vecDevices.begin();
			std::vector< Device >::iterator ite = vecDevices.end();
			for (; it != ite; ++it)
			{
				resultString.append("\"");
				resultString.append(it->m_strLabel );
				resultString.append("\":\"");
				resultString.append(it->m_strID );
				resultString.append("\",");
			}
			resultString=resultString.substr(0, resultString.size()-1);
			resultString.append("}");
			ofstream devices("devices.json");
			jReader.parse(resultString.c_str(), j_result);
			devices << j_result.toStyledString();
		}

		if (strCommand == "list_commands" || strCommand == "list_commands_raw")
		{
			resultString = "";
			if (strCommand == "list_commands")
				prettyheader = "Devices Controllable via Harmony with Commands : \n\n";

			std::vector< Device >::iterator it = vecDevices.begin();
			std::vector< Device >::iterator ite = vecDevices.end();
			for (; it != ite; ++it)
			{
				resultString.append(it->toString());
				resultString.append("\n\n\n");
			}
		}

		if (strCommand == "list_device_commands" || strCommand == "list_device_commands_raw")
		{
			resultString = "{";
			if (strCommand == "list_device_commands")
				prettyheader = "Harmony Commands for Device: \n\n";

			std::vector< Device >::iterator it = vecDevices.begin();
			std::vector< Device >::iterator ite = vecDevices.end();
			for (; it != ite; ++it)
			{
				if (it->m_strID == strCommandParameters[0])
				{
					resultString.append("\"");
					resultString.append(it->toString());
					resultString=resultString.substr(0, resultString.size()-1);
					resultString.append("}" );
					break;
				}
			}
			string filename;
			filename = strCommandParameters[0];
			filename.append(".json");
			ofstream myfile( filename.c_str() );
			jReader.parse(resultString.c_str(), j_result);
			myfile << j_result.toStyledString();
		}
		log("PARSE ACTIVITIES AND DEVICES   : SUCCESS", bQuietMode);
		cout << prettyheader;
	}

//	cout << resultString << endl;
//return 0;
/*
	Json::Value j_conf;
	Json::Reader jReader;
	if (!jReader.parse(resultString.c_str(), j_conf))
		cout << "error parsing resultstring\n";
*/
	cout << j_result.toStyledString();
	return 0;
}
