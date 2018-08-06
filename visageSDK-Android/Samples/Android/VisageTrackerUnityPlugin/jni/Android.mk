LOCAL_PATH := $(call my-dir)
         
include $(CLEAR_VARS)
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
VISAGE_LIBS := ../../../../lib/armeabi-v7a
endif
ifeq ($(TARGET_ARCH_ABI),x86)
VISAGE_LIBS := ../../../../lib/x86
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
VISAGE_LIBS := ../../../../lib/x86_64
endif
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
VISAGE_LIBS := ../../../../lib/arm64-v8a
endif
LOCAL_MODULE := VisageVision
LOCAL_SRC_FILES := $(VISAGE_LIBS)/libVisageVision.so
include $(PREBUILT_SHARED_LIBRARY)

########################
# prepare VisageAnalyser
include $(CLEAR_VARS)
LOCAL_MODULE    := VisageAnalyser
LOCAL_SRC_FILES := $(VISAGE_LIBS)/libVisageAnalyser.so
include $(PREBUILT_SHARED_LIBRARY)
########################

include $(CLEAR_VARS)
# OPENCV_LIB_TYPE := STATIC

 OPENCV_MK_PATH:=../../../../dependencies/OpenCV-2.4.11-Android/jni/OpenCV.mk
# include $(OPENCV_MK_PATH)
$(warning "#######" + $(OPENCV_MK_PATH))

VISAGE_HEADERS  := ../../../include
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
VISAGE_LIBS := ../../../lib/armeabi-v7a
endif
ifeq ($(TARGET_ARCH_ABI),x86)
VISAGE_LIBS := ../../../lib/x86
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
VISAGE_LIBS := ../../../lib/x86_64
endif
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
VISAGE_LIBS := ../../../lib/arm64-v8a
endif
       
LOCAL_C_INCLUDES +=  $(VISAGE_HEADERS) $(VISAGE_HEADERS)/libAfm $(VISAGE_HEADERS)/libVRMLParser
LOCAL_MODULE    := VisageTrackerUnityPlugin
LOCAL_SRC_FILES := \
	VisageTrackerUnityPlugin.cpp \
	FrameRenderer.cpp \
	GLUtils.cpp \
	AndroidCameraCapture.cpp

LOCAL_SHARED_LIBRARIES := VisageVision VisageAnalyser
LOCAL_LDLIBS +=  -L$(VISAGE_LIBS) -L$(/jni) -lVisageVision -lVisageAnalyser -lGLESv1_CM -lEGL -lGLESv3 -llog -ldl -Wl,--gc-sections,-rpath,.

LOCAL_CFLAGS := -DANDROID_NDK \
				-DDISABLE_IMPORTGL \
				-DANDROID \
				-std=c++11 \
				-D_CRT_SECURE_NO_WARNINGS \
				-DVISAGE_STATIC \
				-ffast-math -O2 -funroll-loops -Wno-write-strings				

include $(BUILD_SHARED_LIBRARY)
