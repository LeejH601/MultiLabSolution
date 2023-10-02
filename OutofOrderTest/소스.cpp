#include <iostream>
#include <thread>
#include <atomic>

const auto SIZE = 50000000;
std::atomic<int> x, y;
int trace_x[SIZE], trace_y[SIZE];

void ThreadFunc0() {
	for (int i = 0; i < SIZE; ++i) {
		x = i;
		//std::atomic_thread_fence(std::memory_order_seq_cst);
		trace_y[i] = y;
	}
}

void ThreadFunc1() {
	for (int i = 0; i < SIZE; ++i) {
		y = i;
		//std::atomic_thread_fence(std::memory_order_seq_cst);
		trace_x[i] = x;
	}
}

volatile bool done = false;
volatile int* bound;
int g_error;



void ThreadFunc2() {
	for (int j = 0; j <= 25000000; ++j)
		*bound = -(1 + *bound);
	done = true;
}

void ThreadFunc3() {
	while (!done)
	{
		int v = *bound;
		if ((v != 0) && (v != -1)) {
			printf("%8X, ", v);
			g_error++;
		}
	}
}

int main() {
	/*
	std::thread t0{ ThreadFunc0 };
	std::thread t1{ ThreadFunc1 };

	t0.join();
	t1.join();

	int count = 0;
	for (int i = 0; i < SIZE; ++i) {
		if (trace_x[i] == trace_x[i + 1])
			if (trace_y[trace_x[i]] == trace_y[trace_x[i] + 1]) {
				if (trace_y[trace_x[i]] != i) continue;
				count++;
			}
	}
	std::cout << "Total Memory Inconsistency: " << count << std::endl;
	*/

	/*int num = 0;
	bound = &num;*/

	int ARR[32];
	
	long long temp = (long long)&ARR[16];
	temp = (temp / 64) * 64;
	temp -= 2; // 캐시 라인을 일부러 비틈
	// Cache Line Size Boundary
	bound = (int*)temp;
	*bound = 0;

	std::thread t2{ ThreadFunc2 };
	std::thread t3{ ThreadFunc3 };
	t2.join();
	t3.join();

	std::cout << "error : " << g_error << std::endl;
}