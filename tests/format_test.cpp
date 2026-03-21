#include <gtest/gtest.h>

#include "NNL/nnl.hpp"
#include "test_common.hpp"
using namespace nnl;

TEST(Format, DetermineAssetType) {
  FileReader f{GetPath("dig_entry")};

  auto bin_container = f.ReadArrayLE<u8>(f.Len());

  ASSERT_EQ(format::Detect(bin_container), format::kDigEntry);

  auto cfc_entry = dig_entry::ImportView(bin_container);

  auto& bin_asset_ctr = cfc_entry.at(0);

  auto& bin_asset_col = cfc_entry.at(3);

  ASSERT_EQ(format::Detect(bin_asset_col), format::kCollection);

  ASSERT_EQ(format::Detect(bin_asset_ctr), format::kAssetContainer);

  auto asset_container = asset::ImportView(bin_asset_ctr);

  ASSERT_EQ(format::Detect(asset_container.at(asset::Asset3D::kModel)), format::kModel);

  ASSERT_EQ(format::Detect(asset_container.at(asset::Asset3D::kTextureContainer)), format::kTextureContainer);

  ASSERT_EQ(format::Detect(asset_container.at(asset::Asset3D::kAnimationContainer)), format::kAnimationContainer);

  ASSERT_EQ(format::Detect(asset_container.at(asset::Asset3D::kActionConfig)), format::kActionConfig);

  ASSERT_EQ(format::Detect(asset_container.at(asset::Asset3D::kVisanimationContainer)), format::kVisanimationContainer);

  ASSERT_EQ(format::Detect(asset_container.at(asset::Asset3D::kColboxConfig)), format::kColboxConfig);
}
