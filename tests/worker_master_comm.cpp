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
using std::unique_ptr;



int main(int argc, const char * * args) {
    const vector<int> ports = { 7111, 7112, 7113 };
    const std::size_t words_to_send = 100000; // 1000000;

    vector<wc::word_map> results;
    results.reserve(ports.size());

    using queue_type = wc::queue_loader<wc::buffer_size>;
    vector<unique_ptr<queue_type>> queues;
    queues.reserve(ports.size());

    vector<std::thread> worker_threads;
    worker_threads.reserve(ports.size());

    vector<std::thread> senders;
    senders.reserve(ports.size());

    for (const int port : ports) {
        results.push_back(wc::word_map{});
        wc::word_map & r = results.back();
        auto worker_func = [port, &r]() {
            wc::run_worker(port, [&r](const auto & all_words,
                                            auto & response){
                cerr << "MARIO hi" << endl;
                r = all_words;
                cerr << "MARIO hi2" << endl;
            });
        };
        worker_threads.emplace_back(worker_func);
    }

    wc::nap();
    for (int port : ports) {
        queues.push_back(std::make_unique<queue_type>());
        auto * queue = queues.back().get();
        auto master_sender = [port, queue] () {
            boost::asio::io_service io;
            wc::client client(io, "localhost", boost::lexical_cast<string>(port));
            queue->consume_and_send_data([&client](auto buffer, auto count) {
                client.send_byte('.');
                client.send(string(buffer, buffer + count));
            });
            client.send_byte('!');
            //TODO: Maybe capture the results and compare?
            wc::worker_results_collector trash;
            wc::async_collect_results(client, trash);
            io.run();
        };
        senders.emplace_back(master_sender);
    }

    auto master_reader = [&queues, words_to_send] () {
        for (size_t i = 0; i < words_to_send; ++i) {
            for (auto & queue : queues) {
                string text("the");
                queue->process_text(text.begin(), text.end());
            }
        }
    };
    std::thread master_thread_reader(master_reader);

    cerr << "Waiting for 'reader'." << endl;
    master_thread_reader.join();

    cerr << "Finishing queues." << endl;

    for (auto & queue : queues) {
        queue->finish();
    }

    cerr << "Finishing senders." << endl;
    for (std::thread & t : senders) {
        t.join();
    }

    cerr << "Finishing workers." << endl;
    for (std::thread & t : worker_threads) {
        t.join();
    }

    cerr << "Finishing, summing all words..." << endl;

    wc::word_map totals = wc::sum_word_maps(
        results, [](wc::word_map & w) { return w; }
    );

    cout << "Number of 'the' words seen: " << totals["the"] << endl;
    cout << "Number of unique words seen: " << totals.size() << endl;
    wc::print_word_map(totals);
    wc::print_top_words(totals);
    int expected_words = words_to_send * queues.size();
    cout << "Expected words: " << expected_words << "\n";
    cout << (totals["the"] == expected_words ? ":)" : "FAIL!!") << endl;
    return totals["the"] == expected_words ? 0 : 1;
}
