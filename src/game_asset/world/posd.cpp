#include "NNL/game_asset/world/posd.hpp"

#include "NNL/utility/math.hpp"
#include "NNL/utility/string.hpp"
namespace nnl {
namespace posd {

std::vector<SPosition> Convert(const PositionData& posd) {
  std::vector<SPosition> positions;

  positions.reserve(posd.size());

  for (std::size_t i = 0; i < posd.size(); i++) {
    auto pos = posd.at(i);

    SPosition& spos = positions.emplace_back();

    spos.name = utl::string::PrependZero(i) + (pos.id == 0 ? "_player" : "");

    spos.id = pos.id;

    spos.scale = glm::vec3(pos.radius);
    spos.rotation = glm::quat(glm::radians(glm::vec3(0.0f, pos.rotation, 0.0f)));
    spos.translation = pos.position;
  }
  return positions;
}

PositionData Convert(const std::vector<SPosition>& positions) {
  PositionData posd;

  posd.reserve(positions.size());

  for (std::size_t i = 0; i < positions.size(); i++) {
    auto& spos = positions.at(i);

    Position pos;

    pos.id = spos.id;

    pos.position = spos.translation;
    pos.radius = (spos.scale.x + spos.scale.y + spos.scale.z) / 3.0f;
    auto euler = utl::math::QuatToEuler(spos.rotation);

    auto r = euler.y;

    // In cases yaw becomes > 90.0, glm returns a similar rotation but with yaw < 90.0.
    // Undo this:
    if (glm::abs(euler.x) >= 180.0f || glm::abs(euler.z) >= 180.0f) {
      if (r > 0)
        r = 180.0f - r;
      else
        r = -1.0f * (r + 180.0f);
    }

    pos.rotation = r;

    if (pos.rotation < 0) pos.rotation = 360.0f + pos.rotation;

    posd.push_back(pos);
  }
  return posd;
}

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();
  if (data_size < sizeof(RHeader)) return false;

  auto rheader = f.ReadLE<RHeader>();

  if (rheader.magic_bytes != kMagicBytes) return false;

  if (data_size < sizeof(RHeader) + rheader.num_positions * sizeof(RPosition)) return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

namespace raw {

PositionData Convert(const RPositionData& posd) {
  PositionData positions;

  for (auto& rpos : posd.positions) {
    float rotation = utl::math::FixedToFloat<i16, 4>(rpos.rotation) * 180.0f;
    positions.push_back(
        Position{rpos.id, glm::vec3(rpos.position.x, rpos.position.y, rpos.position.z), rpos.radius, rotation});
  }

  return positions;
}

RPositionData Parse(Reader& f) {
  f.Seek(0);
  RPositionData rposd;

  NNL_TRY {
    rposd.header = f.ReadLE<RHeader>();

    if (rposd.header.magic_bytes != kMagicBytes) NNL_THROW(ParseError(NNL_SRCTAG("invalid magic bytes")));

    rposd.positions = f.ReadArrayLE<RPosition>(rposd.header.num_positions);
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_SRCTAG(e.what())}); }
  return rposd;
}

}  // namespace raw

PositionData Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

PositionData Import(Reader& f) { return Import_(f); }

PositionData Import(BufferView buffer) { return Import_(buffer); }

PositionData Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const PositionData& posd, Writer& f) {
  using namespace raw;
  f.Seek(0);
  RHeader header;

  header.num_positions = utl::data::narrow<u32>(posd.size());

  f.WriteLE(header);

  for (u32 i = 0; i < posd.size(); i++) {
    auto pos = posd.at(i);
    NNL_EXPECTS(pos.rotation >= -360.0f && pos.rotation <= 360.0f);
    NNL_EXPECTS(pos.radius >= 0.0f);
    RPosition rpos;
    rpos.id = pos.id;
    rpos.radius = pos.radius;
    rpos.rotation = utl::math::FloatToFixed<i16, 4>(pos.rotation / 180.0f);
    rpos.position = {pos.position.x, pos.position.y, pos.position.z};
    f.WriteLE(rpos);
  }
}

Buffer Export(const PositionData& posd) {
  BufferRW f;
  Export_(posd, f);
  return f;
}

void Export(const PositionData& posd, Writer& f) { Export_(posd, f); }

void Export(const PositionData& posd, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(posd, f);
}

}  // namespace posd
}  // namespace nnl
