// Given a directory and one or more worker process, iterates the directories
// and sends each worker process a fraction of the files. After receiving the
// word counts from each process, combines them all and prints a list of the
// top 10 words seen in all files.

#include <wc/cmds.h>
#include <wc/communication.h>
#include <wc/count.h>
#include <wc/filesystem.h>
#include <wc/top.h>
#include <iostream>
#include <boost/lexical_cast.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using boost::lexical_cast;
using std::size_t;
using std::string;
using std::vector;


// Contains all of the info / state for a worker.
struct worker {
    string host;
    string port;
    vector<string> files;
    wc::worker_results_collector results_collector;

    worker(const string & host, const string & port)
    :   host(host),
        port(port),
        files(),
        results_collector()
    {
    }
};

// Tells a worker to start counting words. The output of this function
// is the second argument.
void start_worker(boost::asio::io_service & ioservice, worker & worker)
{
    cout << "Starting worker " << worker.host << "...\n";
    wc::client client(ioservice, worker.host, worker.port);

    for (const auto & file : worker.files) {
        client.send(file);
    }
    client.send(";]-done");

    wc::async_collect_results(client, worker.results_collector);
}

int word_count(const string & directory, vector<worker> & workers) {
    // Read directory and create list of files for each worker process.
    // TODO: Sending the files one at a time to each process, letting them
    //       respond, then sending new ones would probably distribute the
    //       work load better in the presence of infrequent but large files.

    int file_count = -1;
    auto record_file = [&file_count, &workers](const string & file){
        file_count ++;
        const int worker_index = (file_count % workers.size());
        workers[worker_index].files.push_back(file);
    };
    wc::read_directory(record_file, directory, cerr);

    boost::asio::io_service ioservice;

    for (size_t i = 0; i < workers.size(); ++ i) {
        start_worker(ioservice, workers[i]);
    }

    cout << "Waiting...\n";
    // The line below will wait until all of the receiver Boost ASIO async code
    // has executed.
    ioservice.run();

    cout << "Finished...\n";
    for (const auto worker : workers) {
        if (worker.results_collector.error_occured()) {
            cerr << "An error occured on " << worker.host << " "
                 << worker.port << ". Results are invalid. :("
                 << endl;
            return 2;
        }
        if (!worker.results_collector.is_finished()) {
            cerr << "Worker didn't finish: " << worker.host << " "
                 << worker.port << "."
                 << endl;
            return 2;
        }
    }

    cout << "Performing final count...\n";

    wc::word_map totals = wc::sum_word_maps(
        workers,
        [](worker & w) {
            return w.results_collector.get_results();
        }
    );

    wc::print_top_words(totals);

    return 0;
}

int main(int argc, const char * * args) {
    if (argc < 4) {
        cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
             << " directory host port [host port...]\n";
        return 1;
    }
    wc::stop_watch watch;

    string directory(args[1]);
    vector<worker> workers;
    for (int i = 2; i + 1 < argc; i+= 2) {
        workers.emplace_back(args[i], args[i + 1]);
    }

    int result = 1;
    try {
        result = word_count(directory, workers);
    } catch(const exception & e) {
        cerr << "An error occured: " << e.what() << "\n";
        result = 1;
    }
    watch.print_time();
    return result;
}
