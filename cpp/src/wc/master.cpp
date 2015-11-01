// Connects to a worker and tells it what to do.

#include <wc/count.h>
#include <wc/tcp.h>
#include <wc/top.h>
#include <iostream>
#include <boost/lexical_cast.hpp>


using namespace wc;
using std::string;
using std::vector;


// Contains all of the info / state for a worker.
struct worker {
    string host;
    string port;
    bool error_occured;
    bool finished;
    wc::word_map results;

    worker(const string & host, const string & port)
    :   host(host),
        port(port),
        error_occured(false),
        finished(false),
        results()
    {
    }

    // The workers send out a series of words and their counts over and over.
    // This function captures that data, recording it into a map.
    template<typename Iterator>
    void operator()(Iterator start, Iterator end) {
        is_odd = !is_odd;
        if (is_odd) {
            last_word = string(start, end);
        } else {
            string count_str(start, end);
            std::size_t count =
                boost::lexical_cast<std::size_t>(count_str);
            results[last_word] = count;
        }
    }

private:
    bool is_odd;
    string last_word;
};


int word_count(const std::string & directory, vector<worker> & workers);


int main(int argc, const char * * args) {
    if (argc < 4) {
        std::cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
                  << " directory host port [host port...]" << std::endl;
        return 1;
    }
    string directory(args[1]);
    vector<worker> workers;

    for (int i = 2; i + 1 < argc; i+= 2) {
        workers.emplace_back(args[i], args[i + 1]);
    }

    try {
        return word_count(directory, workers);
    } catch(const std::exception & e) {
        std::cerr << "An error occured: " << e.what() << std::endl;
        return 1;
    }
}


// Tells a worker to start counting words. The output of this function
// is the second argument.
void start_worker(boost::asio::io_service & ioservice, worker & worker,
                  const string & directory)
{
    std::cout << "Starting worker " << worker.host << "..." << std::endl;
    wc::client client(ioservice, worker.host, worker.port);
    client.send(directory);

    string response = client.async_receive<1024 * 4>(
        [&worker](
            auto begin, auto end, bool eof
        ) {
            return wc::read_blob(begin, end, eof, worker);
        },
        [&worker]() {
            worker.finished = true;
        },
        [&worker](const std::string & msg) {
            worker.error_occured = true;
            std::cerr << msg << std::endl;
        }
    );
}


int word_count(const std::string & directory, vector<worker> & workers) {
    boost::asio::io_service ioservice;

    for (int i = 0; i < workers.size(); ++ i) {
        start_worker(ioservice, workers[i], directory);
    }

    std::cout << "Waiting..." << std::endl;
    // Wait for everyone to finish.
    ioservice.run();

    // Commandeer the first element's results and use it for the running total.
    wc::word_map totals = workers[0].results;
    // Add in the totals from all of the other workers.
    for (int i = 1; i < workers.size(); ++ i) {
        auto rhs = workers[i].results;
        for (auto itr = rhs.begin(); itr != rhs.end(); ++ itr) {
            totals[itr->first] += itr->second;
        }
    }

    // Now insert into word count and print.
    wc::top_word_collection<10> top_words;
    for(auto itr = totals.begin(); itr != totals.end(); ++ itr) {
        top_words.add(itr->first, itr->second);
    }
    std::cout << std::endl;
    std::cout << "Top words: " << std::endl;
    std::cout << std::endl;
    for(int i = 0; i < top_words.total_words(); ++i) {
        const auto word_info = top_words.get_words()[i];
        std::cout << i + 1 << ". " << word_info.first
                  << "\t" << word_info.second << "\n";
    }

}
