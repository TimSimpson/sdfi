// Connects to a worker and tells it what to do.

#include <wc/count.h>
#include <wc/tcp.h>
#include <wc/top.h>
#include <iostream>
#include <boost/lexical_cast.hpp>


using namespace wc;
using std::string;


int main(int argc, const char * * args) {
    if (argc < 4) {
        std::cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
                  << " directory host port" << std::endl;
        return 1;
    }
    string directory(args[1]);
    string host(args[2]);
    string port(args[3]);

    try {
        wc::client client(host, port);
        client.send(directory);
        string response = client.receive();
        // std::cout << response << std::endl;

        wc::top_word_collection<10> top_words;
        string last_word;
        bool is_odd = false;
        auto record_top_words = [&top_words, &last_word, &is_odd](
            auto begin, auto end
        ) {
            is_odd = !is_odd;
            if (is_odd) {
                last_word = string(begin, end);
            } else {
                int count = boost::lexical_cast<int>(string(begin, end));
                top_words.add(last_word, count);
            }
        };

        wc::read_blob(response.begin(), response.end(), true, record_top_words);

        std::cout << std::endl;
        std::cout << "Top words: " << std::endl;
        std::cout << std::endl;
        for(int i = 0; i < top_words.total_words(); ++i) {
            const auto word_info = top_words.get_words()[i];
            std::cout << i + 1 << ". " << word_info.first
                      << "\t" << word_info.second << "\n";
        }
    } catch(const std::exception & e) {
        std::cerr << "An error occured: " << e.what() << std::endl;
    }
}
