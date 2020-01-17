//
// Created by YANHAI on 2019/12/30.
//

#include <fstream>
#include <string>
#include "config.h"
#include "util/log.h"

using namespace std;

bool Config::load_config(const string &filename)
{
    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;

    ifstream is(filename);
    if (!is) {
        spdlog::error("{}: cannot access {}: No such file", __FUNCTION__, filename);
        return false;
    }

    JSONCPP_STRING errs;
    if (!parseFromStream(builder, is, &root, &errs)) {
        spdlog::error("{}: parse {} failed: {}", __FUNCTION__, filename, errs);
        return false;
    }
    return true;
}

Json::UInt64 Config::time_string_to_uint64(Json::Value &s)
{
    Json::UInt64 val = 0;
    auto time_str = s.asString();
    char c = time_str[time_str.length() - 1];
    if (isalpha(c)) {
        auto time_str2 = time_str.substr(0, time_str.length() - 1);
        Json::UInt64 tmp = std::stoll(time_str2);
        switch (c) {
            case 'd':
            case 'D':
                val = tmp * 24 * 60 * 60;
                break;
            case 'h':
            case 'H':
                val = tmp * 60 * 60;
                break;
            case 'm':
            case 'M':
                val = tmp * 60;
                break;
            case 's':
            case 'S':
            default:
                val = tmp;
        }
    } else {
        val = std::stoll(time_str);
    }

    return val;
}
