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
using std::endl;
using std::exception;
using std::string;

int main(int argc, const char * * args) {
    if (argc < 2) {
        cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
             << " [directory]" << endl;
        return 1;
    }

    wc::stop_watch watch;

    wc::word_counter counter;
    auto processor = [&counter](auto begin, auto end, bool eof) {
        return read_blob(begin, end, eof, counter);
    };

    const size_t buffer_size = 10 * 1024;

    auto file_handler = [&processor](const string full_path) {
        cout << "Reading file \"" << full_path << "\"..." << endl;
        wc::read_file<buffer_size>(processor, full_path);
    };

    try {
        wc::read_directory(file_handler, args[1], cerr);
    } catch(const exception & e) {
        cerr << "Error reading directory: " << e.what() << endl;
        return 2;
    }

    wc::top_word_collection<10> top_words;

    for(const auto & word_info : counter.words()) {
        top_words.add(word_info.first, word_info.second);
    }

    cout << endl
         << "Top words: " << endl
         << endl;
    for(int i = 0; i < top_words.total_words(); ++i) {
        const auto word_info = top_words.get_words()[i];
        cout << i + 1 << ". " << word_info.first
             << "\t" << word_info.second << endl;
    }
    watch.print_time();
}
