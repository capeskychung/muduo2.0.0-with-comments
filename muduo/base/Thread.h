// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include <muduo/base/Atomic.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Types.h>

#include <functional>
#include <memory>
#include <pthread.h>

namespace muduo
{

class Thread : noncopyable
{
 public:
  typedef std::function<void ()> ThreadFunc;

  explicit Thread(ThreadFunc, const string& name = string());
  // FIXME: make it movable in C++11
  ~Thread();

  void start(); //启动线程
  int join(); // return pthread_join()

  bool started() const { return started_; } //线程真实启动状态
  // pthread_t pthreadId() const { return pthreadId_; }
  pid_t tid() const { return tid_; }
  const string& name() const { return name_; }

  static int numCreated() { return numCreated_.get(); } //已经启动线程数量

 private:
  void setDefaultName();

  bool       started_; //线程是否已经启动了
  bool       joined_;
  pthread_t  pthreadId_;
  pid_t      tid_; //线程的真实pid
  ThreadFunc func_; //该线程要回调的函数
  string     name_; //线程名称
  CountDownLatch latch_;

  static AtomicInt32 numCreated_; //已经创建的线程的个数（原子整数类），每创建一个，自动加一
};

}  // namespace muduo
#endif  // MUDUO_BASE_THREAD_H
