#ifndef FILE_GUARD_WC_TCP_H
#define FILE_GUARD_WC_TCP_H

#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>


namespace wc {

static constexpr int header_size = 8;


template<int buffer_size,
         typename Function1,
         typename Function2,
         typename Function3
>
class async_receiver
: public std::enable_shared_from_this<
    async_receiver<buffer_size, Function1, Function2, Function3>
  >
{
public:
    async_receiver(boost::asio::ip::tcp::socket socket,
                   Function1 on_read,
                   Function2 on_finish,
                   Function3 on_error)
    :   socket(std::move(socket)),
        bytes(),
        write_start(bytes.data()),
        header_in_buffer(true),
        expected_bytes(),
        expected_bytes_read(false),
        total_bytes_read(0),
        on_read(on_read),
        on_finish(on_finish),
        on_error(on_error)
    {
    }

    void receive_async() {
        // This object would won't be held on the stack for long, which is
        // why it makes sense to have it create a shared pointer which is
        // held onto by the lambda. Otherwise ASIO would try to call back
        // to this object to find it deleted.
        auto self = this->shared_from_this();
        auto skip = write_start - bytes.data();
        auto write_length = std::min(known_bytes_left(), bytes.size() - skip);
        boost::asio::async_read(
            socket,
            boost::asio::buffer(write_start, write_length),
            [this, self] (const boost::system::error_code & ec,
                          std::size_t length) {
                this->_receive(ec, length);
            }
        );
    }

private:

    std::size_t known_bytes_left() const {
        if (!expected_bytes_read) {
            return header_size - total_bytes_read;
        } else {
            return expected_bytes - (total_bytes_read - header_size);
        }
    }

    // Reads more data and takes appropriate action.
    void _receive(const boost::system::error_code & ec, std::size_t length) {
        if (ec) {
            on_error(boost::system::system_error(ec).what());
        } else if (length < 1) {
            on_error("Read less than one byte.");
        } else {
            total_bytes_read += length;
            if (!expected_bytes_read) {
                if (total_bytes_read > 7) {
                    // Read the header.
                    expected_bytes = std::atoi(bytes.data());
                    expected_bytes_read = true;
                } else {
                    write_start += total_bytes_read;
                    receive_async();   // Re-do
                    return;
                }
            }

            bool eof = known_bytes_left() == 0;
            auto end = write_start + length;

            // Send data back to the on_read method, but be sure to skip
            // the header if its still in the buffer.
            auto on_read_start = bytes.data();
            if (header_in_buffer) {
                on_read_start += 8;
                header_in_buffer = false;
            }
            auto last_unprocessed_pos = on_read(on_read_start, end, eof);

            // Finish up, or get ready for the next read.
            // If there is unprocessed data at the end of the buffer, copy it
            // to the start and make sure we write to the correct position
            // on the next read.
            if (eof) {
                on_finish();  // end iteration
            } else {
                //TODO: This is very similar to read_using_buffer- maybe it
                //      could be consolidated somehow.
                if (last_unprocessed_pos == end) {
                    write_start = bytes.data();
                } else {
                    if (last_unprocessed_pos == bytes.data()) {
                        throw std::length_error("Buffer is too small to "
                            "accomodate continous data of this size.");
                    }
                    std::copy(last_unprocessed_pos, end, bytes.data());
                    write_start = bytes.data() + (end - last_unprocessed_pos);
                }
                receive_async();
            }
        }
    }

private:
    boost::asio::ip::tcp::socket socket;
    std::array<char, buffer_size> bytes;
    char * write_start;
    bool header_in_buffer;
    //TODO: Wanted to use boost::optional, but it causes an internal compiler
    //      error on GCC. -.-
    std::size_t expected_bytes;
    bool expected_bytes_read;
    std::size_t total_bytes_read;
    Function1 on_read;
    Function2 on_finish;
    Function3 on_error;
};


// Reads a line synchronously.
std::string socket_read_line(boost::asio::ip::tcp::socket & socket) {
    using boost::asio::buffer;
    using boost::asio::read;

    // Get header length
    char header_bytes[header_size];
    read(socket, buffer(header_bytes, header_size));
    const size_t message_length = std::atoi(header_bytes);

    // Read body
    std::vector<char> body_bytes;
    body_bytes.resize(message_length);
    read(socket, buffer(body_bytes.data(), body_bytes.size()));

    return std::string(body_bytes.data(),
                       body_bytes.data() + body_bytes.size());
}

// Writes a line synchronously.
void socket_write_line(boost::asio::ip::tcp::socket & socket,
                       const std::string & message)
{
    using boost::asio::buffer;
    using boost::asio::write;

    // Write header.
    char header_bytes[header_size];
    std::sprintf(header_bytes, "%8d", static_cast<int>(message.length()));
    write(socket, buffer(header_bytes, sizeof(header_bytes)));

    // Write body.
    write(socket, buffer(message.c_str(), message.size()));
}

// Client. Can synchronously send and receive string messages, and also has
// an asynchronous receive function.
class client {
public:
    client(boost::asio::io_service & ioservice,
           const std::string host,
           const std::string port)
    :   socket(ioservice),
        awaiting_async_response(false)
    {
        using boost::asio::connect;
        using boost::asio::ip::tcp;

        tcp::resolver resolver(ioservice);
        tcp::resolver::query query(host.c_str(), port.c_str());
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;

        boost::system::error_code error = boost::asio::error::host_not_found;
        while(error && endpoint_iterator != end) {
            socket.close();
            socket.connect(*endpoint_iterator, error);
            ++ endpoint_iterator;
        }
        if (error) {
            throw std::runtime_error("Couldn't connect.");
        }
    }

    client(const client & other) = delete;
    client & operator=(const client & other) = delete;

    std::string receive() {
        ensure_sync_mode();
        return socket_read_line(socket);
    }

    void send(const std::string & message) {
        ensure_sync_mode();
        socket_write_line(socket, message);
    }

    // Asynchronously receives a response. After this is called, the client
    // is in async mode and can't be called again.
    template<int buffer_size, typename F1, typename F2, typename F3>
    void async_receive(F1 on_read, F2 on_finished, F3 on_error)
    {
        awaiting_async_response = true;
        std::make_shared<async_receiver<buffer_size, F1, F2, F3>>(
            std::move(socket),
            on_read,
            on_finished,
            on_error)->receive_async();
    }

private:
    boost::asio::ip::tcp::socket socket;
    bool awaiting_async_response;

    void ensure_sync_mode() {
        // The async receive needs to own the socket after being called in
        // case this class is destroyed, plus mixing the synchronous and
        // async code could lead to problems anyway.
        if (awaiting_async_response) {
            throw std::runtime_error("Already waiting for async response.");
        }
    }
};


// Server. Can synchronously accept and respond to string messages.
class server {
public:
    server(int port)
    :   ioservice(),
        endpoint(boost::asio::ip::tcp::v4(), port),
        acceptor(this->ioservice, this->endpoint),
        has_started(false),
        socket(this->ioservice)
    {
    }

    server(const server & other) = delete;
    server & operator=(const server & other) = delete;

    void send(const std::string & data) {
        ensure_started();
        socket_write_line(socket, data);
    }

    std::string receive() {
        ensure_started();
        return socket_read_line(socket);
    }

private:
    boost::asio::io_service ioservice;
    const boost::asio::ip::tcp::endpoint endpoint;
    boost::asio::ip::tcp::acceptor acceptor;
    bool has_started;
    boost::asio::ip::tcp::socket socket;

    void ensure_started() {
        if (!has_started) {
            //acceptor.listen(socket);
            acceptor.accept(socket);
            has_started = true;
        }
    }
};


}  // end namespace wc


#endif
