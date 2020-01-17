//
// Created by YANHAI on 2019/12/30.
//

#pragma once

#include <string>
#include <json/json.h>
#include <iostream>

class Config {
private:
    Config() = default;

    ~Config() = default;

public:
    static Config &instance()
    {
        static Config config;
        return config;
    }

    static Json::UInt64 time_string_to_uint64(Json::Value &s);

    bool load_config(const std::string &config_file);

    Json::Value &operator[](const char *key)
    {
        return root[key];
    }

    const Json::Value &operator[](const char *key) const
    {
        return root[key];
    }

    Json::Value &operator[](const std::string &key)
    {
        return root[key];
    }

    const Json::Value &operator[](const std::string &key) const
    {
        return root[key];
    }

private:
    Json::Value root;
};
