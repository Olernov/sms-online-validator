#pragma once
#include <list>
#include <map>
#include "ps_common.h"
#include "pspacket.h"
#include "Common.h"
#include "ClientRequest.h"


class Server
{
public:
    Server();
    bool Initialize(unsigned int port, std::string &errDescription);
	~Server();
	void Run();
    void Stop();

private:
	static const int MAX_CLIENT_CONNECTIONS = 10;
	static const int PARSE_ERROR = -1;

    int udpSocket;
    bool shutdownInProgress;

    void ProcessIncomingData(const char* buffer, int bufferSize, sockaddr_in &senderAddr);
    int ProcessNextRequestFromBuffer(const char* buffer, int maxLen, sockaddr_in& senderAddr);
    bool SendNotAcceptedResponse(sockaddr_in &senderAddr, uint32_t requestNum, std::string errDescr);
	bool SendIAMAliveResponse(sockaddr_in& senderAddr, uint32_t requestNum, std::string errDescr);
    std::string IPAddr2Text(const in_addr& pinAddr);

};

