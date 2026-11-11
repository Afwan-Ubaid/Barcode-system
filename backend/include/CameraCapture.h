#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

/**
 * Windows Media Foundation Native Real-Time Camera Streamer & Frame Grabber
 * Streams 30 FPS video continuously directly from laptop webcams & USB cameras.
 * Built by Intruders
 */

class CameraStream {
public:
    CameraStream();
    ~CameraStream();

    // Starts continuous background video streaming (30 FPS)
    bool start(int deviceIndex = 0);

    // Stops video stream and releases camera hardware
    void stop();

    // Returns whether stream is currently active
    bool isRunning() const;

    // Retrieves the latest video frame in RGBA format (ready for SFML texture update)
    bool getLatestFrame(std::vector<unsigned char>& outRgba, int& outWidth, int& outHeight);

    // Active camera hardware device name
    std::string getCameraName() const;

private:
    void workerThreadFunc(int deviceIndex);

    std::atomic<bool> mRunning;
    std::atomic<bool> mHasNewFrame;
    std::thread       mThread;
    std::mutex        mFrameMutex;

    std::vector<unsigned char> mRgbaBuffer;
    int mWidth;
    int mHeight;
    std::string mCameraName;
};

// One-shot snapshot helper (captures a single frame to BMP file)
bool captureFrameFromCamera(const std::string& outBmpPath, std::string& cameraName, std::string& errorMessage);

#endif // CAMERA_CAPTURE_H
