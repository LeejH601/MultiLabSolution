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
	NODE() : v(-1), next(nullptr) { }
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
	bool CAS(NODE* volatile* node, NODE* curr, NODE* next)
	{

		long long* old = reinterpret_cast<long long*>(&curr);
		volatile atomic_llong* base = reinterpret_cast<volatile atomic_llong*>(node);
		return atomic_compare_exchange_strong(base, old, reinterpret_cast<long long>(next));
	}
	void ENQ(int x)
	{
		NODE* e = new NODE(x);
		e->v = x;
		while (true)
		{
			NODE* last = tail;
			if (last != tail) continue;
			NODE* next = last->next;
			if (next == nullptr) {
				if (CAS(&last->next, nullptr, e)) {
					CAS(&tail, last, e);
					return;
				}
			}
			else {
				CAS(&tail, last, next);
			}
		}
	}
	// ABA
	// Deq 과정에서 스레드 A의 Head 가 a를 가르키다 b로 교체될 때 대기 상태가 된 이후,
	// 다른 스레드b와 c에서 Enq와 Deq가 되어 삭제되었던 a의 주소와 동일한 위치에 할당된 새로운 노드 a'이 다시 Head에 위치하게 된다음
	// 스레드 A가 깨어나 다시 작업을 하게 되면 a'을 b로 교체하려고 시도하게 되어 오류 발생
	int DEQ()
	{
		while (true)
		{
			NODE* first = head;
			NODE* last = tail;
			NODE* next = first->next;

			if (first != head) continue;
			if (next == nullptr) return -1;
			if (first == last) {
				CAS(&tail, last, next);
				continue;
			}
			int value = next->v;
			if (false == CAS(&head, first, next))
				continue;
			delete first;
			return value;
		}
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


class alignas(8) SPTR
{
public:
	NODE* volatile ptr;
	int stamp;
public:
	SPTR() :ptr{ nullptr }, stamp{ 0 } {};
	SPTR(NODE* node, int s) : ptr{ node }, stamp{ s } {};
	NODE* get_next() {
		return ptr;
	}
	int get_stamp()
	{
		return stamp;
	}
	bool operator==(const SPTR& rhs) const {
		return rhs.ptr == ptr && rhs.stamp == stamp;
	}
	bool operator!=(const SPTR& rhs) const {
		return rhs.ptr != ptr || rhs.stamp != stamp;
	}
};

class SLF_QUEUE {
	SPTR head, tail;

public:
	SLF_QUEUE()
	{
		head = tail = SPTR(new NODE, 0);
	}
	bool CAS(SPTR* ptr, NODE* old_p, NODE* new_p, int old_stamp)
	{
		SPTR old_sptr{ old_p, old_stamp };
		SPTR new_sptr{ new_p,old_stamp + 1 };

		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong*>(&ptr),
			reinterpret_cast<long long*>(&old_sptr),
			reinterpret_cast<long long>(reinterpret_cast<long long*>(&new_sptr))
		);
	}
	bool CAS(NODE* volatile* node, NODE* curr, NODE* next)
	{

		long long* old = reinterpret_cast<long long*>(&curr);
		volatile atomic_llong* base = reinterpret_cast<volatile atomic_llong*>(node);
		return atomic_compare_exchange_strong(base, old, reinterpret_cast<long long>(next));
	}
	bool CAS(NODE* volatile* node, NODE* curr, SPTR* next)
	{

		long long* old = reinterpret_cast<long long*>(&curr);
		volatile atomic_llong* base = reinterpret_cast<volatile atomic_llong*>(node);
		if (atomic_compare_exchange_strong(base, old, reinterpret_cast<long long>(next->ptr)))
			return true;
		return false;
	}
	void ENQ(int x)
	{
		SPTR e = SPTR(new NODE(x), 0);
		while (true)
		{
			NODE* last = tail.get_next();
			NODE* next = last->next;
			if (last != tail.get_next()) continue;
			if (next == nullptr) {
				if (CAS(&last->next, nullptr, e.get_next())) {
					CAS(&tail, last, last->next, tail.get_stamp());
					return;
				}
			}
			else {
				CAS(&tail, last, next, tail.get_stamp());
			}
		}
	}
	int DEQ()
	{
		while (true)
		{
			SPTR first = head;
			SPTR last = tail;
			NODE* next = first.get_next();

			if (first != head) continue;
			if (next == nullptr) return -1;
			if (first == last) {
				CAS(&tail, last.get_next(), next, last.get_stamp());
				continue;
			}
			int value = next->v;
			if (false == CAS(&head, first.get_next(), next, first.get_stamp()))
				continue;
			delete first.get_next();
			return value;
		}
	}
	void print20()
	{
		NODE* p = head.get_next();
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
		NODE* p = head.get_next();
		while (p != nullptr) {
			NODE* t = p->next;
			p = p->next;
			delete t;
		}
		tail = head;
		head = SPTR();
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

//
// 멀티스레드 간 발생하는 문제들
// Data Race
// Compiler 최적화
// Out of Order Excuation
// Cache Line
// ABA
//

//
// ABA
// shared_ptr을 쓰거나 가비지 컬력션을 하는 환경이라면 ABA 문제가 없음
// EBR을 사용할 때도 마찬가지로 문제가 없음
//

// ABA 해결 1
// 포인터 + 스탬프 확장 포인터
// 64비트의 경우 앞의 안 쓰이는 16비트를 활용하여 스탬프 정보를 저장
// 32비트의 경우 64비트로 확장하여 64비트 CAS를 수행
// 메모리가 유한하기에 ABA의 발생가능성이 없어지는 것은 아니지만 발생하기 위한 시간이 매우 길기 때문에 사실상 발생 X
//
// 2
// LL, SC
// 변경 여부를 통해 검사를 하기 때문에 값만을 비교하는 CAS와 달리 ABA 문제가 없음
//
//
// 
//


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