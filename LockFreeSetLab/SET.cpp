#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
using namespace std; 

constexpr int MAX_TH = 16;

class NODE {
public:
	int key;
	NODE* next;
	mutex nlock;

	NODE() { next = NULL; }

	NODE(int key_value) {
		next = NULL;
		key = key_value;
	}

	~NODE() {}

	void lock() { nlock.lock(); }
	void unlock() { nlock.unlock(); }
};

class CLIST {
NODE head, tail;
mutex glock;
public:
	CLIST()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	~CLIST() {}

	void Init()
	{
		NODE* ptr;
		while (head.next != &tail) {
			ptr = head.next;
			head.next = head.next->next;
			delete ptr;
		}
	}
	bool Add(int key)
	{
		NODE* pred, * curr;

		pred = &head;
		glock.lock();
		curr = pred->next;
		while (curr->key < key) {
			pred = curr;
			curr = curr->next;
		}

		if (key == curr->key) {
			glock.unlock();
			return false;
		}
		else {
			NODE* node = new NODE(key);
			node->next = curr;
			pred->next = node;
			glock.unlock();
			return true;
		}

	}
	bool Remove(int key)
	{
		NODE* pred, * curr;

		pred = &head;
		glock.lock();
		curr = pred->next;
		while (curr->key < key) {
			pred = curr;
			curr = curr->next;
		}

		if (key == curr->key) {
			pred->next = curr->next;
			glock.unlock();
			delete curr;
			return true;
		}
		else {
			glock.unlock();
			return false;
		}

	}
	bool Contains(int key)
	{
		NODE* pred, * curr;

		pred = &head;
		glock.lock();
		curr = pred->next;
		while (curr->key < key) {
			pred = curr;
			curr = curr->next;
		}

		if (key == curr->key) {
			glock.unlock();
			return false;
		}
		else {
			NODE* node = new NODE(key);
			node->next = curr;
			pred->next = node;
			glock.unlock();
			return true;
		}

	}
	void print()
	{	
		auto p = head.next;
		for (int i = 0; i < 20; ++i) {
			cout << p->key << ", ";
			if (p == &tail) break;
			p = p->next;
		}
		cout << endl;
	}
};

class OLIST {
	NODE head, tail;
public:
	OLIST()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	~OLIST() {}
	bool validate(NODE* pred, NODE* curr)
	{
		NODE* node = &head;
		while (node->key <= pred->key)
		{
			if (node == pred)
				return pred->next == curr;
			node = node->next;
		}
		return false;
	}
	void Init()
	{
		NODE* ptr;
		while (head.next != &tail) {
			ptr = head.next;
			head.next = head.next->next;
			delete ptr;
		}
	}
	bool Add(int key)
	{
		NODE* pred;
		NODE* curr;

		while (true)
		{
			pred = &head;
			curr = pred->next;
			while (curr->key < key)
			{
				pred = curr;
				curr = curr->next;
			}
			pred->lock();
			curr->lock();
			if (validate(pred, curr)) {
				if (key == curr->key) {
					pred->unlock();
					curr->unlock();
					return false;
				}
				else {
					NODE* node = new NODE(key);
					node->next = curr;
					pred->next = node;
					pred->unlock();
					curr->unlock();
					return true;
				}
			}
			else {
				pred->unlock();
				curr->unlock();
				continue;
			}
		}

	}
	bool Remove(int key)
	{
		NODE* pred;
		NODE* curr;

		while (true)
		{
			pred = &head;
			curr = pred->next;
			while (curr->key < key)
			{
				pred = curr;
				curr = curr->next;
			}
			pred->lock();
			curr->lock();
			if (validate(pred, curr)) {
				if (key == curr->key) {
					pred->next = curr->next;
					pred->unlock();
					curr->unlock();
					//delete curr;
					return true;
				}
				else {
					pred->unlock();
					curr->unlock();
					return false;
				}
			}
			else {
				pred->unlock();
				curr->unlock();
				continue;
			}
		}

	}
	bool Contains(int key)
	{
		NODE* pred;
		NODE* curr;

		while (true)
		{
			pred = &head;
			curr = pred->next;
			while (curr->key < key)
			{
				pred = curr;
				curr = curr->next;
			}
			pred->lock();
			curr->lock();
			if (validate(pred, curr)) {
				if (key == curr->key) {
					pred->unlock();
					curr->unlock();
					return true;
				}
				else {
					pred->unlock();
					curr->unlock();
					return false;
				}
			}
			else {
				pred->unlock();
				curr->unlock();
				continue;
			}
		}

	}
	void print()
	{
		auto p = head.next;
		for (int i = 0; i < 20; ++i) {
			cout << p->key << ", ";
			if (p == &tail) break;
			p = p->next;
		}
		cout << endl;
	}
};

const auto NUM_TEST = 4000000;
const auto KEY_RANGE = 1000;

OLIST my_set;

void ThreadFunc(int num_thread)
{
	int key;

	for (int i = 0; i < NUM_TEST / num_thread; i++) {
		switch (rand() % 3) {
		case 0: key = rand() % KEY_RANGE;
			my_set.Add(key);
			break;
		case 1: key = rand() % KEY_RANGE;
			my_set.Remove(key);
			break;
		case 2: key = rand() % KEY_RANGE;
			my_set.Contains(key);
			break;
		default: cout << "Error\n";
			exit(-1);
		}
	}
}

int main()
{
	for (int num_threads = 1; num_threads <= MAX_TH; num_threads = num_threads * 2) {
		my_set.Init();
		vector <thread> threads;
		auto start_t = chrono::high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(ThreadFunc, num_threads);
		for (auto& th : threads) th.join();
		auto end_t = chrono::high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto msec = chrono::duration_cast<chrono::milliseconds>(exec_t).count();
		cout << num_threads << "threads, ";
		cout << ", " << msec << "ms.\n";
		my_set.print();
	}
}
