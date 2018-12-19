// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include <muduo/base/Mutex.h>
#include <muduo/base/Types.h>

#include <memory>

namespace muduo
{

namespace FileUtil
{
class AppendFile;
}

class LogFile : noncopyable
{
 public:
  LogFile(const string& basename,
          off_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 1024);
  ~LogFile();

// 将logline行，长度len添加到日志文件中
  void append(const char* logline, int len);
//   清空缓冲区
  void flush();
//   滚动日志
  bool rollFile();

 private:
//  不加锁的方式添加
  void append_unlocked(const char* logline, int len);

// 获取日志文件的名称
  static string getLogFileName(const string& basename, time_t* now);

  const string basename_; //日志文件的basename; 
  /*
   pp@pp-vb:$ basename /home/pp/codes/logfile_test.20181121-060448.pp-vb.83707.log
logfile_test.20181121-060448.pp-vb.83707.log
 */

  const off_t rollSize_; //日志文件达到rollSize_换一个文件
  const int flushInterval_; //日志写入间隔时间
  const int checkEveryN_;

  int count_; //计数器，初始值为0，当达到

  std::unique_ptr<MutexLock> mutex_;
  time_t startOfPeriod_; //开始记录日志时间（调整至零点的时间），方便下一次日志时间比较，
//   只要在一天之内，调整为0点的时间距离1970年1月1日0点时间相同，不会滚动
  time_t lastRoll_; //上一次滚动日志文件时间
  time_t lastFlush_;//上一次日志写入文件时间
  std::unique_ptr<FileUtil::AppendFile> file_; 

  const static int kRollPerSeconds_ = 60*60*24;//一天
};

}  // namespace muduo
#endif  // MUDUO_BASE_LOGFILE_H
