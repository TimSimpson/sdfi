// Tests that worker / master communication is working.

#include <wc/cmds.h>
#include <wc/communication.h>
#include <wc/count.h>
#include <wc/filesystem.h>
#include <wc/top.h>
#include <wc/worker.h>
#include <iostream>
#include <sstream>
#include <boost/lexical_cast.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using boost::lexical_cast;
using std::string;
using std::stringstream;
using std::vector;



int main(int argc, const char * * args) {
    const int port = 7111;
    const std::size_t words_to_send = 10000000;

    wc::word_map results;

    wc::queue_loader<wc::buffer_size> queue;

    auto worker = [&results]() {
        wc::run_worker(port, [&results](const auto & all_words, auto & response){
            results = all_words;
        });
    };

    auto master_sender = [port, &queue] () {
        boost::asio::io_service io;
        wc::client client(io, "localhost", boost::lexical_cast<string>(port));
        queue.consume_and_send_data([&client](auto buffer, auto count) {
            client.send_byte('.');
            client.send(string(buffer, buffer + count));
        });
        client.send_byte('!');
    };

    auto master_reader = [&queue, words_to_send] () {
        for (size_t i = 0; i < words_to_send; ++ i) {
            string text("the");
            queue.process_text(text.begin(), text.end());
        }
    };

    std::thread worker_thread(worker);
    wc::nap();
    std::thread master_thread_sender(master_sender);
    std::thread master_thread_reader(master_reader);

    master_thread_reader.join();
    queue.finish();
    master_thread_sender.join();
    worker_thread.join();

    cout << "Number of 'the' words seen: " << results["the"] << endl;
    cout << (results["the"] == words_to_send ? ":)" : "FAIL!!") << endl;
    return results["the"] == words_to_send ? 0 : 1;
}
