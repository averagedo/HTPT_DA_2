#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <error.h>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <time.h>
#include <fstream>
#include <errno.h>

using namespace std;

#define Len_buff 1000
#define TIME 7000000
#define NAME_FILE "_log.txt"
#define FOLDER "./log/"
#define CONF "./conf.txt"

mutex mtx_recv, mtx_pro, mtx_log, mtx_vstr;
condition_variable cv_recv, cv_pro;
vector<string> gb_v_str;
int int_yes = 0;
int round = 0;
int proposer;
bool check_propose = false;
bool flag_pro = false;
bool plusround = true;
bool write_log = true;
bool Byzantine = false;
bool silent = false;
int global_id;

struct info
{
  int id;
  string ip;
  int port;
};

vector<int> nodedown = {1, 3, 4, 2, 7, 6, 5, 3, 5, 7, 1, 2, 6, 4, 7, 1, 2, 5, 6, 3, 4, 2};

clock_t start = clock();

//=======================================
void send_mess(std::string ip, int port, std::string mess);

void recv_mess(int port);

//=======================================

void send_mess(std::string ip, int port, std::string mess)
{
  int sock;
  sockaddr_in serv_addr;
  char buffer[Len_buff] = {0};
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    std::cout << "Socket creation error\n";
    //exit(1);
  }

  int enable = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    cout << "setsockopt(SO_REUSEADDR) failed" << endl;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
    cout << "setsockopt(SO_REUSEPORT) failed" << endl;

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  // Convert IPv4 and IPv6 addresses from text to binary form
  //cout << "ip: " << ip << " " << ip.length() << " " << port << " "<< mess<< endl;
  if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0)
  {
    std::cout << "Invalid address\n";
    fprintf(stderr, "%m\n");
    //exit(1);
  }

  if (connect(sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    std::cout << "Connection Failed\n";
    fprintf(stderr, "%m\n");
    //exit(1);
  }

  if (send(sock, mess.c_str(), mess.length(), 0) < 0)
  {
    std::cout << "ERROR send\n";
    //exit(1);
  }

  close(sock);
}

void recv_mess(int port)
{
  int server_fd, new_socket;
  sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[Len_buff] = {0};

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {
    std::cout << "Socket creation error\n";
    //exit(1);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(server_fd, (sockaddr *)&address, sizeof(address)) < 0)
  {
    std::cout << "Bind failed\n";
    //exit(1);
  }

  if (listen(server_fd, 100) < 0)
  {
    std::cout << "Listen ERROR\n";
  }

  int i = 0;
  while (1)
  {
    if ((new_socket = accept(server_fd, (sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
      std::cout << "ERROR accept\n";
    }

    int recv_len;
    memset(buffer, 0, Len_buff);
    if ((recv_len = recv(new_socket, buffer, Len_buff, 0)) < 0)
    {
      std::cout << "ERROR recv\n";
      //exit(1);
    }

    std::string str = buffer;
    //cout << "Mess duoc gui: " << str << endl;
    mtx_vstr.lock();
    gb_v_str.push_back(str);
    mtx_vstr.unlock();
    cv_recv.notify_one();
  }
}

// het 15s tang round 1 lan
void OutOfTime(int num_proc)
{
  int r4 = -1, z = 0;
  while (1)
  {
    usleep(TIME);
    if (z == 5)
      exit(0);
    flag_pro = true;
    if (plusround == true)
    {
      round++;
      round = round % num_proc;
    }
    cout << "ROUN " << round << endl;
    int_yes = 0;
    plusround = true;
    write_log = true;
    r4++;
    if (r4 == 4)
    {
      z++;
      r4 = 0;
      silent = false;
      for (int i = 0; i < 2; i++)
      {
        int a = nodedown.back();
        nodedown.pop_back();
        if (a == global_id)
        {
          silent = true;
          break;
        }
      }
    }

    cv_pro.notify_one();
  }
}

void log(string id)
{
  int tg = (int)((clock() - start) / 1000 - 3);

  ofstream ff;
  ff.open(FOLDER + id + NAME_FILE, ios::app);
  if (!ff.is_open())
  {
    cout << "File log can't open" << endl;
    exit(1);
  }

  if (check_propose == false)
  {
    cout << "Proposer error" << endl;
    return;
  }

  ff << "Thoi gian: " << to_string(tg) << '\n';
  ff << "Proposer: " << to_string(round + 1) << '\n';
  ff << "Message: " << to_string(proposer) << "\n\n";

  ff.close();
}

void broadcast(int id, vector<info> v_info, string mess)
{
  for (int i = 0; i < v_info.size(); i++)
  {
    thread *thr = new thread(send_mess, v_info[i].ip, v_info[i].port, mess);
    thr->detach();
  }
}

bool check_length_vstr()
{
  if (gb_v_str.size() != 0)
    return true;
  return false;
}

void classify_mess(int id, vector<info> v_info, vector<string> store_mess)
{
  while (store_mess.size() != 0)
  {
    string data = store_mess.back();
    store_mess.pop_back();

    if (data != "yes" && data != "no")
    {
      if (silent == false)
      {
        int x;
        if (stoi(data) == round + 1 && Byzantine == false)
        {
          thread thr_bro(broadcast, id, v_info, "yes");
          thr_bro.detach();
          proposer = stoi(data);
          check_propose = true;
        }
        else
        {
          thread thr_bro(broadcast, id, v_info, "no");
          thr_bro.detach();
        }
      }
    }
    else
    {
      // Khong xet truong hop node loi gui nhieu goi tra loi
      mtx_log.lock();
      if (data == "yes" && write_log == true)
        int_yes++;

      if (int_yes >= 4)
      {
        //ghi file
        log(to_string(id));

        int_yes = 0;
        check_propose = false;
        round++;
        round = round % v_info.size();
        write_log = false;
        plusround = false;
      }
      mtx_log.unlock();
    }
  }
}

void fun_propose(int id, vector<info> v_info)
{
  while (1)
  {
    unique_lock<mutex> lck(mtx_pro);
    cv_pro.wait(lck, []() { return flag_pro; });
    cout << "Chay " << global_id << endl;
    flag_pro = false;
    if (silent == false)
    {
      if (Byzantine == false)
      {
        if (round == id - 1)
        {
          cout << "Proposer: " << id << endl;
          thread thr_send(broadcast, id, v_info, to_string(id));
          thr_send.detach();
        }
      }
      else
      {
        srand((int)time(0));
        int iid = rand() % 7 + 1;
        thread thr_send(broadcast, id, v_info, to_string(iid));
        thr_send.detach();
      }
    }
  }
}

void consensus(int id, vector<info> v_info)
{
  thread thr_recv(recv_mess, v_info[id - 1].port);
  thr_recv.detach();

  thread thr_propo(fun_propose, id, v_info);
  thr_propo.detach();

  sleep(3);

  flag_pro = true;
  cv_pro.notify_one();

  thread thr_timeout(OutOfTime, v_info.size());
  thr_timeout.detach();

  while (1)
  {
    vector<string> store_mess;
    unique_lock<mutex> lock(mtx_recv);
    cv_recv.wait(lock, check_length_vstr);
    if (gb_v_str.size() != 0)
    {
      mtx_vstr.lock();
      while (gb_v_str.size() != 0)
      {
        store_mess.push_back(gb_v_str.back());
        gb_v_str.pop_back();
      }
      mtx_vstr.unlock();
      // Xu ly goi tin
      thread cl_mess(classify_mess, id, v_info, store_mess);
      cl_mess.detach();
    }
  }
}

vector<info> GetInfo()
{
  ifstream ff;
  ff.open(CONF);
  if (!ff.is_open())
  {
    cout << "Can't open conf.txt" << endl;
    exit(1);
  }

  vector<info> v_info;
  while (!ff.eof())
  {
    info inf;
    string str;

    ff >> str;
    inf.id = stoi(str);
    str.clear();
    ff >> str;
    inf.ip = str;
    str.clear();
    ff >> str;
    inf.port = stoi(str);

    v_info.push_back(inf);
  }

  ff.close();
  return v_info;
}

int main(int argc, char *argv[])
{
  if (argc != 2 and argc != 3)
  {
    cout << "argument false." << endl;
    exit(1);
  }

  string id = argv[1];
  global_id = stoi(id);

  string Byz;
  if (argc == 3)
  {
    Byz = argv[2];
    if (Byz == "B")
      Byzantine = true;
  }

  vector<info> v_info;
  v_info = GetInfo();

  consensus(stoi(id), v_info);

  return 0;
}