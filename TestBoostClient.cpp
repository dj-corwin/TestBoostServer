#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

const int max_length = 1024;

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: TestBoostClient <host> <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;
        boost::system::error_code error;

        tcp::socket sock(io_context);
        tcp::resolver resolver(io_context);
        boost::asio::connect(sock, resolver.resolve(argv[1], argv[2]));

        for (;;) {
            std::cout << "Command: ";
            char request[max_length];
            std::cin.getline(request, max_length);
            size_t request_length = std::strlen(request);
            boost::asio::write(sock, boost::asio::buffer(request, request_length));

            char reply[max_length];
            std::cout << "Status: ";
            size_t length = sock.read_some(boost::asio::buffer(reply), error);
            if (error == boost::asio::error::eof)
                break; // Connection closed cleanly by peer.
            else if (error)
                throw boost::system::system_error(error); // Some other error.
            std::cout.write(reply, length);
            std::cout << "\n";
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
