/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "constants.h"
#include "output_handler.h"
#include <Arduino.h>
#include <M5Stack.h>

#define BLACK 0x0000
#define WHITE 0xFFFF

const int dot_radius = 10;
// Size of the drawable area
int width;
int height;
// Midpoint of the y axis
int midpoint;
// Pixels per unit of x_value
int x_increment;
bool initialized = false;


void HandleOutput(tflite::ErrorReporter* error_reporter, float x_value,
                  float y_value) {
  // Do this only once
  if (!initialized) {
    M5.begin();
    Serial.begin(115200);
    Serial.println("Loading Tensorflow model...");  

    // Calculate the drawable area to avoid drawing off the edges
    width = M5.Lcd.width() - (dot_radius * 2);
    height = M5.Lcd.height() - (dot_radius * 2);
    midpoint = height / 2;
    // Calculate fractional pixels per unit of x_value
    x_increment = static_cast<float>(width) / kXrange;
    initialized = true;
  }

  // Log the current X and Y values
  TF_LITE_REPORT_ERROR(error_reporter, "x_value: %f, y_value: %f\n",
                       static_cast<double>(x_value),
                       static_cast<double>(y_value));

  M5.update();
  M5.Lcd.clear(BLACK);  

  // Calculate x position, ensuring the dot is not partially offscreen,
  // which causes artifacts and crashes
  int x_pos = dot_radius + static_cast<int>(x_value * x_increment);

  // Calculate y position, ensuring the dot is not partially offscreen
  int y_pos;
  if (y_value >= 0) {
    // Since the display's y runs from the top down, invert y_value
    y_pos = dot_radius + static_cast<int>(midpoint * (1.f - y_value));
  } else {
    // For any negative y_value, start drawing from the midpoint
    y_pos =
        dot_radius + midpoint + static_cast<int>(midpoint * (0.f - y_value));
  }

  // Draw the dot
  M5.Lcd.fillCircle(x_pos, y_pos, dot_radius, YELLOW);    

}
