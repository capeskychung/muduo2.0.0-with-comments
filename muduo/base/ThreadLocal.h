// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include <muduo/base/Mutex.h>  // MCHECK
#include <muduo/base/noncopyable.h>

#include <pthread.h>

namespace muduo
{

template<typename T>
class ThreadLocal : noncopyable
{
 public:
  ThreadLocal()
  {
    // 指定了数据销毁函数destructor
    MCHECK(pthread_key_create(&pkey_, &ThreadLocal::destructor));
  }

  ~ThreadLocal()
  {
    // 仅仅销毁key，并不是销毁实际数据
    MCHECK(pthread_key_delete(pkey_));
  }

// 获取线程特定数据
  T& value()
  {
    T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));
    // 如果返回指针为空，就创建这个数据
    if (!perThreadValue)
    {
      T* newObj = new T();
      MCHECK(pthread_setspecific(pkey_, newObj));
      perThreadValue = newObj;
    }
    return *perThreadValue;
  }

 private:

// 回调函数，销毁实际
  static void destructor(void *x)
  {
    T* obj = static_cast<T*>(x);
    // 检测是否为完全类型
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy; (void) dummy;
    delete obj;
  }

 private:
  pthread_key_t pkey_;
};

}  // namespace muduo

#endif  // MUDUO_BASE_THREADLOCAL_H
