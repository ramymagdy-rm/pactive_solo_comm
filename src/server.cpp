#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <atomic>
#include <queue>
#include <windows.h>
#include <setupapi.h>
#include <vector>
#include <conio.h>
#include <third_party/nlohmann/json.hpp> // JSON library
#include "SOLOMotorControllersSerial.h"

// Link with SetupAPI.lib for MSV C++
//#pragma comment(lib, "SetupAPI.lib")

#ifndef MAX_DEVICE_ID_LEN
#define MAX_DEVICE_ID_LEN 200
#endif

/*
**------------------------------------------------------------------------------
** namespace usage
**------------------------------------------------------------------------------
*/
using json = nlohmann::json;

/*
**------------------------------------------------------------------------------
** typedefs
**------------------------------------------------------------------------------
*/
typedef struct {
    bool connected;
} MOTION_REL_t;

struct MOTION_ENGINE {
    std::atomic<bool> running;
    std::queue<json> commands_queue;
    std::queue<json> feedback_queue;
    std::queue<int16_t> setpoints_queue;

    // Constructor to initialize stop_flag
    MOTION_ENGINE() : running(true) {}
};

/*
**------------------------------------------------------------------------------
** Public function prototypes
**------------------------------------------------------------------------------
*/
void keyboard_listener(MOTION_ENGINE& motion_engine);
void solo_init(char* comm_port, MOTION_ENGINE& motion_engine);
void solo_deinit(MOTION_ENGINE& motion_engine);
void transmit_test(MOTION_ENGINE& motion_engine);

std::vector<std::string> enumerate_usb_devices();
std::string get_user_selected_device(const std::vector<std::string>& device_list);

/*
**------------------------------------------------------------------------------
** Variable declaration
**------------------------------------------------------------------------------
*/
SOLOMotorControllers *solo; 

/*
**------------------------------------------------------------------------------------
** Public function definitions
**------------------------------------------------------------------------------------
*/

// Function to listen for keyboard input
void keyboard_listener(MOTION_ENGINE& motion_engine) {
    std::string input;

    while (motion_engine.running.load()) {

        std::getline(std::cin, input);

        if (input == "q" || input == "Q") {
            motion_engine.running.store(false);
            std::cout << "Exit command received. Shutting down the server..." << std::endl;
            break;

        } else if (input == "queue") {

            if (motion_engine.commands_queue.empty()) {

                std::cout << "Commands queue is empty" << std::endl;
            } else {

                std::cout << "Current commands queue contents:" << std::endl;
                std::queue<json> temp_queue = motion_engine.commands_queue;

                while (!temp_queue.empty()) {
                    std::cout << temp_queue.front().dump() << std::endl;
                    temp_queue.pop();
                }
            }

        } else if (input == "setpoints") {

            if (motion_engine.setpoints_queue.empty()) {

                std::cout << "Setpoints queue is empty" << std::endl;
            } else {

                std::cout << "Current setpoints queue contents:" << std::endl;
                std::queue<int16_t> temp_queue = motion_engine.setpoints_queue;

                while (!temp_queue.empty()) {
                    std::cout << temp_queue.front() << std::endl;
                    temp_queue.pop();
                }
            }

        } else {
            std::cout << "Undefined command: " << input << std::endl;
        }
    }
}

void solo_init(char* comm_port, MOTION_ENGINE& motion_engine) {

    std::cout << "trying to connect to " << comm_port << std::endl;
    solo = new SOLOMotorControllersSerial(comm_port);

    //TRY CONNECT LOOP
    while(solo->CommunicationIsWorking() == false && motion_engine.running.load()) {
        std::cout << "Solo connection failed. Retry" << std::endl;
        Sleep(500);
        solo->Connect();
    }
    std::cout << "Solo connected!" << std::endl;
}

void solo_deinit(MOTION_ENGINE& motion_engine) {

    //TRY DISCONNECT LOOP
    while(solo->CommunicationIsWorking() == true && motion_engine.running.load()) {
        std::cout << "Solo connection going. Retry" << std::endl;
        Sleep(500);
        solo->Disconnect();
    }
    std::cout << "Solo disconnected!" << std::endl;
}


void transmit_test(MOTION_ENGINE& motion_engine) {
    long desiredPositionReference = 0;
    long actualMotorPosition = 0;

    if (motion_engine.running.load()) {

        actualMotorPosition = solo->GetPositionCountsFeedback();
        std::cout << "Number of Pulse passed: "<< actualMotorPosition << std::endl;

        desiredPositionReference = -1000;
        solo->SetPositionReference(desiredPositionReference);

        // wait till motor reaches to the reference 
        Sleep(2000);

        actualMotorPosition = solo->GetPositionCountsFeedback();
        std::cout << "Number of Pulses passed: "<< actualMotorPosition << std::endl;
        
        desiredPositionReference = 0;
        solo->SetPositionReference(desiredPositionReference);

        Sleep(2000);
    
        actualMotorPosition = solo->GetPositionCountsFeedback();
        std::cout << "Number of Pulses passed: "<< actualMotorPosition << std::endl;
    }
}

std::vector<std::string> enumerate_usb_devices() {
    std::vector<std::string> com_ports;
    HDEVINFO h_dev_info;
    SP_DEVINFO_DATA device_info_data;
    DWORD i;

    // Create a HDEVINFO with all present devices.
    h_dev_info = SetupDiGetClassDevs(NULL, 0, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (h_dev_info == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get device information set." << std::endl;
        return com_ports;
    }

    // Enumerate through all devices in Set.
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i = 0; SetupDiEnumDeviceInfo(h_dev_info, i, &device_info_data); i++) {
        DWORD DataT;
        LPTSTR buffer = NULL;
        DWORD buffersize = 0;

        while (!SetupDiGetDeviceRegistryProperty(
            h_dev_info,
            &device_info_data,
            SPDRP_FRIENDLYNAME,
            &DataT,
            (PBYTE)buffer,
            buffersize,
            &buffersize)) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                // Change the buffer size.
                if (buffer) LocalFree(buffer);
                buffer = (LPTSTR)LocalAlloc(LPTR, buffersize);
            } else {
                break;
            }
        }

        if (buffer) {
            std::string deviceName = buffer;
            // Look for "COM" in the device name
            if (deviceName.find("COM") != std::string::npos) {
                com_ports.push_back(deviceName);
            }
            LocalFree(buffer);
        }
    }

    if (GetLastError() != NO_ERROR && GetLastError() != ERROR_NO_MORE_ITEMS) {
        std::cerr << "Error in enumeration." << std::endl;
    }

    // Cleanup
    SetupDiDestroyDeviceInfoList(h_dev_info);

    return com_ports;
}

std::string get_user_selected_device(const std::vector<std::string>& device_list) {

    if (device_list.empty()) {
        std::cerr << "No USB devices found." << std::endl;
        return "";
    }

    std::cout << "Detected USB Devices:" << std::endl;
    for (size_t i = 0; i < device_list.size(); ++i) {
        std::cout << i + 1 << ": " << device_list[i] << std::endl;
    }

    std::cout << "Select a device (1-" << device_list.size() << "): ";
    size_t choice;
    std::cin >> choice;

    //need to handle invalid input
    if (choice < 1 || choice > device_list.size()) {
        std::cerr << "Invalid choice." << std::endl;
        return "";
    }

    return device_list[choice - 1];
}


int main() {

    // initializations
    MOTION_ENGINE motion_engine;

    char comm_port[25];

    // Enumerate USB devices
    std::vector<std::string> device_list = enumerate_usb_devices();

    // Display the COM ports
    if (device_list.empty()) {
        std::cout << "No COM ports found." << std::endl;
    } else {
        std::cout << "Available COM ports:" << std::endl;

        for (const auto& port : device_list) {
            std::cout << port << std::endl;
        }
        
        // Let the user select a device
        std::string selected_device = get_user_selected_device(device_list);

        if (!selected_device.empty()) {
            std::cout << "You selected: " << selected_device << std::endl;
            
            strcpy(comm_port, selected_device.c_str());
            
            solo_init(comm_port, motion_engine);
            transmit_test(motion_engine);
            solo_deinit(motion_engine);
        }
    }

    // Create necessary threads
    std::thread keyboard_listener_thread(keyboard_listener, std::ref(motion_engine));

    // Wait for the running threads to finish
    keyboard_listener_thread.join();

    if (solo != nullptr) {
        delete solo;
        solo = nullptr;
    }

    std::cout << "Exiting the server program." << std::endl;

    return 0;
}
