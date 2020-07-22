#ifndef DISPATCH_JOB_H
#define DISPATCH_JOB_H

#include <functional>
#include <chrono>

#include "../CC/CCLogger.h"

// force job logging on debug
#ifndef _JOB_LOGGING
	#ifdef _DEBUG
		#define _JOB_LOGGING 1
	#else 
		#define _JOB_LOGGING 0
	#endif // _DEBUG
#endif // _QUEUE_LOGGING

// force queue logging on debug
#ifndef _QUEUE_LOGGING
#ifdef _DEBUG
#define _QUEUE_LOGGING 1
#else 
#define _QUEUE_LOGGING 0
#endif // _DEBUG
#endif // _QUEUE_LOGGING

#define TIME_NOW std::chrono::system_clock::now()

typedef std::chrono::system_clock::time_point TimePoint;
typedef std::chrono::system_clock::duration Duration;
#ifdef _DEBUG
class DispatchQueue;
#endif
class DispatchJob
{
	// for now, this may change
	std::function<void(void)>	_job;
	TimePoint					_runnablePoint;

#if _JOB_LOGGING
	DispatchQueue*				_owner;
	TimePoint					_creation;
#endif // _JOB_LOGGING

#if _QUEUE_LOGGING
	std::string					_file;
	std::string					_line;
#endif // _QUEUE_LOGGING

	void operator()()
	{
#if _JOB_LOGGING
		typedef std::chrono::duration<float> fsec;
		LOG_DEBUG << "Took " << std::chrono::duration_cast<fsec>(TIME_NOW - _creation).count() << " seconds for job to dequeue" << std::endl;
#endif // _DEBUG

		_job();
	}

public:

#if _JOB_LOGGING
	DispatchJob() : _owner(0), _creation(TIME_NOW), _job(0), _runnablePoint(TIME_NOW) {}
	DispatchJob(DispatchQueue* owner) : _owner(owner), _creation(TIME_NOW), _job(0), _runnablePoint(TIME_NOW) {}
	DispatchJob(std::function<void(void)> job, DispatchQueue* owner) : _owner(owner), _creation(TIME_NOW), _runnablePoint(TIME_NOW), _job(job) {}
	DispatchJob(Duration waitFor, std::function<void(void)> job, DispatchQueue* owner) : _owner(owner), _creation(TIME_NOW), _job(job), _runnablePoint(TIME_NOW + waitFor) {}
#if _QUEUE_LOGGING
	DispatchJob(std::string file, std::string line, Duration waitFor, std::function<void(void)> job, DispatchQueue* owner) : _owner(owner), _file(file), _creation(TIME_NOW), _line(line), _job(job), _runnablePoint(TIME_NOW + waitFor) {}
#endif // _QUEUE_LOGGING
#else
	DispatchJob() : _job(0), _runnablePoint(TIME_NOW) {}
	DispatchJob(std::function<void(void)> job) : _runnablePoint(TIME_NOW), _job(job) {}
	DispatchJob(Duration waitFor, std::function<void(void)> job) : _job(job), _runnablePoint(TIME_NOW + waitFor) {}
#if _QUEUE_LOGGING
	DispatchJob(std::string file, std::string line, Duration waitFor, std::function<void(void)> job) : _file(file), _line(line), _job(job), _runnablePoint(TIME_NOW + waitFor) {}
#endif // _QUEUE_LOGGING
#endif // _JOB_LOGGING

	inline bool CanRun()const { return _runnablePoint <= TIME_NOW; }

	friend class DispatchQueue;
};

#endif
