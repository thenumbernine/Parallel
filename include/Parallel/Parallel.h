#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <functional>
#include <algorithm>
#include <vector>
#include <deque>
#include <list>
#include "Common/semaphore"

namespace Parallel {

struct Parallel {
protected:
	//how many threads to divide ourselves among
	size_t numThreads;
	
	//deque of tasks
	std::deque<std::function<void()>> tasks;
	
	//vector of worker threads
	std::vector<std::thread> workers;
	
	//one per thread ... ?  semaphore instead?
	std::vector<Common::semaphore> doneSempahores;
	
	//flag whether each thread needs to unlock their doneMutex if it finds an empty queue.
	//set by singleton before populating tasks, cleared by each thread once an empty queue is found.
	//access is protected by taskMutex.
	std::vector<char> needToUnlockDone;	
	
	//lock/unlock surrounding access of the 'tasks' deque
	// and surrounding access of 'needToUnlock'
	std::mutex tasksMutex;

	//singleton acquires when filling task queue & blocks on.  
	//last thread releases when emptying the task queue.
	std::mutex doneMutex;	

	bool done;
public:
	Parallel(size_t numThreads_ = std::thread::hardware_concurrency())
	: numThreads(numThreads_)
	, workers(numThreads)
	, doneSempahores(numThreads)
	, needToUnlockDone(numThreads)
	, done(false)
	{
		for (size_t i = 0; i < numThreads; ++i) {
			needToUnlockDone[i] = false;
		}
		for (size_t i = 0; i < numThreads; ++i) {
			workers[i] = std::thread([&,i](){
				while (true) {

					//upon 'queue' signal ...
					std::function<void()> next;
					bool gotEmpty = false;
					{
						std::lock_guard<std::mutex> taskLock(tasksMutex);
						if (done) return;
						if (!tasks.empty()) {
							next = tasks.front();
							tasks.pop_front();
						}
						//if we got the last one then say so
						if (tasks.empty() && needToUnlockDone[i]) {
							needToUnlockDone[i] = false;
							gotEmpty = true;
						}
					}
					if (next) {
						next();
					}
					if (gotEmpty) {
						doneSempahores[i].notify();
					}
				}
			});
		}
	}

	~Parallel() {
		done = true;	//protect this if you want
		for (std::thread &worker : workers) {
			worker.join();
		}
	}

	template<typename Iterator, typename Callback>
	void foreach(Iterator begin, Iterator end, Callback callback) {
		//spawn
		{
			//prep
			std::lock_guard<std::mutex> taskLock(tasksMutex);		//acquire task mutex
			for (char &flag : needToUnlockDone) { flag = true; }	//clear all thread done flags
		
			//add threads
			size_t totalRange = end - begin;
			for (size_t i = 0; i < numThreads; ++i) {
				size_t beginIndex = i * totalRange / numThreads;
				size_t endIndex = (i + 1) * totalRange / numThreads;
//std::cout << "thread " << i << " running from " << beginIndex << " to " << endIndex << std::endl;		
				tasks.push_back([=]() {
					std::for_each(begin + beginIndex, begin + endIndex, callback);
				});
			}
		}

		//wait for the threads to finish
		for (size_t i = 0; i < doneSempahores.size(); ++i) {
			doneSempahores[i].wait();
		}
	};

	// a shy step away from std::accumulate
	// in that the values in the iterator are mapped first (via callback)
	// before they are accumulated.
	// I could make a new structure for buffering my intermediate values, but I don't really want to.
	template<
		typename Iterator, 
		typename Result, 
		typename Callback = std::function<Result (typename std::iterator_traits<Iterator>::value_type &)>,
		typename Combiner = std::function<Result (Result, Result)>
	>
	Result reduce(
		Iterator begin, 
		Iterator end, 
		Callback callback,
		Result initialValue = Result(),
		Combiner combiner = std::plus<Result>())
	{
		std::vector<Result> results(numThreads);
		
		//spawn
		{
			std::lock_guard<std::mutex> taskLock(tasksMutex);		//acquire task mutex
			for (char &flag : needToUnlockDone) { flag = true; }	//clear all thread done flags
		
			size_t totalRange = end - begin;
			for (size_t i = 0; i < numThreads; ++i) {
				size_t beginIndex = i * totalRange / numThreads;
				size_t endIndex = (i + 1) * totalRange / numThreads;
				tasks.push_back([&,beginIndex,endIndex,i]() {
					results[i] = initialValue;
					std::for_each(begin + beginIndex, begin + endIndex, [&](
						typename std::iterator_traits<Iterator>::value_type &value) 
					{
						results[i] = combiner(results[i], callback(value));
					});
				});
			}
		}

		//join
		for (size_t i = 0; i < doneSempahores.size(); ++i) {
			doneSempahores[i].wait();
		}

		//combine result
		for (size_t i = 0; i < numThreads; ++i) {
			initialValue = combiner(initialValue, results[i]);
		}
		
		return initialValue;
	}

	size_t getNumThreads() const { return numThreads; }
};

}
