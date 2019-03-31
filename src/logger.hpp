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

    template<typename T>
    friend class ProgressIndicator;

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
    // format must be an {fmt} format string. It is passed the following
    // arguments:
    // 0: the exact time elapsed during the sampling interval
    // 1: the value of the observed variable at the end of the sampling interval
    // 2: the throughput of the observed variable during the sampling interval
    // 3: the progression of the observed variable at the end of the sampling
    // 4: the progression (in percent)
    //
    //
    // If the max_value argument is not used in the constructor, the progression
    // is not computed and the value 0 is given to the formatting string
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

    ThroughputBenchmark(std::string                   format,
                        std::chrono::duration<double> sampling_interval,
                        std::atomic<T>&               observed_value,
                        const T&                      max_value)
        : m_format(std::move(format)), m_sampling_interval(sampling_interval),
          m_observed_value(observed_value), m_compute_progress(true),
          m_max_value(max_value)
    {
    }

    ThroughputBenchmark(std::string     format,
                        std::atomic<T>& observed_value,
                        const T&        max_value)
        : m_format(std::move(format)),
          m_sampling_interval(std::chrono::seconds(1)),
          m_observed_value(observed_value), m_compute_progress(true),
          m_max_value(max_value)
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
            // compute the progression
            double progress = 0.0;
            if (m_compute_progress) {
                progress = ((double)new_value) / ((double)m_max_value);
            }


            if (Benchmark::benchmark_logger_) {
                Benchmark::benchmark_logger_->trace(
                    m_format.c_str(),
                    time_s.count(),   // the time elapsed during the sampling
                    m_observed_value, // the value at the sampling point
                    throughput,       // the throughtput during the interval
                    progress,         // the progress since the beginning
                    progress * 100);  // the progress percentage
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
    const bool       m_compute_progress{false};
    const T          m_max_value;
};

} // namespace sse