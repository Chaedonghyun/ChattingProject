#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include<sstream>
#include <mysql/jdbc.h>

#define MAX_SIZE 1024
#define MAX_CLIENT 4

using std::cout;
using std::endl;
using std::string;
using std::cin;
using std::vector;

const string server = "tcp://127.0.0.1:3306";
const string username = "root";
const string password = "abc1234";

sql::mysql::MySQL_Driver* driver;
sql::Connection* con;
sql::Statement* stmt;
sql::PreparedStatement* pstmt;
sql::ResultSet* result;


struct SOCKET_INFO
{
    SOCKET sck;
    string user;
};
vector<SOCKET_INFO> sck_list;
SOCKET_INFO server_sock;
int client_count = 0;

void server_init();
void add_client();
void send_msg(const char* msg);
void recv_msg(int idx);
void del_client(int idx);
void send_msg_dm(const char* msg, string recv);

int main()
{
    WSADATA wsa;

    int code = WSAStartup(MAKEWORD(2, 2), &wsa);

    if (!code)
    {

        try {
            driver = sql::mysql::get_mysql_driver_instance();
            con = driver->connect(server, username, password);
        }
        catch (sql::SQLException& e) {
            cout << "Could not connect to server. Error message: " << e.what() << endl;
            exit(1);
        }

        con->setSchema("project");

        stmt = con->createStatement();
        stmt->execute("set names euckr");
        if (stmt) { delete stmt; stmt = nullptr; }


        server_init();
        std::thread th1[MAX_CLIENT];
        for (int i = 0; i < MAX_CLIENT; i++)
        {
            th1[i] = std::thread(add_client);
        }

        while (1)
        {
            string text, msg = "";
            std::getline(cin, text);
            const char* buf = text.c_str();
            msg = server_sock.user + ":" + buf;
            send_msg(msg.c_str());
        }

        for (int i = 0; i < MAX_CLIENT; i++)
        {
            th1[i].join();
        }

    }
    else
    {
        cout << "프로그램 종류. (Error Code:" << code << ")";
    }

    WSACleanup();

}

void server_init() {
    server_sock.sck = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    SOCKADDR_IN server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(server_sock.sck, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock.sck, SOMAXCONN);
    server_sock.user = "server";
    cout << "open" << endl;
}

void add_client() {
    SOCKADDR_IN addr = {};
    int addrsize = sizeof(addr);
    char buf[MAX_SIZE] = { };

    ZeroMemory(&addr, addrsize);
    SOCKET_INFO new_client = {};
    new_client.sck = accept(server_sock.sck, (sockaddr*)&addr, &addrsize);
    recv(new_client.sck, buf, MAX_SIZE, 0);
    new_client.user = string(buf);

    string msg = new_client.user + "님이 입장했습니다";
    cout << msg << endl;
    sck_list.push_back(new_client);

    std::thread th(recv_msg, client_count);
    client_count++;

    cout << "현재 인원수:" << client_count << "명" << endl;
    send_msg(msg.c_str());

    th.join();
}


void send_msg(const char* msg) {

    for (int i = 0; i < client_count; i++)
    {
        send(sck_list[i].sck, msg, MAX_SIZE, 0);
    }

}

void send_msg_dm(const char* msg, string recv) {

    for (int i = 0; i < client_count; i++)
    {
        if (sck_list[i].user == recv) {
            send(sck_list[i].sck, msg, MAX_SIZE, 0);
            break;
        }
    }

}


void recv_msg(int idx) {
    char buf[MAX_SIZE] = { };
    string msg = "";
    while (1)
    {
        ZeroMemory(&buf, MAX_SIZE);
        if (recv(sck_list[idx].sck, buf, MAX_SIZE, 0) > 0)
        {
            cout << sck_list[idx].user + ":" + buf << endl;
            string msg1 = buf;
            msg = sck_list[idx].user + ":" + buf;

            // string ss; 
            string to;
            if (msg1.substr(0, 2) == "->") {
                std::stringstream ss(msg);
                // 공백 기준 두번째 단어 to에 저장, 임시로 msg에 ':' 저장
                ss >> to >> to ;
                //cout << "to" << to << endl;
                // 나머지 문자열 msg에 저장
                //cout << "msg" << msg << endl;
                std::getline(ss, msg);

                if (buf != "") {
                    pstmt = con->prepareStatement("insert into direct_msg(send_id,recv_id,msg) values(?,?,?)");
                    pstmt->setString(1, sck_list[idx].user);
                    pstmt->setString(2, to);
                    pstmt->setString(3, msg);
                    pstmt->execute();
                }
                string dm="";
                dm = "(1:1 msg)" + to + ":" + msg;
                send_msg_dm(dm.c_str(), to);

            }
            else
            {
                if (buf != "") {
                    pstmt = con->prepareStatement("insert into chatting(id, chat) values(?,?)");
                    pstmt->setString(1, sck_list[idx].user);
                    pstmt->setString(2, buf);
                    pstmt->execute();
                }
                send_msg(msg.c_str());
            }


        }
        else
        {
            msg = sck_list[idx].user + "님이 퇴장했습니다";
            cout << msg << endl;
            send_msg(msg.c_str());
            del_client(idx);
            return;
        }

    }

}

void del_client(int idx) {
    closesocket(sck_list[idx].sck);
    client_count--;
}