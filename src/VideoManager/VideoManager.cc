/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QUrl>
#include <QDir>

#ifndef QGC_DISABLE_UVC
#include <QCameraInfo>
#endif

#include "ScreenToolsController.h"
#include "VideoManager.h"
#include "QGCToolbox.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "MultiVehicleManager.h"
#include "Settings/SettingsManager.h"
#include "Vehicle.h"
#include "QGCCameraManager.h"

#if defined(QGC_GST_STREAMING)
#include "GStreamer.h"
#include "Video1Settings.h"
#include "Video2Settings.h"
#else
#include "GLVideoItemStub.h"
#endif

#ifdef QGC_GST_TAISYNC_ENABLED
#include "TaisyncHandler.h"
#endif

QGC_LOGGING_CATEGORY(VideoManagerLog, "VideoManagerLog")

#if defined(QGC_GST_STREAMING)
static const char* kFileExtension[VideoReceiver::FILE_FORMAT_MAX - VideoReceiver::FILE_FORMAT_MIN] = {
    "mkv",
    "mov",
    "mp4"
};
#endif

//-----------------------------------------------------------------------------
VideoManager::VideoManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
#if !defined(QGC_GST_STREAMING)
    static bool once = false;
    if (!once) {
        qmlRegisterType<GLVideoItemStub>("org.freedesktop.gstreamer.GLVideoItem", 1, 0, "GstGLVideoItem");
        once = true;
    }
#endif
}

//-----------------------------------------------------------------------------
VideoManager::~VideoManager()
{
    for (int i = 0; i < 2; i++) {
        if (_videoReceiver[i] != nullptr) {
            delete _videoReceiver[i];
            _videoReceiver[i] = nullptr;
        }
#if defined(QGC_GST_STREAMING)
        if (_videoSink[i] != nullptr) {
            // FIXME: AV: we need some interaface for video sink with .release() call
            // Currently VideoManager is destroyed after corePlugin() and we are crashing on app exit
            // calling qgcApp()->toolbox()->corePlugin()->releaseVideoSink(_videoSink[i]);
            // As for now let's call GStreamer::releaseVideoSink() directly
            GStreamer::releaseVideoSink(_videoSink[i]);
            _videoSink[i] = nullptr;
        }
#endif
    }
}

//-----------------------------------------------------------------------------
/*
void
VideoManager::setToolbox(QGCToolbox *toolbox)
{

   QGCTool::setToolbox(toolbox);
   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<VideoManager> ("QGroundControl.VideoManager", 1, 0, "VideoManager", "Reference only");
   qmlRegisterUncreatableType<VideoReceiver>("QGroundControl",              1, 0, "VideoReceiver","Reference only");

   // TODO: Those connections should be Per Video, not per VideoManager.
   _videoSettings = toolbox->settingsManager()->video1Settings();
   QString videoSource = _videoSettings->videoSource()->rawValue().toString();
   connect(_videoSettings->videoSource(),   &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
   connect(_videoSettings->udpPort(),       &Fact::rawValueChanged, this, &VideoManager::_udpPortChanged);
   connect(_videoSettings->rtspUrl(),       &Fact::rawValueChanged, this, &VideoManager::_rtspUrlChanged);
   connect(_videoSettings->tcpUrl(),        &Fact::rawValueChanged, this, &VideoManager::_tcpUrlChanged);
   connect(_videoSettings->aspectRatio(),   &Fact::rawValueChanged, this, &VideoManager::_aspectRatioChanged);
   connect(_videoSettings->lowLatencyMode(),&Fact::rawValueChanged, this, &VideoManager::_lowLatencyModeChanged);
   MultiVehicleManager *pVehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
   connect(pVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &VideoManager::_setActiveVehicle);

#if defined(QGC_GST_STREAMING)
    if(videoSettingNumber == 1){
      GStreamer::blacklist(static_cast<Video1Settings::VideoDecoderOptions>(_videoSettings->forceVideoDecoder()->rawValue().toInt()));
    }else{
      GStreamer::blacklist(static_cast<Video2Settings::VideoDecoderOptions>(_videoSettings->forceVideoDecoder()->rawValue().toInt()));
    }
#ifndef QGC_DISABLE_UVC
   // If we are using a UVC camera setup the device name
   _updateUVC();
#endif

    emit isGStreamerChanged();
    qCDebug(VideoManagerLog) << "New Video Source:" << videoSource;
#if defined(QGC_GST_STREAMING)
    _videoReceiver[0] = toolbox->corePlugin()->createVideoReceiver(this);
    _videoReceiver[1] = toolbox->corePlugin()->createVideoReceiver(this);

    connect(_videoReceiver[0], &VideoReceiver::streamingChanged, this, [this](bool active){
        _streaming = active;
        emit streamingChanged();
    });

    connect(_videoReceiver[0], &VideoReceiver::onStartComplete, this, [this](VideoReceiver::STATUS status) {
        if (status == VideoReceiver::STATUS_OK) {
            _videoStarted[0] = true;
            if (_videoSink[0] != nullptr) {
                // It is absolytely ok to have video receiver active (streaming) and decoding not active
                // It should be handy for cases when you have many streams and want to show only some of them
                // NOTE that even if decoder did not start it is still possible to record video
                _videoReceiver[0]->startDecoding(_videoSink[0]);
            }
        } else if (status == VideoReceiver::STATUS_INVALID_URL) {
            // Invalid URL - don't restart
        } else if (status == VideoReceiver::STATUS_INVALID_STATE) {
            // Already running
        } else {
            _restartVideo(0);
        }
    });

    connect(_videoReceiver[0], &VideoReceiver::onStopComplete, this, [this](VideoReceiver::STATUS) {
        _videoStarted[0] = false;
        _startReceiver(0);
    });

    connect(_videoReceiver[0], &VideoReceiver::decodingChanged, this, [this](bool active){
        _decoding = active;
        emit decodingChanged();
    });

    connect(_videoReceiver[0], &VideoReceiver::recordingChanged, this, [this](bool active){
        _recording = active;
        if (!active) {
            _subtitleWriter.stopCapturingTelemetry();
        }
        emit recordingChanged();
    });

    connect(_videoReceiver[0], &VideoReceiver::recordingStarted, this, [this](){
        _subtitleWriter.startCapturingTelemetry(_videoFile);
    });

    connect(_videoReceiver[0], &VideoReceiver::videoSizeChanged, this, [this](QSize size){
        _videoSize = ((quint32)size.width() << 16) | (quint32)size.height();
        emit videoSizeChanged();
    });

    //connect(_videoReceiver, &VideoReceiver::onTakeScreenshotComplete, this, [this](VideoReceiver::STATUS status){
    //    if (status == VideoReceiver::STATUS_OK) {
    //    }
    //});

    // FIXME: AV: I believe _thermalVideoReceiver should be handled just like _videoReceiver in terms of event
    // and I expect that it will be changed during multiple video stream activity
    if (_videoReceiver[1] != nullptr) {
        connect(_videoReceiver[1], &VideoReceiver::onStartComplete, this, [this](VideoReceiver::STATUS status) {
            if (status == VideoReceiver::STATUS_OK) {
                _videoStarted[1] = true;
                if (_videoSink[1] != nullptr) {
                    _videoReceiver[1]->startDecoding(_videoSink[1]);
                }
            } else if (status == VideoReceiver::STATUS_INVALID_URL) {
                // Invalid URL - don't restart
            } else if (status == VideoReceiver::STATUS_INVALID_STATE) {
                // Already running
            } else {
                _restartVideo(1);
            }
        });

        connect(_videoReceiver[1], &VideoReceiver::onStopComplete, this, [this](VideoReceiver::STATUS) {
            _videoStarted[1] = false;
            _startReceiver(1);
        });
    }
#endif
    _updateSettings(0);
    _updateSettings(1);
    if(isGStreamer()) {
        startVideo();
    } else {
        stopVideo();
    }

#endif
}
*/
void
VideoManager::setToolboxMod(QGCToolbox *toolbox, int _videoSettingNumber)
{
   videoSettingNumber = _videoSettingNumber;
   QGCTool::setToolbox(toolbox);

   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<VideoManager> ("QGroundControl.VideoManager", 1, 0, "VideoManager", "Reference only");
   qmlRegisterUncreatableType<VideoReceiver>("QGroundControl",              1, 0, "VideoReceiver","Reference only");

   // TODO: Those connections should be Per Video, not per VideoManager.
   QString videoSource;

   if(videoSettingNumber == 1){
   _video1Settings = toolbox->settingsManager()->video1Settings();
   videoSource     = _video1Settings->videoSource()->rawValue().toString();
   connect(_video1Settings->videoSource(),   &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
   connect(_video1Settings->udpPort(),       &Fact::rawValueChanged, this, &VideoManager::_udpPortChanged);
   connect(_video1Settings->rtspUrl(),       &Fact::rawValueChanged, this, &VideoManager::_rtspUrlChanged);
   connect(_video1Settings->tcpUrl(),        &Fact::rawValueChanged, this, &VideoManager::_tcpUrlChanged);
   connect(_video1Settings->aspectRatio(),   &Fact::rawValueChanged, this, &VideoManager::_aspectRatioChanged);
   connect(_video1Settings->lowLatencyMode(),&Fact::rawValueChanged, this, &VideoManager::_lowLatencyModeChanged);
   
   }else{ // 2
   _video2Settings = toolbox->settingsManager()->video2Settings();
   videoSource     = _video2Settings->videoSource()->rawValue().toString();
   connect(_video2Settings->videoSource(),   &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
   connect(_video2Settings->udpPort(),       &Fact::rawValueChanged, this, &VideoManager::_udpPortChanged);
   connect(_video2Settings->rtspUrl(),       &Fact::rawValueChanged, this, &VideoManager::_rtspUrlChanged);
   connect(_video2Settings->tcpUrl(),        &Fact::rawValueChanged, this, &VideoManager::_tcpUrlChanged);
   connect(_video2Settings->aspectRatio(),   &Fact::rawValueChanged, this, &VideoManager::_aspectRatioChanged);
   connect(_video2Settings->lowLatencyMode(),&Fact::rawValueChanged, this, &VideoManager::_lowLatencyModeChanged);
   }


   MultiVehicleManager *pVehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
   connect(pVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &VideoManager::_setActiveVehicle);

#if defined(QGC_GST_STREAMING)
    if(videoSettingNumber == 1){
      GStreamer::blacklist(static_cast<Video1Settings::VideoDecoderOptions>(_video1Settings->forceVideoDecoder()->rawValue().toInt()));
    }else{ // 2
      GStreamer::blacklist(static_cast<Video2Settings::VideoDecoderOptions>(_video2Settings->forceVideoDecoder()->rawValue().toInt()));
    }
#ifndef QGC_DISABLE_UVC
   // If we are using a UVC camera setup the device name
   _updateUVC();
#endif

    emit isGStreamerChanged();
    qCDebug(VideoManagerLog) << "New Video Source:" << videoSource;
#if defined(QGC_GST_STREAMING)
    _videoReceiver[0] = toolbox->corePlugin()->createVideoReceiver(this);
    _videoReceiver[1] = toolbox->corePlugin()->createVideoReceiver(this);

    connect(_videoReceiver[0], &VideoReceiver::streamingChanged, this, [this](bool active){
        _streaming = active;
        emit streamingChanged();
    });

    connect(_videoReceiver[0], &VideoReceiver::onStartComplete, this, [this](VideoReceiver::STATUS status) {
        if (status == VideoReceiver::STATUS_OK) {
            _videoStarted[0] = true;
            if (_videoSink[0] != nullptr) {
                // It is absolytely ok to have video receiver active (streaming) and decoding not active
                // It should be handy for cases when you have many streams and want to show only some of them
                // NOTE that even if decoder did not start it is still possible to record video
                _videoReceiver[0]->startDecoding(_videoSink[0]);
            }
        } else if (status == VideoReceiver::STATUS_INVALID_URL) {
            // Invalid URL - don't restart
        } else if (status == VideoReceiver::STATUS_INVALID_STATE) {
            // Already running
        } else {
            _restartVideo(0);
        }
    });

    connect(_videoReceiver[0], &VideoReceiver::onStopComplete, this, [this](VideoReceiver::STATUS) {
        _videoStarted[0] = false;
        _startReceiver(0);
    });

    connect(_videoReceiver[0], &VideoReceiver::decodingChanged, this, [this](bool active){
        _decoding = active;
        emit decodingChanged();
    });

    connect(_videoReceiver[0], &VideoReceiver::recordingChanged, this, [this](bool active){
        _recording = active;
        if (!active) {
            _subtitleWriter.stopCapturingTelemetry();
        }
        emit recordingChanged();
    });

    connect(_videoReceiver[0], &VideoReceiver::recordingStarted, this, [this](){
        _subtitleWriter.startCapturingTelemetry(_videoFile);
    });

    connect(_videoReceiver[0], &VideoReceiver::videoSizeChanged, this, [this](QSize size){
        _videoSize = ((quint32)size.width() << 16) | (quint32)size.height();
        emit videoSizeChanged();
    });

    //connect(_videoReceiver, &VideoReceiver::onTakeScreenshotComplete, this, [this](VideoReceiver::STATUS status){
    //    if (status == VideoReceiver::STATUS_OK) {
    //    }
    //});

    // FIXME: AV: I believe _thermalVideoReceiver should be handled just like _videoReceiver in terms of event
    // and I expect that it will be changed during multiple video stream activity
    if (_videoReceiver[1] != nullptr) {
        connect(_videoReceiver[1], &VideoReceiver::onStartComplete, this, [this](VideoReceiver::STATUS status) {
            if (status == VideoReceiver::STATUS_OK) {
                _videoStarted[1] = true;
                if (_videoSink[1] != nullptr) {
                    _videoReceiver[1]->startDecoding(_videoSink[1]);
                }
            } else if (status == VideoReceiver::STATUS_INVALID_URL) {
                // Invalid URL - don't restart
            } else if (status == VideoReceiver::STATUS_INVALID_STATE) {
                // Already running
            } else {
                _restartVideo(1);
            }
        });

        connect(_videoReceiver[1], &VideoReceiver::onStopComplete, this, [this](VideoReceiver::STATUS) {
            _videoStarted[1] = false;
            _startReceiver(1);
        });
    }
#endif
    _updateSettings(0);
    _updateSettings(1);
    if(isGStreamer()) {
        startVideo();
    } else {
        stopVideo();
    }

#endif
}



void VideoManager::_cleanupOldVideos()
{
#if defined(QGC_GST_STREAMING)
    //-- Only perform cleanup if storage limit is enabled
   if(videoSettingNumber == 1){
      if(!_video1Settings->enableStorageLimit()->rawValue().toBool()) return;
   } else { // 2
      if(!_video2Settings->enableStorageLimit()->rawValue().toBool()) return;
   }
    
    QString savePath = qgcApp()->toolbox()->settingsManager()->appSettings()->videoSavePath();
    QDir videoDir = QDir(savePath);
    videoDir.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
    videoDir.setSorting(QDir::Time);

    QStringList nameFilters;

    for(size_t i = 0; i < sizeof(kFileExtension) / sizeof(kFileExtension[0]); i += 1) {
        nameFilters << QString("*.") + kFileExtension[i];
    }

    videoDir.setNameFilters(nameFilters);
    //-- get the list of videos stored
    QFileInfoList vidList = videoDir.entryInfoList();
    if(!vidList.isEmpty()) {
        uint64_t total   = 0;
        //-- Settings are stored using MB
        uint64_t maxSize;

        if(videoSettingNumber == 1){
            maxSize = _video1Settings->maxVideoSize()->rawValue().toUInt() * 1024 * 1024;
        } else { // 2
            maxSize = _video2Settings->maxVideoSize()->rawValue().toUInt() * 1024 * 1024;
        }
         
        //-- Compute total used storage
        for(int i = 0; i < vidList.size(); i++) {
            total += vidList[i].size();
        }
        //-- Remove old movies until max size is satisfied.
        while(total >= maxSize && !vidList.isEmpty()) {
            total -= vidList.last().size();
            qCDebug(VideoManagerLog) << "Removing old video file:" << vidList.last().filePath();
            QFile file (vidList.last().filePath());
            file.remove();
            vidList.removeLast();
        }
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::startVideo()
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }
    if(videoSettingNumber == 1){
      if(!_video1Settings->streamEnabled()->rawValue().toBool() || !_video1Settings->streamConfigured()) {
        qCDebug(VideoManagerLog) << "Stream not enabled/configured";
        return;
      }
    }else{ // 2
      if(!_video2Settings->streamEnabled()->rawValue().toBool() || !_video2Settings->streamConfigured()) {
        qCDebug(VideoManagerLog) << "Stream not enabled/configured";
        return;
      }
    }

    _startReceiver(0);
    _startReceiver(1);
}

//-----------------------------------------------------------------------------
void
VideoManager::stopVideo()
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }

    _stopReceiver(1);
    _stopReceiver(0);
}

void
VideoManager::startRecording(const QString& videoFile)
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }
#if defined(QGC_GST_STREAMING)
    if (!_videoReceiver[0]) {
        qgcApp()->showAppMessage(tr("Video receiver is not ready."));
        return;
    }

    VideoReceiver::FILE_FORMAT fileFormat;
    if(videoSettingNumber == 1) fileFormat = static_cast<VideoReceiver::FILE_FORMAT>(_video1Settings->recordingFormat()->rawValue().toInt());
    else fileFormat = static_cast<VideoReceiver::FILE_FORMAT>(_video2Settings->recordingFormat()->rawValue().toInt());

    if(fileFormat < VideoReceiver::FILE_FORMAT_MIN || fileFormat >= VideoReceiver::FILE_FORMAT_MAX) {
        qgcApp()->showAppMessage(tr("Invalid video format defined."));
        return;
    }
    QString ext = kFileExtension[fileFormat - VideoReceiver::FILE_FORMAT_MIN];

    //-- Disk usage maintenance
    _cleanupOldVideos();

    QString savePath = qgcApp()->toolbox()->settingsManager()->appSettings()->videoSavePath();

    if (savePath.isEmpty()) {
        qgcApp()->showAppMessage(tr("Unabled to record video. Video save path must be specified in Settings."));
        return;
    }

    _videoFile = savePath + "/"
            + (videoFile.isEmpty() ? QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss") : videoFile)
            + ".";
    QString videoFile2 = _videoFile + "2." + ext;
    _videoFile += ext;

    if (_videoReceiver[0] && _videoStarted[0]) {
        _videoReceiver[0]->startRecording(_videoFile, fileFormat);
    }
    if (_videoReceiver[1] && _videoStarted[1]) {
        _videoReceiver[1]->startRecording(videoFile2, fileFormat);
    }

#else
    Q_UNUSED(videoFile)
#endif
}

void
VideoManager::stopRecording()
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }
#if defined(QGC_GST_STREAMING)

    for (int i = 0; i < 2; i++) {
        if (_videoReceiver[i]) {
            _videoReceiver[i]->stopRecording();
        }
    }
#endif
}

void
VideoManager::grabImage(const QString& imageFile)
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }
#if defined(QGC_GST_STREAMING)
    if (!_videoReceiver[0]) {
        return;
    }

    if (imageFile.isEmpty()) {
        _imageFile = qgcApp()->toolbox()->settingsManager()->appSettings()->photoSavePath();
        _imageFile += + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss.zzz") + ".jpg";
    } else {
        _imageFile = imageFile;
    }

    emit imageFileChanged();

    _videoReceiver[0]->takeScreenshot(_imageFile);
#else
    Q_UNUSED(imageFile)
#endif
}

//-----------------------------------------------------------------------------
double VideoManager::aspectRatio()
{
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if(pInfo) {
            qCDebug(VideoManagerLog) << "Primary AR: " << pInfo->aspectRatio();
            return pInfo->aspectRatio();
        }
    }
    // FIXME: AV: use _videoReceiver->videoSize() to calculate AR (if AR is not specified in the settings?)
    if(videoSettingNumber == 1){
      return _video1Settings->aspectRatio()->rawValue().toDouble();
    }else{ // 2
      return _video2Settings->aspectRatio()->rawValue().toDouble();
    }
    
}

//-----------------------------------------------------------------------------
double VideoManager::thermalAspectRatio()
{
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->cameraManager()->thermalStreamInstance();
        if(pInfo) {
            qCDebug(VideoManagerLog) << "Thermal AR: " << pInfo->aspectRatio();
            return pInfo->aspectRatio();
        }
    }
    return 1.0;
}

//-----------------------------------------------------------------------------
double VideoManager::hfov()
{
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if(pInfo) {
            return pInfo->hfov();
        }
    }
    return 1.0;
}

//-----------------------------------------------------------------------------
double VideoManager::thermalHfov()
{
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->cameraManager()->thermalStreamInstance();
        if(pInfo) {
            return pInfo->aspectRatio();
        }
    }
    if(videoSettingNumber == 1){
      return _video1Settings->aspectRatio()->rawValue().toDouble();
    }else{ // 2
      return _video2Settings->aspectRatio()->rawValue().toDouble();
    }
    
}

//-----------------------------------------------------------------------------
bool
VideoManager::hasThermal()
{
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->cameraManager()->thermalStreamInstance();
        if(pInfo) {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
QString
VideoManager::imageFile()
{
    return _imageFile;
}

//-----------------------------------------------------------------------------
bool
VideoManager::autoStreamConfigured()
{
#if defined(QGC_GST_STREAMING)
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if(pInfo) {
            return !pInfo->uri().isEmpty();
        }
    }
#endif
    return false;
}

//-----------------------------------------------------------------------------
void
VideoManager::_updateUVC()
{
#ifndef QGC_DISABLE_UVC
    QString videoSource;
    if(videoSettingNumber == 1){
      videoSource = _video1Settings->videoSource()->rawValue().toString();
   }else{ // 2
      videoSource = _video2Settings->videoSource()->rawValue().toString();
   }
     
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo: cameras) {
        if(cameraInfo.description() == videoSource) {
            _videoSourceID = cameraInfo.deviceName();
            emit videoSourceIDChanged();
            qCDebug(VideoManagerLog) << "Found USB source:" << _videoSourceID << " Name:" << videoSource;
            break;
        }
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_videoSourceChanged()
{
    _updateUVC();
    emit hasVideoChanged();
    emit isGStreamerChanged();
    emit isAutoStreamChanged();
    _restartVideo(0);
}

//-----------------------------------------------------------------------------
void
VideoManager::_udpPortChanged()
{
    _restartVideo(0);
}

//-----------------------------------------------------------------------------
void
VideoManager::_rtspUrlChanged()
{
    _restartVideo(0);
}

//-----------------------------------------------------------------------------
void
VideoManager::_tcpUrlChanged()
{
    _restartVideo(0);
}

//-----------------------------------------------------------------------------
void
VideoManager::_lowLatencyModeChanged()
{
    _restartAllVideos();
}

//-----------------------------------------------------------------------------
bool
VideoManager::hasVideo()
{
    if(autoStreamConfigured()) {
        return true;
    }
    if(videoSettingNumber == 1){
      QString videoSource = _video1Settings->videoSource()->rawValue().toString();
      return !videoSource.isEmpty() && videoSource != Video1Settings::videoSourceNoVideo && videoSource != Video1Settings::videoDisabled;
    }else{ // 2
      QString videoSource = _video2Settings->videoSource()->rawValue().toString();
      return !videoSource.isEmpty() && videoSource != Video2Settings::videoSourceNoVideo && videoSource != Video2Settings::videoDisabled;
    }
    
}

//-----------------------------------------------------------------------------
bool
VideoManager::isGStreamer()
{
#if defined(QGC_GST_STREAMING)
    if(videoSettingNumber == 1){
      QString videoSource = _video1Settings->videoSource()->rawValue().toString();
      return videoSource == Video1Settings::videoSourceUDPH264 ||
               videoSource == Video1Settings::videoSourceUDPH265 ||
               videoSource == Video1Settings::videoSourceRTSP ||
               videoSource == Video1Settings::videoSourceTCP ||
               videoSource == Video1Settings::videoSourceMPEGTS ||
               videoSource == Video1Settings::videoSource3DRSolo ||
               videoSource == Video1Settings::videoSourceParrotDiscovery ||
               autoStreamConfigured();
    }else{
       QString videoSource = _video2Settings->videoSource()->rawValue().toString();
       return videoSource == Video2Settings::videoSourceUDPH264 ||
               videoSource == Video2Settings::videoSourceUDPH265 ||
               videoSource == Video2Settings::videoSourceRTSP ||
               videoSource == Video2Settings::videoSourceTCP ||
               videoSource == Video2Settings::videoSourceMPEGTS ||
               videoSource == Video2Settings::videoSource3DRSolo ||
               videoSource == Video2Settings::videoSourceParrotDiscovery ||
               autoStreamConfigured();      
    }
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------
#ifndef QGC_DISABLE_UVC
bool
VideoManager::uvcEnabled()
{
    return QCameraInfo::availableCameras().count() > 0;
}
#endif

//-----------------------------------------------------------------------------
void
VideoManager::setfullScreen(bool f)
{
    if(f) {
        //-- No can do if no vehicle or connection lost
        if(!_activeVehicle || _activeVehicle->vehicleLinkManager()->communicationLost()) {
            f = false;
        }
    }
    _fullScreen = f;
    emit fullScreenChanged();
}

//-----------------------------------------------------------------------------
void
VideoManager::_initVideo()
{
#if defined(QGC_GST_STREAMING)
    QQuickItem* root = qgcApp()->mainRootWindow();

    if (root == nullptr) {
        qCDebug(VideoManagerLog) << "mainRootWindow() failed. No root window";
        return;
    }

    QQuickItem* widget;
    
    if(videoSettingNumber == 1){
     widget = root->findChild<QQuickItem*>("video1Content");
    }else{ // 2
      widget = root->findChild<QQuickItem*>("video2Content");
    }

    if (widget != nullptr && _videoReceiver[0] != nullptr) {
        _videoSink[0] = qgcApp()->toolbox()->corePlugin()->createVideoSink(this, widget);
        if (_videoSink[0] != nullptr) {
            if (_videoStarted[0]) {
                _videoReceiver[0]->startDecoding(_videoSink[0]);
            }
        } else {
            qCDebug(VideoManagerLog) << "createVideoSink() failed";
        }
    } else {
        qCDebug(VideoManagerLog) << "video receiver disabled";
    }

    if(videoSettingNumber == 1){
      widget = root->findChild<QQuickItem*>("thermal1Video");
    }else{ // 2
      widget = root->findChild<QQuickItem*>("thermal2Video");
    }

    if (widget != nullptr && _videoReceiver[1] != nullptr) {
        _videoSink[1] = qgcApp()->toolbox()->corePlugin()->createVideoSink(this, widget);
        if (_videoSink[1] != nullptr) {
            if (_videoStarted[1]) {
                _videoReceiver[1]->startDecoding(_videoSink[1]);
            }
        } else {
            qCDebug(VideoManagerLog) << "createVideoSink() failed";
        }
    } else {
        qCDebug(VideoManagerLog) << "thermal video receiver disabled";
    }
#endif
}

//-----------------------------------------------------------------------------
bool
VideoManager::_updateSettings(unsigned id)
{
    if(videoSettingNumber == 1){
      if(!_video1Settings) return false;
    }else{ // 2
      if(!_video2Settings) return false;
    }

   bool lowLatencyStreaming;
    if(videoSettingNumber == 1){
      lowLatencyStreaming = _video1Settings->lowLatencyMode()->rawValue().toBool();
    }else{ // 2
      lowLatencyStreaming = _video2Settings->lowLatencyMode()->rawValue().toBool();
    }
      

    bool settingsChanged = _lowLatencyStreaming[id] != lowLatencyStreaming;

    _lowLatencyStreaming[id] = lowLatencyStreaming;

    //-- Auto discovery

    if(_activeVehicle && _activeVehicle->cameraManager()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if(pInfo) {
            if (id == 0) {
                qCDebug(VideoManagerLog) << "Configure primary stream:" << pInfo->uri();
                switch(pInfo->type()) {
                    case VIDEO_STREAM_TYPE_RTSP:
                        if ((settingsChanged |= _updateVideoUri(id, pInfo->uri()))) {
                            if(videoSettingNumber == 1) _toolbox->settingsManager()->video1Settings()->videoSource()->setRawValue(Video1Settings::videoSourceRTSP);
                            else _toolbox->settingsManager()->video2Settings()->videoSource()->setRawValue(Video2Settings::videoSourceRTSP);
                        }
                        break;
                    case VIDEO_STREAM_TYPE_TCP_MPEG:
                        if ((settingsChanged |= _updateVideoUri(id, pInfo->uri()))) {
                            if(videoSettingNumber == 1) _toolbox->settingsManager()->video1Settings()->videoSource()->setRawValue(Video1Settings::videoSourceTCP);
                            else _toolbox->settingsManager()->video2Settings()->videoSource()->setRawValue(Video2Settings::videoSourceTCP);
                        }
                        break;
                    case VIDEO_STREAM_TYPE_RTPUDP:
                        if ((settingsChanged |= _updateVideoUri(id, QStringLiteral("udp://0.0.0.0:%1").arg(pInfo->uri())))) {
                            if(videoSettingNumber == 1) _toolbox->settingsManager()->video1Settings()->videoSource()->setRawValue(Video1Settings::videoSourceUDPH264);
                            else _toolbox->settingsManager()->video2Settings()->videoSource()->setRawValue(Video2Settings::videoSourceUDPH264);
                        }
                        break;
                    case VIDEO_STREAM_TYPE_MPEG_TS_H264:
                        if ((settingsChanged |= _updateVideoUri(id, QStringLiteral("mpegts://0.0.0.0:%1").arg(pInfo->uri())))) {
                            if(videoSettingNumber == 1) _toolbox->settingsManager()->video1Settings()->videoSource()->setRawValue(Video1Settings::videoSourceMPEGTS);
                            else _toolbox->settingsManager()->video2Settings()->videoSource()->setRawValue(Video2Settings::videoSourceMPEGTS);
                        }
                        break;
                    default:
                        settingsChanged |= _updateVideoUri(id, pInfo->uri());
                        break;
                }
            }
            else if (id == 1) { //-- Thermal stream (if any)
                QGCVideoStreamInfo* pTinfo = _activeVehicle->cameraManager()->thermalStreamInstance();
                if (pTinfo) {
                    qCDebug(VideoManagerLog) << "Configure secondary stream:" << pTinfo->uri();
                    switch(pTinfo->type()) {
                        case VIDEO_STREAM_TYPE_RTSP:
                        case VIDEO_STREAM_TYPE_TCP_MPEG:
                            settingsChanged |= _updateVideoUri(id, pTinfo->uri());
                            break;
                        case VIDEO_STREAM_TYPE_RTPUDP:
                            settingsChanged |= _updateVideoUri(id, QStringLiteral("udp://0.0.0.0:%1").arg(pTinfo->uri()));
                            break;
                        case VIDEO_STREAM_TYPE_MPEG_TS_H264:
                            settingsChanged |= _updateVideoUri(id, QStringLiteral("mpegts://0.0.0.0:%1").arg(pTinfo->uri()));
                            break;
                        default:
                            settingsChanged |= _updateVideoUri(id, pTinfo->uri());
                            break;
                    }
                }
            }
            return settingsChanged;
        }
    }
    if(videoSettingNumber == 1){
      QString source = _video1Settings->videoSource()->rawValue().toString();
      if (source == Video1Settings::videoSourceUDPH264)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("udp://0.0.0.0:%1").arg(_video1Settings->udpPort()->rawValue().toInt()));
      else if (source == Video1Settings::videoSourceUDPH265)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("udp265://0.0.0.0:%1").arg(_video1Settings->udpPort()->rawValue().toInt()));
      else if (source == Video1Settings::videoSourceMPEGTS)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("mpegts://0.0.0.0:%1").arg(_video1Settings->udpPort()->rawValue().toInt()));
      else if (source == Video1Settings::videoSourceRTSP)
         settingsChanged |= _updateVideoUri(0, _video1Settings->rtspUrl()->rawValue().toString());
      else if (source == Video1Settings::videoSourceTCP)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("tcp://%1").arg(_video1Settings->tcpUrl()->rawValue().toString()));
      else if (source == Video1Settings::videoSource3DRSolo)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("udp://0.0.0.0:5600"));
      else if (source == Video1Settings::videoSourceParrotDiscovery)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("udp://0.0.0.0:8888"));
    }else{ // 2
      QString source = _video2Settings->videoSource()->rawValue().toString();
      if (source == Video2Settings::videoSourceUDPH264)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("udp://0.0.0.0:%1").arg(_video2Settings->udpPort()->rawValue().toInt()));
      else if (source == Video2Settings::videoSourceUDPH265)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("udp265://0.0.0.0:%1").arg(_video2Settings->udpPort()->rawValue().toInt()));
      else if (source == Video2Settings::videoSourceMPEGTS)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("mpegts://0.0.0.0:%1").arg(_video2Settings->udpPort()->rawValue().toInt()));
      else if (source == Video2Settings::videoSourceRTSP)
         settingsChanged |= _updateVideoUri(0, _video2Settings->rtspUrl()->rawValue().toString());
      else if (source == Video2Settings::videoSourceTCP)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("tcp://%1").arg(_video2Settings->tcpUrl()->rawValue().toString()));
      else if (source == Video2Settings::videoSource3DRSolo)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("udp://0.0.0.0:5600"));
      else if (source == Video2Settings::videoSourceParrotDiscovery)
         settingsChanged |= _updateVideoUri(0, QStringLiteral("udp://0.0.0.0:8888"));
    }

    return settingsChanged;
}

//-----------------------------------------------------------------------------
bool
VideoManager::_updateVideoUri(unsigned id, const QString& uri)
{
#if defined(QGC_GST_TAISYNC_ENABLED) && (defined(__android__) || defined(__ios__))
    //-- Taisync on iOS or Android sends a raw h.264 stream
    if (isTaisync()) {
        if (id == 0) {
            return _updateVideoUri(0, QString("tsusb://0.0.0.0:%1").arg(TAISYNC_VIDEO_UDP_PORT));
        } if (id == 1) {
            // FIXME: AV: TAISYNC_VIDEO_UDP_PORT is used by video stream, thermal stream should go via its own proxy
            if (!_videoUri[1].isEmpty()) {
                _videoUri[1].clear();
                return true;
            } else {
                return false;
            }
        }
    }
#endif
    if (uri == _videoUri[id]) {
        return false;
    }

    _videoUri[id] = uri;

    return true;
}

//-----------------------------------------------------------------------------
void
VideoManager::_restartVideo(unsigned id)
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }

#if defined(QGC_GST_STREAMING)
    bool oldLowLatencyStreaming = _lowLatencyStreaming[id];
    QString oldUri = _videoUri[id];
    _updateSettings(id);
    bool newLowLatencyStreaming = _lowLatencyStreaming[id];
    QString newUri = _videoUri[id];

    // FIXME: AV: use _updateSettings() result to check if settings were changed
    if (oldUri == newUri && oldLowLatencyStreaming == newLowLatencyStreaming && _videoStarted[id]) {
        qCDebug(VideoManagerLog) << "No sense to restart video streaming, skipped"  << id;
        return;
    }

    qCDebug(VideoManagerLog) << "Restart video streaming"  << id;

    if (_videoStarted[id]) {
        _stopReceiver(id);
    } else {
        _startReceiver(id);
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_restartAllVideos()
{
    _restartVideo(0);
    _restartVideo(1);
}

//----------------------------------------------------------------------------------------
void
VideoManager::_startReceiver(unsigned id)
{
#if defined(QGC_GST_STREAMING)
    unsigned timeout;
    
    if(videoSettingNumber == 1) timeout = _video1Settings->rtspTimeout()->rawValue().toUInt();
    else timeout = _video2Settings->rtspTimeout()->rawValue().toUInt();

    if (id > 1) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
    } else if (_videoReceiver[id] != nullptr/* && _videoSink[id] != nullptr*/) {
        if (!_videoUri[id].isEmpty()) {
            _videoReceiver[id]->start(_videoUri[id], timeout, _lowLatencyStreaming[id] ? -1 : 0);
        }
    }
#endif
}

//----------------------------------------------------------------------------------------
void
VideoManager::_stopReceiver(unsigned id)
{
#if defined(QGC_GST_STREAMING)
    if (id > 1) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
    } else if (_videoReceiver[id] != nullptr) {
        _videoReceiver[id]->stop();
    }
#endif
}

//----------------------------------------------------------------------------------------
void
VideoManager::_setActiveVehicle(Vehicle* vehicle)
{
    if(_activeVehicle) {
        disconnect(_activeVehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this, &VideoManager::_communicationLostChanged);
        if(_activeVehicle->cameraManager()) {
            QGCCameraControl* pCamera = _activeVehicle->cameraManager()->currentCameraInstance();
            if(pCamera) {
                pCamera->stopStream();
            }
            disconnect(_activeVehicle->cameraManager(), &QGCCameraManager::streamChanged, this, &VideoManager::_restartAllVideos);
        }
    }
    _activeVehicle = vehicle;
    if(_activeVehicle) {
        connect(_activeVehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this, &VideoManager::_communicationLostChanged);
        if(_activeVehicle->cameraManager()) {
            connect(_activeVehicle->cameraManager(), &QGCCameraManager::streamChanged, this, &VideoManager::_restartAllVideos);
            QGCCameraControl* pCamera = _activeVehicle->cameraManager()->currentCameraInstance();
            if(pCamera) {
                pCamera->resumeStream();
            }
        }
    } else {
        //-- Disable full screen video if vehicle is gone
        setfullScreen(false);
    }
    emit autoStreamConfiguredChanged();
    _restartAllVideos();
}

//----------------------------------------------------------------------------------------
void
VideoManager::_communicationLostChanged(bool connectionLost)
{
    if(connectionLost) {
        //-- Disable full screen video if connection is lost
        setfullScreen(false);
    }
}

//----------------------------------------------------------------------------------------
void
VideoManager::_aspectRatioChanged()
{
    emit aspectRatioChanged();
}
