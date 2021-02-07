# TFLite_HelloWorld_M5Stack
## Tensorflow Lite HelloWorld 예제를 M5Stack에 포팅

<img src="tflite_m5stack.gif" width="135" height="240" />

### TFLite 빌드
~~~
make -f tensorflow/lite/micro/tools/make/Makefile TARGET=esp generate_hello_world_esp_project
~~~
- 빌드한 tensorflow/lite/micro/tools/make/gen/esp_xtensa-esp32_default/prj/hello_world/esp-idf/ 폴더 아래에 있는 component와 main퐅더의 내용을 platformio 프로젝트 폴더에 copy해야 함.

### PlatformIO 개발환경 설정
- New Project를 선택해서 M5Stack을 선택해서 프로젝트를 구성 (Platform은 Arduino를 선택)
- platformio.ini 파일을 수정하는데, build_flags에 다음과 같이 define과 include되는 부분을 설정해야 함.
~~~
build_flags = -DARDUINOSTL_M_H -Ilib/tfmicro/third_party/gemmlowp -Ilib/tfmicro/third_party/flatbuffers/include -Ilib/tfmicro/third_party/ruy
~~~
- component 폴더 아래 있는 tfmicro폴더의 내용을 lib폴더 아래에 copy
- main 폴더 아래에 있는 파일을 src 폴더 아래에 copy
- main_functions.h 파일은 필요없으니 삭제하고, main_functions.cc파일을 main.cpp로 이름을 변경한다.

### 코드 수정
- main.cpp의 내용을 M5Stack에 맞게 수정
- 일단 컴파일이 되도록 Arduino.h 파일에서 다음을 수정 :joy:
```c++
  // #define DEFAULT 1
  // #define EXTERNAL 0
```
- kernel_util.cc 에서 다음 부분도 코멘트 처리 :joy:
```c++
std::string GetShapeDebugString(const TfLiteIntArray* shape) {
  std::string str;
#if 0
  for (int d = 0; d < shape->size; ++d) {
    if (str.empty())
      str = "[" + std::to_string(shape->data[d]);
    else
      str += ", " + std::to_string(shape->data[d]);
  }
  str += "]";
#endif

  return str;
}
```