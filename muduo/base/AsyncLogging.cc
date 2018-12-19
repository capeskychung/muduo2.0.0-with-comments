// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/AsyncLogging.h>
#include <muduo/base/LogFile.h>
#include <muduo/base/Timestamp.h>

#include <stdio.h>

using namespace muduo;

AsyncLogging::AsyncLogging(const string& basename,
                           off_t rollSize,
                           int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    rollSize_(rollSize),
    thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
    latch_(1),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_()
{
  currentBuffer_->bzero(); //清空缓冲区
  nextBuffer_->bzero();
  buffers_.reserve(16);//预留16个空间
}

void AsyncLogging::append(const char* logline, int len)
{
  muduo::MutexLockGuard lock(mutex_);
  if (currentBuffer_->avail() > len)
  {
    // 当前缓冲区未满，将数据追加到末尾
    currentBuffer_->append(logline, len);
  }
  else
  {
    // 当前缓冲区已满，将当前缓冲区添加到待写入文件的已填满的缓冲区列表
    buffers_.push_back(std::move(currentBuffer_));

// 将当前缓冲区设置为预备缓冲区
    if (nextBuffer_)
    {
      currentBuffer_ = std::move(nextBuffer_);
    }
    else
    {
      // 这种情况，极少发生，前端写入速度太快，一下子把两块缓冲区都写完
      // 那么，只好分配一块新的缓冲区
      currentBuffer_.reset(new Buffer); // Rarely happens
    }
    currentBuffer_->append(logline, len);
    cond_.notify(); //通知后端开始写日志（条件变量来通知）
  }
}

void AsyncLogging::threadFunc()
{
  assert(running_ == true);
  latch_.countDown();
  LogFile output(basename_, rollSize_, false);
  // 准备两块空闲缓冲区
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  while (running_)
  {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty()); 

    {
      muduo::MutexLockGuard lock(mutex_);
      if (buffers_.empty())  // unusual usage! //（条件变量一般用while循环，用if可能会产生虚假唤醒）注意，这里是一个非常规用法
      {
        cond_.waitForSeconds(flushInterval_); //等待前端写满一个或者多个buffer，或者一个超时时间到来
      }
      buffers_.push_back(std::move(currentBuffer_)); //将当前缓冲区移入buffers_
      currentBuffer_ = std::move(newBuffer1); //将空闲的newBuffer1置为当前缓冲区
      buffersToWrite.swap(buffers_); //buffers与buffersToWrite交换，这样后面的代码可以在临界区之外安全地访问buffersToWrite
      if (!nextBuffer_)
      {
        nextBuffer_ = std::move(newBuffer2);
        // 确保前端始终有一个预备buffer可供调配，减少前端临界区分配内存的概率，缩短前端临界区长度
      }
    }

    assert(!buffersToWrite.empty());

// 消息堆积
/* 
  前端陷入死循环，拼命发送日志信息，超过后端的处理能力，这就是典型的生产速度超过消费
  速度问题，会造成数据在内存中堆积，严重时引发性能问题（可用内存不足）或程序崩溃（分配内存失败）
 */
    if (buffersToWrite.size() > 25)
    {
      char buf[256];
      snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               buffersToWrite.size()-2);
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end()); //丢掉多余日志，以腾出内存，仅保存2个buffer
    }

    for (const auto& buffer : buffersToWrite)
    {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffer->data(), buffer->length());
    }

    if (buffersToWrite.size() > 2)
    {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2); //仅保存两个buffer，用于newBuffer与newBuffer2
    
    }

    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}

