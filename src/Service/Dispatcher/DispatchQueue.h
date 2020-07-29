#ifndef DISPATCH_QUEUE_H
#define DISPATCH_QUEUE_H

#include <mutex>
#include <vector>
#include <string>
#include "DispatchJob.h"

typedef std::vector<DispatchJob>::iterator QueueIterator;

class DispatchQueue
{
private:
	std::vector<DispatchJob>	_queue;
	DispatchJob  				_runnableJob;
	std::mutex					_queueMutex;
	bool						_isSerial;
	bool						_isRunningJob;
	std::string					_name;

public:
	DispatchQueue(bool isSerial);
	DispatchQueue(bool isSerial, std::string name);
	DispatchQueue(const DispatchQueue&) = delete;


	void AddJob(DispatchJob& job);
	void AddJob(Duration future, std::function<void(void)> job);
#if _QUEUE_LOGGING
	void AddJob(std::string file, std::string line, Duration future, std::function<void(void)> job);
#endif
	

	inline const size_t NumberOfJobs()const { return _queue.size(); }
	inline const bool IsSerial()const { return _isSerial; }
	inline const std::string Name()const { return _name; }
	/*
		This returns true if a viable job was found or false otherwise
		If a job was found, {*outJob} is set to that job and an internal flag is set
		This assumes you are going to call RunJob after this call.
		Note if this is a serial queue then this will ALWAYS return false
		until RunJob is called with {*outJob} if this returns true
	*/
	bool GetRunnableJob(DispatchJob* outJob);
	/*
		This runs the job passed in and outputs some useful debug info
		The job passed in must be retrieved from GetRunnableJob

		This MUST be called after GetRunnableJob returns true
	*/
	void RunJob(DispatchJob& job);
};

#endif
