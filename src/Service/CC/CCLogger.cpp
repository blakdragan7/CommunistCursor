#include "CCLogger.h"

CCLogger CCLogger::logger;

CCLogger::CCLogger() : _logLevel(LogLevel::Info), _funcLogLevel(LogLevel::Info), _ostream(std::cout)
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

void CCLogger::Error()
{
	_funcLogLevel = LogLevel::Error;
}
