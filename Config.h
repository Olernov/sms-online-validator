#pragma once
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "LogWriter.h"

struct Config
{
public:
    Config();
    Config(std::ifstream& cfgStream);

    void ReadConfigFile(std::ifstream& cfgStream);
    void ValidateParams();
    std::string DumpAllSettings();

    unsigned int serverPort;
	std::string logDir;
    std::string connectString;
    unsigned short connectionCount;
    LogLevel logLevel;
private:
    const std::string logDirParamName = "LOG_DIR";
    const std::string connectStringParamName = "CONNECT_STRING";
    const std::string connectionCountParamName = "CONNECTION_COUNT";
    const std::string logLevelParamName = "LOG_LEVEL";
	const std::string serverPortParamName = "SERVER_PORT";
    const int minConnCount = 1;
    const int maxConnCount = 16;
    unsigned long ParseULongValue(const std::string& name, const std::string& value);
};
