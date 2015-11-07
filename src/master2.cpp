// Given a directory and one or more worker process, iterates the directories
// and sends each worker process a fraction of the files. After receiving the
// word counts from each process, combines them all and prints a list of the
// top 10 words seen in all files.


#include <wc/cmds.h>
#include <wc/communication.h>
#include <wc/count.h>
#include <wc/filesystem.h>
#include <wc/tcp.h>
#include <wc/top.h>
#include <iostream>
#include <memory>
#include <thread>

//#define GCC_IS_CRASHING

#ifdef GCC_IS_CRASHING
    #include <boost/lockfree/queue.hpp>
#else
    //TODO: I really *WANTED* to use the spsc_queue, but I can't because
    //      GCC 5.2 keeps crashing when I simply include the file! >:-(
    #include <boost/lockfree/spsc_queue.hpp>
#endif
#include <boost/lexical_cast.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using boost::lexical_cast;
using std::size_t;
using std::string;
using std::unique_ptr;
using std::vector;

#ifdef GCC_IS_CRASHING
    template<typename T>
    using boost_queue = boost::lockfree::queue<T>;
#else
    template<typename T>
    using boost_queue = boost::lockfree::spsc_queue<T>;
#endif

// http://www.boost.org/doc/libs/1_59_0/doc/html/boost/lockfree/spsc_queue.html
// http://theboostcpplibraries.com/boost.lockfree

constexpr size_t buffer_size = 10 * 1024;


// This takes care of loading up the workers with data by using a queue
// which is written to by the distributor thread and read from by one of the
// sender threads.
class queue_loader {
public:
    queue_loader()
    :   shared_buffer(buffer_size),
        send(true)
    {}

    queue_loader(const queue_loader & rhs) = delete;
    queue_loader & operator=(const queue_loader & rhs) = delete;

    // Called by the distributor to note that the job is finished.
    void finish() {
        send = false;
    }

    // Called by a different thread (only one other one btw) to wait for
    // data from the distributor and then send it.
	template<typename Function>
    void consume_and_send_data(Function on_send) {
        size_t count = 0;
        while(send || count != 0) {
            count = shared_buffer.pop(local_buffer, sizeof(local_buffer));
            on_send(local_buffer, count);
        }
    }

    // Called by the distributor to determine how much space is left for
    // writing.
    auto write_available() const {
        return shared_buffer.write_available();
    }

    // Called by the distributor.
	template<typename Iterator>
	Iterator process_text(Iterator begin, Iterator end) {
        return shared_buffer.push(begin, end);
    }
private:
    bool send;
    char local_buffer[buffer_size];
    boost_queue<char> shared_buffer;
};

// Contains all of the info / state for a worker.
struct worker {
    string host;
    string port;
    queue_loader loader;
    wc::worker_results_collector results_collector;

    worker(const string & host, const string & port)
    :   host(host),
        port(port),
        loader(),
        results_collector()
    {
    }

    worker(const worker & rhs) = delete;
    worker & operator=(const worker & rhs) = delete;
};


void sender_thread(worker & w) {
    boost::asio::io_service io;
    wc::client client(io, w.host, w.port);
    w.loader.consume_and_send_data([&client](auto buffer, auto count) {
        client.send(string(buffer, buffer + count));
    });
    wc::async_collect_results(client, w.results_collector);
    io.run();
}


void reader_thread(const string & root_directory,
                   vector<queue_loader *> & senders) {
    auto processor = [&senders](auto begin, auto end, bool eof) {
        // find max write_available on all clients
        auto max_itr = std::max_element(
            senders.begin(), senders.end(),
            [](const queue_loader * a, const queue_loader * b) {
                return a->write_available() < b->write_available();
            }
        );
        queue_loader * sender = *max_itr;
        const size_t write_size = sender->write_available();

        const size_t size_of_data = end - begin;

        // Discover how far we'll write...
        auto write_until = end;
        if (write_size < size_of_data) {
            write_until -= (size_of_data - write_size);
        }

        // Now see if we might be breaking up a word-
        if (!eof || write_until != end) {
            while(write_until != begin
                    && wc::is_word_character(*write_until)) {
                write_until --;
            }
        }

        // Finally send the data
        // We *have* to send the full chunk out to avoid stopping on a word.
        // In theory this should be quick since write_available had the
        // space free.
        while(begin != write_until) {
            begin = sender->process_text(begin, write_until);
        }
        return begin;
    };
    auto file_handler = [&processor](const string full_path) {
        cout << "Reading file \"" << full_path << "\"..." << endl;
        wc::read_file<buffer_size>(processor, full_path);
    };
    wc::read_directory(file_handler, root_directory, cerr);
}


int word_count(const string & directory, vector<unique_ptr<worker>> & workers) {
    vector<queue_loader *> loaders;
    for(auto & w_ptr : workers) {
        loaders.push_back(&w_ptr->loader);
    }

    std::thread produce([directory, &loaders](){
        reader_thread(directory, loaders);
    });
    vector<std::thread> consumers;
    for (auto & w_ptr : workers) {
        worker & w = *w_ptr;
        consumers.emplace_back([&w](){
            sender_thread(w);
        });
    }

    cout << "Waiting..." << endl;

    // Wait for distributor to finish.
    produce.join();
    // Notify the loaders that no more data will be arriving.
    for(queue_loader * l : loaders) {
        l->finish();
    }
    // Wait for threads to end.
    for (std::thread & t : consumers) {
        t.join();
    }


    cout << "Finished..." << endl;
    for (const auto & w_ptr : workers) {
        if (w_ptr->results_collector.error_occured()) {
            cerr << "An error occured on " << w_ptr->host << " "
                 << w_ptr->port << ". Results are invalid. :("
                 << endl;
            return 2;
        }
        if (!w_ptr->results_collector.is_finished()) {
            cerr << "Worker didn't finish: " << w_ptr->host << " "
                 << w_ptr->port << "."
                 << endl;
            return 2;
        }
    }

    cout << "Performing final count..." << endl;

    // Commandeer the first element's results and use it for the running total.
    wc::word_map & totals = workers[0]->results_collector.get_results();
    // Add in the totals from all of the other workers.
    for (int i = 1; i < workers.size(); ++ i) {
        auto rhs = workers[i]->results_collector.get_results();
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
    wc::stop_watch watch;

    string directory(args[1]);
    vector<unique_ptr<worker>> workers;
    for (int i = 2; i + 1 < argc; i+= 2) {
        workers.push_back(std::make_unique<worker>(args[i], args[i + 1]));
    }

    int result = 1;
    try {
        result = word_count(directory, workers);
    } catch(const exception & e) {
        cerr << "An error occured: " << e.what() << endl;
        result = 1;
    }
    watch.print_time();
    return result;
}
