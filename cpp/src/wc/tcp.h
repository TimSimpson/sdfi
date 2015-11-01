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
        start(bytes.data()),
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
        auto skip = start - bytes.data();
        boost::asio::async_read(
            socket,
            boost::asio::buffer(start, (bytes.size() - skip)),
            [this, self] (const boost::system::error_code & ec,
                          std::size_t length) {
                _receive(ec, length);
            }
        );
    }

private:

    // Reads more data and takes appropriate action.
    void _receive(const boost::system::error_code & ec, std::size_t length) {
        if (ec) {
            on_error(boost::system::system_error(ec).what());
        } else  if (length < 1) {
            on_error("Read less than one byte.");
        } else {
            auto end = start + length;
            bool eof = '!' == *(end - 1);

            if (eof) {
                end --;
            }
            auto last_unprocessed_pos = on_read(start, end, eof);
            if (eof) {
                on_finish();  // end iteration
            } else {
                //TODO: This is very similar to read_using_buffer- maybe it
                //      could be consolidated somehow.
                if (last_unprocessed_pos == end) {
                    start = bytes.data();
                } else {
                    if (last_unprocessed_pos == start) {
                        throw std::length_error("Buffer is too small to "
                            "accomodate continous data of this size.");
                    }
                    std::copy(last_unprocessed_pos, end, bytes.data());
                    start = bytes.data() + (end - last_unprocessed_pos);
                }
                receive_async();
            }
        }
    }

private:
    boost::asio::ip::tcp::socket socket;
    std::array<char, buffer_size> bytes;
    char * start;
    Function1 on_read;
    Function2 on_finish;
    Function3 on_error;
};



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

    template<int buffer_size, typename F1, typename F2, typename F3>
    std::string async_receive(
        F1 on_read, F2 on_finished, F3 on_error)
    {
        std::make_shared<async_receiver<buffer_size, F1, F2, F3>>(
            std::move(socket),
            on_read,
            on_finished,
            on_error)->receive_async();
        ioservice.run();
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

    void close() {
        if (has_started) {
            socket.close();
            has_started = false;
        }
    }

    void write(const std::string & data) {
        using boost::asio::buffer;
        using boost::asio::write;

        ensure_started();

        // Write body.
        write(socket, buffer(data.c_str(), data.size()));

        // Write EOF byte.
        char eof[1];
        eof[0] = '!';
        write(socket, buffer(eof, 1));
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
            boost::asio::socket_base::linger option(true, 30);
            socket.set_option(option);
            has_started = true;
        }
    }
};


}  // end namespace wc


#endif
