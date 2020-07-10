#include "DispatchManager.h"

#include "../CC/CCLogger.h"

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

	// There will also be a main thread so we subtract 1 to get the number of background threads
	if (threadNumber == 0)
		threadNumber = std::thread::hardware_concurrency() - 1;

	LOG_INFO << "Creating " << threadNumber << " threads for pool" << std::endl;

	for (int i = 0; i < threadNumber; i++)
	{
		_threadPool.push_back(std::thread(&DispatchManager::DequeueThead, this));
	}
}

bool DispatchManager::GetAnyPendingQueueWithJob(DispatchQueue** queue, DispatchJob* job)
{
	std::lock_guard<std::mutex> lock(_queueLock);
	for (auto& itr : _allQueues)
	{
		if (itr->GetRunnableJob(job))
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
		DispatchJob job;
		if (GetAnyPendingQueueWithJob(&queue, &job) == false)
		{
			std::unique_lock<std::mutex> lock(_threadLock);
			_threadCondition.wait_for(lock, std::chrono::milliseconds(30), [this, &queue, &job]() 
			{
				return GetAnyPendingQueueWithJob(&queue, &job) || !_threadsShouldRun; 
			});
		}

		if (queue)
		{
			queue->RunJob(job);
		}
	}
}

void DispatchManager::TerminateAllThreads()
{
	_threadsShouldRun = false;
	_threadCondition.notify_all();

	for (auto& thread : _threadPool)
	{
		if(thread.joinable())
			thread.join();
	}
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
	DispatchConcurentAfter(Duration(0), job);
}

void DispatchManager::DispatchConcurent(unsigned int queueID, std::function<void(void)> job)
{
	DispatchConcurentAfter(Duration(0), queueID, job);
}

void DispatchManager::DispatchSerial(unsigned int queueID, std::function<void(void)> job)
{
	DispatchSerialAfter(Duration(0), queueID, job);
}

void DispatchManager::DispatchConcurentAfter(Duration future, std::function<void(void)> job)
{
	if (_allQueues.size() > 0)
	{
		DispatchQueue* queue = GetConcurentQueueWithLeastJobs();
		if (queue == NULL)
		{
			queue = new DispatchQueue(false, "Default Concurent");

			std::lock_guard<std::mutex> lock(_queueLock);
			_allQueues.push_back(queue);
			_concurentQueues.insert({ ++_lastConcurrentQueueID, queue });
		}
		queue->AddJob(future, job);
	}
	else
	{
		DispatchQueue* queue = new DispatchQueue(false, "Default Concurent");
		queue->AddJob(future, job);

		std::lock_guard<std::mutex> lock(_queueLock);
		_allQueues.push_back(queue);
		_concurentQueues.insert({ ++_lastConcurrentQueueID, queue });
	}

	_threadCondition.notify_one();
}

void DispatchManager::DispatchConcurentAfter(Duration future, unsigned int queueID, std::function<void(void)> job)
{
	std::lock_guard<std::mutex> lock(_queueLock);

	auto itr = _concurentQueues.find(queueID);
	if (itr != _concurentQueues.end())
	{
		(*itr).second->AddJob(future, job);
	}
	else
	{
		DispatchQueue* queue = new DispatchQueue(false, "Default Concurrent");
		queue->AddJob(future, job);

		_allQueues.push_back(queue);
		_concurentQueues.insert({ queueID, queue });
		if (_lastConcurrentQueueID < queueID)
		{
			_lastConcurrentQueueID = queueID;
		}
	}

	_threadCondition.notify_one();
}

void DispatchManager::DispatchSerialAfter(Duration future, unsigned int queueID, std::function<void(void)> job)
{
	std::lock_guard<std::mutex> lock(_queueLock);

	auto itr = _serialQueues.find(queueID);
	if (itr != _serialQueues.end())
	{
		(*itr).second->AddJob(future, job);
	}
	else
	{
		DispatchQueue* queue = new DispatchQueue(true, "Default Serial");
		queue->AddJob(future, job);

		_allQueues.push_back(queue);
		_serialQueues.insert({ queueID, queue });
	}

	_threadCondition.notify_one();
}

#if _QUEUE_LOGGING

void DispatchManager::DispatchConcurent(std::string file, std::string line, std::function<void(void)> job)
{
	DispatchConcurentAfter(file, line, Duration(0), job);
}

void DispatchManager::DispatchConcurent(std::string file, std::string line, unsigned int queueID, std::function<void(void)> job)
{
	DispatchConcurentAfter(file, line, Duration(0), queueID, job);
}

void DispatchManager::DispatchSerial(std::string file, std::string line, unsigned int queueID, std::function<void(void)> job)
{
	DispatchSerialAfter(file, line, Duration(0), queueID, job);
}

void DispatchManager::DispatchConcurentAfter(std::string file, std::string line, Duration future, std::function<void(void)> job)
{
	if (_allQueues.size() > 0)
	{
		DispatchQueue* queue = GetConcurentQueueWithLeastJobs();
		if (queue == NULL)
		{
			queue = new DispatchQueue(false, "Default Concurent");

			std::lock_guard<std::mutex> lock(_queueLock);
			_allQueues.push_back(queue);
			_concurentQueues.insert({ ++_lastConcurrentQueueID, queue });
		}
		queue->AddJob(file,line, future, job);
	}
	else
	{
		DispatchQueue* queue = new DispatchQueue(false, "Default Concurent");
		queue->AddJob(file,line, future, job);

		std::lock_guard<std::mutex> lock(_queueLock);
		_allQueues.push_back(queue);
		_concurentQueues.insert({ ++_lastConcurrentQueueID, queue });
	}

	_threadCondition.notify_one();
}

void DispatchManager::DispatchConcurentAfter(std::string file, std::string line, Duration future, unsigned int queueID, std::function<void(void)> job)
{
	std::lock_guard<std::mutex> lock(_queueLock);

	auto itr = _concurentQueues.find(queueID);
	if (itr != _concurentQueues.end())
	{
		(*itr).second->AddJob(file, line, future, job);
	}
	else
	{
		DispatchQueue* queue = new DispatchQueue(false, "Default Concurrent");
		queue->AddJob(file, line, future, job);

		_allQueues.push_back(queue);
		_concurentQueues.insert({ queueID, queue });
		if (_lastConcurrentQueueID < queueID)
		{
			_lastConcurrentQueueID = queueID;
		}
	}

	_threadCondition.notify_one();
}

void DispatchManager::DispatchSerialAfter(std::string file, std::string line, Duration future, unsigned int queueID, std::function<void(void)> job)
{
	std::lock_guard<std::mutex> lock(_queueLock);

	auto itr = _serialQueues.find(queueID);
	if (itr != _serialQueues.end())
	{
		(*itr).second->AddJob(file,line, future, job);
	}
	else
	{
		DispatchQueue* queue = new DispatchQueue(true, "Default Serial");
		queue->AddJob(file, line, future, job);

		_allQueues.push_back(queue);
		_serialQueues.insert({ queueID, queue });
	}

	_threadCondition.notify_one();
}

#endif