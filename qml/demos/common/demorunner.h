// SPDX-License-Identifier: MIT
#pragma once

// Shared main() for the Qt demos.
//   <demo> [--time <s>] [--grab <file.png>] [--invalid]
// --grab runs the scenario from time-2 s to time (like the LVGL headless host),
// saves the window to a PNG and exits. Works with QT_QPA_PLATFORM=offscreen.
int runDemo(int argc, char *argv[], const char *moduleUri, const char *typeName);
