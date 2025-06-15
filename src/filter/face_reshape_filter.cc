/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/face_reshape_filter.h"
#include "core/gpupixel_context.h"

namespace gpupixel {

#if defined(GPUPIXEL_GLES_SHADER)
const std::string kGPUPixelThinFaceFragmentShaderString = R"(
 precision highp float;
 varying highp vec2 textureCoordinate;
 uniform sampler2D inputImageTexture;

 uniform int faceCount; 
 uniform float facePoints[4*222]; 

 uniform highp float aspectRatio;
 uniform float thinFaceDelta;
 uniform float bigEyeDelta;
 
 vec2 enlargeEye(vec2 textureCoord, vec2 originPosition, float radius, float delta) {

     float weight = distance(vec2(textureCoord.x, textureCoord.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio)) / radius;

     weight = 1.0 - (1.0 - weight * weight) * delta;
     weight = clamp(weight,0.0,1.0);
     textureCoord = originPosition + (textureCoord - originPosition) * weight;
     return textureCoord;
 }
 
 vec2 curveWarp(vec2 textureCoord, vec2 originPosition, vec2 targetPosition, float delta) {

     vec2 offset = vec2(0.0);
     vec2 result = vec2(0.0);
     vec2 direction = (targetPosition - originPosition) * delta;

     float radius = distance(vec2(targetPosition.x, targetPosition.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio));
     float ratio = distance(vec2(textureCoord.x, textureCoord.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio)) / radius;

     ratio = 1.0 - ratio;
     ratio = clamp(ratio, 0.0, 1.0);
     offset = direction * ratio;

     result = textureCoord - offset;

     return result;
 }

 vec2 thinFace(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;

     vec2 faceIndexs[9];
     faceIndexs[0] = vec2(3., 44.);
     faceIndexs[1] = vec2(29., 44.);
     faceIndexs[2] = vec2(7., 45.);
     faceIndexs[3] = vec2(25., 45.);
     faceIndexs[4] = vec2(10., 46.);
     faceIndexs[5] = vec2(22., 46.);
     faceIndexs[6] = vec2(14., 49.);
     faceIndexs[7] = vec2(18., 49.);
     faceIndexs[8] = vec2(16., 49.);

     for(int i = 0; i < 9; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

 vec2 bigEye(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;

     vec2 faceIndexs[2];
     faceIndexs[0] = vec2(74., 72.);
     faceIndexs[1] = vec2(77., 75.);

     for(int i = 0; i < 2; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);

         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);

         float radius = distance(vec2(targetPoint.x, targetPoint.y / aspectRatio), vec2(originPoint.x, originPoint.y / aspectRatio));
         radius = radius * 5.;
         currentCoordinate = enlargeEye(currentCoordinate, originPoint, radius, bigEyeDelta);
     }
     return currentCoordinate;
 }

 void main()
 {
     vec2 positionToUse = textureCoordinate;

     for(int i = 0; i < faceCount; i++) {
         positionToUse = thinFace(positionToUse, i);
         positionToUse = bigEye(positionToUse, i);
     }

     gl_FragColor = texture2D(inputImageTexture, positionToUse);
 }
 )";
#elif defined(GPUPIXEL_GL_SHADER)
const std::string kGPUPixelThinFaceFragmentShaderString = R"(
 varying vec2 textureCoordinate;
 uniform sampler2D inputImageTexture;

 uniform int faceCount; 
 uniform float facePoints[4*222]; 
 uniform float aspectRatio;
 uniform float thinFaceDelta;
 uniform float bigEyeDelta;
 
 // 基础矩形区域像素平移函数
 vec2 rectangleTranslate(vec2 textureCoord, vec2 rectTopLeft, vec2 rectBottomRight, vec2 translateOffset) {
     // 检查当前像素是否在矩形框内
     bool inRect = (textureCoord.x >= rectTopLeft.x && textureCoord.x <= rectBottomRight.x) &&
                   (textureCoord.y >= rectTopLeft.y && textureCoord.y <= rectBottomRight.y);
     
     if (inRect) {
         // 在矩形内，应用平移偏移
         return textureCoord + translateOffset;
     } else {
         // 在矩形外，保持原坐标
         return textureCoord;
     }
 }

 // 带平滑边缘的矩形区域像素平移函数  
 vec2 rectangleTranslateSmooth(vec2 textureCoord, vec2 rectCenter, vec2 rectSize, vec2 translateOffset, float smoothEdge) {
     // 计算到矩形中心的相对坐标
     vec2 relativePos = abs(textureCoord - rectCenter);
     vec2 halfSize = rectSize * 0.5;
     
     // 计算距离矩形边界的距离
     vec2 distToEdge = halfSize - relativePos;
     float minDistToEdge = min(distToEdge.x, distToEdge.y);
     
     // 计算权重：在矩形内部权重为1，边缘平滑过渡
     float weight = smoothstep(0.0, smoothEdge, minDistToEdge);
     weight = clamp(weight, 0.0, 1.0);
     
     // 应用加权的平移偏移
     return textureCoord + translateOffset * weight;
 }

 // 使用4个顶点定义的任意四边形平移函数
 vec2 quadTranslate(vec2 textureCoord, vec2 p1, vec2 p2, vec2 p3, vec2 p4, vec2 translateOffset) {
     // 简化的点在四边形内判断（适用于凸四边形）
     // 使用叉积判断点是否在四边形内部
     vec2 v1 = p2 - p1;
     vec2 v2 = p3 - p2; 
     vec2 v3 = p4 - p3;
     vec2 v4 = p1 - p4;
     
     vec2 vp1 = textureCoord - p1;
     vec2 vp2 = textureCoord - p2;
     vec2 vp3 = textureCoord - p3;
     vec2 vp4 = textureCoord - p4;
     
     // 计算叉积
     float cross1 = v1.x * vp1.y - v1.y * vp1.x;
     float cross2 = v2.x * vp2.y - v2.y * vp2.x;
     float cross3 = v3.x * vp3.y - v3.y * vp3.x;
     float cross4 = v4.x * vp4.y - v4.y * vp4.x;
     
     // 检查所有叉积是否同号（在四边形内）
     bool inQuad = (cross1 >= 0.0 && cross2 >= 0.0 && cross3 >= 0.0 && cross4 >= 0.0) ||
                   (cross1 <= 0.0 && cross2 <= 0.0 && cross3 <= 0.0 && cross4 <= 0.0);
     
     if (inQuad) {
         return textureCoord + translateOffset;
     } else {
         return textureCoord;
     }
 }
 
 vec2 enlargeEye(vec2 textureCoord, vec2 originPosition, float radius, float delta) {

     float weight = distance(vec2(textureCoord.x, textureCoord.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio)) / radius;

     weight = 1.0 - (1.0 - weight * weight) * delta;
     weight = clamp(weight,0.0,1.0);
     textureCoord = originPosition + (textureCoord - originPosition) * weight;
     return textureCoord;
 }
 
 vec2 curveWarp(vec2 textureCoord, vec2 originPosition, vec2 targetPosition, float delta) {

     vec2 offset = vec2(0.0);
     vec2 result = vec2(0.0);
     vec2 direction = (targetPosition - originPosition) * delta;

     float radius = distance(vec2(targetPosition.x, targetPosition.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio));
     float ratio = distance(vec2(textureCoord.x, textureCoord.y / aspectRatio), vec2(originPosition.x, originPosition.y / aspectRatio)) / radius;

     ratio = 1.0 - ratio;
     ratio = clamp(ratio, 0.0, 1.0);
     offset = direction * ratio;

     result = textureCoord - offset;

     return result;
 }

// 瘦脸
 vec2 thinFace(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
     vec2 faceIndexs[9];
     faceIndexs[0] = vec2(3., 44.);
     faceIndexs[1] = vec2(29., 44.);
     faceIndexs[2] = vec2(7., 45.);
     faceIndexs[3] = vec2(25., 45.);
     faceIndexs[4] = vec2(10., 46.);
     faceIndexs[5] = vec2(22., 46.);
     faceIndexs[6] = vec2(14., 49.);
     faceIndexs[7] = vec2(18., 49.);
     faceIndexs[8] = vec2(16., 49.);

     for(int i = 0; i < 9; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

// v脸
  vec2 vFace(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
     vec2 faceIndexs[4];
     faceIndexs[0] = vec2(10., 95.);
     faceIndexs[1] = vec2(22., 91.);
     faceIndexs[2] = vec2(14., 20.);
     faceIndexs[3] = vec2(18., 13.);

     for(int i = 0; i < 4; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

// 窄脸
   vec2 zhaiFace(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
    
     vec2 faceIndexs[6];
    faceIndexs[0] = vec2(0., 33.);
     faceIndexs[1] = vec2(33., 0.);
     faceIndexs[2] = vec2(3., 29.);
     faceIndexs[3] = vec2(29., 3.);
     faceIndexs[4] = vec2(7., 25.);
     faceIndexs[5] = vec2(25., 7.);
 

     for(int i = 0; i < 6; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

// 短脸
   vec2 shortFace(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
     vec2 faceIndexs[5];
     faceIndexs[0] = vec2(10., 52.);
     faceIndexs[1] = vec2(22., 61.);
     faceIndexs[2] = vec2(14., 82.);
     faceIndexs[3] = vec2(18., 83.);
     faceIndexs[4] = vec2(16., 49.);
     
     for(int i = 0; i < 5; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

// 颧骨
    vec2 quanGu(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
      vec2 faceIndexs[6];
      faceIndexs[0] = vec2(3., 44.);
      faceIndexs[1] = vec2(29., 44.);
      faceIndexs[2] = vec2(4., 80.);
      faceIndexs[3] = vec2(28., 81.);
      faceIndexs[4] = vec2(5., 80.);
      faceIndexs[5] = vec2(27., 81.);

     
     for(int i = 0; i < 6; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

// 下颌骨
  vec2 xiaheGu(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
      vec2 faceIndexs[8];
      faceIndexs[0] = vec2(7., 49.);
      faceIndexs[1] = vec2(25., 49.);
      faceIndexs[2] = vec2(8., 49.);
      faceIndexs[3] = vec2(24., 49.);
      faceIndexs[4] = vec2(9., 87.);
      faceIndexs[5] = vec2(23., 87.);
      faceIndexs[6] = vec2(10., 87.);
      faceIndexs[7] = vec2(22., 87.);
     
     for(int i = 0; i < 8; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }


 // 下巴
  vec2 xiaBa(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
      vec2 faceIndexs[7];
      faceIndexs[0] = vec2(13., 82.);
      faceIndexs[1] = vec2(19., 83.);
      faceIndexs[2] = vec2(14., 47.);
      faceIndexs[3] = vec2(18., 51.);
      faceIndexs[4] = vec2(15., 48.);
      faceIndexs[5] = vec2(17., 50.);
      faceIndexs[6] = vec2(16., 49.);
     
     for(int i = 0; i < 7; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

  // 瘦鼻
  vec2 shouBi(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
      vec2 faceIndexs[6];
      faceIndexs[0] = vec2(80., 81.);
      faceIndexs[1] = vec2(81., 80.);
      faceIndexs[2] = vec2(82., 83.);
      faceIndexs[3] = vec2(83., 82.);
      faceIndexs[4] = vec2(47., 51.);
      faceIndexs[5] = vec2(51., 47.);
     
     for(int i = 0; i < 6; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

   // 嘴型（大小）
  vec2 zuiXing(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;
      vec2 faceIndexs[6];
      faceIndexs[0] = vec2(80., 81.);
      faceIndexs[1] = vec2(81., 80.);
      faceIndexs[2] = vec2(82., 83.);
      faceIndexs[3] = vec2(83., 82.);
      faceIndexs[4] = vec2(47., 51.);
      faceIndexs[5] = vec2(51., 47.);
     
     for(int i = 0; i < 6; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);
         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);
         currentCoordinate = curveWarp(currentCoordinate, originPoint, targetPoint, thinFaceDelta);
     }
     return currentCoordinate;
 }

    // 眼距
  vec2 yanju(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;     
 
    vec2 center = vec2(facePoints[baseIndex + 74 * 2], facePoints[baseIndex + 74 * 2 + 1]);
    vec2 size = vec2(0.1, 0.05);
    currentCoordinate = rectangleTranslateSmooth(currentCoordinate, center, size, vec2(0.1, 0.0), 1. - thinFaceDelta);
     return currentCoordinate;
 }

// 大眼
 vec2 bigEye(vec2 currentCoordinate, int faceIndex) {
     int baseIndex = faceIndex * 222;

     vec2 faceIndexs[2];
     faceIndexs[0] = vec2(74., 72.);
     faceIndexs[1] = vec2(77., 75.);

     for(int i = 0; i < 2; i++)
     {
         int originIndex = int(faceIndexs[i].x);
         int targetIndex = int(faceIndexs[i].y);

         vec2 originPoint = vec2(facePoints[baseIndex + originIndex * 2], facePoints[baseIndex + originIndex * 2 + 1]);
         vec2 targetPoint = vec2(facePoints[baseIndex + targetIndex * 2], facePoints[baseIndex + targetIndex * 2 + 1]);

         float radius = distance(vec2(targetPoint.x, targetPoint.y / aspectRatio), vec2(originPoint.x, originPoint.y / aspectRatio));
         radius = radius * 5.;
         currentCoordinate = enlargeEye(currentCoordinate, originPoint, radius, bigEyeDelta);
     }
     return currentCoordinate;
 }

 void main()
 {
     vec2 positionToUse = textureCoordinate;

     for(int i = 0; i < faceCount; i++) {
        //  positionToUse = thinFace(positionToUse, i);
         positionToUse = yanju(positionToUse, i);
         positionToUse = bigEye(positionToUse, i);
     }

     gl_FragColor = texture2D(inputImageTexture, positionToUse);
 }
 )";
#endif

FaceReshapeFilter::FaceReshapeFilter() {}

FaceReshapeFilter::~FaceReshapeFilter() {}

std::shared_ptr<FaceReshapeFilter> FaceReshapeFilter::Create() {
  auto ret = std::shared_ptr<FaceReshapeFilter>(new FaceReshapeFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool FaceReshapeFilter::Init() {
  if (!InitWithFragmentShaderString(kGPUPixelThinFaceFragmentShaderString)) {
    return false;
  }
  RegisterProperty("thin_face", 0,
                   "The smoothing of filter with range between -1 and 1.",
                   [this](float& val) { SetFaceSlimLevel(val); });

  RegisterProperty("big_eye", 0,
                   "The smoothing of filter with range between -1 and 1.",
                   [this](float& val) { SetEyeZoomLevel(val); });

  std::vector<float> defaut;
  RegisterProperty("face_landmark", defaut,
                   "The face landmark of filter with range between -1 and 1.",
                   [this](std::vector<float> val) { SetFaceLandmarks(val); });

  this->thin_face_delta_ = 0.0;
  // [0, 0.15]
  this->big_eye_delta_ = 0.0;
  return true;
}

// Process landmarks for single or multiple faces (each face has 478 points with
// x,y coordinates)
void FaceReshapeFilter::SetFaceLandmarks(std::vector<float> landmarks) {
  face_landmarks_.clear();

  if (landmarks.empty()) {
    return;
  }
  face_landmarks_ = landmarks;
  face_count_ = static_cast<int>(face_landmarks_.size() / 222);
}

// Render the filter with current face landmarks and effect parameters
bool FaceReshapeFilter::DoRender(bool updateSinks) {
  float aspect = (float)framebuffer_->GetWidth() / framebuffer_->GetHeight();
  filter_program_->SetUniformValue("aspectRatio", aspect);
  filter_program_->SetUniformValue("thinFaceDelta", thin_face_delta_);
  filter_program_->SetUniformValue("bigEyeDelta", big_eye_delta_);

  filter_program_->SetUniformValue("faceCount", face_count_);

  if (!face_landmarks_.empty()) {
    filter_program_->SetUniformValue("facePoints", face_landmarks_.data(),
                                     static_cast<int>(face_landmarks_.size()));
  }

  return Filter::DoRender(updateSinks);
}

#pragma mark - face slim
void FaceReshapeFilter::SetFaceSlimLevel(float level) {
  thin_face_delta_ = std::clamp(level, -1.0f, 1.0f);
}

#pragma mark - eye zoom
void FaceReshapeFilter::SetEyeZoomLevel(float level) {
  // Recommended range: [0, 0.15]
  big_eye_delta_ = std::clamp(level, -1.0f, 1.0f);
}

}  // namespace gpupixel
