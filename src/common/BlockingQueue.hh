#ifndef _BLOCKINGQUEUE_HH_
#define _BLOCKINGQUEUE_HH_

#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>

using namespace std;

template <class T>
class BlockingQueue {
  private:
    mutex _mutex;
    condition_variable _cv;
    deque<T> _queue;
  public:
    void push(T value) {
      {
        unique_lock<mutex> lock(_mutex);
	_queue.push_back(value);
      }
      _cv.notify_one();
    };

    T pop(){
      unique_lock<mutex> lock(_mutex);
      _cv.wait(lock, [=]{ return !_queue.empty(); });
      T toret(_queue.front());
      _queue.pop_front();

      return toret;
    };

    int getSize() {
      return _queue.size();
    }
};

#endif
