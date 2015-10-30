// Accepts a filename as input or reads from stdin.
#include <wc/count.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <boost/optional.hpp>


using namespace wc;


class file_reader {
public:
    file_reader(const char * file_name)
    :   file(file_name, std::ifstream::binary)
    {
        if (file)
        {
            file.exceptions(std::ifstream::badbit);
        }
    }

     explicit operator bool() const {
       return file && file.good() && !file.eof();
     }

    template<typename Iterator>
    size_t operator() (Iterator start, size_t max) {
        file.read(start, max);
        return file.gcount();
    }

private:
    std::ifstream file;
};


class cin_reader {
public:
    explicit operator bool() const {
        return std::cin && std::cin.good() && !std::cin.eof();
    }

    template<typename Iterator>
    size_t operator() (Iterator start, size_t max) {
        std::cin.read(start, max);
        return std::cin.gcount();
    }
};


int main(int argc, const char * * args) {
    boost::optional<string> file_name;
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
            file_reader reader(file_name.get());
            std::cerr << "Couldn't read file " << file_name.get() << "."
                      << std::endl;
            read_using_buffer<buffer_size>(reader, processor);
        } else {
            cin_reader reader;
            read_using_buffer<buffer_size>(reader, processor);
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
