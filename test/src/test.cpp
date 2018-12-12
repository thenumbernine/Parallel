#include "Parallel/Parallel.h"
#include <vector>
#include <cmath>
#include <future>

//this is here and in EinsteinFIeldEquationSolution
//maybe put it in Common?
double time(std::function<void()> f) {
	auto start = std::chrono::high_resolution_clock::now();
	f();
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = end - start;
	return diff.count();
}

int main() {

	//validity tests
	{
		Parallel::Parallel parallel;

		for (int iter = 0; iter < 100; ++iter) {
			std::vector<int> v;
			for (int i = 0; i < 100; ++i) {
				v.push_back(i % 10);
			}
			parallel.foreach(v.begin(), v.end(), [&](int i) {
				std::cout << i;
			});
		
			std::cout << " DONE!!!" << std::endl;
		}

		std::cout << std::endl;

		//combine all numbers squared from 1 to 10000
		for (int iter = 0; iter < 100; ++iter) {
			long n = 1000;	//don't get overflows
			std::vector<long> v;
			for (long i = 0; i < n; ++i) {
				v.push_back(i+1);
			}
			long result = parallel.reduce(
				v.begin(), 
				v.end(), 
				[&](long& i) -> long { return i * i; },
				0
			);
			long verify = n * (n + 1) * (2 * n + 1) / 6;
			std::cout << "result " << result << " should be " << verify << " " << (verify == result ? "MATCH!" : "DIFFER!") << std::endl;
		}
	}

	//performance tests
	const int maxtries = 100;
	const int vectorsize = 100000;
	{
		// now test sequential
		{
			double minTime = std::numeric_limits<double>::infinity();
			double maxTime = -std::numeric_limits<double>::infinity();
			double avgTime = 0;
			for (int tries = 0; tries < maxtries; ++tries) {
				std::vector<double> v(vectorsize);
				for (int i = 0; i < (int)v.size(); ++i) {
					v[i] = sin(.01 * i);
				}	
			
				double dt = time([&](){
					for (double& x : v) {
						for (int j = 0; j < 100; ++j) {
							x = 4. * x * (1. - x);
						}
					}
				});
				minTime = std::min<double>(minTime, dt);
				maxTime = std::max<double>(maxTime, dt);
				avgTime += dt;
			}
			avgTime /= (double)maxtries;
			std::cout << "sequential\t" << minTime << "\t" << avgTime << "\t" << maxTime << std::endl;
		}

		// my thread pool code
		for (size_t n = 1; n <= std::thread::hardware_concurrency(); ++n) {
			double minTime = std::numeric_limits<double>::infinity();
			double maxTime = -std::numeric_limits<double>::infinity();
			double avgTime = 0;
			for (int tries = 0; tries < maxtries; ++tries) {
				Parallel::Parallel parallel(n);
				
				std::vector<double> v(vectorsize);
				for (int i = 0; i < (int)v.size(); ++i) {
					v[i] = sin(.01 * i);
				}
				
				double dt = time([&](){
					parallel.foreach(v.begin(), v.end(), [&](double &x) {
						for (int j = 0; j < 100; ++j) {
							x = 4. * x * (1. - x);
						}
					});
				});
				minTime = std::min<double>(minTime, dt);
				maxTime = std::max<double>(maxTime, dt);
				avgTime += dt;
			}
			avgTime /= (double)maxtries;
			std::cout << "parallel_" << n << "\t" << minTime << "\t" << avgTime << "\t" << maxTime << std::endl;
		}
	
		// std::async thread pool code
		for (size_t n = 1; n <= std::thread::hardware_concurrency(); ++n) {
			
			double minTime = std::numeric_limits<double>::infinity();
			double maxTime = -std::numeric_limits<double>::infinity();
			double avgTime = 0;
			for (int tries = 0; tries < maxtries; ++tries) {
				std::vector<double> v(vectorsize);
				for (int i = 0; i < (int)v.size(); ++i) {
					v[i] = sin(.01 * i);
				}	
			
				double dt = time([&](){

					auto begin = v.begin();
					auto end = v.end();
					size_t size = end - begin;
					std::vector<std::future<bool>> handles(n);
					for (size_t i = 0; i < n; ++i) {
						handles[i] = std::async(
							std::launch::async,
							[](auto subbegin, auto subend) -> bool {
								
								// body
								for (auto iter = subbegin; iter != subend; ++iter) {
									double& x = *iter;
									for (int j = 0; j < 100; ++j) {
										x = 4. * x * (1. - x);
									}
								}
								
								return true;
							}, 
							begin + i * size / n, 
							begin + (i+1) * size / n
						);
					}

					for (auto& h : handles) {
						h.get();
					}
					
				});
				minTime = std::min<double>(minTime, dt);
				maxTime = std::max<double>(maxTime, dt);
				avgTime += dt;
			}
			avgTime /= (double)maxtries;
			std::cout << "std::async_" << n << "\t" << minTime << "\t" << avgTime << "\t" << maxTime << std::endl;
		}
	}
}
