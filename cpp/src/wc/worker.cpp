// Starts a server on the given port and waits for an action.

#include <wc/count.h>
#include <wc/filesystem.h>
#include <wc/tcp.h>
#include <wc/top.h>
#include <iostream>
#include <sstream>
#include <boost/lexical_cast.hpp>


using namespace wc;
using std::string;
using std::stringstream;


int main(int argc, const char * * args) {
    if (argc < 2) {
        std::cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
                  << " port" << std::endl;
        return 1;
    }
    int port = boost::lexical_cast<int>(args[1]);


    word_counter counter;
    auto processor = [&counter](auto begin, auto end, bool eof) {
        return read_blob(begin, end, eof, counter);
    };

    const size_t buffer_size = 10 * 1024;

    server server(port);

    try
    {
        while(true)
        {
            string directory = server.read();

            // Read all words in the given directory.
            std::cout << "Reading directory " << directory << "..."
                      << std::endl;
            wc::read_directory<buffer_size>(processor, directory, std::cerr);
            std::cout << "Finished." << std::endl;

            // TODO: Using a streamstream is pretty dopey because the entirety
            //       of the message has to be buffered in memory, but it
            //       doesn't seem to affect anything. Still, look into it.

            // Create the giant message in memory.
            stringstream stream;
            for(const auto & word_info : counter.words()) {
                stream << word_info.first << "\t" << word_info.second << "\n";
            }

            // Change it into a giant string and send it.
            auto s = stream.str();

            std::cout << "Responding... (size == " << s.length() << ")"
                      << std::endl;
            server.write(s);
            //server.close();
            break;
        }
    } catch(const std::exception & e) {
        std::cerr << "An error occured: " << e.what() << std::endl;
        return 1;
    }
}
