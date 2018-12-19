// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.
// 用RAII方法封装socket file descriptor

#ifndef MUDUO_NET_SOCKET_H
#define MUDUO_NET_SOCKET_H

#include <muduo/base/noncopyable.h>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo
{
///
/// TCP networking.
///
namespace net
{

class InetAddress;

///
/// Wrapper of socket file descriptor.
///
/// It closes the sockfd when desctructs.
/// It's thread safe, all operations are delagated to OS.
class Socket : noncopyable
{
 public:
  explicit Socket(int sockfd)
    : sockfd_(sockfd) //用createNonblockingOrDie返回的文件描述符进行赋值
  { }

  // Socket(Socket&&) // move constructor in C++11
  ~Socket();

// 返回文件描述符
  int fd() const { return sockfd_; }
  // return true if success.
  bool getTcpInfo(struct tcp_info*) const;
  bool getTcpInfoString(char* buf, int len) const;

  /// abort if address in use
  void bindAddress(const InetAddress& localaddr);
  /// abort if address in use
  void listen();

  /// On success, returns a non-negative integer that is
  /// a descriptor for the accepted socket, which has been
  /// set to non-blocking and close-on-exec. *peeraddr is assigned.
  /// On error, -1 is returned, and *peeraddr is untouched.
  int accept(InetAddress* peeraddr);

  void shutdownWrite();

  ///
  /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
  ///
  ///Nagle算法可以一定程度上避免网络拥塞，将小包攒在一起发送
  ///TCP NODELAY选项可以禁用Nagle算法
  ///禁用Nagle算法，可以避免连续发包出现延迟，这对于编写低延迟的网络服务很重要
  void setTcpNoDelay(bool on);

  ///
  /// Enable/disable SO_REUSEADDR
  /// 设置地址重复利用
  void setReuseAddr(bool on);

  ///
  /// Enable/disable SO_REUSEPORT
  ///
  void setReusePort(bool on);

  ///
  /// Enable/disable SO_KEEPALIVE
  /// TCP keepalibe是指定期探测连接是否存在，如果应用层有心跳的话，这个选项不是必须要设置的
  void setKeepAlive(bool on);

 private:
  const int sockfd_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_SOCKET_H
