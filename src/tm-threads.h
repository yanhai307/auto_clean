//
// Created by YANHAI on 2019/5/6.
//

#pragma once

#include "util-threads.h"

class TmThreads{
public:
    static void *loop(void *td);

    static void kill(ThreadVars *tv);
    static void kills();

    static void clear(ThreadVars *tv);
    static void clears();

    static void append(ThreadVars *tv);
    static int spawn(ThreadVars *tv);
};
