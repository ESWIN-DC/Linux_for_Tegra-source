/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
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
#include <chrono>
#include <opencv2/core/core.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

static std::string create_capture (int width, int height, int fps);

static void help()
{
    std::cout << "\nThis is a sample OpenCV application to demonstrate "
    "CSI-camera capture pipeline for NVIDIA accelerated GStreamer.\n\n"
    "./opencv_nvgstcam [--Options]\n\n"
    "OPTIONS:\n"
    "\t-h,--help            Prints this message\n"
    "\t--width              Capture width [Default = 1280]\n"
    "\t--height             Capture height [Default = 720]\n"
    "\t--fps                Frames per second [Default = 30]\n"
    "\tq                    Runtime command to stop capture\n\n"
    << std::endl;

}

static std::string create_capture (int width, int height, int fps)
{
    std::stringstream pipeline_str;
    pipeline_str << "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=(int)"
        << std::to_string(width) << ", height=(int)" << std::to_string(height)
        << ", format=(string)NV12, framerate=(fraction)" << std::to_string(fps)
        << "/1 ! nvvidconv ! video/x-raw, format=(string)I420 ! videoconvert"
        " ! video/x-raw, format=(string)BGR ! appsink ";

    return pipeline_str.str();
}

int main(int argc, char const *argv[])
{
    unsigned int fps;
    int width;
    int height;
    int return_val = 0;
    double fps_calculated;
    cv::VideoCapture capture;
    cv::Mat frame;
    cv::TickMeter ticks;

    const std::string keys =
    "{h help         |     | message }"
    "{width          |1280 | width }"
    "{height         |720  | height }"
    "{fps            |30   | frame per second }"
    ;

    cv::CommandLineParser cmd_parser(argc, argv, keys);

    if (cmd_parser.has("help"))
    {
        help();
        goto cleanup;
    }

    fps = cmd_parser.get<unsigned int>("fps");
    width = cmd_parser.get<int>("width");
    height = cmd_parser.get<int>("height");

    if (!cmd_parser.check())
    {
        cmd_parser.printErrors();
        help();
        return_val = -1;
        goto cleanup;
    }

    capture.open(create_capture(width, height, fps), cv::CAP_GSTREAMER);

    if (!capture.isOpened())
    {
        std::cerr << "Failed to open VideoCapture" << std::endl;
        return_val = -4;
        goto cleanup;
    }

    while (true)
    {
        ticks.start();
        capture >> frame;

        // computations
        cv::imshow("Capture Window", frame);
        int key = cv::waitKey(1);

        // 'q' for termination
        if (key == 'q' )
        {
            break;
        }
        ticks.stop();
    }

    if (ticks.getCounter() == 0)
    {
        std::cerr << "No frames processed" << std::endl;
        return_val = -10;
        goto cleanup;
    }

    fps_calculated = ticks.getCounter() / ticks.getTimeSec();
    std::cout << "Fps observed " << fps_calculated << std::endl;

cleanup:
    capture.release();
    return return_val;
}
