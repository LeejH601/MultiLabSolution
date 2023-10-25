#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

using namespace std;
using namespace chrono;

constexpr int MAX_THREADS = 32;

class null_mutex {
public:
	void lock() {}
	void unlock() {}
};

class NODE {
public:
	int v;
	NODE* volatile next;
	NODE() : v(-1), next(nullptr){ }
	NODE(int x) : v(x), next(nullptr) {}
};

class CQUEUE {
	NODE* volatile head, * volatile tail;
	mutex enq_l, deq_l;

public:
	CQUEUE()
	{
		head = new NODE();
		tail = head;
	}
	void ENQ(int x)
	{
		NODE* e = new NODE(x);
		enq_l.lock();
		tail->next = e;
		tail = e;
		enq_l.unlock();
	}
	int DEQ()
	{
		deq_l.lock();
		if (head->next == nullptr) {
			deq_l.unlock();
			return -1;
		}
		int result = head->next->v;
		NODE* temp = head;
		head = head->next;
		deq_l.unlock();

		delete temp;

		return result;
	}
	void print20()
	{
		NODE* p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr)
				break;
			std::cout << p->v << " ";
			p = p->next;
		}
		std::cout << endl;
	}
	void clear()
	{
		NODE* p = head->next;
		while (p != nullptr) {
			NODE* t = p->next;
			p = p->next;
			delete t;
		}
		tail = head;
		head->next = nullptr;
	}
};

class LF_QUEUE {
	NODE* volatile head, * volatile tail;

public:
	LF_QUEUE()
	{
		head = new NODE();
		tail = head;
	}
	void ENQ(int x)
	{
		NODE* e = new NODE(x);
		while (true)
		{
			NODE* last = tail;
			NODE* next = last->next;
			if (last != tail) continue;
			if (next == nullptr) {
				if (CAS(&last->next, nullptr, &e)) {
					CAS(&tail, last, e);
					return;
				}
			}
			else {
				CAS(&tail, last, next);
			}
		}
	}
	int DEQ()
	{
		deq_l.lock();
		if (head->next == nullptr) {
			deq_l.unlock();
			return -1;
		}
		int result = head->next->v;
		NODE* temp = head;
		head = head->next;
		deq_l.unlock();

		delete temp;

		return result;
	}
	void print20()
	{
		NODE* p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr)
				break;
			std::cout << p->v << " ";
			p = p->next;
		}
		std::cout << endl;
	}
	void clear()
	{
		NODE* p = head->next;
		while (p != nullptr) {
			NODE* t = p->next;
			p = p->next;
			delete t;
		}
		tail = head;
		head->next = nullptr;
	}
};

thread_local int tl_id;

int Thread_id()
{
	return tl_id;
}

typedef CQUEUE MY_QUEUE;

class HISTORY {
public:
	int op;
	int i_value;
	bool o_value;
	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re) {}
};

constexpr int N = 10000000;

void worker(MY_QUEUE* my_queue, int threadNum, int thread_id)
{
	tl_id = thread_id;
	for (int i = 0; i < N / threadNum; i++) {
		if ((rand() % 2) || i < 128 / threadNum) {
			my_queue->ENQ(i);
		}
		else {
			my_queue->DEQ();
		}
	}
}


int main()
{
	cout << "======== SPEED CHECK =============\n";

	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		vector <thread> threads;
		MY_QUEUE my_queue;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker, &my_queue, num_threads, i);
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_queue.print20();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
	}
}