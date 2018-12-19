// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Types.h>

#include <deque>
#include <vector>

namespace muduo
{

class ThreadPool : noncopyable
{
 public:
//  Task任务函数
  typedef std::function<void ()> Task;

  explicit ThreadPool(const string& nameArg = string("ThreadPool"));
  ~ThreadPool();

  // Must be called before start().
  void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
  void setThreadInitCallback(const Task& cb)
  { threadInitCallback_ = cb; }

// 启动线程池，numThreads个线程
  void start(int numThreads); 
  // 关闭线程池
  void stop();

  const string& name() const
  { return name_; }

  size_t queueSize() const;

  // Could block if maxQueueSize > 0
  // 运行任务，往线程池队列中添加任务
  void run(Task f);

 private:
  bool isFull() const REQUIRES(mutex_);
  // 线程池中线程要执行的函数
  void runInThread();
  // 获取任务
  Task take();

  mutable MutexLock mutex_;
  Condition notEmpty_ GUARDED_BY(mutex_);
  Condition notFull_ GUARDED_BY(mutex_);
  string name_; //线程池名称
  Task threadInitCallback_;
  std::vector<std::unique_ptr<muduo::Thread>> threads_; //线程指针的数组
  std::deque<Task> queue_ GUARDED_BY(mutex_); //任务队列
  size_t maxQueueSize_;
  bool running_; //线程池是否处于运行状态
};

}  // namespace muduo

#endif  // MUDUO_BASE_THREADPOOL_H
