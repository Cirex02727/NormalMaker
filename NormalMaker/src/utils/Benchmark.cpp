#include "vkpch.h"
#include "Benchmark.h"

#include "Timer.h"

#include<iostream>
#include<chrono>

void Benchmark::Bench(unsigned int iterations, const std::function<void()>& f)
{
	double millis = 0;

	for(unsigned int i = 0; i < iterations; i++)
	{
		auto start = std::chrono::high_resolution_clock::now();

		f();

		millis += (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count()) * 0.001f * 0.001f;
	}

	std::cout << "Benchmarch " << iterations << " its - " << millis << "ms (~" << (millis / iterations) << "ms each)" << std::endl;
}

void Benchmark::Bench(unsigned int iterations, const std::function<void()>& f, const std::function<void()>& after)
{
	double millis = 0;

	for (unsigned int i = 0; i < iterations; i++)
	{
		auto start = std::chrono::high_resolution_clock::now();

		f();

		millis += (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count()) * 0.001f * 0.001f;

		after();
	}

	std::cout << "Benchmarch " << iterations << " its - " << millis << "ms (~" << (millis / iterations) << "ms each)" << std::endl;
}
