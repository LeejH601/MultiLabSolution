#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <array>
#include <mutex>
#include <set>

using namespace std;
using namespace chrono;

constexpr int MAX_THREADS = 32;

class null_mutex {
public:
	void lock() {}
	void unlock() {}
};

class NODE {
	mutex n_lock;
public:
	int v;
	NODE* volatile next;
	volatile bool removed;
	NODE() : v(-1), next(nullptr), removed(false) {}
	NODE(int x) : v(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

class SPNODE {
	mutex n_lock;
public:
	int v;
	shared_ptr <SPNODE> next;
	volatile bool removed;
	SPNODE() : v(-1), next(nullptr), removed(false) {}
	SPNODE(int x) : v(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};



class SET {
	NODE head, tail;
	null_mutex ll;
public:
	SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}
	bool ADD(int x)
	{
		NODE* prev = &head;
		ll.lock();
		NODE* curr = prev->next;
		while (curr->v < x) {
			prev = curr;
			curr = curr->next;
		}
		if (curr->v != x) {
			NODE* node = new NODE{ x };
			node->next = curr;
			prev->next = node;
			ll.unlock();
			return true;
		}
		else
		{
			ll.unlock();
			return false;
		}
	}

	bool REMOVE(int x)
	{
		NODE* prev = &head;
		ll.lock();
		NODE* curr = prev->next;
		while (curr->v < x) {
			prev = curr;
			curr = curr->next;
		}
		if (curr->v != x) {
			ll.unlock();
			return false;
		}
		else {
			prev->next = curr->next;
			delete curr;
			ll.unlock();
			return true;
		}
	}

	bool CONTAINS(int x)
	{
		NODE* prev = &head;
		ll.lock();
		NODE* curr = prev->next;
		while (curr->v < x) {
			prev = curr;
			curr = curr->next;
		}
		bool res = (curr->v == x);
		ll.unlock();
		return res;
	}
	void print20()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head.next;
		while (p != &tail) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;
	}
};

class F_SET {
	NODE head, tail;
public:
	F_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}
	bool ADD(int x)
	{
		head.lock();
		NODE* prev = &head;
		NODE* curr = prev->next;
		curr->lock();
		while (curr->v < x) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->v != x) {
			NODE* node = new NODE{ x };
			node->next = curr;
			prev->next = node;
			curr->unlock();
			prev->unlock();
			return true;
		}
		else
		{
			curr->unlock();
			prev->unlock();
			return false;
		}
	}

	bool REMOVE(int x)
	{
		head.lock();
		NODE* prev = &head;
		NODE* curr = prev->next;
		curr->lock();
		while (curr->v < x) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->v != x) {
			curr->unlock();
			prev->unlock();
			return false;
		}
		else {
			prev->next = curr->next;
			curr->unlock();
			prev->unlock();
			delete curr;
			return true;
		}
	}

	bool CONTAINS(int x)
	{
		head.lock();
		NODE* prev = &head;
		NODE* curr = prev->next;
		curr->lock();
		while (curr->v < x) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		bool res = (curr->v == x);
		curr->unlock();
		prev->unlock();
		return res;
	}
	void print20()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head.next;
		while (p != &tail) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;
	}
};

class O_SET {
	NODE head, tail;
public:
	O_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}
	bool validate(NODE* prev, NODE* curr)
	{
		NODE* n = &head;
		while (n->v <= prev->v) {
			if (n == prev)
				return prev->next == curr;
			n = n->next;
		}
		return false;
	}

	bool ADD(int x)
	{
		while (true) {
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					NODE* node = new NODE{ x };
					node->next = curr;
					prev->next = node;
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					prev->next = curr->next;
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		while (true) {
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v == x) {
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	void print20()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head.next;
		while (p != &tail) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;
	}
};

class L_SET {
	NODE head, tail;
public:
	L_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}

	bool validate(NODE* prev, NODE* curr)
	{
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}

	bool ADD(int x)
	{
		while (true) {
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					NODE* node = new NODE{ x };
					node->next = curr;
					prev->next = node;
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					prev->next = curr->next;
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		while (true) {
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v == x) {
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	void print20()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head.next;
		while (p != &tail) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;
	}
};
#if 0
class LSP_SET {
	shared_ptr <SPNODE> head, tail;
public:
	LSP_SET()
	{
		head = make_shared<SPNODE>(0x80000000);
		tail = make_shared<SPNODE>(0x7FFFFFFF);
		head->next = tail;
	}

	~LSP_SET()
	{
		//clear();
	}

	bool validate(const shared_ptr<SPNODE>& prev, const shared_ptr<SPNODE>& curr)
	{
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}

	bool ADD(int x)
	{
		while (true) {
			shared_ptr<SPNODE> prev = head;
			shared_ptr<SPNODE> curr = std::atomic_load(&prev->next);
			while (curr->v < x) {
				prev = curr;
				curr = std::atomic_load(&curr->next);
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					auto node = make_shared<SPNODE>(x);
					node->next = curr;
					std::atomic_exchange(&prev->next, node);
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			shared_ptr<SPNODE> prev = head;
			shared_ptr<SPNODE> curr = std::atomic_load(&prev->next);
			while (curr->v < x) {
				prev = curr;
				curr = std::atomic_load(&curr->next);
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					curr->removed = true;
					std::atomic_exchange(&prev->next, curr->next); // lock을 걸어서 굳이 curr->next를 아토믹 로드를 하지 않아도 괜찮음
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		shared_ptr<SPNODE> curr = head;
		while (curr->v < x)
			curr = std::atomic_load(&curr->next);
		return (x == curr->v) && (false == curr->removed);
	}

	void print20()
	{
		shared_ptr<SPNODE> p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		head->next = tail;
	}
};
#endif


// x86 cpu는 64비트를 쓴다면 앞에 16비트를 사용 x, 이 공간은 바로 다음 비트값에 따라 채워짐
// 마찬가지로 뒤로 2비트도 사용 x
// 이는 메모리는 항상 4의 배수 혹은 8의 배수로 할당되기 때문
//

namespace JNH {
	using namespace std;

	template <class T>

	struct atomic_shared_ptr {
	private:
		mutable mutex m_lock;
		shared_ptr<T> m_ptr;
	public:
		bool is_lock_free() const noexcept
		{
			return false;
		}

		void store(shared_ptr<T> sptr, memory_order = memory_order_seq_cst) noexcept
		{
			m_lock.lock();
			m_ptr = sptr;
			m_lock.unlock();
		}

		shared_ptr<T> load(memory_order = memory_order_seq_cst) const noexcept
		{
			m_lock.lock();
			shared_ptr<T> t = m_ptr;
			m_lock.unlock();
			return t;
		}

		operator shared_ptr<T>() const noexcept
		{
			m_lock.lock();
			shared_ptr<T> t = m_ptr;
			m_lock.unlock();
			return t;
		}

		shared_ptr<T> exchange(shared_ptr<T> sptr, memory_order = memory_order_seq_cst) noexcept
		{
			m_lock.lock();
			shared_ptr<T> t = m_ptr;
			m_ptr = sptr;
			m_lock.unlock();
			return t;
		}

		bool compare_exchange_strong(shared_ptr<T>& expected_sptr, shared_ptr<T> new_sptr, memory_order, memory_order) noexcept
		{
			bool success = false;
			m_lock.lock();
			shared_ptr<T> t = m_ptr;
			if (m_ptr.get() == expected_sptr.get()) {
				m_ptr = new_sptr;
				success = true;
			}
			expected_sptr = m_ptr;
			m_lock.unlock();
		}

		bool compare_exchange_weak(shared_ptr<T>& expected_sptr, shared_ptr<T> target_sptr, memory_order, memory_order) noexcept
		{
			return compare_exchange_strong(expected_sptr, target_sptr, memory_order);
		}

		atomic_shared_ptr() noexcept = default;

		constexpr atomic_shared_ptr(shared_ptr<T> sptr) noexcept
		{
			m_lock.lock();
			m_ptr = sptr;
			m_lock.unlock();
		}

		shared_ptr<T> operator=(shared_ptr<T> sptr) noexcept
		{
			m_lock.lock();
			m_ptr = sptr;
			m_lock.unlock();
			return sptr;
		}

		void reset()
		{
			m_lock.lock();
			m_ptr = nullptr;
			m_lock.unlock();
		}

		atomic_shared_ptr(const atomic_shared_ptr& rhs)
		{
			store(rhs);
		}

		atomic_shared_ptr& operator=(const atomic_shared_ptr& rhs)
		{
			store(rhs);
			return *this;
		}

		shared_ptr<T>& operator->() {
			std::lock_guard<mutex> tt(m_lock);
			return m_ptr;
		}

		template< typename TargetType >
		inline bool operator ==(shared_ptr< TargetType > const& rhs)
		{
			return load() == rhs;
		}

		template< typename TargetType >
		inline bool operator ==(atomic_shared_ptr<TargetType> const& rhs)
		{
			std::lock_guard<mutex> t1(m_lock);
			std::lock_guard<mutex> t2(rhs.m_lock);
			return m_ptr == rhs.m_ptr;
		}
	};

	template <class T> struct atomic_weak_ptr {
	private:
		mutable mutex m_lock;
		weak_ptr<T> m_ptr;
	public:
		bool is_lock_free() const noexcept
		{
			return false;
		}
		void store(weak_ptr<T> wptr, memory_order = memory_order_seq_cst) noexcept
		{
			m_lock.lock();
			m_ptr = wptr;
			m_lock.unlock();
		}
		weak_ptr<T> load(memory_order = memory_order_seq_cst) const noexcept
		{
			m_lock.lock();
			weak_ptr<T> t = m_ptr;
			m_lock.lock();
			return t;
		}
		operator weak_ptr<T>() const noexcept
		{
			m_lock.lock();
			weak_ptr<T> t = m_ptr;
			m_lock.unlock();
			return t;
		}
		weak_ptr<T> exchange(weak_ptr<T> wptr, memory_order = memory_order_seq_cst) noexcept
		{
			m_lock.lock();
			weak_ptr<T> t = m_ptr;
			m_ptr = wptr;
			m_lock.unlock();
			return t;
		}

		bool compare_exchange_strong(weak_ptr<T>& expected_wptr, weak_ptr<T> new_wptr, memory_order, memory_order) noexcept
		{
			bool success = false;
			lock_guard(m_lock);

			weak_ptr<T> t = m_ptr;
			shared_ptr<T> my_ptr = t.lock();
			if (!my_ptr) return false;
			shared_ptr<T> expected_sptr = expected_wptr.lock();
			if (!expected_wptr) return false;

			if (my_ptr.get() == expected_sptr.get()) {
				success = true;
				m_ptr = new_wptr;
			}
			expected_wptr = t;
			return success;
		}

		bool compare_exchange_weak(weak_ptr<T>& exptected_wptr, weak_ptr<T> new_wptr, memory_order, memory_order) noexcept
		{
			return compare_exchange_strong(exptected_wptr, new_wptr, memory_order);
		}

		atomic_weak_ptr() noexcept = default;

		constexpr atomic_weak_ptr(weak_ptr<T> wptr) noexcept
		{
			m_lock.lock();
			m_ptr = wptr;
			m_lock.unlock();
		}

		atomic_weak_ptr(const atomic_weak_ptr&) = delete;
		atomic_weak_ptr& operator=(const atomic_weak_ptr&) = delete;
		weak_ptr<T> operator=(weak_ptr<T> wptr) noexcept
		{
			m_lock.lock();
			m_ptr = wptr;
			m_lock.unlock();
			return wptr;
		}
		shared_ptr<T> lock() const noexcept
		{
			m_lock.lock();
			shared_ptr<T> sptr = m_ptr.lock();
			m_lock.unlock();
			return sptr;
		}
		void reset()
		{
			m_lock.lock();
			m_ptr.reset();
			m_lock.unlock();
		}
	};
}

using namespace JNH;

class ASPNODE {
	mutex n_lock;
public:
	int v;
	atomic_shared_ptr <ASPNODE> next;
	volatile bool removed;
	ASPNODE() : v(-1), next(nullptr), removed(false) {}
	ASPNODE(int x) : v(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

/*
class LASP2_SET {
	atomic_shared_ptr <ASPNODE> head, tail;
public:
	LASP2_SET()
	{
		head = make_shared<ASPNODE>(0x80000000);
		tail = make_shared<ASPNODE>(0x7FFFFFFF);
		head->next = tail;
	}

	~LASP2_SET()
	{
		//clear();
	}

	bool validate(const shared_ptr<ASPNODE>& prev, const shared_ptr<ASPNODE>& curr)
	{
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}

	bool ADD(int x)
	{
		while (true) {
			shared_ptr<ASPNODE> prev = head;
			atomic_shared_ptr<ASPNODE> curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					auto node = make_shared<ASPNODE>(x);
					node->next = curr;
					prev->next = node;
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			shared_ptr<ASPNODE> prev = head;
			shared_ptr<ASPNODE> curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					curr->removed = true;
					prev->next = curr->next;
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		shared_ptr<ASPNODE> curr = head;
		while (curr->v < x)
			curr = curr->next;
		return (x == curr->v) && (false == curr->removed);
	}

	void print20()
	{
		atomic_shared_ptr<ASPNODE> p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		head->next = tail;
	}
};
*/
class ASPNODE2 {
	mutex n_lock;
public:
	int v;
	atomic<std::shared_ptr<ASPNODE2>> next;
	volatile bool removed;
	ASPNODE2() : v(-1), next(nullptr), removed(false) {}
	ASPNODE2(int x) : v(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

// c++20 atomic_shared_ptr 
class LASP3_SET {
	atomic<std::shared_ptr<ASPNODE2>> head, tail;
public:
	LASP3_SET()
	{
		head = make_shared<ASPNODE2>(0x80000000);
		tail = make_shared<ASPNODE2>(0x7FFFFFFF);
		shared_ptr<ASPNODE2> t = tail;
		shared_ptr<ASPNODE2> h = head;
		h->next = t;
	}

	~LASP3_SET()
	{
		//clear();
	}

	bool validate(const std::shared_ptr<ASPNODE2>& prev, const std::shared_ptr<ASPNODE2>& curr)
	{
		shared_ptr<ASPNODE2> t = prev->next;
		return (prev->removed == false) && (curr->removed == false) && (t == curr);
	}

	bool ADD(int x)
	{
		while (true) {
			shared_ptr<ASPNODE2> prev = head;
			shared_ptr<ASPNODE2> curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					auto node = make_shared<ASPNODE2>(x);
					node->next = curr;
					prev->next = node;
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			shared_ptr<ASPNODE2> prev = head;
			shared_ptr<ASPNODE2> curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					curr->removed = true;
					shared_ptr<ASPNODE2> t = prev;
					shared_ptr<ASPNODE2> t2 = curr->next;
					t->next = t2;
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		shared_ptr<ASPNODE2> curr = head;
		while (curr->v < x)
			curr = curr->next;
		return (x == curr->v) && (false == curr->removed);
	}

	void print20()
	{
		shared_ptr<ASPNODE2> t = head;
		shared_ptr<ASPNODE2> t2 = tail;
		shared_ptr<ASPNODE2> p = t->next;
		for (int i = 0; i < 20; ++i) {
			if (p == tail.load()) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		shared_ptr<ASPNODE2> t = head;
		shared_ptr<ASPNODE2> t2 = tail;
		t->next = t2;
	}
};



constexpr long long ADDR_MARK = 0xFFFFFFFFFFFFFFFE;
class LFNODE {
private:
	LFNODE* next;
	bool CAS(long long old_v, long long new_v)
	{
		return std::atomic_compare_exchange_strong(reinterpret_cast<atomic_llong*>(&next), &old_v, new_v);
	}

public:
	int v;
	LFNODE() : v(-1), next(nullptr) {}
	LFNODE(int x) : v(x), next(nullptr) {}



	LFNODE* get_next()
	{
		return reinterpret_cast<LFNODE*>(reinterpret_cast<long long>(next) & ADDR_MARK);
	}

	LFNODE* get_next(bool* removed)
	{
		long long temp = reinterpret_cast<long long>(next);
		*removed = ((temp & 1) == 1);
		return reinterpret_cast<LFNODE*>(temp & ADDR_MARK);
	}

	bool get_mark()
	{
		long long temp = reinterpret_cast<long long>(next);
		return ((temp & 1) == 1);
	}

	bool attemptMark(LFNODE* next_node, bool newmark)
	{
		long long oldvalue = reinterpret_cast<long long>(next);
		oldvalue = oldvalue & ADDR_MARK;

		long long newalue = reinterpret_cast<long long> (next_node);
		newalue = newalue | 1;

		return CAS(oldvalue, newalue);
	}

	void set_next(LFNODE* p)
	{
		next = p;
	}

	bool CAS(LFNODE* old_node, LFNODE* new_node, bool oldMark, bool newMark)
	{
		long long oldvalue = reinterpret_cast<long long>(old_node);
		if (oldMark) oldvalue = oldvalue | 1;
		//else oldvalue = oldvalue & ADDR_MARK;

		long long newalue = reinterpret_cast<long long> (new_node);
		if (newMark) newalue = newalue | 1;
		//else newalue = newalue & ADDR_MARK;

		return CAS(oldvalue, newalue);
	}
};

class LF_SET {
	LFNODE head, tail;
public:
	LF_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.set_next(&tail);
	}

	~LF_SET()
	{
		clear();
	}

	void find(int x, LFNODE*& prev, LFNODE*& curr)
	{
	retry:
		prev = &head;
		curr = prev->get_next();
		while (true)
		{
			bool removed;
			LFNODE* succ = curr->get_next(&removed);
			while (removed == true) {
				if (prev->CAS(curr, succ, false, false) == false)
					goto retry;
				curr = succ;
				succ = curr->get_next(&removed);
			}
			if (curr->v >= x) return;
			prev = curr;
			curr = succ;
		}
	}

	bool ADD(int x)
	{
		LFNODE* node = new LFNODE{ x };
		while (true) {
			LFNODE* prev, * curr;
			find(x, prev, curr);

			if (curr->v != x) {
				node->set_next(curr);
				if (prev->CAS(curr, node, false, false))
					return true;
			}
			else
			{
				delete node;
				return false;
			}
		}

	}

	bool REMOVE(int x)
	{
		bool flag;
		while (true) {
			LFNODE* prev, * curr;
			find(x, prev, curr);
			if (curr->v != x) {
				return false;
			}
			else
			{
				bool removed;
				removed = curr->get_mark();
				LFNODE* succ = curr->get_next();

				flag = curr->attemptMark(succ, true);
				if (!flag)
					continue;
				prev->CAS(curr, succ, false, false);
				return true;
			}
		}

	}

	bool CONTAINS(int x)
	{
		LFNODE* curr = &head;
		bool removed;
		while (curr->v < x)
		{
			curr = curr->get_next(&removed);
		}
		return curr->v == x && removed == false;
	}

	void print20()
	{
		LFNODE* p = head.get_next();
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->get_next();
		}
		cout << endl;
	}

	void clear()
	{
		/*LFNODE* p = head.get_next();
		while (p != &tail) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;*/
		head.set_next(&tail); // 임시
	}
};

enum class METHOD_TYPE
{
	OP_ADD,
	OP_REMOVE,
	OP_CONTAINS,
	OP_CLEAR,
	OP_EMPTY,
	OP_GET20,
	OP_FRONT
};

struct Invocation
{
	METHOD_TYPE m_op;
	int v;
};

struct Response
{
	bool res;
	std::vector<int> get20;
};

class SeqObject_set
{
public:
	std::set<int> m_set;
public:
	Response apply(Invocation& inv)
	{
		Response res;

		switch (inv.m_op)
		{
		case METHOD_TYPE::OP_ADD:
			res.res = (m_set.count(inv.v) == 0);
			if (res.res == true)
				m_set.insert(inv.v);
			break;
		case METHOD_TYPE::OP_REMOVE:
			res.res = (m_set.count(inv.v) == 1);
			if (res.res == true)
				m_set.erase(inv.v);
			break;
		case METHOD_TYPE::OP_CONTAINS:
			res.res = (m_set.count(inv.v) == 1);
			break;
		case METHOD_TYPE::OP_CLEAR:
			m_set.clear();
			break;
		case METHOD_TYPE::OP_EMPTY:
			res.res = m_set.empty();
			break;
		case METHOD_TYPE::OP_GET20:
		{
			int count = 20;
			for (auto v : m_set) {
				if (count < 0)
					break;
				res.get20.emplace_back(v);
				count--;
			}
		}
			break;
		default:
			cout << "Unknown Method!" << endl;
			exit(-1);
			break;
		}

		return res;
	}
};


class UNODE;
class Consensus
{
	UNODE* ptr = nullptr;
public:
	Consensus() : ptr{ nullptr } {};
	bool CAS(UNODE* old_p, UNODE* new_p)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_llong*>(&ptr), reinterpret_cast<long long*>(&old_p), reinterpret_cast<long long>(new_p));
	}

	UNODE* decide(UNODE* v)
	{
		CAS(nullptr, v);
		return ptr;
	}
};



class STD_SET {
	SeqObject_set std_set;
public:
	STD_SET()
	{

	}
	bool ADD(int x)
	{
		Invocation inv(METHOD_TYPE::OP_ADD, x);
		Response a = std_set.apply(inv);
		return a.res;
	}

	bool REMOVE(int x)
	{
		Invocation inv(METHOD_TYPE::OP_REMOVE, x);
		Response a = std_set.apply(inv);
		return a.res;
	}

	bool CONTAINS(int x)
	{
		Invocation inv(METHOD_TYPE::OP_CONTAINS, x);
		Response a = std_set.apply(inv);
		return a.res;
	}
	void print20()
	{
		int count = 20;
		for (auto& p : std_set.m_set) {
			cout << p << ", ";
			count--;
			if (count == 0) break;
		}

		cout << endl;
	}

	void clear()
	{
		Invocation inv(METHOD_TYPE::OP_CLEAR, 0);
		Response a = std_set.apply(inv);
	}
};

thread_local int tl_id;

int Thread_id()
{
	return tl_id;
}

class UNODE
{
public:
	Invocation inv;
	UNODE* next;
	Consensus decided_next;
	volatile int seq;
	UNODE() : next{ nullptr }, seq{ 0 } {};
	UNODE(Invocation& v) : inv{ v } {};
};

class LF_UNIV_STD_SET {
private:
	UNODE* head[MAX_THREADS];
	UNODE tail;
public:
	LF_UNIV_STD_SET() {
		tail.seq = 0;
		for (int i = 0; i < MAX_THREADS; ++i) head[i] = &tail;
	}
	UNODE* max_node()
	{
		UNODE* max_node = head[0];
		for (int i = 1; i < MAX_THREADS; ++i)
			if (head[i]->seq > max_node->seq)
				max_node = head[i];

		return max_node;
	}

	Response apply(Invocation invoc) {
		int i = Thread_id();
		UNODE* prefer = new UNODE(invoc);
		while (prefer->seq == 0) {
			UNODE* before = max_node();
			UNODE* after = before->decided_next.decide(prefer);
			before->next = after; after->seq = before->seq + 1;
			head[i] = after;
		}
		SeqObject_set myObject;
		UNODE* current = tail.next;
		while (current != prefer) {
			myObject.apply(current->inv);
			current = current->next;
		}
		return myObject.apply(current->inv);
	}};
class LF_STD_SET {
	LF_UNIV_STD_SET std_set;
public:
	LF_STD_SET()
	{

	}
	bool ADD(int x)
	{
		Invocation inv(METHOD_TYPE::OP_ADD, x);
		Response a = std_set.apply(inv);
		return a.res;
	}

	bool REMOVE(int x)
	{
		Invocation inv(METHOD_TYPE::OP_REMOVE, x);
		Response a = std_set.apply(inv);
		return a.res;
	}

	bool CONTAINS(int x)
	{
		Invocation inv(METHOD_TYPE::OP_CONTAINS, x);
		Response a = std_set.apply(inv);
		return a.res;
	}
	void print20()
	{
		Invocation inv{ METHOD_TYPE::OP_GET20, 0 };
		Response a = std_set.apply(inv);

		for (auto n : a.get20)
			cout << n << ", ";

		cout << endl;
	}

	void clear()
	{
		Invocation inv(METHOD_TYPE::OP_CLEAR, 0);
		Response a = std_set.apply(inv);
	}
};

class WF_UNIV_STD_SET {
private:
	UNODE* announce[MAX_THREADS];
	UNODE* head[MAX_THREADS];
	UNODE tail;
public:
	WF_UNIV_STD_SET() {
		tail.seq = 0;
		for (int i = 0; i < MAX_THREADS; ++i) {
			head[i] = &tail;
			announce[i] = &tail;
		}
	}

	UNODE* max_node()
	{
		UNODE* max_node = head[0];
		for (int i = 1; i < MAX_THREADS; ++i)
			if (head[i]->seq > max_node->seq)
				max_node = head[i];

		return max_node;
	}

	Response apply(Invocation invoc) {
		int i = Thread_id();
		announce[i] = new UNODE(invoc);
		UNODE* prefer = new UNODE(invoc);
		head[i] = max_node();
		while (prefer->seq == 0) {
			UNODE* before = max_node();
			UNODE* after = before->decided_next.decide(prefer);
			before->next = after; after->seq = before->seq + 1;
			head[i] = after;
		}
		SeqObject_set myObject;
		UNODE* current = tail.next;
		while (current != prefer) {
			myObject.apply(current->inv);
			current = current->next;
		}
		return myObject.apply(current->inv);
	}};

#define MY_SET LF_SET

//SET my_set;   // 성긴 동기화
//F_SET my_set;   // 세밀한 동기화
//O_SET my_set;	// 낙천적 동기화
//LF_SET my_set;	// 게으른 동기화
//LASP3_SET my_set;	// c++ atomic_shared_ptr 비멈춤 동기화
//LF_STD_SET my_set;	// 


class HISTORY {
public:
	int op;
	int i_value;
	bool o_value;
	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re) {}
};

constexpr int RANGE = 1000;
constexpr int N = 4000;

void worker(MY_SET* my_set, vector<HISTORY>* history, int num_threads, int thread_id)
{
	tl_id = thread_id;
	for (int i = 0; i < N / num_threads; ++i) {
		int op = rand() % 3;
		switch (op) {
		case 0: {
			int v = rand() % RANGE;
			my_set->ADD(v);
			break;
		}
		case 1: {
			int v = rand() % RANGE;
			my_set->REMOVE(v);
			break;
		}
		case 2: {
			int v = rand() % RANGE;
			my_set->CONTAINS(v);
			break;
		}
		}
	}
}

void worker_check(MY_SET* my_set, vector<HISTORY>* history, int num_threads, int thread_id)
{
	tl_id = thread_id;
	for (int i = 0; i < N / num_threads; ++i) {
		int op = rand() % 3;
		switch (op) {
		case 0: {
			int v = rand() % RANGE;
			history->emplace_back(0, v, my_set->ADD(v));
			break;
		}
		case 1: {
			int v = rand() % RANGE;
			history->emplace_back(1, v, my_set->REMOVE(v));
			break;
		}
		case 2: {
			int v = rand() % RANGE;
			history->emplace_back(2, v, my_set->CONTAINS(v));
			break;
		}
		}
	}
}

void check_history(MY_SET* my_set, array <vector <HISTORY>, MAX_THREADS>& history, int num_threads)
{
	array <int, RANGE> survive = {};
	cout << "Checking Consistency : ";
	if (history[0].size() == 0) {
		cout << "No history.\n";
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
	for (int i = 0; i < RANGE; ++i) {
		int val = survive[i];
		if (val < 0) {
			cout << "ERROR. The value " << i << " removed while it is not in the set.\n";
			exit(-1);
		}
		else if (val > 1) {
			cout << "ERROR. The value " << i << " is added while the set already have it.\n";
			exit(-1);
		}
		else if (val == 0) {
			if (my_set->CONTAINS(i)) {
				cout << "ERROR. The value " << i << " should not exists.\n";
				exit(-1);
			}
		}
		else if (val == 1) {
			if (false == my_set->CONTAINS(i)) {
				cout << "ERROR. The value " << i << " shoud exists.\n";
				exit(-1);
			}
		}
	}
	cout << " OK\n";
}

int main()
{
	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		vector <thread> threads;
		array<vector <HISTORY>, MAX_THREADS> history;
		MY_SET my_set;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker_check, &my_set, &history[i], num_threads, i);
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_set.print20();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
		check_history(&my_set, history, num_threads);
	}

	cout << "======== SPEED CHECK =============\n";

	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		vector <thread> threads;
		array<vector <HISTORY>, MAX_THREADS> history;
		MY_SET my_set;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker, &my_set , &history[i], num_threads, i);
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_set.print20();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
		check_history(&my_set, history, num_threads);
	}
}