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


#define HARMONY_COMMUNICATION_PORT 5222


using namespace std;


void log(const char* message, bool bQuiet)
{
	if(bQuiet)
		return;

	cout << message << endl;
}


int main(int argc, char * argv[])
{
	HarmonyHubAPI hclient = HarmonyHubAPI();

	ifstream myFile("HarmonyHub.AuthorizationToken");
	if (myFile.fail()) {
		ofstream auth("HarmonyHub.AuthorizationToken");
		auth << "KsRE6VVA3xrhtbqFbh0jWn8YTiweDeB/b94Qeqf3ofWGM79zLSr62XQh8geJxw/V";
	}

	if (argc < 2)
	{
		const char *suffix = "";
#if WIN32
		suffix = ".exe";
#endif
		printf("Syntax:\n");
		printf("HarmonyHubControl%s [harmony_ip] [command (optional)]\n", suffix);
		printf("    where command can be any of the following:\n");
		printf("        list_activities\n");
		printf("        list_activities_raw\n");
		printf("        get_current_activity_id\n");
		printf("        get_current_activity_id_raw\n");
		printf("        start_activity [ID]\n");
		printf("        list_devices\n");
		printf("        list_devices_raw\n");
		printf("        list_device_commands [deviceId]\n");
		printf("        list_device_commands_raw [deviceId]\n");
		printf("        issue_device_command [deviceId] [command]\n");
		printf("        get_config\n");
		printf("        get_config_raw\n");
		printf("\n");
		return 0;
	}

	std::string strUserEmail = "guest@x.com";
	std::string strUserPassword = "guest";
	std::string strHarmonyIP = argv[1];
	std::string strCommand;
	std::string strCommandParameterPrimary;
	std::string strCommandParameterSecondary;
	std::string strCommandParameterThird;
	std::string strCommandParameterFourth;

	// User requested an action to be performed
	if(argc >= 3)
		strCommand = argv[2];

	if(argc >= 4)
		strCommandParameterPrimary = argv[3];

	if(argc >= 5)
		strCommandParameterSecondary = argv[4];

	if(argc >= 6)
		strCommandParameterThird = argv[5];

	if(argc == 7)
		strCommandParameterFourth = argv[6];

	bool bQuietMode = ((strCommand.length() > 0) && (strCommand.find("_raw") != std::string::npos));


	// Read the token
	std::string strAuthorizationToken = hclient.ReadAuthorizationTokenFile();

	//printf("\nLogin Authorization Token is: %s\n\n", strAuthorizationToken.c_str());

	bool bAuthorizationComplete = false;

	if (strAuthorizationToken.length() > 0)
	{
		csocket authorizationcsocket;
		if(!hclient.connectToHarmony(strHarmonyIP, authorizationcsocket))
		{
			log("HARMONY COMMUNICATION LOGIN    : FAILURE", false);
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
			log("LOGITECH WEB SERVICE LOGIN     : FAILURE", false);
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
			log("HARMONY COMMUNICATION LOGIN    : FAILURE", false);
			cerr << "ERROR : " << hclient.getErrorString() << endl;
			return 1;
		}

		if(!hclient.swapAuthorizationToken(&authorizationcsocket, strAuthorizationToken))
		{
			log("HARMONY COMMUNICATION LOGIN    : FAILURE", false);
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
		log("HARMONY COMMAND SUBMISSION     : FAILURE", false);
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
	if(!hclient.submitCommand(&commandcsocket, strAuthorizationToken, lstrCommand, strCommandParameterPrimary, strCommandParameterSecondary, strCommandParameterThird, strCommandParameterFourth, resultString))
	{
		log("HARMONY COMMAND SUBMISSION     : FAILURE", false);
		cerr << "ERROR : " << hclient.getErrorString() << endl;
		return 1;
	}
	log("HARMONY COMMAND SUBMISSION     : SUCCESS", bQuietMode);

	if (lstrCommand == "get_config_raw")
	{
		std::string prettyheader="";

		std::map< std::string, std::string> mapActivities;
		std::vector< Device > vecDevices;

		if (!hclient.parseConfiguration(resultString, mapActivities, vecDevices))
		{
			log("PARSE ACTIVITIES AND DEVICES   : FAILURE", false);
			cerr << "ERROR : " << hclient.getErrorString() << endl;
			return 1;
		}

		ofstream config("config.json");
		config << resultString << endl;

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
			activities << resultString << endl;
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
			devices << resultString << endl;
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
				if (it->m_strID == strCommandParameterPrimary)
				{
					if(strCommandParameterSecondary.length())
					{
						if (strCommandParameterSecondary == it->m_strID)
						{
							resultString.append("\"");
							resultString.append(it->toString());
						}
					}
					else
					{
						resultString.append("\"");
						resultString.append(it->toString());
					}
					resultString=resultString.substr(0, resultString.size()-1);
					resultString.append("}" );
					break;
				}
			}
			string filename;
			filename = strCommandParameterPrimary;
			filename.append(".json");
			ofstream myfile( filename.c_str() );
			myfile << resultString << endl;
		}
		log("PARSE ACTIVITIES AND DEVICES   : SUCCESS", bQuietMode);
		cout << prettyheader;
	}

	cout << resultString << endl;
	return 0;
}
