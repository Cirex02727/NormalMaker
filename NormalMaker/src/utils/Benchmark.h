#pragma once

#include <functional>

class Benchmark
{
public:
	static void Bench(unsigned int iterations, const std::function<void()>& f);

	static void Bench(unsigned int iterations, const std::function<void()>& f, const std::function<void()>& after);
};
