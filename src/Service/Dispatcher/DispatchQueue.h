#ifndef DISPATCH_QUEUE_H
#define DISPATCH_QUEUE_H

#include <mutex>
#include <queue>
#include <string>
#include "DispatchJob.h"

class DispatchQueue
{
private:
	std::queue<DispatchJob> _queue;
	std::mutex				_queueMutex;
	bool					_isSerial;
	bool					_isRunningJob;
	std::string				_name;

public:
	DispatchQueue(bool isSerial);
	DispatchQueue(bool isSerial, std::string name);
	DispatchQueue(const DispatchQueue&) = delete;
	DispatchQueue(DispatchQueue&&) = default;


	void AddJob(DispatchJob& job);
	void AddJob(std::function<void(void)> job);
	void DequeueJob();

	inline const size_t NumberOfJobs()const { return _queue.size(); }
	inline const bool IsSerial()const { return _isSerial; }
	inline const bool CanRunJob()const { return !_queue.empty() && !(_isSerial && _isRunningJob); }
};

#endif