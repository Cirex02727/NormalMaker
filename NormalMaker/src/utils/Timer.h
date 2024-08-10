#pragma once

#include <chrono>

class Timer
{
public:
	Timer();

	void Reset();

	float ElapsedSeconds();
	float ElapsedMillis();
	long long ElapsedNanoseconds();

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};

class ScopedTimer
{
public:
	ScopedTimer(const std::string& name)
		: m_Name(name) {}

	~ScopedTimer()
	{
		float time = m_Timer.ElapsedMillis();
		printf("[TIMER] %s - %f ms\n", m_Name.c_str(), time);
	}

private:
	std::string m_Name;
	Timer m_Timer;
};
