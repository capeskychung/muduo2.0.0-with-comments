// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include <muduo/base/noncopyable.h>

#include <assert.h>
#include <pthread.h>
#include <stdlib.h> // atexit

namespace muduo
{

namespace detail
{
// This doesn't detect inherited member functions!
// http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
template<typename T>
struct has_no_destroy
{
  template <typename C> static char test(decltype(&C::no_destroy));
  template <typename C> static int32_t test(...);
  const static bool value = sizeof(test<T>(0)) == 1;
};
}  // namespace detail

template<typename T>
class Singleton : noncopyable //不可被拷贝
{
 public:
  Singleton() = delete;
  ~Singleton() = delete;

// 返回单个对象
  static T& instance()
  {
    // pthread_once保证了该函数只执行一次，而且多个线程同时调用时，pthread_once能够保证线程安全
    pthread_once(&ponce_, &Singleton::init);
    assert(value_ != NULL);
    return *value_;
  }

 private:
//  在内部创建对象
  static void init()
  {
    value_ = new T();
    if (!detail::has_no_destroy<T>::value)
    {
      // 登记销毁函数，在整个程序结束时调用销毁函数
      ::atexit(destroy);
    }
  }

// 在内部销毁对象
  static void destroy()
  {
    // complete_type完整的类型，数组大小不能是-1
    /* 
      class A;做了一个前项声明
      A*p; 
      delete p;//会警报，不会错误；    
     */
    // 数组大小不能为0，编译时不能发现错误
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy; (void) dummy;

    delete value_;
    value_ = NULL;
  }

 private:
  static pthread_once_t ponce_; //能够保证一个对象执行一次
  static T*             value_;
};

template<typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template<typename T>
T* Singleton<T>::value_ = NULL;

}  // namespace muduo

#endif  // MUDUO_BASE_SINGLETON_H
