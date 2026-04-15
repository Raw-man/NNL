#include "NNL/game_asset/visual/lit.hpp"

#include "NNL/utility/color.hpp"
#include "NNL/utility/math.hpp"

namespace nnl {
namespace lit {

std::vector<nnl::SLight> Convert(const Lit& lit) {
  std::vector<nnl::SLight> slights;
  for (std::size_t i = 0; i < lit.lights.size(); i++) {
    auto& light = lit.lights.at(i);

    if (!light.active) continue;

    auto& slight = slights.emplace_back();

    slight.name = "light_" + std::to_string(i);
    slight.color = utl::color::IntToFloat(light.diffuse);
    slight.direction = glm::normalize(light.direction) * -1.0f;
  }

  return slights;
}

Lit Convert(const std::vector<nnl::SLight>& slights, glm::vec3 ambient, bool enable_specular,
            float character_brightness) {
  Lit lit;

  glm::vec3 global_ambient{0};

  std::size_t num_directional = 0;

  for (std::size_t i = 0; i < slights.size(); i++) {
    auto& slight = slights[i];
    NNL_EXPECTS_DBG(utl::math::IsFinite(slight.direction));

    if (num_directional >= 3) continue;

    auto& light = lit.lights.at(num_directional++);

    light.active = true;

    light.diffuse = utl::color::FloatToInt(slight.color);

    light.direction = glm::normalize(slight.direction) * -1.0f;

    global_ambient += slight.color * (0.4f / (float)std::min<std::size_t>(slights.size(), 3U));
  }

  if (enable_specular) lit.lights[0].specular = 0xFF'FF'FF'FF;

  lit.character_brightness = utl::color::FloatToInt(character_brightness);

  if (ambient == glm::vec3(0))
    lit.global_ambient = utl::color::FloatToInt(global_ambient);
  else
    lit.global_ambient = utl::color::FloatToInt(ambient);

  lit.cel_shadow_light_direction = lit.lights[0].direction;

  return lit;
}

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  if (f.Len() < 0x30) return false;

  RLit rlit;
  // some modded configs have this size
  // the rest is unused anyway

  NNL_TRY { f.ReadBuf(&rlit, 0x30); }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_SRCTAG(e.what())}); }

  if (rlit.magic_bytes != kMagicBytes) return false;

  if ((rlit.active_flags & 0xF0) != 0) return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

namespace raw {

Lit Convert(const RLit& rlit) {
  Lit lit;

  for (std::size_t i = 0; i < lit.lights.size(); i++) {
    auto& light = lit.lights[i];

    u32 mask = 1 << i;

    light.active = (rlit.active_flags & mask) != 0;

    light.direction = glm::vec3(utl::math::FixedToFloat(rlit.lights[i].direction.x),
                                utl::math::FixedToFloat(rlit.lights[i].direction.y),
                                utl::math::FixedToFloat(rlit.lights[i].direction.z));

    light.diffuse = 0xFF'00'00'00 | (rlit.lights[i].diffuse.z << 16) | (rlit.lights[i].diffuse.y << 8) |
                    (rlit.lights[i].diffuse.x << 0);

    light.specular = 0xFF'00'00'00 | (rlit.lights[i].specular.z << 16) | (rlit.lights[i].specular.y << 8) |
                     (rlit.lights[i].specular.x << 0);
  }

  lit.cel_shadow_light_direction = glm::vec3(utl::math::FixedToFloat(rlit.shadow_light_direction.x),
                                             utl::math::FixedToFloat(rlit.shadow_light_direction.y),
                                             utl::math::FixedToFloat(rlit.shadow_light_direction.z));

  lit.character_brightness = rlit.character_brightness;

  lit.global_ambient =
      0xFF'00'00'00 | (rlit.global_ambient.z << 16) | (rlit.global_ambient.y << 8) | (rlit.global_ambient.x << 0);

  return lit;
}

RLit Parse(Reader& f) {
  f.Seek(0);
  RLit rlit;

  f.ReadBuf(&rlit, 0x30);

  if (rlit.magic_bytes != kMagicBytes) NNL_THROW(ParseError(NNL_SRCTAG("invalid magic bytes")));

  return rlit;
}
}  // namespace raw

Lit Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

Lit Import(Reader& f) { return Import_(f); }

Lit Import(BufferView buffer) { return Import_(buffer); }

Lit Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const Lit& lit, Writer& f) {
  using namespace raw;
  f.Seek(0);
  RLit rlit;

  for (std::size_t i = 0; i < 3; i++) {
    auto& light = lit.lights[i];
    NNL_EXPECTS(!light.active || (light.direction.x != 0.0f || light.direction.y != 0.0f || light.direction.z != 0.0f));
    rlit.active_flags |= ((u8)light.active) << i;

    glm::vec3 direction = light.direction;

    rlit.lights[i].direction =
        Vec3<i8>{utl::math::FloatToFixed<i8>(direction.x), utl::math::FloatToFixed<i8>(direction.y),
                 utl::math::FloatToFixed<i8>(direction.z)};

    rlit.lights[i].diffuse =
        Vec3<u8>{(u8)(light.diffuse & 0xFF), (u8)((light.diffuse >> 8) & 0xFF), (u8)((light.diffuse >> 16) & 0xFF)};

    rlit.lights[i].specular =
        Vec3<u8>{(u8)(light.specular & 0xFF), (u8)((light.specular >> 8) & 0xFF), (u8)((light.specular >> 16) & 0xFF)};
  }

  rlit.lights[3] = RLight{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

  rlit.shadow_light_direction = Vec3<i8>{utl::math::FloatToFixed<i8>(lit.cel_shadow_light_direction.x),
                                         utl::math::FloatToFixed<i8>(lit.cel_shadow_light_direction.y),
                                         utl::math::FloatToFixed<i8>(lit.cel_shadow_light_direction.z)};

  rlit.character_brightness = lit.character_brightness;

  rlit.global_ambient = Vec3<u8>{(u8)(lit.global_ambient & 0xFF), (u8)((lit.global_ambient >> 8) & 0xFF),
                                 (u8)((lit.global_ambient >> 16) & 0xFF)};
  f.WriteLE(rlit);
}

Buffer Export(const Lit& lit) {
  BufferRW f;
  Export_(lit, f);
  return f;
}

void Export(const Lit& lit, Writer& f) { Export_(lit, f); }

void Export(const Lit& lit, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(lit, f);
}

}  // namespace lit
}  // namespace nnl
