#ifndef FILE_GUARD_WC_FILESYSTEM_H
#define FILE_GUARD_WC_FILESYSTEM_H

#include <fstream>
#include <string>
#include <boost/filesystem.hpp>

namespace wc {

// Reads a file using read_using_buffer, sending it to a processor function.
template<int buffer_size, typename Processor>
void read_file(Processor & processor, const std::string & full_path) {
    std::ifstream actual_file(full_path, std::ifstream::binary);
    actual_file.exceptions(std::ifstream::badbit);
    read_using_buffer<buffer_size>(actual_file, processor);
}

template<typename FileHandler, typename LogStream>
void read_directory(FileHandler & file_handler,
                    const std::string & directory,
                    LogStream & log) {
    using boost::filesystem::current_path;
    using boost::filesystem::exists;
    using boost::filesystem::is_regular_file;
    using boost::filesystem::path;
    using boost::filesystem::recursive_directory_iterator;

    path root(directory);
    if (!exists(directory)) {
        log << "\"" << directory << "\" does not exist." << std::endl;
        throw std::runtime_error("Directory not found.");
    }
    for (recursive_directory_iterator itr(root);
         itr != recursive_directory_iterator{};
         ++ itr) {
        path file_path(*itr);
        if (is_regular_file(file_path)) {
            // Divy up files in a directory if using multiple workers.
            try {
                file_handler(file_path.string());
            } catch(const std::length_error &) {
                log << "A word in file \"" << file_path << "\" was too "
                    "large to be processed." << std::endl;
                throw;
            }
        }
    }
}

} // end wc namespace

#endif
