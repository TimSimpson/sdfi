// Starts a server on the given port and waits for an action.

#include <wc/count.h>
#include <wc/tcp.h>
#include <iostream>
#include <boost/lexical_cast.hpp>


using namespace wc;
using std::string;



int main(int argc, const char * * args) {
    if (argc < 2) {
        std::cerr << "Usage:" << ((argc > 0) ? args[0] : "prog")
                  << " port" << std::endl;
        return 1;
    }
    int port = boost::lexical_cast<int>(args[1]);

    try {
        server server(port);
        string input = server.read();
        std::cout << input << std::endl;
        server.write("Response.");
    } catch(const std::exception & e) {
        std::cerr << "An error occured: " << e.what() << std::endl;
    }
}
