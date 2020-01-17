//
// Created by YANHAI on 2019/5/5.
//

#include <regex>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctime>
#include <boost/timer/timer.hpp>
#include <boost/date_time.hpp>

#include "util/log.h"
#include "util-file.h"

FileCtx::FileCtx(const std::string &directory, unsigned int limit, unsigned int safe, unsigned long timeout,
                 bool emptydir)
{
    this->directory = directory;
    this->limit = limit;
    this->safe = safe;
    this->timeout = timeout;
    this->emptydir = emptydir;
    d_dir = 0;
    d_file = 0;
}

bool FileCtx::recursive_directory()
{
    try {
        boost::filesystem::path p(directory);
        boost::filesystem::recursive_directory_iterator end_iter;
        for (boost::filesystem::recursive_directory_iterator iter(p); iter != end_iter; ++iter) {
            parse_file(iter->path());
            // 遍历过程中，remove文件会抛出异常?
        }
    } catch (boost::filesystem::filesystem_error &e) {
        spdlog::error("recursive directory {} exception: {}", directory, e.what());
    }

    remove_empty_directorys();
    sort();
    print_queue();
    return true;
}

//仅删除包含时间的目录 /var/log/bd_input_cache/smtp/log/2018/05/29/03/xxxxx/xxxx
// 如 /var/log/bd_input_cache/smtp/log 则不删除
static bool is_timer_dir(const boost::filesystem::path &path)
{
    static thread_local smatch result;
    static regex pattern(R"(.*/\d+/.*)");
    return regex_match(path.string(), result, pattern);
}

inline bool FileCtx::remove_empty_directory(const boost::filesystem::path &path)
{
    boost::system::error_code ec;
    auto t = boost::filesystem::last_write_time(path);
    auto r = boost::filesystem::remove(path, ec);
    auto time_str = boost::posix_time::to_simple_string(boost::posix_time::from_time_t(t));
    if (!ec) {
        d_dir += 1;
        spdlog::info("delete empty dir: {} [{}]", path.string(), time_str);
    } else {
        spdlog::warn("delete empty dir: {} [{}] ## failed: {} ##", path.string(), time_str, ec.message());
    }

    return r;
}

inline bool FileCtx::remove_empty_directory2(const boost::filesystem::path &path)
{
    auto p = path;
    auto current_time = std::time(nullptr);

    while (boost::filesystem::exists(p)) {
        if (boost::filesystem::is_empty(p) && is_timer_dir(p) &&
            current_time - boost::filesystem::last_write_time(p) > 3600) {
            auto parent_path = p.parent_path();
            remove_empty_directory(p);
            p = parent_path;
        } else {
            break;
        }
    }
    return true;
}

void FileCtx::remove_empty_directorys()
{
    while (!queue_empty_dir.empty()) {
        auto &dir = queue_empty_dir.front();
        remove_empty_directory2(dir);
        queue_empty_dir.pop_front();
    }
}

inline bool FileCtx::remove_file(const boost::filesystem::path &path)
{
    boost::system::error_code ec;
    auto t = boost::filesystem::last_write_time(path);
    auto file_size = boost::filesystem::file_size(path);
    auto r = boost::filesystem::remove(path, ec);
    auto time_str = boost::posix_time::to_simple_string(boost::posix_time::from_time_t(t));
    if (!ec) {
        d_file += 1;
        spdlog::info("delete file: {} [{} {}]", path.string(), file_size, time_str);
    } else {
        spdlog::warn("delete file: {} [{} {}] ## failed: {} ##", path.string(), file_size, time_str, ec.message());
    }

    return r;
}

void FileCtx::parse_file(const boost::filesystem::path &path)
{
    if (boost::filesystem::is_directory(path)) {
        // is_empty 表示 文件为空或者目录为空
        if (boost::filesystem::is_empty(path) && is_timer_dir(path)) {
            if (std::time(nullptr) - boost::filesystem::last_write_time(path) > 3600)
                queue_empty_dir.push_back(path);
        }
    } else if (boost::filesystem::is_regular_file(path)) {
        queue.push_back(path);
    }
}

void FileCtx::print_queue()
{
    spdlog::debug("{} queue size is {}", directory, queue.size());
    return;
    if (queue.empty()) {
        spdlog::debug("{} queue is empty", directory);
    } else {
        for (auto &p: queue) {
            spdlog::debug("name: {}, time: {}", p.string(), boost::filesystem::last_write_time(p));
        }
    }
}

/**
 * 为队列中的节点按最后修改时间进行排序，较旧的文件放在队列头，较新的文件放在队列尾
 */
void FileCtx::sort()
{
    queue.sort([](boost::filesystem::path &p1, boost::filesystem::path &p2) {
        return boost::filesystem::last_write_time(p1) < boost::filesystem::last_write_time(p2);
    });
}

/**
 * 该目录到达设置的阈值，开始删除文件
 * @param bytes
 */
void FileCtx::delete_for_limit(off_t bytes)
{
    off_t delete_bytes = 0;
    while (delete_bytes < bytes && !queue.empty()) {
        auto &file = queue.front();
        delete_bytes += boost::filesystem::file_size(file);
        remove_file(file);
        queue.pop_front();
    }
}

/**
 * 判断是否有文件超时，超时则删除
 */
void FileCtx::delele_for_timeout()
{
    std::time_t next_file_time = 0;
    boost::timer::cpu_timer cpu_timer;
    auto current_time = std::time(nullptr);
    while (!queue.empty()) {
        auto &file = queue.front();
        if (!boost::filesystem::exists(file)) {
            spdlog::debug("{} no exists, ignore", file.c_str());
            queue.pop_front();
            continue;
        }
        if (current_time - boost::filesystem::last_write_time(file) > timeout) {
            remove_file(file);
            queue.pop_front();
        } else {
            next_file_time = boost::filesystem::last_write_time(file);
            break;
        }
    }
    cpu_timer.stop();
    if (next_file_time != 0)
        spdlog::info("next timeout after {}s, use time: {}s",
                     timeout - (current_time - next_file_time), cpu_timer.format(3, "%w"));
    else
        spdlog::info("use time: {}s", cpu_timer.format(3, "%w"));
}
