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
#include <opencv2/core/core.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

static void help()
{
    std::cout << "\nThis is a sample OpenCV application to demonstrate usage "
    "of NVIDIA accelerated GStreamer to encode CSI camera capture in an H264 "
    "mp4 container file.\n\n"
    "./opencv_nvgstenc [--Options]\n\n"
    "OPTIONS:\n"
    "\t-h,--help            Prints this message\n"
    "\t--width              Capture width [Default = 1280]\n"
    "\t--height             Capture height [Default = 720]\n"
    "\t--fps                Frames per second [Default = 30]\n"
    "\t--filename           Target H264 encoded mp4 filestream "
                            "[Default = test_camera_h264.mp4]\n"
    "\t--time               Duration for capture in seconds [Default = 10]\n\n"
    << std::endl;
}

static std::string create_cap_pipeline (int width, int height, int fps,
    unsigned int num_buffers)
{
    std::stringstream cap_pipeline_str;
    cap_pipeline_str << "nvarguscamerasrc num-buffers="
        << std::to_string(num_buffers) << " ! video/x-raw(memory:NVMM), "
        "width=(int)" << std::to_string(width) << ", height=(int)"
        << std::to_string(height) << ", format=(string)NV12, "
        "framerate=(fraction)" << std::to_string(fps) << "/1 ! nvvidconv ! "
        "video/x-raw, format=(string)I420 ! videoconvert ! video/x-raw, "
        "format=(string)BGR ! appsink";

    return cap_pipeline_str.str();
}

static std::string create_out_pipeline (std::string filename)
{
    std::stringstream out_pipeline_str;
    out_pipeline_str << "appsrc ! video/x-raw, format=(string)BGR ! "
        "videoconvert ! video/x-raw, format=(string)I420 ! nvvidconv ! "
        "video/x-raw(memory:NVMM), format=(string)NV12 ! nvv4l2h264enc ! "
        "h264parse ! qtmux ! filesink location="
        << filename << " ";

    return out_pipeline_str.str();
}

int main(int argc, char const *argv[])
{
    unsigned int fps;
    unsigned int time;
    unsigned int num_buffers;
    int width;
    int height;
    int return_val = 0;
    double fps_calculated;
    std::string filename;
    std::string cap_pipeline;
    std::string out_pipeline;
    cv::VideoCapture capture;
    cv::VideoWriter output;
    cv::Mat frame;
    cv::TickMeter ticks;

    const std::string keys =
    "{h help usage ?    |                       | message   }"
    "{width             |1280                   | width }"
    "{height            |720                    | height }"
    "{fps               |30                     | fps (supported: fps > 0) }"
    "{filename          |test_camera_h264.mp4   | h264 encoded filename }"
    "{time              |10                     | capture time in seconds }"
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
    filename = cmd_parser.get<std::string>("filename");
    time = cmd_parser.get<unsigned int>("time");
    if (!cmd_parser.check())
    {
        cmd_parser.printErrors();
        help();
        return_val = -1;
        goto cleanup;
    }

    num_buffers = fps * time;
    cap_pipeline = create_cap_pipeline(width, height, fps, num_buffers);
    out_pipeline = create_out_pipeline(filename);
    capture.open(cap_pipeline, cv::CAP_GSTREAMER);
    output.open(out_pipeline, cv::CAP_GSTREAMER, 0, capture.get(cv::CAP_PROP_FPS),
        cv::Size(capture.get(cv::CAP_PROP_FRAME_WIDTH),
        capture.get(cv::CAP_PROP_FRAME_HEIGHT)));

    if (!capture.isOpened() || !output.isOpened())
    {
        std::cerr << "Failed to open VideoCapture/ VideoWriter" << std::endl;
        return_val = -4;
        goto cleanup;
    }

    while (true)
    {
        ticks.start();
        capture >> frame;
        if (frame.empty())
        {
            break;
        }

        // computations

        output.write(frame);
        cv::imshow("Capture Window", frame);
        if (cv::waitKey(1) == 'q')
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
    std::cout << "Fps observed: " << fps_calculated << std::endl;

cleanup:
    capture.release();
    output.release();
    return return_val;
}
