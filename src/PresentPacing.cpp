#include "PresentPacing.h"
#include <algorithm>
#include <cmath>
#include <winrt/base.h>

void PresentPacing::Reset()
{
	std::scoped_lock lock(mutex);
	valid = false;
	hasFrame = false;
}

void PresentPacing::WaitForFrame(uint32_t frame, double outputFPS, unsigned multiplier, HWND window)
{
	std::scoped_lock lock(mutex);
	if (hasFrame && lastFrame == frame) { return; }
	lastFrame = frame;
	hasFrame = true;
	WaitLocked(outputFPS, multiplier, window);
}

void PresentPacing::WaitLocked(double outputFPS, unsigned multiplier, HWND window)
{
	if (!std::isfinite(outputFPS) || outputFPS <= 0.0 || (window && IsIconic(window))) {
		valid = false;
		return;
	}
	multiplier = std::clamp(multiplier, 1u, 6u);
	// Give an outgoing FG batch its full interval on an ON -> OFF transition.
	const auto batchSize = valid ? (std::max)(previousMultiplier, multiplier) : multiplier;
	const auto interval = std::chrono::duration_cast<Clock::duration>(
		std::chrono::duration<double>(batchSize / std::clamp(outputFPS, 10.0, 500.0)));
	const auto now = Clock::now();
	// No catch-up bursts after stalls. Focus loss alone is not suspension.
	if (valid && now - last < std::chrono::seconds(1)) {
		const auto deadline = last + interval;
		static thread_local winrt::handle timer{ CreateWaitableTimerExW(nullptr, nullptr,
			CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_MODIFY_STATE | SYNCHRONIZE) };
		while (Clock::now() < deadline) {
			if (window && IsIconic(window)) { valid = false; return; }
			const auto ticks = std::chrono::duration_cast<std::chrono::duration<long long, std::ratio<1, 10000000>>>(deadline - Clock::now()).count();
			if (ticks <= 0) { break; }
			LARGE_INTEGER due{};
			due.QuadPart = -std::min<long long>(ticks, 500000);
			if (!timer || !SetWaitableTimer(timer.get(), &due, 0, nullptr, nullptr, FALSE) ||
				WaitForSingleObject(timer.get(), 100) != WAIT_OBJECT_0) { Sleep(1); }
		}
	}
	last = Clock::now();
	previousMultiplier = multiplier;
	valid = true;
}
