#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <array>

#define MAX_THREADS 32

class my_mutex
{
public:
	void lock() {}
	void unlock() {}
};

class NODE {
public:
	int key;
	NODE* volatile next;
	std::mutex nlock;
	volatile bool removed;
	NODE() { next = NULL; }
	NODE(int key_value) {
		next = NULL;
		key = key_value;
		removed = false;
	}
	~NODE() {}
	void lock() { nlock.lock(); }
	void unlock() { nlock.unlock(); }
};

class SPNODE {
public:
	int key;
	std::shared_ptr<SPNODE> next;
	std::mutex nlock;
	volatile bool removed;
	SPNODE() { next = nullptr; }
	SPNODE(int key_value) {
		next = nullptr;
		key = key_value;
		removed = false;
	}
	~SPNODE() {}
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

class OSET {
	NODE head, tail;
public:
	OSET()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	~OSET() {}
	void Init()
	{
		NODE* ptr;
		while (head.next != &tail) {
			ptr = head.next;
			head.next = head.next->next;
			delete ptr;
		}
	}
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
	void Print(int count) {
		NODE* p = head.next;
		for (int i = 0; i < count; ++i) {
			std::cout << p->key << " ";
			p = p->next;
		}
		std::cout << std::endl;
	}
};


class LSET {
	NODE head, tail;
public:
	LSET()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	~LSET() {}
	void Init()
	{
		NODE* ptr;
		while (head.next != &tail) {
			ptr = head.next;
			head.next = head.next->next;
			delete ptr;
		}
	}
	bool validate(NODE* pred, NODE* curr)
	{
		return (pred->removed == false) && (curr->removed == false) && (pred->next == curr);
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
					curr->removed = true;
					pred->next = curr->next;
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
	bool Contains(int key)
	{
		NODE* curr = &head;

		while (curr->key < key)
		{
			curr = curr->next;
		}
		return curr->key == key && curr->removed == false;
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


class LSPSET {
	std::shared_ptr<SPNODE> head, tail;
public:
	LSPSET()
	{
		head = std::make_shared<SPNODE>(0x80000000);
		tail = std::make_shared<SPNODE>(0x7FFFFFFF);
		head->next = tail;
	}
	~LSPSET() {}
	void Init()
	{
		std::shared_ptr<SPNODE> ptr;
		while (head->next != tail) {
			ptr = head->next;
			head->next = head->next->next;
		}
	}
	bool validate(const std::shared_ptr<SPNODE>& pred, const std::shared_ptr<SPNODE>& curr)
	{
		return (pred->removed == false) && (curr->removed == false) && (pred->next == curr);
	}
	bool Add(int key)
	{
		std::shared_ptr<SPNODE> pred;
		std::shared_ptr<SPNODE> curr;

		while (true)
		{
			pred = head;
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
					std::shared_ptr<SPNODE> node = std::make_shared<SPNODE>(key);
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
		std::shared_ptr<SPNODE> pred;
		std::shared_ptr<SPNODE> curr;

		while (true)
		{
			pred = head;
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
					curr->removed = true;
					pred->next = curr->next;
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
	bool Contains(int key)
	{
		std::shared_ptr<SPNODE> curr = head;

		while (curr->key < key)
		{
			curr = curr->next;
		}
		return curr->key == key && curr->removed == false;
	}
	void Clear()
	{
		head->next = tail;
	}
	void Print(int count) {
		std::shared_ptr<SPNODE> p = head->next;
		for (int i = 0; i < count; ++i) {
			std::cout << p->key << " ";
			p = p->next;
		}
		std::cout << std::endl;
	}
};


const auto NUM_TEST = 4000000;
const auto KEY_RANGE = 1000;
CLIST clist;

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


class HISTORY {
public:
	int op;
	int i_value;
	bool o_value;
	HISTORY(int o, int i, bool re) : op{ o }, i_value{ i }, o_value{ re } {};
};


void worker(std::vector<HISTORY>* history, int num_thread)
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

void worker_check(std::vector<HISTORY>* history, int num_thread)
{
	int key;
	for (int i = 0; i < NUM_TEST / num_thread; i++) {
		switch (rand() % 3) {
		case 0: key = rand() % KEY_RANGE;
			history->emplace_back(0, key, clist.Add(key));
			break;
		case 1: key = rand() % KEY_RANGE;
			history->emplace_back(1, key, clist.Remove(key));
			break;
		case 2: key = rand() % KEY_RANGE;
			history->emplace_back(2, key, clist.Contains(key));
			break;
		default: std::cout << "Error\n";
			exit(-1);
		}
	}
}

void check_history(std::array <std::vector <HISTORY>, MAX_THREADS>& history, int num_threads)
{
	std::array <int, KEY_RANGE> survive = {};
	std::cout << "Checking Consistency : ";
	if (history[0].size() == 0) {
		std::cout << "No history.\n";
		return;
	}
	for (int i = 0; i < num_threads; ++i) {
		for (auto& op : history[i]) {
			if (false == op.o_value) continue;
			if (op.op == 3) continue;
			if (op.op == 0) survive[op.i_value]++;
			if (op.op == 1) survive[op.i_value]--;
		}
	}
	for (int i = 0; i < KEY_RANGE; ++i) {
		int val = survive[i];
		if (val < 0) {
			std::cout << "ERROR. The value " << i << " removed while it is not in the set.\n";
			exit(-1);
		}
		else if (val > 1) {
			std::cout << "ERROR. The value " << i << " is added while the set already have it.\n";
			exit(-1);
		}
		else if (val == 0) {
			if (clist.Contains(i)) {
				std::cout << "ERROR. The value " << i << " should not exists.\n";
				exit(-1);
			}
		}
		else if (val == 1) {
			if (false == clist.Contains(i)) {
				std::cout << "ERROR. The value " << i << " shoud exists.\n";
				exit(-1);
			}
		}
	}
	std::cout << " OK\n";
}

void main()
{
	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		std::vector <std::thread> threads;
		std::array<std::vector <HISTORY>, MAX_THREADS> history;
		clist.Init();
		auto start_t = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker_check, &history[i], num_threads);
		for (auto& th : threads)
			th.join();
		auto end_t = std::chrono::high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_t).count();
		//clist.Print(20);
		std::cout << num_threads << " Threads.  Exec Time : " << exec_ms << std::endl;
		check_history(history, num_threads);
	}

	std::cout << "======== SPEED CHECK =============\n";

	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		std::vector <std::thread> threads;
		std::array<std::vector <HISTORY>, MAX_THREADS> history;
		clist.Init();
		auto start_t = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker, &history[i], num_threads);
		for (auto& th : threads)
			th.join();
		auto end_t = std::chrono::high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_t).count();
		//clist.Print(20);
		std::cout << num_threads << " Threads.  Exec Time : " << exec_ms << std::endl;
		check_history(history, num_threads);
	}

	return;
}