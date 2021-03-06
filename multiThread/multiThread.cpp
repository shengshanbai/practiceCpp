// practiceCpp.cpp: 定义应用程序的入口点。
//

#include "multiThread.h"
#include <thread>
#include <atomic>
#include <vector>
#include <chrono> 
#include <mutex>
#include <ctime>
#include <string>

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

std::mutex mtx;

void print_thread_id(int id) {
	// critical section (exclusive access to std::cout signaled by locking mtx):
	mtx.lock();
	std::cout << "thread #" << id << '\n';
	mtx.unlock();
}

volatile int counter(0); // non-atomic counter

void attempt_10k_increases() {
	for (int i = 0; i<10000; ++i) {
		if (mtx.try_lock()) {   // only increase if currently not locked:
			++counter;
			mtx.unlock();
		}
	}
}

timed_mutex tmtx;

void fireworks() {
	// waiting to get a lock: each thread prints "-" every 200ms:
	while (!tmtx.try_lock_for(std::chrono::milliseconds(2))) {
		std::cout << "-";
	}
	// got a lock! - wait for 1s, then this thread prints "*"
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "*\n";
	tmtx.unlock();
}

std::timed_mutex cinderella;
// gets time_point for next midnight:
std::chrono::time_point<std::chrono::system_clock> midnight() {
	using std::chrono::system_clock;
	std::time_t tt = system_clock::to_time_t(system_clock::now());
	struct std::tm * ptm = std::localtime(&tt);
	++ptm->tm_mday; ptm->tm_hour = 0; ptm->tm_min = 0; ptm->tm_sec = 0;
	return system_clock::from_time_t(mktime(ptm));
}

void carriage() {
	if (cinderella.try_lock_until(midnight())) {
		std::cout << "ride back home on carriage\n";
		cinderella.unlock();
	}
	else
		std::cout << "carriage reverts to pumpkin\n";
}

void ball() {
	cinderella.lock();
	std::cout << "at the ball...\n";
	cinderella.unlock();
}

void print_thread_id2(int id) {
	mtx.lock();
	std::lock_guard<std::mutex> lck(mtx, std::adopt_lock);
	std::cout << "thread #" << id << '\n';
}

std::mutex foomtx, barmtx;

void task_a() {
	std::lock(foomtx, barmtx);         // simultaneous lock (prevents deadlock)
	std::unique_lock<std::mutex> lck1(foomtx, std::adopt_lock);
	std::unique_lock<std::mutex> lck2(barmtx, std::adopt_lock);
	std::cout << "task a\n";
	// (unlocked automatically on destruction of lck1 and lck2)
}

void task_b() {
	// foo.lock(); bar.lock(); // replaced by:
	std::unique_lock<std::mutex> lck1, lck2;
	lck1 = std::unique_lock<std::mutex>(barmtx, std::defer_lock);
	lck2 = std::unique_lock<std::mutex>(foomtx, std::defer_lock);
	std::lock(lck1, lck2);       // simultaneous lock (prevents deadlock)
	std::cout << "task b\n";
	// (unlocked automatically on destruction of lck1 and lck2)
}

int winner;
void set_winner(int x) { winner = x; }
std::once_flag winner_flag;
void wait_1000ms(int id) {
	// count to 1000, waiting 1ms between increments:
	for (int i = 0; i<1000; ++i)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	// claim to be the winner (only the first such call is executed):
	std::call_once(winner_flag, set_winner, id);
}

/*
call_once
std::call_once 公有模版函数
template <class Fn, class... Args>
void call_once (once_flag& flag, Fn&& fn, Args&&... args);
call_once调用将args 作为fn的参数调用fn，除非另一个线程已经(或正在执行)使用相同的flag调用执行call_once。如果已经有一个线程使用相同flag调用call_once,会使得当前变为被动执行，所谓被动执行不执行fn也不返回直到恢复执行后返回。这这个时间点上所有的并发调用这个函数相同的flag都是同步的。
注意,一旦一个活跃调用返回了,所有当前被动执行和未来可能的调用call_once相同相同的flag也还不会成为积极执行。
*/
void callOnceDemo()
{
	std::thread threads[10];
	// spawn 10 threads:
	for (int i = 0; i<10; ++i)
		threads[i] = std::thread(wait_1000ms, i + 1);

	std::cout << "waiting for the first among 10 threads to count 1000   ms...\n";

	for (auto& th : threads) th.join();
	std::cout << "winner thread: " << winner << '\n';
}

/*
类 unique_lock 是通用互斥包装器，允许延迟锁定、锁定的有时限尝试、递归锁定、所有权转移和与条件变量一同使用。
unique_lock比lock_guard使用更加灵活，功能更加强大。
使用unique_lock需要付出更多的时间、性能成本。
构造函数(constructor):
default (1)  unique_lock() noexcept;
locking (2)  explicit unique_lock (mutex_type& m);
try-locking (3)  unique_lock (mutex_type& m, try_to_lock_t tag);
deferred (4) unique_lock (mutex_type& m, defer_lock_t tag) noexcept;
adopting (5) unique_lock (mutex_type& m, adopt_lock_t tag);
locking for (6) template <class Rep, class Period>
unique_lock (mutex_type& m, const chrono::duration<Rep,Period>& rel_time);
locking until (7) template <class Clock, class Duration>
unique_lock (mutex_type& m, const chrono::time_point<Clock,Duration>& abs_time);
copy [deleted] (8)  unique_lock (const unique_lock&) = delete;
move (9)  unique_lock (unique_lock&& x);
下面我们来分别介绍以上各个构造函数：
(1) 默认构造函数
unique_lock 对象不管理任何 Mutex 对象m。
(2) locking 初始化
unique_lock 对象管理 Mutex 对象 m，并调用 m.lock() 对 Mutex 对象进行上锁，如果其他 unique_lock 对象已经管理了m，该线程将会被阻塞。
(3) try-locking 初始化
unique_lock 对象管理 Mutex 对象 m，并调用 m.try_lock() 对 Mutex 对象进行上锁，但如果上锁不成功，不会阻塞当前线程。
(4) deferred 初始化
unique_lock 对象管理 Mutex 对象 m并不锁住m。 m 是一个没有被当前线程锁住的 Mutex 对象。
(5) adopting 初始化
unique_lock 对象管理 Mutex 对象 m， m 应该是一个已经被当前线程锁住的 Mutex 对象。(当前unique_lock 对象拥有对锁(lock)的所有权)。
(6) locking 一段时间(duration)
新创建的 unique_lock 对象管理 Mutex 对象 m，通过调用 m.try_lock_for(rel_time) 来锁住 Mutex 对象一段时间(rel_time)。
(7) locking 直到某个时间点(time point)
新创建的 unique_lock 对象管理 Mutex 对象m，通过调用 m.try_lock_until(abs_time) 来在某个时间点(abs_time)之前锁住 Mutex 对象。
(8) 拷贝构造 [被禁用]
unique_lock 对象不能被拷贝构造。
(9) 移动(move)构造
新创建的 unique_lock 对象获得了由 x 所管理的 Mutex 对象的所有权(包括当前 Mutex 的状态)。调用 move 构造之后， x 对象如同通过默认构造函数所创建的，就不再管理任何 Mutex 对象了。
*/
void uniqueLockDemo()
{
	std::thread th1(task_a);
	std::thread th2(task_b);
	th1.join();
	th2.join();
}
/*
lock_guard自动管理上锁和解锁，
构造函数(constructor)
locking (1)  explicit lock_guard (mutex_type& m);
adopting (2)      lock_guard (mutex_type& m, adopt_lock_t tag);
copy deleted    lock_guard (const lock_guard&) = delete;
(1)锁定初始化：对象管理m，并锁定它(通过调用m.lock())。
(2)采用初始化：对象管理m，它是已经当前被构造线程锁定的互斥对象。
(3)复制构造： 删除(lock_guard对象不能被复制/移动)。
注：lock_guard的对象保持锁定并管理m(通过调用它的成员解锁)来解锁它，当lock_guard的对象析构的时候，mtx将会被解锁。
*/
void lockguardDemo()
{
	std::thread threads[10];
	// spawn 10 threads:
	for (int i = 0; i<10; ++i)
		threads[i] = std::thread(print_thread_id, i + 1);

	for (auto& th : threads) th.join();
}

void timeUtilDemo()
{
	std::thread th1(ball);
	std::thread th2(carriage);
	th1.join();
	th2.join();
}

/*
timed_mutex
timed_mutex锁比较mutex所多了两个成员函数try_lock_for 和 try_lock_until。


try_lock_for
传入时间段，在时间范围内未获得所就阻塞住线程，如果在此期间其他线程释放了锁，则该线程可以获得对互斥量的锁，如果超时，则返回 false。

try_lock_until
同上面的解释，只是传入参数为一个未来的一个时间点。
*/
void timeMutexDemo()
{
	std::thread threads[10];
	// spawn 10 threads:
	for (int i = 0; i<10; ++i) {
		threads[i] = std::thread(fireworks);
	}
	for (auto& th : threads) th.join();
}

/*
try_lock 试图锁定互斥锁，不阻塞。
若互斥对象当前没有被任何线程锁定，则调用线程锁定它；
若互斥锁当前被另一个线程锁定，则该函数失败并返回false，没有阻塞；若互斥锁当前被相同的线程所锁定，调用此函数，则会产生死锁。
*/
void tryLockDemo()
{
	std::thread threads[10];
	// spawn 10 threads:
	for (int i = 0; i<10; ++i)
		threads[i] = std::thread(attempt_10k_increases);
	for (auto& th : threads) th.join();
	std::cout << counter << " successful increases of the counter.\n";
}

/*
lock 锁住互斥对象

1,如果互斥对象当前没有被任何线程锁定，则调用线程锁定它(从这一点开始，直到它的成员解锁被调用，线程拥有互斥对象)。
2,如果互斥锁当前被另一个线程锁定，则调用线程的执行将被阻塞，直到其他线程解锁(其他非锁定线程继续执行它们)。
3,如果互斥锁当前被相同的线程所锁定，调用此函数，则会产生死锁(未定义的行为)。
*/
void mutexDemo()
{
	std::thread threads[10];
	// spawn 10 threads:
	for (int i = 0; i<10; ++i)
		threads[i] = std::thread(print_thread_id, i + 1);

	for (auto& th : threads) th.join();
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

std::atomic<bool> ready(false);
std::atomic_flag winnerFlag = ATOMIC_FLAG_INIT;

void count1m(int id) {
	while (!ready) { std::this_thread::yield(); }      // wait for the ready signal
	for (volatile int i = 0; i<1000000; ++i) {}          // go!, count to 1 million
	//test_and_set是返回先前值
	if (!winnerFlag.test_and_set()) { std::cout << "thread #" << id << " won!\n"; }
};

void atomicFlagDemo() {
	std::vector<std::thread> threads;
	std::cout << "spawning 10 threads that count to 1 million...\n";
	for (int i = 1; i <= 10; ++i) threads.push_back(std::thread(count1m, i));
	ready = true;
	for (auto& th : threads) th.join();
}

struct A { int a[100]; };
struct B { int x, y; };

void lockFreeDemo()
{
	std::cout << std::boolalpha
		<< "std::atomic<A> is lock free? "
		<< std::atomic<A>{}.is_lock_free() << '\n'
		<< "std::atomic<B> is lock free? "
		<< std::atomic<B>{}.is_lock_free() << '\n';
}

std::atomic<int> fooS(0);

void set_foo(int x) {
	fooS.store(x, std::memory_order_relaxed);     // set value atomically
}

void print_foo() {
	int x;
	do {
		x = fooS.load(std::memory_order_relaxed);  // get value atomically
	} while (x == 0);
	std::cout << "foo: " << x << '\n';
}

/*
std::atomic::store
void store (T val, memory_order sync = memory_order_seq_cst) volatile noexcept;
void store (T val, memory_order sync = memory_order_seq_cst) noexcept;
用val替换包含的值。操作是原子的，按照同步所指定的内存顺序内存数序包括（std::memory_order_relaxed, std::memory_order_release 和std::memory_order_seq_cst）。
参数sync的描述(后续会介绍memory_order)：
memory_order_relaxed： 不同步副作用。
memory_order_release：同步下一个使用或者获取操作的副作用。
memory_order_seq_cst：同步所有与其他顺序一致操作的可见的副作用，并遵循一个完整的顺序。
std::atomic::load
T load (memory_order sync = memory_order_seq_cst) const volatile noexcept;
T load (memory_order sync = memory_order_seq_cst) const noexcept;
返回包含值。操作是原子的，按照同步所指定的内存顺序。指令必须是std::memory_order_relaxed, std::memory_order_consume, std::memory_order_acquire 和 std::memory_order_seq_cst)；否则，行为是没有定义的。
sync指令描述：
上文已经描述了std::memory_order_relaxed和 std::memory_order_seq_cst，这里只描述memory_order_acquire和memory_order_consume。
memory_order_acquire：同步从最后一个Release或顺序一致的操作所有可见的副作用。
memory_order_consume：同步与最后一个Release或顺序一致的操作所产生的依赖关系的可见的副作用。
*/
void relaxedOrderDemo()
{
	std::thread first(print_foo);
	std::thread second(set_foo, 10);
	first.join();
	second.join();
}

// a simple global linked list:
struct Node { int value; Node* next; };
std::atomic<Node*> list_head(nullptr);

void append(int val) {     // append an element to the list
	Node* oldHead = list_head;
	Node* newNode = new Node{ val,oldHead };

	// what follows is equivalent to: list_head = newNode, but in a thread-safe way:
	while (!list_head.compare_exchange_weak(oldHead, newNode))
		newNode->next = oldHead;
}
/*
std::atomic::compare_exchange_weak 和std::atomic::compare_exchange_strong
compare_exchange_weak()是C++11中提供的compare-exchange原语之一。之所以是weak，是因为即使在对象的值等于expected的情况下，也返回false。这是因为在某些平台上的spurious failure，这些平台使用了一系列的指令（而不是像在x86上一样，使用单条的指令）来实现CAS。在这种平台上，context switch, reloading of the same address (or cache line) by another thread等，将会导致这条原语失败。由于不是因为对象的值（不等于expected）导致的操作失败，因此是spurious。相反的，it’s kind of timing issues。
不使用strong版本是因为它的开销会很大。
*/

int exchangeDemo()
{
	// spawn 10 threads to fill the linked list:
	std::vector<std::thread> threads;
	for (int i = 0; i<10; ++i) threads.push_back(std::thread(append, i));
	for (auto& th : threads) th.join();

	// print contents:
	for (Node* it = list_head; it != nullptr; it = it->next)
		std::cout << ' ' << it->value;
	std::cout << '\n';

	// cleanup:
	Node* it; while (it = list_head) { list_head = it->next; delete it; }

	return 0;
}

std::atomic<long long> dataL;
void do_work()
{
	dataL.fetch_add(1, std::memory_order_relaxed);
}

/*
fetch_add
fetch_sub
特定的操作支持(整形和指针)
*将val加或者减去到包含的值并返回在操作之前的值。
*整个操作是原子的(一个原子的读-修改-写操作):当在这个函数被修改的时候,读取的(返回)值被读取，值不受其他线程的影响。
*这个成员函数是对整数(1)和指针(2)类型(除了bool除外)的原子专门化中定义。
*如果第二个参数使用默认值，则该函数等价于原子::运算符+ =。
*/
int fetchAddDemo()
{
	std::thread th1(do_work);
	std::thread th2(do_work);
	std::thread th3(do_work);
	std::thread th4(do_work);
	std::thread th5(do_work);

	th1.join();
	th2.join();
	th3.join();
	th4.join();
	th5.join();
	std::cout << "Result:" << dataL << '\n';
	return 0;
}

std::atomic_flag lockF = ATOMIC_FLAG_INIT;

void f(int n)
{
	for (int cnt = 0; cnt < 100; ++cnt) {
		while (lockF.test_and_set(std::memory_order_acquire))  // acquire lock
			; // spin
		std::cout << "Output from thread " << n << '\n';
		lockF.clear(std::memory_order_release);               // release lock
	}
}

/*
atomic_flag是一个原子布尔类型。不同于std::atomic的所有专门化，它保证是lock_free。不像std::stomic< bool >，std::atomic_flag不提供负载或存储操作。
test_and_set
* 设置atomic_flag并返回是否在调用之前已经设置的。
* 整个操作是原子的(一个原子的读-修改-写操作):当在这个函数被修改的时候,读取的(返回)值被读取，值不受其他线程的影响。
*/
int flagDemo()
{
	std::vector<std::thread> v;
	for (int n = 0; n < 10; ++n) {
		v.emplace_back(f, n);
	}
	for (auto& t : v) {
		t.join();
	}
	return 0;
}

#include <condition_variable>
std::condition_variable cv;
bool readyC = false;

void print_id(int id) {
	std::unique_lock<std::mutex> lck(mtx);
	while (!readyC) cv.wait(lck);
	// ...
	std::cout << "thread " << id << '\n';
}

void go() {
	std::unique_lock<std::mutex> lck(mtx);
	readyC = true;
	cv.notify_all();
}
/*
class condition_variable;
* 条件变量类是一个同步原语，它可以用来阻塞一个线程或多个线程，直到另一个线程同时修改一个共享变量(条件)，并通知条件变量。
* 条件变量是能够阻塞调用线程的对象，直到通知恢复为止。
它使用unique_lock(在互斥锁上)来锁定线程，当它的一个等待函数被调用时。线程仍然被阻塞，直到被另一个线程唤醒，该线程调用同一个条件变量对象上的通知函数。
* 类型条件变量的对象总是使用unique_lock < mutex >等待:对于可以使用任何类型的可锁定类型的选项，参见condition_variable_any。
*/
int conditionDemo()
{
	std::thread threads[10];
	// spawn 10 threads:
	for (int i = 0; i<10; ++i)
		threads[i] = std::thread(print_id, i);

	std::cout << "10 threads ready to race...\n";
	go();                       // go!

	for (auto& th : threads) th.join();

	return 0;
}

std::mutex m;
std::string dataC;
bool processed = false;

void worker_thread()
{
	// Wait until main() sends data
	std::unique_lock<std::mutex> lk(m);
	cv.wait(lk, [] {return readyC; });

	// after the wait, we own the lock.
	std::cout << "Worker thread is processing data\n";
	dataC += " after processing";

	// Send data back to main()
	processed = true;
	std::cout << "Worker thread signals data processing completed\n";

	// Manual unlocking is done before notifying, to avoid waking up
	// the waiting thread only to block again (see notify_one for details)
	lk.unlock();
	cv.notify_one();
}

int condition2Demo()
{
	std::thread worker(worker_thread);

	dataC = "Example data";
	// send data to the worker thread
	{
		std::lock_guard<std::mutex> lk(m);
		ready = true;
		std::cout << "main() signals data ready for processing\n";
	}
	cv.notify_one();

	// wait for the worker
	{
		std::unique_lock<std::mutex> lk(m);
		cv.wait(lk, [] {return processed; });
	}
	std::cout << "Back in main(), data = " << dataC << '\n';
	worker.join();
	return 0;
}

int value;

void read_value() {
	std::cin >> value;
	cv.notify_one();
}
/*
std::condition_variable::wait_for
1、当前执行线程在rel_time时间内或者在被通知之前(该线程将锁定lck的互斥锁)被阻塞(如果后者先发生的话)。
2、在阻塞线程的时刻，该函数将自动调用lck.解锁()，允许其他锁定的线程继续运行。
3、一旦通知或一次rel_time时间段已经过了，函数将会unblocks并调用lck. lock()，将lck与调用函数时的状态保持一致。然后函数返回(注意，最后一个互斥锁可能会在返回之前阻塞线程)。
4、一般情况下，函数会被另一个线程的调用唤醒，无论是对成员notify_one还是成员notify_all。但是某些实现可能产生虚假的唤醒调用，而不需要调用这些函数。因此，该函数的用户将确保满足恢复的条件。
5、如果指定pred(定义2)，如果pred返回false，则函数只会阻塞，并且只有当它变为true时，通知才能解除线程(这对于检查伪唤醒调用特别有用)。它的行为好像是:
return wait_until (lck, chrono::steady_clock::now() + rel_time, std::move(pred));
*/
int waitForDemo()
{
	std::cout << "Please, enter an integer (I'll be printing dots): \n";
	std::thread th(read_value);

	std::mutex mtx;
	std::unique_lock<std::mutex> lck(mtx);
	while (cv.wait_for(lck, std::chrono::seconds(1)) == std::cv_status::timeout) {
		std::cout << '.' << std::endl;
	}
	std::cout << "You entered: " << value << '\n';

	th.join();

	return 0;
}

std::mutex cv_m;
std::atomic<int> i{ 0 };

void waits(int idx)
{
	std::unique_lock<std::mutex> lk(cv_m);
	auto now = std::chrono::system_clock::now();
	if (cv.wait_until(lk, now + idx * std::chrono::milliseconds(100), []() {return i == 1; }))
		std::cerr << "Thread " << idx << " finished waiting. i == " << i << '\n';
	else
		std::cerr << "Thread " << idx << " timed out. i == " << i << '\n';
}

void signals()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(120));
	std::cerr << "Notifying...\n";
	cv.notify_all();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	i = 1;
	std::cerr << "Notifying again...\n";
	cv.notify_all();
}

/*
std::condition_variable::wait_until
1、当前线程的执行(该线程将锁定lck的互斥锁)会被阻塞，直到被通知或到达abs_time时间点，无论那一个第一次发生。
2、在线程还处在阻塞时，该函数将自动调用lck.unlock()，允许其他锁定的线程继续运行。
3、一旦被通知或一旦abs_time时间到了，函数就会接触阻塞状态并调用lck. lock()。在调用函数时将lck与函数的状态保持在同一状态。然后函数返回(注意，最后一个互斥锁可能会在返回之前阻塞线程)。
4、一般情况下，函数会被另一个线程的调用唤醒，无论是对成员notify_one还是成员notify_all。但是某些实现可能产生虚假的唤醒调用，而不需要调用这些函数。因此，该函数的使用者将要确保满足恢复的条件。
5、(定义2)如果指定pred，如果pred返回false，则函数只会阻塞，并且只有当它变为true时，通知才能解除线程(这对于检查伪唤醒调用特别有用).
*/
int waitUntilDemo()
{
	std::thread t1(waits, 1), t2(waits, 2), t3(waits, 3), t4(signals);
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	return 0;
}

void goN() {
	std::unique_lock<std::mutex> lck(mtx);
	std::notify_all_at_thread_exit(cv, std::move(lck));
	readyC = true;
}
/*
当调用线程退出时，等待在cond上的所有线程都被通知恢复执行。
该函数还获得由lck管理的mutex对象上的锁的所有权，该对象在内部由函数存储，并在线程退出时解锁(仅在通知所有线程之前)，行为如下:一旦所有具有线程存储时间的对象都被销毁:1 lck.unlock(); 2 cond.notify_all();
*/
int notifyExitDemo()
{
	std::thread threads[10];
	// spawn 10 threads:
	for (int i = 0; i<10; ++i)
		threads[i] = std::thread(print_id, i);
	std::cout << "10 threads ready to race...\n";

	std::thread(goN).detach();   // go!

	for (auto& th : threads) th.join();

	return 0;
}

#include <future>

void print_int(std::future<int>& fut) {
	int x = fut.get();
	std::cout << "value: " << x << '\n';
}

/*
promise是一个存储由一个(可能是另一个线程)future对象检索的类型T的值的对象，可以提供一个同步点。在构造上，承诺对象与一个新的共享状态相关联，它们可以存储类型T的值或从std::exception.中派生的异常。
这个共享状态可以通过调用成员get_future关联到一个future对象。调用后，两个对象共享相同的共享状态:
1、 promise对象是异步提供程序，它将为共享状态设置一个值。
2、 future对象是一个异步返回对象，它可以检索共享状态的值，等待它在必要时就绪。
共享状态的生命周期至少持续到最后一个对象释放它或被销毁。因此，它可以使promise对象存活下来，如果把它与future对象联系在一起的话，它就能在第一个地方得到它。
两个promise的特化定义在<future>头文件中，它们以与非专门化模板相同的方式操作，除非参数是set_value和set_value_at_thread_exit成员函数。
*/
int promiseDemo()
{
	std::promise<int> prom;                      // create promise

	std::future<int> fut = prom.get_future();    // engagement with future

	std::thread th1(print_int, std::ref(fut));  // send future to new thread

	prom.set_value(10);                         // fulfill promise
												// (synchronizes with getting the future)
	th1.join();
	return 0;
}

// count down taking a second for each value:
int countdown(int from, int to) {
	for (int i = from; i != to; --i) {
		std::cout << i << '\n';
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::cout << "Lift off!\n";
	return from - to;
}

/*
packaged_task

包装的任务
packaged_task封装可调用的元素，并允许异步检索其结果。
它类似于std::function，但是将其结果自动传输到一个future对象。
对象包含两个元素:
1、一个存储的任务，它是一些可调用的对象(例如函数指针、指向成员或函数对象的指针)，其调用签名将在Args中进行类型的参数。返回一个类型为Ret的值。
2、一个共享状态，它可以存储调用存储任务(类型为Ret)的结果，并通过未来异步访问。
共享状态通过调用成员get_future关联到一个future对象。调用后，两个对象共享相同的共享状态:
1、 packaged_task对象是异步提供程序，并期望通过调用存储任务在某个点上设置共享状态。
2、 future对象是一个异步返回对象，可以检索共享状态的值，等待它在必要时就绪。
共享状态的生命周期至少持续到最后一个对象释放它或被销毁。因此，它可以在packaged_task对象中生存，该对象在与将来相关联的情况下，首先获得它。
*/

int packageTaskDemo()
{
	std::packaged_task<int(int, int)> tsk(countdown);   // set up packaged_task
	std::future<int> ret = tsk.get_future();            // get future

	std::thread th(std::move(tsk), 10, 0);   // spawn thread to count down from 10 to 0

	int value = ret.get();                  // wait for the task to finish and get result

	std::cout << "The countdown lasted for " << value << " seconds.\n";

	th.join();

	return 0;
}

int get_value() { return 10; }
/*
返回一个shared_future对象，该对象获取future对象的共享状态。future对象(* this)没有共享状态(如默认构建的)，不再有效。
*/
int shareFutureDemo()
{
	std::future<int> fut = std::async(get_value);
	std::shared_future<int> shfut = fut.share();

	// shared futures can be accessed multiple times:
	std::cout << "value: " << shfut.get() << '\n';
	std::cout << "its double: " << shfut.get() * 2 << '\n';

	return 0;
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
	//mutexDemo();
	//tryLockDemo();
	//timeMutexDemo();
	//timeUtilDemo();
	//lockguardDemo();
	//uniqueLockDemo();
	//callOnceDemo();
	//atomicFlagDemo();
	//lockFreeDemo();
	//relaxedOrderDemo();
	//exchangeDemo();
	//fetchAddDemo();
	//flagDemo();
	//conditionDemo();
	//condition2Demo();
	//waitForDemo();
	//waitUntilDemo();
	//notifyExitDemo();
	//promiseDemo();
	//packageTaskDemo();
	shareFutureDemo();
	return 0;
}
