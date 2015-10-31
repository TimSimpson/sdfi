#ifndef FILE_GUARD_WC_SERVER_H
#define FILE_GUARD_WC_SERVER_H

#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>


namespace wc {


class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket)
    :   header_bytes(),
        body_bytes(),
        socket(std::move(socket)) {
    }

    void start() {
        read_header();
    }

private:
    using error_code = boost::system::error_code;
    using tcp = boost::asio::ip::tcp;
    using size_t = std::size_t;
    using string = std::string;

    std::array<char, 4> header_bytes;
    std::vector<char> body_bytes;
    //boost::asio::io_service ioservice;
    //const boost::asio::ip::tcp::endpoint endpoint;
    //boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket socket;

    void on_error(const std::string & msg) {
        std::cerr << "Session error:" << msg << std::endl;
    }

    void on_input(const std::string & input) {
        std::cout << "INPUT:" << input << std::endl;
    }

    void write_header() {

    }

    void read_header() {
        auto self = shared_from_this();
        boost::asio::async_read(
            socket,
            boost::asio::buffer(header_bytes.data(), header_bytes.length()),
            [this, self](const error_code & ec, const size_t length) {
                // TODO: Should retry to read this... but we're talking about 4
                //       bytes. For now just treat it as an error.
                if (!ec && header_bytes.length() == length) {
                    size_t message_length = std::atoi(header_bytes.data());
                    body_bytes.resize(message_length);
                    read_body(body_bytes.data(),
                              body_bytes.data() + message_length);
                } else {
                    on_error("Error reading header.");
                }
            }
        );
    }

    template<typename Iterator>
    void read_body(Iterator start, const Iterator end) {
        auto self = shared_from_this();
        boost::asio::async_read(
            buffer(start, (end - start)),
            [this, self, message_length, current_message] (
                const error_code & ec, const size_t bytes_read
            ) {
                if (!ec) {
                    const auto size_left = (end - start);
                    if (bytes_read < size_left) {
                        read_body(start + length, end);
                    } else if (bytes_read == size_left) {
                        on_input(std::string(body_bytes.begin(),
                                             body_bytes.end()));
                        write_header();
                    } else {  // bytes_read > size_left
                        on_error("Body was larger than indicated by header.");
                    }
                } else {
                    on_error("Error reading body.");
                }
            }
        );
    }

};


class Server {
public:
    Server(int port)
    :   ioservice(),
        endpoint(boost::asio::ip::tcp::v4(), port),
        acceptor(this->ioservice, this->endpoint),
        socket(this->ioservice)
    {}
    Server(const Server & other) = delete;
    Server & operator=(const Server & other) = delete;

    void on_accept() {

    }

private:
    using error_code = boost::system::error_code;
    using tcp = boost::asio::ip::tcp;
    using size_t = std::size_t;
    using string = std::string;

    std::array<char, 4> header_bytes;
    std::vector<char> body_bytes;
    boost::asio::io_service ioservice;
    const boost::asio::ip::tcp::endpoint endpoint;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket socket;

    void on_error(const std::string & msg) {
        std::cerr << "Session error:" << msg << std::endl;
    }

    void on_input(const std::string & input) {
        std::cout << "INPUT:" << input << std::endl;
    }

    void read_header() {
        auto self = shared_from_this();
        boost::asio::async_read(
            socket,
            boost::asio::buffer(header_bytes.data(), 4),
            [this, self](const error_code & ec, size_t) {
                if (!ec) {
                    size_t message_length = std::atoi(header_bytes.data());
                    body_bytes.resize(message_length);
                    read_body(body_bytes.data(),
                              body_bytes.data() + message_length);
                } else {
                    on_error("Error reading header.");
                }
            }
        );
    }

    template<typename Iterator>
    void read_body(Iterator start, const Iterator end) {
        auto self = shared_from_this();
        boost::asio::async_read(
            buffer(start, (end - start)),
            [this, self, message_length, current_message] (
                const error_code & ec, const size_t bytes_read
            ) {
                if (!ec) {
                    const auto size_left = (end - start);
                    if (bytes_read < size_left) {
                        read_body(start + length, end);
                    } else if (bytes_read == size_left) {
                        on_input(std::string(body_bytes.begin(),
                                             body_bytes.end()));
                    } else {  // bytes_read > size_left
                        on_error("Body was larger than indicated by header.");
                    }
                } else {
                    on_error("Error reading body.");
                }
            }
        );
    }


        if (!ec) {

            std::cout.write(bytes.data(), length);
        }
    }

    void run() {
        using boost::asio::buffer;
        using boost::asio::ip::tcp;
        using boost::system::error_code;
        using boost::asio::boost::asio::async_write;

        acceptor.listen();

        auto read_handler = [this](const error_code & ec, length) {
            if (!ec) {
                std::cout.write(bytes.data(), length);
            }
        };
        auto accept_handler = [this](const error_code & ec) {
            if (!ec) {
                boost::asio::async_read_some(buffer(bytes), )
                this->on_accept();
                std::string ok("ok");
                boost::asio::async_write(socket, buffer(ok),
                    [this](error_code ec,
                           std::size_t bytes_sent) {
                        if (!ec) {
                            socket.shutdown(tcp::socket::shutdown_send);
                        } else {
                            std::cerr << "Error shutting down socket."
                                      << std::endl;
                        }
                    });
            }
        };

        acceptor.async_accept(socket, [this](error_code ec) {
            if (!ec) {
                boost::asio::async_read_some(buffer(bytes), )
                this->on_accept();
                std::string ok("ok");
                boost::asio::async_write(socket, buffer(ok),
                    [this](error_code ec,
                           std::size_t bytes_sent) {
                        if (!ec) {
                            socket.shutdown(tcp::socket::shutdown_send);
                        } else {
                            std::cerr << "Error shutting down socket."
                                      << std::endl;
                        }
                    });
            }
            std::cerr << "Error accepting connection." << std::endl;
        });
        ioservice.run();
    }


};


}  // end namespace wc


#endif
