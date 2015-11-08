// Starts a server on the given port and waits to receive a list of files,
// which it reads and then responds with a mapping between all words in the
// files and their count.

#include <wc/cmds.h>
#include <wc/communication.h>
#include <wc/count.h>
#include <wc/filesystem.h>
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


struct server_reader {
    char last_byte;
    string last_string;
    wc::server & server;

    server_reader(wc::server & server)
    :   last_byte(),
        last_string(),
        server(server)
    {
        last_byte = server.receive_byte();
    }

    bool has_more() const {
        return last_string.size() != 0 || last_byte == '.';
    }

    template<typename Iterator, typename SizeType>
    SizeType read(Iterator output_buffer, SizeType count) {
        if (last_string.size() == 0) {
            last_string = server.receive();
            last_byte = server.receive_byte();
        }
        if (count >= last_string.size()) {
            std::copy(last_string.begin(), last_string.end(), output_buffer);
            SizeType result = last_string.size();
            last_string.erase();
            return result;
        } else {
            std::copy(last_string.begin(), last_string.begin() + count,
                      output_buffer);
            last_string.erase(0, count);
            return count;
        }
    }
};


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

            vector<string> files;
            cout << "Reading in text..." << endl;

            wc::word_counter counter;

            server_reader reader(server);  // Waits for first byte.

            wc::stop_watch watch;

            auto processor = [&counter](auto begin, auto end, auto eof) {
                return wc::read_blob(begin, end, eof, counter);
            };
            wc::read_using_buffer<buffer_size>(reader, processor);

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
            server.send(s);
            watch.print_time();
        }
    } catch(const exception & e) {
        cerr << "An error occured: " << e.what() << endl;
        return 1;
    }
}
