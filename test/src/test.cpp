#include "Parallel/Parallel.h"
#include <vector>
#include <cmath>

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
		Parallel::Parallel parallel(8);

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
	{
		for (int n = 1; n <= 16; ++n) {
			double bestTime = std::numeric_limits<double>::infinity();
			for (int tries = 0; tries < 1000; ++tries) {
				Parallel::Parallel parallel(n);
				std::vector<double> v(10000);
				for (int i = 0; i < (int)v.size(); ++i) {
					v[i] = sin(.01 * i);
				}
				bestTime = std::min<double>(bestTime, time([&](){
					parallel.foreach(v.begin(), v.end(), [&](double &x) {
						for (int j = 0; j < 100; ++j) {
							x = 4. * x * (1. - x);
						}
					});
				}));
			}
			std::cout << "parallel " << n << " (" << bestTime << " s)" << std::endl;
		}
	}
}
