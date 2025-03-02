#include "timer.h"
Timer::Timer(std::function<void()> func, std::chrono::milliseconds interval)
	: func_(func)
	, interval_(interval)
{
}

int Timer::start_timer()
{
	std::unique_lock<std::mutex> lock(timer_lock_);
	if (!running_) {
		running_ = true;
		timer_thread_ = std::thread(&Timer::timer_loop, this);
	}
	return 0;
}

int Timer::stop_timer()
{
	std::unique_lock<std::mutex> lock(timer_lock_);
	if (running_) {
		running_ = false;
		if (timer_thread_.joinable()) {
			timer_thread_.join();
		}
	}
	return 0;
}
void Timer::timer_loop()
{
	auto next_run = std::chrono::steady_clock::now();
	while (running_) {
		func_();
		next_run += interval_;
		std::this_thread::sleep_until(next_run);
	}
}

Timer::~Timer()
{
	stop_timer();
}