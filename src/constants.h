#pragma once
#include <string>
#include <chrono>

namespace Colors {
	const std::string WHITE = "\033[0m";
	const std::string GREEN = "\033[38;2;0;255;0m";
	const std::string RED = "\033[38;2;255;0;0m";
	const std::string LIGHT_YELLOW = "\033[38;2;255;255;153m";
	const std::string LIGHT_BLUE = "\033[38;2;173;216;230m";
}

namespace Timing {
	// How long the kernel's main loop sleeps between CPU ticks.
	// Without this, main_loop() spins as fast as the CPU allows, so
	// "1 cycle" is effectively microseconds - meaning quantum-cycles,
	// batch-process-freq, and memory snapshots all fire absurdly fast.
	// This paces 1 tick to a human-observable interval instead.
	constexpr std::chrono::milliseconds TICK_DURATION{20};
}