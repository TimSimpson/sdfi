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


int main(int argc, const char * * args) {
    if (argc < 4) {
        cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
             << " directory host port [host port...]" << endl;
        return 1;
    }
    wc::stop_watch watch;

    const string directory(args[1]);

    vector<wc::worker_args> workers;
    for (int i = 2; i + 1 < argc; i += 2) {
        workers.emplace_back(args[i], args[i + 1]);
    }

    int result = 1;
    try {
        auto producer_func = [&directory](auto & queues) {
            wc::queue_distributor<
                    std::vector<wc::queue_loader<wc::buffer_size> *>,
                    wc::queue_loader<wc::buffer_size>
                > dist(queues);
            wc::reader_thread(dist, directory);
        };
        result = wc::master_word_count(directory, workers, producer_func);
    } catch(const exception & e) {
        cerr << "An error occured: " << e.what() << endl;
        result = 1;
    }
    watch.print_time();
    return result;
}
