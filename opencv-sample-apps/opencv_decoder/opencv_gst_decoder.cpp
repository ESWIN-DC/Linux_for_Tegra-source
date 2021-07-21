/*
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <pthread.h>
#include <queue>
#include <opencv2/core/core.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

void * display_thread(void *);

bool capture_complete = false;
pthread_mutex_t lock;
pthread_t displayThread;
std::queue<cv::Mat> frame_queue;

static void help()
{
    std::cout << "\nThis is a sample OpenCV application to demonstrate the "
    "usage of NVIDIA accelerated GStreamer to decode an mp4 H264 container "
    "file for VideoCapture\n\n"
    "./opencv_nvgstdec --file-path=<file_name>\n\n"
    "OPTIONS:\n"
    "\t-h,--help            Prints this message.\n"
    "\t--file-path          Absolute path of file.\n"
    "\t--show-fps[=true]    Option to display framerate from VideoCapture.\n\n"
    << std::endl;

}

static std::string create_capture(std::string filename)
{
    std::stringstream pipeline;
    pipeline << "filesrc location= "
        << filename << " !  qtdemux ! h264parse ! nvv4l2decoder ! nvvidconv ! "
        "video/x-raw, format=(string)I420 ! appsink drop=true sync=false ";

    return pipeline.str();
}

void * display_thread(void *)
{
    if (capture_complete)
    {
        std::cout << "End of stream" << std::endl;
        return NULL;
    }
    while (true)
    {
        cv::Mat display_frame;
        if (frame_queue.empty() && capture_complete)
        {
            break;
        }
        pthread_mutex_lock(&lock);
        if (!frame_queue.empty())
        {
            display_frame = frame_queue.front();
            frame_queue.pop();
        }
        pthread_mutex_unlock(&lock);
        if (!display_frame.empty())
        {
            cv::imshow("Display Window", display_frame);
            cv::waitKey(1);
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int count = 0;
    int return_val = 0;
    bool fps_display;
    double fps_captured;
    std::string filename;
    cv::VideoCapture capture;
    cv::TickMeter tick_in_loop;
    cv::Mat frame;

    pthread_mutex_init(&lock, NULL);
    cv::CommandLineParser parser(argc, argv,
        "{help h||}"
        "{file-path||}"
        "{show-fps||}"
    );

    if (parser.has("help"))
    {
        help();
        goto cleanup;
    }

    filename = parser.get<std::string>("file-path");
    fps_display = parser.has("show-fps");

    if (!parser.check())
    {
        parser.printErrors();
        return_val = -1;
        goto cleanup;
    }
    if(filename.empty())
    {
        std::cout << "ERROR: Input file is required" << std::endl;
        help ();
        return_val = -1;
        goto cleanup;
    }

    capture.open(create_capture(filename), cv::CAP_GSTREAMER);

    if (!capture.isOpened())
    {
        std::cerr << "Failed to open VideoCapture" << std::endl;
        return_val = -4;
        goto cleanup;
    }

    pthread_create(&displayThread, NULL, display_thread, NULL);

    while (true)
    {
        tick_in_loop.start();
        capture.read(frame);

        if (frame.empty())
        {
            break;
        }
        cv::Mat bgr;
        cv::cvtColor(frame, bgr, cv::COLOR_YUV2BGR_I420);

        pthread_mutex_lock(&lock);

        frame_queue.push(bgr);

        pthread_mutex_unlock (&lock);

        count++;
        tick_in_loop.stop();

    }
    capture_complete = true;
    pthread_join(displayThread, NULL);

    fps_captured = capture.get(cv::CAP_PROP_FPS);
    std::cout << "Display FPS "<< fps_captured << std::endl;

    if (tick_in_loop.getCounter() == 0)
    {
        std::cerr << "No frames processed" << std::endl;
        return_val = -10;
        goto cleanup;
    }

    if (fps_display)
    {
        double fps_calculated = tick_in_loop.getCounter() / tick_in_loop.getTimeSec();
        std::cout << "Captured Fps : " << fps_calculated << std::endl;
    }

cleanup:
    capture.release();
    pthread_mutex_destroy(&lock);
    return return_val;
}
