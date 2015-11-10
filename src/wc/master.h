// Helper functionality for the master binaries.
#ifndef FILE_GUARD_WC_MASTER_H
#define FILE_GUARD_WC_MASTER_H

#include <wc/cmds.h>
#include <wc/communication.h>


namespace wc {

typedef std::vector<queue_loader<buffer_size> *> queue_loader_vec;

struct worker_args {
    std::string host;
    std::string port;

    worker_args(const std::string & host, const std::string & port)
    :   host(host),
        port(port)
    {
    }
};

// Contains all of the info / state for a worker.
struct worker {
    std::string host;
    std::string port;
    queue_loader<buffer_size> loader;
    worker_results_collector results_collector;

    worker(const std::string & host, const std::string & port)
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
    client client(io, w.host, w.port);
    w.loader.consume_and_send_data([&client](auto buffer, auto count) {
        client.send_byte('.');
        client.send(std::string(buffer, buffer + count));
    });
    client.send_byte('!');
    async_collect_results(client, w.results_collector);
    io.run();
}

template<typename Distributor>
void reader_thread(Distributor & distributor,
                   const std::string & root_directory) {
    auto file_handler = [&distributor](const std::string full_path) {
        std::cerr << "Reading file \"" << full_path << "\"..." << std::endl;
        read_file<buffer_size>(distributor, full_path);
    };
    read_directory(file_handler, root_directory, std::cerr);
}

template<typename Function>
int master_word_count(const std::string & directory,
                      std::vector<worker_args> & args,
                      Function & producer_function) {

    std::vector<std::unique_ptr<worker>> workers;
    for (const worker_args & pair : args) {
        workers.push_back(std::make_unique<worker>(pair.host, pair.port));
    }

    std::vector<queue_loader<buffer_size> *> queues;
    for(auto & w_ptr : workers) {
        queues.push_back(&(w_ptr->loader));
    }

    auto thread_producer_function = [&queues, &producer_function]() {
        producer_function(queues);
    };
    std::thread produce(thread_producer_function);
    std::vector<std::thread> consumers;
    consumers.reserve(workers.size());
    for (auto & w_ptr : workers) {
        worker & w = *w_ptr;
        consumers.emplace_back([&w](){
            sender_thread(w);
        });
    }

    std::cout << "Waiting...\n";

    // Wait for distributor to finish.
    produce.join();
    // Notify the queues that no more data will be arriving.
    for(auto * l : queues) {
        l->finish();
    }
    // Wait for threads to end.
    for (std::thread & t : consumers) {
        t.join();
    }

    std::cout << "Finished...\n";
    for (const auto & w_ptr : workers) {
        if (w_ptr->results_collector.error_occured()) {
            std::cerr << "An error occured on " << w_ptr->host << " "
                      << w_ptr->port << ". Results are invalid. :(\n";
            return 2;
        }
        if (!w_ptr->results_collector.is_finished()) {
            std::cerr << "Worker didn't finish: " << w_ptr->host << " "
                      << w_ptr->port << ".\n";
            return 2;
        }
    }

    std::cout << "Performing final count...\n";

    // Commandeer the first element's results and use it for the running total.
    word_map & totals = workers[0]->results_collector.get_results();
    // Add in the totals from all of the other workers.
    for (int i = 1; i < workers.size(); ++ i) {
        auto rhs = workers[i]->results_collector.get_results();
        for (auto itr = rhs.begin(); itr != rhs.end(); ++ itr) {
            totals[itr->first] += itr->second;
        }
    }

    print_top_words(totals);

    return 0;
}



}

#endif
