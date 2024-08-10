#pragma once

class Utils
{
public:
    template<typename T>
    static void ParallelSort(T* data, size_t len, int grainsize, const std::function<bool(const T&, const T&)> compare);

    static std::string BytesToText(double bytes);

private:
    static std::string FormatFloat(double n, int digit);
};
