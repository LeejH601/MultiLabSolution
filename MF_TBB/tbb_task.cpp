#include <tbb/task_group.h>

#include <tbb/parallel_for.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

int serial_fibo(int n)
{
	if (n < 2)
		return n;
	else
		return serial_fibo(n - 1) + serial_fibo(n - 2);
}

void mt_fibo(int n, int& result)
{
	if (n < 2) {
		result = n;
	}
	else
	{
		int x, y;
		std::thread t1{ mt_fibo, n - 1, std::ref(x) };
		std::thread t2{ mt_fibo, n - 2 ,std::ref(y) };
		t1.join();
		t2.join();
		result = x + y;
	}
}

int parallel_fib(int n)
{
	if (n < 30)
		return serial_fibo(n);
	else {
		int x, y;
		tbb::task_group g;
		g.run([&] {x = parallel_fib(n - 1); }); // spawn a task
		g.run([&] {y = parallel_fib(n - 2); }); // spawn another task
		g.wait(); // wait for both tasks to complete
		return x + y;
	}
}

int main()
{
	{
		std::atomic_int sum;
		sum = 0;
		auto start_time = std::chrono::system_clock::now();
		sum = serial_fibo(45);
		auto end_time = std::chrono::system_clock::now();
		auto exec_time = end_time - start_time;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_time).count();

		std::cout << "Single Thread Sum = " << sum << ",   " << ms << "ms." << std::endl;

		sum = 0;
	}

	//{
	//	int sum;
	//	sum = 0;
	//	auto start_time = std::chrono::system_clock::now();
	//	mt_fibo(45, sum);
	//	auto end_time = std::chrono::system_clock::now();
	//	auto exec_time = end_time - start_time;
	//	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_time).count();

	//	std::cout << "Single Thread Sum = " << sum << ",   " << ms << "ms." << std::endl;

	//	sum = 0;
	//}

	{
		std::atomic_int sum;
		sum = 0;
		auto start_time = std::chrono::system_clock::now();
		sum = parallel_fib(45);
		auto end_time = std::chrono::system_clock::now();
		auto exec_time = end_time - start_time;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_time).count();

		std::cout << "Single Thread Sum = " << sum << ",   " << ms << "ms." << std::endl;

		sum = 0;
	}
}

