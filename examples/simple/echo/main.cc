#include "echo.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <unistd.h>

// using namespace muduo;
// using namespace muduo::net;

int main()
{
  LOG_INFO << "pid = " << getpid();
  // one loop per thread + threadpool
  // 每个线程有一个even-loop线程，没有就不是I/O线程
  muduo::net::EventLoop loop;
  // 初始化一个地址对象
  muduo::net::InetAddress listenAddr(2007);
  EchoServer server(&loop, listenAddr);
  server.start();
  loop.loop();
}

