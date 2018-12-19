#include <muduo/base/BlockingQueue.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Thread.h>

#include <memory>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>

class Test
{
 public:
  Test(int numThreads)
    : latch_(numThreads)
  {
    for (int i = 0; i < numThreads; ++i)
    {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i); //线程名称
      threads_.emplace_back(new muduo::Thread(
            std::bind(&Test::threadFunc, this), muduo::string(name)));//线程绑定的函数
    }
    for (auto& thr : threads_)
    {
      thr->start(); //启动线程
    }
  }

  void run(int times)
  {
    printf("waiting for count down latch\n");
    latch_.wait();//等待倒计时
    printf("all threads started\n");
    // 无界队列，添加100个产品
    for (int i = 0; i < times; ++i)
    {
      char buf[32];
      snprintf(buf, sizeof buf, "hello %d", i);
      queue_.put(buf);
      printf("tid=%d, put data = %s, size = %zd\n", muduo::CurrentThread::tid(), buf, queue_.size());
    }
  }

  void joinAll()
  {
    // 添加了5个stop产品，会被5各产品消费，从而结束5个线程
    for (size_t i = 0; i < threads_.size(); ++i)
    {
      queue_.put("stop");
    }

    for (auto& thr : threads_)
    {
      thr->join();
    }
  }

 private:

  void threadFunc()
  {
    printf("tid=%d, %s started\n",
           muduo::CurrentThread::tid(),
           muduo::CurrentThread::name());

    latch_.countDown();//计数减一，五个线程减到0后，主线程才会运行

    // 消费100个产品
    bool running = true;
    while (running)
    {
      std::string d(queue_.take());
      printf("tid=%d, get data = %s, size = %zd\n", muduo::CurrentThread::tid(), d.c_str(), queue_.size());
      running = (d != "stop");//产品名称为“stop"时跳出循环
    }

    printf("tid=%d, %s stopped\n",
           muduo::CurrentThread::tid(),
           muduo::CurrentThread::name());
  }

  muduo::BlockingQueue<std::string> queue_;
  muduo::CountDownLatch latch_;
  std::vector<std::unique_ptr<muduo::Thread>> threads_;
};

void testMove()
{
  muduo::BlockingQueue<std::unique_ptr<int>> queue;
  queue.put(std::unique_ptr<int>(new int(42)));
  std::unique_ptr<int> x = queue.take();
  printf("took %d\n", *x);
  *x = 123;
  queue.put(std::move(x));
  std::unique_ptr<int> y = queue.take();
  printf("took %d\n", *y);
}

int main()
{
  printf("pid=%d, tid=%d\n", ::getpid(), muduo::CurrentThread::tid());
  Test t(5);
  t.run(100);
  t.joinAll();

  testMove();

  printf("number of created threads %d\n", muduo::Thread::numCreated());
}
