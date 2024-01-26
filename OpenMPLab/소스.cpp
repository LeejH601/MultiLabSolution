#include <omp.h>
#include <stdio.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>

volatile int sum = 0;
std::mutex lock;

#define CHUNKSIZE 100
#define N 1000

int main2()
{
	int nthreads, tid;
	/* Fork a team of threads with each thread having a private tid variable */
	auto start_time = std::chrono::system_clock::now();
#pragma omp parallel private(tid)
	{
		for (int i = 0; i < 50000000 / omp_get_num_threads(); ++i) {
#pragma omp critical
			sum = sum + 2;
		}
	} /* All threads join master thread and terminate */
	auto end_time = std::chrono::system_clock::now();
	auto exec_time = end_time - start_time;
	int ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_time).count();

	std::cout << "OpenMP SUM = " << sum << ", " << ms << std::endl;

	std::vector<std::thread> threads;
	sum = 0;
	start_time = std::chrono::system_clock::now();
	for (int i = 0; i < 16; ++i) {
		threads.emplace_back([]() {
			for (int i = 0; i < 50000000 / 16; ++i) {
				lock.lock();
				sum = sum + 2;
				lock.unlock();
			}
			});
	}
	for (int i = 0; i < threads.size(); ++i) {
		threads[i].join();
	}
	end_time = std::chrono::system_clock::now();
	exec_time = end_time - start_time;
	ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_time).count();

	std::cout << "STD SUM = " << sum << ", " << ms << std::endl;

};



int main()
{
	int i;
	int chunk = 50000;

	auto start_time = std::chrono::system_clock::now();
	{
#pragma omp for schedule(dynamic,chunk) nowait
		for (int i = 0; i < 50000000 / omp_get_num_threads(); ++i) {
#pragma omp critical
			sum = sum + 2;
		}
	} /* end of parallel section */

	auto end_time = std::chrono::system_clock::now();
	auto exec_time = end_time - start_time;
	int ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_time).count();

	std::cout << "OpenMP SUM = " << sum << ", " << ms << std::endl;


	std::vector<std::thread> threads;
	sum = 0;
	start_time = std::chrono::system_clock::now();
	for (int i = 0; i < 16; ++i) {
		threads.emplace_back([]() {
			for (int i = 0; i < 50000000 / 16; ++i) {
				lock.lock();
				sum = sum + 2;
				lock.unlock();
			}
			});
	}
	for (int i = 0; i < threads.size(); ++i) {
		threads[i].join();
	}
	end_time = std::chrono::system_clock::now();
	exec_time = end_time - start_time;
	ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_time).count();

	std::cout << "STD SUM = " << sum << ", " << ms << std::endl;
}