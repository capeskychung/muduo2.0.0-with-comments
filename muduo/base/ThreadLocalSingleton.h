// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include <muduo/base/noncopyable.h>

#include <assert.h>
#include <pthread.h>

namespace muduo
{

// 线程本地单例类型封装
template<typename T>
class ThreadLocalSingleton : noncopyable
{
 public:
  ThreadLocalSingleton() = delete;
  ~ThreadLocalSingleton() = delete;

// 返回单例对象，不用线程安全，因为每个线程都有一个指针，按照普通指针操作即可
  static T& instance()
  {
    if (!t_value_)
    {
      t_value_ = new T();
      // 调用Deleter构造函数，
      deleter_.set(t_value_);
    }
    return *t_value_;
  }

// 返回单例对象，返回对象的指针
  static T* pointer()
  {
    return t_value_;
  }

 private:
//  销毁对象，借助Deleter才能调用
  static void destructor(void* obj)
  {
    assert(obj == t_value_);
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy; (void) dummy;
    delete t_value_;
    t_value_ = 0;
  }

// 这个嵌套类，主要是为了去销毁
  class Deleter
  {
   public:
    Deleter()
    {
      // 借助POSIX TSD
      pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
    }

    ~Deleter()
    {
      pthread_key_delete(pkey_);
    }

    void set(T* newObj)
    {
      assert(pthread_getspecific(pkey_) == NULL);
      pthread_setspecific(pkey_, newObj);
    }

    pthread_key_t pkey_;
  };

  static __thread T* t_value_; //pod类型？__thread关键字，表示每个线程都有一份
  static Deleter deleter_; //主要用来销毁指针所指向的对象
};

template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = 0;

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

}  // namespace muduo
#endif  // MUDUO_BASE_THREADLOCALSINGLETON_H
