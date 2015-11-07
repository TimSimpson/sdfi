#ifndef FILE_GUARD_WC_COMMUNICATION_H
#define FILE_GUARD_WC_COMMUNICATION_H

#include <wc/count.h>
#include <wc/tcp.h>
#include <boost/lexical_cast.hpp>


namespace wc {

// Collects the word counts from a worker.
struct worker_results_collector {
    worker_results_collector()
    :   error(false),
        finished(false),
        is_odd(false),
        last_word(),
        results()
    {
    }

    // The workers send out a series of words and their counts over and over.
    // This function captures that data, recording it into a map.
    template<typename Iterator>
    void operator()(Iterator start, Iterator end) {
        is_odd = !is_odd;
        if (is_odd) {
            last_word = std::string(start, end);
        } else {
            std::string count_str(start, end);
            size_t count = boost::lexical_cast<size_t>(count_str);
            results[last_word] = count;
        }
    }

    bool error_occured() const {
        return error;
    }

    void finish() {
        finished = true;
    }

    word_map & get_results() {
        return results;
    }

    bool is_finished() const {
        return finished;
    }

    void set_error() {
        error = true;
    }
private:
    bool error;
    bool finished;
    bool is_odd;
    std::string last_word;
    word_map results;
};


// Asynchronously collects results into a worker_results_collector.
// Do not deallocate the worker_results_collector until io_server.run()
// finished.
void async_collect_results(client & client,
                           worker_results_collector & collector) {
    client.async_receive<1024 * 10>(
        [&collector](
            auto begin, auto end, bool eof
        ) {
            return wc::read_blob(begin, end, eof, collector);
        },
        [&collector]() {
            collector.finish();
        },
        [&collector](const std::string & msg) {
            collector.set_error();
            std::cerr << msg << std::endl;
        }
    );
}


}  // end namespace wc


#endif
