// Accepts a filename as input or reads from stdin.

#include <wc/count.h>
#include <wc/top.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <boost/optional.hpp>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::length_error;
using boost::optional;
using std::string;

int main(int argc, const char * * args) {
    optional<string> file_name;
    if (argc > 1) {
        file_name = args[1];
    }

    wc::word_counter counter;
    auto processor = [&counter](auto begin, auto end, bool eof) {
        return wc::read_blob(begin, end, eof, counter);
    };

    const size_t buffer_size = 10 * 1024;

    try {
        if (file_name) {
            ifstream actual_file(file_name.get(),ifstream::binary);
            actual_file.exceptions(std::ifstream::badbit);
            wc::read_using_buffer<buffer_size>(actual_file, processor);
        } else {
            cin.exceptions(ifstream::badbit);
            wc::read_using_buffer<buffer_size>(cin, processor);
        }
    } catch(const length_error &) {
        cerr << "A word in this file was too large to be processed." << endl;
        return 2;
    }

    wc::top_word_collection<10> top_words;

    auto map = counter.words();
    for(auto itr = map.begin(); itr != map.end(); ++ itr) {
        cout << itr->first << "\t" << itr->second << endl;
        top_words.add(itr->first, itr->second);
    }

    cout << endl
         << "Top words: " << endl
         << endl;
    for(const auto & word_info : top_words.get_words()) {
        cout << word_info.first << "\t" << word_info.second << "\n";
    }
}
