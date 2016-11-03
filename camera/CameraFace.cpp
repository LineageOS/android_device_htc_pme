/*
 * Copyright (C) 2016 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class BEAUTYSHOT_SCENARIO_t {};
class BEAUTYSHOT_EFFECT_t {};
class camera_frame_metadata;

namespace android {
class CameraParameters;
class face_config_type;
class face_info_t;
class faceBeauty_config_type;
class multi_face_info_t;

class CameraFace {
    CameraFace();
    ~CameraFace();

    int initialize(face_config_type* a, bool b);
    int enableFaceCallback(bool a);
    int setUserTouchPoint(int a, int b, bool*c, int d);
    int stopFaceDetection(int a, int b);
    int setFaceDtEvent(int a);
    int setParameters(android::CameraParameters const&a);
    int processPreview(unsigned char*a, int b, faceBeauty_config_type* c, android::face_info_t* d, multi_face_info_t* e, camera_frame_metadata *f);
    int setBeautyEvent(BEAUTYSHOT_SCENARIO_t a, BEAUTYSHOT_EFFECT_t b, int c);
    int processImage(unsigned char* a, int b, int c, int d, faceBeauty_config_type* e);
    int startFaceDetection(int a, int b);
    int getFaceDetectResult(unsigned char* a, unsigned char* b, int* c, float d, int e, int f);
};

CameraFace::CameraFace() { }
CameraFace::~CameraFace() { }
int CameraFace::initialize(face_config_type* a, bool b) { return 0; }
int CameraFace::enableFaceCallback(bool a) { return 0; }
int CameraFace::setUserTouchPoint(int a, int b, bool*c, int d) { return 0; }
int CameraFace::stopFaceDetection(int a, int b) { return 0; }
int CameraFace::setFaceDtEvent(int a) { return 0; }
int CameraFace::setParameters(android::CameraParameters const&a) { return 0; }
int CameraFace::processPreview(unsigned char*a, int b, faceBeauty_config_type* c, android::face_info_t* d, multi_face_info_t* e, camera_frame_metadata *f) { return 0; }
int CameraFace::setBeautyEvent(BEAUTYSHOT_SCENARIO_t a, BEAUTYSHOT_EFFECT_t b, int c) { return 0; }
int CameraFace::processImage(unsigned char* a, int b, int c, int d, faceBeauty_config_type* e) { return 0; }
int CameraFace::startFaceDetection(int a, int b) { return 0; }
int CameraFace::getFaceDetectResult(unsigned char* a, unsigned char* b, int* c, float d, int e, int f) { return 0; }

};

