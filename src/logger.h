#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Logger {

inline void Init()
{
  auto logger = spdlog::stdout_color_mt("polyseg");
  logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
  spdlog::set_default_logger(logger);
  spdlog::flush_on(spdlog::level::trace);
#ifdef NDEBUG
  spdlog::set_level(spdlog::level::info);
#else
  spdlog::set_level(spdlog::level::debug);
#endif
}

}  // namespace Logger
