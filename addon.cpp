#include <nan.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <chrono>

extern "C" {
  #include <libavformat/avformat.h>
  #include <libavutil/imgutils.h>
  #include <libswscale/swscale.h>
  #include <libavcodec/avcodec.h>
}

using namespace v8;
using namespace std;

class ProgressWorker : public Nan::AsyncProgressWorker {
 public:
  ProgressWorker(Nan::Callback *callback)
    : Nan::AsyncProgressWorker(callback) {}

  void Execute (const Nan::AsyncProgressWorker::ExecutionProgress& progress) {
    // This is your background thread. Do your processing here,
    // and periodically call progress.Send to send progress updates.

    for (double percentage = 0; percentage <= 100; percentage += 10) {
        // Here we're just sending a single byte at a time, but you could send a
        // larger chunk of data if needed.
        char percentDone = static_cast<char>(percentage);
        progress.Send(&percentDone, 1);

        // Sleep for one second to simulate work being done
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  void HandleProgressCallback(const char *data, size_t size) {
    // This function is called in the main thread whenever you call
    // progress.Send. You can use it to emit progress events.

    Nan::HandleScope scope;

    // Extract the percentage complete from the data pointer
    int percentComplete = *data;

    // Call the JavaScript callback with the progress percentage
    v8::Local<v8::Value> argv[] = { Nan::New<v8::Number>(percentComplete) };
    callback->Call(1, argv, async_resource);
  }
};

NAN_METHOD(StartWorker) {
  Nan::Callback *callback = new Nan::Callback(info[0].As<v8::Function>());
  Nan::AsyncQueueWorker(new ProgressWorker(callback));
}

NAN_METHOD(CreateGradientVideo) {
  const char* filename = "source.mp4";
  const char* outputFilename = "output.mp4";

  const char* info1 = av_version_info();
  const int info2 = avformat_version();
  const char* info3 = avformat_configuration();
  printf ("Hello Info\n");
  printf(info1);
  printf("%d", info2);
  printf(info3);

  // AVFormatContext* pFormatCtx = avformat_alloc_context();
  // if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0)
  //     printf("Error: Couldn't open file\n");
  //     // return;

  // if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
  //     printf("Error: Couldn't find stream information\n");
  //     // return;
}

NAN_METHOD(Print) {
  if (!info[0]->IsString()) return Nan::ThrowError("Must pass a string");
  Nan::Utf8String path(info[0]);
  printf("Printed from C++: %s\n", *path);
}

NAN_MODULE_INIT(InitAll) {
  Nan::Set(target, Nan::New<String>("startWorker").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(StartWorker)).ToLocalChecked());
  Nan::Set(target, Nan::New<String>("print").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(Print)).ToLocalChecked());
  Nan::Set(target, Nan::New<String>("createGradientVideo").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(CreateGradientVideo)).ToLocalChecked());
}

NODE_MODULE(a_native_module, InitAll)