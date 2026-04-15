#include "NNL/game_asset/interaction/colbox.hpp"

namespace nnl {
namespace colbox {

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();

  if (data_size < sizeof(RHeader)) return false;

  auto rheader = f.ReadLE<RHeader>();

  if (rheader.magic_bytes != kMagicBytes) return false;

  if (rheader.offset_colbox_attack < sizeof(RHeader) ||
      data_size < rheader.offset_colbox_attack + rheader.num_colbox_attack * sizeof(RHitboxHeader))
    return false;

  if (rheader.offset_colbox_dmg < sizeof(RHeader) ||
      data_size < rheader.offset_colbox_dmg + rheader.num_colbox_dmg * sizeof(RCollisionBox14))
    return false;

  if (rheader.offset_colbox_entities < sizeof(RHeader) ||
      data_size < rheader.offset_colbox_entities + rheader.num_colbox_entities * sizeof(RCollisionBox18))
    return false;

  if (rheader.offset_colbox_enviroment < sizeof(RHeader) ||
      data_size < rheader.offset_colbox_enviroment + rheader.num_colbox_enviroment * sizeof(RCollisionBox14))
    return false;

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

namespace raw {

RCollisionBoxConfig Parse(Reader& f) {
  RCollisionBoxConfig rhitbox_config;
  f.Seek(0);

  NNL_TRY {
    rhitbox_config.header = f.ReadLE<RHeader>();
    auto& header = rhitbox_config.header;

    if (header.magic_bytes != kMagicBytes) NNL_THROW(ParseError(NNL_ERMSG("invalid magic bytes")));

    f.Seek(header.offset_colbox_enviroment);
    rhitbox_config.col_enviroment = f.ReadArrayLE<RCollisionBox14>(header.num_colbox_enviroment);

    f.Seek(header.offset_colbox_unknown);
    rhitbox_config.colbox_unknown = f.ReadArrayLE<RCollisionBox14>(header.num_colbox_unknown);

    // some unknown structure may be here

    f.Seek(header.offset_colbox_entities);
    rhitbox_config.colbox_entities = f.ReadArrayLE<RCollisionBox18>(header.num_colbox_entities);

    f.Seek(header.offset_colbox_dmg);
    rhitbox_config.colbox_damage = f.ReadArrayLE<RCollisionBox14>(header.num_colbox_dmg);

    f.Seek(header.offset_colbox_attack);
    rhitbox_config.colbox_attack = f.ReadArrayLE<RHitboxHeader>(header.num_colbox_attack);

    for (auto& attack_header : rhitbox_config.colbox_attack) {
      f.Seek(attack_header.offset_hitboxes);
      rhitbox_config.hitboxes[attack_header.offset_hitboxes] = f.ReadArrayLE<RHitbox>(attack_header.num_hitboxes);
      for (auto& attack_argument : rhitbox_config.hitboxes[attack_header.offset_hitboxes]) {
        f.Seek(attack_argument.offset_colboxes);
        rhitbox_config.hitbox_colboxes[attack_argument.offset_colboxes] =
            f.ReadArrayLE<RCollisionBox20>(attack_argument.num_colboxes);
      }
    }
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_ERMSG(e.what())}); }
  return rhitbox_config;
}

ColBoxConfig Convert(const RCollisionBoxConfig& rhitbox_config) {
  ColBoxConfig hitbox_config;

  for (auto& rhitbox : rhitbox_config.col_enviroment)
    hitbox_config.col_enviroment.push_back(
        {rhitbox.bone_target, rhitbox.unknown2,
         glm::vec3(rhitbox.translation.x, rhitbox.translation.y, rhitbox.translation.z), rhitbox.size});

  for (auto& rhitbox : rhitbox_config.colbox_unknown)
    hitbox_config.col_unknown.push_back({rhitbox.bone_target, rhitbox.unknown2,
                                         glm::vec3(rhitbox.translation.x, rhitbox.translation.y, rhitbox.translation.z),
                                         rhitbox.size});

  for (auto& rhitbox : rhitbox_config.colbox_entities)
    hitbox_config.col_entities.push_back(
        {rhitbox.bone_target, rhitbox.unknown2,
         glm::vec3(rhitbox.translation.x, rhitbox.translation.y, rhitbox.translation.z), rhitbox.size,
         rhitbox.unknown14});

  for (auto& rhitbox : rhitbox_config.colbox_damage)
    hitbox_config.col_damage.push_back({rhitbox.bone_target, rhitbox.unknown2,
                                        glm::vec3(rhitbox.translation.x, rhitbox.translation.y, rhitbox.translation.z),
                                        rhitbox.size});

  for (auto& rattack_config : rhitbox_config.colbox_attack) {
    auto& rattack_arguments = rhitbox_config.hitboxes.at(rattack_config.offset_hitboxes);

    for (auto& rattack_argument : rattack_arguments) {
      HitboxConfig attack_argument;
      attack_argument.dmg_start_frame = rattack_argument.dmg_start;
      attack_argument.dmg_end_frame = rattack_argument.dmg_end;
      attack_argument.effect_id = rattack_argument.effect_id;
      attack_argument.unknownC = rattack_argument.unknownC;
      attack_argument.unknownD = rattack_argument.unknownD;
      attack_argument.unknownE = rattack_argument.unknownE;

      for (auto& rhitbox : rhitbox_config.hitbox_colboxes.at(rattack_argument.offset_colboxes)) {
        attack_argument.colboxes.push_back(
            {rhitbox.bone_target, rhitbox.distance_attack,
             glm::vec3(rhitbox.translation.x, rhitbox.translation.y, rhitbox.translation.z),
             glm::vec3(rhitbox.translation_2.x, rhitbox.translation_2.y, rhitbox.translation_2.z), rhitbox.size});
      }

      u16 numeric_id = rattack_config.action_id;

      action::Id id{static_cast<action::ActionCategory>((numeric_id & 0xC000) >> 14),
                    utl::data::narrow<u8>(numeric_id & action::kMaxActionIndex)};

      hitbox_config.col_attack[id].push_back(std::move(attack_argument));
    }
  }

  return hitbox_config;
}

}  // namespace raw

ColBoxConfig Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

ColBoxConfig Import(Reader& f) { return Import_(f); }

ColBoxConfig Import(BufferView buffer) { return Import_(buffer); }

ColBoxConfig Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const ColBoxConfig& colbox_config, Writer& f) {
  using namespace raw;
  f.Seek(0);
  RHeader header;
  header.num_colbox_enviroment = utl::data::narrow<u16>(colbox_config.col_enviroment.size());

  header.num_colbox_dmg = utl::data::narrow<u16>(colbox_config.col_damage.size());

  header.num_colbox_unknown = utl::data::narrow<u16>(colbox_config.col_unknown.size());

  header.num_colbox_entities = utl::data::narrow<u16>(colbox_config.col_entities.size());

  header.num_colbox_attack = utl::data::narrow<u16>(colbox_config.col_attack.size());

  header.offset_colbox_enviroment = sizeof(RHeader);

  auto header_off = f.WriteLE(header);

  for (auto& colbox : colbox_config.col_enviroment) {
    RCollisionBox14 rcolbox;
    rcolbox.bone_target = colbox.bone_target;
    rcolbox.unknown2 = colbox.unknown2;
    rcolbox.translation = Vec3<float>{colbox.translation.x, colbox.translation.y, colbox.translation.z};
    rcolbox.size = colbox.size;

    f.WriteLE(rcolbox);
  }

  f.AlignData(sizeof(float), 0);

  header_off->*& RHeader::offset_unknown = utl::data::narrow<u32>(f.Tell());

  header_off->*& RHeader::offset_colbox_unknown = utl::data::narrow<u32>(f.Tell());

  for (auto& colbox : colbox_config.col_unknown) {
    RCollisionBox14 rcolbox;
    rcolbox.bone_target = colbox.bone_target;
    rcolbox.unknown2 = colbox.unknown2;
    rcolbox.translation = Vec3<float>{colbox.translation.x, colbox.translation.y, colbox.translation.z};
    rcolbox.size = colbox.size;

    f.WriteLE(rcolbox);
  }

  f.AlignData(sizeof(float), 0);

  header_off->*& RHeader::offset_colbox_entities = utl::data::narrow<u32>(f.Tell());

  for (auto& colbox : colbox_config.col_entities) {
    RCollisionBox18 rcolbox;
    rcolbox.bone_target = colbox.bone_target;
    rcolbox.unknown2 = colbox.unknown2;
    rcolbox.translation = Vec3<float>{colbox.translation.x, colbox.translation.y, colbox.translation.z};
    rcolbox.size = colbox.size;
    rcolbox.unknown14 = colbox.unknown14;

    f.WriteLE(rcolbox);
  }

  f.AlignData(sizeof(float), 0);

  header_off->*& RHeader::offset_colbox_dmg = utl::data::narrow<u32>(f.Tell());

  for (auto& colbox : colbox_config.col_damage) {
    RCollisionBox14 rcolbox;
    rcolbox.bone_target = colbox.bone_target;
    rcolbox.unknown2 = colbox.unknown2;
    rcolbox.translation = Vec3<float>{colbox.translation.x, colbox.translation.y, colbox.translation.z};
    rcolbox.size = colbox.size;

    f.WriteLE(rcolbox);
  }

  f.AlignData(sizeof(u32), 0);

  header_off->*& RHeader::offset_colbox_attack = utl::data::narrow<u32>(f.Tell());

  auto hitbox_header_off = f.MakeOffsetLE<RHitboxHeader>();

  for (auto& [id, hitboxes] : colbox_config.col_attack) {
    NNL_EXPECTS(id.action_index <= action::kMaxActionIndex);
    RHitboxHeader rhitbox_header;
    u16 numeric_id = static_cast<u16>(utl::data::as_int(id.action_category) << 14 | id.action_index);
    rhitbox_header.action_id = numeric_id;
    rhitbox_header.num_hitboxes = utl::data::narrow<u16>(hitboxes.size());
    f.WriteLE(rhitbox_header);
  }

  f.AlignData(sizeof(u32), 0);

  auto hitbox_off = f.MakeOffsetLE<RHitbox>();

  for (auto& hitbox_config : colbox_config.col_attack) {
    hitbox_header_off->*& RHitboxHeader::offset_hitboxes = utl::data::narrow<u32>(f.Tell());

    for (auto& hitbox : hitbox_config.second) {
      NNL_EXPECTS(hitbox.dmg_end_frame >= hitbox.dmg_start_frame);
      RHitbox rhitbox;
      rhitbox.num_colboxes = utl::data::narrow<u16>(hitbox.colboxes.size());
      rhitbox.dmg_start = hitbox.dmg_start_frame;
      rhitbox.dmg_end = hitbox.dmg_end_frame;
      rhitbox.effect_id = hitbox.effect_id;
      rhitbox.unknownC = hitbox.unknownC;
      rhitbox.unknownD = hitbox.unknownD;
      rhitbox.unknownE = hitbox.unknownE;
      f.WriteLE(rhitbox);
    }

    ++hitbox_header_off;
  }

  f.AlignData(sizeof(float), 0);

  for (auto& hitbox_config : colbox_config.col_attack) {
    for (auto& hitbox : hitbox_config.second) {
      hitbox_off->*& RHitbox::offset_colboxes = utl::data::narrow<u32>(f.Tell());

      for (auto& colbox : hitbox.colboxes) {
        RCollisionBox20 rcolbox;
        rcolbox.bone_target = colbox.bone_target;
        rcolbox.distance_attack = colbox.distance_attack;
        rcolbox.translation = Vec3<float>{colbox.translation.x, colbox.translation.y, colbox.translation.z};
        rcolbox.translation_2 = Vec3<float>{colbox.translation_2.x, colbox.translation_2.y, colbox.translation_2.z};
        rcolbox.size = colbox.size;
        f.WriteLE(rcolbox);
      }

      ++hitbox_off;
    }
  }
}

Buffer Export(const ColBoxConfig& colbox_config) {
  BufferRW f;
  Export_(colbox_config, f);
  return f;
}

void Export(const ColBoxConfig& colbox_config, Writer& f) { Export_(colbox_config, f); }

void Export(const ColBoxConfig& colbox_config, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(colbox_config, f);
}

}  // namespace colbox
}  // namespace nnl
