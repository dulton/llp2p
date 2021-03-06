﻿/*
#include "JSObject.h"
#include "variant_list.h"
#include "DOM/Document.h"
#include "DOM/Window.h"
#include "global/config.h"
#include "llp2pFBAPI.h"
// IRC client
//#include "irc/libircclient.h"
//#include "irc/keyboard.h"
*/


#include "common.h"
#ifdef _WIN32
#include "EpollFake.h"
#endif
#include "configuration.h"
#include "logger.h"
#include "network.h"
#include "network_udp.h"
#include "basic_class.h" 
#include "pk_mgr.h"
#include "peer_mgr.h"
#include "peer.h"
//#include "librtmp/rtmp_server.h"
//#include "librtmp/rtmp.h"
//#include "librtmp/rtmp_supplement.h"
#include "bit_stream_server.h"
#include "peer_communication.h"
#include "io_connect.h"
#include "io_accept.h"
#include "io_accept_udp.h"
#include "logger_client.h"
#ifdef STUNT_FUNC
#include "stunt_mgr.h"
#endif
#include "register_mgr.h"
#include "udt_lib/udt.h"

//#include "irc/irc_client.h"

#include "stund/stun.h"
#include "stund/udp.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;
using namespace UDT;

//#pragma comment(lib,"ws2_32.lib")
//#pragma comment(lib,"my_udt.lib")
//#pragma comment(lib,"../udt.lib")


const char version[] = "1.0.0.0";


#ifdef _FIRE_BREATH_MOD_

typedef struct {
	volatile sig_atomic_t handle_sig_alarm;
	volatile sig_atomic_t srv_shutdown;
	int errorRestartFlag;
	//static logger_client *logger_client_ptr = NULL;
	volatile unsigned short streamingPort;
	list<int> *fd_list;
	list<int> *udp_fd_list;
	map<string, string> *map_config;
	pk_mgr *pk_mgr_ptr_copy;
	volatile sig_atomic_t is_Pk_mgr_ptr_copy_delete;
	volatile sig_atomic_t http_srv_ready;
	volatile sig_atomic_t thread_num;    // 0 can open thread  ,1 cant open
	volatile sig_atomic_t is_init;		// 0: This object is not initialized, 1: This object is initialized and can be launched as threads
	unsigned char exit_code;		// Peer exit error code
	FB::DOM::WindowPtr window;		// A reference to the DOM Window
	/*  irc related
	irc_cli_thread irc_arg;
	irc_session_t *session;
	*/
	HANDLE hThread;
	unsigned int threadID;
} GlobalVars;

//map<string, string> map_config;
map<int, GlobalVars*> map_channelID_globalVar;
void launchThread(void * arg);
void launch_irc_cli_thread(void *arg);
int mainFunction(int chid);
int connect_irc(int thread_key);
//string ssss;
int set_config_done = 0;

#else

//struct globalVar{
static volatile sig_atomic_t handle_sig_alarm = 0;
static volatile sig_atomic_t srv_shutdown = 0;
static int errorRestartFlag = 0;
//static logger_client *logger_client_ptr = NULL;
static volatile unsigned short streamingPort = 0;
list<int> fd_list;			// Store tcp socket fd
list<int> udp_fd_list;		// Store udp socket fd

#endif
//};


#ifdef _FIRE_BREATH_MOD_
#else

void signal_handler(int sig)
{
	fprintf(stdout, "\n\nrecv Signal %d\n\n", sig);

	//if(logger_client_ptr == NULL){
	//	logger_client_ptr = new logger_client();
	//	logger_client_ptr->log_init();
	//}

	switch (sig)
	{
	case SIGTERM:
		srv_shutdown = 1;
		//logger_client_ptr->log_exit();
		break;
	case SIGINT:
		//srv_shutdown = 1;
#ifdef _WIN32
		//system("PAUSE");
		//exit(1);
#else
		exit(1);
#endif
		errorRestartFlag = 1;
		//logger_client_ptr->log_exit();
		break;
#ifndef _WIN32
	case SIGALRM:
		handle_sig_alarm = 1;
		//logger_client_ptr->log_exit();
		break;
#endif
	case SIGSEGV:
		srv_shutdown = 1;
		//logger_client_ptr->log_exit();
#ifdef _WIN32
		MessageBox(NULL, _T("SIGSEGV"), _T("EXIT"), MB_OK | MB_ICONSTOP);
#else
		printf("SIGSEGV\n");
		PAUSE
			exit(0);		// it must exit on linux			
#endif
		break;
	case SIGABRT:
		srv_shutdown = 1;
		//logger_client_ptr->log_exit();
#ifdef _WIN32
		//MessageBox(NULL, _T("SIGABRT"), _T("EXIT"), MB_OK | MB_ICONSTOP);
#else
		printf("SIGABRT\n");
		PAUSE
			exit(0);		// it must exit on linux
#endif			
		break;
	}

}

#endif


#ifdef _FIRE_BREATH_MOD_
///////////////////////////////////////////////////////////////////////////////
/// @fn FB::variant llp2pAPI::echo(const FB::variant& msg)
///
/// @brief  Echos whatever is passed from Javascript.
///         Go ahead and change it. See what happens!
///////////////////////////////////////////////////////////////////////////////
FB::variant llp2pFBAPI::echo(const FB::variant& msg)
{
	static int n(0);
	fire_echo("So far, you clicked this many times: ", n++);
	fire_echo("tess ", 3051);

	return "Debug_mode";
}

///////////////////////////////////////////////////////////////////////////////
/// @fn llp2pPtr llp2pAPI::getPlugin()
///
/// @brief  Gets a reference to the plugin that was passed in when the object
///         was created.  If the plugin has already been released then this
///         will throw a FB::script_error that will be translated into a
///         javascript exception in the page.
///////////////////////////////////////////////////////////////////////////////
llp2pFBPtr llp2pFBAPI::getPlugin()
{
	llp2pFBPtr plugin(m_plugin.lock());
	if (!plugin) {
		throw FB::script_error("The plugin is invalid");
	}
	return plugin;
}

// Read/Write property testString
std::string llp2pFBAPI::get_testString()
{
	return m_testString;
}

void llp2pFBAPI::set_testString(const std::string& val)
{
	m_testString = val;
}

// Read-only property version
std::string llp2pFBAPI::get_version()
{
	return FBSTRING_PLUGIN_VERSION;
}

/*
// This API tell web the error code
std::string firebreathTestAPI::get_error_code(int chid)
{

if (map_channelID_globalVar.find(chid) == map_channelID_globalVar.end()) {
return "Input parameter error";
}

switch (map_channelID_globalVar[chid]->exit_code) {
case PEER_ALIVE :		return "Peer is alive";	break;
case CLOSE_CHANNEL :	return "Channel is closed";	break;
case CLOSE_STREAM :		return "Stream is closed";	break;
case BUFFER_OVERFLOW :	return "Server buffer overflow";	break;
case RECV_NODATA :		return "Receive no source from server";	break;
case MALLOC_ERROR :		return "Memory allocation error";	break;
case MACCESS_ERROR :	return "Memory access error";	break;
case PK_SOCKET_ERROR :	return "socket error from server";	break;
case LOG_SOCKET_ERROR :	return "socket error from log server";	break;
case UNKNOWN :			return "Unknown error";	break;
default :				return "(default)Unexpected error";	break;
}

}
*/
// This function return a unique number for javascript as a key to launch a thread
int llp2pFBAPI::get_thread_key()
{
	static int thread_key(0);
	thread_key++;
	return thread_key;
}

void llp2pFBAPI::testEvent()
{
	// Retrieve a reference to the DOM Window
	FB::DOM::WindowPtr window = m_host->getDOMWindow();

	// Check if the DOM Window has an alert peroperty
	if (window && window->getJSObject()->HasProperty("window")) {
		// Create a reference to alert
		FB::JSObjectPtr obj = window->getProperty<FB::JSObjectPtr>("window");

		// Invoke alert with some text
		//obj->Invoke("alert", FB::variant_list_of("This is a test alert invoked from an NPAPI Plugin"));
		obj->Invoke("testCallByPlugin", FB::variant_list_of("......"));
	}
}


// This function is called by javascript so that an object is created before initialization
int llp2pFBAPI::create_obj(int thread_key)
{
	// Check whether the object of this channel is in the plugin or not

	if (map_channelID_globalVar.count(thread_key) == 0) {
		GlobalVars *temp = new GlobalVars;
		temp->handle_sig_alarm = 0;
		temp->srv_shutdown = 0;
		temp->errorRestartFlag = 0;
		temp->streamingPort = -1;
		temp->fd_list = new list<int>;
		temp->udp_fd_list = new list<int>;
		temp->map_config = NULL;
		temp->pk_mgr_ptr_copy = NULL;
		temp->is_Pk_mgr_ptr_copy_delete = TRUE;
		temp->http_srv_ready = 0;
		temp->thread_num = 0;
		temp->exit_code = PEER_ALIVE;
		temp->window = m_host->getDOMWindow();
		//  irc related
		//memset(temp->irc_arg.channel, 0, sizeof(temp->irc_arg.channel));
		//memcpy(temp->irc_arg.channel, "#p2ptv_game", 11);
		//memset(temp->irc_arg.ip, 0, sizeof(temp->irc_arg.ip));
		//memcpy(temp->irc_arg.ip, "140.114.71.174", 14);
		//memset(temp->irc_arg.nick, 0, sizeof(temp->irc_arg.nick));
		//memcpy(temp->irc_arg.nick, "USER1", 5);
		//

		map_channelID_globalVar[thread_key] = temp;

		return 0;
	}
	else {
		FB::DOM::WindowPtr window = m_host->getDOMWindow();
		if (window && window->getJSObject()->HasProperty("window")) {
			// Invoke certain function of javascript
			FB::JSObjectPtr obj = window->getProperty<FB::JSObjectPtr>("window");
			obj->Invoke("errCode_from_plugin", FB::variant_list_of(-1));
		}
		return -11;
	}

}

int llp2pFBAPI::start(int thread_key)
{
	map<int, GlobalVars*>::iterator iter = map_channelID_globalVar.find(thread_key);
	if (iter == map_channelID_globalVar.end()) {
		return -1;
	}
	if (iter->second->is_init == 0) {
		return -2;
	}

	iter->second->thread_num++;
	_beginthread(launchThread, 0, (void *)thread_key);

	return 0;
}

void llp2pFBAPI::stop(int thread_key)
{
	map_channelID_globalVar[thread_key]->thread_num--;
	if (map_channelID_globalVar[thread_key]->thread_num <= 0)
	{
		map_channelID_globalVar[thread_key]->srv_shutdown = 1;
	}

}

int llp2pFBAPI::isReady(int channel_id)
{
	map<int, GlobalVars*>::iterator iter;
	iter = map_channelID_globalVar.find(channel_id);
	if (iter != map_channelID_globalVar.end()) {
		return map_channelID_globalVar[channel_id]->http_srv_ready;
	}
	else {
		return -1;
	}
}

int llp2pFBAPI::isStreamInChannel(int stream_id, int channel_id)
{
	if (map_channelID_globalVar[channel_id]->is_Pk_mgr_ptr_copy_delete == FALSE){
		map<int, struct update_stream_header *>::iterator  map_streamID_header_iter;
		map_streamID_header_iter = map_channelID_globalVar[channel_id]->pk_mgr_ptr_copy->map_streamID_header.find(stream_id);
		//		return TRUE;
		if (map_streamID_header_iter != map_channelID_globalVar[channel_id]->pk_mgr_ptr_copy->map_streamID_header.end()){
			return TRUE;
		}
		else{
			return FALSE;
		}
	}
	else{
		return FALSE;
	}

	return TRUE;
}

unsigned short llp2pFBAPI::streamingPortIs(int channel_id)
{
	return map_channelID_globalVar[channel_id]->streamingPort;
}

// Set up configuration and store in map_config
int llp2pFBAPI::set_config(int thread_key, const std::string& msg)
{
	std::string input = msg;
	Json::Reader reader;
	Json::Value value;
	if (reader.parse(input, value)) {

		map<int, GlobalVars*>::iterator iter = map_channelID_globalVar.find(thread_key);
		if (iter == map_channelID_globalVar.end()) {
			return -1;
		}
		if (iter->second->map_config != NULL) {
			return -1;
		}

		iter->second->map_config = new map<string, string>;

		iter->second->map_config->insert(pair<string, string>("bucket_size", value["BUCKET_SIZE"].asString()));
		iter->second->map_config->insert(pair<string, string>("channel_id", value["CHANNEL_ID"].asString()));
		iter->second->map_config->insert(pair<string, string>("html_size", value["HTML_SIZE"].asString()));
		iter->second->map_config->insert(pair<string, string>("lane_depth", value["LANE_DEPTH"].asString()));
		iter->second->map_config->insert(pair<string, string>("max_lane", value["MAX_LANE"].asString()));
		iter->second->map_config->insert(pair<string, string>("min_lane", value["MIN_LANE"].asString()));
		iter->second->map_config->insert(pair<string, string>("pk_ip", value["PK_SERVER"]["IP"].asString()));
		iter->second->map_config->insert(pair<string, string>("pk_port", value["PK_SERVER"]["PORT"].asString()));
		iter->second->map_config->insert(pair<string, string>("reg_ip", value["REG_SERVER"]["IP"].asString()));
		iter->second->map_config->insert(pair<string, string>("reg_port", value["REG_SERVER"]["PORT"].asString()));
		iter->second->map_config->insert(pair<string, string>("log_ip", value["LOG_SERVER"]["IP"].asString()));
		iter->second->map_config->insert(pair<string, string>("log_port", value["LOG_SERVER"]["PORT"].asString()));
		iter->second->map_config->insert(pair<string, string>("stun_ip", value["STUN_SERVER"]["IP"].asString()));
		iter->second->map_config->insert(pair<string, string>("stream_local_port", value["STREAM"]["PORT"].asString()));
		iter->second->map_config->insert(pair<string, string>("svc_tcp_port", value["P2P_TCP_PORT"].asString()));
		iter->second->map_config->insert(pair<string, string>("svc_tcp_port", value["P2P_UDP_PORT"].asString()));

		iter->second->is_init = 1;
	}
	/*
	map<int, GlobalVars*>::iterator iter = map_channelID_globalVar.find(thread_key);
	if (iter == map_channelID_globalVar.end()) {
	return -1;
	}
	if (iter->second->map_config != NULL) {
	return -1;
	}

	iter->second->map_config = new map<string, string>;

	string ss(msg.begin() + 1, msg.end() - 1);
	std::string delimiter = ",";

	size_t pos = 0;
	string token;
	map<string, string> m;
	while ((pos = ss.find(delimiter)) != std::string::npos) {
	token = ss.substr(0, pos);

	int n = token.find(":");
	string key(token.begin() + 1, token.begin() + n - 1);
	string value(token.begin() + n + 2, token.end() - 1);
	iter->second->map_config->insert(pair<string, string>(key, value));
	//map_config.insert(pair<string, string>(key, value));
	//ssss += key + ":" + value + "\n";
	ss.erase(0, pos + delimiter.length());
	}
	token = ss.substr(0, pos);

	int n = token.find(":");
	string key(token.begin() + 1, token.begin() + n - 1);
	string value(token.begin() + n + 2, token.end() - 1);
	iter->second->map_config->insert(pair<string, string>(key, value));
	//map_config.insert(pair<string, string>(key, value));
	//ssss += key + ":" + value + "\n";

	iter->second->is_init = 1;

	//set_config_done = 1;
	*/
	return 0;
}

void llp2pFBAPI::launch_irc(int thread_key, const std::string& nickname)
{
#ifdef IRC_CLIENT
	map<int, GlobalVars*>::iterator iter = map_channelID_globalVar.find(thread_key);
	if (iter == map_channelID_globalVar.end()) {
		return;
	}
	if (iter->second->is_init == 0) {
		return;
	}


	memset(iter->second->irc_arg.nick, 0, sizeof(iter->second->irc_arg.nick));
	memcpy(iter->second->irc_arg.nick, nickname.c_str(), nickname.length());
	iter->second->hThread = (HANDLE)_beginthread(launch_irc_cli_thread, 0, (void *)thread_key);
#endif
}

void llp2pFBAPI::dislaunch_irc(int thread_key)
{
#ifdef IRC_CLIENT
	map<int, GlobalVars*>::iterator iter = map_channelID_globalVar.find(thread_key);
	if (iter == map_channelID_globalVar.end()) {
		return;
	}
	if (iter->second->is_init == 0) {
		return;
	}

	irc_disconnect(iter->second->session);
#endif
}

std::string llp2pFBAPI::sendtoirc(int thread_key, const std::string& msg)
{
#ifdef IRC_CLIENT
	map<int, GlobalVars*>::iterator iter = map_channelID_globalVar.find(thread_key);
	if (iter == map_channelID_globalVar.end()) {
		return "FAIL";
	}

	irc_cmd_msg(iter->second->session, iter->second->irc_arg.channel, msg.c_str());
	//irc_cmd_msg (iter->second->session, iter->second->irc_arg.channel, "654987");
#endif
	return msg;

}

std::string llp2pFBAPI::get_config(int channel_id)
{
	map<int, GlobalVars*>::iterator iter = map_channelID_globalVar.find(channel_id);
	if (iter == map_channelID_globalVar.end()) {
		return "The object is not created";
	}
	if (iter->second->is_init == 0) {
		return "Initialization of the object is not finished";
	}

	string ret_msg("");
	for (map<string, string>::iterator iter_temp = iter->second->map_config->begin(); iter_temp != iter->second->map_config->end(); iter_temp++) {
		ret_msg += iter_temp->first + ":" + iter_temp->second + "\n";
	}
	return ret_msg;
}



int llp2pFBAPI::is_set_config_done()
{
	return set_config_done;
}

void launchThread(void * arg)
{
	int a = (int)arg;
	mainFunction(a);
}

void launch_irc_cli_thread(void *arg)
{
	int a = (int)arg;
	connect_irc(a);
}

#endif


void send_error_to_web(int thread_key, const std::string& msg)
{
#ifdef _FIRE_BREATH_MOD_
	if (map_channelID_globalVar[thread_key]->window && map_channelID_globalVar[thread_key]->window->getJSObject()->HasProperty("window")) {
		// Invoke certain function of javascript
		FB::JSObjectPtr obj = map_channelID_globalVar[thread_key]->window->getProperty<FB::JSObjectPtr>("window");
		obj->Invoke("get_error_code", FB::variant_list_of(msg));
	}
#endif
}



// Once restart flag is set, leave while loop. 
// This function handle whether restart or shut-down, and handle web-side
void handle_restart(int thread_key, int pk_exit_code, int log_exit_code, int *shutdown)
{
	if (pk_exit_code == CLOSE_CHANNEL) {
		debug_printf("Channel is closed by pk %d \n", pk_exit_code, log_exit_code);
		send_error_to_web(thread_key, "Channel is closed");
		*shutdown = 1;
	}
	else if (pk_exit_code == CLOSE_STREAM) {
		debug_printf("Stream is closed by pk %d \n", pk_exit_code, log_exit_code);
		send_error_to_web(thread_key, "Channel is closed");
		*shutdown = 1;
	}
	else if (pk_exit_code == BUFFER_OVERFLOW) {
		debug_printf("pk's Buffer overflow %d %d \n", pk_exit_code, log_exit_code);
		send_error_to_web(thread_key, "Channel is closed");
		//PAUSE
	}
	else if (pk_exit_code == CHANGE_PK) {
		debug_printf("change pk %d %d \n", pk_exit_code, log_exit_code);
		send_error_to_web(thread_key, "change pk");
		//PAUSE
	}
	else if (pk_exit_code == PK_BUILD_ERROR) {
		debug_printf("Fail to build connection with server(pk/log) %d %d \n", pk_exit_code, log_exit_code);
		send_error_to_web(thread_key, "Channel is closed");
		*shutdown = 1;
	}
	else if (pk_exit_code == PK_SOCKET_ERROR) {
		debug_printf("pk socket error %d %d \n", pk_exit_code, log_exit_code);
		send_error_to_web(thread_key, "Channel is closed");
		*shutdown = 1;
	}
	else if (pk_exit_code == PK_SOCKET_CLOSED) {
		debug_printf("Socket is closed by pk %d %d \n", pk_exit_code, log_exit_code);
		send_error_to_web(thread_key, "Channel is closed");
		*shutdown = 1;
	}
	else {
		debug_printf("pk_exit_code Unknown %d %d \n", pk_exit_code, log_exit_code);
		//*shutdown = 1;
		//PAUSE
	}

	if (log_exit_code == LOG_BUILD_ERROR) {
		debug_printf("Fail to build connection with log-server %d %d \n", pk_exit_code, log_exit_code);
		send_error_to_web(thread_key, "Channel is closed");
		*shutdown = 1;
	}
	else if (log_exit_code == LOG_SOCKET_ERROR) {
		debug_printf("server socket error %d %d \n", pk_exit_code, log_exit_code);
		//PAUSE
	}
	else if (log_exit_code == LOG_SOCKET_CLOSED) {
		debug_printf("Socket is closed by log-server %d %d \n", pk_exit_code, log_exit_code);
		//PAUSE
	}
	else {
		debug_printf("log_exit_code Unknown %d %d \n", pk_exit_code, log_exit_code);
		//*shutdown = 1;
		//PAUSE
	}
}

#ifdef _FIRE_BREATH_MOD_
int mainFunction(int thread_key){
#else
int main(int argc, char **argv){
#endif
	
	//cout << UDT::stunRandomPort() << endl;


	cout << "tst_speed_svr " << version << " (Compiled Time: "__DATE__ << " "__TIME__")" << endl << endl;

	FILE *record_file_fp2 = NULL;
#ifdef RECORD_FILE
	record_file_fp2 = fopen("file", "wb");
#endif
#ifdef _FIRE_BREATH_MOD_
	while (!map_channelID_globalVar[thread_key]->srv_shutdown) {
#else
	while (!srv_shutdown) {
#endif



		int svc_fd_tcp;		// listening-socket for peers 
		int	svc_fd_udp;
		unsigned short tcp_port;		// TCP port of listening port
		unsigned short udp_port_ext;	// external UDP port of listening port
		unsigned short udp_port_int;	// internal UDP port of listening port
		unsigned long html_size;
		int optval = 1;
		struct sockaddr_in sin;
		unsigned char pk_exit_code = PEER_ALIVE;
		unsigned char log_exit_code = PEER_ALIVE;

		string svc_tcp_port("");
		string svc_udp_port("");
		string stream_local_port("");
		string config_file("config.ini");


		configuration *prep = NULL;
		network *net_ptr = NULL;
		network_udp *net_udp_ptr = NULL;
		logger *log_ptr = NULL;					// record log on local file
		logger_client *logger_client_ptr = NULL;	// send log Info to server
		peer_mgr *peer_mgr_ptr = NULL;
		stunt_mgr *stunt_mgr_ptr = NULL;
		pk_mgr *pk_mgr_ptr = NULL;
		register_mgr *register_mgr_ptr = NULL;
		//rtmp_server *rtmp_svr = NULL;
		//rtmp *rtmp_ptr = NULL;
		//amf *amf_ptr = NULL; 
		//rtmp_supplement *rtmp_supplement_ptr = NULL;
		bit_stream_server *bit_stream_server_ptr = NULL;
		peer_communication *peer_communication_ptr = NULL;
		io_accept_udp *io_accept_udp_ptr = NULL;
		// Create constructors
		//prep = new configuration(config_file);



#ifdef _FIRE_BREATH_MOD_
		prep = new configuration(config_file);
		for (map<string, string>::iterator iter = map_channelID_globalVar[thread_key]->map_config->begin(); iter != map_channelID_globalVar[thread_key]->map_config->end(); iter++) {
			//prep->map_table.insert(pair<string, string>(iter->first, iter->second));
			prep->map_table[iter->first] = iter->second;
		}
		net_ptr = new network(&(map_channelID_globalVar[thread_key]->errorRestartFlag), map_channelID_globalVar[thread_key]->fd_list);
		net_udp_ptr = new network_udp(&(map_channelID_globalVar[thread_key]->errorRestartFlag), map_channelID_globalVar[thread_key]->fd_list);
#else

		prep = new configuration(config_file);
		if (prep == NULL) {
			printf("[ERROR] prep new error \n");
			PAUSE
		}

		net_ptr = new network(&errorRestartFlag, &fd_list);
		if (net_ptr == NULL) {
			printf("[ERROR] net_ptr new error \n");
			PAUSE
		}
		net_udp_ptr = new network_udp(&errorRestartFlag, &fd_list);
		if (net_udp_ptr == NULL) {
			printf("[ERROR] net_ptr new error \n");
			PAUSE
		}

#endif

		register_mgr_ptr = new register_mgr();
		if (register_mgr_ptr == NULL) {
			printf("[ERROR] register_mgr_ptr new error \n");
			PAUSE
		}



		log_ptr = new logger();
		if (log_ptr == NULL) {
			printf("[ERROR] log_ptr new error \n");
			PAUSE
		}
		log_ptr->logger_set(net_ptr);
		logger_client_ptr = new logger_client(log_ptr);
		if (logger_client_ptr == NULL) {
			printf("[ERROR] logger_client_ptr new error \n");
			PAUSE
		}
#ifdef _FIRE_BREATH_MOD_
		peer_mgr_ptr = new peer_mgr(map_channelID_globalVar[thread_key]->fd_list);
		//stunt_mgr_ptr = new stunt_mgr(map_channelID_globalVar[thread_key]->fd_list);
#else

		peer_mgr_ptr = new peer_mgr(&fd_list);
		if (peer_mgr_ptr == NULL) {
			printf("[ERROR] peer_mgr_ptr new error \n");
			PAUSE
		}
		//stunt_mgr_ptr = new stunt_mgr(&fd_list);	// Release版本會有問題
		//if (stunt_mgr_ptr == NULL) {
		//	printf("[ERROR] stunt_mgr_ptr new error \n");
		//	PAUSE
		//}
#endif

#ifdef _WIN32
		WSADATA wsaData;										// Winsock initial data
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			debug_printf("WSAStartup ERROR\n");
			exit(0);
		}
#endif





#ifdef _FIRE_BREATH_MOD_
		char s[64];
		sprintf(s, "%d", thread_key);
#endif

		// Connect to register server
#ifdef _FIRE_BREATH_MOD_
		string channel_id_tmp = map_channelID_globalVar.find(thread_key)->second->map_config->find("channel_id")->second;
		register_mgr_ptr->build_connect(atoi(channel_id_tmp.c_str()));
#else
		//register_mgr_ptr->build_connect(1);
#endif		


		if (register_mgr_ptr->got_pk) {
			prep->map_table["pk_ip"] = register_mgr_ptr->pk_ip;
			prep->map_table["pk_port"] = register_mgr_ptr->pk_port;
			debug_printf("PK  ip:%s port:%s \n", register_mgr_ptr->pk_ip.c_str(), register_mgr_ptr->pk_ip.c_str());
		}


		log_ptr->start_log_record(SYS_FREQ);

		prep->read_key("html_size", html_size);
		prep->read_key("svc_tcp_port", svc_tcp_port);
		prep->read_key("svc_udp_port", svc_udp_port);
		prep->read_key("stream_local_port", stream_local_port);

#ifdef _FIRE_BREATH_MOD_
		pk_mgr_ptr = new pk_mgr(html_size, map_channelID_globalVar[thread_key]->fd_list, net_ptr, net_udp_ptr, log_ptr, prep, logger_client_ptr, stunt_mgr_ptr);
#else
		pk_mgr_ptr = new pk_mgr(html_size, &fd_list, net_ptr, net_udp_ptr, log_ptr, prep, logger_client_ptr, stunt_mgr_ptr);
#endif
		if (!pk_mgr_ptr) {
			printf("pk_mgr_ptr error !!!!!!!!!!!!!!!\n");
		}

		pk_mgr_ptr->record_file_fp = record_file_fp2;

		net_ptr->epoll_creater();
		net_udp_ptr->epoll_creater();
		//log_ptr->start_log_record(SYS_FREQ);

		peer_mgr_ptr->peer_mgr_set(net_ptr, net_udp_ptr, log_ptr, prep, pk_mgr_ptr, logger_client_ptr);
		//peer_mgr_ptr->pk_mgr_set(pk_mgr_ptr);
		pk_mgr_ptr->peer_mgr_set(peer_mgr_ptr);

		peer_communication_ptr = new peer_communication(net_ptr, net_udp_ptr, log_ptr, prep, peer_mgr_ptr, peer_mgr_ptr->get_peer_object(), pk_mgr_ptr, logger_client_ptr);
		if (!peer_communication_ptr) {
			printf("peer_commuication_ptr error!!!!!!!!!!\n");
		}
		peer_mgr_ptr->peer_communication_set(peer_communication_ptr);

		logger_client_ptr->set_net_obj(net_ptr);
		logger_client_ptr->set_pk_mgr_obj(pk_mgr_ptr);
		logger_client_ptr->set_prep_obj(prep);

#ifndef _WIN32
		signal(SIGALRM, signal_handler);
		signal(SIGPIPE, SIG_IGN);		// avoid crash on socket pipe 
		signal(SIGUSR1, SIG_IGN);
#endif



#ifdef _FIRE_BREATH_MOD_
#else
		signal(SIGTERM, signal_handler);
		signal(SIGINT, signal_handler);
		signal(SIGSEGV, signal_handler);
		signal(SIGABRT, signal_handler);
#endif

		if (MODE == MODE_RTMP) {
			/*
			printf("MODE_RTMP\n");
			amf_ptr = new amf(log_ptr);
			if (!amf_ptr) {
			printf("Can't new amf class\n");
			return -1;
			}

			rtmp_ptr = new rtmp(net_ptr, amf_ptr, log_ptr);
			if (!rtmp_ptr) {
			printf("Can't new rtmp class\n");
			return -1;
			}

			rtmp_supplement_ptr = new rtmp_supplement(log_ptr, rtmp_ptr, amf_ptr);
			if (!rtmp_supplement_ptr) {
			printf("Can't new rtmp_supplement class\n");
			return -1;
			}

			rtmp_svr = new rtmp_server(net_ptr, log_ptr, amf_ptr, rtmp_ptr, rtmp_supplement_ptr, pk_mgr_ptr, &fd_list);
			if (!rtmp_svr) {
			printf("Can't new rtsp_server class\n");
			return -1;
			}
			rtmp_svr->init(1,stream_local_port);
			printf("new rtmp_svr successfully\n");
			*/

			//MODE BitStream
		}
		else if (MODE == MODE_BitStream || MODE == MODE_HTTP) {
			debug_printf("MODE_BitStream \n");

#ifdef _FIRE_BREATH_MOD_
			bit_stream_server_ptr = new bit_stream_server(net_ptr, log_ptr, logger_client_ptr, pk_mgr_ptr, map_channelID_globalVar[thread_key]->fd_list);
#else
			bit_stream_server_ptr = new bit_stream_server(net_ptr, log_ptr, logger_client_ptr, pk_mgr_ptr, &fd_list);
#endif
			if (!bit_stream_server_ptr) {
				debug_printf("[ERROR] bit_stream_server_ptr error \n");
			}

			stringstream ss_tmp;
			unsigned short port_tcp;
			ss_tmp << stream_local_port;
			ss_tmp >> port_tcp;
#ifdef _FIRE_BREATH_MOD_
			map_channelID_globalVar[thread_key]->streamingPort = bit_stream_server_ptr->init(0, port_tcp);
			log_ptr->write_log_format("s(u) s u \n", __FUNCTION__, __LINE__, "Create bit_stream_server success. port =", map_channelID_globalVar[thread_key]->streamingPort);
			debug_printf("Create bit_stream_server success. port = %u", map_channelID_globalVar[thread_key]->streamingPort);
#else
			streamingPort = bit_stream_server_ptr->init(0, port_tcp);
			log_ptr->write_log_format("s(u) s d \n", __FUNCTION__, __LINE__, "Create bit_stream_server success. port =", streamingPort);
			debug_printf("Create bit_stream_server success, port = %u \n", streamingPort);
#endif
		}
		else if (MODE == MODE_RTSP) {
			debug_printf("MODE_RTSP \n");
		}

		// Create TCP listening socket
		if ((svc_fd_tcp = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
#ifdef _WIN32
			int socketErr = WSAGetLastError();
#else
			int socketErr = errno;
#endif
			log_ptr->write_log_format("s(u) s s d d \n", __FUNCTION__, __LINE__, "[ERROR] Create TCP socket failed", ". Socket error", socketErr, svc_fd_tcp);
			debug_printf("[ERROR] Create TCP socket failed. socket error %d %d \n", socketErr, svc_fd_tcp);
			PAUSE
		}

		// Read default TCP port
		tcp_port = (unsigned short)atoi(svc_tcp_port.c_str());

		// Find an available TCP port
		for (;; tcp_port++) {
			memset(&sin, 0, sizeof(struct sockaddr_in));
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = INADDR_ANY;
			sin.sin_port = htons(tcp_port);
			if (::bind(svc_fd_tcp, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0) {
#ifdef _WIN32
				int socketErr = WSAGetLastError();
#else
				int socketErr = errno;
#endif
				log_ptr->write_log_format("s(u) s d s d \n", __FUNCTION__, __LINE__, "Socket bind failed at port", tcp_port, ". Socket error", socketErr);
				continue;
			}
			if (::listen(svc_fd_tcp, MAX_POLL_EVENT) < 0) {
#ifdef _WIN32
				int socketErr = WSAGetLastError();
#else
				int socketErr = errno;
#endif
				log_ptr->write_log_format("s(u) s d s d \n", __FUNCTION__, __LINE__, "Socket listen failed at port", tcp_port, ". Socket error", socketErr);
				continue;
			}
			break;
		}
		debug_printf("Create TCP listening socket, port = %u \n", tcp_port);
		log_ptr->write_log_format("s(u) s d \n", __FUNCTION__, __LINE__, "Create TCP listening socket, port =", tcp_port);
		net_ptr->set_nonblocking(svc_fd_tcp);

		udp_port_int = (unsigned short)atoi(svc_udp_port.c_str());

		// Find an available UDP port and get the external port
		string stun_ip;
		StunAddress4 stunServerAddr;
		StunAddress4 sAddr;
		int srcPort = stunRandomPort();
		memset(&stunServerAddr, 0, sizeof(stunServerAddr));
		memset(&sAddr, 0, sizeof(sAddr));

		sAddr.port = udp_port_int;
		prep->read_key("stun_ip", stun_ip);

		bool ret = stunParseServerName(const_cast<char *>(stun_ip.c_str()), stunServerAddr);

		// Bind an available port and get public port from STUN server
		stunTest(stunServerAddr, 1, true, &udp_port_int, &udp_port_ext, &sAddr);

		// If cannot get the response from STUN, udp_port_ext will be 0. Then set it as internal port
		if (udp_port_ext == 0) {
			udp_port_ext = udp_port_int;
		}

		// Create UDP listening socket
		if ((svc_fd_udp = UDT::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
#ifdef _WIN32
			int socketErr = WSAGetLastError();
#else
			int socketErr = errno;
#endif
			log_ptr->write_log_format("s(u) s s d d \n", __FUNCTION__, __LINE__, "[ERROR] Create UDP socket failed", ". Socket error", socketErr, svc_fd_udp);
			debug_printf("[ERROR] Create UDP socket failed. socket error %d %d \n", socketErr, svc_fd_udp);
			PAUSE
		}

		for (;; udp_port_int++) {
			memset(&sin, 0, sizeof(struct sockaddr_in));
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = htonl(INADDR_ANY);
			sin.sin_port = htons(udp_port_int);

			// UDT_REUSEADDR is true by default
			if (UDT::bind(svc_fd_udp, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) == UDT::ERROR) {
				debug_printf("ErrCode: %d  ErrMsg: %s \n", UDT::getlasterror().getErrorCode(), UDT::getlasterror().getErrorMessage());
				log_ptr->write_log_format("s(u) s d s s \n", __FUNCTION__, __LINE__, "ErrCode:", UDT::getlasterror().getErrorCode(), "ErrMsg", UDT::getlasterror().getErrorMessage());
				continue;
			}

			if (UDT::listen(svc_fd_udp, MAX_POLL_EVENT) == UDT::ERROR) {
				debug_printf("ErrCode: %d  ErrMsg: %s \n", UDT::getlasterror().getErrorCode(), UDT::getlasterror().getErrorMessage());
				log_ptr->write_log_format("s(u) s d s s \n", __FUNCTION__, __LINE__, "ErrCode:", UDT::getlasterror().getErrorCode(), "ErrMsg", UDT::getlasterror().getErrorMessage());
				continue;
			}
			break;
		}
		debug_printf("Create UDP listen-socket success, internal port = %u, external port = %u \n", udp_port_int, udp_port_ext);
		log_ptr->write_log_format("s(u) s u s u \n", __FUNCTION__, __LINE__, "Create UDP listen-socket, internal port", udp_port_int, "external port", udp_port_ext);
		prep->add_key("svc_udp_port", udp_port_int);


		io_accept_udp_ptr = new io_accept_udp(net_udp_ptr, log_ptr, peer_mgr_ptr, peer_communication_ptr, logger_client_ptr);

		if (!log_ptr->check_arch_compatible()) {
			cout << "Hardware Architecture is not support." << endl;
			PAUSE
				log_ptr->exit(0, "Hardware Architecture is not support.");
		}

#ifndef _WIN32
		struct itimerval interval;
		interval.it_interval.tv_sec = SIG_FREQ;
		interval.it_interval.tv_usec = 0;
		interval.it_value.tv_sec = SIG_FREQ;
		interval.it_value.tv_usec = 0;

		// setup periodic timer (3 second) 
		if (setitimer(ITIMER_REAL, &interval, NULL)) {
			log_ptr->write_log_format("s =>u s d \n", __FUNCTION__, __LINE__, "setitimer ERROR ");
			return -1;
		}

		getitimer(ITIMER_REAL, &interval);
#endif

		net_ptr->set_nonblocking(svc_fd_tcp);
		net_ptr->epoll_control(svc_fd_tcp, EPOLL_CTL_ADD, EPOLLIN);
		net_ptr->set_fd_bcptr_map(svc_fd_tcp, dynamic_cast<basic_class *>(peer_communication_ptr->get_io_accept_handler()));

		net_ptr->set_nonblocking(svc_fd_udp);
		net_udp_ptr->epoll_control(svc_fd_udp, EPOLL_CTL_ADD, UDT_EPOLL_IN);
		net_udp_ptr->set_fd_bcptr_map(svc_fd_udp, dynamic_cast<basic_class *>(io_accept_udp_ptr));

#ifdef _FIRE_BREATH_MOD_
		(map_channelID_globalVar[thread_key]->fd_list)->push_back(svc_fd_tcp);
		(map_channelID_globalVar[thread_key]->udp_fd_list)->push_back(svc_fd_udp);
#else
		fd_list.push_back(svc_fd_tcp);
		udp_fd_list.push_back(svc_fd_udp);
#endif
		pk_mgr_ptr->init(tcp_port, udp_port_ext, udp_port_int);
		logger_client_ptr->log_init();


		log_ptr->write_log_format("s(u) s d \n", __FUNCTION__, __LINE__, "bit_stream_server", *bit_stream_server_ptr);
		log_ptr->write_log_format("s(u) s d \n", __FUNCTION__, __LINE__, "io_accept_udp", *io_accept_udp_ptr);
		log_ptr->write_log_format("s(u) s d \n", __FUNCTION__, __LINE__, "io_connect_udp", peer_communication_ptr->_io_connect_udp_ptr);
		log_ptr->write_log_format("s(u) s d \n", __FUNCTION__, __LINE__, "io_nonblocking_udp", peer_communication_ptr->_io_nonblocking_udp_ptr);
		log_ptr->write_log_format("s(u) s d \n", __FUNCTION__, __LINE__, "peer_communication", *peer_communication_ptr);
		log_ptr->write_log_format("s(u) s d \n", __FUNCTION__, __LINE__, "peer", peer_communication_ptr->_peer_ptr);
		log_ptr->write_log_format("s(u) s d \n", __FUNCTION__, __LINE__, "logger_client", *logger_client_ptr);
		log_ptr->write_log_format("s(u) s d \n", __FUNCTION__, __LINE__, "pk_mgr", *pk_mgr_ptr);

#ifdef _FIRE_BREATH_MOD_

		// Invoke jwplayer in javascript
		FB::JSObjectPtr obj = map_channelID_globalVar[thread_key]->window->getProperty<FB::JSObjectPtr>("window");
		obj->Invoke("start_jwplayer", FB::variant_list_of(map_channelID_globalVar[thread_key]->streamingPort));




		map_channelID_globalVar[thread_key]->pk_mgr_ptr_copy = pk_mgr_ptr;
		map_channelID_globalVar[thread_key]->http_srv_ready = 1;
		map_channelID_globalVar[thread_key]->is_Pk_mgr_ptr_copy_delete = FALSE;

		map<string, string>::iterator iter;
		for (iter = map_channelID_globalVar[thread_key]->map_config->begin(); iter != map_channelID_globalVar[thread_key]->map_config->end(); iter++) {
			log_ptr->write_log_format("s(u) s s \n", __FUNCTION__, __LINE__, (iter->first).c_str(), (iter->second).c_str());
		}
		for (iter = prep->map_table.begin(); iter != prep->map_table.end(); iter++) {
			log_ptr->write_log_format("s(u) -- s s \n", __FUNCTION__, __LINE__, (iter->first).c_str(), (iter->second).c_str());
		}
		log_ptr->write_log_format("s(u) s \n", __FUNCTION__, __LINE__, "Before PAUSE");
		//PAUSE
		log_ptr->write_log_format("s(u) s \n", __FUNCTION__, __LINE__, "After PAUSE");
		debug_printf("\n");
		while (!map_channelID_globalVar[thread_key]->srv_shutdown && !map_channelID_globalVar[thread_key]->errorRestartFlag) {
#else
		while (!srv_shutdown && !errorRestartFlag) {
#endif

#ifdef _WIN32
			for (int i = 0; i < 1; i++) {
#ifdef _FIRE_BREATH_MOD_
				net_ptr->epoll_waiter(1000, map_channelID_globalVar[thread_key]->fd_list);
#else
				net_ptr->epoll_waiter(1000, &fd_list);
#endif

#else
				net_ptr->epoll_waiter(1000);
#endif
				net_ptr->epoll_dispatcher();
			}
			pk_mgr_ptr->time_handle();
#ifdef _FIRE_BREATH_MOD_
			net_udp_ptr->epoll_waiter(5, map_channelID_globalVar[thread_key]->udp_fd_list);
#else
			net_udp_ptr->epoll_waiter(5, &udp_fd_list);
#endif

			net_udp_ptr->epoll_dispatcher();

			if (*(net_ptr->_errorRestartFlag) == RESTART) {
				log_ptr->write_log_format("s(u) s \n\n\n", __FUNCTION__, __LINE__, "Program Restart");
				break;
			}

		}

		pk_exit_code = pk_mgr_ptr->exit_code;
		log_exit_code = logger_client_ptr->exit_code;


		log_ptr->write_log_format("s(u) s d s d \n", __FUNCTION__, __LINE__, "pk_exit_code =", pk_exit_code, "log_exit_code =", log_exit_code);
		debug_printf("pk_exit_code = %d, log_exit_code = %d \n", pk_exit_code, log_exit_code);


#ifdef _FIRE_BREATH_MOD_
		handle_restart(thread_key, pk_exit_code, log_exit_code, (int *)&(map_channelID_globalVar[thread_key]->srv_shutdown));
#else
		handle_restart(0, pk_exit_code, log_exit_code, (int *)&srv_shutdown);
#endif




#ifdef _FIRE_BREATH_MOD_
		log_ptr->write_log_format("s(u) s d \n", __FUNCTION__, __LINE__, "exit_code =", map_channelID_globalVar[thread_key]->exit_code);
		log_ptr->write_log_format("s(u) s d s d \n", __FUNCTION__, __LINE__, "srv_shutdown", map_channelID_globalVar[thread_key]->srv_shutdown, "errorRestartFlag", map_channelID_globalVar[thread_key]->errorRestartFlag);
#else
		log_ptr->write_log_format("s(u) s d s d \n", __FUNCTION__, __LINE__, "pk_exit_code =", pk_exit_code, "log_exit_code =", log_exit_code);
		log_ptr->write_log_format("s(u) s d s d \n", __FUNCTION__, __LINE__, "srv_shutdown", srv_shutdown, "errorRestartFlag", errorRestartFlag);
#endif

		net_ptr->garbage_collection();
		net_udp_ptr->garbage_collection();
		log_ptr->write_log_format("s => s (s)\n", (char*)__PRETTY_FUNCTION__, "PF", "graceful exit!!");
		log_ptr->stop_log_record();

		if (UDT::close(svc_fd_udp) == UDT::ERROR) {
			debug_printf("ErrCode: %d  ErrMsg: %s \n", UDT::getlasterror().getErrorCode(), UDT::getlasterror().getErrorMessage());
			log_ptr->write_log_format("s(u) s d s s \n", __FUNCTION__, __LINE__, "ErrCode:", UDT::getlasterror().getErrorCode(), "ErrMsg", UDT::getlasterror().getErrorMessage());
			PAUSE
		}

		debug_printf("1 \n");

		if (prep) {
			debug_printf("2 \n");
			delete prep;
		}
		debug_printf("3 \n");
		prep = NULL;

		if (bit_stream_server_ptr) {
			delete bit_stream_server_ptr;
		}
		bit_stream_server_ptr = NULL;

		if (peer_communication_ptr) {
			delete peer_communication_ptr;
		}
		peer_communication_ptr = NULL;

		if (net_ptr) {
			delete net_ptr;
		}
		net_ptr = NULL;

		if (net_udp_ptr) {
			delete net_udp_ptr;
		}
		net_udp_ptr = NULL;

#ifdef _FIRE_BREATH_MOD_
		map_channelID_globalVar[thread_key]->is_Pk_mgr_ptr_copy_delete = TRUE;
		map_channelID_globalVar[thread_key]->pk_mgr_ptr_copy = NULL;
		map_channelID_globalVar[thread_key]->http_srv_ready = 0;
#endif
		if (pk_mgr_ptr) {
			delete pk_mgr_ptr;
		}
		pk_mgr_ptr = NULL;

		//		if(rtmp_svr)
		//			delete rtmp_svr;
		//		rtmp_svr =NULL;

		if (peer_mgr_ptr) {
			delete peer_mgr_ptr;
		}
		peer_mgr_ptr = NULL;

		if (log_ptr) {
			delete log_ptr;
		}
		log_ptr = NULL;

		if (logger_client_ptr) {
			delete logger_client_ptr;
		}
		logger_client_ptr = NULL;
		/*
		for (int i = 0; i < 3; i++) {
		printf("Some Thing Error Wait %d Sec To Restart ............\n",3-i );
		Sleep(1000);
		}
		*/

		//srv_shutdown = 1;
		debug_printf("Client Restart \n");
		PAUSE

		WSACleanup();

		srand(time(NULL));
		Sleep(rand() % 3 + 1);


#ifdef _FIRE_BREATH_MOD_
		map_channelID_globalVar[thread_key]->errorRestartFlag = 0;
#else
		errorRestartFlag = 0;
#endif

		//PAUSE
		debug_printf("addr %d %s  port %d \n", stunServerAddr.addr, inet_ntoa(*(struct in_addr *)&stunServerAddr), stunServerAddr.port);
	}

#ifdef _FIRE_BREATH_MOD_
	map_channelID_globalVar[thread_key]->streamingPort = 0;
	map_channelID_globalVar[thread_key]->handle_sig_alarm = 0;
	map_channelID_globalVar[thread_key]->srv_shutdown = 0;
	map_channelID_globalVar[thread_key]->errorRestartFlag = 0;
	delete map_channelID_globalVar[thread_key]->fd_list;
	free(map_channelID_globalVar[thread_key]);
	map_channelID_globalVar.erase(thread_key);
#else
	streamingPort = 0;
	handle_sig_alarm = 0;
	srv_shutdown = 0;
	errorRestartFlag = 0;
#endif



	return EXIT_SUCCESS;
}






int connect_irc(int thread_key)
{
#ifdef IRC_CLIENT
	map<int, GlobalVars*>::iterator iter = map_channelID_globalVar.find(thread_key);
	if (iter == map_channelID_globalVar.end()) {
		return -1;
	}
	if (iter->second->is_init == 0) {
		return -2;
	}



	//irc_cli_thread *irc_cli_thread_ptr = (irc_cli_thread *)irc_arg;

	//unsigned short port = iter->second->irc_arg.port;
	unsigned short port = 6667;
	char *ip = iter->second->irc_arg.ip;
	char *nick = iter->second->irc_arg.nick;
	char *channel = iter->second->irc_arg.channel;

	//memcpy(ip, irc_cli_thread_ptr->ip, strlen(irc_cli_thread_ptr->ip));
	//memcpy(nick, irc_cli_thread_ptr->nick, strlen(irc_cli_thread_ptr->nick));
	//memcpy(channel, irc_cli_thread_ptr->channel, strlen(irc_cli_thread_ptr->channel));

	irc_callbacks_t	callbacks;
	irc_ctx_t ctx;
	irc_session_t * s;



	memset(&callbacks, 0, sizeof(callbacks));

	callbacks.event_connect = event_connect;
	callbacks.event_join = event_join;
	callbacks.event_nick = dump_event;
	callbacks.event_quit = dump_event;
	callbacks.event_part = dump_event;
	callbacks.event_mode = dump_event;
	callbacks.event_topic = dump_event;
	callbacks.event_kick = dump_event;
	callbacks.event_channel = event_channel;
	callbacks.event_privmsg = event_privmsg;
	callbacks.event_notice = dump_event;
	callbacks.event_invite = dump_event;
	callbacks.event_umode = dump_event;
	callbacks.event_ctcp_rep = dump_event;
	callbacks.event_ctcp_action = dump_event;
	callbacks.event_unknown = dump_event;
	callbacks.event_numeric = event_numeric;

	callbacks.event_dcc_chat_req = irc_event_dcc_chat;
	callbacks.event_dcc_send_req = irc_event_dcc_send;

	s = irc_create_session(&callbacks);

	if (!s)
	{
		printf("Could not create session\n");
		return 1;
	}
	iter->second->session = s;

	ctx.channel = NULL;
	ctx.nick = NULL;
	//memcpy(ctx.channel, channel.c_str(), channel.length());
	//memcpy(ctx.nick, nick.c_str(), nick.length());
	ctx.channel = channel;
	ctx.nick = nick;
	//ctx.channel = argv[3];
	//ctx.nick = argv[2];

	irc_set_ctx(s, &ctx);


	// To handle the "SSL certificate verify failed" from command line we allow passing ## in front 
	// of the server name, and in this case tell libircclient not to verify the cert

	// Initiate the IRC server connection
	if (irc_connect(s, ip, port, 0, nick, 0, 0))
	{
		printf("Could not connect: %s\n", irc_strerror(irc_errno(s)));
		return 1;
	}

	// and run into forever loop, generating events

	if (irc_run(s))
	{
		printf("Could not connect or I/O error: %s\n", irc_strerror(irc_errno(s)));
		return 1;
	}
	int as = 2;
#endif
	return 1;

}

/**********************************************************\

Auto-generated llp2pFBAPI.cpp

\**********************************************************/
/*
#include "JSObject.h"
#include "variant_list.h"
#include "DOM/Document.h"
#include "DOM/Window.h"
#include "global/config.h"

#include "llp2pFBAPI.h"

///////////////////////////////////////////////////////////////////////////////
/// @fn FB::variant llp2pFBAPI::echo(const FB::variant& msg)
///
/// @brief  Echos whatever is passed from Javascript.
///         Go ahead and change it. See what happens!
///////////////////////////////////////////////////////////////////////////////
FB::variant llp2pFBAPI::echo(const FB::variant& msg)
{
static int n(0);
fire_echo("So far, you clicked this many times: ", n++);

// return "foobar";
return msg;
}

///////////////////////////////////////////////////////////////////////////////
/// @fn llp2pFBPtr llp2pFBAPI::getPlugin()
///
/// @brief  Gets a reference to the plugin that was passed in when the object
///         was created.  If the plugin has already been released then this
///         will throw a FB::script_error that will be translated into a
///         javascript exception in the page.
///////////////////////////////////////////////////////////////////////////////
llp2pFBPtr llp2pFBAPI::getPlugin()
{
llp2pFBPtr plugin(m_plugin.lock());
if (!plugin) {
throw FB::script_error("The plugin is invalid");
}
return plugin;
}

// Read/Write property testString
std::string llp2pFBAPI::get_testString()
{
return m_testString;
}

void llp2pFBAPI::set_testString(const std::string& val)
{
m_testString = val;
}

// Read-only property version
std::string llp2pFBAPI::get_version()
{
return FBSTRING_PLUGIN_VERSION;
}

void llp2pFBAPI::testEvent()
{
fire_test();
}

void llp2pFBAPI::mytestEvent()
{


// Retrieve a reference to the DOM Window
FB::DOM::WindowPtr window = m_host->getDOMWindow();

// Check if the DOM Window has an alert peroperty
if (window && window->getJSObject()->HasProperty("window")) {
// Create a reference to alert
FB::JSObjectPtr obj = window->getProperty<FB::JSObjectPtr>("window");

// Invoke alert with some text
obj->Invoke("alert", FB::variant_list_of("This is a test alert invoked from an NPAPI Plugin !!!"));
//obj->Invoke("testCallByPlugin", FB::variant_list_of(ss.str()));
}

}
*/