//
// Created by YANHAI on 2019/10/12.
//

#pragma once

#include <spdlog/spdlog.h>

void init_logger(spdlog::level::level_enum log_level = spdlog::level::warn,
                 const char *filename = nullptr);

void init_logger(const std::string &level_str = "warning",
                 const char *filename = nullptr);

void register_logger(const char *name,
                     spdlog::level::level_enum log_level = spdlog::level::warn,
                     const char *filename = nullptr);

void register_logger(const char *name,
                     const std::string &level_str = "warning",
                     const char *filename = nullptr);