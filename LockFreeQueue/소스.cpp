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
	// Deq �������� ������ A�� Head �� a�� ����Ű�� b�� ��ü�� �� ��� ���°� �� ����,
	// �ٸ� ������b�� c���� Enq�� Deq�� �Ǿ� �����Ǿ��� a�� �ּҿ� ������ ��ġ�� �Ҵ�� ���ο� ��� a'�� �ٽ� Head�� ��ġ�ϰ� �ȴ���
	// ������ A�� ��� �ٽ� �۾��� �ϰ� �Ǹ� a'�� b�� ��ü�Ϸ��� �õ��ϰ� �Ǿ� ���� �߻�
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
// ��Ƽ������ �� �߻��ϴ� ������
// Data Race
// Compiler ����ȭ
// Out of Order Excuation
// Cache Line
// ABA
//

//
// ABA
// shared_ptr�� ���ų� ������ �÷¼��� �ϴ� ȯ���̶�� ABA ������ ����
// EBR�� ����� ���� ���������� ������ ����
//

// ABA �ذ� 1
// ������ + ������ Ȯ�� ������
// 64��Ʈ�� ��� ���� �� ���̴� 16��Ʈ�� Ȱ���Ͽ� ������ ������ ����
// 32��Ʈ�� ��� 64��Ʈ�� Ȯ���Ͽ� 64��Ʈ CAS�� ����
// �޸𸮰� �����ϱ⿡ ABA�� �߻����ɼ��� �������� ���� �ƴ����� �߻��ϱ� ���� �ð��� �ſ� ��� ������ ��ǻ� �߻� X
//
// 2
// LL, SC
// ���� ���θ� ���� �˻縦 �ϱ� ������ ������ ���ϴ� CAS�� �޸� ABA ������ ����
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