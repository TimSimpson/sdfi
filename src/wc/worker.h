// Helper functionality for the worker binaries.
#ifndef FILE_GUARD_WC_WORKER_H
#define FILE_GUARD_WC_WORKER_H

#include <wc/cmds.h>
#include <wc/communication.h>


namespace wc {


// Used by a worker, this allows a server to be used from the read_using_buffer
// function. The server expects a single byte == '.' to mean "read one more
// line" while a byte of "!" means EOF.
// Note that this takes a reference to a server.
struct server_reader {
    char last_byte;
    std::string last_string;
    wc::server * server;

    server_reader(wc::server * server)
    :   last_byte(),
        last_string(),
        server(server)
    {
        if (nullptr == server) {
            throw std::logic_error("Null not allowed for server_reader.");
        }
        last_byte = server->receive_byte();
    }

    bool has_more() const {
        return last_string.size() != 0 || last_byte == '.';
    }

    template<typename Iterator, typename SizeType>
    SizeType read(Iterator output_buffer, SizeType count) {
        if (last_string.size() == 0) {
            last_string = server->receive();
            last_byte = server->receive_byte();
        }
        if (count >= last_string.size()) {
            std::copy(last_string.begin(), last_string.end(), output_buffer);
            SizeType result = last_string.size();
            last_string.erase();
            return result;
        } else {
            std::copy(last_string.begin(), last_string.begin() + count,
                      output_buffer);
            last_string.erase(0, count);
            return count;
        }
    }
};


// Opens a connection on the given port and runs the worker routine.
// A function is called when the worker has accepted all input from the master
// which lets the way the response is created be changed.
void run_worker(int port, auto fill_response) {
    wc::server server(port);

    std::vector<std::string> files;
    std::cout << "Reading in text...\n";

    wc::word_counter counter;

    wc::server_reader reader(&server);  // Waits for first byte.

    wc::stop_watch watch;

    auto processor = [&counter](auto begin, auto end, auto eof) {
        return wc::read_blob(begin, end, eof, counter);
    };
    wc::read_using_buffer<wc::buffer_size>(reader, processor);

    std::cout << "Finished.\n";

    // Create the giant message in memory.
    std::stringstream stream;

    fill_response(counter.words(), stream);

    // Change it into a giant string and send it.
    auto s = stream.str();

    std::cout << "Responding... (size == " << s.length() << ")\n";
    server.send(s);
    watch.print_time();
}

}

#endif
