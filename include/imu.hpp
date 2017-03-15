#ifndef IMAGE_ACQUISITION_IMU_HPP
#define IMAGE_ACQUISITION_IMU_HPP

#include "util_clock.hpp"

#include <thread>
#include <mutex>

#include <deque>
#include <string>
#include <vector>
#include <atomic>
#include <condition_variable>

namespace imu
{
static constexpr size_t max_size_data = 200;//Max number of measurement to keep
static constexpr int timeout_ms_get_data = 500;//Timeout value for getting data from the worker thread


struct IMU_data
{
    IMU_data(double x_ = 0, double y_ = 0, double z_ = 0):
        x(x_),
        y(y_),
        z(z_)
    {}
    double x;
    double y;
    double z;
    std::chrono::time_point<cam::clock_type> time_pt;
};

class IMU
{
    public:
    IMU();
    ~IMU();

    int start_acq();
    int stop_acq();
    
    int retrieve_data(std::vector<IMU_data>& data_retrieved, size_t size);//Retrieve last size measurement (if size == 0
    //then retrieve them all). Retrieved Elements are removed from the data

    void set_write_on_disk(bool b, const std::string& filepath_ = std::string());//Activate/deactivate the writing of the data to the disk

    void set_data_keep_rate(unsigned int value);
    private:
    bool is_opened;//True if the device was successfully opened

    std::atomic<bool> write_on_disk;
    std::string filepath;
    std::mutex mtx_writing_var;//Protect the writing variable filepath

    std::thread thd;//Acquisition thread
    std::atomic<bool> started;//Of the thread has started
    
    std::deque<IMU_data> data;//Data fetched from the IMU
    std::mutex mtx_data;//Protect the data from the IMU

    std::condition_variable data_has_changed;//Notification when new IMU data is acquired
    bool data_has_been_returned;

    std::atomic<unsigned int> data_keep_rate;//Keep 1 out of data_keep_rate value

    void init();
    IMU_data get_data_from_imu() const;//Gets the data from the IMU
    void thread_func();
    void write_data_disk();

};

}//End of imu namespace
#endif
