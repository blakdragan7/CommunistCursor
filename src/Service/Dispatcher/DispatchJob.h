#ifndef DISPATCH_JOB_H
#define DISPATCH_JOB_H

#include <functional>

struct DispatchJob
{
	// for now, this may change
	std::function<void(void)> _job;
	DispatchJob() : _job(0) {}
	DispatchJob(std::function<void(void)> job) : _job(job) {}

	void operator()()
	{_job();}
};

#endif