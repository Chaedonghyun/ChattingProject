#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h> 
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <mysql/jdbc.h>

using std::cout;
using std::endl;
using std::string;

const string server = "tcp://127.0.0.1:3306";
const string username = "root";
const string password = "abc1234";


#define MAX_SIZE 1024

using std::cout;
using std::cin;
using std::endl;
using std::string;

sql::mysql::MySQL_Driver* driver;
sql::Connection* con;
sql::Statement* stmt;
sql::PreparedStatement* pstmt;
sql::ResultSet* result;


SOCKET client_sock;
string id_in;

int chat_recv() {
    char buf[MAX_SIZE] = { };
    string msg;

    while (1) {
        ZeroMemory(&buf, MAX_SIZE);
        if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
            msg = buf;
            std::stringstream ss(msg);
            string user;
            ss >> user;
            if (user != id_in) cout << buf << endl;
        }
        else {
            cout << "Server Off" << endl;
            return -1;
        }
    }
}

void SignUp() {
    string id, pw, name;
    string check_id, check_pw;
    while (1) {
        cout << "사용할 아이디를 입력하세요:";
        cin >> id;

        pstmt = con->prepareStatement("SELECT * FROM user where id=? ;");
        pstmt->setString(1, id);
        pstmt->execute();
        result = pstmt->executeQuery();

        while (result->next()) {
            check_id = result->getString(1).c_str();
        }
        if (check_id == id) {
            cout << "이미 사용중인 아이디 입니다." << endl;
        }
        else if (check_id != id) {
            cout << "사용할 비밀번호를 입력하세요: ";
            cin >> pw;
            cout << "본인의 이름을 입력하세요: ";
            cin >> name;
            break;
        }
    }
    pstmt = con->prepareStatement("insert into user(id, pw, user_name) values(?,?,?)");
    pstmt->setString(1, id);
    pstmt->setString(2, pw);
    pstmt->setString(3, name);
    pstmt->execute();

    cout << "회원가입 성공" << endl;
}

void Store(string check_id) {
    string StoreUser, StoreMsg;

    cout << "저장된 내용" << endl;
    con->setSchema("project");
    pstmt = con->prepareStatement("SELECT * FROM chatting;");
    result = pstmt->executeQuery();

    while (result->next()) {
        StoreUser = result->getString("id");
        StoreMsg = result->getString("chat");
        cout << StoreUser << " : " << StoreMsg << endl;   
    }

}

void Revise() {
    string id, pw;
    string check_id, check_pw;

    cout << "아이디를 입력하세요. : ";
    cin >> id;
    cout << "비밀번호를 입력하세요";
    cin >> pw;

    pstmt = con->prepareStatement("SELECT * FROM user where id = ? and pw =?;");
    pstmt->setString(1, id);
    pstmt->setString(2, pw);
    pstmt->execute();
    result = pstmt->executeQuery();

    while (result->next()) {
        check_id = result->getString(1).c_str();
        check_pw = result->getString(2).c_str();
    }

    if (check_id != id || check_pw!=pw) {
        cout << "아이디,비밀번호가 맞지 않습니다.\n";
    }

    else {
        cout << "변경할 비밀번호를 입력해 주세요. : ";
        cin >> pw;
        pstmt = con->prepareStatement("select id from user");
        result = pstmt->executeQuery();

        while (result->next()) {
            pstmt = con->prepareStatement("UPDATE user SET pw = ? WHERE id = ?");
            pstmt->setString(1, pw);
            pstmt->setString(2, id);
            pstmt->executeQuery();
        }
        cout << "변경 되었습니다\n";
    }

}

void Leave() {
    string id, pw, name;
    string check_id, check_pw, check_name;
    cout << "아이디를 입력해주세요: ";
    cin >> id;
    cout << "비밀번호를 입력해주세요: ";
    cin >> pw;
    cout << "이름을 입력해주세요: ";
    cin >> name;
    pstmt = con->prepareStatement("SELECT * FROM user where id = ? and pw = ? and user_name = ?");
    pstmt->setString(1, id);
    pstmt->setString(2, pw);
    pstmt->setString(3, name);
    pstmt->execute();
    result = pstmt->executeQuery();
    while (result->next()) {
        check_id = result->getString(1).c_str();
        check_pw = result->getString(2).c_str();
        check_name = result->getString(3).c_str();
    }
    if (check_id != id || check_pw != pw || check_name != name) {
        cout << "회원 정보가 일치하지 않습니다.\n";
    }
    else {
        pstmt = con->prepareStatement("DELETE FROM user WHERE id = ?");
        pstmt->setString(1, id);
        result = pstmt->executeQuery();
        pstmt = con->prepareStatement("DELETE FROM chatting WHERE id = ?");
        pstmt->setString(1, id);
        result = pstmt->executeQuery();
        cout << "탈퇴되었습니다.\n";
    }
}

int main()
{
    WSADATA wsa;
    int choice = 0;
    bool log = true;
    int code = WSAStartup(MAKEWORD(2, 2), &wsa);

    // sql 연결
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

    stmt = con->createStatement();
    delete stmt;
    
    //createtable();


    if (!code) {

        client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        SOCKADDR_IN client_addr = {};
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(8080);
        InetPton(AF_INET, TEXT("127.0.0.1"), &client_addr.sin_addr);
        client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        while (true) {

            cout << "1: 로그인하기 2: 회원가입하기 3: 비밀번호 수정 4:회원탈퇴" << endl;
            cin >> choice;

            // 로그인

            if (choice == 1) {
                string pw;
                string check_id, check_pw;

                cout << "ID:";
                cin >> id_in;
                cout << "PW:";
                cin >> pw;

                pstmt = con->prepareStatement("SELECT * FROM user where id=? and pw=?;");
                pstmt->setString(1, id_in);
                pstmt->setString(2, pw);
                pstmt->execute();
                result = pstmt->executeQuery();


                while (result->next()) {
                    check_id = result->getString(1).c_str();
                    check_pw = result->getString(2).c_str();
                }
                if (check_id == id_in && check_pw == pw) {
                    cout << "로그인 되었습니다." << endl;
                    while (1) {
                        while (1) {
                            if (!connect(client_sock, (SOCKADDR*)&client_addr, sizeof(client_addr))) {
                                cout << "Server Connect" << endl;
                                send(client_sock, id_in.c_str(), id_in.length(), 0);
                                break;
                            }
                            cout << "Connecting..." << endl;
                        }

                        std::thread th2(chat_recv);
                        Store(check_id);
                        while (1) {
                            string text;
                            std::getline(cin, text);
                            const char* buffer = text.c_str(); // string형을 char* 타입으로 변환
                            send(client_sock, buffer, strlen(buffer), 0);

                            
                            if (text != "") {
                                pstmt = con->prepareStatement("insert into chatting(id, chat) values(?,?)");
                                pstmt->setString(1, id_in);
                                pstmt->setString(2, buffer);
                                pstmt->execute();
                            }
                       
                        }
                        th2.join();
                        closesocket(client_sock);
                    }
                }
                else {
                    cout << "로그인에 실패했습니다." << endl;
                    continue;
                }



            }
            if (choice == 2)
            {
                SignUp();
                continue;
            }

            if (choice == 3)
            {
                Revise();
                continue;
            }

            if (choice == 4) {
                int num;
                cout << "정말 탈퇴하시겠습니까? (예: 1 아니오: 2)\n";
                cin >> num;
                if (num == 1) {
                    Leave();
                }
                else {
                    choice = 0;
                }
            }
        }


        WSACleanup();

    }
    //
}