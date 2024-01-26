#include <tbb/parallel_for.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

using namespace std;
using namespace tbb;
using namespace chrono;


int main()
{
	{
		atomic_int sum;
		sum = 0;
		auto start_time = system_clock::now();
		for (int i = 0; i < 50000000; ++i)
			sum += 2;
		auto end_time = system_clock::now();
		auto exec_time = end_time - start_time;
		auto ms = duration_cast<milliseconds>(exec_time).count();

		cout << "Single Thread Sum = " << sum << ",   " << ms << "ms." << endl;

		sum = 0;
	}

	{
		auto num_thread = 8;
		atomic_int sum;
		sum = 0;
		vector<thread> threads;
		auto start_time = system_clock::now();
		for (int i = 0; i < num_thread; ++i)
			threads.emplace_back([&sum, num_thread]() {
			for (int i = 0; i < 50000000 / num_thread; ++i)
				sum += 2;
				});

		for (auto& th : threads)
			th.join();
		auto end_time = system_clock::now();
		auto exec_time = end_time - start_time;
		auto ms = duration_cast<milliseconds>(exec_time).count();

		cout << "Mult Thread Sum = " << sum << ",   " << ms << "ms." << endl;

		sum = 0;
	}

	{
		auto num_thread = 8;
		atomic_int sum;
		sum = 0;
		auto start_time = system_clock::now();
		parallel_for(0, 50000000, 1, [&sum, num_thread](int i) {
			sum += 2;
			});

		auto end_time = system_clock::now();
		auto exec_time = end_time - start_time;
		auto ms = duration_cast<milliseconds>(exec_time).count();

		cout << "TBB Sum = " << sum << ",   " << ms << "ms." << endl;

		sum = 0;
	}
}
