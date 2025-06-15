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

namespace gpupixel {
class GPUPIXEL_API ColorMatrixFilter : public Filter {
 public:
  static std::shared_ptr<ColorMatrixFilter> Create();
  bool Init();

  virtual bool DoRender(bool updateSinks = true) override;

  void setIntensity(float intensity) { intensity_factor_ = intensity; }
  void setColorMatrix(glm::mat4 color_matrix) { color_matrix_ = color_matrix; }

 protected:
  ColorMatrixFilter();

  float intensity_factor_;
  glm::mat4 color_matrix_;
};

}  // namespace gpupixel
