#include "codec.h"

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/ThreadLocalSingleton.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

#include <set>
#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer : noncopyable
{
 public:
  ChatServer(EventLoop* loop,
             const InetAddress& listenAddr)
  : server_(loop, listenAddr, "ChatServer"),
    codec_(std::bind(&ChatServer::onStringMessage, this, _1, _2, _3))
  {
    server_.setConnectionCallback(
        std::bind(&ChatServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
  }

  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void start()
  {
    server_.setThreadInitCallback(std::bind(&ChatServer::threadInit, this, _1));
    server_.start();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected()) //每个IO线程都有一个conn实例，不需要锁
    {
      LocalConnections::instance().insert(conn);
    }
    else
    {
      LocalConnections::instance().erase(conn);
    }
  }

  void onStringMessage(const TcpConnectionPtr&,
                       const string& message,
                       Timestamp)
  {
    EventLoop::Functor f = std::bind(&ChatServer::distributeMessage, this, message);
    LOG_DEBUG;

    MutexLockGuard lock(mutex_);
    // 转发消息给所有客户端，高效转发(多线程来转发)
    for (std::set<EventLoop*>::iterator it = loops_.begin();
        it != loops_.end();
        ++it)
    {
      // 1、让对应的IO线程来执行distributeMessage
      // 2、distributeMessage放到IO线程队列中执行，因此，这里的mutex_锁竞争大大减小
      // 3、distributeMessage不受mutex_保护
      (*it)->queueInLoop(f); //*it 是从loops中取出的EventLoop对象，EventLoop对象所在的IO线程来执行distributeMessage
      // 通过客户端所在IO来转发
    }
    LOG_DEBUG;
  }

  typedef std::set<TcpConnectionPtr> ConnectionList;

  void distributeMessage(const string& message)
  {
    LOG_DEBUG << "begin";
    
    for (ConnectionList::iterator it = LocalConnections::instance().begin();
        it != LocalConnections::instance().end();
        ++it)
    {
      codec_.send(get_pointer(*it), message);
    }
    LOG_DEBUG << "end";
  }

  void threadInit(EventLoop* loop)
  {
    assert(LocalConnections::pointer() == NULL);
    LocalConnections::instance();
    assert(LocalConnections::pointer() != NULL);
    MutexLockGuard lock(mutex_);
    loops_.insert(loop);
  }

  TcpServer server_;
  LengthHeaderCodec codec_;
  // 线程局部单例变量，每个线程都有一个connections_实例
  typedef ThreadLocalSingleton<ConnectionList> LocalConnections;

  MutexLock mutex_;
  std::set<EventLoop*> loops_ GUARDED_BY(mutex_); //EventLoop列表
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress serverAddr(port);
    ChatServer server(&loop, serverAddr);
    if (argc > 2)
    {
      server.setThreadNum(atoi(argv[2]));
    }
    server.start();
    loop.loop();
  }
  else
  {
    printf("Usage: %s port [thread_num]\n", argv[0]);
  }
}


