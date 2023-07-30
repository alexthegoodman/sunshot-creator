#include <nan.h>
#include <stdio.h>
#include <iostream>

extern "C" {
  #include <libavformat/avformat.h>
  #include <libavutil/imgutils.h>
  #include <libswscale/swscale.h>
  #include <libavcodec/avcodec.h>
}

using namespace v8;
using namespace std;

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
  Nan::Set(target, Nan::New<String>("print").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(Print)).ToLocalChecked());
  Nan::Set(target, Nan::New<String>("createGradientVideo").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(CreateGradientVideo)).ToLocalChecked());
}

NODE_MODULE(a_native_module, InitAll)