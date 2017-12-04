// ==========================================================
// alterIWnet project
// 
// Component: xnp
// Sub-component: libnp
// Purpose: friends methods
//
// Initial author: NTAuthority
// Started: 2011-11-10
// ==========================================================

#include "StdInc.h"
#include <tomcrypt.h>
#include <strophe.h>

#include <google/dense_hash_map>

#define MAX_FRIENDS 1024

static struct  
{
	// connected?
	bool connected;

	// roster
	google::dense_hash_map<NPID, NPFriend> registeredFriends;//Holds all the friend data
	NPID roster[MAX_FRIENDS];//Holds the npid is (index of registeredFriends)
	int rosterIndex;//size of roster

	// local rich presence state
	std::map<std::string, std::string> richPresenceProps;
	char richPresenceBody[256];
} g_friends;


// Sorts the friend roster
static int Friends_SortRosterCompare(const void* left, const void* right)
{
	NPID leftID = *(NPID*)left;
	NPID rightID = *(NPID*)right;

	int presenceLeft = g_friends.registeredFriends[leftID].isonline;
	int presenceRight = g_friends.registeredFriends[rightID].isonline;

	if (presenceLeft == presenceRight)
	{
		return _stricmp(g_friends.registeredFriends[leftID].username, g_friends.registeredFriends[rightID].username);
	}

	return (presenceRight - presenceLeft);
}

//Get a friends roster
/*
void Friends_ReceivedRoster(INPRPCMessage* message)
{
	RPCFriendsRosterMessage* result = (RPCFriendsRosterMessage*)message;

	for (int i = 0; i < result->GetBuffer()->friends_size(); i++)
	{
		FriendsRosterMessage_FriendDetails details = result->GetBuffer()->friends(i);

		if (details.game() != "IW4M")
			continue;

		NPFriend aFriend;
		aFriend.id = details.id();
		aFriend.username = details.username();
		aFriend.current_server = details.current_server().c_str();
		aFriend.isonline = details.isonline();
		
		//Save
		g_friends.registeredFriends[aFriend.id] = aFriend;

		//Keep track of the order of friends
		g_friends.roster[g_friends.rosterIndex] = aFriend.id;
		g_friends.rosterIndex++;
	}

	//Sort roster
	qsort(g_friends.roster, g_friends.rosterIndex, sizeof(int), Friends_SortRosterCompare);

	g_friends.connected = true;
}
*/

void Friends_Init()
{
	Log_Print("Friends_Init()");
	// initialize the hash map
	g_friends.registeredFriends.set_empty_key(0);

	// initial state
	g_friends.connected = false;

	// register dispatcherss
	//RPC_RegisterDispatch(RPCFriendsRosterMessage::Type, &Friends_ReceivedRoster);
	//RPC_RegisterDispatch(RPCFriendsPresenceMessage::Type, &Friends_ReceivedPresence);
}

void Friends_Shutdown()
{

}

void Friends_Connect()
{
	
}

// buffer details
struct np_get_user_avatar_state_s
{
	NPAsyncImpl<NPGetUserAvatarResult>* asyncResult;
	uint8_t* buffer;
	size_t bufferLength;
};

static void GetUserAvatarCB(NPAsync<INPRPCMessage>* async)
{
	np_get_user_avatar_state_s* state = (np_get_user_avatar_state_s*)async->GetUserData();
	RPCFriendsGetUserAvatarResultMessage* rpcResult = (RPCFriendsGetUserAvatarResultMessage*)async->GetResult();

	// print log
	Log_Print("GetUserAvatarCB for client %d\n", rpcResult->GetBuffer()->guid());

	// create and fill out result object
	NPGetUserAvatarResult* result = new NPGetUserAvatarResult();
	result->result = (EGetFileResult)rpcResult->GetBuffer()->result();
	result->buffer = state->buffer;
	result->fileSize = 0;
	result->guid = rpcResult->GetBuffer()->guid();

	// set the buffer to 0
	memset(state->buffer, 0, state->bufferLength);

	// copy file data to the buffer
	if (result->result == GetFileResultOK)
	{
		result->fileSize = rpcResult->GetBuffer()->filedata().length();

		size_t toCopy = (result->fileSize > state->bufferLength) ? state->bufferLength : result->fileSize;
		memcpy(result->buffer, rpcResult->GetBuffer()->filedata().c_str(), toCopy);
		result->fileSize = toCopy;
	}

	// and handle the async result
	state->asyncResult->SetResult(result);
	delete state;
}

// ----------------
// friends API
// ----------------

LIBNP_API bool LIBNP_CALL NP_FriendsConnected()
{
	return g_friends.connected;
}

LIBNP_API int LIBNP_CALL NP_GetNumFriends()
{
	return g_friends.rosterIndex;
}

struct np_get_profile_data_state_s
{
	NPAsyncImpl<NPFriendResult>* asyncResult;
	NPFriend* buffer;
};

/*
This was the code that wasn't requesting anything to the np. The np would send it when required.
LIBNP_API NPAsync<NPFriendResult>* LIBNP_CALL NP_GetFriends(NPFriend* outData)
{
	NPAsyncImpl<NPFriendResult>* result = new NPAsyncImpl<NPFriendResult>();

	np_get_profile_data_state_s* state = new np_get_profile_data_state_s();
	state->numIDs = g_friends.rosterIndex;
	state->buffer = outData;
	state->asyncResult = result;

	// make a result object
	NPFriendResult* result2 = new NPFriendResult();
	result2->numResults = g_friends.rosterIndex;
	result2->results = state->buffer;

	// add the results to the buffer
	for (uint32_t i = 0; i < g_friends.rosterIndex; i++)
	{
		memcpy(&state->buffer[i], &g_friends.registeredFriends[g_friends.roster[i]], sizeof(g_friends.registeredFriends[g_friends.roster[i]]));
	}

	// and handle the async result
	state->asyncResult->SetResult(result2);
	delete state;

	return result;
}
*/

static void GetFriendsRoserCB(NPAsync<INPRPCMessage>* async)
{
	//Clear arrays
	g_friends.registeredFriends.clear();
	for (int x = 0; x < MAX_FRIENDS; ++x)
	{
		g_friends.roster[x] = 0;
	}
	g_friends.rosterIndex = 0;

	np_get_profile_data_state_s* state = (np_get_profile_data_state_s*)async->GetUserData();
	RPCFriendsRosterMessage* rpcResult = (RPCFriendsRosterMessage*)async->GetResult();
	printf("Friends size: %i", rpcResult->GetBuffer()->friends_size());

	// Go through all the friends
	for (int i = 0; i < rpcResult->GetBuffer()->friends_size(); i++)
	{
		FriendsRosterMessage_FriendDetails details = rpcResult->GetBuffer()->friends(i);//FriendsRosterMessage_FriendDetails

		//if (details.game() != "IW4M")
		//	continue;

		std::string username = details.username();
		std::string hostname = details.hostname();
		std::string game = details.game();
		std::string current_server = details.current_server();

		NPFriend aFriend;
		aFriend.id = details.id();
		strncpy(aFriend.username, username.c_str(), sizeof(aFriend.username));
		strncpy(aFriend.hostname, hostname.c_str(), sizeof(aFriend.hostname));
		strncpy(aFriend.game, game.c_str(), sizeof(aFriend.game));
		strncpy(aFriend.current_server, current_server.c_str(), sizeof(aFriend.current_server));
		aFriend.isonline = details.isonline();

		//Save
		g_friends.registeredFriends[aFriend.id] = aFriend;
		
		//Keep track of the order of friends
		g_friends.roster[g_friends.rosterIndex] = aFriend.id;
		g_friends.rosterIndex++;
	}

	//Sort friend roster
	qsort(g_friends.roster, g_friends.rosterIndex, sizeof(NPID), Friends_SortRosterCompare);
	g_friends.connected = true;

	// add the sorted result to the buffer we are returning
	for (int i = 0; i < g_friends.rosterIndex; i++)
	{
		memcpy(&state->buffer[i], &g_friends.registeredFriends[g_friends.roster[i]], sizeof(g_friends.registeredFriends[g_friends.roster[i]]));
	}

	// make a result object
	NPFriendResult* result = new NPFriendResult();
	result->result = (FriendsRosterMessageResult)rpcResult->GetBuffer()->result();
	result->numResults = g_friends.rosterIndex;
	result->friends = state->buffer;

	// and handle the async result
	state->asyncResult->SetResult(result);
	delete state;
}

LIBNP_API NPAsync<NPFriendResult>* LIBNP_CALL NP_GetFriends()
{
	static NPFriend profileData[MAX_FRIENDS];
	RPCFriendsRequestRoster* request = new RPCFriendsRequestRoster();

	NPAsync<INPRPCMessage>* async = RPC_SendMessageAsync(request);
	NPAsyncImpl<NPFriendResult>* result = new NPAsyncImpl<NPFriendResult>();

	np_get_profile_data_state_s* state = new np_get_profile_data_state_s();
	state->buffer = profileData;
	state->asyncResult = result;

	async->SetCallback(GetFriendsRoserCB, state);

	request->Free();

	return result;
}

//Used by steam (T5 client)
LIBNP_API const char* LIBNP_CALL NP_GetFriendName(NPID npID)
{
	if (!g_friends.connected)
	{
		static NPFriend profileData[MAX_FRIENDS];
		RPCFriendsRequestRoster* request = new RPCFriendsRequestRoster();

		NPAsync<INPRPCMessage>* async = RPC_SendMessageAsync(request);
		NPAsyncImpl<NPFriendResult>* result = new NPAsyncImpl<NPFriendResult>();

		np_get_profile_data_state_s* state = new np_get_profile_data_state_s();
		state->buffer = profileData;
		state->asyncResult = result;

		async->SetCallback(GetFriendsRoserCB, state);
		async->Wait();
	}

	if (g_friends.registeredFriends[npID].id == 0)
	{
		return NULL;
	}

	return g_friends.registeredFriends[npID].username;
}

LIBNP_API EPresenceState LIBNP_CALL NP_GetFriendPresence(NPID npID)
{
	if (!g_friends.connected)
	{
		static NPFriend profileData[MAX_FRIENDS];
		RPCFriendsRequestRoster* request = new RPCFriendsRequestRoster();

		NPAsync<INPRPCMessage>* async = RPC_SendMessageAsync(request);
		NPAsyncImpl<NPFriendResult>* result = new NPAsyncImpl<NPFriendResult>();

		np_get_profile_data_state_s* state = new np_get_profile_data_state_s();
		state->buffer = profileData;
		state->asyncResult = result;

		async->SetCallback(GetFriendsRoserCB, state);
		async->Wait();
	}
	
	if (g_friends.registeredFriends[npID].id == 0)
	{
		return PresenceStateOffline;
	}

	if (g_friends.registeredFriends[npID].isonline == 1)
	{
		return PresenceStateOnline;
	}

	return PresenceStateOffline;
}

LIBNP_API NPID LIBNP_CALL NP_GetFriend(NPID index)
{
	if (!g_friends.connected)
	{
		static NPFriend profileData[MAX_FRIENDS];
		RPCFriendsRequestRoster* request = new RPCFriendsRequestRoster();

		NPAsync<INPRPCMessage>* async = RPC_SendMessageAsync(request);
		NPAsyncImpl<NPFriendResult>* result = new NPAsyncImpl<NPFriendResult>();

		np_get_profile_data_state_s* state = new np_get_profile_data_state_s();
		state->buffer = profileData;
		state->asyncResult = result;

		async->SetCallback(GetFriendsRoserCB, state);
		async->Wait();
	}

	return g_friends.roster[index];
}

//end of steam api patch

LIBNP_API void LIBNP_CALL NP_SetRichPresence(const char* key, const char* value)
{
	if (value)
	{
		g_friends.richPresenceProps[key] = value;
	}
	else
	{
		g_friends.richPresenceProps.erase(key);
	}
}

//Update status, can be hostname or game whatever
LIBNP_API void LIBNP_CALL NP_SetRichPresenceBody(const char* body)
{
	if (body)
	{
		strncpy(g_friends.richPresenceBody, body, 255);
	}
	else
	{
		g_friends.richPresenceBody[0] = '\0';
	}
}

//Get friends status
LIBNP_API const char* LIBNP_CALL NP_GetFriendRichPresence(NPID npID, const char* key)
{
	if (!g_friends.connected)
	{
		static NPFriend profileData[MAX_FRIENDS];
		RPCFriendsRequestRoster* request = new RPCFriendsRequestRoster();

		NPAsync<INPRPCMessage>* async = RPC_SendMessageAsync(request);
		NPAsyncImpl<NPFriendResult>* result = new NPAsyncImpl<NPFriendResult>();

		np_get_profile_data_state_s* state = new np_get_profile_data_state_s();
		state->buffer = profileData;
		state->asyncResult = result;

		async->SetCallback(GetFriendsRoserCB, state);
		async->Wait();
	}

	if (g_friends.registeredFriends[npID].id == 0)
	{
		Log_Print("Requested friend status of someone who isn't our friend... Ignoring\n");
		return NULL;
	}

	if (strcmp(key,"current_server")) {
		return g_friends.registeredFriends[npID].current_server;
	}

	if (key == "hostname") {
		return g_friends.registeredFriends[npID].hostname;
	}

	if (key == "username") {
		return g_friends.registeredFriends[npID].username;
	}

	if (strcmp(key, "game")) {
		return g_friends.registeredFriends[npID].game;
	}

	Log_Print("Unkown friend status request %s\n", key);
	return NULL;
}

LIBNP_API void LIBNP_CALL NP_StoreRichPresence()
{
	RPCFriendsSetPresenceMessage* request = new RPCFriendsSetPresenceMessage();

	for (std::map<std::string, std::string>::iterator iter = g_friends.richPresenceProps.begin(); iter != g_friends.richPresenceProps.end(); iter++)
	{
		FriendsPresence* item = request->GetBuffer()->add_presence();

		item->set_key(iter->first);
		item->set_value(iter->second);
	}

	FriendsPresence* bodyItem = request->GetBuffer()->add_presence();

	bodyItem->set_key("__body");
	bodyItem->set_value(g_friends.richPresenceBody);

	RPC_SendMessage(request);

	request->Free();
}

LIBNP_API NPAsync<NPGetUserAvatarResult>* LIBNP_CALL NP_GetUserAvatar(int id, uint8_t* buffer, size_t bufferLength)
{
	RPCFriendsGetUserAvatarMessage* request = new RPCFriendsGetUserAvatarMessage();
	request->GetBuffer()->set_guid(id);

	NPAsync<INPRPCMessage>* async = RPC_SendMessageAsync(request);
	NPAsyncImpl<NPGetUserAvatarResult>* result = new NPAsyncImpl<NPGetUserAvatarResult>();

	np_get_user_avatar_state_s* state = new np_get_user_avatar_state_s();
	state->asyncResult = result;
	state->buffer = buffer;
	state->bufferLength = bufferLength;

	async->SetCallback(GetUserAvatarCB, state);

	request->Free();

	return result;
}