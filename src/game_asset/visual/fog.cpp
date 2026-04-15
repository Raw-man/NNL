#include "NNL/game_asset/visual/fog.hpp"

namespace nnl {
namespace fog {

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  if (f.Len() < sizeof(RFog)) return false;

  auto magic_bytes = f.ReadLE<u32>();

  if (magic_bytes != kMagicBytes) return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

namespace raw {

Fog Convert(const RFog& rfog) {
  Fog fog;
  fog.far_ = rfog.far_;
  fog.near_ = rfog.near_;
  fog.color = rfog.color;
  return fog;
}

RFog Parse(Reader& f) {
  f.Seek(0);

  RFog rfog;

  NNL_TRY { rfog = f.ReadLE<RFog>(); }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_SRCTAG(e.what())}); }

  if (rfog.magic_bytes != kMagicBytes) NNL_THROW(ParseError(NNL_SRCTAG("invalid magic bytes")));

  return rfog;
}

}  // namespace raw

Fog Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

Fog Import(Reader& f) { return Import_(f); }

Fog Import(BufferView buffer) { return Import_(buffer); }

Fog Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const Fog& _fog, Writer& f) {
  using namespace raw;
  f.Seek(0);
  NNL_EXPECTS(_fog.near_ >= -2048.0f);
  NNL_EXPECTS(_fog.far_ >= 0.0f);
  NNL_EXPECTS(_fog.far_ >= _fog.near_);
  NNL_EXPECTS(_fog.far_ <= 512000.0f);

  RFog rfog;
  rfog.far_ = _fog.far_;
  rfog.near_ = _fog.near_;
  rfog.color = _fog.color | 0xFF'00'00'00;

  f.WriteLE(rfog);
}

Buffer Export(const Fog& fog) {
  BufferRW f;
  Export_(fog, f);
  return f;
}

void Export(const Fog& fog, Writer& f) { Export_(fog, f); }

void Export(const Fog& fog, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(fog, f);
}

}  // namespace fog
}  // namespace nnl
