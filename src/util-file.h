//
// Created by YANHAI on 2019/5/5.
//

#pragma once

#include <cstdint>
#include <sys/param.h>
#include <cstring>
#include <string>
#include <list>
#include <json/json.h>
#include <boost/filesystem.hpp>

using namespace std;

// 1TB 空间 大约有500万个文件
class FileCtx {
public:
    FileCtx(const std::string &directory, unsigned int limit, unsigned int safe, unsigned long timeout, bool emptydir);

    ~FileCtx() = default;

    bool empty() const noexcept
    {
        return queue.empty();
    }

    bool recursive_directory();

    void delete_for_limit(off_t bytes);

    void delele_for_timeout();

private:
    void parse_file(const boost::filesystem::path &path);

    inline bool remove_empty_directory(const boost::filesystem::path &path);

    inline bool remove_empty_directory2(const boost::filesystem::path &path);

    void remove_empty_directorys();

    inline bool remove_file(const boost::filesystem::path &path);

    void sort();

    void print_queue();

private:
    // config
    string directory;
    unsigned int limit;
    unsigned int safe;
    unsigned long timeout;
    bool emptydir;

    // stats
    unsigned long d_dir;
    unsigned long d_file;

    // queue
    list <boost::filesystem::path> queue;
    list <boost::filesystem::path> queue_empty_dir;
};
