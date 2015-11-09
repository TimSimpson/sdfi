// Accepts a filename as it's first argument or reads from standard in.
// Reads all text and prints out all words and their counts followed by the
// top ten most frequently seen words.

#include <wc/cmds.h>
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

    try {
        if (file_name) {
            ifstream actual_file(file_name.get(),ifstream::binary);
            actual_file.exceptions(std::ifstream::badbit);
            wc::read_using_buffer<wc::buffer_size>(actual_file, processor);
        } else {
            cin.exceptions(ifstream::badbit);
            wc::read_using_buffer<wc::buffer_size>(cin, processor);
        }
    } catch(const length_error &) {
        cerr << "A word in this file was too large to be processed." << endl;
        return 2;
    }

    wc::print_word_map(counter.words());
    wc::print_top_words(counter.words());
}
