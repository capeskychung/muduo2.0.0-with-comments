// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>

namespace muduo
{
namespace net
{

class EventLoop;

class EventLoopThread : noncopyable
{
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

// 默认传入空的回调函数
  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const string& name = string());
  ~EventLoopThread();
  EventLoop* startLoop(); //启动线程，该线程就成为了IO线程，线程函数运行

 private:
  void threadFunc(); //线程函数，启动后会创建一个EventLoop对象，将loop_指针指向EventLoop对象

  EventLoop* loop_ GUARDED_BY(mutex_); //loop_指针指向一个EventLoop对象
  bool exiting_; //是否退出
  Thread thread_;
  MutexLock mutex_; //
  Condition cond_ GUARDED_BY(mutex_); //条件变量
  ThreadInitCallback callback_; //如果回调函数不是空的，回调函数在EventLoop::loop事件循环之前被调用
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_EVENTLOOPTHREAD_H

