//
// Created by YANHAI on 2019/10/12.
//

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "log.h"

/**
 * init console
 * @param log_level
 */
static std::shared_ptr<spdlog::sinks::sink> init_logger_console(spdlog::level::level_enum log_level)
{
    auto logger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    logger->set_level(log_level);
    return logger;
}

/**
 * init file
 * @param log_level
 * @param base_filename
 * @param max_size
 * @param max_files
 * @return
 */
static std::shared_ptr<spdlog::sinks::sink> init_logger_file(spdlog::level::level_enum log_level,
                                                             const std::string &base_filename,
                                                             std::size_t max_size,
                                                             std::size_t max_files)
{
    auto logger = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(base_filename, max_size, max_files);
    logger->set_level(log_level);
    return logger;
}

/**
 * init logger
 * @param log_level
 * @param filename
 */
void init_logger(spdlog::level::level_enum log_level, const char *filename)
{
    std::vector<spdlog::sink_ptr> sinks;

    sinks.push_back(init_logger_console(log_level));
    if (filename) {
        sinks.push_back(init_logger_file(log_level, filename, 1048576 * 5, 3)); // 5MB
    }

    auto logger = std::make_shared<spdlog::logger>("main", sinks.begin(), sinks.end());
    logger->set_level(log_level);
    logger->flush_on(log_level);
    logger->set_pattern("%^[%L %Y-%m-%d %H:%M:%S.%e]%$ <%T> %v");
    spdlog::set_default_logger(std::move(logger));
}

/**
 * init logger
 * @param level_str
 * @param filename
 */
void init_logger(const std::string &level_str, const char *filename)
{
    return init_logger(spdlog::level::from_str(level_str), filename);
}

void register_logger(const char *name, spdlog::level::level_enum log_level, const char *filename)
{
    std::vector<spdlog::sink_ptr> sinks;

    // main is default logger
    if (!strcmp(name, "main"))
        return;

    sinks.push_back(init_logger_console(log_level));
    if (filename) {
        sinks.push_back(init_logger_file(log_level, filename, 1048576 * 5, 3)); // 5MB
    }

    auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    logger->set_level(log_level);
    logger->flush_on(log_level);
    spdlog::register_logger(std::move(logger));
}

void register_logger(const char *name, const std::string &leval_str, const char *filename)
{
    register_logger(name, spdlog::level::from_str(leval_str), filename);
}
