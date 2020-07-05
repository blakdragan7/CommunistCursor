#include "DispatchQueue.h"

DispatchQueue::DispatchQueue(bool isSerial) : _isSerial(isSerial), _isRunningJob(false)
{
}

DispatchQueue::DispatchQueue(bool isSerial, std::string name) : _isSerial(isSerial), _name(name),  _isRunningJob(false)
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
	_queue.push(job);
}

void DispatchQueue::AddJob(std::function<void(void)> job)
{
	std::lock_guard<std::mutex> lock(_queueMutex);
	_queue.push(job);
}

void DispatchQueue::DequeueJob()
{
	DispatchJob job;
	{ // we only want to lock accessing the que
		std::lock_guard<std::mutex> lock(_queueMutex);

		// is there anything to do
		if (_queue.empty())
			return;

		_isRunningJob = true;

		job = _queue.front();
		_queue.pop();
	}

	job();

	_isRunningJob = false;
}
