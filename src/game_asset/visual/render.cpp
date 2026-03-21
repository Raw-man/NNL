#include "NNL/game_asset/visual/render.hpp"

namespace nnl {

namespace render {

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  if (f.Len() < sizeof(RRenderConfig)) return false;

  auto dis = f.ReadLE<RRenderConfig>();

  if (dis.magic_bytes != kMagicBytes || dis.unknown4 != 0 || dis.unknown8 != 0 || dis.unknownC != 0) return false;

  if ((dis.distance_fog_red & 0xFF'FF'FF'00) != 0) return false;
  if ((dis.distance_fog_green & 0xFF'FF'FF'00) != 0) return false;
  if ((dis.distance_fog_blue & 0xFF'FF'FF'00) != 0) return false;
  if ((dis.distance_transition_translucency & 0xFF'FF'FF'00) != 0) return false;
  if ((dis.bloom_translucency & 0xFF'FF'FF'00) != 0) return false;
  if (dis.enable_buffer_effects > 1) return false;

  // Check for unreasonable values
  if (dis.draw_distance_far < dis.draw_distance_near || dis.draw_distance_far < 0.0f ||
      dis.draw_distance_far > 512000.0f)
    return false;

  if (dis.padding0 != dis.padding1) return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

namespace raw {

RenderConfig Convert(const RRenderConfig& rdistance) {
  RenderConfig distance;
  distance.bloom_translucency = rdistance.bloom_translucency;
  distance.mipmap_bias = rdistance.mipmap_bias;
  distance.mipmap_slope = rdistance.mipmap_slope;
  distance.fog_near = rdistance.draw_distance_near;
  distance.fog_draw_distance_far = rdistance.draw_distance_far;

  distance.fog_color = 0xFF'00'00'00 | (rdistance.distance_fog_blue << 16) | (rdistance.distance_fog_green << 8) |
                       (rdistance.distance_fog_red);

  distance.distance_transition_translucency = rdistance.distance_transition_translucency;
  return distance;
}

RRenderConfig Parse(Reader& f) {
  f.Seek(0);
  RRenderConfig rdistance;

  NNL_TRY { rdistance = f.ReadLE<RRenderConfig>(); }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_ERMSG(e.what())}); }

  if (rdistance.magic_bytes != kMagicBytes) NNL_THROW(ParseError(NNL_ERMSG("invalid magic bytes")));
  return rdistance;
}

}  // namespace raw

RenderConfig Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

RenderConfig Import(Reader& f) { return Import_(f); }

RenderConfig Import(BufferView buffer) { return Import_(buffer); }

RenderConfig Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const RenderConfig& distance, Writer& f) {
  using namespace raw;
  f.Seek(0);
  NNL_EXPECTS(distance.fog_near >= -2048.0f);  // It may be negative
  NNL_EXPECTS(distance.fog_draw_distance_far >= 0.0f);
  NNL_EXPECTS(distance.fog_draw_distance_far >= distance.fog_near);
  NNL_EXPECTS(distance.fog_draw_distance_far <= 512000.0f);
  NNL_EXPECTS(std::abs(distance.mipmap_bias) <= 15.0f);
  NNL_EXPECTS(std::abs(distance.mipmap_slope) <= 15.0f);

  RRenderConfig rdistance;
  rdistance.enable_buffer_effects = 1;  // re-enabled anyway even if 0
  rdistance.bloom_translucency = distance.bloom_translucency;

  rdistance.mipmap_bias = distance.mipmap_bias;
  rdistance.mipmap_slope = distance.mipmap_slope;
  rdistance.draw_distance_near = distance.fog_near;
  rdistance.draw_distance_far = distance.fog_draw_distance_far;

  rdistance.distance_fog_blue = (distance.fog_color >> 16) & 0xFF;
  rdistance.distance_fog_green = (distance.fog_color >> 8) & 0xFF;
  rdistance.distance_fog_red = (distance.fog_color >> 0) & 0xFF;

  rdistance.distance_transition_translucency = distance.distance_transition_translucency;

  f.WriteLE(rdistance);
}

Buffer Export(const RenderConfig& distance) {
  BufferRW f;
  Export_(distance, f);
  return f;
}

void Export(const RenderConfig& distance, Writer& f) { Export_(distance, f); }

void Export(const RenderConfig& distance, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(distance, f);
}

}  // namespace render

}  // namespace nnl
