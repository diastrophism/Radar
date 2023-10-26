/*
#include <iostream>
#include <boost/asio.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/serialization.hpp>

using boost::asio::ip::udp;

struct Position {
    double x;
    double y;
    template<typename Ar> void serialize(Ar& ar, unsigned) { ar&x&y; }
};

struct Message {
    int id;
    Position ATCurPos;
    Position MissileCurPos;
    bool Fire;
    template<typename Ar> void serialize(Ar& ar, unsigned) { ar & id & ATCurPos.x& ATCurPos.y& MissileCurPos.x& MissileCurPos.y& Fire; }
};

// 함수포인터 형태
typedef void(*OnEventCallback)(const Message);

class client
{
    udp::socket socket_;
    udp::endpoint sender_endpoint_;
    OnEventCallback user_callback;

    // 비동기 수신 루프를 구성함
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
                    on_receive(response);

                    do_receive();
                }
            });
    }

public:
    client(boost::asio::io_context& io_context, udp::endpoint& ep) : sender_endpoint_(ep), socket_(io_context, udp::endpoint(udp::v4(), 0)), user_callback(nullptr)
    {
        do_receive();
    }
    ~client() {
        socket_.close();
    }

    // 콜백함수 등록하는 함수
    void registerUserCallback(OnEventCallback newVal) {
        user_callback = newVal;
    }


    void on_receive(const Message& msg)
    {
        if (user_callback)
            user_callback(msg);
    }

    void send(const Message& req)
    {
        boost::asio::streambuf buf;
        {
            std::ostream os(&buf);
            boost::archive::binary_oarchive oa(os);  // 출력 스트림 지정
            oa << req;    // 직렬화할 객체를 oarchive 에 전달
        }
        socket_.send_to(buf.data(), sender_endpoint_);
    }
};

void MyCallback(const Message msg)
{
    // msg 수신 시, 원하는 동작을 기술
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //std::cout << "MyCallback : {" << msg.id << ", " << msg.x << "}" << std::endl;
}

int main(int argc, char* argv[])
{
    boost::asio::io_context io_context;
    udp::resolver resolver(io_context);
    udp::endpoint endpoint = *resolver.resolve(udp::v4(), "127.0.0.1", "8000").begin();

    client c(io_context, endpoint);
    c.registerUserCallback(MyCallback); //서버로 콜백 받으면 실행되는 함수

    // 수신 스레드 동작 시작
    std::thread t1([&]() {
        io_context.run();
        });


    // 내맘대로 원할 때 5번 전송하기
    while(1) {
        Position p1;
        p1.x = 1;
        p1.y = 2;

        Position p2;
        p2.x = 5;
        p2.y = 8;

        Message m;
        m.id = 0xC3;
        m.ATCurPos= p1;
        m.MissileCurPos = p2;
        m.Fire = 0;

        c.send(m);

        std::cout << "Send" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        break;
    }

    c.~client();
    t1.join();


    return 0;
}

*/