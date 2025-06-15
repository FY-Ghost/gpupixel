/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gpupixel/filter/filter.h"
#include "gpupixel/gpupixel_define.h"
#include "gpupixel/source/source_image.h"

namespace gpupixel {
class GPUPIXEL_API TieZhiFilter : public Filter {
 public:
  static std::shared_ptr<TieZhiFilter> Create();
  bool Init();
  virtual bool DoRender(bool updateSinks = true) override;
  void SetAngle(float pitch, float yaw, float roll);

  void SetLandmark(const std::vector<float>& landmarks);

 protected:
  TieZhiFilter() {};

  float pitch_ = 0.0f;
  float yaw_ = 0.0f;
  float roll_ = 0.0f;

  std::shared_ptr<SourceImage> source_image_;
  std::vector<float> face_landmarks_;

 private:
  glm::mat4 CreateRotationMatrix(float angle, float x, float y, float z);
};

}  // namespace gpupixel
