// nptest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <libnp.h>
#include <iostream>
#include <fstream>

#include <string>
#include <sstream>

bool done = false;

void WatHappen(NPAsync<NPAuthenticateResult>* async)
{
	NPAuthenticateResult* result = async->GetResult();
	printf("%d %llx\n", result->result, result->id);

	//NP_SetExternalSteamID(0x1100001026753C3);

	done = true;
}

void RegisterServerCB(NPAsync<NPRegisterServerResult>* async)
{
	NPRegisterServerResult* result = async->GetResult();
	printf("license key %s; id %i\n", result->licenseKey, result->serverID);

	NPAsync<NPAuthenticateResult>* async2 = NP_AuthenticateWithLicenseKey(result->licenseKey);
	async2->SetCallback(WatHappen, NULL);
}

void write_to_file(uint8_t *ptr, size_t len) {
	std::ofstream fp;
	fp.open("FileResultCB.data", std::ios::out | std::ios::binary);
	fp.write((char*)ptr, len);
}

void FileResultCB(NPAsync<NPGetPublisherFileResult>* async)
{
	NPGetPublisherFileResult* result = async->GetResult();
	printf("returned %d\nread %d bytes\n", result->result, result->fileSize);
	write_to_file(result->buffer, sizeof(result->buffer));
}

void GetSessionCB(NPAsync<bool>* async)
{
	printf("%d sessions\n", NP_GetNumSessions());

	for (int i = 0; i < NP_GetNumSessions(); i++)
	{
		NPSessionInfo info;
		NP_GetSessionData(i, &info);
		printf("%s\n", info.hostname);
	}
}

void SessionCB(NPAsync<NPCreateSessionResult>* async)
{
	NPCreateSessionResult* result = async->GetResult();
	printf("sid %llu\n", result->sid);

	NPAsync<bool>* res = NP_RefreshSessions("synthesis");
	res->SetCallback(GetSessionCB, NULL);
}

void receivedFriends(NPAsync<NPFriendResult>* async)
{
	printf("receivedFriends()\n");
	NPFriendResult* result = async->GetResult();

	printf("Friends received! %i\n", result->numResults);

	for (int i = 0; i < result->numResults; i++)
	{
		printf("id %i\nUsername: %s\nHostname %s\n\n", result->friends[i].id, result->friends[i].username, result->friends[i].hostname);
	}
}

void Auth_VerifyIdentity();
char* Auth_GetSessionID();

//String to hex from Xen0 :)
/*
LPCSTR hexit(LPCSTR str)
{
	stringstream ss("");
	DWORD strlength = strlen(str);
	LPCSTR buff = (LPCSTR)malloc(strlength);

	for (BYTE i = 0; i < strlength; ++i)
	{
		ss << hex << uppercase << (DWORD)(BYTE)str[i]; //byte makes it unsigned, dword will give it the integer type. okayy...
	}
	//ah lol I know, you removed the ' ' that fucked stuff up
	strcpy((char*)buff, ss.str().c_str());
	return buff;
}
*/

void NP_LogCB(const char* message)
{
	printf("[NP] %s", message);
}

int _tmain(int argc, _TCHAR* argv[])
{
	printf("NP_Init()\n");
	NP_SetLogCallback(NP_LogCB);
	NP_Init();


	//Encryption test
	NP_Encrypt("JAJA");

	system("Pause");
	return 0;
	char* plain = "test";
	static char packetOutput[8192];
	int outPacketLen;
	// now, encrypt the packet

	//NP_Encrypt(plain, sizeof(plain), packetOutput, outPacketLen);

	//const char* test = hexit(packetOutput);

	//printf(test);

	printf("Auth_VerifyIdentity()\n");
	Auth_VerifyIdentity();
	printf("ID: %s\n", Auth_GetSessionID());



	//NP_Connect("iw4.prod.fourdeltaone.net", 3025);
	printf("Connect to np server...\n");
	NP_Connect("server.repziw4.de", 3037);//3036 = live np, 3037 is dev np
	printf("Connected\n");
	//NPAsync<NPAuthenticateResult>* async = NP_AuthenticateWithLicenseKey("123456789012345678901234");
	//NPAsync<NPAuthenticateResult>* async = NP_AuthenticateWithDetails("xnp", "xnpxnp");

	printf("NP_AuthenticateWithToken\n");
	NPAsync<NPAuthenticateResult>* async = NP_AuthenticateWithToken(Auth_GetSessionID());
	async->SetCallback(WatHappen, NULL);
	
	//result = async->Wait();
	//printf("Auth result: %s\n", result->result);
	
	//NPAsync<NPRegisterServerResult>* async = NP_RegisterServer("hello_world.cfg");
	//async->SetCallback(RegisterServerCB, NULL);

	bool doneGroup = false;
	bool doneFriends = false;
	bool doneServers = true;
	bool doneProfiles = true;

	NPID npID;
	NP_GetNPID(&npID);

	printf("NPID: %llu\n", npID);
	printf("GroupID: %d\n", NP_GetUserGroup());

	int runtime = 0;
	while (true)
	{
		Sleep(1);
		runtime++;
		NP_RunFrame();

		NP_SendRandomString("test");

		/*
		if (runtime % 1000 == 0)
		{

			static uint8_t worldFile[131072];

			NPAsync<NPGetPublisherFileResult>* fileResult = NP_GetPublisherFile("hello_world.txt", worldFile, sizeof(worldFile));
			fileResult->SetCallback(FileResultCB, NULL);
		}
		*/

		/*if (!doneServers)
		{
			NPSessionInfo info;
			strcpy(info.hostname, "hello world");
			strcpy(info.mapname, "my map");
			info.maxplayers = 18;
			info.players = 2;
			info.version = 1;

			NPAsync<NPCreateSessionResult>* sessionResult = NP_CreateSession(&info);
			sessionResult->SetCallback(SessionCB, NULL);

			doneServers = true;
			doneProfiles = false;
		}*/

		//Get friends
		if (runtime % 1000 == 0)
		{
			printf("Requesting friends...\n");
			NPAsync<NPFriendResult>* profileResult = NP_GetFriends();
			profileResult->SetCallback(receivedFriends, NULL);

		}

	}

	NP_Shutdown();
	return 0;
}

