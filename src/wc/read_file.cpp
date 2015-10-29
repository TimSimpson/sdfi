#include <wc/count.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>


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


int main(int argc, const char * * args) {
    if (argc < 2) {
        std::cerr << "Usage: " << ((argc > 0) ? args[0] : "prog")
                  << " filename" << std::endl;
        return 1;
    }

    file_reader file(args[1]);
	if (!file) {
		std::cerr << "Couldn't read file " << args[1] << "." << std::endl;
		return 1;
	}
    word_counter counter;
    auto processor = [&counter](auto begin, auto end) {
        return read_blob(begin, end, counter);
    };
    const size_t buffer_size = 10 * 1024;
    try {
        read_using_buffer<buffer_size>(file, processor);
    } catch(const std::length_error &) {
        std::cerr << "A word in this file was too large to be processed."
                  << std::endl;
        return 2;
    }
}
