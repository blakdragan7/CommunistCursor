#include "CCLogger.h"
#include <algorithm>
#include <string>
#include <cctype>

CCLogger CCLogger::logger;

CCLogger::CCLogger() : _logLevel(LogLevel::Info), _funcLogLevel(LogLevel::Debug), _ostream(std::cout)
{

}

void CCLogger::Info()
{
	_funcLogLevel = LogLevel::Info;
}

void CCLogger::Debug()
{
	_funcLogLevel = LogLevel::Debug;
}

void CCLogger::Warn()
{
	_funcLogLevel = LogLevel::Warn;
}

void CCLogger::Error()
{
	_funcLogLevel = LogLevel::Error;
}

void CCLogger::SetLogLevel(std::string level)
{
	std::string orig = level;
	std::transform(level.begin(), level.end(), level.begin(),
		[](unsigned char c) { return std::tolower(c); });
	if (level == "debug")
	{
		SetLogLevel(LogLevel::Debug);
	}
	else if (level == "info")
	{
		SetLogLevel(LogLevel::Info);
	}
	else if (level == "warn")
	{
		SetLogLevel(LogLevel::Warn);
	}
	else if (level == "error")
	{
		SetLogLevel(LogLevel::Error);
	}
	else if (level == "none")
	{
		SetLogLevel(LogLevel::None);
	}
	else
	{
		std::string errorStr = "Invalid Log Level Given " + orig;
		throw std::runtime_error(errorStr);
	}
}

std::string CCLogger::GetLogLevelAsString() const
{
	switch (_logLevel)
	{
	case LogLevel::Debug:
		return "Debug";
	case LogLevel::Info:
		return "Info";
	case LogLevel::Warn:
		return "Warn";
	case LogLevel::Error:
		return "Error";
	case LogLevel::None:
		return "None";
	default:
		return "Invalid";
	}
}
