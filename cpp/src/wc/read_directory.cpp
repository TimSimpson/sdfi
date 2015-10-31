// Accepts a filename as input or reads from stdin.
#include <wc/count.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>


using namespace wc;
using std::string;
using boost::filesystem::current_path;
using boost::filesystem::path;
using boost::filesystem::recursive_directory_iterator;


int main(int argc, const char * * args) {
    if (argc < 2) {
        std::cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
                  << " [directory]" << std::endl;
        return 1;
    }

    path p = current_path() / args[1];
    for (recursive_directory_iterator itr(p);
         itr != recursive_directory_iterator{};
         ++ itr) {
        std::cout << *itr << std::endl;
    }
}
