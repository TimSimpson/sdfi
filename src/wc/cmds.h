// Contains some helper utilities for the various commands.

#ifndef FILE_GUARD_WC_CMDS_H
#define FILE_GUARD_WC_CMDS_H

#include <chrono>
#include <iostream>
#include <thread>
#include <wc/count.h>
#include <wc/top.h>

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;


namespace wc {

constexpr std::size_t buffer_size = 10 * 1024;

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


void nap() {
    using namespace std::literals;
    std::this_thread::sleep_for(2ms);
}


void print_word_map(const wc::word_map & map) {
    for(auto itr = map.begin(); itr != map.end(); ++ itr) {
        std::cout << itr->first << "\t" << itr->second << "\n";
    }
}

void print_top_words(const wc::word_map & map) {
    wc::top_word_collection<10> top_words;
    for(auto itr = map.begin(); itr != map.end(); ++ itr) {
        top_words.add(itr->first, itr->second);
    }
    std::cout << "\nTop words: \n\n";
    for(int i = 0; i < top_words.total_words(); ++i) {
        const auto word_info = top_words.get_words()[i];
        std::cout << i + 1 << ". " << word_info.first
                  << "\t" << word_info.second << "\n";
    }
}


// Adds every word map in a collection except for the zeroth element to the
// zeroeth element and returns a reference to it.
wc::word_map sum_word_maps(auto collection, auto get_word_map) {
    // Commandeer the first element's results and use it for the running total.
    wc::word_map totals = get_word_map(collection[0]);
    // Add in the totals from all of the other workers.
    for (int i = 1; i < collection.size(); ++ i) {
        const auto & rhs = get_word_map(collection[i]);
        for (auto itr = rhs.begin(); itr != rhs.end(); ++ itr) {
            totals[itr->first] += itr->second;
        }
    }
    return totals;
}

}  // end namespace wc

#endif
