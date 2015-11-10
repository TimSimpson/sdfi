// Starts a server on the given port and waits to receive a list of files,
// which it reads and then responds with a mapping between all words in the
// files and their count.

#include <wc/cmds.h>
#include <wc/communication.h>
#include <wc/count.h>
#include <wc/filesystem.h>
#include <wc/top.h>
#include <wc/worker.h>
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
    cout << "Worker 2\n";
    if (argc < 2) {
        cerr << "Usage:" << ((argc > 0) ? args[0] : "prog") << " port" << endl;
        return 1;
    }
    int port = lexical_cast<int>(args[1]);
    try
    {
        while(true)
        {
            wc::run_worker(port, [](const auto & all_words, auto & response){
                // Return the counts of every word.
                for(const auto & word_info : all_words) {
                    response << word_info.first << "\t"
                             << word_info.second << "\n";
                }
            });
        }
    } catch(const exception & e) {
        cerr << "An error occured: " << e.what() << endl;
        return 1;
    }
}
