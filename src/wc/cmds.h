// Contains some helper utilities to run commands.

#ifndef FILE_GUARD_WC_CMDS_H
#define FILE_GUARD_WC_CMDS_H

#include <chrono>
#include <iostream>

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;


namespace wc {


class timer {
public:
    timer()
    :   start_time(std::chrono::high_resolution_clock::now()) {
    }

    ~timer() {
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;

        auto end_time = hrclock::now();
        std::cout << "Elapsed time: "
                  << duration_cast<milliseconds>(end_time - start_time).count()
                  << "ms" << std::endl;
    }

    timer(const timer & rhs) = delete;
    timer & operator=(const timer & rhs) = delete;

private:
    using hrclock = std::chrono::high_resolution_clock;
    decltype(hrclock::now()) start_time;
};


}  // end namespace wc

#endif
