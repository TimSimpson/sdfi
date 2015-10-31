// Accepts a filename as input or reads from stdin.
#include <wc/count.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <boost/optional.hpp>


using namespace wc;
using boost::optional;
using std::string;




int main(int argc, const char * * args) {
    optional<string> file_name;
    if (argc > 1) {
        file_name = args[1];
    }

    word_counter counter;
    auto processor = [&counter](auto begin, auto end) {
        return read_blob(begin, end, counter);
    };

    const size_t buffer_size = 10 * 1024;
    try {
        if (file_name) {
            std::ifstream actual_file(file_name.get(), std::ifstream::binary);
            actual_file.exceptions(std::ifstream::badbit);
            read_using_buffer<buffer_size>(actual_file, processor);
        } else {
            std::cin.exceptions(std::ifstream::badbit);
            read_using_buffer<buffer_size>(std::cin, processor);
        }
    } catch(const std::length_error &) {
        std::cerr << "A word in this file was too large to be processed."
                  << std::endl;
        return 2;
    }

    auto map = counter.words();
    for(auto itr = map.begin(); itr != map.end(); ++ itr) {
        std::cout << itr->first << "\t" << itr->second << "\n";
    }
}
