#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_map.h>
#include <tbb/parallel_for.h>
#include <vector>
#include <string> 
#include <fstream>
#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <locale>
#include <mutex>

using namespace tbb;
using namespace std;
using namespace chrono;

// Structure that defines hashing and comparison operations for user's type. 
typedef concurrent_unordered_map<string, int> StringTable;

int main()
{
	vector<string> Data;
	vector<string> Original;
	StringTable table;
	locale loc;

	cout << "Loading!\n";
	ifstream openFile("TextBook.txt");
	if (openFile.is_open()) {
		string word;
		while (false == openFile.eof()) {
			openFile >> word;
			if (isalpha(word[0], loc))
				Original.push_back(word);
		}
		openFile.close();
	}

	cout << "Loaded Total " << Original.size() << " Words.\n";

	cout << "Duplicating!\n";

	for (int i = 0; i < 1000; ++i)
		for (auto& word : Original) Data.push_back(word);

	cout << "Counting!\n";

	auto start = high_resolution_clock::now();
	parallel_for(size_t(0), Data.size(), [&table, &Data](int i) {
		(*reinterpret_cast<atomic_int*>(&table[Data[i]]))++;
		});

	auto du = high_resolution_clock::now() - start;

	int count = 0;
	for (StringTable::iterator i = table.begin(); i != table.end(); ++i) {
		if (count++ > 10) break;
		printf("[%s %d], ", i->first.c_str(), i->second);
	}

	cout << "\nParallel_For Time : " << duration_cast<milliseconds>(du).count() << endl;

	unordered_map<string, int> SingleTable;

	start = high_resolution_clock::now();
	
	for (auto& word : Data) SingleTable[word]++;
	du = high_resolution_clock::now() - start;

	count = 0;
	for (auto& item : SingleTable) {
		if (count++ > 10) break;
		printf("[%s %d],", item.first.c_str(), item.second);
	}

	cout << "\nSingle Thread Time : " << duration_cast<milliseconds>(du).count() << endl;

	StringTable table2;
	
	vector<thread> threads;
	int size = static_cast<int>(Data.size());
	int num_th = thread::hardware_concurrency();
	start = high_resolution_clock::now();
	for (int i = 0; i < num_th; ++i) threads.emplace_back([&, i]() {
		int task = Data.size() / num_th;
		for (int j = 0; j < task; ++j) {
			int loc = j + i * task;
			if (loc >= size) break;
			(*reinterpret_cast<atomic_int*>(&table2[Data[loc]]))++;
			//table2[Data[loc]]++;
		};
		});
	for (auto& th : threads) th.join();
	du = high_resolution_clock::now() - start;

	count = 0;
	for (auto& item : table2) {
		if (count++ > 10) break;
		printf("[%s %d],", item.first.c_str(), item.second);
	}

	cout << "\nConcurrent_Unordered_Map Thread Time : " << duration_cast<milliseconds>(du).count() << endl;


	{
		concurrent_hash_map<string, int> table;
		int size = static_cast<int>(Data.size());
		int num_th = thread::hardware_concurrency();
		start = high_resolution_clock::now();
		parallel_for(size_t(0), Data.size(), [&table, &Data](int i) {
			concurrent_hash_map<string, int>::accessor p;
			table.insert(p, Data[i]);
			p->second++;
			});
		du = high_resolution_clock::now() - start;

		count = 0;
		for (auto& item : table2) {
			if (count++ > 10) break;
			printf("[%s %d],", item.first.c_str(), item.second);
		}

		cout << "\TBB concurrent_Hash_Map Thread Time : " << duration_cast<milliseconds>(du).count() << endl;
	}
	
	{
		concurrent_map<string, int> table;
		auto start = high_resolution_clock::now();
		parallel_for(size_t(0), Data.size(), [&table, &Data](int i) {
			(*reinterpret_cast<atomic_int*>(&table[Data[i]]))++;
			});

		auto du = high_resolution_clock::now() - start;

		int count = 0;
		for (concurrent_map<string, int>::iterator i = table.begin(); i != table.end(); ++i) {
			if (count++ > 10) break;
			printf("[%s %d], ", i->first.c_str(), i->second);
		}

		cout << "\nTBB concurrent_Map Time : " << duration_cast<milliseconds>(du).count() << endl;
	}

	system("pause");
}
