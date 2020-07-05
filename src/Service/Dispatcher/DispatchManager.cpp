#include "DispatchManager.h"

DispatchManager DispatchManager::manager;

DispatchQueue* DispatchManager::GetConcurentQueueWithLeastJobs()
{
	DispatchQueue* queue = nullptr;
	size_t numJobs = 0;

	std::lock_guard<std::mutex> lock(_queueLock);

	for (auto itr : _concurentQueues)
	{
		if (queue)
		{
			size_t otherJobs = itr.second->NumberOfJobs();
			
			if (numJobs < otherJobs)
			{
				queue = itr.second;
				numJobs = otherJobs;
				if (numJobs == 0)
				{
					break;
				}
			}
		}
		else
		{
			queue = itr.second;
			numJobs = queue->NumberOfJobs();
		}
	}

	return queue;
}

DispatchManager::DispatchManager() : _threadsShouldRun(false), _lastConcurrentQueueID(0), _lastSerialQueueID(0)
{
}

DispatchManager::~DispatchManager()
{
	if (_threadsShouldRun)
		TerminateAllThreads();

	_concurentQueues.clear();
	_serialQueues.clear();

	for (auto queue : _allQueues)
	{
		delete queue;
	}

	_allQueues.clear();
}

void DispatchManager::SetupThreads(int threadNumber)
{
	_threadsShouldRun = true;

	// we there will also be a main thread so we subtract 1 to get the number of background threads
	if (threadNumber == 0)
		threadNumber = std::thread::hardware_concurrency() - 1;

	for (int i = 0; i < threadNumber; i++)
	{
		_threadPool.push_back(std::thread(&DispatchManager::DequeueThead, this));
	}
}

bool DispatchManager::HasAnyJob()
{
	std::lock_guard<std::mutex> lock(_queueLock);
	for (auto& itr : _allQueues)
		if (itr->CanRunJob())
			return true;

	return false;
}

bool DispatchManager::GetAnyPendingQueue(DispatchQueue** queue)
{
	std::lock_guard<std::mutex> lock(_queueLock);
	for (auto& itr : _allQueues)
	{
		if (itr->CanRunJob())
		{
			*queue = itr;
			return true;
		}
	}

	return false;
}

void DispatchManager::DequeueThead()
{
	while (_threadsShouldRun)
	{
		DispatchQueue* queue = NULL;
		if (GetAnyPendingQueue(&queue))
		{
			queue->DequeueJob();
		}
		else
		{
			std::unique_lock<std::mutex> lock(_threadLock);
			_threadCondition.wait(lock, [this, &queue]() 
			{return GetAnyPendingQueue(&queue) || !_threadsShouldRun; });
			if (queue)queue->DequeueJob();
		}
	}
}

void DispatchManager::TerminateAllThreads()
{
	_threadsShouldRun = false;

	std::lock_guard<std::mutex> lock(_threadLock);
	_threadCondition.notify_all();
}

unsigned int DispatchManager::CreateConcurentQueue(std::string name)
{
	DispatchQueue* queue = new DispatchQueue(false, name);

	std::lock_guard<std::mutex> lock(_queueLock);
	_allQueues.push_back(queue);
	_concurentQueues.insert({ ++_lastConcurrentQueueID , queue });

	return _lastConcurrentQueueID;
}

unsigned int DispatchManager::CreateSerialQueue(std::string name)
{
	DispatchQueue* queue = new DispatchQueue(true, name);

	std::lock_guard<std::mutex> lock(_queueLock);
	_allQueues.push_back(queue);
	_serialQueues.insert({ ++_lastSerialQueueID , queue });

	return _lastSerialQueueID;
}

void DispatchManager::DispatchConcurent(std::function<void(void)> job)
{
	if (_allQueues.size() > 0)
	{
		DispatchQueue* queue = GetConcurentQueueWithLeastJobs();
		queue->AddJob(job);
	}
	else
	{
		DispatchQueue* queue = new DispatchQueue(false, "Default Concurent");
		queue->AddJob(job);

		std::lock_guard<std::mutex> lock(_queueLock);
		_allQueues.push_back(queue);
		_concurentQueues.insert({++_lastConcurrentQueueID, queue});
	}

	_threadCondition.notify_one();
}

void DispatchManager::DispatchConcurent(unsigned int queueID, std::function<void(void)> job)
{
	std::lock_guard<std::mutex> lock(_queueLock);

	auto itr = _concurentQueues.find(queueID);
	if (itr != _concurentQueues.end())
	{
		(*itr).second->AddJob(job);
	}
	else
	{
		_allQueues.push_back(new DispatchQueue(false, "Default Concurrent"));
		DispatchQueue* queue = _allQueues.back();

		_concurentQueues.insert({ queueID, queue });
		if (_lastConcurrentQueueID < queueID)
		{
			_lastConcurrentQueueID = queueID;
		}
	}

	_threadCondition.notify_one();
}

void DispatchManager::DispatchSerial(unsigned int queueID, std::function<void(void)> job)
{
	std::lock_guard<std::mutex> lock(_queueLock);

	auto itr = _serialQueues.find(queueID);
	if (itr != _serialQueues.end())
	{
		(*itr).second->AddJob(job);
	}
	else
	{
		_allQueues.push_back(new DispatchQueue(true, "Default Serial"));
		DispatchQueue* queue = _allQueues.back();

		_serialQueues.insert({ queueID, queue });
	}

	_threadCondition.notify_one();
}
