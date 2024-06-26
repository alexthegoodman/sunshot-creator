#include <nan.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>

extern "C"
{
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
}

using namespace v8;
using namespace std;

using json = nlohmann::json;

#define GRADIENT_SPEED 2
#define ZOOM_FACTOR 2.0

double springAnimation(double target, double current, double velocity, double tension, double friction, double direction = 0)
{
    // double direction = target - current; // negative if going down, positive if going up
    double springForce = -(current - target);
    double dampingForce = -velocity;
    double force = (springForce * tension) + (dampingForce * friction);

    // printf("Forces: %f %f %f %f %f\n", force, springForce, tension, friction, dampingForce);

    double acceleration = force;
    double newVelocity = velocity + acceleration;
    double displacement = newVelocity;

    // printf("Spring Animation: %f %f %f %f %f\n", acceleration, newVelocity, displacement, current, target);

    if (direction < 0)
    {
        if (current < target)
        {
            return 0;
        }
    }
    else if (direction > 0)
    {
        if (current > target)
        {
            return 0;
        }
    }

    return displacement;
}

// based on air friction physics
double frictionalAnimation(double target, double current, double velocity, double friction)
{
    double direction = (target - current);
    double newVelocity = direction * std::exp(-friction);
    return newVelocity;
}

// double frictionalAnimation(double target, double current, double velocity, double friction, double easeFactor) {
//     double displacement = target - current;

//     // Apply friction to velocity
//     velocity *= friction;

//     // Apply ease factor
//     double easedDisplacement = displacement * (1.0 - std::pow(1.0 - easeFactor, std::abs(displacement)));

//     // Update current position using velocity and eased displacement
//     current += velocity + easedDisplacement;

//     return current;
// }

// double cubicBezierEasing(double t, double x1, double y1, double x2, double y2) {
//     double tSquared = t * t;
//     double tCubed = tSquared * t;

//     double mt = 1.0 - t;
//     double mtSquared = mt * mt;
//     double mtCubed = mtSquared * mt;

//     double x = mtCubed + 3 * x1 * t * mtSquared + 3 * x2 * tSquared * mt + tCubed;
//     double y = mtCubed + 3 * y1 * t * mtSquared + 3 * y2 * tSquared * mt + tCubed;

//     return y;
// }

double calculateY(double r, double g, double b)
{
    return 0.299 * r + 0.587 * g + 0.114 * b;
}

// double calculateU(double r, double g, double b) {
//     return -0.14713 * r - 0.28886 * g + 0.436 * b;
// }

// double calculateU(double r, double g, double b) {
//     return -0.16874 * r - 0.33126 * g + 0.5 * b;
// }

double calculateU(double r, double g, double b)
{
    return -0.16874 * r - 0.33126 * g + 0.5 * b;
}

// double calculateV(double r, double g, double b) {
//     return 0.615 * r - 0.51498 * g - 0.10001 * b;
// }

// double calculateV(double r, double g, double b) {
//     return 0.5 * r - 0.41869 * g - 0.08131 * b;
// }

double calculateV(double r, double g, double b)
{
    return 0.5 * r - 0.41869 * g - 0.08131 * b;
}

static int transform_video(nlohmann::json config, const Nan::AsyncProgressWorker::ExecutionProgress &progress)
{
    printf("Extracting JSON...\n");

    // Extract values from the JSON object
    int duration = config["duration"];
    std::string positionsFile = config["positionsFile"];
    std::string sourceFile = config["sourceFile"];
    std::string inputFile = config["inputFile"];
    std::string outputFile = config["outputFile"];
    std::vector<nlohmann::json> zoomInfo = config["zoomInfo"];
    std::vector<nlohmann::json> backgroundInfo = config["backgroundInfo"];

    printf("Duration: %d\n", duration);

    printf("Opening Mouse Events...\n");

    std::ifstream mouseEventsFile(positionsFile);

    if (!mouseEventsFile.is_open())
    {
        printf("Could not open mouse events file\n");
        return -1;
    }

    json mouseEventsJson;
    mouseEventsFile >> mouseEventsJson;

    // if (mouseEventsJson.find("mouseEvents") == mouseEventsJson.end()) {
    //     printf("Could not find mouse events\n");
    //     return -1;
    // }

    // mouseEventsJson = mouseEventsJson["mouseEvents"];

    printf("Opening Window Data...\n");

    std::ifstream windowDataFile(sourceFile);
    json windowDataJson;
    windowDataFile >> windowDataJson;

    printf("Starting Transform...\n");

    // Initialize FFmpeg
    avformat_network_init();

    // *** decode video ***
    const char *filename = inputFile.c_str();
    const char *outputFilename = outputFile.c_str();
    int fpsInt = 60;

    AVFormatContext *decoderFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&decoderFormatCtx, filename, NULL, NULL) != 0)
    {
        printf("Could not open file\n");
        return -1;
    }

    printf("Finding Video Info...\n");

    if (avformat_find_stream_info(decoderFormatCtx, NULL) < 0)
    {
        printf("Could not find stream information\n");
        return -1;
    }

    printf("Finding Video Stream... Num Streams: %d Num Frames: %d \n", decoderFormatCtx->nb_streams, decoderFormatCtx->streams[0]->nb_frames);

    int videoStream = -1;
    AVCodecParameters *codecpar = NULL;
    for (int i = 0; i < decoderFormatCtx->nb_streams; i++)
    {
        printf("Stream %d type: %d\n", i, decoderFormatCtx->streams[i]->codecpar->codec_id);
        if (decoderFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            printf("Found video stream\n");
            videoStream = i;
            codecpar = decoderFormatCtx->streams[i]->codecpar;
            break;
        }
    }

    if (videoStream == -1)
    {
        printf("Could not find video stream\n");
        return -1;
    }

    printf("Allocate Context...\n");

    AVCodecContext *decoderCodecCtx = avcodec_alloc_context3(NULL);
    if (!decoderCodecCtx)
    {
        printf("Could not allocate video codec context\n");
    }

    if (avcodec_parameters_to_context(decoderCodecCtx, codecpar) < 0)
    {
        // Handle error
        printf("Could not assign parameters to context\n");
    }

    printf("Find Decoder... %d\n", decoderCodecCtx->codec_id);

    const AVCodec *decoderCodec = avcodec_find_decoder_by_name("h264");
    if (decoderCodec == NULL)
    {
        printf("Unsupported codec\n");
        return -1;
    }

    printf("Found Codec... %s\n", decoderCodec->name);

    if (avcodec_open2(decoderCodecCtx, decoderCodec, NULL) < 0)
    {
        printf("Could not open codec\n");
        return -1;
    }

    // print decoder pixel format
    printf("Decoder Pixel Format: %d\n", decoderCodecCtx->pix_fmt);

    // *** prep enncoding ***
    // Create output context
    AVFormatContext *encoderFormatCtx = NULL;
    avformat_alloc_output_context2(&encoderFormatCtx, NULL, NULL, outputFilename);
    if (!encoderFormatCtx)
    {
        printf("Could not create output context\n");
        return -1;
    }

    // Setup the codec
    AVStream *encoderStream = avformat_new_stream(encoderFormatCtx, NULL);
    if (!encoderStream)
    {
        printf("Failed to allocate output stream\n");
        return -1;
    }

    const AVCodec *encoderCodec = avcodec_find_encoder_by_name("libx264");
    if (!encoderCodec)
    {
        printf("Could not find encoder\n");
        return -1;
    }

    AVCodecContext *encoderCodecCtx = avcodec_alloc_context3(encoderCodec);
    if (!encoderCodecCtx)
    {
        printf("Could not create video codec context\n");
        return -1;
    }

    printf("Setting up codec context...\n");
    printf("Bit Rate: %d\n", decoderCodecCtx->bit_rate);

    encoderCodecCtx->bit_rate = decoderCodecCtx->bit_rate;
    encoderCodecCtx->width = decoderCodecCtx->width;
    encoderCodecCtx->height = decoderCodecCtx->height;
    encoderCodecCtx->time_base = AVRational{1, fpsInt}; // 30FPS
    encoderCodecCtx->framerate = AVRational{fpsInt, 1}; // 30FPS
    encoderCodecCtx->gop_size = 10;
    encoderCodecCtx->max_b_frames = 1;
    encoderCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    encoderStream->time_base = encoderCodecCtx->time_base;

    // Step 3: Open the codec and add the stream info to the format context
    if (avcodec_open2(encoderCodecCtx, NULL, NULL) < 0)
    {
        printf("Failed to open encoder for stream\n");
        return -1;
    }

    if (avcodec_parameters_from_context(encoderStream->codecpar, encoderCodecCtx) < 0)
    {
        printf("Failed to copy encoder parameters to output stream\n");
        return -1;
    }

    // Open output file
    printf("Open output file...\n");
    if (!(encoderFormatCtx->oformat->flags & AVFMT_NOFILE))
    {
        int ret = avio_open(&encoderFormatCtx->pb, outputFilename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            printf("Could not open output file %s\n", outputFilename);
            return ret;
        }
    }

    // Write file header
    if (avformat_write_header(encoderFormatCtx, NULL) < 0)
    {
        printf("Error occurred when opening output file\n");
        return -1;
    }

    int y = 0;
    double zoom = 1.0;

    printf("Starting read frames...\n");
    // printf("Stream Index: %d : %d\n", packet->stream_index, videoStream);
    // Initialize these at the start of the animation (i.e., when frameIndex == zoomStartFrame)
    double currentMultiplier = 1.0;
    double velocity = 0.0;

    // duration * fpsInt
    int totalFrames = (duration / 1000) * fpsInt;
    int frameIndex = 0;
    int successfulFrameIndex = 0;

    auto startTime = std::chrono::high_resolution_clock::now();

    double mouseX = 0, mouseY = 0;
    double currentMouseX = decoderCodecCtx->width / 2, currentMouseY = decoderCodecCtx->height / 2;
    double velocityMouseX = 0, velocityMouseY = 0;
    int zoomTop = 0, zoomLeft = 0;
    bool zoomingIn = false;
    bool zoomingIn2 = false;
    static double velocityWidth = 0; // move outside loop and into reset?
    static double velocityHeight = 0;
    bool autoZoom = false;

    // Set the tension and friction to control the behavior of the animation.
    double friction1 = 2.5;
    // double tension2 = 0.001;
    double friction2 = 3; // for frictional
    // double tension3 = 0.01;
    double friction3 = 5; // for frictional
    double easingFactor = 1.0;

    double directionX = 0;
    double directionY = 0;

    double prevZoomTop = 0;
    double prevZoomLeft = 0;
    double smoothZoomTop = 0;
    double smoothZoomLeft = 0;
    double usedZoomTop = 0;
    double usedZoomLeft = 0;

    double smoothWidth = 0;
    double smoothHeight = 0;
    double usedWidth = 0;
    double usedHeight = 0;

    int animationDuration = 2000;

    bool enableDimensionSmoothing = true;
    bool enableCoordSmoothing = true; // reatains new proportional coords

    while (1)
    {
        // printf("Read Frame %d\n", frameIndex);
        AVPacket *inputPacket = av_packet_alloc();
        if (av_read_frame(decoderFormatCtx, inputPacket) < 0)
        {
            // Break the loop if we've read all packets
            av_packet_free(&inputPacket);
            break;
        }
        if (inputPacket->stream_index == videoStream)
        {
            // printf("Send Packet\n");
            if (avcodec_send_packet(decoderCodecCtx, inputPacket) < 0)
            {
                printf("Error sending packet for decoding\n");
                av_packet_free(&inputPacket);
                return -1;
            }
            while (1)
            {
                // printf("Receive Frame\n");
                AVFrame *frame = av_frame_alloc();
                if (avcodec_receive_frame(decoderCodecCtx, frame) < 0)
                {
                    // Break the loop if no more frames
                    av_frame_free(&frame);
                    break;
                }

                // *** Frame Presentation Transformation *** //

                // Create a new AVFrame for the background.
                AVFrame *bg_frame = av_frame_alloc();
                bg_frame->format = AV_PIX_FMT_YUV420P; // Your desired format.
                bg_frame->width = encoderCodecCtx->width;
                bg_frame->height = encoderCodecCtx->height;
                // bg_frame->pts = av_rescale_q(frameIndex, encoderCodecCtx->time_base, encoderStream->time_base);
                av_frame_get_buffer(bg_frame, 0);

                //  *** Background / Gradient *** //

                // Parameters for gradient colors
                uint8_t startColorR = backgroundInfo[0]["start"]["r"];
                uint8_t startColorG = backgroundInfo[0]["start"]["g"];
                uint8_t startColorB = backgroundInfo[0]["start"]["b"];

                uint8_t endColorR = backgroundInfo[0]["end"]["r"];
                uint8_t endColorG = backgroundInfo[0]["end"]["g"];
                uint8_t endColorB = backgroundInfo[0]["end"]["b"];

                // color shift
                int colorShift = 128;

                // TODO: pregen data to avoid calculating on each frame
                for (int y = 0; y < bg_frame->height; ++y)
                {
                    for (int x = 0; x < bg_frame->width; ++x)
                    {
                        // Calculate normalized gradient position
                        double gradientPosition = static_cast<double>(x) / bg_frame->width;

                        // Calculate RGB color values
                        int colorR = static_cast<int>(startColorR + gradientPosition * (endColorR - startColorR));
                        int colorG = static_cast<int>(startColorG + gradientPosition * (endColorG - startColorG));
                        int colorB = static_cast<int>(startColorB + gradientPosition * (endColorB - startColorB));

                        // printf("Color: %d, %d, %d\n", colorR, colorG, colorB);

                        // Convert RGB to YUV
                        double colorY = calculateY(colorR, colorG, colorB);
                        double colorU = calculateU(colorR, colorG, colorB) + colorShift;
                        double colorV = calculateV(colorR, colorG, colorB) + colorShift;

                        // printf("YUV: %f, %f, %f\n", colorY, colorU, colorV);

                        // Fill Y plane
                        bg_frame->data[0][y * bg_frame->linesize[0] + x] = colorY;

                        // Fill U and V planes
                        if (y % 2 == 0 && x % 2 == 0)
                        {
                            bg_frame->data[1][(y / 2) * bg_frame->linesize[1] + (x / 2)] = colorU;
                            bg_frame->data[2][(y / 2) * bg_frame->linesize[2] + (x / 2)] = colorV;
                        }
                    }
                }

                // *** Inset Video *** //

                // Scale down the frame using libswscale.
                double scaleMultiple = 0.8; // TODO: should be 0.8?

                // printf("Frame Format: %d, %d\n", frame->format, bg_frame->format);

                struct SwsContext *swsCtx = sws_getContext(
                    frame->width, frame->height, (enum AVPixelFormat)frame->format,
                    frame->width * scaleMultiple, frame->height * scaleMultiple, (enum AVPixelFormat)frame->format,
                    SWS_BILINEAR, NULL, NULL, NULL);

                AVFrame *scaled_frame = av_frame_alloc();
                scaled_frame->format = frame->format;
                scaled_frame->width = frame->width * scaleMultiple;
                scaled_frame->height = frame->height * scaleMultiple;
                av_frame_get_buffer(scaled_frame, 0);

                sws_scale(swsCtx, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height, scaled_frame->data, scaled_frame->linesize);
                sws_freeContext(swsCtx);

                // Insert your scaled frame into the background frame.
                int offsetX = (bg_frame->width - scaled_frame->width) / 2; // Center the video.
                int offsetY = (bg_frame->height - scaled_frame->height) / 2;
                for (int y = 0; y < scaled_frame->height; ++y)
                {
                    for (int x = 0; x < scaled_frame->width; ++x)
                    {
                        // Copy Y plane.
                        bg_frame->data[0][(y + offsetY) * bg_frame->linesize[0] + (x + offsetX)] = scaled_frame->data[0][y * scaled_frame->linesize[0] + x];
                        // Copy U and V planes.
                        if (y % 2 == 0 && x % 2 == 0)
                        {
                            bg_frame->data[1][((y + offsetY) / 2) * bg_frame->linesize[1] + ((x + offsetX) / 2)] = scaled_frame->data[1][(y / 2) * scaled_frame->linesize[1] + (x / 2)];
                            bg_frame->data[2][((y + offsetY) / 2) * bg_frame->linesize[2] + ((x + offsetX) / 2)] = scaled_frame->data[2][(y / 2) * scaled_frame->linesize[2] + (x / 2)];
                        }
                    }
                }

                // *** Zoom *** //
                // auto currentTime = std::chrono::high_resolution_clock::now();
                // auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
                int timeElapsed = frameIndex * 1000 / fpsInt;

                printf("Time Elapsed: %d\n", timeElapsed);

                // Determine the portion of the background to zoom in on.
                // Start with the entire frame and gradually decrease these dimensions.
                static double targetWidth = bg_frame->width;
                static double targetHeight = bg_frame->height;

                // Search for the current zoom level

                double t = 1.0;
                double targetMultiplier = 1.0; // Default value when no zoom effect is active
                for (nlohmann::json &zoom : zoomInfo)
                {
                    int start = zoom["start"];
                    int end = zoom["end"];
                    double zoomFactor = zoom["zoom"];

                    // Process each zoom info...
                    if (timeElapsed >= start && timeElapsed < end)
                    {
                        // if (timeElapsed >= start && timeElapsed < start + animationDuration) {
                        // if (timeElapsed >= zoomInfo.startTimestamp && timeElapsed < zoomInfo.startTimestamp + 1000) {
                        if (zoomingIn == false)
                        {
                            velocity = 0;
                            velocityMouseX = 0;
                            velocityMouseY = 0;
                            velocityWidth = 0;
                            velocityHeight = 0;
                            // tension2 = 0.4; // really small values work better for zoom out
                            // friction2 = 0.8;
                            // currentMouseX = mouseX; // setting to mouseX causes a jump?
                            // currentMouseY = mouseY;
                            zoomingIn = true;
                            printf("Zooming In\n");
                        }
                        // Rest of the zooming in code...
                        targetMultiplier = zoomFactor;
                        // Calculate the interpolation factor t based on the animation progress
                        t = timeElapsed / (start + animationDuration);
                        // break;
                    }
                    else if (timeElapsed >= end && timeElapsed < end + animationDuration)
                    {
                        if (zoomingIn == true)
                        {
                            velocity = 0;
                            velocityMouseX = 0;
                            velocityMouseY = 0;
                            velocityWidth = 0;
                            velocityHeight = 0;
                            // tension2 = 0.004; // really small values work better for zoom out
                            // friction2 = 0.008;
                            // currentMouseX = mouseX;
                            // currentMouseY = mouseY;
                            zoomingIn = false;
                            printf("Zooming Out\n");
                        }
                        // Rest of the zooming out code...
                        targetMultiplier = 1.0;
                        // break;
                    }
                    // else {
                    //     zoomingIn = false;
                    //     printf("Reset Zoom\n");
                    //     // break;
                    // }
                }

                // printf("t %f\n", t);

                // this way targetWidth and targetHeight do no change on each frame
                currentMultiplier = targetMultiplier;

                // (ex. 1.0 is 100% while 0.8 is ~120%)
                // printf("currentMultiplier %f\n", currentMultiplier);

                targetWidth = bg_frame->width * currentMultiplier;
                targetHeight = bg_frame->height * currentMultiplier;

                static double currentWidth = bg_frame->width;
                static double currentHeight = bg_frame->height;

                // double displacementWidth = springAnimation(targetWidth, currentWidth, velocityWidth, tension2, friction2);
                // double displacementHeight = springAnimation(targetHeight, currentHeight, velocityHeight, tension2, friction2);

                double displacementWidth = frictionalAnimation(targetWidth, currentWidth, velocityWidth, friction2);
                double displacementHeight = frictionalAnimation(targetHeight, currentHeight, velocityHeight, friction2);

                // Round small displacements to zero
                // reduces end of animation shakiness? ???
                // if (std::abs(displacementWidth) < 25.0) {
                //     displacementWidth = displacementWidth < 0 ? -25.0 : 25.0;
                // }

                // if (std::abs(displacementHeight) < 25.0) {
                //     displacementHeight = displacementHeight < 0 ? -25.0 : 25.0;
                // }

                // // if currentWidth+= displacementWidth is within 5 of targetWidth, set displacementWidth to 0
                // if (std::abs(currentWidth + displacementWidth - targetWidth) < 100) {
                //     displacementWidth = 0;
                // }
                // if (std::abs(currentHeight + displacementHeight - targetHeight) < 100) {
                //     displacementHeight = 0;
                // }

                // printf("Displacement: %f %f\n", displacementWidth, displacementHeight);

                currentWidth += displacementWidth;
                currentHeight += displacementHeight;
                velocityWidth += displacementWidth;
                velocityHeight += displacementHeight;

                // Define the control points for your cubic Bezier curve
                // double x1 = 0.17;
                // double y1 = 0.45;
                // double x2 = 0.31;
                // double y2 = 0.95;

                // // Use the cubicBezierEasing function to interpolate between current and target values
                // currentWidth = currentWidth + (targetWidth - currentWidth) * cubicBezierEasing(t, x1, y1, x2, y2);
                // currentHeight = currentHeight + (targetHeight - currentHeight) * cubicBezierEasing(t, x1, y1, x2, y2);

                // if (t == 1.0) {
                //     currentWidth = targetWidth;
                //     currentHeight = targetHeight;
                // }

                // TESTING: if current is within 10, just snap (shakiness test)
                // if (currentWidth + 10 >= targetWidth) {
                //     currentWidth = targetWidth;
                // }
                // if (currentHeight + 10 >= targetHeight) {
                //     currentHeight = targetHeight;
                // }

                // slow down velocity
                // velocityWidth *= 0.1;
                // velocityHeight *= 0.1;

                // clamp (?)
                // currentWidth = currentWidth > 10000 ? 10000 : currentWidth < 1 ? 2 : currentWidth;
                // currentHeight = currentHeight > 10000 ? 10000 : currentHeight < 1 ? 2 : currentHeight;

                // printf("zoomingIn %d\n", zoomingIn);
                if (zoomingIn)
                {
                    // when zooming in, the targetWidth should be LESS than the currentWidth
                    // want to prevent currentWidth from being less than targetWidth
                    currentWidth = currentWidth < targetWidth ? targetWidth : currentWidth;
                    currentHeight = currentHeight < targetHeight ? targetHeight : currentHeight;
                }
                else
                {
                    currentWidth = currentWidth > targetWidth ? targetWidth : currentWidth;
                    currentHeight = currentHeight > targetHeight ? targetHeight : currentHeight;
                }

                // printf("Dimensions: %f %f %f %f %f %f\n", targetWidth, targetHeight, currentWidth, currentHeight, displacementWidth, displacementHeight);

                velocityWidth = velocityWidth > 10000 ? 10000 : velocityWidth < -10000 ? -10000
                                                                                       : velocityWidth;
                velocityHeight = velocityHeight > 10000 ? 10000 : velocityHeight < -10000 ? -10000
                                                                                          : velocityHeight;

                // test smoothing
                if (enableDimensionSmoothing)
                {
                    double smoothingFactor1 = 0.02;
                    if (successfulFrameIndex == 0)
                    {
                        smoothWidth = currentWidth + (smoothingFactor1 * currentWidth + (1 - smoothingFactor1) * smoothWidth);
                        smoothHeight = currentHeight + (smoothingFactor1 * currentHeight + (1 - smoothingFactor1) * smoothHeight);
                    }
                    else
                    {
                        smoothWidth = (smoothingFactor1 * currentWidth + (1 - smoothingFactor1) * smoothWidth);
                        smoothHeight = (smoothingFactor1 * currentHeight + (1 - smoothingFactor1) * smoothHeight);
                    }

                    // assure dimension are within frame size
                    smoothWidth = smoothWidth > bg_frame->width ? bg_frame->width : smoothWidth < 1 ? 1
                                                                                                    : smoothWidth;
                    smoothHeight = smoothHeight > bg_frame->height ? bg_frame->height : smoothHeight < 1 ? 1
                                                                                                         : smoothHeight;

                    // printf("Smooth Dimensions: %f x %f and %f x %f\n", smoothWidth, smoothHeight, targetWidth, targetHeight);

                    usedWidth = smoothWidth;
                    usedHeight = smoothHeight;
                }
                else
                {
                    usedWidth = currentWidth;
                    usedHeight = currentHeight;
                }

                // Make sure the dimensions are integers and within the frame size.
                int zoomWidth = round(usedWidth);
                zoomWidth = zoomWidth > bg_frame->width ? bg_frame->width : zoomWidth < 1 ? 1
                                                                                          : zoomWidth;
                int zoomHeight = round(usedHeight);
                zoomHeight = zoomHeight > bg_frame->height ? bg_frame->height : zoomHeight < 1 ? 1
                                                                                               : zoomHeight;

                // animate the mouse positions as well
                if (autoZoom)
                {
                    for (size_t i = 0; i < mouseEventsJson.size(); ++i)
                    {
                        if (mouseEventsJson[i]["timestamp"] >= timeElapsed)
                        {
                            // TODO: calculate based on windowDataJson (and surrounding frame?)
                            // smoother mouse movements based on distance of 100px or more
                            int deltaX = abs(mouseX - mouseEventsJson[i]["x"].get<double>());
                            int deltaY = abs(mouseY - mouseEventsJson[i]["y"].get<double>());

                            // printf("Mouse Delta: %f, %d, %d\n", mouseEventsJson[i]["x"].get<double>(), deltaX, deltaY);

                            if (deltaX < 100 && deltaY < 100)
                            {
                                break;
                            }

                            mouseX = mouseEventsJson[i]["x"].get<double>();
                            mouseY = mouseEventsJson[i]["y"].get<double>();

                            // add windowOffset
                            mouseX += windowDataJson["x"].get<double>();
                            mouseY += windowDataJson["y"].get<double>();

                            // scale mouse positions
                            mouseX = mouseX * scaleMultiple + frame->width * 0.1;
                            mouseY = mouseY * scaleMultiple + frame->height * 0.1;

                            directionX = mouseX - currentMouseX;
                            directionY = mouseY - currentMouseY;
                            break;
                        }
                    }
                }
                else
                {
                    for (nlohmann::json &zoom : zoomInfo)
                    {
                        int start = zoom["start"];
                        int end = zoom["end"];
                        double zoomFactor = zoom["zoom"];

                        // Process each zoom info...
                        //   if (timeElapsed >= start && timeElapsed < end) {
                        if (timeElapsed >= start && timeElapsed < start + animationDuration)
                        {
                            if (zoomingIn2 == false)
                            {
                                //   printf("Zooming In 2 Init\n");
                                // set mouse coods to the first mouse event after the start timestamp
                                for (size_t i = 0; i < mouseEventsJson.size(); ++i)
                                {
                                    if (mouseEventsJson[i]["timestamp"] >= timeElapsed)
                                    {
                                        mouseX = mouseEventsJson[i]["x"].get<double>();
                                        mouseY = mouseEventsJson[i]["y"].get<double>();
                                        break;
                                    }
                                }
                                zoomingIn2 = true;

                                printf("setting mouse %f %f %d %f %f\n", mouseX, scaleMultiple, frame->width, currentWidth, windowDataJson["x"].get<double>());
                                //   printf("setting mouse 2 %f %f %d %f %f\n\n", mouseY, scaleMultiple, frame->height, currentHeight, windowDataJson["y"].get<double>());

                                // DPI scaling
                                double scaleFactor = windowDataJson["scaleFactor"].get<double>();

                                mouseX = mouseX * scaleFactor;
                                mouseY = mouseY * scaleFactor;

                                // add windowOffset
                                mouseX -= windowDataJson["x"].get<double>();
                                mouseY -= windowDataJson["y"].get<double>();

                                // // scale mouse positions
                                mouseX = mouseX * scaleMultiple + frame->width * 0.1;
                                mouseY = mouseY * scaleMultiple + frame->height * 0.1;

                                // TEST: set mouseX and mouseY to center of frame
                                // mouseX = ((frame->width  * scaleMultiple) / 2) + frame->width * 0.1;
                                // mouseY = ((frame->height  * scaleMultiple) / 2) + frame->height * 0.1;

                                // shift
                                // mouseX += 80; // why?
                                // mouseY += 130;

                                // printf("Mouse %f %f\n\n", mouseX, mouseY);

                                if (!autoZoom)
                                {
                                    currentMouseX = mouseX;
                                    currentMouseY = mouseY;
                                }

                                directionX = mouseX - currentMouseX;
                                directionY = mouseY - currentMouseY;
                                printf("Zooming In 2\n");
                                // break;
                            }
                        }
                        else if (timeElapsed >= end && timeElapsed < end + animationDuration)
                        {
                            if (zoomingIn2 == true)
                            {
                                zoomingIn2 = false;

                                // TODO: unneeded? shouldn't this reset to center?

                                //   // add windowOffset
                                //   mouseX -= windowDataJson["x"].get<double>();
                                //   mouseY -= windowDataJson["y"].get<double>();

                                //   // // scale mouse positions
                                //   mouseX = mouseX * scaleMultiple + frame->width * 0.1;
                                //   mouseY = mouseY * scaleMultiple + frame->height * 0.1;

                                //   directionX = mouseX - currentMouseX;
                                //   directionY = mouseY - currentMouseY;
                                printf("Zooming Out 2\n");
                                // break;
                            }
                        }
                        // else {
                        //     zoomingIn2 = false;
                        //     printf("Reset Zoom 2\n");
                        //     break;
                        // }
                    }
                }

                if (autoZoom)
                {
                    if (mouseX != currentMouseX || mouseY != currentMouseY)
                    {
                        // autoZoom requires animation between mouse positions
                        // printf("Spring Args %f %f %f %f %f\n", mouseX, currentMouseX, velocityMouseX, tension, friction);

                        // double displacementMouseX = springAnimation(mouseX, currentMouseX, velocityMouseX, tension3, friction3, directionX);
                        // double displacementMouseY = springAnimation(mouseY, currentMouseY, velocityMouseY, tension3, friction3, directionY);

                        double displacementMouseX = frictionalAnimation(mouseX, currentMouseX, velocityMouseX, friction3);
                        double displacementMouseY = frictionalAnimation(mouseY, currentMouseY, velocityMouseY, friction3);

                        // printf("Displacement Position: %f, %f\n", displacementMouseX, displacementMouseY);

                        // TODO: smoothing?

                        // slow down velocity
                        // displacementMouseX *= 0.1;
                        // displacementMouseY *= 0.1;

                        // Update 'current' and 'velocity' variables for the next frame
                        velocityMouseX += displacementMouseX;
                        currentMouseX += displacementMouseX; // or += displacementMouseX?
                        velocityMouseY += displacementMouseY;
                        currentMouseY += displacementMouseY;
                    }
                }

                // clamp max
                double frameWidth = (double)frame->width;
                double frameHeight = (double)frame->height;

                // alterative clamp
                currentMouseX = currentMouseX > frameWidth ? frameWidth : currentMouseX < 0 ? 0
                                                                                            : currentMouseX; // or with targetWidth?
                currentMouseY = currentMouseY > frameHeight ? frameHeight : currentMouseY < 0 ? 0
                                                                                              : currentMouseY;

                velocityMouseX = velocityMouseX > frameWidth ? frameWidth : velocityMouseX < -(mouseX) ? -(mouseX)
                                                                                                       : velocityMouseX;
                velocityMouseY = velocityMouseY > frameHeight ? frameHeight : velocityMouseY < -(mouseY) ? -(mouseY)
                                                                                                         : velocityMouseY;

                // printf("Mouse Positions: %f, %f and %f, %f\n", mouseX, mouseY, currentMouseX, currentMouseY);

                // printf("Spring Position: %f, %f\n", currentMouseX, currentMouseY);

                // zoomLeft and zoomTop are position of zooming viewport overtop background
                // then size of zoomed region are with zoomWidth and zoomHeight

                // printf("Smooth Info: %f, %f\n", smoothHeight, smoothWidth);

                // Center the zoom on the current mouse position
                zoomTop = (std::max)(0, static_cast<int>((std::min)(currentMouseY - zoomHeight / 2, static_cast<double>(bg_frame->height) - zoomHeight)));
                zoomLeft = (std::max)(0, static_cast<int>((std::min)(currentMouseX - zoomWidth / 2, static_cast<double>(bg_frame->width) - zoomWidth)));

                double targetZoomTop = (std::max)(0, static_cast<int>((std::min)(currentMouseY - targetHeight / 2, static_cast<double>(bg_frame->height) - targetHeight)));
                double targetZoomLeft = (std::max)(0, static_cast<int>((std::min)(currentMouseX - targetWidth / 2, static_cast<double>(bg_frame->width) - targetWidth)));

                // zoomTop = std::max(0, static_cast<int>(std::min(currentMouseY - targetHeight / 2, static_cast<double>(bg_frame->height) - targetHeight)));
                // zoomLeft = std::max(0, static_cast<int>(std::min(currentMouseX - targetWidth / 2, static_cast<double>(bg_frame->width) - targetWidth)));

                // alternative approach
                // double centerY = static_cast<double>(bg_frame->height) / 2.0;
                // double centerX = static_cast<double>(bg_frame->width) / 2.0;
                // zoomTop = std::max(0, static_cast<int>(std::min(currentMouseY - centerY, centerY - targetHeight / 2)));
                // zoomLeft = std::max(0, static_cast<int>(std::min(currentMouseX - centerX, centerX - targetWidth / 2)));

                // round to nearest 10 number
                // zoomTop = std::round(zoomTop / 10.0) * 10;
                // zoomLeft = std::round(zoomLeft / 10.0) * 10;
                // int remainderTop = zoomTop % 10;
                // int remainderLeft = zoomLeft % 10;
                // zoomTop = remainderTop < 5 ? zoomTop - remainderTop : zoomTop + (10 - remainderTop);
                // zoomLeft = remainderLeft < 5 ? zoomLeft - remainderLeft : zoomLeft + (10 - remainderLeft);

                // printf("Mid Info1: %d, %d\n", zoomTop, zoomLeft);

                // max clamps
                if (zoomTop + zoomHeight > bg_frame->height)
                {
                    zoomTop = bg_frame->height - zoomHeight;
                }
                if (zoomLeft + zoomWidth > bg_frame->width)
                {
                    zoomLeft = bg_frame->width - zoomWidth;
                }

                // clamp zoomTop and zoomLeft
                zoomTop = zoomTop > bg_frame->height ? bg_frame->height : zoomTop;
                zoomLeft = zoomLeft > bg_frame->width ? bg_frame->width : zoomLeft;

                // printf("Mid Info2: %d, %d\n", zoomTop, zoomLeft);

                if (enableCoordSmoothing)
                {
                    prevZoomTop = smoothZoomTop;
                    prevZoomLeft = smoothZoomLeft;

                    if (frameIndex == 0)
                    {
                        smoothZoomTop = zoomTop;
                        smoothZoomLeft = zoomLeft;
                    }

                    double frameProportion = bg_frame->height / bg_frame->width;

                    // seems to get confused but proportionality is maintained
                    // double smoothingFactor = 0.95; // Adjust this value to change the amount of smoothing (0-1)
                    // double topChange = (1 - smoothingFactor) * smoothZoomTop;
                    // smoothZoomTop = smoothingFactor * zoomTop + topChange;
                    // smoothZoomLeft = smoothingFactor * zoomLeft + (topChange * frameProportion);

                    double smoothingFactor = 0.95; // Adjust this value to change the amount of smoothing (0-1)
                    double topChange = (1 - smoothingFactor) * smoothZoomTop;
                    smoothZoomTop = zoomTop + topChange;
                    smoothZoomLeft = zoomLeft + (topChange * frameProportion);

                    // printf("Smooth Info: %d, %d\n", smoothZoomTop, smoothZoomLeft);

                    // no negative numbers
                    smoothZoomTop = smoothZoomTop < 0 ? 0 : smoothZoomTop;
                    smoothZoomLeft = smoothZoomLeft < 0 ? 0 : smoothZoomLeft;

                    // round
                    smoothZoomTop = std::round(smoothZoomTop);
                    smoothZoomLeft = std::round(smoothZoomLeft);

                    // assure even numbers
                    smoothZoomTop = (static_cast<int>(smoothZoomTop) % 2 == 0) ? smoothZoomTop : smoothZoomTop - 1;
                    smoothZoomLeft = (static_cast<int>(smoothZoomLeft) % 2 == 0) ? smoothZoomLeft : smoothZoomLeft - 1;

                    // max clamps
                    if (smoothZoomTop + zoomHeight > bg_frame->height)
                    {
                        smoothZoomTop = bg_frame->height - zoomHeight;
                    }
                    if (smoothZoomLeft + zoomWidth > bg_frame->width)
                    {
                        smoothZoomLeft = bg_frame->width - zoomWidth;
                    }

                    // printf("Mid Info: %d, %d\n", zoomTop, zoomLeft);

                    // clamp zoomTop and zoomLeft
                    smoothZoomTop = smoothZoomTop > bg_frame->height ? bg_frame->height : smoothZoomTop;
                    smoothZoomLeft = smoothZoomLeft > bg_frame->width ? bg_frame->width : smoothZoomLeft;

                    // double sure
                    smoothZoomTop = (static_cast<int>(smoothZoomTop) % 2 == 0) ? smoothZoomTop : smoothZoomTop - 1;
                    smoothZoomLeft = (static_cast<int>(smoothZoomLeft) % 2 == 0) ? smoothZoomLeft : smoothZoomLeft - 1;

                    usedZoomTop = smoothZoomTop;
                    usedZoomLeft = smoothZoomLeft;
                }
                else
                {
                    // assure even numbers
                    zoomTop = (static_cast<int>(zoomTop) % 2 == 0) ? zoomTop : zoomTop - 1;
                    zoomLeft = (static_cast<int>(zoomLeft) % 2 == 0) ? zoomLeft : zoomLeft - 1;

                    usedZoomTop = zoomTop;
                    usedZoomLeft = zoomLeft;
                }

                printf("Used Info: %f, %f and %f, %f\n", usedZoomTop, usedZoomLeft, targetZoomTop, targetZoomLeft);

                // Create a new AVFrame for the zoomed portion.
                AVFrame *zoom_frame = av_frame_alloc();
                zoom_frame->format = bg_frame->format;
                zoom_frame->width = bg_frame->width; // Keep the output frame size the same.
                zoom_frame->height = bg_frame->height;
                zoom_frame->pts = av_rescale_q(frameIndex, encoderCodecCtx->time_base, encoderStream->time_base);
                av_frame_get_buffer(zoom_frame, 0);

                // printf("Zoom Frame: %d x %d\n", zoomWidth, zoomHeight);
                // printf("Diagnostic Info: %d x %d\n", zoom_frame->width, zoom_frame->height);

                // Scale up the zoomed portion to the output frame size using libswscale.
                struct SwsContext *swsCtxZoom = sws_getContext(
                    zoomWidth, zoomHeight, (enum AVPixelFormat)bg_frame->format,
                    zoom_frame->width, zoom_frame->height, (enum AVPixelFormat)zoom_frame->format,
                    SWS_BILINEAR, NULL, NULL, NULL);

                // Get pointers to the zoomed portion in the background frame.
                uint8_t *zoomData[3];

                int usedZoomTopInt = static_cast<int>(usedZoomTop);
                int usedZoomLeftInt = static_cast<int>(usedZoomLeft);

                zoomData[0] = &bg_frame->data[0][usedZoomTopInt * bg_frame->linesize[0] + usedZoomLeftInt];
                if (usedZoomTopInt % 2 == 0 && usedZoomLeftInt % 2 == 0)
                {
                    zoomData[1] = &bg_frame->data[1][(usedZoomTopInt / 2) * bg_frame->linesize[1] + (usedZoomLeftInt / 2)];
                    zoomData[2] = &bg_frame->data[2][(usedZoomTopInt / 2) * bg_frame->linesize[2] + (usedZoomLeftInt / 2)];
                }
                else
                {
                    zoomData[1] = NULL;
                    zoomData[2] = NULL;
                }

                // check zoomData
                for (int i = 0; i < 3; i++)
                {
                    if (zoomData[i] == nullptr)
                    {
                        printf("zoomData[%d] is null\n", i);
                    }
                }

                // check other sws_scale args
                // printf("sws_scale args %d %d %d %d \n", bg_frame->linesize, zoomHeight, zoom_frame->data, zoom_frame->linesize);

                // zoom scale
                sws_scale(swsCtxZoom, (const uint8_t *const *)zoomData, bg_frame->linesize, 0, zoomHeight, zoom_frame->data, zoom_frame->linesize);

                // some cleanup
                sws_freeContext(swsCtxZoom);

                av_frame_free(&bg_frame);     // We don't need the original background frame anymore.
                av_frame_free(&frame);        // We don't need the original frame anymore.
                av_frame_free(&scaled_frame); // We don't need the scaled frame anymore.

                // printf("Send Frame\n");

                if (avcodec_send_frame(encoderCodecCtx, zoom_frame) < 0)
                {
                    printf("Error sending frame for encoding\n");
                    av_frame_free(&zoom_frame);
                    av_packet_free(&inputPacket);
                    return -1;
                }

                while (1)
                {
                    // printf("Receive Packet\n");
                    AVPacket *outputPacket = av_packet_alloc();
                    if (avcodec_receive_packet(encoderCodecCtx, outputPacket) < 0)
                    {
                        // Break the loop if no more packets
                        av_packet_free(&outputPacket);
                        break;
                    }
                    // printf("Write Packet\n");
                    if (av_write_frame(encoderFormatCtx, outputPacket) < 0)
                    {
                        printf("Error writing packet\n");
                        av_frame_free(&bg_frame);
                        av_packet_free(&inputPacket);
                        av_packet_free(&outputPacket);
                        return -1;
                    }
                    av_packet_free(&outputPacket);
                    successfulFrameIndex++;
                }
                av_frame_free(&bg_frame);
            }
        }
        av_packet_free(&inputPacket);
        frameIndex++;

        int percentage = round(((double)frameIndex / (double)totalFrames) * 100.0);
        char percentDone = static_cast<char>(percentage);
        progress.Send(&percentDone, 1);
    }

    // // flush the decoder
    // avcodec_send_packet(decoderCodecCtx, NULL);
    // while (avcodec_receive_frame(decoderCodecCtx, pFrame) == 0) {
    //     avcodec_send_frame(encoderCodecCtx, pFrame);
    //     av_frame_unref(pFrame);
    // }

    // // flush the encoder
    // avcodec_send_frame(encoderCodecCtx, NULL);
    // while (avcodec_receive_packet(encoderCodecCtx, packet) == 0) {
    //     av_interleaved_write_frame(encoderFormatCtx, packet);
    //     av_packet_unref(packet);
    // }

    // TODO: write final frame (unnecessary?)

    // Write file trailer and close the file
    printf("Write trailer...\n");
    av_write_trailer(encoderFormatCtx);
    if (!(encoderFormatCtx->oformat->flags & AVFMT_NOFILE))
    {
        avio_closep(&encoderFormatCtx->pb);
    }

    printf("Clean Up\n");

    // fclose(outfile);

    // Cleanup encode
    avcodec_free_context(&encoderCodecCtx);
    avformat_free_context(encoderFormatCtx);
    // av_packet_free(&packet);

    // cleanup decode
    // av_free(buffer);
    // av_frame_free(&pFrameRGB);
    // av_frame_free(&pFrame);
    // av_frame_free(&pFrameFinal);
    avcodec_close(decoderCodecCtx);
    avformat_close_input(&decoderFormatCtx);
    // avcodec_free_context(&decoderCodecCtx); ?

    // -1 signifies completion of generation for simplicity
    int percentage = -1;
    char percentDone = static_cast<char>(percentage);
    progress.Send(&percentDone, 1);

    return 0;
}

class ProgressWorker : public Nan::AsyncProgressWorker
{
    nlohmann::json config;
    // Nan::Callback *callback;

public:
    ProgressWorker(nlohmann::json config, Nan::Callback *callback)
        : Nan::AsyncProgressWorker(callback)
    {
        this->config = config;
    }

    ~ProgressWorker() {}

    void Execute(const Nan::AsyncProgressWorker::ExecutionProgress &progress)
    {
        printf("Execute...\n");
        // This is your background thread. Do your processing here,
        // and periodically call progress.Send to send progress updates.

        // transform video
        transform_video(config, progress);
    }

    void HandleProgressCallback(const char *data, size_t size)
    {
        // This function is called in the main thread whenever you call
        // progress.Send. You can use it to emit progress events.

        Nan::HandleScope scope;

        // Extract the percentage complete from the data pointer
        int percentComplete = *data;

        // Call the JavaScript callback with the progress percentage
        v8::Local<v8::Value> argv[] = {Nan::New<v8::Number>(percentComplete)};
        callback->Call(1, argv, async_resource);
    }
};

NAN_METHOD(StartWorker)
{
    printf("Setup Worker...\n");

    // Convert the V8 object to a JSON string
    v8::String::Utf8Value jsonStr(info.GetIsolate(), info[0]->ToString(Nan::GetCurrentContext()).ToLocalChecked());

    printf("Parse JSON... %s\n", *jsonStr);

    // Parse the JSON string into a nlohmann::json object
    nlohmann::json config = nlohmann::json::parse(*jsonStr);

    printf("Create Callback...\n");

    Nan::Callback *callback = new Nan::Callback(info[1].As<v8::Function>());

    printf("Start Worker...\n");

    Nan::AsyncQueueWorker(new ProgressWorker(config, callback));
}

NAN_METHOD(CreateGradientVideo)
{
    const char *filename = "source.mp4";
    const char *outputFilename = "output.mp4";

    const char *info1 = av_version_info();
    const int info2 = avformat_version();
    const char *info3 = avformat_configuration();
    printf("Hello Info\n");
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

NAN_METHOD(Print)
{
    if (!info[0]->IsString())
        return Nan::ThrowError("Must pass a string");
    Nan::Utf8String path(info[0]);
    printf("Printed from C++: %s\n", *path);
}

NAN_MODULE_INIT(InitAll)
{
    Nan::Set(target, Nan::New<String>("startWorker").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(StartWorker)).ToLocalChecked());
    Nan::Set(target, Nan::New<String>("print").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(Print)).ToLocalChecked());
    Nan::Set(target, Nan::New<String>("createGradientVideo").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(CreateGradientVideo)).ToLocalChecked());
}

NODE_MODULE(a_native_module, InitAll)