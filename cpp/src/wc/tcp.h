#ifndef FILE_GUARD_WC_TCP_H
#define FILE_GUARD_WC_TCP_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>


namespace wc {

static constexpr int header_size = 8;


class client {
public:
    client(const std::string host, const std::string port)
    :   ioservice(),
        socket(this->ioservice)
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


    std::string receive() {
        //TODO: Eliminate copy-paste, this is nearly the same as read
        using boost::asio::buffer;
        using boost::asio::read;

        // Get header length
        char header_bytes[header_size];
        read(socket, buffer(header_bytes, sizeof(header_bytes)));
        const size_t message_length = std::atoi(header_bytes);

        // Read body
        std::vector<char> body_bytes;
        body_bytes.resize(message_length);
        read(socket, buffer(body_bytes.data(), body_bytes.size()));

        return std::string(body_bytes.data(),
                           body_bytes.data() + body_bytes.size());
    }


    template<typename Buffer, typename Function>
    void _read(const error_code & ec, std::size_t length) {

    }

    // Example usage:
    // Buffer = std::array<char, 2 * 1024>
    template<typename Buffer, typename Function>
    std::string async_receive(Buffer & bytes, Function on_finished) {
        using boost::asio::async_read;
        using boost::asio::buffer;
        using boost::system::error_code;

        const int buffer_size = 2 * 1024;
        std::array<char, buffer_size> bytes;

        std::size_t bytes_read = 0;
        boost::optional<std::size_t> expected_bytes;

        read_more = [this, &header_bytes](
            const error_code & ec, std::size_t length
        ){
            if (!ec) {
                bytes_read += length;
                if (expected_bytes && bytes_read >= expected_bytes.get()) {
                    on_finished();
                }
                if (!expected_bytes && bytes_read > 7) {
                    expected_bytes = std::atoi(bytes.data());
                }
                async_read(socket, buffer(bytes.data(), bytes.size()), read_more);
            }
        };

        async_read(socket, buffer(header_bytes, sizeof(header_bytes)),

            );
        const size_t message_length = std::atoi(header_bytes);

        // Read body
        std::vector<char> body_bytes;
        body_bytes.resize(message_length);
        read(socket, buffer(body_bytes.data(), body_bytes.size()));

        return std::string(body_bytes.data(),
                           body_bytes.data() + body_bytes.size());
    }

    void send(const std::string & message) {
        //TODO: Eliminate copy-pasta

        using boost::asio::buffer;
        using boost::asio::write;

        // Write header- make sure the length is 4.
        char header_bytes[header_size];
        std::sprintf(header_bytes, "%8d", static_cast<int>(message.length()));
        write(socket, buffer(header_bytes, sizeof(header_bytes)));
        // Write body.
        write(socket, buffer(message.c_str(), message.size()));
    }

private:
    boost::asio::io_service ioservice;
    boost::asio::ip::tcp::socket socket;
};


template<int buffer_size, typename Function>
class async_receiver {
public:
    async_receiver(boost::asio::ip::tcp::socket socket,
                   Function on_finished,
                   Function on_read,
                   Function on_error)
    :   socket(std::move(socket)),
        bytes(),
        expected_bytes(),
        total_bytes_read(0),
        on_read(on_read),
        on_error(on_error)
    {
    }

private:

    // Returns expected bytes left to read. If that isn't known yet, returns
    // none.
    boost::optional<std::size_t> expected_bytes_left() const {
        if (expected_bytes) {
            return (expected_bytes.get() + header_size) - total_bytes_read;
        } else {
            return boost::none;
        }
    }

    // Reads more data and takes appropriate action.
    void _read(const error_code & ec, std::size_t length) {
        if (ec) {
            on_error();
        } else {
            total_bytes_read += length;
            bool eof = false;
            bool skip_header = false;
            if (!expected_bytes && total_bytes_read > 7) {
                expected_bytes = std::atoi(bytes.data());
                skip_header = true;
            }
            if (!expected_bytes) {
                // Still somehow didn't read enough. Read again but start
                // later in the buffer and don't read as far as normal.
                async_read(
                    socket,
                    buffer(bytes.data() + total_bytes_read,
                           bytes.size() - total_bytes_read),
                    _read);
            } else {
                // Since we're past the header, we can feed actual data
                // back to the caller using "on_read". But we have to
                // work around the fact that on the first read we don't
                // mention the header.
                eof = ((total_bytes_read - header_size)
                       >= expected_bytes.get());
                auto start = bytes.data();
                auto end = start + length;
                if (skip_header) {
                    start += header_size;
                }
                on_read(start, end, eof);

                if (!eof) {
                    // We don't care about the header so just read whatever.
                    auto read_length = std::min(
                        (expected_bytes.get() + header_size) - total_bytes_read,
                        bytes.size()
                    );
                    async_read(socket, buffer(bytes.data(), read_length), _read);
                }
            }
        }
    }

private:
    boost::asio::ip::tcp::socket socket;
    std::array<char, buffer_size> bytes;
    boost::optional<std::size_t> expected_bytes;
    std::size_t total_bytes_read;
    Function on_read;
    Function on_error;
};


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


    void write(const std::string & data) {
        using boost::asio::buffer;
        using boost::asio::write;

        ensure_started();

        // Write header- make sure the length is 4.
        char header_bytes[header_size];
        std::sprintf(header_bytes, "%8d", static_cast<int>(data.length()));
        write(socket, buffer(header_bytes, sizeof(header_bytes)));

        // Write body.
        write(socket, buffer(data.c_str(), data.size()));
    }

    std::string read() {
        using boost::asio::buffer;
        using boost::asio::read;

        ensure_started();

        // Get header length
        char header_bytes[4];
        read(socket, buffer(header_bytes, 4));
        const size_t message_length = std::atoi(header_bytes);

        // Read body
        std::vector<char> body_bytes;
        body_bytes.resize(message_length);
        read(socket, buffer(body_bytes.data(), body_bytes.size()));

        return std::string(body_bytes.data(),
                           body_bytes.data() + body_bytes.size());
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
