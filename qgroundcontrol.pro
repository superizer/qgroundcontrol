# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Author:
# Lorenz Meier <mavteam@student.ethz.ch>
# (c) 2009-2010 PIXHAWK Team
# This file is part of the mav groundstation project
# QGroundControl is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# QGroundControl is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with QGroundControl. If not, see <http://www.gnu.org/licenses/>.
# -------------------------------------------------
# Include QMapControl map library
# prefer version from external directory /
# from http://github.com/pixhawk/qmapcontrol/
# over bundled version in lib directory
# Version from GIT repository is preferred
# include ( "../qmapcontrol/QMapControl/QMapControl.pri" ) #{
# Include bundled version if necessary
include(lib/QMapControl/QMapControl.pri)

# message("Including bundled QMapControl version as FALLBACK. This is fine on Linux and MacOS, but not the best choice in Windows")
QT += network \
    opengl \
    svg \
    xml \
    phonon
TEMPLATE = app
TARGET = qgroundcontrol
BASEDIR = $$IN_PWD
TARGETDIR = $$OUT_PWD
BUILDDIR = $$TARGETDIR/build
LANGUAGE = C++
CONFIG += console
OBJECTS_DIR = $$BUILDDIR/obj
MOC_DIR = $$BUILDDIR/moc
UI_HEADERS_DIR = src/ui/generated
MAVLINK_CONF = ""

# If the user config file exists, it will be included.
# if the variable MAVLINK_CONF contains the name of an
# additional project, QGroundControl includes the support
# of custom MAVLink messages of this project
exists(user_config.pri) { 
    include(user_config.pri)
    message("----- USING CUSTOM USER QGROUNDCONTROL CONFIG FROM user_config.pri -----")
    message("Adding support for additional MAVLink messages for: " $$MAVLINK_CONF)
    message("------------------------------------------------------------------------")
}
INCLUDEPATH += $$BASEDIR/../mavlink/include/common
contains(MAVLINK_CONF, pixhawk) { 
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$BASEDIR/../mavlink/include/common
    
    # PIXHAWK SPECIAL MESSAGES
    INCLUDEPATH += $$BASEDIR/../mavlink/include/pixhawk
    DEFINES += QGC_USE_PIXHAWK_MESSAGES
}
contains(MAVLINK_CONF, slugs) { 
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$BASEDIR/../mavlink/include/common
    
    # SLUGS SPECIAL MESSAGES
    INCLUDEPATH += $$BASEDIR/../mavlink/include/slugs
    DEFINES += QGC_USE_SLUGS_MESSAGES
}
contains(MAVLINK_CONF, ualberta) { 
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$BASEDIR/../mavlink/include/common
    
    # UALBERTA SPECIAL MESSAGES
    INCLUDEPATH += $$BASEDIR/../mavlink/include/ualberta
    DEFINES += QGC_USE_UALBERTA_MESSAGES
}
contains(MAVLINK_CONF, ardupilotmega) { 
    # Remove the default set - it is included anyway
    INCLUDEPATH -= $$BASEDIR/../mavlink/include/common
    
    # UALBERTA SPECIAL MESSAGES
    INCLUDEPATH += $$BASEDIR/../mavlink/include/ardupilotmega
    DEFINES += QGC_USE_ARDUPILOTMEGA_MESSAGES
}

# }
# Include general settings for MAVGround
# necessary as last include to override any non-acceptable settings
# done by the plugins above
include(qgroundcontrol.pri)

# QWT plot and QExtSerial depend on paths set by qgroundcontrol.pri
# Include serial port library
include(src/lib/qextserialport/qextserialport.pri)

# Include QWT plotting library
include(src/lib/qwt/qwt.pri)
DEPENDPATH += . \
    lib/QMapControl \
    lib/QMapControl/src \
    plugins
INCLUDEPATH += . \
    lib/QMapControl \
    $$BASEDIR/../mavlink/include

# ../mavlink/include \
# MAVLink/include \
# mavlink/include
# Input
FORMS += src/ui/MainWindow.ui \
    src/ui/CommSettings.ui \
    src/ui/SerialSettings.ui \
    src/ui/UASControl.ui \
    src/ui/UASList.ui \
    src/ui/UASInfo.ui \
    src/ui/Linechart.ui \
    src/ui/UASView.ui \
    src/ui/ParameterInterface.ui \
    src/ui/WaypointList.ui \
    src/ui/WaypointView.ui \
    src/ui/ObjectDetectionView.ui \
    src/ui/JoystickWidget.ui \
    src/ui/DebugConsole.ui \
    src/ui/MapWidget.ui \
    src/ui/XMLCommProtocolWidget.ui \
    src/ui/HDDisplay.ui \
    src/ui/MAVLinkSettingsWidget.ui \
    src/ui/AudioOutputWidget.ui \
    src/ui/QGCSensorSettingsWidget.ui \
    src/ui/watchdog/WatchdogControl.ui \
    src/ui/watchdog/WatchdogProcessView.ui \
    src/ui/watchdog/WatchdogView.ui \
    src/ui/QGCFirmwareUpdate.ui \
    src/ui/QGCPxImuFirmwareUpdate.ui \
    src/ui/QGCDataPlot2D.ui \
    src/ui/QGCRemoteControlView.ui \
    src/ui/QMap3D.ui

# src/ui/WaypointGlobalView.ui
INCLUDEPATH += src \
    src/ui \
    src/ui/linechart \
    src/ui/uas \
    src/ui/map \
    src/uas \
    src/comm \
    include/ui \
    src/input \
    src/lib/qmapcontrol \
    src/ui/mavlink \
    src/ui/param \
    src/ui/watchdog \
    src/ui/map3D
HEADERS += src/MG.h \
    src/Core.h \
    src/uas/UASInterface.h \
    src/uas/UAS.h \
    src/uas/UASManager.h \
    src/comm/LinkManager.h \
    src/comm/LinkInterface.h \
    src/comm/SerialLinkInterface.h \
    src/comm/SerialLink.h \
    src/comm/SerialSimulationLink.h \
    src/comm/ProtocolInterface.h \
    src/comm/MAVLinkProtocol.h \
    src/comm/AS4Protocol.h \
    src/ui/CommConfigurationWindow.h \
    src/ui/SerialConfigurationWindow.h \
    src/ui/MainWindow.h \
    src/ui/uas/UASControlWidget.h \
    src/ui/uas/UASListWidget.h \
    src/ui/uas/UASInfoWidget.h \
    src/ui/HUD.h \
    src/ui/linechart/LinechartWidget.h \
    src/ui/linechart/LinechartPlot.h \
    src/ui/linechart/Scrollbar.h \
    src/ui/linechart/ScrollZoomer.h \
    src/configuration.h \
    src/ui/uas/UASView.h \
    src/ui/CameraView.h \
    src/comm/MAVLinkSimulationLink.h \
    src/comm/UDPLink.h \
    src/ui/ParameterInterface.h \
    src/ui/WaypointList.h \
    src/Waypoint.h \
    src/ui/WaypointView.h \
    src/ui/ObjectDetectionView.h \
    src/input/JoystickInput.h \
    src/ui/JoystickWidget.h \
    src/ui/DebugConsole.h \
    src/ui/MapWidget.h \
    src/ui/XMLCommProtocolWidget.h \
    src/ui/mavlink/DomItem.h \
    src/ui/mavlink/DomModel.h \
    src/comm/MAVLinkXMLParser.h \
    src/ui/HDDisplay.h \
    src/ui/MAVLinkSettingsWidget.h \
    src/ui/AudioOutputWidget.h \
    src/GAudioOutput.h \
    src/LogCompressor.h \
    src/ui/QGCParamWidget.h \
    src/ui/QGCSensorSettingsWidget.h \
    src/ui/linechart/Linecharts.h \
    src/uas/SlugsMAV.h \
    src/uas/PxQuadMAV.h \
    src/uas/ArduPilotMegaMAV.h \
    src/comm/MAVLinkSyntaxHighlighter.h \
    src/ui/watchdog/WatchdogControl.h \
    src/ui/watchdog/WatchdogProcessView.h \
    src/ui/watchdog/WatchdogView.h \
    src/uas/UASWaypointManager.h \
    src/ui/HSIDisplay.h \
    src/QGC.h \
    src/ui/QGCFirmwareUpdate.h \
    src/ui/QGCPxImuFirmwareUpdate.h \
    src/comm/MAVLinkLightProtocol.h \
    src/ui/QGCDataPlot2D.h \
    src/ui/linechart/IncrementalPlot.h \
    src/ui/map/Waypoint2DIcon.h \
    src/ui/map/MAV2DIcon.h \
    src/ui/QGCRemoteControlView.h \
    src/ui/RadioCalibration/RadioCalibrationData.h \
    src/ui/RadioCalibration/RadioCalibrationWindow.h \
    src/ui/RadioCalibration/AirfoilServoCalibrator.h \
    src/ui/RadioCalibration/SwitchCalibrator.h \
    src/ui/RadioCalibration/CurveCalibrator.h \
    src/ui/RadioCalibration/AbstractCalibrator.h \
    src/comm/QGCMAVLink.h
	 # src/ui/WaypointGlobalView.h \


contains(DEPENDENCIES_PRESENT, osg) { 
    message("Including headers for OpenSceneGraph")
    
    # Enable only if OpenSceneGraph is available
    HEADERS += src/ui/map3D/Q3DWidget.h \
        src/ui/map3D/GCManipulator.h \
        src/ui/map3D/ImageWindowGeode.h \
        src/ui/map3D/QOSGWidget.h \
        src/ui/map3D/PixhawkCheetahGeode.h \
        src/ui/map3D/Pixhawk3DWidget.h \
        src/ui/map3D/Q3DWidgetFactory.h \
        src/ui/map3D/WebImageCache.h \
        src/ui/map3D/WebImage.h \
        src/ui/map3D/TextureCache.h \
        src/ui/map3D/Texture.h \
        src/ui/map3D/Imagery.h
    contains(DEPENDENCIES_PRESENT, osgearth) { 
        message("Including headers for OSGEARTH")
        
        # Enable only if OpenSceneGraph is available
        HEADERS += src/ui/map3D/QMap3D.h
    }
}
contains(DEPENDENCIES_PRESENT, libfreenect) { 
    message("Including headers for libfreenect")
    
    # Enable only if libfreenect is available
    HEADERS += src/input/Freenect.h
}
SOURCES += src/main.cc \
    src/Core.cc \
    src/uas/UASManager.cc \
    src/uas/UAS.cc \
    src/comm/LinkManager.cc \
    src/comm/SerialLink.cc \
    src/comm/SerialSimulationLink.cc \
    src/comm/MAVLinkProtocol.cc \
    src/comm/AS4Protocol.cc \
    src/ui/CommConfigurationWindow.cc \
    src/ui/SerialConfigurationWindow.cc \
    src/ui/MainWindow.cc \
    src/ui/uas/UASControlWidget.cc \
    src/ui/uas/UASListWidget.cc \
    src/ui/uas/UASInfoWidget.cc \
    src/ui/HUD.cc \
    src/ui/linechart/LinechartWidget.cc \
    src/ui/linechart/LinechartPlot.cc \
    src/ui/linechart/Scrollbar.cc \
    src/ui/linechart/ScrollZoomer.cc \
    src/ui/uas/UASView.cc \
    src/ui/CameraView.cc \
    src/comm/MAVLinkSimulationLink.cc \
    src/comm/UDPLink.cc \
    src/ui/ParameterInterface.cc \
    src/ui/WaypointList.cc \
    src/Waypoint.cc \
    src/ui/WaypointView.cc \
    src/ui/ObjectDetectionView.cc \
    src/input/JoystickInput.cc \
    src/ui/JoystickWidget.cc \
    src/ui/DebugConsole.cc \
    src/ui/MapWidget.cc \
    src/ui/XMLCommProtocolWidget.cc \
    src/ui/mavlink/DomItem.cc \
    src/ui/mavlink/DomModel.cc \
    src/comm/MAVLinkXMLParser.cc \
    src/ui/HDDisplay.cc \
    src/ui/MAVLinkSettingsWidget.cc \
    src/ui/AudioOutputWidget.cc \
    src/GAudioOutput.cc \
    src/LogCompressor.cc \
    src/ui/QGCParamWidget.cc \
    src/ui/QGCSensorSettingsWidget.cc \
    src/ui/linechart/Linecharts.cc \
    src/uas/SlugsMAV.cc \
    src/uas/PxQuadMAV.cc \
    src/uas/ArduPilotMegaMAV.cc \
    src/comm/MAVLinkSyntaxHighlighter.cc \
    src/ui/watchdog/WatchdogControl.cc \
    src/ui/watchdog/WatchdogProcessView.cc \
    src/ui/watchdog/WatchdogView.cc \
    src/uas/UASWaypointManager.cc \
    src/ui/HSIDisplay.cc \
    src/QGC.cc \
    src/ui/QGCFirmwareUpdate.cc \
    src/ui/QGCPxImuFirmwareUpdate.cc \
    src/comm/MAVLinkLightProtocol.cc \
    src/ui/QGCDataPlot2D.cc \
    src/ui/linechart/IncrementalPlot.cc \
    src/ui/map/Waypoint2DIcon.cc \
    src/ui/map/MAV2DIcon.cc \
    src/ui/QGCRemoteControlView.cc \
    src/ui/RadioCalibration/RadioCalibrationWindow.cc \
    src/ui/RadioCalibration/AirfoilServoCalibrator.cc \
    src/ui/RadioCalibration/SwitchCalibrator.cc \
    src/ui/RadioCalibration/CurveCalibrator.cc \
    src/ui/RadioCalibration/AbstractCalibrator.cc \
    src/ui/RadioCalibration/RadioCalibrationData.cc
contains(DEPENDENCIES_PRESENT, osg) { 
    message("Including sources for OpenSceneGraph")
    
    # Enable only if OpenSceneGraph is available
    SOURCES += src/ui/map3D/Q3DWidget.cc \
        src/ui/map3D/ImageWindowGeode.cc \
        src/ui/map3D/GCManipulator.cc \
        src/ui/map3D/QOSGWidget.cc \
        src/ui/map3D/PixhawkCheetahGeode.cc \
        src/ui/map3D/Pixhawk3DWidget.cc \
        src/ui/map3D/Q3DWidgetFactory.cc \
        src/ui/map3D/WebImageCache.cc \
        src/ui/map3D/WebImage.cc \
        src/ui/map3D/TextureCache.cc \
        src/ui/map3D/Texture.cc \
        src/ui/map3D/Imagery.cc
    contains(DEPENDENCIES_PRESENT, osgearth) { 
        message("Including sources for osgEarth")
        
        # Enable only if OpenSceneGraph is available
        SOURCES += src/ui/map3D/QMap3D.cc
    }
}
contains(DEPENDENCIES_PRESENT, libfreenect) { 
    message("Including sources for libfreenect")
    
    # Enable only if libfreenect is available
    SOURCES += src/input/Freenect.cc
}
RESOURCES += mavground.qrc

# Include RT-LAB Library
win32:exists(src/lib/opalrt/OpalApi.h):exists(C:\OPAL-RT\RT-LAB7.2.4\Common\bin) { 
    message("Building support for Opal-RT")
    LIBS += -LC:\OPAL-RT\RT-LAB7.2.4\Common\bin \
        -lOpalApi
    INCLUDEPATH += src/lib/opalrt
    HEADERS += src/comm/OpalRT.h \
        src/comm/OpalLink.h \
        src/comm/Parameter.h \
        src/comm/QGCParamID.h \
        src/comm/ParameterList.h \
        src/ui/OpalLinkConfigurationWindow.h
    SOURCES += src/comm/OpalRT.cc \
        src/comm/OpalLink.cc \
        src/comm/Parameter.cc \
        src/comm/QGCParamID.cc \
        src/comm/ParameterList.cc \
        src/ui/OpalLinkConfigurationWindow.cc
    FORMS += src/ui/OpalLinkSettings.ui
    DEFINES += OPAL_RT
}
