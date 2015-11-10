// Tests that worker / master communication is working.

#include <wc/cmds.h>
#include <wc/communication.h>
#include <wc/count.h>
#include <wc/filesystem.h>
#include <wc/top.h>
#include <wc/worker.h>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <wc/master.h>

using std::cerr;
using std::cout;
using std::endl;
using boost::lexical_cast;
using std::string;
using std::vector;
using std::unique_ptr;


int main(int argc, const char * * args) {
    if (argc < 3) {
        cerr << "Usage:"
             << "\tOpens up multiple worker threads, actually opening \n"
             << "\tports, and then reads a directory. When finished, it \n"
             << "\tchecks that the number of times the word \"the\" appears \n"
             << "\tis equal to the argument given at the command line. \n"
             << "\tIf it is, it runs again. It does this forever or until \n"
             << "\tit fails. This test is useful for finding async bugs.\n"
             << "Usage:" << ((argc > 0) ? args[0] : "prog")
             << " directory number-of-times-\"the\"-appears\n";
        return 1;
    }
    const string directory(args[1]);
    const int expected_words(boost::lexical_cast<int>(string(args[2])));

    int program_result = 0;
    while(program_result == 0) {
        const vector<int> ports = { 7111, 7112, 7113, 7114, 7115, 7116 };
        const std::size_t words_to_send = 1000000; // 1000000;

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
                    r = all_words;
                });
            };
            worker_threads.emplace_back(worker_func);
        }

        // Give the worker threads a chance to start.
        // TODO: Fix race condition... in some universe where fixing a race
        //       condition for this manual test is worth doing.
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

        using queue_loader = wc::queue_loader<wc::buffer_size>;
        using queue_loader_vec = vector<queue_loader *>;

        queue_loader_vec queue_ptrs;
        for (auto & q : queues) {
            queue_ptrs.push_back(q.get());
        }

        auto master_reader = [&queue_ptrs, words_to_send, &directory] () {
            wc::queue_distributor<queue_loader_vec, queue_loader> dist(queue_ptrs);

            auto file_handler = [&dist](const std::string full_path) {
                std::cerr << "Reading file \"" << full_path << "\"..." << std::endl;
                wc::read_file<wc::buffer_size>(dist, full_path);
            };
            wc::read_directory(file_handler, directory, std::cerr);
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
        // wc::print_word_map(totals);
        wc::print_top_words(totals);
        //int expected_words = words_to_send * 2;
        // int expected_words = 29865;
        //int expected_words = 1148;
        cout << "Expected times \"the\" appears: " << expected_words << "\n";
        cout << (totals["the"] == expected_words ? ":)" : "FAIL!!") << endl;
        program_result = totals["the"] == expected_words ? 0 : 1;
    }
}
