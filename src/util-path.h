//
// Created by YANHAI on 2019/5/5.
//

#pragma once

class Path {
public:
    Path(const char *path) : _path(path) {}

    bool isAbsolute();

    /**
     *  \brief Check if a path is relative
     *
     *  \param path string with the path
     *
     *  \retval true relative
     *  \retval false not relative
     */
    bool isRelative() {
        return !isAbsolute();
    }

private:
    const char *_path;
};
