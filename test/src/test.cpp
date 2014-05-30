#include "Parallel/Parallel.h"

int main() {
	
	std::vector<int> v;
	for (int i = 0; i < 10; ++i) {
		v.push_back(i%10);
	}

	//run 10 iterations
	for (int iter = 0; iter < 100; ++iter) {
		
		//before adding to queue, tell the children	
		{
			std::lock_guard<std::mutex> taskLock(Parallel::parallel->tasksMutex);
			for (char &flag : Parallel::parallel->needToUnlockDone) { flag = true; }
			for (int i : v) {
				Parallel::parallel->tasks.push_back([=](){
					for (int j = 0; j < 10; ++j) {
						std::cout << i;
					}
				});
			}
		}
		
		//wait for last thread to finish then reacquire the mutex
		for (int i = 0; i < Parallel::parallel->doneMutexes.size(); ++i) { 
			Parallel::parallel->doneMutexes[i].lock();
		}
		
		std::cout << "DONE!!!" << std::endl;
	}
}
