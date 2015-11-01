// Contains some helper utilities for the various commands.

#ifndef FILE_GUARD_WC_CMDS_H
#define FILE_GUARD_WC_CMDS_H

#include <chrono>
#include <iostream>

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;


namespace wc {

// This class prints out the elapsed time.
class stop_watch {
public:
    stop_watch()
    :   start_time(std::chrono::high_resolution_clock::now()) {
    }

    void print_time() {
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;

        auto end_time = hrclock::now();
        std::cout << "Elapsed time: "
                  << duration_cast<milliseconds>(end_time - start_time).count()
                  << "ms" << std::endl;
    }

private:
    using hrclock = std::chrono::high_resolution_clock;
    decltype(hrclock::now()) start_time;
};


}  // end namespace wc

#endif
