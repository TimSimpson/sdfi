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

            wc::server_reader reader(&server);  // Waits for first byte.

            wc::stop_watch watch;

            auto processor = [&counter](auto begin, auto end, auto eof) {
                return wc::read_blob(begin, end, eof, counter);
            };
            wc::read_using_buffer<buffer_size>(reader, processor);

            cout << "Finished." << endl;

            // Find the top 10 words.
            wc::top_word_collection<10> top_words;
            const wc::word_map & totals = counter.words();
            for(auto itr = totals.begin(); itr != totals.end(); ++ itr) {
                top_words.add(itr->first, itr->second);
            }

            // Stream back the top words.
            stringstream stream;
            for(int i = 0; i < top_words.total_words(); ++i) {
                const auto word_info = top_words.get_words()[i];
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
