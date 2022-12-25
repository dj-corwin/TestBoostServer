#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>
#include <map>
#include <mutex>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

#define _GET_STATE "get-led-state"
#define _SET_STATE "set-led-state"
#define _GET_COLOR "get-led-color"
#define _SET_COLOR "set-led-color"
#define _GET_RATE  "get-led-rate"
#define _SET_RATE  "set-led-rate"

#define _REPLY_OK     "OK"
#define _REPLY_FAILED "FAILED"

const int max_length = 1024;

enum light_t {
    OFF   = 0x00,
    GREEN = 0x01,
    BLUE  = 0x02,
    RED   = 0x04
};

std::map<light_t, const char*> LIGTH{ {OFF, "off"}, {GREEN, "green"}, {BLUE, "blue"}, {RED, "red"} };
std::map<std::string, light_t> COLOR{ {"green", GREEN}, {"blue", BLUE}, {"red", RED} };

std::map<std::string, bool> STATE{ {"on", true}, {"off", false} };

class LED {
private:
    struct rate_t {
    public:
        rate_t(void) { _value = 0; }
        rate_t(size_t _rate) { _value = range(_rate); }

        operator const char* () const { char* res = (char*)malloc(sizeof(char) * 2); res[0] = nums[_value]; res[1] = 0;  return res; }

        rate_t& operator= (const rate_t& _rate) { _value = _rate._value; return *this; }
        rate_t& operator= (size_t _rate) { _value = range(_rate); return *this; }
    private:
        const size_t max_rate = 5;
        const char nums[11] = "0123456789";

        size_t range(size_t _rate) { return _rate > max_rate ? max_rate : _rate; }
        size_t _value;
    };

public:
    LED() { state = false; color = OFF; rate = 0; }

    const char* getState(void) const { return state ? "on" : "off"; }
    const char* getColor(void) const { return LIGTH[color]; }
    const char* getRate(void)  const { return rate; }

    bool setState(const char* newState) {
        bool found = STATE.count(newState);
        if (found) state = STATE[newState];
        return found;
    }
    bool setColor(const char* newColor) {
        bool found = COLOR.count(newColor);
        if (found) color = COLOR[newColor];
        return found;
    }
    bool setRate(const char* newRate) { rate = std::atoi(newRate); return true; }

private:
    bool state;
    light_t color;
    rate_t rate;
};

class controller {
public:
    void process(std::string& command, std::string& reply) {
        reply.assign(_REPLY_OK);
        std::string arg;
        size_t sp = command.find(" ", 1);
        if (sp) {
            arg.assign(command.substr(sp + 1, command.length() - sp));
            command.assign(command.substr(0, sp));
        }

        _lock.lock();
        if (command._Equal(_GET_STATE)) {
            reply.append(" ").append(led.getState());
        } else if (command._Equal(_SET_STATE)) {
            if (!led.setState(arg.c_str())) reply.assign(_REPLY_FAILED);
        } else if (command._Equal(_GET_COLOR)) {
            reply.append(" ").append(led.getColor());
        } else if (command._Equal(_SET_COLOR)) {
            if (!led.setColor(arg.c_str())) reply.assign(_REPLY_FAILED);
        } else if (command._Equal(_GET_RATE)) {
            reply.append(" ").append(led.getRate());
        } else if (command._Equal(_SET_RATE)) {
            if (!led.setRate(arg.c_str())) reply.assign(_REPLY_FAILED);
        } else {
            reply.assign(_REPLY_FAILED);
        }
        _lock.unlock();

        showState();
    }

private:
    LED led;
    std::mutex _lock;

    void showState(void) const {
        std::cout << led.getState() << "[" << led.getColor() << "] " << led.getRate() << "\n";
    }
};

static controller* ctrl;

void session(tcp::socket sock) {
    try {
        std::string command;
        std::string status;
        char data[max_length];
        boost::system::error_code error;

        for (;;) {
            size_t length = sock.read_some(boost::asio::buffer(data), error);
            if (error == boost::asio::error::eof)
                break; // Connection closed cleanly by peer.
            else if (error)
                throw boost::system::system_error(error); // Some other error.

             if (length > 0) {
                command.assign(data, length);
                ctrl->process(command, status);
                boost::asio::write(sock, boost::asio::buffer(status.c_str(), status.length()));
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

void server(boost::asio::io_context& io_context, unsigned short port) {
    tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), port));
    for (;;) {
        std::thread(session, a.accept()).detach();
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: TestBoostServer <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;

        ctrl = new controller();
        server(io_context, std::atoi(argv[1]));

        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    delete ctrl;

    return 0;
}
