#include <iostream>
#include <boost/asio.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
using boost::asio::ip::udp;

using namespace std;


struct Position {
    double x;
    double y;
    template<typename Ar> void serialize(Ar& ar, unsigned) { ar& x& y; }
};

struct Message {
    int id;
    Position ATCurPos;
    Position MissileCurPos;
    bool Fire;
    template<typename Ar> void serialize(Ar& ar, unsigned) { ar& id& ATCurPos.x& ATCurPos.y& MissileCurPos.x& MissileCurPos.y& Fire; }
};

class server
{
    udp::socket socket_;
    udp::endpoint sender_endpoint_;
    enum { max_length = 1024 };
    char data_[max_length];
    Message m;
    //std::string data_;

public:
    server(boost::asio::io_context& io_context, short port) : socket_(io_context, udp::endpoint(udp::v4(), port))
    {
        do_receive();
    }

    void do_receive()
    {
        static boost::asio::streambuf b;
        boost::asio::streambuf::mutable_buffers_type buf = b.prepare(512);
        socket_.async_receive_from(
            buf, sender_endpoint_,
            [&](boost::system::error_code ec, std::size_t bytes_recvd)
            {
                if (!ec && bytes_recvd > 0)
                {
                    b.commit(bytes_recvd);
                    Message response; // uninitialized
                    {
                        std::istream is(&b);
                        boost::archive::binary_iarchive ia(is);
                        ia >> response;
                    }

                    cout << "response.id = " << response.id << endl;
                    cout << "response.ATCurPos.x = " << response.ATCurPos.x << endl;
                    cout << "response.ATCurPos.y = " << response.ATCurPos.y << endl;
                    cout << "response.MissileCurPos.x = " << response.MissileCurPos.x << endl;
                    cout << "response.MissileCurPos.y = " << response.MissileCurPos.y << endl;
                    cout << "response.Fire = " << response.Fire << endl;
                }
            });

  
    }

    void do_send(std::size_t length)
    {
        socket_.async_send_to(
            boost::asio::buffer(data_, length), sender_endpoint_,
            [this](boost::system::error_code /*ec*/, std::size_t /*bytes_sent*/)
            {
                do_receive();
            });
    };
};

int main() {
    boost::asio::io_context io_context;
    server s(io_context, 8000); //8000 = port
    io_context.run();

    return 0;
}

