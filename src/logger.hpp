//
// Sophos - Forward Private Searchable Encryption
// Copyright (C) 2016 Raphael Bost
//
// This file is part of Sophos.
//
// Sophos is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// Sophos is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with Sophos.  If not, see <http://www.gnu.org/licenses/>.
//


#pragma once

#include <spdlog/spdlog.h>

#include <array>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

namespace sse {
namespace logger {

std::shared_ptr<spdlog::logger> logger();
void set_logger(const std::shared_ptr<spdlog::logger>& logger);

void set_logging_level(spdlog::level::level_enum log_level);

} // namespace logger

class Benchmark
{
public:
    template<typename T>
    friend class ThroughputBenchmark;

    static void set_benchmark_file(const std::string& path);
    static void set_log_to_console();

    explicit Benchmark(std::string format);
    Benchmark() = delete;

    void stop();
    void stop(size_t count);

    inline void set_count(size_t c)
    {
        count_ = c;
    }

    virtual ~Benchmark();

protected:
    static std::shared_ptr<spdlog::logger> benchmark_logger_;

    std::string                                    format_;
    size_t                                         count_;
    bool                                           stopped_;
    std::chrono::high_resolution_clock::time_point begin_;
    std::chrono::high_resolution_clock::time_point end_;
};

class SearchBenchmark : public Benchmark
{
public:
    explicit SearchBenchmark(std::string message);
};

template<typename T>
class ThroughputBenchmark
{
public:
    ThroughputBenchmark(std::string                   format,
                        std::chrono::duration<double> sampling_interval,
                        std::atomic<T>&               observed_value)
        : m_format(std::move(format)), m_sampling_interval(sampling_interval),
          m_observed_value(observed_value)
    {
    }

    ThroughputBenchmark(std::string format, std::atomic<T>& observed_value)
        : m_format(std::move(format)),
          m_sampling_interval(std::chrono::seconds(1)),
          m_observed_value(observed_value)
    {
    }

    void run_loop()
    {
        T prev_value;

        auto t1    = std::chrono::high_resolution_clock::now();
        prev_value = m_observed_value;

        std::this_thread::sleep_for(m_sampling_interval);

        while (!m_stop) {
            auto   t2         = std::chrono::high_resolution_clock::now();
            T      new_value  = m_observed_value;
            size_t diff_value = new_value - prev_value;

            std::chrono::duration<double> time_s
                = t2 - t1; // get the duration in seconds
            // compute the throughput
            double throughput = diff_value / time_s.count();
            if (Benchmark::benchmark_logger_) {
                Benchmark::benchmark_logger_->trace(
                    m_format.c_str(), throughput, time_s.count());
            }

            t1         = std::chrono::high_resolution_clock::now();
            prev_value = m_observed_value;
            std::this_thread::sleep_for(m_sampling_interval);
        }
    }

    std::thread run_loop_in_thread()
    {
        return std::thread([this]() { this->run_loop(); });
    }


    void stop()
    {
        m_stop = true;
    }

private:
    std::string                         m_format;
    const std::chrono::duration<double> m_sampling_interval;

    std::atomic_bool m_stop{false};
    std::atomic<T>&  m_observed_value;
};

} // namespace sse