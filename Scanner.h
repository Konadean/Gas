// Background serial reader for a DS9208 (RS-232 mode).
// start() opens the port and launches a reader thread that queues each completed scan;
// the main loop pulls scans with tryGetScan() without ever blocking on the port.
#pragma once

// Need window.h library for serial port communication
#include <windows.h>
#include <atomic>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

class Scanner {
public:
    explicit Scanner(std::string port = "COM3") : port_(std::move(port)) {}
    ~Scanner() { stop(); }

    // Owns a HANDLE and a thread, so copying makes no sense
    Scanner(const Scanner&) = delete;
    Scanner& operator=(const Scanner&) = delete;

    bool start();                               // open port, launch reader thread
    void stop();                                // stop thread, close port
    std::optional<std::string> tryGetScan();    // non-blocking; empty if no scan waiting

private:
    void readLoop();

    std::string port_;
    HANDLE h_ = INVALID_HANDLE_VALUE;
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::mutex mtx_;                            // guards scans_
    std::queue<std::string> scans_;
};
