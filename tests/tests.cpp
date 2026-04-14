
#include <gtest/gtest.h>

#include <filesystem>

#include "NNL/nnl.hpp"
#include "test_common.hpp"

std::filesystem::path g_asset_dir;
std::filesystem::path g_main_archive_path;

static void Save(nnl::BufferView view, const std::filesystem::path& out_dir, std::string_view name) {
  auto out_p = out_dir / std::filesystem::u8path(name);
  nnl::FileRW f{out_p, true};
  f.WriteBuf(view.data(), view.size());
  f.Close();
}

void ExtractTestAssets(const std::filesystem::path& archive_path, const std::filesystem::path& out_path) {
  using namespace nnl;

  std::cout << "[ SETUP    ] Extracting assets from " << archive_path.filename().u8string() << std::endl;

  dig::Dig cfc = dig::Import(archive_path);

  {
    const dig::FileRecord& naruto_rec = cfc.at(8);

    Buffer decomp_buf = dig::Decompress(naruto_rec.buffer, naruto_rec.decompressed_size);

    Save(decomp_buf, out_path, "dig_entry");

    dig_entry::DigEntry cfc_ent = dig_entry::Import(decomp_buf);

    Buffer& naruto_bin = cfc_ent.at(0);

    Save(naruto_bin, out_path, "naruto");

    asset::AssetView ast_container = asset::ImportView(naruto_bin);

    Save(ast_container.at(asset::Asset3D::kModel), out_path, "naruto_model");
    Save(ast_container.at(asset::Asset3D::kTextureContainer), out_path, "naruto_texture");
    Save(ast_container.at(asset::Asset3D::kAnimationContainer), out_path, "naruto_animation");
    Save(ast_container.at(asset::Asset3D::kActionConfig), out_path, "naruto_action");
    Save(ast_container.at(asset::Asset3D::kColboxConfig), out_path, "naruto_colbox");
    Save(ast_container.at(asset::Asset3D::kVisanimationContainer), out_path, "naruto_visanimation");
  }

  {
    const dig::FileRecord& naruto_snd_rec = cfc.at(10);

    dig_entry::DigEntry cfc_ent = dig_entry::Import(naruto_snd_rec.buffer);

    Buffer& naruto_bin = cfc_ent.at(0);

    Save(naruto_bin, out_path, "naruto_sfx");

    asset::AssetView ast_container = asset::ImportView(naruto_bin);

    Save(ast_container.at(asset::SoundBank::kPBD), out_path, "naruto_adpcm");
  }

  {
    const dig::FileRecord& tower_map_rec = cfc.at(496);

    Buffer decomp_buf = dig::Decompress(tower_map_rec.buffer, tower_map_rec.decompressed_size);
    dig_entry::DigEntry cfc_ent = dig_entry::Import(decomp_buf);

    // Save specific sub-asset (e.g., index 0 is the 3D model)
    Buffer& tower_collection_bin = cfc_ent.at(0);

    Save(tower_collection_bin, out_path, "collection");

    collection::CollectionView collection = collection::ImportView(tower_collection_bin);

    asset::AssetView ast_container = asset::ImportView(collection.at(0));

    Save(collection.at(0), out_path, "before_tower");

    Save(cfc_ent.at(1), out_path, "before_tower_lit");
    Save(cfc_ent.at(2), out_path, "before_tower_fog");
    Save(cfc_ent.at(3), out_path, "before_tower_posd");
    Save(ast_container.at(asset::Asset3D::kModel), out_path, "before_tower_model");
    Save(ast_container.at(asset::Asset3D::kCollision), out_path, "before_tower_collision");
    Save(ast_container.at(asset::Asset3D::kShadowCollision), out_path, "before_tower_shadow_collision");
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  // NSLAR (ULUS10447)
  g_main_archive_path = std::filesystem::current_path() / std::filesystem::u8path("0000.BIN");

  if (argc > 1) {
    std::cout << "[ INFO     ] Using a custom path to the main archive" << std::endl;
    g_main_archive_path = argv[1];
  }

  if (argc > 2) {
    std::cerr << "[ WARN    ] Only 1 argument was expected!" << std::endl;
  }

  if (!std::filesystem::is_regular_file(g_main_archive_path)) {
    std::cerr << "[ ERROR    ] The file does not exist: " << g_main_archive_path.u8string() << std::endl;
    std::cerr << "[ ERROR    ] Expected a valid path to the main asset archive from NSLAR (ULUS10447)" << std::endl;
    return 1;
  }

  if (!nnl::dig::IsOfType(g_main_archive_path)) {
    std::cerr << "[ ERROR    ] The file is not a dig archive: " << g_main_archive_path.u8string() << std::endl;
    return 1;
  }

  g_asset_dir = g_main_archive_path.parent_path() / std::filesystem::u8path("assets");

  std::filesystem::create_directories(g_asset_dir);

  ExtractTestAssets(g_main_archive_path, g_asset_dir);

  return RUN_ALL_TESTS();
}
