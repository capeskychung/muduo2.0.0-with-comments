// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

namespace muduo
{
/* 
  CountDownLatch：对条件变量的封装；一个倒计时门闩类
  既可以用于所有子线程等待主线程发起“起跑”
  也可以用于主线程等待子线程初始化完毕才开始工作
 */
class CountDownLatch : noncopyable
{
 public:

  explicit CountDownLatch(int count);

  void wait();

  void countDown(); //计数器减一，减到0后发起通知

  int getCount() const;

 private:
  mutable MutexLock mutex_; //mutable使得mutex_可变
  Condition condition_ GUARDED_BY(mutex_);
  int count_ GUARDED_BY(mutex_); //计数器
};

}  // namespace muduo
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
