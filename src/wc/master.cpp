// Connects to a worker and tells it what to do.

#include <wc/cmds.h>
#include <wc/count.h>
#include <wc/filesystem.h>
#include <wc/tcp.h>
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
            size_t count = lexical_cast<size_t>(count_str);
            results[last_word] = count;
        }
    }

private:
    bool is_odd;
    string last_word;
};

// Tells a worker to start counting words. The output of this function
// is the second argument.
void start_worker(boost::asio::io_service & ioservice, worker & worker)
{
    cout << "Starting worker " << worker.host << "..." << endl;
    wc::client client(ioservice, worker.host, worker.port);

    for (const auto & file : worker.files) {
        client.send(file);
    }
    client.send(";]-done");

    client.async_receive<1024 * 10>(
        [&worker](
            auto begin, auto end, bool eof
        ) {
            return wc::read_blob(begin, end, eof, worker);
        },
        [&worker]() {
            worker.finished = true;
        },
        [&worker](const string & msg) {
            worker.error_occured = true;
            cerr << msg << endl;
        }
    );
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

    cout << "Waiting..." << endl;
    // The line below will wait until all of the receiver Boost ASIO async code
    // has executed.
    ioservice.run();

    cout << "Finished..." << endl;
    for (const auto worker : workers) {
        if (worker.error_occured) {
            cerr << "An error occured on " << worker.host << " "
                 << worker.port << ". Results are invalid. :("
                 << endl;
            return 2;
        }
        if (!worker.finished) {
            cerr << "Worker didn't finish: " << worker.host << " "
                 << worker.port << "."
                 << endl;
            return 2;
        }
    }

    cout << "Performing final count..." << endl;

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
    cout << endl;
    cout << "Top words: " << endl;
    cout << endl;
    for(int i = 0; i < top_words.total_words(); ++i) {
        const auto word_info = top_words.get_words()[i];
        cout << i + 1 << ". " << word_info.first
             << "\t" << word_info.second << "\n";
    }
    return 0;
}

int main(int argc, const char * * args) {
    if (argc < 4) {
        cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
             << " directory host port [host port...]" << endl;
        return 1;
    }
    wc::timer t;

    string directory(args[1]);
    vector<worker> workers;
    for (int i = 2; i + 1 < argc; i+= 2) {
        workers.emplace_back(args[i], args[i + 1]);
    }

    try {
        return word_count(directory, workers);
    } catch(const exception & e) {
        cerr << "An error occured: " << e.what() << endl;
        return 1;
    }
}
