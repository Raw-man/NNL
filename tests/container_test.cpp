#include <gtest/gtest.h>

#include "NNL/game_asset/container/asset.hpp"
#include "NNL/game_asset/container/collection.hpp"
#include "NNL/game_asset/container/dig.hpp"
#include "NNL/game_asset/container/dig_entry.hpp"
#include "NNL/game_asset/container/md5list.hpp"
#include "test_common.hpp"
using namespace nnl;

TEST(Asset, IsOfType) {
  FileReader f{GetPath("naruto")};

  auto bin_container = f.ReadArrayLE<u8>(f.Len());

  ASSERT_TRUE(asset::IsOfType(bin_container));
}

TEST(Asset, ImportExport) {
  FileReader f{GetPath("naruto")};

  auto bin_container = f.ReadArrayLE<u8>(f.Len());

  auto asset_container = asset::Import(bin_container);

  auto bin_container_re = asset::Export(asset_container);

  ASSERT_TRUE(bin_container == bin_container_re);
}

TEST(Asset, ImportView) {
  FileReader f{GetPath("naruto")};

  auto bin_container = f.ReadArrayLE<u8>(f.Len());

  auto asset = asset::Import(bin_container);
  auto asset_view = asset::ImportView(bin_container);

  ASSERT_EQ(asset_view.size(), 6);
  ASSERT_TRUE(asset_view.at(0) == asset.at(0));
  ASSERT_TRUE(asset_view.at(1) == asset.at(1));
  ASSERT_TRUE(asset_view.at(2) == asset.at(2));
  ASSERT_TRUE(asset_view.at(3) == asset.at(3));
  ASSERT_TRUE(asset_view.at(4) == asset.at(4));
  ASSERT_TRUE(asset_view.at(9) == asset.at(9));
}

TEST(Asset, ImportExportEmptyBuffer) {
  Buffer bin_container{0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x01, 0x00, 0x88, 0x88, 0x18, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
                       0x43, 0x44, 0x4c, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  auto asset_container = asset::Import(bin_container);

  auto bin_container_re = asset::Export(asset_container);

  ASSERT_TRUE(bin_container == bin_container_re);
}

TEST(Dig, ImportView) {
  FileReader f{g_main_archive_path};

  auto bin_cfc = f.ReadArrayLE<u8>(f.Len());

  auto rcfcdig = dig::Import(bin_cfc);
  auto rcfcdig_view = dig::ImportView(bin_cfc);

  auto list = md5list::Generate(rcfcdig_view);

  auto listb = md5list::Export(list);

  ASSERT_TRUE(!rcfcdig.empty());
  ASSERT_TRUE(rcfcdig.size() == rcfcdig_view.size());
  ASSERT_TRUE(rcfcdig_view.at(0).buffer == rcfcdig.at(0).buffer);
}

TEST(DigEntry, IsOfType) {
  FileReader f{GetPath("dig_entry")};
  ASSERT_TRUE(dig_entry::IsOfType(f));
}

TEST(DigEntry, ImportExport) {
  FileReader f{GetPath("dig_entry")};

  auto bin_cfc_entry = f.ReadArrayLE<u8>(f.Len());

  auto cfc_entry = dig_entry::Import(bin_cfc_entry);

  ASSERT_TRUE(!cfc_entry.empty());

  Buffer bin_cfc_entry_re = dig_entry::Export(cfc_entry);

  ASSERT_TRUE(bin_cfc_entry == bin_cfc_entry_re);
}

TEST(DigEntry, ImportView) {
  FileReader f{GetPath("dig_entry")};

  auto bin_cfc_entry = f.ReadArrayLE<u8>(f.Len());

  auto rcfc_entry = dig_entry::Import(bin_cfc_entry);
  auto rcfc_entry_view = dig_entry::ImportView(bin_cfc_entry);

  ASSERT_TRUE(!rcfc_entry.empty());
  ASSERT_TRUE(rcfc_entry.size() == rcfc_entry_view.size());
  ASSERT_TRUE(rcfc_entry_view.at(0) == rcfc_entry.at(0));
}

TEST(Collection, IsOfType) {
  FileReader f{GetPath("collection")};
  ASSERT_TRUE(collection::IsOfType(f));
}

TEST(Collection, ImportExport) {
  FileReader f{GetPath("collection")};

  auto bin_asset_col = f.ReadArrayLE<u8>(f.Len());

  auto asset_col = collection::Import(bin_asset_col);

  ASSERT_TRUE(!asset_col.empty());

  Buffer bin_asset_col_re = collection::Export(asset_col);

  ASSERT_TRUE(bin_asset_col == bin_asset_col_re);
}

TEST(Collection, ImportView) {
  FileReader f{GetPath("collection")};

  auto bin_asset_col = f.ReadArrayLE<u8>(f.Len());

  auto asset_col = collection::Import(bin_asset_col);
  auto asset_col_view = collection::ImportView(bin_asset_col);

  ASSERT_TRUE(!asset_col.empty());
  ASSERT_TRUE(asset_col.size() == asset_col_view.size());
  ASSERT_TRUE(asset_col_view.at(0) == asset_col.at(0));
}
