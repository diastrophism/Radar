#include <iostream>
#include <boost/asio.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <mutex>
#include <map>

using boost::asio::ip::udp;
using namespace std;

struct NetworkMessage {
    int id;
    unsigned char payload[256];
    template<typename Ar> void serialize(Ar& ar, unsigned) { ar& id & payload; }
};

struct Position {
    double x;
    double y;
    template<typename Ar> void serialize(Ar& ar, unsigned) { ar& x& y; }
};

struct Message {
    int id;
    int data1;
    Position data2;
    Position data3;
    bool data4;
    double data5;
    Position data6;
    Position data7;
    double data8;
    double data9;

    template<typename Ar> void serialize(Ar& ar, unsigned) { ar& id& data1& data2& data3& data4& data5& data6& data7& data8& data9; }
};

// �Լ������� ����
typedef void(*OnEventCallback)(const Message);

Message msg;
mutex mu;

class server
{
    udp::socket socket_;
    udp::endpoint sender_endpoint_;
    enum { max_length = 1024 };
    char data_[max_length];
    Message m;
    //std::string data_;
    std::map<int, boost::asio::ip::udp::endpoint> client_endpoints;
    boost::asio::ip::udp::endpoint endpoint_OS = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("172.16.12.114"), 50001);
public:
    server(boost::asio::io_context& io_context, short port) : socket_(io_context, udp::endpoint(udp::v4(), port))
    {
       do_receive();
    }

    void do_receive()
    {
        static boost::asio::streambuf b;
        boost::asio::streambuf::mutable_buffers_type buf = b.prepare(2048);
        socket_.async_receive_from(
            buf, sender_endpoint_,
            [&](boost::system::error_code ec, std::size_t bytes_recvd)
            {
                if (!ec && bytes_recvd > 0)
                {
                    Message response;
                    mu.lock();
                    b.commit(bytes_recvd);
                    {
                        std::istream is(&b);
                        boost::archive::binary_iarchive ia(is);
                        ia >> response;
                    }

                    cout << response.id << endl;
                    //�޽����� ID�� �����������̸� ����
                    if (response.id == 2) {
                        cout << "�������� ������ ����" << endl;
                        send(response);
                    }
                    //�޽����� ID�� ����ź�̸� ����
                    else if (response.id == 5) {
                        cout << "����ź ������ ����" << endl;
                        send(response);
                    }
                    else if (response.id == 1) {
                        cout << "�ó����� ����" << endl;
                        msg.id = response.id;
                        msg.data1 = response.data1;
                        msg.data2.x = response.data2.x;
                        cout << msg.id << endl;
                        cout << msg.data1 << endl;
                        cout << msg.data2.x << endl;
                    }
                    mu.unlock();
                }
                do_receive();
            });
        }
    

    void do_send(size_t length)
    {
        socket_.async_send_to(
            boost::asio::buffer(data_, length), sender_endpoint_,
            [this](boost::system::error_code /*ec*/, std::size_t /*bytes_sent*/)
            {
                do_receive();
            });
    };

    void send(const Message& req)
    {
        boost::asio::streambuf buf;
        {
            std::ostream os(&buf);
            boost::archive::binary_oarchive oa(os);  // ��� ��Ʈ�� ����
            oa << req;    // ����ȭ�� ��ü�� oarchive �� ����
        }
        socket_.send_to(buf.data(), endpoint_OS);
    }
};

class client
{
    udp::socket socket_;
    udp::endpoint sender_endpoint_;
    OnEventCallback user_callback;

    // �񵿱� ���� ������ ������
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

    // �ݹ��Լ� ����ϴ� �Լ�
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
            boost::archive::binary_oarchive oa(os);  // ��� ��Ʈ�� ����
            oa << req;    // ����ȭ�� ��ü�� oarchive �� ����
        }
        socket_.send_to(buf.data(), sender_endpoint_);
    }
};



void MyCallback(const Message msg)
{
    // msg ���� ��, ���ϴ� ������ ���
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    

}


int main(int argc, char* argv[])
{
    //���̴� ����
    boost::asio::io_context server_io_context;
    server s(server_io_context, 50003); //���̴ٸ��Ǳ� ����
    cout << "���̴ټ��� ���� ����" << endl;
    thread t1([&]() {
        server_io_context.run();
        });

    //���̴� Ŭ���̾�Ʈ (��������� ������ �����ϱ� ����)
    boost::asio::io_context RAS_io_context;
    udp::resolver RASresolver(RAS_io_context);
    udp::endpoint RASendpoint = *RASresolver.resolve(udp::v4(), "172.16.12.114", "50001").begin();
    client RASClient(RAS_io_context, RASendpoint);
    RASClient.registerUserCallback(MyCallback);

    Position GMSPosition;
    Position ATSPosition;

    while(1) {

    }

    //std::this_thread::sleep_for(std::chrono::milliseconds(10));

    /*
    c1.~client();
    c2.~client();
    */

    while (1) {}

    return 0;
}

