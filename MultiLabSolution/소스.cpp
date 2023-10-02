#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <atomic>

// volatile 릴리즈 최적화를 하지 않도록 설정
volatile long long sum = 0; 

//
//std::atomic<long long> sum = 0;

//constexpr int MAX_THREADS 32

// 배열을 이용한 최적화
//volatile long long sums[32];

struct NUM {
	alignas(64) volatile long long num;
};

NUM sums[32];

std::mutex myLock;

void thread_func(int num_threads)
{
	for (auto i = 0; i < 50000000 / num_threads; ++i) {
		myLock.lock();
		sum = sum + 2;
		myLock.unlock();
	}
}

void thread_func_noeLock(int num_threads) {
	for (auto i = 0; i < 50000000 / num_threads; ++i) {
		sum = sum + 2;
	}
}

void thread_func_Atomic(int num_threads)
{
	for (auto i = 0; i < 50000000 / num_threads; ++i) {
		sum += 2;
		// sum = sum + 2 와 갑이 전혀 다름
	}
}

void thread_func_optimal(int num_threads) {
	volatile long long acc = 0;
	for (int i = 0; i < 50000000 / num_threads; ++i) {
		acc += 2;
	}
	myLock.lock();
	sum += acc;
	myLock.unlock();
}

void thread_func_optimal2(int th_id, int num_threads) {
	for (int i = 0; i < 50000000 / num_threads; ++i) {
		sums[th_id].num += 2;
	}
}

int main() {

	char t = getchar();
	std::vector<std::thread> threads;


	//std::thread thread1{ thread_func };
	//std::thread thread2{ thread_func };

	//thread1.join();
	//thread2.join();

	/*thread_func();
	thread_func();*/

	threads.resize(128);
	
	while (true)
	{
		int nThreadCount;
		std::cout << "input ThreadNum :";
		std::cin >> nThreadCount;

		sum = 0;
		for (auto& n : sums)
			n.num = 0;

		threads.clear();

		auto startTime = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < nThreadCount; ++i) {
			threads.emplace_back(thread_func_optimal2, i, nThreadCount);
		}

		for (std::thread& thread : threads) {
			thread.join();
		}

		for (auto& n : sums)
			sum += n.num;

		auto endTime = std::chrono::high_resolution_clock::now();
		auto duration = endTime - startTime;

		std::cout << "sum = " << sum << std::endl;
		std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << " msecs\n";
	}
	

	return 0;
}