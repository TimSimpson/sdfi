// Accepts a directory as it's first argument and reads all files.

#include <wc/count.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>


using namespace wc;
using std::string;
using boost::filesystem::current_path;
using boost::filesystem::is_regular_file;
using boost::filesystem::path;
using boost::filesystem::recursive_directory_iterator;


template<typename Processor>
bool read_file(Processor & processor, const std::string & full_path) {
    std::cerr << "Reading file " << full_path << "..." << std::endl;
    const size_t buffer_size = 10 * 1024;
    try {
        std::ifstream actual_file(full_path, std::ifstream::binary);
        actual_file.exceptions(std::ifstream::badbit);
        read_using_buffer<buffer_size>(actual_file, processor);
        return true;
    } catch(const std::length_error &) {
        std::cerr << "A word in file \"" << full_path << "\" was too large "
                     "to be processed." << std::endl;
        return false;
    }
}

int main(int argc, const char * * args) {
    if (argc < 2) {
        std::cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
                  << " [directory]" << std::endl;
        return 1;
    }

    word_counter counter;
    auto processor = [&counter](auto begin, auto end) {
        return read_blob(begin, end, counter);
    };

    path root = current_path() / args[1];
    for (recursive_directory_iterator itr(root);
         itr != recursive_directory_iterator{};
         ++ itr) {
        path p(*itr);
        if (is_regular_file(p)) {
            if (!read_file(processor, p.string())) {
                return 2;
            }
        }
    }

    auto map = counter.words();
    for(auto itr = map.begin(); itr != map.end(); ++ itr) {
        std::cout << itr->first << "\t" << itr->second << "\n";
    }
}
