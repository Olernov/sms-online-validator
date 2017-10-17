#pragma once
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "LogWriterOtl.h"

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
    std::string kafkaBroker;
    std::string kafkaTopicSms;
    std::string kafkaTopicCalls;
    std::string kafkaTopicTest;
private:
    const std::string logDirParamName = "LOG_DIR";
    const std::string connectStringParamName = "CONNECT_STRING";
    const std::string connectionCountParamName = "CONNECTION_COUNT";
    const std::string logLevelParamName = "LOG_LEVEL";
	const std::string serverPortParamName = "SERVER_PORT";
    const std::string kafkaBrokerParamName = "KAFKA_BROKER";
    const std::string kafkaTopicSmsParamName = "KAFKA_TOPIC_SMS";
    const std::string kafkaTopicCallsParamName = "KAFKA_TOPIC_CALLS";
    const int minConnCount = 1;
    const int maxConnCount = 16;
    unsigned long ParseULongValue(const std::string& name, const std::string& value);
};
