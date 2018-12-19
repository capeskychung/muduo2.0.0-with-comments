#include <muduo/base/BlockingQueue.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>

class Bench //用来度量时间的类
{
 public:
  Bench(int numThreads)
    : latch_(numThreads)
  {
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i)
    {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i);
      threads_.emplace_back(new muduo::Thread(
            std::bind(&Bench::threadFunc, this), muduo::string(name)));
    }
    for (auto& thr : threads_)
    {
      thr->start();
    }
  }

  void run(int times)
  {
    printf("waiting for count down latch\n");
    latch_.wait();
    printf("all threads started\n");
    // 生产产品，把时间添加到队列中（有界对列），1000微秒一次
    for (int i = 0; i < times; ++i)
    {
      muduo::Timestamp now(muduo::Timestamp::now());
      queue_.put(now);
      usleep(1000);
    }
  }

  void joinAll()
  {
    for (size_t i = 0; i < threads_.size(); ++i)
    {
      queue_.put(muduo::Timestamp::invalid());//生产非法时间
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

    std::map<int, int> delays;//<迟延，迟延的个数>
    latch_.countDown();
    bool running = true;
    while (running)
    {
      muduo::Timestamp t(queue_.take());
      muduo::Timestamp now(muduo::Timestamp::now());
      if (t.valid()) //如果是合法的时间
      {
        // 以微妙为单位，做时间差；生产这个时间到消费这个时间的时间差
        int delay = static_cast<int>(timeDifference(now, t) * 1000000);
        // printf("tid=%d, latency = %d us\n",
        //        muduo::CurrentThread::tid(), delay);
        ++delays[delay];
      }
      running = t.valid();//时间非法时跳出循环
    }

    printf("tid=%d, %s stopped\n",
           muduo::CurrentThread::tid(),
           muduo::CurrentThread::name());
    for (const auto& delay : delays)
    {
      printf("tid = %d, delay = %d, count = %d\n",
             muduo::CurrentThread::tid(),
             delay.first, delay.second);
    }
  }

  muduo::BlockingQueue<muduo::Timestamp> queue_;
  muduo::CountDownLatch latch_;
  std::vector<std::unique_ptr<muduo::Thread>> threads_;
};

int main(int argc, char* argv[])
{
  int threads = argc > 1 ? atoi(argv[1]) : 1;

  Bench t(threads);
  t.run(10000);
  t.joinAll();
}
