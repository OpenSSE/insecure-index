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

#include "logger.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fstream>
#include <iostream>
#include <memory>

namespace sse {
namespace logger {

std::shared_ptr<spdlog::logger> shared_logger_(nullptr);

std::shared_ptr<spdlog::logger> logger()
{
    if (!shared_logger_) {
        // initialize the logger
        shared_logger_ = spdlog::stderr_color_mt("console");
    }
    return shared_logger_;
}

void set_logger(const std::shared_ptr<spdlog::logger>& logger)
{
    if (logger) {
        shared_logger_ = logger;
    } else {
        shared_logger_
            = spdlog::create<spdlog::sinks::null_sink_mt>("null_logger");
    }
}

void set_logging_level(spdlog::level::level_enum log_level)
{
    logger()->set_level(log_level);
}
} // namespace logger

std::shared_ptr<spdlog::logger> Benchmark::benchmark_logger_(nullptr);

void Benchmark::set_benchmark_file(const std::string& path, bool log_to_console)
{
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path);
    file_sink->set_level(spdlog::level::trace);
    file_sink->set_pattern("[%Y-%m-%d %T.%e] %v");


    if (log_to_console) {
        auto console_sink
            = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[%Y-%m-%d %T.%e] %v");

        benchmark_logger_ = std::shared_ptr<spdlog::logger>(
            new spdlog::logger("benchmark", {file_sink, console_sink}));
    } else {
        benchmark_logger_ = std::shared_ptr<spdlog::logger>(
            new spdlog::logger("benchmark", file_sink));
    }
    benchmark_logger_->set_level(spdlog::level::trace);
    spdlog::flush_every(std::chrono::seconds(3));
}


void Benchmark::set_benchmark_file(const std::string& path)
{
    Benchmark::set_benchmark_file(path, false);
}

void Benchmark::set_log_to_console()
{
    benchmark_logger_ = spdlog::stderr_color_mt("benchmark");
    benchmark_logger_->set_level(spdlog::level::trace);

    benchmark_logger_->set_pattern("[%Y-%m-%d %T.%e] %v");
}

Benchmark::Benchmark(std::string format)
    : format_(std::move(format)), count_(0), stopped_(false),
      begin_(std::chrono::high_resolution_clock::now())
{
}

void Benchmark::stop()
{
    if (!stopped_) {
        end_     = std::chrono::high_resolution_clock::now();
        stopped_ = true;
    }
}

void Benchmark::stop(size_t count)
{
    if (!stopped_) {
        end_   = std::chrono::high_resolution_clock::now();
        count_ = count;
    }
}

void Benchmark::trace(std::chrono::duration<double, std::milli> time_ms,
                      std::chrono::duration<double, std::milli> time_per_item)
{
    if (benchmark_logger_) {
        benchmark_logger_->trace(
            format_.c_str(), count_, time_ms.count(), time_per_item.count());
    }
}

void Benchmark::stop_trace()
{
    if (!stopped_) {
        stop();

        std::chrono::duration<double, std::milli> time_ms = end_ - begin_;

        auto time_per_item = time_ms;

        if (count_ > 1) {
            time_per_item /= count_;
        }

        trace(time_ms, time_per_item);
    }
}

Benchmark::~Benchmark()
{
    stop_trace();
}

constexpr auto search_JSON_begin
    = "{{ \"message\" : \""; // double { to escape it in fmt
constexpr auto search_JSON_end = "\", \"items\" : {0}, \"time\" : {1}, "
                                 "\"time/item\" : {2}, \"locality\" : {3} }}";

SearchBenchmark::SearchBenchmark(std::string message)
    : Benchmark(search_JSON_begin + std::move(message) + search_JSON_end)
{
}

void SearchBenchmark::trace(
    std::chrono::duration<double, std::milli> time_ms,
    std::chrono::duration<double, std::milli> time_per_item)
{
    if (benchmark_logger_) {
        benchmark_logger_->trace(format_.c_str(),
                                 count_,
                                 time_ms.count(),
                                 time_per_item.count(),
                                 locality_);
    }
}
SearchBenchmark::~SearchBenchmark()
{
    stop_trace(); // calling a virtual method inside the destructor does not
                  // work properly. Instead, duplicate the destructor code,
                  // counting on the fact that the first destructor to be called
                  // is the one of the derived class.
}

} // namespace sse