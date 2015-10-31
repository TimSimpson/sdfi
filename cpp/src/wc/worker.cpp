// Accepts a filename as input or reads from stdin.
#include <wc/count.h>
#include <wc/server.h>
#include <iostream>
#include <boost/lexical_cast.hpp>


using namespace wc;
using std::string;



int main(int argc, const char * * args) {
    if (argc < 2) {
        std::cerr << "Usage:" << ((argc > 0) ? args[0] : "port")
                  << " port" << std::endl;
        return 1;
    }
    int port = boost::lexical_cast<int>(args[1]);

    Server server(port);
    server.run();
}
