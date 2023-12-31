#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <set>
#include <unordered_set>

/// <summary>
/// stack이나 queue는 CAS를 할 때 Stack은 모든 PUSH, POP이 충돌하고 Queue는 50%이므로 병렬성이 떨어지는 구조임
/// </summary>

using namespace std;
using namespace chrono;

constexpr int MAX_THREADS = 8;

class null_mutex {
public:
	void lock() {}
	void unlock() {}
};

class NODE {
public:
	int v;
	NODE* volatile next;
	NODE() : v(-1), next(nullptr) {}
	NODE(int x) : v(x), next(nullptr) {}
};

class CSTACK {
	NODE* volatile top;
	mutex top_l;
public:
	CSTACK() : top(nullptr) { }
	void PUSH(int x)
	{
		NODE* e = new NODE{ x };
		top_l.lock();
		e->next = top;
		top = e;
		top_l.unlock();
	}

	int POP()
	{
		top_l.lock();
		if (nullptr == top) {
			top_l.unlock();
			return -2;
		}
		int temp = top->v;
		NODE* ptr = top;
		top = top->next;
		top_l.unlock();
		delete ptr;
		return temp;
	}

	void print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = top;
		while (p != nullptr) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		top = nullptr;
	}
};


class LFSTACK {
	NODE* volatile top;
public:
	LFSTACK() : top(nullptr) { }
	bool CAS(NODE* volatile* addr, NODE* old_p, NODE* new_p)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong*>(addr),
			reinterpret_cast<long long*>(&old_p),
			reinterpret_cast<long long>(new_p));
	}
	void PUSH(int x)
	{
		NODE* e = new NODE(x);

		while (true) {
			NODE* ptr = top;
			e->next = ptr;
			if (CAS(&top, ptr, e)) {
				return;
			}
		}
	}

	int POP()
	{
		while (true)
		{
			NODE* ptr = top;
			if (ptr == nullptr) {
				return -2;
			}
			int value = ptr->v;
			NODE* next_top = ptr->next;
			if (CAS(&top, ptr, next_top)) {
				return value;
			}
		}

		return -2;
	}

	void print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = top;
		while (p != nullptr) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		top = nullptr;
	}
};

/// <summary>
/// Back-Off Stack CAS는 순차적으로 실행되며 Stack에 특성상 경쟁이 심해 지연이 많음
/// 경쟁이 많다고 느껴지면 일정 시간 동안 쉬도록하여 경쟁을 줄이도록 개선
/// 만능 알고리즘을 활용한 경우 코어 수에 따라 성능이 증가함
/// </summary>



class BackOff {
	int minDelay, maxDelay;
	int limit;
public:
	BackOff() {
		minDelay = 0;
		limit = minDelay;
		maxDelay = MAX_THREADS;
	}
	BackOff(int min, int max)
		: minDelay(min), maxDelay(max), limit(min) {}
	void relax() {
		int delay = 0;
		if (limit != 0) delay = rand() % limit;
		if (limit != 0) limit *= 2;
		else limit = 1;
		if (limit > maxDelay) limit = maxDelay;

		/*int start, current;
		_asm RDTSC;
		_asm mov start, eax;
		do {
			_asm RDTSC;
			_asm mov current, eax;
		} while (current - start < delay);*/
		// start, current를 쓰는 과정에서 메모리 접근을 하기 때문에 이 방법은 성능 향상이 크게 의미가 없다
		// 또한, 컴퓨터의 성능은 클럭 수가 아닌 컴퓨터 구조에 의해 큰 영향을 받는다. 그래서 RDTSC가 그리 효율적이진 않음

		/*_asm mov eax, delay;
	myloop:
		_asm dec eax
		_asm jnz myloop;*/
		// 컴퓨터의 성능을 보다 고려한 방식
		// 딜레이를 계산하는 것을 모두 CPU 레지스터에서 처리.
		// 그러나 매우 큰 차이는 없음
		// Lock Free 알고리즘 자체가 오버헤드가 최소화되도록 되어있고, 코어의 개수가 적다면 큰 영향 X

		this_thread::sleep_for(chrono::microseconds(delay));;
		// 운영체제를 호출하여 대기하는 것은 낭비임.
	}
	void reduce() {
		limit /= 2;
	}
};

thread_local int tl_id;

int Thread_id()
{
	return tl_id;
}

class LFBOSTACK {
	NODE* volatile top;
	BackOff m_backoff{ 1,MAX_THREADS };
public:
	LFBOSTACK() : top(nullptr) { }
	bool CAS(NODE* volatile* addr, NODE* old_p, NODE* new_p)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong*>(addr),
			reinterpret_cast<long long*>(&old_p),
			reinterpret_cast<long long>(new_p));
	}
	void PUSH(int x)
	{
		NODE* e = new NODE(x);
		bool first_time = true;
		while (true) {
			NODE* ptr = top;
			e->next = ptr;
			if (CAS(&top, ptr, e)) {
				if (first_time == true)
					m_backoff.reduce();
				return;
			}
			first_time = false;
			m_backoff.relax();

		}
	}

	int POP()
	{
		bool first_time = true;
		while (true)
		{
			NODE* ptr = top;
			if (ptr == nullptr) {
				return -2;
			}
			int value = ptr->v;
			NODE* next_top = ptr->next;
			if (CAS(&top, ptr, next_top)) {
				if (first_time == true)
					m_backoff.reduce();
				return value;
			}
			first_time = false;
			m_backoff.relax();
		}

		return -2;
	}

	void print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = top;
		while (p != nullptr) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		top = nullptr;
	}
};

#define ST_EMPTY 0x00000000
#define ST_WAITING 0x40000000
#define ST_BUSY 0x80000000
#define VALUE_MASK 0x3FFFFFFF
#define STATUS_MASK 0xc0000000

constexpr int LOOP_LIMIT = 1000;


class EXCHANGER {
	volatile unsigned int slot;
	bool CAS(unsigned old_slot, unsigned new_status, unsigned new_value)
	{
		return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_uint*>(&slot), &old_slot, new_status | new_value);
	}
public:
	EXCHANGER() : slot(ST_EMPTY) {};

	int exchange(int v)
	{
		if (-1 == v)
			v = v & VALUE_MASK;
		if (STATUS_MASK & v != ST_EMPTY) {
			cout << "Value is too big" << endl;
			return -4;
		}
		int loop_counter = 0;
		while (true)
		{
		retry:
			if (LOOP_LIMIT < loop_counter++)
				return -3;	// 교환 실패, 충돌이 너무 심함
			unsigned curr = slot;
			unsigned status = curr & STATUS_MASK;
			unsigned value = curr & VALUE_MASK;
			switch (status)
			{
			case ST_EMPTY:
				if (CAS(curr, ST_WAITING, v) == true) {
					int count = 0;
					while (STATUS_MASK & slot == ST_WAITING)
					{
						if (LOOP_LIMIT < count++) {
							if (CAS(ST_WAITING | v, ST_EMPTY, 0))
								return -2;	// 교환 실패
							else {
								unsigned ret = VALUE_MASK & slot;
								slot = ST_EMPTY;
								if (ret == VALUE_MASK)
									return -1;
								return ret;
							}
						}
					}
					unsigned ret = VALUE_MASK & slot;
					slot = ST_EMPTY;
					if (ret == VALUE_MASK)
						return -1;
					return ret;
				}
				else {
					continue;
				}
				break;
			case ST_WAITING:
				if (CAS(curr, ST_BUSY, v) == true) {
					if (value == VALUE_MASK)
						return -1;
					return value;
				}
				else
					continue;
				break;
			case ST_BUSY:
				continue;
			default:
				break;
			}
		}
	}
};

class EliminationArray {
	volatile int range;
	EXCHANGER exchanger[MAX_THREADS];
	bool CAS(int old_range, int new_range) {
		return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(&range), &old_range, new_range);
	}
public:
	EliminationArray() { range = 1; }
	~EliminationArray() {}
	int Visit(int value) {
		int old_range = range;
		int slot = rand() % old_range;
		int ret = exchanger[slot].exchange(value);

		if ((ret == -2) && (old_range > 1)) // WAIT TIME OUT
			CAS(old_range, old_range - 1);
		if ((ret == -3) && (old_range <= MAX_THREADS / 2))
			CAS(old_range, old_range + 1); // MAX RANGE is # of thread / 2
		return ret;
	}	
};

class LFELSTACK {
	NODE* volatile top;
	EliminationArray el;
	volatile int eliminationCount = 0;
	bool CAS_EL(int old_value, int new_value) {
		return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(&eliminationCount), &old_value, new_value);
	}
public:
	LFELSTACK() : top(nullptr) { }
	bool CAS(NODE* volatile* addr, NODE* old_p, NODE* new_p)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong*>(addr),
			reinterpret_cast<long long*>(&old_p),
			reinterpret_cast<long long>(new_p));
	}
	void PUSH(int x)
	{
		NODE* e = new NODE(x);
		while (true) {
			NODE* ptr = top;
			e->next = ptr;
			if (CAS(&top, ptr, e)) {
				return;
			}
			int ret = el.Visit(x);

			if (ret == -1) {
				while (true)
				{
					int count = eliminationCount;
					if (CAS_EL(count, count + 1))
						break;
				}
				return;
			}
			if (ret == -2 || ret == -3) {
				continue;
			}
		}
	}

	int POP()
	{
		while (true)
		{
			NODE* ptr = top;
			if (ptr == nullptr) {
				return -2;
			}
			int value = ptr->v;
			NODE* next_top = ptr->next;
			if (CAS(&top, ptr, next_top)) {
				return value;
			}

			int ret = el.Visit(-1);

			if (ret == -2 || ret == -3) {
				continue;
			}
			if (ret != -1) {
				while (true)
				{
					int count = eliminationCount;
					if (CAS_EL(count, count + 1))
						break;
				}
				return ret;
			}
		}

		return -2;
	}

	void print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
		cout << "elimination - " << eliminationCount << endl;
	}

	void clear()
	{
		NODE* p = top;
		while (p != nullptr) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		top = nullptr;
	}
};

typedef LFELSTACK MY_STACK;

constexpr int LOOP = 10000000;

void worker(MY_STACK* my_stack, int threadNum, int th_id)
{
	tl_id = th_id;

	for (int i = 0; i < LOOP / threadNum; i++) {
		if ((rand() % 2) || i < 1024 / threadNum) {
			my_stack->PUSH(i);
		}
		else {
			my_stack->POP();
		}
	}
}




struct HISTORY {
	vector <int> push_values, pop_values;
};

atomic_int stack_size = 0;


void worker_check(MY_STACK* my_stack, int num_threads, int th_id, HISTORY& h)
{
	tl_id = th_id;
	for (int i = 0; i < LOOP / num_threads; i++) {
		if ((rand() % 2) || i < 128 / num_threads) {
			h.push_values.push_back(i);
			stack_size++;
			my_stack->PUSH(i);
		}
		else {
			stack_size--;
			int res = my_stack->POP();
			if (res == -2) {
				stack_size++;
				if (stack_size > num_threads) {
					cout << "ERROR Non_Empty Stack Returned NULL\n";
					exit(-1);
				}
			}
			else h.pop_values.push_back(res);
		}
	}
}

void check_history(MY_STACK& my_stack, vector <HISTORY>& h)
{
	unordered_multiset <int> pushed, poped, in_stack;

	for (auto& v : h)
	{
		for (auto num : v.push_values) pushed.insert(num);
		for (auto num : v.pop_values) poped.insert(num);
		while (true) {
			int num = my_stack.POP();
			if (num == -2) break;
			poped.insert(num);
		}
	}
	for (auto num : pushed) {
		if (poped.count(num) < pushed.count(num)) {
			cout << "Pushed Number " << num << " does not exists in the STACK.\n";
			exit(-1);
		}
		if (poped.count(num) > pushed.count(num)) {
			cout << "Pushed Number " << num << " is poped more than " << poped.count(num) - pushed.count(num) << " times.\n";
			exit(-1);
		}
	}
	for (auto num : poped)
		if (pushed.count(num) == 0) {
			std::multiset <int> sorted;
			for (auto num : poped)
				sorted.insert(num);
			cout << "There was elements in the STACK no one pushed : ";
			int count = 20;
			for (auto num : sorted)
				cout << num << ", ";
			cout << endl;
			exit(-1);

		}
	cout << "NO ERROR detectd.\n";
}



int main()
{
	cout << "======== BENCHMARKING =========\n";
	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		vector <thread> threads;
		MY_STACK my_stack;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker, &my_stack, num_threads, i);
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_stack.print20();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
	}

	cout << "======== Error Checking =========\n";
	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		vector <thread> threads;
		vector <HISTORY> log(num_threads);
		MY_STACK my_stack;
		stack_size = 0;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker_check, &my_stack, num_threads, i, std::ref(log[i]));
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_stack.print20();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
		check_history(my_stack, log);
	}

}