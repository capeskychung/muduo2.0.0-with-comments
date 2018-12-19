// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include <muduo/base/Mutex.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/Channel.h>

namespace muduo
{
namespace net
{

class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
/*
 定时器队列，定时器的管理器，内部维护了一个定时器列表 
  TimerQueue数据结构的选择，能快速根据当前时间找到已到期（按时间排序）的定时器，
  也要高效的添加和删除Timer，因而可以用二叉搜索树，用map或者set
*/
class TimerQueue : noncopyable
{
 public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  // 添加一个定时器，一定是线程安全的，可以跨线程调用。通常情况下被其他线程调用。
  TimerId addTimer(TimerCallback cb,
                   Timestamp when,
                   double interval);

// 取消一个定时器，传入一个TimerId外部类
  void cancel(TimerId timerId);

 private:

  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  // This requires heterogeneous comparison lookup (N3465) from C++14
  // so that we can find an T* in a set<unique_ptr<T>>.
  // unique_ptr是C++11标准的一个独享所有权的智能指针
  // 无法得到指向同一个对象的两个unique_ptr指针
  // 但可以进行移动构造与移动赋值操作，即所有权可以移动到另一个对象（而非拷贝构造）
  // 可能会存在<Timestamp, Timer*>中时间戳相同，操作不同的定时器，不可以用map；用multimap可以，但不常用
  // 
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TimerList; //按照时间戳排序
  typedef std::pair<Timer*, int64_t> ActiveTimer;//<地址,序号>
  typedef std::set<ActiveTimer> ActiveTimerSet;//按地址排序，保存的定时器列表与TimerList相同

//以下成员 函数只可能在其所属的I/O线程中调用，因而不必加锁；不能跨线程调用
// 服务器性能杀手之一是锁竞争，所以要尽可能少用锁
  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerId timerId);
  // called when timerfd alarms
  // 定时器到期后的回调函数
  void handleRead();
  // move out all expired timers
  // 返回超时的定时器列表
  std::vector<Entry> getExpired(Timestamp now);
  void reset(const std::vector<Entry>& expired, Timestamp now);

  bool insert(Timer* timer);

  EventLoop* loop_; //所属的EventLoop
  const int timerfd_; //所创建的定时器描述符
  Channel timerfdChannel_;
  // Timer list sorted by expiration
  TimerList timers_; //timers_是按到期时间排序

  // for cancel()
  // timers与activeTimers_保存的是相同的数据
  // timers_是按到期时间排序，activeTimers_是按对象地址排序
  ActiveTimerSet activeTimers_;
  bool callingExpiredTimers_; /* atomic *///是否处于调用处理那些到期定时器中
  ActiveTimerSet cancelingTimers_; //保存的是被取消的定时器
};

}  // namespace net
}  // namespace muduo
#endif  // MUDUO_NET_TIMERQUEUE_H
