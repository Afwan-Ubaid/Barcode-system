#include "CameraCapture.h"
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <fstream>
#include <chrono>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

static bool writeBmp24(const std::string& path, const std::vector<unsigned char>& rgb, int width, int height) {
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;

    int rowPitch = (width * 3 + 3) & ~3;
    int imageSize = rowPitch * height;

    BITMAPFILEHEADER bfh = { 0 };
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + imageSize;

    BITMAPINFOHEADER bih = { 0 };
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height; // bottom-up
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = imageSize;

    f.write(reinterpret_cast<const char*>(&bfh), sizeof(bfh));
    f.write(reinterpret_cast<const char*>(&bih), sizeof(bih));

    std::vector<unsigned char> row(rowPitch, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            int srcIdx = (y * width + x) * 3;
            row[x * 3 + 0] = rgb[srcIdx + 2]; // B
            row[x * 3 + 1] = rgb[srcIdx + 1]; // G
            row[x * 3 + 2] = rgb[srcIdx + 0]; // R
        }
        f.write(reinterpret_cast<const char*>(row.data()), rowPitch);
    }
    return true;
}

static bool isInternalCameraName(const std::string& name) {
    std::string lower = name;
    for (char& c : lower) c = (char)tolower(c);
    if (lower.find("integrated") != std::string::npos) return true;
    if (lower.find("internal") != std::string::npos) return true;
    if (lower.find("true vision") != std::string::npos) return true;
    if (lower.find("truevision") != std::string::npos) return true;
    if (lower.find("facetime") != std::string::npos) return true;
    if (lower.find("easycamera") != std::string::npos) return true;
    return false;
}

static bool isLikelyExternalCamera(const std::string& name, int index, int count) {
    std::string lower = name;
    for (char& c : lower) c = (char)tolower(c);
    if (lower.find("droidcam") != std::string::npos) return true;
    if (lower.find("iriun") != std::string::npos) return true;
    if (lower.find("camo") != std::string::npos) return true;
    if (lower.find("phone") != std::string::npos) return true;
    if (lower.find("mobile") != std::string::npos) return true;
    if (lower.find("ivcam") != std::string::npos) return true;
    if (lower.find("epoccam") != std::string::npos) return true;
    if (lower.find("ip webcam") != std::string::npos) return true;
    if (lower.find("obs virtual") != std::string::npos) return true;
    if (lower.find("android") != std::string::npos) return true;
    if (lower.find("pixel") != std::string::npos) return true;
    if (lower.find("usb video") != std::string::npos) return true;
    if (count > 1 && index > 0 && !isInternalCameraName(name)) return true;
    return false;
}

std::vector<CameraDeviceInfo> CameraStream::getAvailableCameras() {
    std::vector<CameraDeviceInfo> result;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION);

    IMFAttributes* pConfig = nullptr;
    if (SUCCEEDED(MFCreateAttributes(&pConfig, 1))) {
        pConfig->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

        IMFActivate** ppDevices = nullptr;
        UINT32 count = 0;
        if (SUCCEEDED(MFEnumDeviceSources(pConfig, &ppDevices, &count)) && ppDevices) {
            for (UINT32 i = 0; i < count; ++i) {
                WCHAR* friendlyName = nullptr;
                UINT32 len = 0;
                ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendlyName, &len);
                std::string name = "Camera " + std::to_string(i);
                if (friendlyName) {
                    char nameBuf[256] = { 0 };
                    WideCharToMultiByte(CP_UTF8, 0, friendlyName, -1, nameBuf, sizeof(nameBuf) - 1, NULL, NULL);
                    name = nameBuf;
                    CoTaskMemFree(friendlyName);
                }

                bool isExt = isLikelyExternalCamera(name, (int)i, (int)count);
                result.push_back({ (int)i, name, isExt });
                ppDevices[i]->Release();
            }
            CoTaskMemFree(ppDevices);
        }
        pConfig->Release();
    }

    MFShutdown();
    CoUninitialize();
    return result;
}

int CameraStream::getAvailableCameraCount() {
    auto cams = getAvailableCameras();
    return (int)cams.size();
}

int CameraStream::getBestCameraIndex() {
    auto cams = getAvailableCameras();
    if (cams.empty()) return -1;

    // 1st Priority: Look for mobile phone camera software/driver or external scanner
    for (const auto& cam : cams) {
        if (cam.isMobileOrExternal) {
            return cam.index;
        }
    }

    // 2nd Priority: If multiple cameras exist and secondary camera is not internal, pick device 1
    if (cams.size() > 1 && !isInternalCameraName(cams[1].name)) {
        return 1;
    }

    // Fallback: Device 0 (Laptop integrated webcam)
    return 0;
}

CameraStream::CameraStream()
    : mRunning(false), mHasNewFrame(false), mWidth(640), mHeight(480), mActiveDeviceIndex(0), mCameraName("Camera") {
}

CameraStream::~CameraStream() {
    stop();
}

bool CameraStream::start(int deviceIndex) {
    if (mRunning) {
        if (deviceIndex == mActiveDeviceIndex) return true;
        stop();
    }

    if (deviceIndex < 0) {
        deviceIndex = getBestCameraIndex();
        if (deviceIndex < 0) deviceIndex = 0;
    }

    mActiveDeviceIndex = deviceIndex;
    mRunning = true;
    mHasNewFrame = false;
    mThread = std::thread(&CameraStream::workerThreadFunc, this, deviceIndex);
    return true;
}

bool CameraStream::switchCamera(int targetIndex) {
    auto cams = getAvailableCameras();
    if (cams.empty()) return false;

    int nextIdx = 0;
    if (targetIndex >= 0) {
        nextIdx = targetIndex % (int)cams.size();
    } else {
        nextIdx = (mActiveDeviceIndex + 1) % (int)cams.size();
    }

    stop();
    return start(nextIdx);
}

int CameraStream::getActiveDeviceIndex() const {
    return mActiveDeviceIndex;
}

bool CameraStream::isActiveCameraExternal() const {
    std::string name = getCameraName();
    int count = getAvailableCameraCount();
    return isLikelyExternalCamera(name, mActiveDeviceIndex, count);
}

void CameraStream::stop() {
    if (!mRunning) return;
    mRunning = false;
    if (mThread.joinable()) {
        mThread.join();
    }
}

bool CameraStream::isRunning() const {
    return mRunning;
}

std::string CameraStream::getCameraName() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mFrameMutex));
    return mCameraName;
}

bool CameraStream::getLatestFrame(std::vector<unsigned char>& outRgba, int& outWidth, int& outHeight) {
    if (!mHasNewFrame) return false;

    std::lock_guard<std::mutex> lock(mFrameMutex);
    outRgba = mRgbaBuffer;
    outWidth = mWidth;
    outHeight = mHeight;
    mHasNewFrame = false;
    return !outRgba.empty();
}

void CameraStream::workerThreadFunc(int deviceIndex) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION);

    IMFAttributes* pConfig = nullptr;
    MFCreateAttributes(&pConfig, 1);
    pConfig->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;
    MFEnumDeviceSources(pConfig, &ppDevices, &count);
    pConfig->Release();

    if (count == 0) {
        MFShutdown();
        CoUninitialize();
        mRunning = false;
        return;
    }

    UINT32 targetIdx = (deviceIndex >= 0 && (UINT32)deviceIndex < count) ? (UINT32)deviceIndex : 0;

    // Retrieve camera name
    WCHAR* friendlyName = nullptr;
    UINT32 len = 0;
    ppDevices[targetIdx]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendlyName, &len);
    if (friendlyName) {
        char nameBuf[256] = { 0 };
        WideCharToMultiByte(CP_UTF8, 0, friendlyName, -1, nameBuf, sizeof(nameBuf) - 1, NULL, NULL);
        std::lock_guard<std::mutex> lock(mFrameMutex);
        mCameraName = nameBuf;
        CoTaskMemFree(friendlyName);
    }

    IMFMediaSource* pSource = nullptr;
    HRESULT hr = ppDevices[targetIdx]->ActivateObject(IID_PPV_ARGS(&pSource));
    if (FAILED(hr) || !pSource) {
        for (UINT32 i = 0; i < count; ++i) ppDevices[i]->Release();
        CoTaskMemFree(ppDevices);
        MFShutdown();
        CoUninitialize();
        mRunning = false;
        return;
    }

    // Enable color space converter
    IMFAttributes* pAttrs = nullptr;
    MFCreateAttributes(&pAttrs, 1);
    pAttrs->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

    IMFSourceReader* pReader = nullptr;
    hr = MFCreateSourceReaderFromMediaSource(pSource, pAttrs, &pReader);
    pAttrs->Release();

    if (FAILED(hr) || !pReader) {
        pSource->Release();
        for (UINT32 i = 0; i < count; ++i) ppDevices[i]->Release();
        CoTaskMemFree(ppDevices);
        MFShutdown();
        CoUninitialize();
        mRunning = false;
        return;
    }

    // Configure stream to RGB32
    IMFMediaType* pType = nullptr;
    MFCreateMediaType(&pType);
    pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    hr = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
    pType->Release();

    UINT32 width = 640, height = 480;
    IMFMediaType* pCurrent = nullptr;
    if (SUCCEEDED(pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrent)) && pCurrent) {
        MFGetAttributeSize(pCurrent, MF_MT_FRAME_SIZE, &width, &height);
        pCurrent->Release();
    }

    {
        std::lock_guard<std::mutex> lock(mFrameMutex);
        mWidth = width;
        mHeight = height;
        mRgbaBuffer.resize(width * height * 4, 255);
    }

    // Continuous capture loop
    while (mRunning) {
        DWORD streamIndex = 0, flags = 0;
        LONGLONG llTimeStamp = 0;
        IMFSample* pSample = nullptr;

        hr = pReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIndex, &flags, &llTimeStamp, &pSample);
        if (SUCCEEDED(hr) && pSample) {
            IMFMediaBuffer* pBuffer = nullptr;
            if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&pBuffer)) && pBuffer) {
                BYTE* pData = nullptr;
                DWORD maxLen = 0, curLen = 0;
                if (SUCCEEDED(pBuffer->Lock(&pData, &maxLen, &curLen)) && pData) {
                    if (curLen >= width * height * 4) {
                        std::lock_guard<std::mutex> lock(mFrameMutex);
                        if (mRgbaBuffer.size() < width * height * 4) {
                            mRgbaBuffer.resize(width * height * 4, 255);
                        }
                        // Convert RGB32 (BGRX) to RGBA for SFML
                        unsigned char* dst = mRgbaBuffer.data();
                        for (size_t i = 0; i < width * height; ++i) {
                            size_t srcIdx = i * 4;
                            size_t dstIdx = i * 4;
                            dst[dstIdx + 0] = pData[srcIdx + 2]; // R
                            dst[dstIdx + 1] = pData[srcIdx + 1]; // G
                            dst[dstIdx + 2] = pData[srcIdx + 0]; // B
                            dst[dstIdx + 3] = 255;              // A
                        }
                        mHasNewFrame = true;
                    }
                    pBuffer->Unlock();
                }
                pBuffer->Release();
            }
            pSample->Release();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    pReader->Release();
    pSource->Release();

    for (UINT32 i = 0; i < count; ++i) ppDevices[i]->Release();
    CoTaskMemFree(ppDevices);

    MFShutdown();
    CoUninitialize();
}

bool captureFrameFromCamera(const std::string& outBmpPath, std::string& cameraName, std::string& errorMessage, int deviceIndex) {
    CameraStream stream;
    if (!stream.start(deviceIndex)) {
        errorMessage = "Could not initialize camera stream.";
        return false;
    }

    // Let camera settle and capture a frame
    std::vector<unsigned char> rgba;
    int w = 0, h = 0;
    bool gotFrame = false;

    for (int i = 0; i < 30; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (stream.getLatestFrame(rgba, w, h) && !rgba.empty()) {
            gotFrame = true;
            break;
        }
    }
    cameraName = stream.getCameraName();
    stream.stop();

    if (!gotFrame || rgba.empty()) {
        errorMessage = "No frame received from camera.";
        return false;
    }

    // Convert RGBA to RGB24 for BMP file
    std::vector<unsigned char> rgb(w * h * 3);
    for (size_t i = 0; i < (size_t)(w * h); ++i) {
        rgb[i * 3 + 0] = rgba[i * 4 + 0];
        rgb[i * 3 + 1] = rgba[i * 4 + 1];
        rgb[i * 3 + 2] = rgba[i * 4 + 2];
    }

    if (!writeBmp24(outBmpPath, rgb, w, h)) {
        errorMessage = "Failed to save camera frame to BMP file.";
        return false;
    }

    return true;
}
