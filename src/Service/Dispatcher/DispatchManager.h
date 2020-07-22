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

	bool GetAnyPendingQueueWithJob(DispatchQueue** queue, DispatchJob* job);
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

	void DispatchConcurentAfter(Duration, std::function<void(void)> job);
	void DispatchConcurentAfter(Duration, unsigned int queueID, std::function<void(void)> job);
	void DispatchSerialAfter(Duration, unsigned int queueID, std::function<void(void)> job);

#if _QUEUE_LOGGING
	void DispatchConcurent(std::string file, std::string line, std::function<void(void)> job);
	void DispatchConcurent(std::string file, std::string line, unsigned int queueID, std::function<void(void)> job);
	void DispatchSerial(std::string file, std::string line, unsigned int queueID, std::function<void(void)> job);

	void DispatchConcurentAfter(std::string file, std::string line, Duration, std::function<void(void)> job);
	void DispatchConcurentAfter(std::string file, std::string line, Duration, unsigned int queueID, std::function<void(void)> job);
	void DispatchSerialAfter(std::string file, std::string line, Duration, unsigned int queueID, std::function<void(void)> job);
#endif

	inline int NumberOfThreads()const { return (int)_threadPool.size(); }
	inline int NumberOfConcurentQueues()const { return (int)_concurentQueues.size(); }
	inline int NumberOfSerialQueues()const { return (int)_serialQueues.size(); }

	static DispatchManager manager;
};

#if _QUEUE_LOGGING
#ifdef _WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
#define DISPATCH_ASYNC(job)DispatchManager::manager.DispatchConcurent(std::string(__FILENAME__), std::to_string(__LINE__), job);
#define DISPATCH_ASYNC_ID(id, job)DispatchManager::manager.DispatchConcurent(std::string(__FILENAME__), std::to_string(__LINE__), id, job);
#define DISPATCH_ASYNC_SERIAL(id, job)DispatchManager::manager.DispatchSerial(std::string(__FILENAME__), std::to_string(__LINE__), id, job);

#define DISPATCH_AFTER(time, job)DispatchManager::manager.DispatchConcurentAfter(std::string(__FILENAME__), std::to_string(__LINE__), time, job);
#define DISPATCH_AFTER_ID(time, id, job)DispatchManager::manager.DispatchConcurentAfter(std::string(__FILENAME__), std::to_string(__LINE__), time, id, job);
#define DISPATCH_AFTER_SERIAL(time, id, job)DispatchManager::manager.DispatchSerialAfter(std::string(__FILENAME__), std::to_string(__LINE__), time, id, job);

#else

#define DISPATCH_ASYNC(job)DispatchManager::manager.DispatchConcurent(job);
#define DISPATCH_ASYNC_ID(id, job)DispatchManager::manager.DispatchConcurent(id, job);
#define DISPATCH_ASYNC_SERIAL(id, job)DispatchManager::manager.DispatchSerial(id, job);

#define DISPATCH_AFTER(time, job)DispatchManager::manager.DispatchConcurentAfter(time, job);
#define DISPATCH_AFTER_ID(time, id, job)DispatchManager::manager.DispatchConcurentAfter(time, id, job);
#define DISPATCH_AFTER_SERIAL(time, id, job)DispatchManager::manager.DispatchSerialAfter(time, id, job);

#endif

#define CREATE_CONCURENT_QUEUE(name)DispatchManager::manager.CreateConcurentQueue(name)
#define CREATE_SERIAL_QUEUE(name)DispatchManager::manager.CreateSerialQueue(name)

#define INIT_DISPATCHER DispatchManager::manager.SetupThreads();
#define INIT_DISPATCHER_WITH_THREAD_COUNT(count) DispatchManager::manager.SetupThreads(count);

#define SHUTDOWN_DISPATCHER DispatchManager::manager.TerminateAllThreads();

#endif
