// Helper functionality for the master binaries.
#ifndef FILE_GUARD_WC_MASTER_H
#define FILE_GUARD_WC_MASTER_H

#include <wc/cmds.h>
#include <wc/communication.h>


namespace wc {

typedef std::vector<queue_loader<wc::buffer_size> *> queue_loader_vec;

// Contains all of the info / state for a worker.
struct worker {
    std::string host;
    std::string port;
    queue_loader<wc::buffer_size> loader;
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
    wc::client client(io, w.host, w.port);
    w.loader.consume_and_send_data([&client](auto buffer, auto count) {
        client.send_byte('.');
        client.send(std::string(buffer, buffer + count));
    });
    client.send_byte('!');
    wc::async_collect_results(client, w.results_collector);
    io.run();
}

template<typename Distributor>
void reader_thread(Distributor & distributor,
                   const std::string & root_directory) {
    auto file_handler = [&distributor](const std::string full_path) {
        std::cerr << "Reading file \"" << full_path << "\"..." << std::endl;
        read_file<wc::buffer_size>(distributor, full_path);
    };
    read_directory(file_handler, root_directory, std::cerr);
}



}

#endif
