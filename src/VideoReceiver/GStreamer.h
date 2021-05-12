#pragma once

#include <QObject>
#include <QQuickItem>

#include "VideoReceiver.h"
#include "Video1Settings.h"
#include "Video2Settings.h"

Q_DECLARE_LOGGING_CATEGORY(GStreamerLog)
Q_DECLARE_LOGGING_CATEGORY(GStreamerAPILog)

class GStreamer {
public:
    static void blacklist(Video1Settings::VideoDecoderOptions option);
    static void blacklist(Video2Settings::VideoDecoderOptions option);
    static void initialize(int argc, char* argv[], int debuglevel);
    static void* createVideoSink(QObject* parent, QQuickItem* widget);
    static void releaseVideoSink(void* sink);
    static VideoReceiver* createVideoReceiver(QObject* parent);
};
