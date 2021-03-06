#include <time.h>
#include <stdarg.h>
#ifndef _WIN32
#include <signal.h>
#include <unistd.h>
#endif
#include "Common.h"
#include "Config.h"
#include "LogWriterOtl.h"
#include "ConnectionPool.h"
#include "Server.h"


LogWriterOtl logWriter;
Server server;


#ifndef _WIN32
void SignalHandler(int signum, siginfo_t *info, void *ptr)
{
    std::cout << "Received signal #" <<signum << " from process #" << info->si_pid << ". Stopping ..." << std::endl;
    server.Stop();
}
#endif


void printUsage(char* programName)
{
    std::cerr << "Usage: " << std::endl << programName << " <config-file> [-test]" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printUsage(argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* confFilename = argv[1];
    std::ifstream confFile(confFilename, std::ifstream::in);
    if (!confFile.is_open()) {
        std::cerr << "Unable to open config file " << confFilename << std::endl;
        exit(EXIT_FAILURE);
    }
    Config config;
    try {
        config.ReadConfigFile(confFile);
        config.ValidateParams();
    }
    catch(const std::exception& ex) {
        std::cerr << "Error when parsing config file " << confFilename << " " << std::endl;
        std::cerr << ex.what() <<std::endl;
        exit(EXIT_FAILURE);
    }

#ifndef _WIN32
    const std::string pidFilename = "/var/run/sms-validator.pid";
    std::ofstream pidFile(pidFilename, std::ofstream::out);
    if (pidFile.is_open()) {
        pidFile << getpid();
    }
    pidFile.close();
#endif
#ifdef WIN32
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2), &wsaData)) {
		std::cout << "Unable to init WinSock. Exiting." << std::endl;
		exit(EXIT_FAILURE);
	}
#endif
	
    try {
        logWriter.Initialize(config.logDir, "sms", config.logLevel);
        logWriter << "SMS online validator start. Configuration settings:";
        logWriter << config.DumpAllSettings();

        std::string errDescription;
        if (!server.Initialize(config, /*&connectionPool,*/ errDescription)) {
            std::cerr << "Unable to initialize server: " << errDescription << ". Exiting." << std::endl;
            exit(EXIT_FAILURE);
        }

	#ifndef _WIN32
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_sigaction = SignalHandler;
        act.sa_flags = SA_SIGINFO;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
	#endif
		
        std::cout << "SMS online validator started. See log files at LOG_DIR for further details" << std::endl;
        server.Run();
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() <<  ". Exiting." <<std::endl;
        logWriter << std::string(ex.what()) +  ". Exiting.";
    }
    logWriter << "Stopping.";
#ifdef _WIN32
	WSACleanup();
#endif
#ifndef _WIN32
    remove(pidFilename.c_str());
#endif
}
