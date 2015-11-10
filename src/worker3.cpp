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
using std::exception;
using boost::lexical_cast;
using std::string;
using std::stringstream;
using std::vector;



int main(int argc, const char * * args) {
    cout << "Worker 3\n";
    if (argc < 2) {
        cerr << "Usage:" << ((argc > 0) ? args[0] : "prog") << " port\n";
        return 1;
    }
    int port = lexical_cast<int>(args[1]);

    try
    {
        while(true)
        {
            wc::run_worker(port, [](const auto & all_words, auto & response){
                // Find the top 10 words.
                wc::top_word_collection<10> top_words;
                for(auto itr = all_words.begin();
                    itr != all_words.end();
                    ++ itr) {
                    top_words.add(itr->first, itr->second);
                }

                // Stream back the top words.
                for(int i = 0; i < top_words.total_words(); ++i) {
                    const auto word_info = top_words.get_words()[i];
                    response << word_info.first << "\t" << word_info.second << "\n";
                }
            });
        }
    } catch(const exception & e) {
        cerr << "An error occured: " << e.what() << "\n";
        return 1;
    }
}
