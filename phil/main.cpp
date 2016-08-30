#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <deque>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

unsigned debug_flag;

void sleep(unsigned milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

class Fork {
public:

	Fork(): m() {};

	Fork(Fork& other): m() {};

	Fork& operator = (Fork& other) {
		return *this;
	}

    std::unique_lock<std::mutex> getDeferLock() {
		std::unique_lock<std::mutex> lock(m, std::defer_lock);
		return lock;
	}

private:
	std::mutex m;
};

typedef std::chrono::microseconds Microseconds;
typedef std::chrono::steady_clock Clock;
typedef Clock::time_point Time;

class Philosopher {
public:
    Philosopher(
        unsigned id,
        Fork* fork_left, Fork* fork_right,
        unsigned think_delay, unsigned eat_delay) :
        id(id),
        fork_left(fork_left), fork_right(fork_right),
        r_engine(std::random_device()()),
        think_delay_dist(0, think_delay),
        eat_delay_dist(0, eat_delay),
        eat_count(0), wait_time(0), stop_flag(false) {
    }


    void run() {
        while (!stop_flag) {
            think();

			bool success = false;
			std::unique_lock<std::mutex> lock_left(fork_left->getDeferLock());
			std::unique_lock<std::mutex> lock_right(fork_right->getDeferLock());
			while(true) {
				if (lock_left.try_lock()) {
					if (lock_right.try_lock()) {
						if (debug_flag) std::printf("[%u] took left fork\n", id);
						if (debug_flag) std::printf("[%u] took right fork\n", id);
						eat();
						lock_right.unlock();
						success = true;
						if (debug_flag) std::printf("[%u] put left fork\n", id);
					    if (debug_flag) std::printf("[%u] put right fork\n", id);
					}
					lock_left.unlock();
				}
				if (success) break;
			}
        }
        if (debug_flag) std::printf("[%u] stopped\n", id);
    }

    void stop() {
        stop_flag = true;
    }

    void printStats() const {
        std::printf("[%u] %u %lu\n", id, eat_count, wait_time);
    }

private:
    void think() {
        if (debug_flag) std::printf("[%u] thinking\n", id);
        sleep(think_delay_dist(r_engine));
        if (debug_flag) std::printf("[%u] hungry\n", id);
        wait_start = Clock::now();
    }

    void eat() {
        wait_time += std::chrono::duration_cast<Microseconds>(
            Clock::now() - wait_start).count();
        if (debug_flag) std::printf("[%u] eating\n", id);
        sleep(eat_delay_dist(r_engine));
        ++eat_count;
    }

    unsigned id;
    Fork* fork_left;
    Fork* fork_right;
    std::default_random_engine r_engine;
    std::uniform_int_distribution<unsigned> think_delay_dist;
    std::uniform_int_distribution<unsigned> eat_delay_dist;
    unsigned eat_count;
    unsigned long wait_time;
    Time wait_start;
    std::atomic<bool> stop_flag;
};

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::fprintf(stderr,
            "Usage: %s phil_count duration think_delay eat_delay debug_flag\n",
            argv[0]);
        return 1;
    }

    unsigned N = std::atoi(argv[1]);
    unsigned duration = std::atoi(argv[2]);
    unsigned think_delay = std::atoi(argv[3]);
    unsigned eat_delay = std::atoi(argv[4]);
    debug_flag = std::atoi(argv[5]);

    std::setbuf(stdout, NULL);
	
    std::vector<Fork> forks(N);
	
    std::deque<Philosopher> phils;
    for (unsigned i = 0; i < N; ++i) {
        phils.emplace_back(i + 1,
            &forks[(i + 1) % N], &forks[i],
            think_delay, eat_delay);
    }

    std::vector<std::thread> threads;
    threads.reserve(N);
    for (auto& phil : phils) {
        threads.emplace_back([&phil] { phil.run(); });
    }

    sleep(duration * 1000);

    for (auto& phil : phils) {
        phil.stop();
    }
    for (auto& thread : threads) {
        thread.join();
    }

    for (const auto& phil : phils) {
        phil.printStats();
    }
}
