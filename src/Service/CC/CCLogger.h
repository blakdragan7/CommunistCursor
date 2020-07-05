#ifndef CC_LOGGER_H
#define CC_LOGGER_H
#include <iostream>

enum class LogLevel :int
{
	Info =	3,
	Debug = 2,
	Error = 1,
	None =	0
};

class CCLogger
{
private:
	LogLevel _logLevel;
	LogLevel _funcLogLevel;
	std::ostream& _ostream;


	CCLogger();
public:

	void Info();
	void Debug();
	void Error();

	template<typename t>
	CCLogger& operator <<(const t& other)
	{
		if(_funcLogLevel <= _logLevel)
			_ostream << other;

		return *this;
	}
	typedef std::ostream& (*StandardEndLine)(std::ostream&);
	CCLogger& operator <<(StandardEndLine endl)
	{
		if (_funcLogLevel <= _logLevel)
			endl(_ostream);

		return *this;
	}

	inline void SetLogLevel(LogLevel level)
	{_logLevel = level;}

	static CCLogger logger;
};

#define LOG_INFO CCLogger::logger.Info();CCLogger::logger
#define LOG_DEBUG CCLogger::logger.Debug();CCLogger::logger
#define LOG_ERROR CCLogger::logger.Error();CCLogger::logger

#endif