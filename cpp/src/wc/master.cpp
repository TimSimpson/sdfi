// Accepts a filename as input or reads from stdin.
#include <wc/count.h>
#include <wc/server.h>
#include <iostream>
#include <boost/lexical_cast.hpp>


using namespace wc;
using std::string;


int main(int argc, const char * * args) {
    if (argc < 3) {
        std::cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
                  << " host port" << std::endl;
        return 1;
    }
    string host(args[1]);
    string port(args[2]);
    //int port = boost::lexical_cast<int>(args[2]);

    try {
        wc::client client(host, port);
        client.send("HI!");
        string response = client.receive();
        std::cout << response << std::endl;
    } catch(const std::exception & e) {
        std::cerr << "An error occured: " << e.what() << std::endl;
    }
}
