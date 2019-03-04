// practiceCpp.cpp: 定义应用程序的入口点。
//

#include "multiThread.h"
#include <thread>
#include <atomic>
#include <vector>
#include <chrono> 

using namespace std;

void foo()
{
	cout << "foo is called" << endl;
}

void bar(int x) {
	std::cout << "bar is called" <<endl;
}

std::atomic<int> global_counter(0);

void increase_global(int n)
{ 
	for (int i = 0; i<n; ++i) 
		++global_counter;
}

void increase_reference(std::atomic<int>& variable, int n)
{ 
	for (int i = 0;   i<n; ++i)
		++variable;
}

struct C : std::atomic<int> {
	C() : std::atomic<int>(0) {}
	void increase_member(int n)
	{
		for (int i = 0; i<n; ++i)
			fetch_add(1);
	}
};


void pause_thread(int n)
{
	std::this_thread::sleep_for(std::chrono::seconds(n));
	std::cout << "pause of " << n << " seconds ended\n";
}

std::thread::id main_thread_id = std::this_thread::get_id();

void is_main_thread() {
	if (main_thread_id == std::this_thread::get_id())
		std::cout << "This is the main thread.\n";
	else
		std::cout << "This is not the main thread.\n";
}

void mythread()
{
	// do stuff...
}

int main()
{
	//createThreadDemo();
	//atomicDemo();
	//sleepDemo();
	//threadIdDemo();
	//joinableDemo();
	//joinDemo();
	//detachDemo();
	return 0;
}

/*
detach分离出调用线程对象所代表的线程，允许它们彼此独立地执行;这两个线程在任何方式上都不阻塞或同步；注意，当一个结束执行时，它的资源被释放。在调用此函数之后，线程对象变得不可连接，可以安全地销毁。
*/
void detachDemo()
{
	std::cout << "Spawning and detaching 3 threads...\n";
	std::thread(pause_thread, 1).detach();
	std::thread(pause_thread, 2).detach();
	std::thread(pause_thread, 3).detach();
	std::cout << "Done spawning threads.\n";

	std::cout << "(the main thread will now pause for 5 seconds)\n";
	// give the detached threads time to finish (but not guaranteed!):
	pause_thread(5);
}
/*
join 函数在线程执行完成的时候返回；此函数在函数返回时与线程中所有操作的完成是同步的；调用join直到join被构造函数调用返回间，阻塞调用的线程；在调用此函数之后，线程对象变得不可连接，可以安全地销毁。
*/
void joinDemo()
{
	std::cout << "Spawning 3 threads...\n";
	std::thread t1(pause_thread, 1);
	std::thread t2(pause_thread, 2);
	std::thread t3(pause_thread, 3);
	std::cout << "Done spawning threads. Now waiting for them to join:\n";
	t1.join();
	t2.join();
	t3.join();
	std::cout << "All threads joined!\n";
}

/*
std::thread::joinable
返回线程对象是否可joinable。
如果线程对象表示执行的线程，则是可joinable。
在这些情况下，一个线程对象是不可连接的:

如果是默认构造。
如果它已经被移动(或者构造另一个线程对象，或者分配给它)。
如果它的成员加入或分离被调用。
*/
void joinableDemo()
{
	std::thread foo;
	std::thread bar(mythread);

	std::cout << "Joinable after construction:\n" << std::boolalpha;
	std::cout << "foo: " << foo.joinable() << '\n';
	std::cout << "bar: " << bar.joinable() << '\n';

	if (foo.joinable()) foo.join();
	if (bar.joinable()) bar.join();

	std::cout << "Joinable after joining:\n" << std::boolalpha;
	std::cout << "foo: " << foo.joinable() << '\n';
	std::cout << "bar: " << bar.joinable() << '\n';
}

/*
如果线程对象是joinable，函数将返回唯一标识线程的值。
如果线程对象不可joinable，函数将返回成员类型线程的默认构造对象:id。
*/
void threadIdDemo()
{
	is_main_thread();
	std::thread th(is_main_thread);
	th.join();
}

void sleepDemo()
{
	std::thread threads[5];                         // default-constructed threads

	std::cout << "Spawning 5 threads...\n";
	for (int i = 0; i<5; ++i)
		threads[i] = std::thread(pause_thread, i + 1);   // move-assign threads

	std::cout << "Done spawning threads. Now waiting for them to join:\n";
	for (int i = 0; i<5; ++i)
		threads[i].join();

	std::cout << "All threads joined!\n";
}

void atomicDemo()
{
	std::vector<std::thread> threads;
	std::cout << "increase global counter with 10 threads...\n";
	for (int i = 1; i <= 10; ++i)
		threads.push_back(std::thread(increase_global, 1000));
	std::cout << "increase counter (foo) with 10 threads using   reference...\n";
	std::atomic<int> foo(0);
	for (int i = 1; i <= 10; ++i)
	{
		threads.push_back(std::thread(increase_reference, std::ref(foo), 1000));
	}
	std::cout << "increase counter (bar) with 10 threads using member...\n";
	C bar;
	for (int i = 1; i <= 10; ++i)
	{
		threads.push_back(std::thread(&C::increase_member, std::ref(bar), 1000));
	}
	std::cout << "synchronizing all threads...\n";
	for (auto& th : threads) th.join();

	std::cout << "global_counter: " << global_counter << '\n';
	std::cout << "foo: " << foo << '\n';
	std::cout << "bar: " << bar << '\n';
}

void createThreadDemo()
{
	thread first(foo);// spawn new thread that calls foo()
	thread second(bar, 0);// spawn new thread that calls bar(0)
	std::cout << "main, foo and bar now execute concurrently...\n";
	// synchronize threads:
	first.join();                // pauses until first finishes
	second.join();              // pauses until second finishes
	std::cout << "foo and bar completed.\n";
}
