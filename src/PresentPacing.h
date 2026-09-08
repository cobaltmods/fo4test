#pragma once
#include <chrono>
#include <cstdint>
#include <mutex>
#include <Windows.h>

// Limits frame starts; the FG SDK spaces synthetic frames within each batch.
class PresentPacing
{
public:
	void WaitForFrame(uint32_t frame, double outputFPS, unsigned multiplier, HWND window);
	void Reset();
private:
	void WaitLocked(double outputFPS, unsigned multiplier, HWND window);
	uint32_t lastFrame = 0;
	bool hasFrame = false;
	using Clock = std::chrono::steady_clock;
	std::mutex mutex;
	Clock::time_point last{};
	bool valid = false;
	unsigned previousMultiplier = 1;
};
