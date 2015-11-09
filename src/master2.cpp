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
#include <wc/master.h>
#include <iostream>
#include <memory>
#include <thread>
#include <boost/lockfree/spsc_queue.hpp>
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


using queue_loader = wc::queue_loader<wc::buffer_size>;
using queue_loader_vec = vector<queue_loader *>;


int word_count(const string & directory,
               vector<unique_ptr<wc::worker>> & workers) {
    queue_loader_vec queues;
    for(auto & w_ptr : workers) {
        queues.push_back(&(w_ptr->loader));
    }

    std::thread produce([directory, &queues](){
        wc::queue_distributor<queue_loader_vec, queue_loader> dist(queues);
        wc::reader_thread(dist, directory);
    });
    vector<std::thread> consumers;
    consumers.reserve(workers.size());
    for (auto & w_ptr : workers) {
        wc::worker & w = *w_ptr;
        consumers.emplace_back([&w](){
            wc::sender_thread(w);
        });
    }

    cout << "Waiting..." << endl;

    // Wait for distributor to finish.
    produce.join();
    // Notify the queues that no more data will be arriving.
    for(queue_loader * l : queues) {
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

    wc::print_top_words(totals);

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
    vector<unique_ptr<wc::worker>> workers;
    for (int i = 2; i + 1 < argc; i+= 2) {
        workers.push_back(std::make_unique<wc::worker>(args[i], args[i + 1]));
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
