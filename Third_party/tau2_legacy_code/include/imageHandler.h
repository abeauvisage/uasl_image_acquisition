#ifndef IMAGEHANDLER_H
#define IMAGEHANDLER_H

#include <deque>
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>

#include <opencv2/core/core.hpp>

class ImageHandler
{
    public:
        enum class Action {SAVE, SHOW};
        static std::mutex mutex_opencv;
        ImageHandler(){}
        ImageHandler(std::string dir,std::string cam_name,Action action=Action::SAVE) : m_dir(dir), m_cam_name(cam_name), m_action(action){}
        void configure(std::string dir,std::string cam_name,Action action=Action::SAVE){m_dir = dir; m_cam_name = cam_name; m_action = action;}
        void processImages();
        void addImage(std::pair<int,cv::Mat> pair);
        std::pair<int,cv::Mat> getLast();
        bool imagesToProcess(){/*std::lock_guard<std::mutex> lock(m_mutex);*/ return (m_isProcessing && !m_imgs.empty());}
        void stop(){m_isProcessing=false;m_cnd_var.notify_one();}

    private:

    std::deque<std::pair<int,cv::Mat>> m_imgs;
    std::string m_dir;
    std::string m_cam_name;
    Action m_action;
    std::mutex m_mutex;
    std::condition_variable m_cnd_var;
    bool m_isProcessing=false;

};

#endif // IMAGEHANDLER_H
