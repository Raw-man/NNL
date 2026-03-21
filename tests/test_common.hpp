#pragma once
#include <filesystem>

extern std::filesystem::path g_asset_dir;
extern std::filesystem::path g_main_archive_path;

inline std::filesystem::path GetPath(std::string_view asset_name){
return g_asset_dir/std::filesystem::u8path(asset_name);
}
