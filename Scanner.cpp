// Minimal Windows serial reader for a DS9208 (RS-232 mode).
// Build (MSVC):  cl /EHsc scanner_read.cpp
// Build (MinGW): g++ scanner_read.cpp -o scanner_read.exe
// Usage: scanner_read.exe COM3


// Need window.h library for serail port communication
#include <windows.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    std::string port = (argc > 1) ? argv[1] : "COM3";

    // The \\.\ prefix is required for COM10 and above, harmless for lower ports.
    std::string path = "\\\\.\\" + port;

    // Windows treats serial ports like files
    // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
    // CreateFileA() -> Creates or opens a file or I/O device
    // 
    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                           0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        std::cerr << "Could not open " << port
                  << " (error " << GetLastError() << ")\n";
        return 1;
    }

    // Match the scanner's serial settings (DS9208 default: 9600 8N1, no flow control).
    DCB dcb = {};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) {
        std::cerr << "GetCommState failed\n";
        CloseHandle(h);
        return 1;
    }
    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl  = DTR_CONTROL_ENABLE;
    dcb.fRtsControl  = RTS_CONTROL_ENABLE;
    if (!SetCommState(h, &dcb)) {
        std::cerr << "SetCommState failed\n";
        CloseHandle(h);
        return 1;
    }

    // Return from ReadFile after 100 ms of no data instead of blocking forever.
    // https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-commtimeouts
    // COMMTIMEOUTS -> Contains the time-out parameters for a communications device. The parameters 
    // determine the behavior of ReadFile, WriteFile, ReadFileEx, and WriteFileEx operations on the device.
    COMMTIMEOUTS to = {};
    to.ReadIntervalTimeout        = 50;
    to.ReadTotalTimeoutConstant   = 100;
    to.ReadTotalTimeoutMultiplier = 0;
    // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcommtimeouts
    SetCommTimeouts(h, &to);

    std::cout << "Listening on " << port << " - scan a barcode (Ctrl+C to quit)\n";

    std::string scan;
    char buf[256];
    // https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/262627d8-3418-4627-9218-4ffe110850b2
    // DWORD -> 32-bit unsigned integer (range: 0 through 4294967295 decimal)
    DWORD n = 0;

    while (true) {
        // ReadFile() asks port for input, if so shoves it into buf
        if (!ReadFile(h, buf, sizeof(buf), &n, nullptr)) {
            std::cerr << "ReadFile failed (error " << GetLastError() << ")\n";
            break;
        }
        for (DWORD i = 0; i < n; ++i) {
            char c = buf[i];
            
            if (c == '\r' || c == '\n') {      // scanner terminates each scan with CR
                if (!scan.empty()) {
                    std::cout << "Scanned: " << scan << "\n";
                    scan.clear();
                }
            } else {
                // printf("%02X ", (unsigned char)c);
                scan += c;
            }
        }
    }

    CloseHandle(h);
    return 0;
}