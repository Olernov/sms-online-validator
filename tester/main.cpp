#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <iostream>
#include <string>
#ifdef _WIN32
    #include <conio.h>
    #include <Winsock2.h>
#else
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <inttypes.h>
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <ctype.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>
#endif
#include "ps_common.h"
#include "pspacket.h"
#include "Common.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

#ifdef WIN32
    #define SOCK_ERR WSAGetLastError()
#else
    #define SOCK_ERR errno
#endif

void CloseSocket(int socket)
{
#ifdef WIN32
	shutdown(socket, SD_BOTH);
	closesocket(socket);
#else
	shutdown(socket, SHUT_RDWR);
	close(socket);
#endif
}

#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif

#if defined(__linux__)
    #define ntohll(x) be64toh(x)
    #define htonll(x) htobe64(x)
    #define SOCKET_ERROR -1
    #define INVALID_SOCKET -1
    #define sprintf_s snprintf
    #define ioctlsocket ioctl
    #define WSAEWOULDBLOCK EWOULDBLOCK
#endif


const int PARSE_SUCCESS = 0;

int ConnectToVLRService(const char* ipAddress, int port)
{
    /* First call to socket() function */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
      printf("Unable to create socket AF_INET, SOCK_DGRAM.\n");
      return -1;
    }
    /* Initialize socket structure */
    struct sockaddr_in serv_addr;
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ipAddress);
    serv_addr.sin_port = htons(port);

//    if(connect(sock,(sockaddr*) &serv_addr, sizeof( serv_addr ))==-1) {
//        printf("Failed connecting to host %s. Error code=%d. Initialization failed.", ipAddress, SOCK_ERR);
//		CloseSocket(sock);
//        return -1;
//    }
//    printf("Connected to %s.\n-----------------------------------------------\n", ipAddress);
#ifdef WIN32
    u_long iMode=1;
    if(ioctlsocket(sock, FIONBIO, &iMode) != 0) {
        // Catch error
        printf("Error setting socket in non-blocking mode: %d. Initialization failed.", SOCK_ERR);
        return -1;
    }
#else
    fcntl(sock, F_SETFL, O_NONBLOCK);  // set to non-blocking
#endif
    return sock;
}


bool ComposeAndSendRequest(std::string origination, unsigned long requestNum, int socket,
                           sockaddr_in serverAddr)
{
    CPSPacket psPacket;
    const std::string destination = "79027111125";
    const std::string servingMSC = "79506656021";

	unsigned char buffer[1024];

    if(psPacket.Init((SPSRequest*)buffer, sizeof(buffer), requestNum++, VALIDATEEX_REQ)) {
        printf("SPSRequest.Init failed, buffer too small" );
        return false;
    }

    if (!origination.empty()) {
        psPacket.AddAttr((SPSRequest*)buffer, sizeof(buffer), VLD_OA,
            (const void*)origination.data(), origination.size());
	}

    psPacket.AddAttr((SPSRequest*)buffer, sizeof(buffer), VLD_DA,
        (const void*)destination.data(), destination.size());
    uint8_t oaflags = 145;
    uint8_t daflags = 145;
    psPacket.AddAttr((SPSRequest*)buffer, sizeof(buffer), VLD_OAFLAGS,
        (const void*)&oaflags, sizeof(oaflags));
    psPacket.AddAttr((SPSRequest*)buffer, sizeof(buffer), VLD_DAFLAGS,
        (const void*)&daflags, sizeof(daflags));
    uint16_t referenceNum = 1000;
    uint16_t referenceNumNO = htons(referenceNum);
    psPacket.AddAttr((SPSRequest*)buffer, sizeof(buffer), VLD_REFNUM,
        (const void*)&referenceNumNO, sizeof(referenceNumNO));
    uint8_t totalParts = 3;
    uint8_t partNum = 1;
    psPacket.AddAttr((SPSRequest*)buffer, sizeof(buffer), VLD_TOTAL,
        (const void*)&totalParts, sizeof(totalParts));
    psPacket.AddAttr((SPSRequest*)buffer, sizeof(buffer), VLD_PART,
        (const void*)&partNum, sizeof(partNum));
    int len = psPacket.AddAttr((SPSRequest*)buffer, sizeof(buffer), VLD_SERVINGMSC,
        (const void*)servingMSC.data(), servingMSC.size());
    std::cout << "Sending " << len << " bytes" << std::endl;

    int serverLen = sizeof(serverAddr)  ;
    if(sendto(socket, (char*)buffer, len, 0, (sockaddr*)&serverAddr, serverLen) <= 0) {
        printf("Error sending data on socket: %d\n" ,SOCK_ERR);
        return false;
    }
    return true;
}


void PrintBinaryDump(const unsigned char* data, size_t size)
{
    char buffer[2048];
    size_t i = 0;
    for (; i < size; i++) {
        if (3 * (i + 1) >= sizeof(buffer) - 1)
            break;
        sprintf(&buffer[3 * i], "%02X ", data[i]);
    }
	buffer[3 * i] = '\0';
	std::cout << buffer;
}


int ParseNextResponseFromBuffer(unsigned char* buffer, int dataLen)
{
    __uint32_t requestNum;
    __uint16_t requestType;
    __uint16_t packetLen = 0;
    char szTextParse[2048];
    std::multimap<__uint16_t, SPSReqAttrParsed> mmRequest;
    const int VALIDATE_PACKET = 1;
    CPSPacket spPacket;
    SPSRequest *pspRequest = (SPSRequest *)buffer;
    mmRequest.clear();
    int parseRes = spPacket.Parse(pspRequest, dataLen, requestNum, requestType, packetLen, mmRequest, VALIDATE_PACKET);
     if (parseRes == PARSE_SUCCESS) {
        if (requestType == VALIDATEEX_RESP) {
            std::cout << "VALIDATEEX_RESP, requestType: 0x" << std::hex << requestType << std::dec
				<<", requestNum: " << requestNum << std::endl;
            for(auto it = mmRequest.begin(); it != mmRequest.end(); it++) {
                size_t dataLen = it->second.m_usDataLen;
                std::cout << "AttrID: " << std::hex << it->second.m_usAttrType
                          << ", len: " << std::dec << dataLen << std::endl;
				if (it->second.m_usAttrType == PS_RESULT) {
                    int32_t resultCode = ntohl(*reinterpret_cast<int32_t*>(it->second.m_pvData));
                    std::cout << "PS_RESULT: " << std::to_string(resultCode) << std::endl;
				}
				else if (it->second.m_usAttrType == PS_DESCR) {
					char descr[1024];
					strncpy(descr, static_cast<char*>(it->second.m_pvData), it->second.m_usDataLen);
					descr[it->second.m_usDataLen] = '\0';
                    std::cout << "PS_DESCR: " << descr << std::endl;
				}
				else {
					unsigned char* data = static_cast<unsigned char*>(it->second.m_pvData);
					std::cout << "Data: ";
					PrintBinaryDump(data, dataLen);
					std::cout << std::endl;
				}
            }
        }
        else {
            spPacket.Parse(pspRequest, 2048, szTextParse, 2048);
            printf("%s\n",szTextParse );
        }
    }
    else {
        std::cout << "Parsing response packet failed" << std::endl;
        return -1;
    }
    return packetLen;
}


int main(int argc, char* argv[])
{
    unsigned int requestNum = 1;
    int port;
	unsigned char buffer[10240];

    if(argc<2) {
        printf("Usage: %s ip_address [port]", argv[0]);
        exit(1);
    }
	if (argc > 2) {
		port = atoi(argv[2]);
	}
	else {
        port = 5300;
	}

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
      printf("Unable to create socket AF_INET, SOCK_DGRAM.\n");
      exit(EXIT_FAILURE);
    }
    /* Initialize socket structure */
    struct sockaddr_in serv_addr;
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(port);

    std::cout << "Choose IMSI: \n\t1-2 - correct, \n\t3 - my test phone, \n\t4 - roaming, \n\t5 - missing OA in request, "
        "\n\t6 - 100 requests pack\n\tq - quit \n\t"
        "any other letter - read socket data:\n";
	while (true) {
		fd_set read_set;
        struct timeval tv;
        tv.tv_sec = 0;  // time-out
        tv.tv_usec = 200;
        FD_ZERO( &read_set );
        FD_SET( sock, &read_set );

		if (select(sock + 1, &read_set, NULL, NULL, &tv) != 0) {
			int bytesReceived = recv(sock, (char*)buffer, sizeof(buffer), 0);
			if (bytesReceived <= 0) {
				printf("Error receiving data on socket: %d\nPress any key to exit ...", SOCK_ERR);
				char c = getchar();
				break;
			}
			printf("\n%d bytes received.\n", bytesReceived);
			int bytesProcessed = 0;
			while (bytesProcessed < bytesReceived) {
				int nextLen = ParseNextResponseFromBuffer(buffer + bytesProcessed, bytesReceived);
				if (nextLen < 0) {
					break;
				}
				bytesProcessed += nextLen;
			}
			printf("\n----------------------------\n");
		}
		else {
            std::string cmd;
            std::cin >> cmd;

            std::cout << "Entered option: " << cmd << std::endl;
            std::string origination;
			int requestsCount = 1;
            if (cmd == "1") {
                origination = "79506656066";
            }
            else if (cmd == "2") {
                origination = "79506656066";
            }
            else if (cmd == "3") {
                origination = "79506656066";
            }
            else if (cmd == "4") {
                requestType = keepAlive;
            }
            else if (cmd == "5") {
                ;
            }
            else if (cmd == "6") {
                requestsCount = 100;
                origination = "79506656066";
            }
            else if (cmd == "q" || cmd == "Q") {
                std::cout << "Exit option entered, goodbye!" << std::endl;
                break;
            }
            else {
				continue;
			}

			for (int i = 0; i < requestsCount; i++) {
                ComposeAndSendRequest(origination, requestNum++, sock, serv_addr);
			}
		}
	}

	CloseSocket(sock);
}


