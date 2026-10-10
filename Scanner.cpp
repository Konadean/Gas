// Minimal Windows serial reader for a DS9208 (RS-232 mode).
// Build (MinGW): g++ -std=c++17 Store.cpp Scanner.cpp -o store.exe
// Build (MSVC):  cl /EHsc /std:c++17 Store.cpp Scanner.cpp

#include "Scanner.h"
#include <iostream>

bool Scanner::start()
{
    // The \\.\ prefix is required for COM10 and above, harmless for lower ports.
    std::string path = "\\\\.\\" + port_;

    // Windows treats serial ports like files
    // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
    // CreateFileA() -> Creates or opens a file or I/O device
    h_ = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                     0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h_ == INVALID_HANDLE_VALUE) {
        std::cerr << "Could not open " << port_
                  << " (error " << GetLastError() << ")\n";
        return false;
    }

    // Match the scanner's serial settings (DS9208 default: 9600 8N1, no flow control).
    DCB dcb = {};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h_, &dcb)) {
        std::cerr << "GetCommState failed\n";
        stop();
        return false;
    }
    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl  = DTR_CONTROL_ENABLE;
    dcb.fRtsControl  = RTS_CONTROL_ENABLE;
    if (!SetCommState(h_, &dcb)) {
        std::cerr << "SetCommState failed\n";
        stop();
        return false;
    }

    // Return from ReadFile after 100 ms of no data instead of blocking forever.
    // This also lets the reader thread notice running_ == false and exit on stop().
    // https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-commtimeouts
    // COMMTIMEOUTS -> Contains the time-out parameters for a communications device. The parameters
    // determine the behavior of ReadFile, WriteFile, ReadFileEx, and WriteFileEx operations on the device.
    COMMTIMEOUTS to = {};
    to.ReadIntervalTimeout        = 50;
    to.ReadTotalTimeoutConstant   = 100;
    to.ReadTotalTimeoutMultiplier = 0;
    // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcommtimeouts
    SetCommTimeouts(h_, &to);

    running_ = true;
    worker_ = std::thread(&Scanner::readLoop, this);
    return true;
}

void Scanner::stop()
{
    running_ = false;
    if (worker_.joinable()) worker_.join();
    if (h_ != INVALID_HANDLE_VALUE) {
        CloseHandle(h_);
        h_ = INVALID_HANDLE_VALUE;
    }
}

std::optional<std::string> Scanner::tryGetScan()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (scans_.empty()) return std::nullopt;
    std::string s = std::move(scans_.front());
    scans_.pop();
    return s;
}

// Runs on the background thread
void Scanner::readLoop()
{
    std::string scan;
    bool atStart = true;    // first byte of each scan is the barcode type
    char buf[256];
    // https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/262627d8-3418-4627-9218-4ffe110850b2
    // DWORD -> 32-bit unsigned integer (range: 0 through 4294967295 decimal)
    DWORD n = 0;

    // Monitoring loop for input
    while (running_) {
        // ReadFile() asks port for input, if so shoves it into buf
        if (!ReadFile(h_, buf, sizeof(buf), &n, nullptr)) {
            std::cerr << "ReadFile failed (error " << GetLastError() << ")\n";
            break;
        }
        // Iterate byte by byte through the buffer, appending to scan until a terminating character is reached.
        // A scan can be split across ReadFile calls, so the type byte is skipped per scan, not per read.
        for (DWORD i = 0; i < n; ++i) {
            char c = buf[i];

            if (c == '\r' || c == '\n') {      // scanner terminates each scan with CR
                if (!scan.empty()) {
                    std::lock_guard<std::mutex> lock(mtx_);
                    scans_.push(std::move(scan));
                    scan.clear();
                }
                atStart = true;
            } else if (atStart) {
                atStart = false;               // drop the barcode type byte
            } else {
                scan += c;
            }
        }
    }
    running_ = false;
}
