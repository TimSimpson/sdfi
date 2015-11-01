// Connects to a worker and tells it what to do.

#include <wc/count.h>
#include <wc/tcp.h>
#include <wc/top.h>
#include <iostream>
#include <boost/lexical_cast.hpp>


using namespace wc;
using std::string;
using std::vector;


struct worker {
    string host;
    string port;
};


int word_count(const std::string & directory, const vector<worker> & workers);


int main(int argc, const char * * args) {
    if (argc < 4) {
        std::cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
                  << " directory host port [host port...]" << std::endl;
        return 1;
    }
    string directory(args[1]);
    vector<worker> workers;

    for (int i = 2; i + 1 < argc; i+= 2) {
        worker w;
        w.host = args[i];
        w.port = args[i + 1];
        workers.push_back(w);
    }

    return word_count(directory, workers);
}

int word_count(const std::string & directory, const vector<worker> & workers) {
    try {
        wc::client client(workers[0].host, workers[0].port);
        client.send(directory);

        wc::word_counter word_counts;

        bool error_occured = false;

        string response = client.async_receive<1024 * 4>(
            [&word_counts](auto start, auto end, bool eof) {
                std::cerr << ".";
                return wc::read_blob(start, end, eof, word_counts);
            },
            [&word_counts]() {
                wc::top_word_collection<10> top_words;
                for(const auto & word_info : word_counts.words()) {
                    top_words.add(word_info.first, word_info.second);
                }
                std::cout << std::endl;
                std::cout << "Top words: " << std::endl;
                std::cout << std::endl;
                for(int i = 0; i < top_words.total_words(); ++i) {
                    const auto word_info = top_words.get_words()[i];
                    std::cout << i + 1 << ". " << word_info.first
                              << "\t" << word_info.second << "\n";
                }
            },
            [&error_occured](const std::string & msg) {
                error_occured = true;
                std::cerr << msg << std::endl;
            }
        );
        if (error_occured) {
            return 1;
        }
    } catch(const std::exception & e) {
        std::cerr << "An error occured: " << e.what() << std::endl;
        return 1;
    }
}
