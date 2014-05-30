#pragma once

#include "Common/Singleton.h"

#include <thread>
#include <functional>
#include <algorithm>
#include <vector>
#include <deque>
#include <list>

namespace Parallel {

template<int numThreads = 4>
struct ParallelCount {

	std::deque<std::function<void()>> tasks;
	std::vector<std::thread> workers;
	std::mutex tasksMutex;	//lock/unlock surrounding access of the 'tasks' deque
	std::mutex doneMutex;	//singleton acquires when filling task queue & blocks on.  last thread releases when emptying the task queue.
	std::mutex shutdownMutex;	//singleton acquires upon start - before thread creation - and releases upon exit.  threads acquire & release to know when to stop.

	ParallelCount() 
	: workers(numThreads)
	{
		shutdownMutex.lock();
		doneMutex.lock();
		for (int i = 0; i < numThreads; ++i) {
			workers[i] = std::thread([&](){
				while (true) {
					//upon 'done' signal, break
					if (shutdownMutex.try_lock()) {
						shutdownMutex.unlock();
						break;
					}

					//upon 'queue' signal ...
					std::function<void()> next;
					bool gotEmpty = false;
					{
						std::lock_guard<std::mutex> taskLock(tasksMutex);
						if (!tasks.empty()) {
							next = tasks.front();
							tasks.pop_front();
							//if we got the last one then say so
							if (tasks.empty()) {
								gotEmpty = true;
							}
						}
					}
					if (next) {
						next();
						if (gotEmpty) {
							//BUG: the last thread can still call this before another thread finishes its work
							// so this technically isn't fired when the queue is done...
							doneMutex.unlock();
						}
					}
				}
			});
		}
	}

	~ParallelCount() {
		shutdownMutex.unlock();
		for (std::thread &worker : workers) {
			worker.join();
		}
	}

	template<typename Iterator, typename Callback>
	void foreach(Iterator begin, Iterator end, Callback callback) {
		int totalRange = end - begin;
		std::thread threads[numThreads];
		for (int i = 0; i < numThreads; ++i) {
			int beginIndex = i * totalRange / numThreads;
			int endIndex = (i + 1) * totalRange / numThreads;
			threads[i] = std::thread([&,beginIndex,endIndex]() {
				std::for_each(begin + beginIndex, begin + endIndex, callback);
			});
		}
		for (int i = 0; i < numThreads; ++i) {
			threads[i].join();
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
		Combiner combiner = [&](Result a, Result b) -> Result { return a + b; })
	{
		int totalRange = end - begin;
		std::thread threads[numThreads];
		Result results[numThreads];
		for (int i = 0; i < numThreads; ++i) {
			int beginIndex = i * totalRange / numThreads;
			int endIndex = (i + 1) * totalRange / numThreads;
			threads[i] = std::thread([&,beginIndex,endIndex,i]() {
				results[i] = initialValue;
				std::for_each(begin + beginIndex, begin + endIndex, [&](typename std::iterator_traits<Iterator >::value_type &value) {
					results[i] = combiner(results[i], callback(value));
				});
			});
		}
		for (int i = 0; i < numThreads; ++i) {
			threads[i].join();
			initialValue = combiner(initialValue, results[i]);
		}
		
		return initialValue;
	}
};

typedef ParallelCount<4> Parallel;

extern Singleton<Parallel> parallel;

};
