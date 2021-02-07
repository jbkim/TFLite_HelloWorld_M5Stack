/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

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

// #include "main_functions.h"
#include <Arduino.h>
#include <math.h>

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "constants.h"
#include "model.h"
#include "output_handler.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

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

// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
int inference_count = 0;

constexpr int kTensorArenaSize = 2000;
uint8_t tensor_arena[kTensorArenaSize];
}  // namespace

// The name of this function is important for Arduino compatibility.
void setup() {
  M5.begin();
  Serial.begin(115200);
  Serial.println("Loading Tensorflow model...");  

  // Set up logging. Google style is to avoid globals or statics because of
  // lifetime uncertainty, but since this has a trivial destructor it's okay.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // This pulls in all the operation implementations we need.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::AllOpsResolver resolver;

  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  // Obtain pointers to the model's input and output tensors.
  input = interpreter->input(0);
  output = interpreter->output(0);

  // Keep track of how many inferences we have performed.
  inference_count = 0;

  Serial.println("Starting inferences...");
  M5.Lcd.drawString("TensorFlow Lite for Microcontrollers", 60, 0);  
  width = M5.Lcd.width() - (dot_radius * 2);
  height = M5.Lcd.height() - (dot_radius * 2);
  midpoint = height / 2;
  x_increment = static_cast<float>(width) / kXrange;

}

// The name of this function is important for Arduino compatibility.
void loop() {
#if 0
    M5.update();

    M5.Lcd.fillCircle(int(user_input * 50 + 3), int(140 - output->data.f[0] * 100), 8, BLACK);

    user_input = user_input + 0.1;
    if (user_input > 6.28) {
        user_input = 0;
    }
    input->data.f[0] = user_input;
    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("There was an error invoking the interpreter!");
        Serial.print("Input: ");
        Serial.println(user_input);
        return;
    }

    M5.Lcd.fillCircle(int(user_input * 50 + 3), int(140 - output->data.f[0] * 100), 8, WHITE);
    delay(10);
#else

  M5.update();

  // Calculate an x value to feed into the model. We compare the current
  // inference_count to the number of inferences per cycle to determine
  // our position within the range of possible x values the model was
  // trained on, and use this to calculate a value.
  float position = static_cast<float>(inference_count) /
                   static_cast<float>(kInferencesPerCycle);
  float x = position * kXrange;

  // Quantize the input from floating-point to integer
  int8_t x_quantized = x / input->params.scale + input->params.zero_point;
  // Place the quantized input in the model's input tensor
  input->data.int8[0] = x_quantized;

  // Run inference, and report any error
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed on x: %f\n",
                         static_cast<double>(x));
    return;
  }

  // Obtain the quantized output from model's output tensor
  int8_t y_quantized = output->data.int8[0];
  // Dequantize the output from integer to floating-point
  float y = (y_quantized - output->params.zero_point) * output->params.scale;
  
  // Output the results. A custom HandleOutput function can be implemented
  // for each supported hardware target.
  M5.Lcd.clear(BLACK);  
  HandleOutput(error_reporter, x, y);
  int x_pos = dot_radius + static_cast<int>(x * x_increment);  
  int y_pos;
    if (y >= 0) {
      // Since the display's y runs from the top down, invert y_value
      y_pos = dot_radius + static_cast<int>(midpoint * (1.f - y));
    } else {
      // For any negative y_value, start drawing from the midpoint
      y_pos =
          dot_radius + midpoint + static_cast<int>(midpoint * (0.f - y));
    }

  // M5.Lcd.fillCircle(x, int(140 - output->data.f[0] * 100), 8, WHITE);
  M5.Lcd.fillCircle(x_pos, y_pos, dot_radius, YELLOW);  

  // Increment the inference_counter, and reset it if we have reached
  // the total number per cycle
  inference_count += 1;
  if (inference_count >= kInferencesPerCycle) inference_count = 0;
#endif  
}
