#include "DispatchQueue.h"
#include "../CC/CCLogger.h"

#include <assert.h>
#include <iostream>

DispatchQueue::DispatchQueue(bool isSerial) : _isSerial(isSerial), _isRunningJob(false)
{
}

DispatchQueue::DispatchQueue(bool isSerial, std::string name) : _isSerial(isSerial),
_name(name),  _isRunningJob(false)
{
}

//DispatchQueue::DispatchQueue(const DispatchQueue& other) : _isSerial(other._isSerial), _name(other._name),
//_isRunningJob(other._isRunningJob), _queue(other._queue)
//{
//
//}

void DispatchQueue::AddJob(DispatchJob& job)
{
	std::lock_guard<std::mutex> lock(_queueMutex);
#if _JOB_LOGGING
	job._owner = this;
	_queue.push_back({ job });
#else
	_queue.push_back({ job });
#endif
}

void DispatchQueue::AddJob(Duration future, std::function<void(void)> job)
{
	std::lock_guard<std::mutex> lock(_queueMutex);
#if _JOB_LOGGING
	_queue.push_back({ future, job, this });
#else
	_queue.push_back({ future, job });
#endif
}

#if _QUEUE_LOGGING
void DispatchQueue::AddJob(std::string file, std::string line, Duration future, std::function<void(void)> job)
{
	std::lock_guard<std::mutex> lock(_queueMutex);
#ifdef _DEBUG
	_queue.push_back({ file, line, future, job, this });
#else
	_queue.push_back({ file, line, future, job });
#endif
}
#endif

void DispatchQueue::RunJob(DispatchJob& job)
{
#if _JOB_LOGGING && _DEBUG
	assert(job._owner == this);
#endif

#if _QUEUE_LOGGING
	LOG_DEBUG << "Queue \"" << _name << "\" Performing {" << job._file << ":" << job._line << "}" << std::endl;
#endif // _QUEUE_LOGGING

	// run the job
	job();
	_isRunningJob = false;
}

bool DispatchQueue::GetRunnableJob(DispatchJob* outJob)
{
	if (_isSerial && _isRunningJob)
		return false;

	std::lock_guard<std::mutex> lock(_queueMutex);

	if (!_queue.empty())
	{
		auto itr = std::find_if(_queue.begin(), _queue.end(), 
			[](const DispatchJob& job) {return job.CanRun(); });

		if (itr != _queue.end())
		{
			*outJob = *itr;
			_queue.erase(itr);
			_isRunningJob = true;
			return true;
		}
		return false;
	}

	return false;
}
