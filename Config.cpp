#include "Config.h"
#include "Common.h"

Config::Config() :
    serverPort(5300),
    connectionCount(2),
    kafkaTopic("sms-events"),
    logLevel(notice)
{
}


Config::Config(std::ifstream& configStream) :
    Config()
{
    ReadConfigFile(configStream);
}


void Config::ReadConfigFile(std::ifstream& configStream)
{
    std::string line;
    while (getline(configStream, line))
	{
		size_t pos = line.find_first_not_of(" \t\r\n");
        if (pos != std::string::npos) {
            if (line[pos] == '#' || line[pos] == '\0') {
				continue;
            }
        }
		size_t delim_pos = line.find_first_of(" \t=", pos);
        std::string option_name;
        if (delim_pos != std::string::npos) {
			option_name = line.substr(pos, delim_pos - pos);
        }
        else {
			option_name = line;
        }
		
        std::transform(option_name.begin(), option_name.end(), option_name.begin(), ::toupper);

		size_t value_pos = line.find_first_not_of(" \t=", delim_pos);
        std::string option_value;
        if (value_pos != std::string::npos) {
			option_value = line.substr(value_pos);
			size_t comment_pos = option_value.find_first_of(" \t#");
            if (comment_pos != std::string::npos)
				option_value = option_value.substr(0, comment_pos);
		}

        if (option_name == serverPortParamName) {
            serverPort = ParseULongValue(option_name, option_value);
        }
        else if (option_name == logDirParamName) {
            logDir = option_value;
        }
        else if (option_name == connectStringParamName) {
            connectString = option_value;
        }
        else if (option_name == connectionCountParamName) {
            connectionCount = ParseULongValue(option_name, option_value);
        }
        else if (option_name == logLevelParamName) {
            if (option_value == "error") {
                logLevel = error;
            }
            else if (option_value == "notice") {
                logLevel = notice;
            }
            else if (option_value == "debug") {
                logLevel = debug;
            }
            else {
                throw std::runtime_error("Wrong value passed for " + option_name + ".");
            }
        }
        else if (option_name == kafkaBrokerParamName) {
            kafkaBroker = option_value;
        }
        else if (option_name == kafkaTopicParamName) {
            kafkaTopic = option_value;
        }
        else if (!option_name.empty()){
            throw std::runtime_error("Unknown parameter " + option_name + " found");
        }
	}	
}


unsigned long Config::ParseULongValue(const std::string& name, const std::string& value)
{
    try {
        return std::stoul(value);
    }
    catch(const std::invalid_argument&) {
        throw std::runtime_error("Wrong value given for numeric config parameter " + name);
    }
}

void Config::ValidateParams()
{
    if (connectString.empty()) {
        throw std::runtime_error(connectStringParamName + " parameter is not set.");
    }
    if (!(connectionCount >= minConnCount && connectionCount <= maxConnCount)) {
        throw std::runtime_error(connectionCountParamName + " must have value from " +
                                 std::to_string(minConnCount) + " to " + std::to_string(maxConnCount));
    }
}

std::string Config::DumpAllSettings()
{
    return serverPortParamName + ": " + std::to_string(serverPort) + crlf +
		logDirParamName + ": " + logDir + crlf +
		connectionCountParamName + ": " + std::to_string(connectionCount) + crlf +
        kafkaBrokerParamName + ": " + kafkaBroker + crlf +
        kafkaTopicParamName + ": " + kafkaTopic + crlf +
       logLevelParamName + ": " + (logLevel == error ? "error" : (logLevel == debug ? "debug" : "notice")) + crlf;
}

