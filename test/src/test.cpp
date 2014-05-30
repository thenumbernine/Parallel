#include <iostream>
#include "Parallel/Parallel.h"

int main() {
	
	std::vector<int> v;
	for (int i = 0; i < 100; ++i) {
		v.push_back(i%10);
	}

	for (int i = 0; i < 10; ++i) {
		for (int i : v) {
			Parallel::parallel->tasks.push_back([=](){
				std::cout << i;
			});
		}
		Parallel::parallel->doneMutex.lock();	//wait for last thread to finish then reacquire the mutex
		std::cout << "DONE!!!" << std::endl;
	}
}
