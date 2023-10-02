#include <iostream>
#include <mutex>
#include <vector>
#include <thread>

class my_mutex
{
public:
	void lock() {}
	void unlock() {}
};

class NODE {
public:
	int key;
	NODE* next;
	std::mutex nlock;
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
	std::mutex glock;
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
		NODE* pred;
		NODE* curr;

		pred = &head;
		glock.lock();
		curr = pred->next;
		while (curr->key < key)
		{
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
		NODE* pred;
		NODE* curr;

		pred = &head;
		glock.lock();
		curr = pred->next;
		while (curr->key < key)
		{
			pred = curr;
			curr = curr->next;
		}
		if (key == curr->key) {
			pred->next = curr->next;
			delete curr;
			glock.unlock();
			return true;
		}
		else {
			glock.unlock();
			return false;
		}
	}
	bool Contains(int key)
	{
		NODE* pred;
		NODE* curr;

		pred = &head;
		glock.lock();
		curr = pred->next;
		while (curr->key < key)
		{
			pred = curr;
			curr = curr->next;
		}
		if (key == curr->key) {
			glock.unlock();
			return true;
		}
		else {
			glock.unlock();
			return false;
		}
	}
};

class FSET {
	NODE head, tail;
public:
	FSET()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	~FSET() {}
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
		NODE* prev;
		NODE* curr;

		prev = &head;
		prev->lock();
		curr = prev->next;
		curr->lock();
		while (curr->key < key)
		{
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (key == curr->key) {
			prev->unlock();
			curr->unlock();
			return false;
		}
		else {
			NODE* node = new NODE(key);
			node->next = curr;
			prev->next = node;
			prev->unlock();
			curr->unlock();
			return true;
		}
	}
	bool Remove(int key)
	{
		NODE* prev;
		NODE* curr;

		prev = &head;
		prev->lock();
		curr = prev->next;
		curr->lock();
		while (curr->key < key)
		{
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (key == curr->key) {
			prev->next = curr->next;
			prev->unlock();
			curr->unlock();
			delete curr;
			return true;
		}
		else {
			prev->unlock();
			curr->unlock();
			return false;
		}
	}
	bool Contains(int key)
	{
		NODE* prev;
		NODE* curr;

		prev = &head;
		prev->lock();
		curr = prev->next;
		curr->lock();
		while (curr->key < key)
		{
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (key == curr->key) {
			prev->unlock();
			curr->unlock();
			return true;
		}
		else {
			prev->unlock();
			curr->unlock();
			return false;
		}
	}

	void Print(int count) {
		NODE* p = head.next;
		for (int i = 0; i < count; ++i) {
			std::cout << p->key << " ";
			p = p->next;
		}
		std::cout << std::endl;
	}
};

const auto NUM_TEST = 4000000;
const auto KEY_RANGE = 1000;
FSET clist;

void ThreadFunc(int num_thread)
{
	int key;
	for (int i = 0; i < NUM_TEST / num_thread; i++) {
		switch (rand() % 3) {
		case 0: key = rand() % KEY_RANGE;
			clist.Add(key);
			break;
		case 1: key = rand() % KEY_RANGE;
			clist.Remove(key);
			break;
		case 2: key = rand() % KEY_RANGE;
			clist.Contains(key);
			break;
		default: std::cout << "Error\n";
			exit(-1);
		}
	}
}

void main()
{
	std::vector<std::thread> threads;
	auto startTime = std::chrono::high_resolution_clock::now();
	auto endTime = startTime;

	for (int i = 1; i <= 32; i *= 2) {
		threads.clear();
		clist.Init();
		for (int j = 0; j < i; ++j)
			threads.emplace_back(ThreadFunc, i);

		for (int j = 0; j < i; ++j)
			threads[j].join();

		endTime = std::chrono::high_resolution_clock::now();
		//clist.Print(20);
		std::cout << i << " Threads : " << "Time = " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << std::endl;
	}

	return;
}