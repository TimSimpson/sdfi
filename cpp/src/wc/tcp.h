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
        char header_bytes[4];
        read(socket, buffer(header_bytes, sizeof(header_bytes)));
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
        char header_bytes[4];
        std::sprintf(header_bytes, "%4d", static_cast<int>(message.length()));
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


    void write(const std::string & data) {
        using boost::asio::buffer;
        using boost::asio::write;

        if (!has_started) {
            //acceptor.listen(socket);
            acceptor.accept(socket);
            has_started = true;
        }

        // Write header- make sure the length is 4.
        char header_bytes[4];
        std::sprintf(header_bytes, "%4d", static_cast<int>(data.length()));
        write(socket, buffer(header_bytes, sizeof(header_bytes)));

        // Write body.
        write(socket, buffer(data.c_str(), data.size()));
    }

    std::string read() {
        using boost::asio::buffer;
        using boost::asio::read;

        if (!has_started) {
            //acceptor.listen(socket);
            acceptor.accept(socket);
            has_started = true;
        }

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
};


}  // end namespace wc


#endif
