// Copyright (c) 2021 Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <tuple>
#include <windows.h>

namespace
{
    const char *GetChangeDisplaySettingsErrorMessage(int errorCode)
    {
        switch (errorCode) {
        case DISP_CHANGE_SUCCESSFUL:
            return "The settings change was successful";
        case DISP_CHANGE_BADDUALVIEW:
            return "The settings change was unsuccessful because the system is DualView capable";
        case DISP_CHANGE_BADFLAGS:
            return "An invalid set of flags was passed in";
        case DISP_CHANGE_BADMODE:
            return "The graphics mode is not supported";
        case DISP_CHANGE_BADPARAM:
            return "An invalid parameter was passed in";
        case DISP_CHANGE_FAILED:
            return "The display driver failed the specified graphics mode";
        case DISP_CHANGE_NOTUPDATED:
            return "Unable to write settings to the registry";
        case DISP_CHANGE_RESTART:
            return "The computer must be restarted for the graphics mode to work";
        }
        return "Unknown error";
    }

    void PrintUsage(const char *programName)
    {
        std::cerr << "Usage:\n\n"
            << programName << " list <display>\n"
            << "\tPrint a list of available display modes for the specified display\n"
            << programName << " set <display> <mode>\n"
            << "\tChange display mode\n\n"
            << "Examples:\n\n"
            << programName << " set 0 1920x1080\n"
            << "\tChange resolution of display 0 (first display) to 1920 (width) by 1080 (height) pixels\n"
            << programName << " set 0 1920x1080x32\n"
            << "\tChange resolution of display 0 to 1920x1080 with 32-bit colors\n"
            << programName << " set 0 1920x1080@60\n"
            << "\tSet both resolution and refresh rate (60 Hz)\n"
            << programName << " set 0 @144\n"
            << "\tChange refresh rate to 144 Hz keeping the same resolution and color depth\n"
            << std::endl;
    }

    bool FindDisplay(int displayIndex, DISPLAY_DEVICE &device)
    {
        for (auto iDevNum = 0; ; iDevNum++) {
            ZeroMemory(&device, sizeof(device));
            device.cb = sizeof(device);

            auto result = EnumDisplayDevices(
                nullptr,
                iDevNum,
                &device,
                DISPLAY_DEVICE_ATTACHED_TO_DESKTOP);
            if (!result) {
                break;
            }
            if (iDevNum == displayIndex) {
                return true;
            }
        }

        return false;
    }

    bool FindMonitor(const char *displayDeviceName, int monitorIndex, DISPLAY_DEVICE &monitorDevice)
    {
        ZeroMemory(&monitorDevice, sizeof(monitorDevice));
        monitorDevice.cb = sizeof(monitorDevice);

        return EnumDisplayDevices(
            displayDeviceName,
            monitorIndex,
            &monitorDevice,
            DISPLAY_DEVICE_ATTACHED_TO_DESKTOP);
    }

    int ListDisplayModes(int displayIndex)
    {
        using DisplayMode = std::tuple<int, int, int, int>;

        DISPLAY_DEVICE displayDevice;

        if (!FindDisplay(displayIndex, displayDevice)) {
            std::cerr << "Display not found: " << displayIndex << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Display:\n\t" << displayDevice.DeviceString;

        DISPLAY_DEVICE monitorDevice;

        if (FindMonitor(displayDevice.DeviceName, 0, monitorDevice)) {
            std::cout << " - " << monitorDevice.DeviceString;
        }

        std::set<DisplayMode> displayModes;

        for (auto iModeNum = 0; ; iModeNum++) {
            DEVMODE mode = { 0 };
            mode.dmSize = sizeof(mode);

            auto result = EnumDisplaySettings(displayDevice.DeviceName, iModeNum, &mode);
            if (!result) {
                break;
            }

            displayModes.insert(DisplayMode{
                mode.dmPelsWidth,
                mode.dmPelsHeight,
                mode.dmBitsPerPel,
                mode.dmDisplayFrequency
            });
        }

        std::cout << "\n\nAvailable modes:" << std::endl;

        for (const auto &mode : displayModes) {
            std::cout
                << '\t'
                << std::get<0>(mode) << "x" << std::get<1>(mode) << "x" << std::get<2>(mode)
                << "@" << std::get<3>(mode)
                << "\n";
        }

        std::cout << std::endl;

        return EXIT_SUCCESS;
    }

    int SetDisplayMode(int displayIndex, const std::string &modeString)
    {
        DISPLAY_DEVICE displayDevice;

        if (!FindDisplay(displayIndex, displayDevice)) {
            std::cerr << "Display not found: " << displayIndex << std::endl;
            return EXIT_FAILURE;
        }

        DEVMODE mode = { 0 };
        mode.dmSize = sizeof(mode);

        if (!EnumDisplaySettings(displayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &mode)) {
            std::cerr << "Error: Failed to obtain current monitor settings" << std::endl;
            return EXIT_FAILURE;
        }

        const auto regexFlags = std::regex_constants::icase;
        const auto modeRegex = std::regex("(\\d+)x(\\d+)(?:x(\\d+))?(?:@(\\d+))?", regexFlags);
        const auto refreshRateRegex = std::regex("@(\\d+)", regexFlags);

        std::smatch match;
        if (std::regex_match(modeString, match, modeRegex)) {
            mode.dmFields |= DM_PELSWIDTH;
            mode.dmPelsWidth = std::atoi(match[1].str().c_str());
            mode.dmFields |= DM_PELSHEIGHT;
            mode.dmPelsHeight = std::atoi(match[2].str().c_str());;
            if (match[3].matched) {
                mode.dmFields = DM_BITSPERPEL;
                mode.dmBitsPerPel = std::atoi(match[3].str().c_str());
            }
            if (match[4].matched) {
                mode.dmFields = DM_DISPLAYFREQUENCY;
                mode.dmDisplayFrequency = std::atoi(match[4].str().c_str());
            }
        }
        else if (std::regex_match(modeString, match, refreshRateRegex)) {
            mode.dmFields = DM_DISPLAYFREQUENCY;
            mode.dmDisplayFrequency = std::atoi(match[1].str().c_str());
        }
        else {
            std::cerr << "Error: Invalid mode string: " << modeString << std::endl;
            return EXIT_FAILURE;
        }

        auto changeResult = ChangeDisplaySettingsEx(
            displayDevice.DeviceName,
            &mode,
            nullptr,
            0,
            nullptr);
        if (changeResult != DISP_CHANGE_SUCCESSFUL) {
            std::cerr
                << "Error: Failed to change display settings: "
                << GetChangeDisplaySettingsErrorMessage(changeResult) << std::endl;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
}

int main(int argc, char **argv)
{
    char programName[_MAX_FNAME] = "windisplaymode";
    _splitpath_s(
        argv[0],
        nullptr, 0,
        nullptr, 0,
        programName, _MAX_FNAME,
        nullptr, 0);

    auto paramCount = argc - 1;
    auto params = argv + 1;

	if (paramCount < 2) {
        PrintUsage(programName);
		return EXIT_FAILURE;
	}

    auto command = std::string(params[0]);
    params++;
    paramCount--;

    auto displayIndex = std::atoi(params[0]);
    params++;
    paramCount--;

    if (command == "list") {
        return ListDisplayModes(displayIndex);
    }
    if (command == "set") {
        if (paramCount < 1) {
            PrintUsage(programName);
            return EXIT_FAILURE;
        }
        return SetDisplayMode(displayIndex, params[0]);
    }

    std::cerr << "Unknown command: " << command << std::endl;

    return EXIT_FAILURE;
}
