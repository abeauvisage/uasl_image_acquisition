#include "imu.hpp"


namespace imu
{

IMU::IMU():
    is_opened(false),
    write_on_disk(false),
    filepath("imu_data.txt"),
    started(false),
    data_has_been_returned(false),
    data_keep_rate(1)
{
    init();
}

IMU::~IMU()
{
    stop_acq();
}


int IMU::start_acq()
{
    //Start the acquisition
    if(!is_opened)
    {
        std::cerr << "IMU is not opened, cannot start acquisition." << std::endl;
        return -1;
    }

    if(started.load()) return 0;//Already started

    started = true;

    //Start the IMU, initialize params for this acquisition, etc ...

    thd = std::thread(&IMU::thread_func,this);

}

int IMU::stop_acq()
{
    started = false;
    if(thd.joinable()) thd.join();
    return 0;
}


int IMU::retrieve_data(std::vector<IMU_data>& data_retrieved, size_t size)
{
    if(!is_opened) return -1;
    std::unique_lock<std::mutex> mlock(mtx_data);

    bool success = data_has_changed.wait_for(mlock, std::chrono::milliseconds(timeout_ms_get_data), [this]{return !data_has_been_returned;}); 

    if(!success || data.empty()) return -1;
    
    size_t nb_element = (size > 0) ? std::min(size, data.size()) : data.size();//Number of elements to retrieve

    data_retrieved.resize(nb_element);

    for(size_t i = 0;i<nb_element;++i)
    {
        data_retrieved[i] = data.front();
        data.pop_front();
    }

    data_has_been_returned = true;
    return 0;
}

void IMU::set_write_on_disk(bool b, const std::string& filepath_)
{
    if(!is_opened)
    {
        std::cerr << "Error : IMU not opened." << std::endl;
        return;
    }

    std::lock_guard<std::mutex> mlock(mtx_writing_var);
    if(!filepath_.empty()) filepath = filepath_;
    write_on_disk = b;
}

void IMU::set_data_keep_rate(unsigned int value)
{
    if(!is_opened)
    {
        std::cerr << "Error : IMU not opened." << std::endl;
        return;
    }
    if(value>0)
    {
        data_keep_rate.store(value);
    }
    else
    {
        std::cerr << "The rate of keeping data cannot be zero" << std::endl;
    }
}
void IMU::init()
{
    //Do initialization of the IMU here
    
    //If we arrive here, the IMU has been opened correctly
    is_opened = true;
}

IMU_data IMU::get_data_from_imu() const
{
    //TODO : get the data from the IMU
    IMU_data current_data;

    return current_data;
}

void IMU::thread_func()
{
    unsigned int data_count = 0;
    while(started.load())
    {
       ++data_count;
       IMU_data current_data = get_data_from_imu();
       if(data_count%data_keep_rate.load() == 0)
       {
            {//Mutex scope
                std::lock_guard<std::mutex> mlock(mtx_data);
                data.push_front(current_data);
                if(data.size() > max_size_data) data.pop_back(); 
            }
            write_data_disk();
       }
    }

    //Clear the IMU state if needed
}

void IMU::write_data_disk()
{
    if(write_on_disk.load())
    {
        std::string data_path;
        {
            std::lock_guard<std::mutex> mlock(mtx_writing_var);
            data_path = filepath;
        }
        std::lock_guard<std::mutex> mlock(mtx_data);

        //TODO : write the actual data on the disk, file data_path
    }
}

}//End of imu namespace
