//#pragma warning(2:4235)
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <atomic>

class Bakery
{
	bool* volatile m_Flags = nullptr;
	int* volatile  m_Labels = nullptr;
	int size = 0;

public:
	Bakery() = default;
	Bakery(int n) {
		size = n;
		m_Flags = new bool[n];
		m_Labels = new int[n];
		for (int i = 0; i < n; ++i) {
			m_Flags[i] = false;
			m_Labels[i] = 0;
		}
	}
	Bakery(const Bakery& other) {

		size = other.size;
		m_Flags = new bool[size];
		m_Labels = new int[size];
		for (int i = 0; i < size; ++i) {
			m_Flags[i] = false;
			m_Labels[i] = 0;
		}
	}
	Bakery& operator=(const Bakery& other) {
		size = other.size;
		m_Flags = new bool[size];
		m_Labels = new int[size];
		for (int i = 0; i < size; ++i) {
			m_Flags[i] = false;
			m_Labels[i] = 0;
		}

		return *this;
	}

	~Bakery() {
		if(m_Flags != nullptr)
			delete[] m_Flags;
		m_Flags = nullptr;
		if(m_Labels != nullptr)
			delete[] m_Labels;
		m_Labels = nullptr;
	}

	int GetMaxLabel() {
		int num = INT_MIN;
		for (int i = 0; i < size; ++i) {
			num = num < m_Labels[i] ? m_Labels[i] : num;
		}

		return num;
	}

	void lock(int th_ID) {
		int i = th_ID;
		m_Flags[i] = true;
		std::atomic_thread_fence(std::memory_order_seq_cst);
		m_Labels[i] = GetMaxLabel() + 1;
		std::atomic_thread_fence(std::memory_order_seq_cst);
		int nThreads = size;
		for (int j = 0; j < nThreads; ++j) {
			bool otherFlag = m_Flags[j];
			if (i != j && otherFlag) {
				if (m_Labels[i] > m_Labels[j])
					j = 0;
			}
		}
	}

	void unLock(int th_ID) {
		m_Flags[th_ID] = false;
	}
};


volatile int victim = 0;
volatile bool flag[2] = { false, false };

void Lock(int myID)
{
	int other = 1 - myID;
	flag[myID] = true;
	victim = myID;
	std::atomic_thread_fence(std::memory_order_seq_cst);
	while (flag[other] && victim == myID) {}
}
void Unlock(int myID)
{
	flag[myID] = false;
}


volatile int sum = 0;
Bakery gBakery;
std::mutex myLock;

void Func(int th_id, int nThread) {
	int id = th_id;
	for (int i = 0; i < 10000000 / nThread; ++i) {
		gBakery.lock(id);
		sum = sum + 1;
		gBakery.unLock(id);
	}
}


void Func_fit(int th_id, int nThread) {
	int id = th_id;
	for (int i = 0; i < 10000000 / nThread; ++i) {
		Lock(id);
		sum = sum + 1;
		Unlock(id);
	}
}

void Func_NoenLock(int th_id, int nThread) {
	for (int i = 0; i < 10000000 / nThread; ++i) {
		sum = sum + 1;
	}
}

void Fund_Mutex(int th_id, int nThread) {
	for (int i = 0; i < 10000000 / nThread; ++i) {
		myLock.lock();
		sum = sum + 1;
		myLock.unlock();
	}
}

int main() {
	std::vector<std::thread> threads;
	auto startTime = std::chrono::high_resolution_clock::now();
	auto endTime = startTime;

	std::cout << "Peterson Lock" << std::endl;
	threads.clear();
	sum = 0;
	startTime = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 2; ++i) {
		threads.emplace_back(Func_fit, i, 2);
	}

	for (int i = 0; i < 2; ++i) {
		threads[i].join();
	}
	endTime = std::chrono::high_resolution_clock::now();
	std::cout << "sum = " << sum << "  Time = " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << std::endl;


	std::cout << "None Lock" << std::endl;
	for (int k = 1; k <= 8; k *= 2) {
		threads.clear();
		sum = 0;
		startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < k; ++i) {
			threads.emplace_back(Func_NoenLock, i, k);
		}

		for (int i = 0; i < k; ++i) {
			threads[i].join();
		}

		endTime = std::chrono::high_resolution_clock::now();
		std::cout << "sum = " << sum << "  Time = " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << std::endl;
	}


	std::cout << "Bakery" << std::endl;
	for (int k = 1; k <= 8; k *= 2) {
		threads.clear();
		//gBakery.~Bakery();
		gBakery = Bakery(k);
		sum = 0;
		startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < k; ++i) {
			threads.emplace_back(Func, i, k);
		}

		for (int i = 0; i < k; ++i) {
			threads[i].join();
		}

		endTime = std::chrono::high_resolution_clock::now();
		std::cout << "sum = " << sum << "  Time = " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << std::endl;
	}


	std::cout << "Mutex" << std::endl;
	for (int k = 1; k <= 8; k *= 2) {
		threads.clear();
		sum = 0;
		startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < k; ++i) {
			threads.emplace_back(Fund_Mutex, i, k);
		}

		for (int i = 0; i < k; ++i) {
			threads[i].join();
		}

		endTime = std::chrono::high_resolution_clock::now();
		std::cout << "sum = " << sum << "  Time = " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << std::endl;
	}


	return 0;
}