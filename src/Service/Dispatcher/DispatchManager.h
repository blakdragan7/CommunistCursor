#ifndef DISPATCH_MANAGER_H
#define DISPATCH_MANAGER_H

#include "DispatchQueue.h"

#include <map>
#include <vector>
#include <thread>
#include <mutex>

#define ANY_QUEUE -1

typedef std::map<unsigned int, DispatchQueue*> ConcurrentQueues;
typedef std::map<unsigned int, DispatchQueue*> SerialQueues;

class DispatchManager
{
private:
	std::vector<std::thread>	_threadPool;
	std::vector<DispatchQueue*>	_allQueues;
	ConcurrentQueues			_concurentQueues;
	SerialQueues				_serialQueues;

	bool						_threadsShouldRun;
	std::condition_variable	    _threadCondition;
	std::mutex					_threadLock;
	std::mutex					_queueLock;
	unsigned int				_lastConcurrentQueueID;
	unsigned int				_lastSerialQueueID;

private:
	DispatchQueue* GetConcurentQueueWithLeastJobs();

	bool HasAnyJob();
	bool GetAnyPendingQueue(DispatchQueue** queue);
	void DequeueThead();

	DispatchManager();
public:
	~DispatchManager();

	void SetupThreads(int threadNumber = 0);
	void TerminateAllThreads();

	unsigned int CreateConcurentQueue(std::string name);
	unsigned int CreateSerialQueue(std::string name);

	void DispatchConcurent(std::function<void(void)> job);
	void DispatchConcurent(unsigned int queueID, std::function<void(void)> job);
	void DispatchSerial(unsigned int queueID, std::function<void(void)> job);

	inline int NumberOfThreads()const { return (int)_threadPool.size(); }
	inline int NumberOfConcurentQueues()const { return (int)_concurentQueues.size(); }
	inline int NumberOfSerialQueues()const { return (int)_serialQueues.size(); }

	static DispatchManager manager;
};

#define DISPATCH_ASYNC(job)DispatchManager::manager.DispatchConcurent(job);
#define DISPATCH_ASYNC_ID(id, job)DispatchManager::manager.DispatchConcurent(id, job);
#define DISPATCH_ASYNC_SERIAL(id, job)DispatchManager::manager.DispatchSerial(id, job);

#endif