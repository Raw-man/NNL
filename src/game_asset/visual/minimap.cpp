
#include "NNL/game_asset/visual/minimap.hpp"

#include "NNL/utility/string.hpp"
namespace nnl {
namespace minimap {

constexpr inline std::size_t kMaxNumMarkers_ = 255;  // An arbitrary limit, it should be enough

constexpr inline f32 kScreenHalfWidth_ = 240.0f;
constexpr inline f32 kScreenHalfHeight_ = 136.0f;

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_len = f.Len();

  if (data_len < sizeof(0x20)) return false;

  auto num_markers = f.ReadLE<u32>();

  if (num_markers == 0 || num_markers > 0xFF) return false;

  if (data_len < 0x10 + sizeof(Vec4<f32>) * num_markers) return false;

  auto reserved_0 = f.ReadLE<u32>();
  auto reserved_1 = f.ReadLE<u32>();
  auto reserved_2 = f.ReadLE<u32>();

  if (reserved_0 != 0 || reserved_1 != 0 || reserved_2 != 0) return false;

  f.Seek(0x10);

  auto ranchor = f.ReadLE<Vec4<f32>>();

  if (ranchor.z < 0.0f || ranchor.w != 0.0f) return false;

  for (std::size_t i = 1; i < num_markers; i++) {
    auto m = f.ReadLE<Vec4<f32>>();
    if (!utl::math::IsFinite(m) || m.w != 0.0f) return false;

    if (m.x < 0.0f || m.x > 512.0f || m.y < 0.0f || m.y > 512.0f || std::abs(m.z) > 360.0f) return false;
  }

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

namespace raw {

MinimapConfig Convert(const RMinimapConfig& rminimap) {
  MinimapConfig minimap;

  auto& ref_point = rminimap.markers.at(0);

  minimap.anchor_x = ref_point.x;
  minimap.anchor_z = ref_point.y;
  minimap.pixels_per_unit = ref_point.z;

  minimap.markers.reserve(rminimap.num_markers - 1);

  for (std::size_t i = 1; i < rminimap.num_markers; i++) {
    auto& rmarker = rminimap.markers.at(i);
    minimap.markers.push_back({rmarker.x, rmarker.y, rmarker.z});
  }

  return minimap;
}

RMinimapConfig Parse(Reader& f) {
  f.Seek(0);

  RMinimapConfig rminimap;

  rminimap.num_markers = f.ReadLE<u32>();

  if (rminimap.num_markers == 0 || rminimap.num_markers > kMaxNumMarkers_)
    NNL_THROW(ParseError(NNL_ERMSG("the number of markers is invalid")));

  f.Seek(0x10);

  rminimap.markers = f.ReadArrayLE<Vec4<f32>>(rminimap.num_markers);

  return rminimap;
}

}  // namespace raw

MinimapConfig Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

MinimapConfig Import(Reader& f) { return Import_(f); }

MinimapConfig Import(BufferView buffer) { return Import_(buffer); }

MinimapConfig Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const MinimapConfig& minimap, Writer& f) {
  using namespace raw;
  f.Seek(0);

  RMinimapConfig rminimap;

  NNL_EXPECTS(minimap.markers.size() < kMaxNumMarkers_);
  NNL_EXPECTS(utl::math::IsFinite(minimap.anchor_x));
  NNL_EXPECTS(utl::math::IsFinite(minimap.anchor_z));
  NNL_EXPECTS(minimap.pixels_per_unit > 0.0f);

  rminimap.num_markers = 1 + minimap.markers.size();

  rminimap.markers.push_back({minimap.anchor_x, minimap.anchor_z, minimap.pixels_per_unit, 0.0f});

  for (const auto& marker : minimap.markers) {
    NNL_EXPECTS(marker.z >= -360.0f && marker.z <= 360.0f);
    NNL_EXPECTS(marker.x >= 0.0f && marker.z <= 512.0f);
    NNL_EXPECTS(marker.y >= 0.0f && marker.y <= 512.0f);

    rminimap.markers.push_back({marker.x, marker.y, marker.z, 0.0f});
  }

  f.WriteLE<u32>(rminimap.num_markers);
  f.AlignData(0x10);
  f.WriteArrayLE(rminimap.markers);
}

Buffer Export(const MinimapConfig& minimap) {
  BufferRW f;
  Export_(minimap, f);
  return f;
}

void Export(const MinimapConfig& minimap, Writer& f) { Export_(minimap, f); }

void Export(const MinimapConfig& minimap, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(minimap, f);
}

std::vector<SPosition> Convert(const MinimapConfig& minimap, unsigned int texture_width) {
  std::vector<SPosition> minimap_pos;

  minimap_pos.reserve(2 + minimap.markers.size());

  const f32 world_width = static_cast<f32>(texture_width) / minimap.pixels_per_unit;

  glm::vec3 top_left{0.0f};

  top_left.x = minimap.anchor_x - (kScreenHalfWidth_ / minimap.pixels_per_unit);
  top_left.z = minimap.anchor_z - (kScreenHalfHeight_ / minimap.pixels_per_unit);

  auto& world_anchor_top_left = minimap_pos.emplace_back();

  world_anchor_top_left.name = "00_top_left_anchor";

  world_anchor_top_left.translation = top_left;

  auto& world_anchor_top_right = minimap_pos.emplace_back(world_anchor_top_left);

  world_anchor_top_right.name = "01_top_right_anchor";

  world_anchor_top_right.id = 1;

  world_anchor_top_right.translation.x += world_width;

  for (std::size_t i = 0; i < minimap.markers.size(); i++) {
    auto& marker = minimap.markers[i];

    auto& marker_world = minimap_pos.emplace_back();

    marker_world.id = i + 2;

    marker_world.name = utl::string::PrependZero(marker_world.id) + "_marker";

    marker_world.translation.x = (marker.x + (minimap.pixels_per_unit * top_left.x)) / minimap.pixels_per_unit;
    marker_world.translation.y = 0.0f;
    marker_world.translation.z = (marker.y + (minimap.pixels_per_unit * top_left.z)) / minimap.pixels_per_unit;

    marker_world.translation = marker_world.translation;

    marker_world.rotation = glm::angleAxis(glm::radians(marker.z), glm::vec3(0.0f, 1.0f, 0.0f));
  }

  return minimap_pos;
}

MinimapConfig Convert(const std::vector<SPosition>& minimap_pos, unsigned int texture_width) {
  NNL_EXPECTS(minimap_pos.size() >= 2);
  NNL_EXPECTS(texture_width <= 1024);

  MinimapConfig minimap;

  auto& world_anchor_top_left = minimap_pos.at(0);

  auto& world_anchor_top_right = minimap_pos.at(1);

  const f32 world_width = glm::length(world_anchor_top_right.translation - world_anchor_top_left.translation);

  const f32 texture_width_f = static_cast<f32>(texture_width);

  minimap.pixels_per_unit = texture_width_f / world_width;

  minimap.anchor_x = world_anchor_top_left.translation.x + (kScreenHalfWidth_ / minimap.pixels_per_unit);
  minimap.anchor_z = world_anchor_top_left.translation.z + (kScreenHalfHeight_ / minimap.pixels_per_unit);

  for (std::size_t i = 2; i < minimap_pos.size(); i++) {
    auto& marker_world = minimap_pos[i];

    auto& marker = minimap.markers.emplace_back();

    marker.x = std::round((marker_world.translation.x - world_anchor_top_left.translation.x) * minimap.pixels_per_unit);
    marker.y = std::round((marker_world.translation.z - world_anchor_top_left.translation.z) * minimap.pixels_per_unit);

    marker.x = std::clamp(marker.x, 0.0f, texture_width_f);
    marker.y = std::clamp(marker.y, 0.0f, texture_width_f);

    marker.z = std::round(utl::math::QuatToEuler(marker_world.rotation).y);

    if (marker.z < 0.0f) marker.z += 360.0f;

    marker.x = std::abs(marker.x);
    marker.y = std::abs(marker.y);
    marker.z = std::abs(marker.z);
  }

  return minimap;
}

}  // namespace minimap
}  // namespace nnl
