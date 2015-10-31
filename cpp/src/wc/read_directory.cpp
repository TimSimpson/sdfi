// Accepts a directory as it's first argument and reads all files.

#include <wc/count.h>
#include <wc/filesystem.h>
#include <wc/top.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>


using namespace wc;
using std::string;


int main(int argc, const char * * args) {
    if (argc < 2) {
        std::cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
                  << " [directory]" << std::endl;
        return 1;
    }

    word_counter counter;
    auto processor = [&counter](auto begin, auto end, bool eof) {
        return read_blob(begin, end, eof, counter);
    };

    const size_t buffer_size = 10 * 1024;

    try {
        wc::read_directory<buffer_size>(processor, args[1], std::cerr);
    } catch(const std::exception & e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
        return 2;
    }

    top_word_collection<10> top_words;

    auto map = counter.words();
    for(const auto & word_info : counter.words()) {
        // std::cout << word_info.first << "\t" << word_info.second << "\n";
        top_words.add(word_info.first, word_info.second);
    }

    std::cout << std::endl;
    std::cout << "Top words: " << std::endl;
    std::cout << std::endl;
    for(int i = 0; i < top_words.total_words(); ++i) {
        const auto word_info = top_words.get_words()[i];
        std::cout << i + 1 << ". " << word_info.first
                  << "\t" << word_info.second << "\n";
    }
}
