#ifndef TIMER_H
#define TIMER_H
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
class Timer
{
  public:
	Timer(std::function<void()> func, std::chrono::milliseconds interval);
	~Timer();
	int start_timer();
	int stop_timer();

  private:
	void timer_loop();
	std::function<void()> func_;
	std::chrono::milliseconds interval_;
	std::mutex timer_lock_;
	bool running_;
	std::thread timer_thread_;
};
#endif