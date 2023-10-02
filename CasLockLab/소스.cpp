#include <iostream>
#include <thread>
#include <memory>
#include <cstdio>
#include <cstdint>
#include <vector>

// CAS
// x86 -> 197x년 등장 
// 당시 멀티 스레딩 개념이 없음에도 ACE를 만든 이유 - lock 구현을 위함임
// volatile를 포함하여 후대에 사용법이 재발견 되었음
//

volatile int Sum = 0;
#define MAX_SUM 50000000
std::atomic<int> LOCK = 0;

bool CAS(std::atomic_int* addr, int expected, int new_val)
{
	return atomic_compare_exchange_strong(
		addr, &expected, new_val);
}

bool CasLock()
{
	while (!CAS(&LOCK, 0, 1)) std::this_thread::yield();
	/*int a = 0;
	while (!atomic_compare_exchange_strong(&LOCK, &a, 1)) { a = 0; }*/
	return true;
}

void CasUnLock()
{
	//CAS(&LOCK, 1, 0);
	LOCK = 0;
}

void threadFunc(int num_thID)
{
	for (int i = 0; i < MAX_SUM / num_thID; ++i) {
		CasLock();
		Sum = Sum + 2;
		CasUnLock();
	}
}

void main()
{
	std::vector<std::thread> threads;
	auto startTime = std::chrono::high_resolution_clock::now();
	auto endTime = startTime;

	for (int i = 1; i <= 32; i *= 2) {
		threads.clear();
		Sum = 0;
		for (int j = 0; j < i; ++j)
			threads.emplace_back(threadFunc, i);

		for (int j = 0; j < i; ++j)
			threads[j].join();
		endTime = std::chrono::high_resolution_clock::now();

		std::cout << "sum = " << Sum << "  Time = " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << std::endl;
	}

	return;
}