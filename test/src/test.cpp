#include "Parallel/Parallel.h"
#include <vector>

int main() {

	for (int iter = 0; iter < 100; ++iter) {
		std::vector<int> v;
		for (int i = 0; i < 100; ++i) {
			v.push_back(i % 10);
		}
		Parallel::parallel->foreach(v.begin(), v.end(), [&](int i) {
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
		long result = Parallel::parallel->reduce(
			v.begin(), 
			v.end(), 
			[&](long &i) -> long
			{
				return i * i; 
			},
			0
		);
		long verify = n * (n + 1) * (2 * n + 1) / 6;
		std::cout << "result " << result << " should be " << verify << " " << (verify == result ? "MATCH!" : "DIFFER!") << std::endl;
	}
}
