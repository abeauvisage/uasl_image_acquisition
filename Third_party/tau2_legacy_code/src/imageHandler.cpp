#include "imageHandler.h"

#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>

using namespace cv;
using namespace std;

mutex ImageHandler::mutex_opencv;

void ImageHandler::processImages(){

    if(m_cam_name.empty()){
        cerr << "[Error] No camera name specified. Please configure camera." << endl;
        return;
    }

    if(m_dir.empty())
        cerr << "[Warning] No output directory specified. Please configure camera." << endl;

    m_isProcessing = true;
    while(m_isProcessing || !m_imgs.empty()){

        unique_lock<mutex> lock(m_mutex);
        if(m_imgs.empty()){
            m_cnd_var.wait(lock);
            continue;
        }

        int i = m_imgs.front().first;
        cv::Mat img = m_imgs.front().second.clone();
        m_imgs.pop_front();
        lock.unlock();

        lock_guard<mutex> lock_opencv(ImageHandler::mutex_opencv);
        switch(m_action){
            case Action::SAVE:{
                string img_nb_s = to_string(i);
                imwrite(m_dir+"/"+m_cam_name+"_image"+string(5-img_nb_s.length(),'0')+img_nb_s+".png",img);break;
            }
            case Action::SHOW:
                imshow(m_cam_name,img);waitKey(1);break;
        }
    }
}


void ImageHandler::addImage(std::pair<int,cv::Mat> pair){

    lock_guard<mutex> lock(m_mutex);
    m_imgs.push_back(pair);
    if(m_imgs.size() % 20 == 0)
        cerr << "[Warning] " << m_cam_name << " has "  << m_imgs.size() << "images in the queue" << endl;
    m_cnd_var.notify_all();
}

std::pair<int,cv::Mat> ImageHandler::getLast(){

    lock_guard<mutex> lock(m_mutex);
    std::pair<int,cv::Mat> pair = m_imgs.back();
    m_imgs.clear();

    return pair;
}
