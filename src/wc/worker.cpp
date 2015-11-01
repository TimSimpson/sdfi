// Starts a server on the given port and waits for an action.

#include <wc/count.h>
#include <wc/filesystem.h>
#include <wc/tcp.h>
#include <wc/top.h>
#include <iostream>
#include <sstream>
#include <boost/lexical_cast.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using boost::lexical_cast;
using std::string;
using std::stringstream;
using std::vector;

int main(int argc, const char * * args) {
    if (argc < 2) {
        cerr << "Usage:" << ((argc > 0) ? args[0] : "prog") << " port" << endl;
        return 1;
    }
    int port = lexical_cast<int>(args[1]);
    const size_t buffer_size = 10 * 1024;

    try
    {
        while(true)
        {
            wc::server server(port);

            wc::word_counter counter;
            auto processor = [&counter](auto begin, auto end, bool eof) {
                return read_blob(begin, end, eof, counter);
            };

            vector<string> files;
            cout << "Reading in directory list..." << endl;

            string input = server.read();
            while(input != ";]-done") {
                files.push_back(input);
                input = server.read();
            }

            // Read all words in the given directory.
            for(const string & file : files) {
                cout << "Reading file \"" << file << "\"..." << endl;
                wc::read_file<buffer_size>(processor, file);
            }

            cout << "Finished." << endl;

            // TODO: Using a streamstream is pretty dopey because the entirety
            //       of the message has to be buffered in memory, but it
            //       doesn't seem to affect performance. However in theory
            //       this could cause problems if you wanted to look through
            //       arbitrary files with a huge number of potential "words."

            // Create the giant message in memory.
            stringstream stream;
            for(const auto & word_info : counter.words()) {
                stream << word_info.first << "\t" << word_info.second << "\n";
            }

            // Change it into a giant string and send it.
            auto s = stream.str();

            cout << "Responding... (size == " << s.length() << ")" << endl;
            server.write(s);
        }
    } catch(const exception & e) {
        cerr << "An error occured: " << e.what() << endl;
        return 1;
    }
}
