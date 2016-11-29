#ifndef IMAGE_ACQUISITION_MULTIPLE_CAMERA_SYNC_HPP
#define IMAGE_ACQUISITION_MULTIPLE_CAMERA_SYNC_HPP

#include <thread>
#include <condition_variable>

namespace cam
{

class Barrier {
public:
    explicit Barrier(std::size_t initial_count) :
      threshold(initial_count),
      count(initial_count),
      generation(0) {}

    void wait()
    {
        size_t local_gen = generation;
        std::unique_lock<std::mutex> local_lock(mtx);
        if (!--count)
        {
            generation++;
            count = threshold;
            cond.notify_all();
        }
        else
        {
            cond.wait(local_lock, [this, local_gen] { return local_gen != generation; });
        }
    }

private:
    std::mutex mtx;
    std::condition_variable cond;
    const std::size_t threshold;//Keep the initial_count in memory
    std::size_t count;//Count the number of threads waiting
    std::size_t generation;//Become different from local_gen when the number of waiting threads reach threshold
};



}//namespace cam

#endif
