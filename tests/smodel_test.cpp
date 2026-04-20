#include <gtest/gtest.h>

#include "NNL/nnl.hpp"

using namespace nnl;

TEST(SVertex, Weights) {
  SVertex v;

  v.weights = {0.25f, 0.5f, 0.6f};
  v.bones = {0, 1, 2};

  v.SortWeights();

  ASSERT_TRUE(v.weights[0] >= v.weights[1]);
  ASSERT_TRUE(v.bones[0] == 2);

  v.NormalizeWeights();

  ASSERT_TRUE((v.weights[0] + v.weights[1] + v.weights[2]) <= 1.0f);

  v.LimitWeights(2);

  ASSERT_TRUE(v.bones[2] == 0);
  ASSERT_TRUE(v.weights[2] == 0.0f);
  ASSERT_TRUE(v.bones[0] == 2);
  ASSERT_TRUE(v.bones[1] == 1);

  v.QuantWeights(255);

  ASSERT_TRUE((v.weights[0] + v.weights[1]) <= 1.0f);

  ASSERT_TRUE((v.weights[0] * 255.0f + v.weights[1] * 255.0f) <= 255.0f);
}
