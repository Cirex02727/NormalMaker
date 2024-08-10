#include "vkpch.h"
#include "Utils.h"

template<typename T>
void Utils::ParallelSort(T* data, size_t len, int grainsize, const std::function<bool(const T&, const T&)> compare)
{
    if (len < grainsize)
        std::sort(data, data + len, compare);
    else
    {
        auto future = std::async(ParallelSort<T>, data, len / 2, grainsize, compare);

        ParallelSort(data + len / 2, len - len / 2, grainsize, compare);

        future.wait();

        std::inplace_merge(data, data + len / 2, data + len, compare);
    }
}

std::string Utils::BytesToText(double bytes)
{
    if (bytes < 1000) return FormatFloat(bytes, 2) + "B";
    bytes *= 0.001;
    if (bytes < 1000) return FormatFloat(bytes, 2) + "kB";
    bytes *= 0.001;
    if (bytes < 1000) return FormatFloat(bytes, 2) + "MB";
    bytes *= 0.001;
    if (bytes < 1000) return FormatFloat(bytes, 2) + "GB";
    return std::string();
}

std::string Utils::FormatFloat(double n, int digit)
{
    std::string s = std::to_string(n);
    return s.substr(0, std::min(s.find(".") + digit, s.size()));
}
