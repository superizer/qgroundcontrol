/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Video1Settings.h"
#include "QGCApplication.h"
#include "VideoManager.h"

#include <QQmlEngine>
#include <QtQml>
#include <QVariantList>

#ifndef QGC_DISABLE_UVC
#include <QCameraInfo>
#endif

const char* Video1Settings::videoSourceNoVideo           = "No Video Available";
const char* Video1Settings::videoDisabled                = "Video Stream Disabled";
const char* Video1Settings::videoSourceRTSP              = "RTSP Video Stream";
const char* Video1Settings::videoSourceUDPH264           = "UDP h.264 Video Stream";
const char* Video1Settings::videoSourceUDPH265           = "UDP h.265 Video Stream";
const char* Video1Settings::videoSourceTCP               = "TCP-MPEG2 Video Stream";
const char* Video1Settings::videoSourceMPEGTS            = "MPEG-TS (h.264) Video Stream";
const char* Video1Settings::videoSource3DRSolo           = "3DR Solo (requires restart)";
const char* Video1Settings::videoSourceParrotDiscovery   = "Parrot Discovery";

DECLARE_SETTINGGROUP(Video1, "Video1")
{
    qmlRegisterUncreatableType<Video1Settings>("QGroundControl.SettingsManager", 1, 0, "Video1Settings", "Reference only");

    // Setup enum values for videoSource settings into meta data
    QStringList videoSourceList;
#ifdef QGC_GST_STREAMING
    videoSourceList.append(videoSourceRTSP);
#ifndef NO_UDP_VIDEO
    videoSourceList.append(videoSourceUDPH264);
    videoSourceList.append(videoSourceUDPH265);
#endif
    videoSourceList.append(videoSourceTCP);
    videoSourceList.append(videoSourceMPEGTS);
    videoSourceList.append(videoSource3DRSolo);
    videoSourceList.append(videoSourceParrotDiscovery);
#endif
#ifndef QGC_DISABLE_UVC
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo: cameras) {
        videoSourceList.append(cameraInfo.description());
    }
#endif
    if (videoSourceList.count() == 0) {
        _noVideo = true;
        videoSourceList.append(videoSourceNoVideo);
    } else {
        videoSourceList.insert(0, videoDisabled);
    }
    QVariantList videoSourceVarList;
    for (const QString& videoSource: videoSourceList) {
        videoSourceVarList.append(QVariant::fromValue(videoSource));
    }
    _nameToMetaDataMap[videoSourceName]->setEnumInfo(videoSourceList, videoSourceVarList);

    const QVariantList removeForceVideoDecodeList{
#ifdef Q_OS_LINUX
        VideoDecoderOptions::ForceVideoDecoderDirectX3D,
        VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
#endif
#ifdef Q_OS_WIN
        VideoDecoderOptions::ForceVideoDecoderVAAPI,
        VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
#endif
#ifdef Q_OS_MAC
        VideoDecoderOptions::ForceVideoDecoderDirectX3D,
        VideoDecoderOptions::ForceVideoDecoderVAAPI,
#endif
    };

    for(const auto& value : removeForceVideoDecodeList) {
        _nameToMetaDataMap[forceVideoDecoderName]->removeEnumInfo(value);
    }

    // Set default value for videoSource
    _setDefaults();
}

void Video1Settings::_setDefaults()
{
    if (_noVideo) {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoSourceNoVideo);
    } else {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoDisabled);
    }
}

DECLARE_SETTINGSFACT(Video1Settings, aspectRatio)
DECLARE_SETTINGSFACT(Video1Settings, videoFit)
DECLARE_SETTINGSFACT(Video1Settings, gridLines)
DECLARE_SETTINGSFACT(Video1Settings, showRecControl)
DECLARE_SETTINGSFACT(Video1Settings, recordingFormat)
DECLARE_SETTINGSFACT(Video1Settings, maxVideoSize)
DECLARE_SETTINGSFACT(Video1Settings, enableStorageLimit)
DECLARE_SETTINGSFACT(Video1Settings, rtspTimeout)
DECLARE_SETTINGSFACT(Video1Settings, streamEnabled)
DECLARE_SETTINGSFACT(Video1Settings, disableWhenDisarmed)
DECLARE_SETTINGSFACT(Video1Settings, lowLatencyMode)

DECLARE_SETTINGSFACT_NO_FUNC(Video1Settings, videoSource)
{
    if (!_videoSourceFact) {
        _videoSourceFact = _createSettingsFact(videoSourceName);
        //-- Check for sources no longer available
        if(!_videoSourceFact->enumStrings().contains(_videoSourceFact->rawValue().toString())) {
            if (_noVideo) {
                _videoSourceFact->setRawValue(videoSourceNoVideo);
            } else {
                _videoSourceFact->setRawValue(videoDisabled);
            }
        }
        connect(_videoSourceFact, &Fact::valueChanged, this, &Video1Settings::_configChanged);
    }
    return _videoSourceFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(Video1Settings, forceVideoDecoder)
{
    if (!_forceVideoDecoderFact) {
        _forceVideoDecoderFact = _createSettingsFact(forceVideoDecoderName);

        _forceVideoDecoderFact->setVisible(
#ifdef Q_OS_IOS
            false
#else
#ifdef Q_OS_ANDROID
            false
#else
            true
#endif
#endif
        );

        connect(_forceVideoDecoderFact, &Fact::valueChanged, this, &Video1Settings::_configChanged);
    }
    return _forceVideoDecoderFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(Video1Settings, udpPort)
{
    if (!_udpPortFact) {
        _udpPortFact = _createSettingsFact(udpPortName);
        connect(_udpPortFact, &Fact::valueChanged, this, &Video1Settings::_configChanged);
    }
    return _udpPortFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(Video1Settings, rtspUrl)
{
    if (!_rtspUrlFact) {
        _rtspUrlFact = _createSettingsFact(rtspUrlName);
        connect(_rtspUrlFact, &Fact::valueChanged, this, &Video1Settings::_configChanged);
    }
    return _rtspUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(Video1Settings, tcpUrl)
{
    if (!_tcpUrlFact) {
        _tcpUrlFact = _createSettingsFact(tcpUrlName);
        connect(_tcpUrlFact, &Fact::valueChanged, this, &Video1Settings::_configChanged);
    }
    return _tcpUrlFact;
}

bool Video1Settings::streamConfigured(void)
{
#if !defined(QGC_GST_STREAMING)
    return false;
#endif
    //-- First, check if it's autoconfigured
    if(qgcApp()->toolbox()->videoManager1()->autoStreamConfigured()) {
        qCDebug(VideoManagerLog) << "Stream auto configured";
        return true;
    }
    //-- Check if it's disabled
    QString vSource = videoSource()->rawValue().toString();
    if(vSource == videoSourceNoVideo || vSource == videoDisabled) {
        return false;
    }
    //-- If UDP, check if port is set
    if(vSource == videoSourceUDPH264 || vSource == videoSourceUDPH265) {
        qCDebug(VideoManagerLog) << "Testing configuration for UDP Stream:" << udpPort()->rawValue().toInt();
        return udpPort()->rawValue().toInt() != 0;
    }
    //-- If RTSP, check for URL
    if(vSource == videoSourceRTSP) {
        qCDebug(VideoManagerLog) << "Testing configuration for RTSP Stream:" << rtspUrl()->rawValue().toString();
        return !rtspUrl()->rawValue().toString().isEmpty();
    }
    //-- If TCP, check for URL
    if(vSource == videoSourceTCP) {
        qCDebug(VideoManagerLog) << "Testing configuration for TCP Stream:" << tcpUrl()->rawValue().toString();
        return !tcpUrl()->rawValue().toString().isEmpty();
    }
    //-- If MPEG-TS, check if port is set
    if(vSource == videoSourceMPEGTS) {
        qCDebug(VideoManagerLog) << "Testing configuration for MPEG-TS Stream:" << udpPort()->rawValue().toInt();
        return udpPort()->rawValue().toInt() != 0;
    }
    return false;
}

void VideoSettings1::_configChanged(QVariant)
{
    emit streamConfiguredChanged(streamConfigured());
}
