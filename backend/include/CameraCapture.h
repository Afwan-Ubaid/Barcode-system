#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

/**
 * Windows Media Foundation Native Real-Time Camera Streamer & Frame Grabber
 * Supports auto-detection of mobile phone cameras (DroidCam, Iriun, Camo, USB UVC)
 * with seamless fallback to integrated laptop webcams and instant camera switching.
 * Built by Intruders
 */

struct CameraDeviceInfo {
    int index;
    std::string name;
    bool isMobileOrExternal;
};

class CameraStream {
public:
    CameraStream();
    ~CameraStream();

    // Starts continuous background video streaming (30 FPS).
    // If deviceIndex == -1, auto-detects external/mobile camera first, falling back to laptop webcam.
    bool start(int deviceIndex = -1);

    // Stops video stream and releases camera hardware
    void stop();

    // Cycles to the next available camera or specific index
    bool switchCamera(int targetIndex = -1);

    // Returns whether stream is currently active
    bool isRunning() const;

    // Retrieves the latest video frame in RGBA format (ready for SFML texture update)
    bool getLatestFrame(std::vector<unsigned char>& outRgba, int& outWidth, int& outHeight);

    // Active camera hardware device name
    std::string getCameraName() const;

    // Active camera device index
    int getActiveDeviceIndex() const;

    // Whether active camera is a connected mobile phone / external scanner
    bool isActiveCameraExternal() const;

    // Enumerates all video devices currently connected to Windows
    static std::vector<CameraDeviceInfo> getAvailableCameras();

    // Returns the total count of connected camera devices
    static int getAvailableCameraCount();

    // Auto-detects best device index (prefers mobile/external camera, falls back to 0)
    static int getBestCameraIndex();

private:
    void workerThreadFunc(int deviceIndex);

    std::atomic<bool> mRunning;
    std::atomic<bool> mHasNewFrame;
    std::thread       mThread;
    std::mutex        mFrameMutex;

    std::vector<unsigned char> mRgbaBuffer;
    int mWidth;
    int mHeight;
    int mActiveDeviceIndex;
    std::string mCameraName;
};

// One-shot snapshot helper (captures a single frame to BMP file)
bool captureFrameFromCamera(const std::string& outBmpPath, std::string& cameraName, std::string& errorMessage, int deviceIndex = -1);

#endif // CAMERA_CAPTURE_H
