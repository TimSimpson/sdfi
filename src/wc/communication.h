#ifndef FILE_GUARD_WC_COMMUNICATION_H
#define FILE_GUARD_WC_COMMUNICATION_H

#include <wc/count.h>
#include <wc/tcp.h>
#include <condition_variable>
#include <thread>
#include <boost/lexical_cast.hpp>
#include <boost/lockfree/spsc_queue.hpp>


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


// A functor that can be used with the read_using_buffer function,
// this takes data and then pipes it one of a list of queues which are
// ready to receive data. Because each queue needs a complete words though
// some extra work has to happen to avoid cutting words up in the middle.
// Queues is intended to be a vector like collection of pointers to Queue
// instances.
template<typename Queues, typename Queue>
class queue_distributor {
public:
    queue_distributor(Queues & queues)
    :   queues(queues) {
    }

    // Returns the start of words, or the end iterator if no valid start found.
    template<typename Iterator>
    Iterator find_valid_start(Iterator begin, const Iterator end) const {
        while (begin != end && !wc::is_word_character(*begin)) {
            begin ++;
        }
        return begin;
    }

    // Finds an available queue. This will go into an infinite loop if
    // the queues never get any available space!
    Queue * find_available_queue() const {
        // Spin until someone has space.
        Queue * q = nullptr;
        while(nullptr == q || q->write_available() < 1) {
            // find max write_available on all clients
            auto max_itr = std::max_element(
                queues.begin(), queues.end(),
                [](const auto * a, const auto * b) {
                    return a->write_available() < b->write_available();
                }
            );
            q = *max_itr;
        }
        return q;
    }

    // Avoids breaking off a word by returning a valid end iterator within
    // the given amount of consumable space.
    // So if the word is "big cat", the desired max size is 6, that would
    // leave us with "big ca"- i.e. the end iterator points to "a".
    // This function though would return "c", leaving only "big " to be
    // consumed. That's to prevent "c" from being consumed by a queue and
    // and ultimately getting interpretted as the word "c" by a worker.
    // However, this may mean the function returns the begin argument as there
    // is nothing that can be processed, possibly due to the desired_max_size
    // argument being too small.
    template<typename Iterator>
    Iterator find_valid_end(const Iterator begin, const Iterator end,
                            const bool eof,
                            const std::size_t desired_max_size) const
    {
        Iterator itr = end;
        const size_t size_of_data = end - begin;

        // If the desired size is less than end - begin, move itr to the left.
        if (desired_max_size < size_of_data) {
            itr -= (size_of_data - desired_max_size);
        }

        // itr, if it points to a word (or the end and we aren't at eof),
        // it isn't valid because it would be breaking up a word. So we need
        // to move it back.
        if (itr == end) {
            if (eof) {   // We can assume a break in the word here.
                return itr;
            }
        } else if (!wc::is_word_character(*itr)) {
            return itr;
        }


        while (itr != begin) {
            if (!wc::is_word_character(*(itr -1))) {
                break;
            }
            -- itr;
        }
        return itr;
    }


    template<typename Iterator>
    Iterator operator()(const Iterator begin, const Iterator end,
                        const bool eof)
    {
        const Iterator proc_start = find_valid_start(begin, end);
        if (end == proc_start) {
            return end; // No work characters found, so processing complete.
        }

        // Now we search for a valid queue and then, using how much we can
        // write to that queue, figure out what the valid processable end
        // iterator would be. If the processable end would be equal to begin
        // that is no good and we have to try again, because if we return
        // an iterator equal to end the caller will think we couldn't process
        // anything and throw an exception that the buffer was too small.
        Queue * q;
        Iterator proc_end = proc_start;
        while(proc_end <= proc_start) {
            q = find_available_queue();
            const auto available = q->write_available();
            proc_end = find_valid_end(proc_start, end, eof, available);
            if (proc_end == proc_start && available >= (end - begin)) {
                // We're not seeing *any* place we can use, even though
                // we have enough room. Something is screwed up - probably
                // the eof flag isn't set correctly - so throw an error.
                throw std::logic_error("Can't process data- eof not set?");
            }
        }

        // Finally send the data.
        q->process_text(proc_start, proc_end);

        return proc_end;
    };
private:
    Queues & queues;
};


// This takes care of loading up the workers with data by using a queue
// which is written to by the distributor thread and read from by one of the
// sender threads.
template<int buffer_size>
class queue_loader {
public:
    queue_loader()
    :   consumer(),
        producer(),
        mutex(),
        count(0),
        send(true)
    {}

    queue_loader(const queue_loader & rhs) = delete;
    queue_loader & operator=(const queue_loader & rhs) = delete;

    // Called by the distributor to note that the job is finished.
    void finish() {
        {
            std::unique_lock<std::mutex> lock(mutex);
            producer.wait(lock, [this]() { return count == 0; });
            send = false;
            count.store(1);
        }
        consumer.notify_one();
    }

    // Called by a different thread (only one other one btw) to wait for
    // data from the distributor and then send it.
    template<typename Function>
    void consume_and_send_data(Function on_send) {
        while(send) {
            producer.notify_one();
            {
                std::unique_lock<std::mutex> lock(mutex);
                consumer.wait(lock, [this]() {return count > 0; });
                if (!send) {
                    return;
                }
                on_send(local_buffer, count.load());
                count.store(0);
            }
        }
        producer.notify_one();
    }

    // Called by the distributor to determine how much space is left for
    // writing.
    auto write_available() const {
        return (sizeof(local_buffer) - 1) - count;
    }

    // Called by the distributor, actually pushes text into this queue.
    // Note that this will block until all the data is pushed. To avoid
    // blocking, call write_available first.
    template<typename Iterator>
    void process_text(Iterator begin, Iterator end) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            producer.wait(lock, [this]() { return count == 0; });
            std::copy(begin, end, local_buffer);
            // Add some extra garbage so the words are interpretted correctly
            // (this is why we subtract one in write_available).
            int new_count = end - begin;
            if (wc::is_word_character(*(end - 1))) {
                local_buffer[new_count] = '#';
                ++ new_count;
            }
            count.store(new_count);
        }
        consumer.notify_one();
    }
private:
    std::condition_variable consumer;
    std::condition_variable producer;
    std::mutex mutex;
    std::atomic_bool send;
    std::atomic<int> count;
    char local_buffer[buffer_size];
};





class word_char_divvy {
public:
    word_char_divvy(int worker_count) {
        if (worker_count > 36) {
            throw std::logic_error("Can't handle that many workers.");
        }
        int current_index = 0;
        for (int i = 0; i < 36; ++ i) {
            word_chars[i] = current_index;
            ++ current_index;
            if (current_index >= worker_count) {
                current_index = 0;
            }
        }
    }

    static inline int index(char c) {
        if (c >= 'A' && c <= 'Z') {
            return c - 'A';
        } else if (c >= 'a' && c <= 'z') {
            return c - 'a';
        } else if (c >= '0' && c <= '9') {
            return c - '0';
        }
        throw std::logic_error("Bad argument.");
    }

    int find_worker(char c) const {
        return word_chars[index(c)];
    }

private:
    int worker_count;
    int word_chars[36];
};


// This version of the distributor looks for words and sends them to the
// various queues based on the first letter.
// This takes longer, because it has to scan every letter and possibly
// may wait on queues that end up being over-burdened based on the div'ier.
template<typename CharToWorkerIndexer, typename Queues, typename Queue>
class letter_based_distributor {
public:
    letter_based_distributor(CharToWorkerIndexer indexer, Queues & queues)
    :   queues(queues),
        indexer(indexer)
    {
    }

    template<typename Iterator>
    Iterator operator()(const Iterator begin, const Iterator end,
                        const bool eof)
    {
        auto receive_word = [this](auto begin, auto end) {
            if (end <= begin) {
                return;
            }
            int index = indexer.find_worker(*begin);
            queues[index]->process_text(begin, end);
        };
        return wc::read_blob(begin, end, eof, receive_word);
    };
private:
    Queues & queues;
    CharToWorkerIndexer indexer;
};


}  // end namespace wc


#endif
