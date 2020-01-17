//
// Created by YANHAI on 2019/5/6.
//

#include <iostream>
#include <util/config.h>
#include "worker.h"
#include "util/log.h"

using namespace std;

int Worker::_worker_threads = 0;

Worker::Worker(Json::Value &config) : config(config)
{
    _worker_threads++;
    name = "Work";
    name += "#" + std::to_string(_worker_threads);
    sleep = 3;
    file = nullptr;
    disk = nullptr;
}

Worker::~Worker()
{
    delete file;
    delete disk;
}

int Worker::init()
{
    name = config["name"].asString();
    sleep = config["sleep"].asUInt();
    timeout = Config::time_string_to_uint64(config["timeout"]);

    const string &path = config["path"].asString();
    unsigned int limit = config["limit"].asUInt();
    unsigned int safe = config["safe"].asUInt();
    bool emptydir = config["empty"].asBool();
    file = new FileCtx(path, limit, safe, timeout, emptydir);
    disk = new Disk(path.c_str(), limit);
    return 0;
}

int Worker::loop()
{
    if (file->empty()) {
        file->recursive_directory();
        if (file->empty())
            return 0;
    }

    auto delete_bytes = disk->deleteBytes();
    if (delete_bytes > 0) {
        file->delete_for_limit(delete_bytes);
    }

    file->delele_for_timeout();
    return 0;
}

void Worker::exitPrintStats()
{
    spdlog::debug("Worker thread exit print stats: {}", name);
}

int Worker::deinit()
{
    spdlog::debug("Worker thread deinit: {}", name);
    return 0;
}

ThreadVars *Worker::create(Json::Value &config)
{
    return new Worker(config);
}
