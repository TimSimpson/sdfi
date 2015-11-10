// Accepts a directory as it's first argument and reads all files before
// printing out the top 10 most frequently recurring words.

#include <wc/cmds.h>
#include <wc/count.h>
#include <wc/filesystem.h>
#include <wc/top.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>

using std::cerr;
using std::cout;
using std::exception;
using std::string;


int main(int argc, const char * * args) {
    if (argc < 2) {
        cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
             << " [directory]\n";
        return 1;
    }

    wc::stop_watch watch;

    wc::word_counter counter;
    auto processor = [&counter](auto begin, auto end, bool eof) {
        return read_blob(begin, end, eof, counter);
    };

    auto file_handler = [&processor](const string full_path) {
        cout << "Reading file \"" << full_path << "\"...\n";
        wc::read_file<wc::buffer_size>(processor, full_path);
    };

    try {
        wc::read_directory(file_handler, args[1], cerr);
    } catch(const exception & e) {
        cerr << "Error reading directory: " << e.what() << "\n";
        return 2;
    }

    wc::print_top_words(counter.words());

    watch.print_time();
}
