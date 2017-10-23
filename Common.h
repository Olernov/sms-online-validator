#pragma once
#ifdef _WIN32
	#include <Winsock2.h>
	#include <windows.h>
#else
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <inttypes.h>
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <ctype.h>
#endif

#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <map>
#include "ps_common.h"
#include "pspacket.h"

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



#define MAX_THREADS							16
#define MAX_DMS_RESPONSE_LEN				1000
#define ERROR_INIT_PARAMS					-1
#define INIT_FAIL							-2
#define NETWORK_ERROR						-3
#define BAD_DEVICE_RESPONSE					-4
#define NO_CONNECTION_TO_VLR				-5
#define BAD_REQUEST					-11
#define CMD_NOTEXECUTED						-10
#define CMD_UNKNOWN							-30
#define EXCEPTION_CAUGHT					-999
#define OPERATION_SUCCESS					0
#define INFO_NOT_COMPLETE					1
#define ALL_CONNECTIONS_BUSY				-33333

#define SOCKET_TIMEOUT_SEC					10



#define STR_TERMINATOR				'\0'
#define CR_CHAR_CODE				'\r'
#define LF_CHAR_CODE				'\n'
#define HLR_PROMPT					"\x03<"
#define TERMINAL_TYPE				"vt100"


enum {
    VLD_OA = 0x1003,
    VLD_OAFLAGS,
    VLD_DA,
    VLD_DAFLAGS,
    VLD_REFNUM,
    VLD_TOTAL,
    VLD_PART,
    VLD_SERVINGMSC,
    VLD_IMSI
};

enum {
    CAMEL_IMSI = 0x0001,
    CAMEL_CALLING_PARTY = 0x0002,
    CAMEL_CALLING_NOA = 0x0003,
    CAMEL_CALLED_PARTY = 0x0004,
    CAMEL_CALLED_NOA = 0x0005,
    CAMEL_CALL_REF_NUM = 0x0006,
    CAMEL_EVENT_TYPE = 0x0007,
    CAMEL_SERVICE_KEY = 0x0008,
    CAMEL_TOTAL_DURATION = 0x0009,
    CAMEL_SUCCESS_CODE = 0x0000,
    CAMEL_ERROR_DESCR = 0x0001,
    CAMEL_QUOTA_RESULT = 0x0002,
    CAMEL_QUOTA_SECONDS = 0x0003
};


const int mainThreadIndex = -1;
const std::string crlf = "\n";

#if defined(_WIN32) || defined(_WIN64) 
	#define vsnprintf _vsnprintf 
	#define strcasecmp _stricmp 
	#define strncasecmp _strnicmp 
	#define localtime_r(time_t, tm) localtime_s(tm, time_t)
	#define snprintf sprintf_s
#endif

typedef std::multimap<__uint16_t, SPSReqAttrParsed> psAttrMap;

static const uint16_t VALIDATEEX_REQ = 0x1100;
static const uint16_t VALIDATEEX_RESP = 0x1101;
static const uint16_t ARE_Y_ALIVE = 0x0004;
static const uint16_t I_AM_ALIVE = 0x0005;
static const uint16_t QUOTA_REQ = 0x1200;
static const uint16_t QUOTA_RESP = 0x1201;
static const uint16_t CALL_FINISH_INFO = 0x1202;
static const uint16_t CALL_FINISH_ACK = 0x1203;
