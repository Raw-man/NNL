#include <stb_image.h>
#include <stb_image_write.h>
#include <tiny_gltf.h>

#include <set>

#include "NNL/common/logger.hpp"
#include "NNL/common/src_info.hpp"
#include "NNL/simple_asset/sasset3d.hpp"
#include "NNL/simple_asset/svalue.hpp"  //move to other headers
#include "NNL/utility/color.hpp"
#include "NNL/utility/filesys.hpp"
#include "NNL/utility/math.hpp"
#include "NNL/utility/string.hpp"

namespace nnl {

class GLTFExporter {
 public:
  GLTFExporter(const SAsset3D& sasset, const std::filesystem::path& path, bool flip_uv, bool pack_textures)
      : sasset_(sasset), path_(path), buffer_{nullptr}, flip_uv_(flip_uv), pack_textures_(pack_textures) {
    Export();
  }

  GLTFExporter(const SAsset3D& sasset, Buffer* buffer, bool flip_uv, bool pack_textures)
      : sasset_(sasset), path_{}, buffer_(buffer), flip_uv_(flip_uv), pack_textures_(pack_textures) {
    Export();
  }

 private:
  BufferRW blob_;  // single buf for GLB data

  const SAsset3D& sasset_;

  const std::filesystem::path path_;  // output path

  Buffer* buffer_;  // output buffer

  const bool flip_uv_;

  const bool pack_textures_;

  std::set<std::string> extensions_used_;

  tinygltf::Model gltf_model_;

  tinygltf::TinyGLTF gltf_ctx_;

  std::size_t num_bones_ = 0;

  int max_texture_id_ = -1;

  STexture texture_placeholder_;

  std::vector<glm::mat4> inverse_bind_;

  std::map<std::size_t, int> mesh_group_node_id_;

  std::map<std::size_t, int> mesh_node_id_;

  std::map<std::size_t, std::size_t> exported_textures_;

  std::map<std::size_t, int> bone_joint_id_;

  std::map<int, std::size_t> joint_pos_;  // index of a joint id in a joint array

  const std::map<STextureFilter, int> kGltfToSTextureFilter{
      {STextureFilter::kNearest, TINYGLTF_TEXTURE_FILTER_NEAREST},
      {STextureFilter::kLinear, TINYGLTF_TEXTURE_FILTER_LINEAR},
      {STextureFilter::kNearestMipmapNearest, TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST},
      {STextureFilter::kLinearMipmapNearest, TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST},
      {STextureFilter::kNearestMipmapLinear, TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR},
      {STextureFilter::kLinearMipmapLinear, TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR}};

  std::vector<double> GLMMatToGLTF(const glm::mat4& m) {
    return {m[0][0], m[0][1], m[0][2], 0.0f, m[1][0], m[1][1], m[1][2], 0.0f,
            m[2][0], m[2][1], m[2][2], 0.0f, m[3][0], m[3][1], m[3][2], 1.0f};
  }

  std::tuple<std::vector<double>, std::vector<double>, std::vector<double>> GLMMatToGLTFTRS(const glm::mat4& m) {
    auto [scale, rotation, translation] = utl::math::Decompose(m);

    std::vector<double> scale_v{scale.x, scale.y, scale.z};
    std::vector<double> rotation_v{rotation.x, rotation.y, rotation.z, rotation.w};
    std::vector<double> translation_v{translation.x, translation.y, translation.z};

    return {scale_v, rotation_v, translation_v};
  }

  glm::mat4 MatResetRow(glm::mat4 m) {
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[3][3] = 1.0f;
    return m;
  }

  template <typename T>
  int WriteData(const std::vector<T>& data, int type, int component_type = TINYGLTF_COMPONENT_TYPE_FLOAT,
                int target = TINYGLTF_TARGET_ARRAY_BUFFER) {
    int accessor_id = gltf_model_.accessors.size();

    tinygltf::Accessor& accessor = gltf_model_.accessors.emplace_back();

    accessor.byteOffset = 0;

    accessor.componentType = component_type;

    accessor.type = type;

    accessor.count = data.size();

    accessor.bufferView = WriteBuffer(data, target);

    return accessor_id;
  }

  template <typename T>
  int WriteBuffer(const std::vector<T>& data, int target) {
    int buffer_view_id = gltf_model_.bufferViews.size();

    tinygltf::BufferView& buffer_view = gltf_model_.bufferViews.emplace_back();

    buffer_view.target = target;

    buffer_view.byteLength = sizeof(T) * data.size();

    buffer_view.buffer = 0;

    blob_.AlignData(0x10);

    buffer_view.byteOffset = blob_.Tell();

    blob_.WriteArrayLE(data);

    return buffer_view_id;
  }

  template <typename T>
  std::vector<double> ToVec(T vec) {
    std::vector<double> vector;

    vector.reserve(vec.length());

    for (std::size_t i = 0; i < vec.length(); i++) {
      vector.push_back(vec[i]);
    }

    return vector;
  }

  tinygltf::Value SValueToGLTFValue(const nnl::SValue& o) {
    using namespace tinygltf;

    Value val{};

    switch (o.GetType()) {
      case SValue::kObject: {
        Value::Object value_object;
        for (auto& [key, value] : o.Items()) {
          Value entry = SValueToGLTFValue(value);
          if (entry.Type() != NULL_TYPE) value_object.emplace(key, std::move(entry));
        }
        if (value_object.size() > 0) val = Value(std::move(value_object));
      } break;
      case SValue::kArray: {
        Value::Array value_array;
        value_array.reserve(o.Size());
        for (auto& value : o.Values()) {
          Value entry = SValueToGLTFValue(value);
          if (entry.Type() != NULL_TYPE) value_array.emplace_back(std::move(entry));
        }
        if (value_array.size() > 0) val = Value(std::move(value_array));
      } break;
      case SValue::kString:
        val = Value(o.Get<std::string>());
        break;
      case SValue::kBool:
        val = Value(o.Get<bool>());
        break;
      case SValue::kInt:
        val = Value(static_cast<int>(o.Get<i64>()));
        break;
      case SValue::kDouble:
        val = Value(o.Get<double>());
        break;
      case SValue::kNull:
      default:
        break;
    }

    return val;
  }

  void ProcessMesh(std::size_t smesh_id) {
    const SMesh& smesh = sasset_.model.meshes.at(smesh_id);

    if (smesh.indices.size() < 3 || smesh.vertices.empty()) {
      NNL_LOG_WARN("mesh \"" + smesh.name + "\" is empty");
      return;
    }
    const bool exp_as_group = !sasset_.visibility_animations.empty();
    // If visibility animation is used, export each mesh as a primitive of a single mesh
    if (exp_as_group) {
      if (mesh_group_node_id_.count(smesh.mesh_group) == 0) {
        int group_node_id = gltf_model_.nodes.size();
        mesh_node_id_.insert({smesh_id, group_node_id});
        gltf_model_.scenes.back().nodes.push_back(group_node_id);

        tinygltf::Node& node = gltf_model_.nodes.emplace_back();

        node.name = "mesh_group_" + utl::string::PrependZero(smesh.mesh_group);

        tinygltf::Value::Object ext_mesh_group_id = {{"mesh_group", tinygltf::Value((int)smesh.mesh_group)}};

        node.extras = tinygltf::Value(ext_mesh_group_id);

        mesh_group_node_id_[smesh.mesh_group] = group_node_id;

        tinygltf::Value::Object khr_vis_ext = {{"visible", tinygltf::Value(true)}};

        node.extensions["KHR_node_visibility"] = tinygltf::Value(khr_vis_ext);

        extensions_used_.insert("KHR_node_visibility");

        node.mesh = gltf_model_.meshes.size();

        tinygltf::Mesh& mesh = gltf_model_.meshes.emplace_back();

        mesh.name = smesh.name;
      }

      mesh_node_id_.insert({smesh_id, mesh_group_node_id_.at(smesh.mesh_group)});

    } else {
      // If no visibility animation present, export each mesh as a separate glTF mesh.
      mesh_node_id_.insert({smesh_id, (int)gltf_model_.nodes.size()});
      gltf_model_.scenes.back().nodes.push_back(gltf_model_.nodes.size());
      tinygltf::Node& node = gltf_model_.nodes.emplace_back();

      node.mesh = gltf_model_.meshes.size();

      node.name = "_n_" + utl::string::PrependZero(gltf_model_.meshes.size(), 3) + "_" + smesh.name;

      if (smesh.extras.IsObject() && !smesh.extras.IsEmpty())
        node.extras = SValueToGLTFValue(smesh.extras);
      else
        node.extras = tinygltf::Value(tinygltf::Value::Object());

      auto& extras = node.extras.Get<tinygltf::Value::Object>();

      if (smesh.mesh_group != 0) extras["mesh_group"] = tinygltf::Value((int)smesh.mesh_group);

      tinygltf::Mesh& mesh = gltf_model_.meshes.emplace_back();

      mesh.name = smesh.name;
    }

    tinygltf::Node& node = gltf_model_.nodes.at(mesh_node_id_.at(smesh_id));

    tinygltf::Mesh& mesh = gltf_model_.meshes.at(node.mesh);

    tinygltf::Primitive& primitive = mesh.primitives.emplace_back();

    // If meshes exported as primitives, store extras with them
    if (exp_as_group && smesh.extras.IsObject() && !smesh.extras.IsEmpty())
      primitive.extras = SValueToGLTFValue(smesh.extras);

    primitive.mode = TINYGLTF_MODE_TRIANGLES;

    primitive.material = smesh.material_id;

    tinygltf::Value::Array mappings;

    for (auto& [new_id, svariants] : smesh.material_variants) {
      tinygltf::Value::Array variants;

      for (auto svariant : svariants) variants.push_back(tinygltf::Value((int)svariant));

      tinygltf::Value::Object map{{"material", tinygltf::Value((int)new_id)}, {"variants", tinygltf::Value(variants)}};

      mappings.push_back(tinygltf::Value(map));
    }

    if (!mappings.empty()) {
      extensions_used_.insert("KHR_materials_variants");
      primitive.extensions["KHR_materials_variants"] =
          tinygltf::Value(tinygltf::Value::Object{{"mappings", tinygltf::Value(mappings)}});
    }

    {
      std::vector<Vec3<float>> positions;

      glm::vec3 min{std::numeric_limits<float>::max()};

      glm::vec3 max{std::numeric_limits<float>::lowest()};

      positions.reserve(smesh.vertices.size());

      for (auto& svertex : smesh.vertices) {
        max.x = std::max(svertex.position.x, max.x);

        max.y = std::max(svertex.position.y, max.y);

        max.z = std::max(svertex.position.z, max.z);

        min.x = std::min(svertex.position.x, min.x);

        min.y = std::min(svertex.position.y, min.y);

        min.z = std::min(svertex.position.z, min.z);

        positions.push_back({svertex.position.x, svertex.position.y, svertex.position.z});
      }

      primitive.attributes["POSITION"] = WriteData(positions, TINYGLTF_TYPE_VEC3);

      auto& accessor = gltf_model_.accessors.back();

      accessor.minValues = {min.x, min.y, min.z};

      accessor.maxValues = {max.x, max.y, max.z};
    }

    auto smat = sasset_.model.materials.at(smesh.material_id);

    if (smesh.uses_uv || smat.texture_id != -1) {
      std::vector<Vec2<float>> uvs;

      uvs.reserve(smesh.vertices.size());

      glm::mat3 flip_v{1.0f};

      if (flip_uv_) {
        flip_v[1][1] = -1;
        flip_v[2][1] = 1;
      }

      for (auto& svertex : smesh.vertices) {
        glm::vec2 uv = flip_v * glm::vec3(svertex.uv.x, svertex.uv.y, 1.0f);
        uvs.push_back({uv.x, uv.y});
      }

      primitive.attributes["TEXCOORD_0"] = WriteData(uvs, TINYGLTF_TYPE_VEC2);
    }

    if (!exp_as_group && smat.projection_mode == STextureProjection::kEnvironment) {
      // Hide this mesh to make the asset look nicer in viewers
      tinygltf::Value::Object khr_vis_ext = {{"visible", tinygltf::Value(false)}};

      extensions_used_.insert("KHR_node_visibility");

      node.extensions["KHR_node_visibility"] = tinygltf::Value(khr_vis_ext);
    }

    if (smesh.uses_normal) {
      std::vector<Vec3<float>> normals(smesh.vertices.size());

      for (std::size_t i = 0; i < smesh.vertices.size(); i++) {
        auto& svertex = smesh.vertices[i];
        auto& normal = svertex.normal;

        normals[i] = {normal.x, normal.y, normal.z};
      }

      primitive.attributes["NORMAL"] = WriteData(normals, TINYGLTF_TYPE_VEC3);
    }

    if (smesh.uses_color) {
      std::vector<Vec4<float>> colors(smesh.vertices.size());

      for (std::size_t i = 0; i < smesh.vertices.size(); i++) {
        auto& svertex = smesh.vertices[i];
        auto& color = svertex.color;
        // utl::color::SRGBToLinear(svertex.color)
        colors[i] = {color.x, color.y, color.z, color.w};
      }

      primitive.attributes["COLOR_0"] = WriteData(colors, TINYGLTF_TYPE_VEC4);
    }

    // Export weights only if the armature was exported
    if (!gltf_model_.skins.empty()) {
      std::vector<Vec4<u16>> joints_0(smesh.vertices.size(), {0});
      std::vector<Vec4<float>> weights_0(smesh.vertices.size(), {0.0f});

      std::vector<Vec4<u16>> joints_1(smesh.vertices.size(), {0});
      std::vector<Vec4<float>> weights_1(smesh.vertices.size(), {0.0f});

      bool export_joints_1 = false;

      for (std::size_t i = 0; i < smesh.vertices.size(); i++) {
        auto& svertex = smesh.vertices[i];
        auto& weight_0 = weights_0[i];
        auto& joint_0 = joints_0[i];

        auto& weight_1 = weights_1[i];
        auto& joint_1 = joints_1[i];

        for (std::size_t i = 0; i < 4; i++) {
          joint_0[i] = joint_pos_.at(bone_joint_id_.at(svertex.bones[i]));
          weight_0[i] = svertex.weights[i];
        }

        export_joints_1 |= (kMaxNumVertWeight > 4 && ((svertex.weights[4] > 0.0f) || (svertex.weights[5] > 0.0f) ||
                                                      (svertex.weights[6] > 0.0f) || (svertex.weights[7] > 0.0f)));

        for (std::size_t i = 4; i < svertex.weights.size(); i++) {
          joint_1[i - 4] = joint_pos_.at(bone_joint_id_.at(svertex.bones[i]));
          weight_1[i - 4] = svertex.weights[i];
        }
      }

      node.skin = 0;
      primitive.attributes["JOINTS_0"] =
          WriteData(joints_0, TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);

      primitive.attributes["WEIGHTS_0"] = WriteData(weights_0, TINYGLTF_TYPE_VEC4);

      if (export_joints_1) {
        primitive.attributes["JOINTS_1"] =
            WriteData(joints_1, TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);

        primitive.attributes["WEIGHTS_1"] = WriteData(weights_1, TINYGLTF_TYPE_VEC4);
      }
    }

    {
      primitive.indices = WriteData(smesh.indices, TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
                                    TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);
    }
  }

  void ProcessBone(int node_id, const SBone& bone) {
    auto& node = gltf_model_.nodes.at(node_id);

    node.name = bone.name;

    node.scale = {bone.scale.x, bone.scale.y, bone.scale.z};

    node.rotation = {bone.rotation.x, bone.rotation.y, bone.rotation.z, bone.rotation.w};

    node.translation = {bone.translation.x, bone.translation.y, bone.translation.z};

    inverse_bind_.push_back(MatResetRow(bone.inverse));

    for (auto& child : bone.children) {
      int child_node_id = gltf_model_.nodes.size();

      bone_joint_id_[num_bones_++] = child_node_id;

      joint_pos_[child_node_id] = gltf_model_.skins.back().joints.size();

      gltf_model_.skins.back().joints.push_back(child_node_id);

      gltf_model_.nodes.at(node_id).children.push_back(child_node_id);

      gltf_model_.nodes.emplace_back();

      ProcessBone(child_node_id, child);
    }
  }

  void ProcessTexture(std::size_t tex_id) {
    const STexture& stexture = tex_id < sasset_.textures.size() ? sasset_.textures.at(tex_id) : texture_placeholder_;

    tinygltf::Texture& texture = gltf_model_.textures.emplace_back();

    if (stexture.extras.IsObject() && !stexture.extras.IsEmpty()) {
      texture.extras = SValueToGLTFValue(stexture.extras);
    }

    texture.name = tex_id < sasset_.textures.size() ? stexture.name : stexture.name + std::to_string(tex_id);

    texture.source = gltf_model_.images.size();

    texture.sampler = gltf_model_.samplers.size();

    tinygltf::Sampler& sampler = gltf_model_.samplers.emplace_back();

    sampler.minFilter = kGltfToSTextureFilter.at(stexture.min_filter);

    sampler.magFilter = kGltfToSTextureFilter.at(stexture.mag_filter);

    tinygltf::Image& image = gltf_model_.images.emplace_back();

    if (!pack_textures_) {
      image.bufferView = -1;

      image.mimeType = "";

      image.uri = texture.name + ".png";

      stexture.ExportPNG(path_.parent_path() / utl::filesys::u8path(image.uri), flip_uv_);
    } else {
      image.bufferView = WriteBuffer(stexture.ExportPNG(flip_uv_), 0);

      image.mimeType = "image/png";
    }

    image.component = 4;

    image.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;

    image.bits = 8;

    image.width = stexture.width;

    image.height = stexture.height;

    image.name = stexture.name;
  }

  void ProcessMaterial(std::size_t mat_id) {
    const SMaterial& smat = sasset_.model.materials.at(mat_id);

    tinygltf::Material& mat = gltf_model_.materials.emplace_back();

    mat.name = smat.name;

    mat.doubleSided = false;

    if (smat.extras.IsObject() && !smat.extras.IsEmpty())
      mat.extras = SValueToGLTFValue(smat.extras);
    else
      mat.extras = tinygltf::Value(tinygltf::Value::Object());

    auto& mat_ext = mat.extras.Get<tinygltf::Value::Object>();

    tinygltf::Value::Object omi_materials_blend = {};

    if (smat.alpha_mode == SBlendMode::kOpaque) mat.alphaMode = "OPAQUE";

    if ((smat.alpha_mode & SBlendMode::kClip) != SBlendMode::kOpaque) mat.alphaMode = "MASK";

    if ((smat.alpha_mode & SBlendMode::kAlpha) != SBlendMode::kOpaque) mat.alphaMode = "BLEND";

    if ((smat.alpha_mode & SBlendMode::kAdd) != SBlendMode::kOpaque) {
      mat_ext["blend"] = tinygltf::Value("add");
    }

    if ((smat.alpha_mode & SBlendMode::kSub) != SBlendMode::kOpaque) {
      mat_ext["blend"] = tinygltf::Value("sub");
    }

    /*
    switch (smat.alpha_mode) {
      case SAlphaMode::kOpaque:
        mat.alphaMode = "OPAQUE";
        break;

      case SAlphaMode::kBlend:
        mat.alphaMode = "BLEND";
        break;

      case SAlphaMode::kClip:
        mat.alphaMode = "MASK";
        break;

      case SAlphaMode::kAdd: {
        mat.alphaMode = "BLEND";

        mat_ext["blend"] = tinygltf::Value("add");

        //                omi_materials_blend = {{"alphaMode",
        //                tinygltf::Value("ADD")}};

        //                extensions_used.insert("OMI_materials_blend");

        //                mat.extensions["OMI_materials_blend"] =
        //                tinygltf::Value(omi_materials_blend);
      } break;

      case SAlphaMode::kSub: {
        mat.alphaMode = "BLEND";

        mat_ext["blend"] = tinygltf::Value("sub");

        //                omi_materials_blend = {{"alphaMode",
        //                tinygltf::Value("SUBTRACT")}};

        //                extensions_used.insert("OMI_materials_blend");

        //                mat.extensions["OMI_materials_blend"] =
        //                tinygltf::Value(omi_materials_blend);
      } break;
    }*/

    switch (smat.projection_mode) {
      case STextureProjection::kUV:
        break;

      case STextureProjection::kMatrix:
        mat_ext["projection"] = tinygltf::Value("mat");
        break;

      case STextureProjection::kEnvironment:
        mat_ext["projection"] = tinygltf::Value("env");
        break;
    }

    if (smat.vfx_group_id != 0) {
      mat_ext["vfx_group"] = tinygltf::Value(smat.vfx_group_id);
    }

    mat.extras = tinygltf::Value(mat_ext);

    if (smat.texture_id != -1) {
      max_texture_id_ = std::max(max_texture_id_, smat.texture_id);

      mat.pbrMetallicRoughness.baseColorTexture.index = smat.texture_id;

      extensions_used_.insert("KHR_texture_transform");

      mat.pbrMetallicRoughness.baseColorTexture.extensions["KHR_texture_transform"];
    }

    if (!smat.lit) {
      extensions_used_.insert("KHR_materials_unlit");

      mat.extensions["KHR_materials_unlit"];

      mat.pbrMetallicRoughness.baseColorFactor = ToVec(glm::vec4(utl::color::SRGBToLinear(smat.ambient), smat.opacity));
    } else {
      mat.pbrMetallicRoughness.baseColorFactor = ToVec(glm::vec4(utl::color::SRGBToLinear(smat.diffuse), smat.opacity));

      mat.emissiveFactor = ToVec(utl::color::SRGBToLinear(smat.emissive));

      double spec_intensity = smat.specular.x * 0.2125 + smat.specular.y * 0.7154 + smat.specular.z * 0.0721;

      mat.pbrMetallicRoughness.roughnessFactor = std::sqrt(2.0 / (smat.specular_power * spec_intensity + 2.0));

      auto specular = utl::color::SRGBToLinear(smat.specular);

      tinygltf::Value::Array spec_color = {tinygltf::Value(specular.x), tinygltf::Value(specular.y),
                                           tinygltf::Value(specular.z)};

      tinygltf::Value::Object khr_spec_ext = {{"specularColorFactor", tinygltf::Value(spec_color)},
                                              {"specularFactor", tinygltf::Value(spec_intensity > 0.01 ? 1.0 : 0.0)}};

      extensions_used_.insert("KHR_materials_specular");

      mat.extensions["KHR_materials_specular"] = tinygltf::Value(khr_spec_ext);

      mat.pbrMetallicRoughness.metallicFactor = 0.0;

      if (!(utl::math::IsApproxEqual(smat.ambient.r, smat.ambient.g, 0.05f) &&
            utl::math::IsApproxEqual(smat.ambient.g, smat.ambient.b, 0.05f))) {
        auto ambient = utl::color::SRGBToLinear(smat.ambient);

        tinygltf::Value::Array amb_color{tinygltf::Value(ambient.x), tinygltf::Value(ambient.y),
                                         tinygltf::Value(ambient.z)};

        tinygltf::Value::Object khr_sheen_ext{{"sheenColorFactor", tinygltf::Value(amb_color)},
                                              {"sheenRoughnessFactor", tinygltf::Value(0.5)}};

        extensions_used_.insert("KHR_materials_sheen");

        mat.extensions["KHR_materials_sheen"] = tinygltf::Value(khr_sheen_ext);
      }
    }
  }

  void ProcessAnimation(std::size_t sanimation_id) {
    const SAnimation& sanimation = sasset_.animations.at(sanimation_id);

    tinygltf::Animation& animation = gltf_model_.animations.emplace_back();

    animation.name = sanimation.name;

    NNL_EXPECTS_DBG(sanimation.duration != 0);

    if (sanimation.extras.IsObject() && !sanimation.extras.IsEmpty()) {
      animation.extras = SValueToGLTFValue(sanimation.extras);
    }

    for (std::size_t i = 0; i < sanimation.bone_channels.size(); i++) {
      auto& schannel = sanimation.bone_channels.at(i);

      auto target_node = bone_joint_id_.at(i);

      if (target_node == -1) continue;

      {
        tinygltf::AnimationChannel& channel = animation.channels.emplace_back();

        channel.target_node = target_node;

        channel.target_path = "rotation";

        channel.sampler = animation.samplers.size();

        tinygltf::AnimationSampler& sampler = animation.samplers.emplace_back();

        sampler.interpolation = "LINEAR";

        std::vector<float> keys;
        std::vector<Vec4<float>> values;

        keys.reserve(schannel.rotation_keys.size() + 1);
        values.reserve(schannel.rotation_keys.size() + 1);

        float min = 0.0f;

        float max = (sanimation.duration - 1) / 30.0f;

        assert(schannel.rotation_keys.empty() || schannel.rotation_keys.front().time == 0);
        assert(schannel.rotation_keys.empty() || schannel.rotation_keys.back().time == sanimation.duration - 1);

        for (auto& key : schannel.rotation_keys) {
          float time = key.time / 30.0f;

          keys.push_back(time);
          values.push_back({key.value.x, key.value.y, key.value.z, key.value.w});
        }

        assert(keys.size() == values.size());

        sampler.input = WriteData(keys, TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_FLOAT, 0);

        tinygltf::Accessor& accessor = gltf_model_.accessors.at(sampler.input);

        accessor.minValues = {min};
        accessor.maxValues = {max};

        sampler.output = WriteData(values, TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT, 0);
      }

      {
        tinygltf::AnimationChannel& channel = animation.channels.emplace_back();

        channel.target_node = target_node;

        channel.target_path = "translation";

        channel.sampler = animation.samplers.size();

        tinygltf::AnimationSampler& sampler = animation.samplers.emplace_back();

        sampler.interpolation = "LINEAR";

        std::vector<float> keys;
        std::vector<Vec3<float>> values;

        keys.reserve(schannel.translation_keys.size() + 1);
        values.reserve(schannel.translation_keys.size() + 1);

        float min = 0.0f;

        float max = (sanimation.duration - 1) / 30.0f;

        assert(schannel.translation_keys.empty() || schannel.translation_keys.front().time == 0);
        assert(schannel.translation_keys.empty() || schannel.translation_keys.back().time == sanimation.duration - 1);

        for (auto& key : schannel.translation_keys) {
          float time = key.time / 30.0f;

          keys.push_back(time);
          values.push_back({key.value.x, key.value.y, key.value.z});
        }

        assert(keys.size() == values.size());

        sampler.input = WriteData(keys, TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_FLOAT, 0);

        tinygltf::Accessor& accessor = gltf_model_.accessors.at(sampler.input);

        accessor.minValues = {min};
        accessor.maxValues = {max};

        sampler.output = WriteData(values, TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT, 0);
      }
      // scale
      {
        tinygltf::AnimationChannel& channel = animation.channels.emplace_back();

        channel.target_node = target_node;

        channel.target_path = "scale";

        channel.sampler = animation.samplers.size();

        tinygltf::AnimationSampler& sampler = animation.samplers.emplace_back();

        sampler.interpolation = "LINEAR";

        std::vector<float> keys;
        std::vector<Vec3<float>> values;

        keys.reserve(schannel.scale_keys.size() + 1);
        values.reserve(schannel.scale_keys.size() + 1);

        float min = 0.0f;

        float max = (sanimation.duration - 1) / 30.0f;

        assert(schannel.scale_keys.empty() || schannel.scale_keys.front().time == 0);

        assert(schannel.scale_keys.empty() || schannel.scale_keys.back().time == sanimation.duration - 1);

        for (auto& key : schannel.scale_keys) {
          float time = key.time / 30.0f;

          keys.push_back(time);
          values.push_back({key.value.x, key.value.y, key.value.z});
        }

        assert(keys.size() == values.size());

        sampler.input = WriteData(keys, TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_FLOAT, 0);

        tinygltf::Accessor& accessor = gltf_model_.accessors.at(sampler.input);

        accessor.minValues = {min};
        accessor.maxValues = {max};

        sampler.output = WriteData(values, TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT, 0);
      }
    }
  }

  void ProcessVisibilityAnimation(std::size_t animation_id) {
    if (gltf_model_.animations.size() <= animation_id) {
      tinygltf::Animation& animation = gltf_model_.animations.emplace_back();

      animation.name = utl::string::PrependZero(animation_id);
    }

    tinygltf::Animation& animation = gltf_model_.animations.at(animation_id);

    const SVisibilityAnimation& sanimation = sasset_.visibility_animations.at(animation_id);

    for (std::size_t i = 0; i < sanimation.visibility_channels.size(); i++) {
      auto& schannel = sanimation.visibility_channels.at(i);

      if (schannel.empty()) continue;

      tinygltf::AnimationChannel& channel = animation.channels.emplace_back();

      tinygltf::Value::Object ext_mesh_group_id = {{"mesh_group", tinygltf::Value((int)i)}};

      // There may be mesh groups without meshes
      if (mesh_group_node_id_.count(i) == 0) {
        int group_node_id = gltf_model_.nodes.size();

        gltf_model_.scenes.back().nodes.push_back(group_node_id);

        tinygltf::Node& node = gltf_model_.nodes.emplace_back();

        node.name = "mesh_group_" + utl::string::PrependZero(i);

        node.extras = tinygltf::Value(ext_mesh_group_id);

        mesh_group_node_id_[i] = group_node_id;

        tinygltf::Value::Object khr_vis_ext = {{"visible", tinygltf::Value(true)}};

        node.extensions["KHR_node_visibility"] = tinygltf::Value(khr_vis_ext);

        extensions_used_.insert("KHR_node_visibility");
      }

      channel.sampler = animation.samplers.size();

      tinygltf::AnimationSampler& sampler = animation.samplers.emplace_back();

      sampler.interpolation = "STEP";

      extensions_used_.insert("KHR_node_visibility");
      extensions_used_.insert("KHR_animation_pointer");

      channel.target_node = -1;

      channel.target_path = "pointer";

      std::string target =
          "/nodes/" + std::to_string(mesh_group_node_id_.at(i)) + "/extensions/KHR_node_visibility/visible";

      tinygltf::Value::Object khr_pointer = {{"pointer", tinygltf::Value(target)}};

      channel.target_extensions["KHR_animation_pointer"] = tinygltf::Value(khr_pointer);

      std::vector<float> keys;

      std::vector<u8> values_bool;

      keys.reserve(schannel.size() + 1);

      values_bool.reserve(schannel.size() + 1);

      u16 duration_ticks = animation_id < sasset_.animations.size() ? sasset_.animations.at(animation_id).duration : 1;

      assert(duration_ticks != 0);

      float min = 0.0f;

      float max = (duration_ticks - 1) / 30.0f;

      assert(schannel.empty() || schannel.front().time == 0);

      for (auto& key : schannel) {
        if (key.time >= duration_ticks) break;

        float time = key.time / 30.0f;

        keys.push_back(time);

        values_bool.push_back({(u8)key.value});
      }

      // Make sure the last keyframe is at max
      if (!keys.empty() && keys.back() < max) {
        keys.push_back(max);

        values_bool.push_back(values_bool.back());
      }

      assert(keys.size() == values_bool.size());

      sampler.input = WriteData(keys, TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_FLOAT, 0);

      tinygltf::Accessor& accessor = gltf_model_.accessors.at(sampler.input);

      accessor.minValues = {min};
      accessor.maxValues = {max};

      sampler.output = WriteData(values_bool, TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, 0);
    }
  }

  void ProcessUVAnimation(std::size_t animation_id) {
    const float flip_v = flip_uv_ ? -1.0f : 1.0f;

    const SUVAnimation& sanimation = sasset_.model.uv_animations.at(animation_id);

    if (sanimation.translation_channels.empty()) return;

    tinygltf::Animation& animation = gltf_model_.animations.emplace_back();

    if (sanimation.extras.IsObject() && !sanimation.extras.IsEmpty()) {
      animation.extras = SValueToGLTFValue(sanimation.extras);
    }

    animation.name = sanimation.name;

    for (const auto& [mat_id, mat_anim] : sanimation.translation_channels) {
      auto& [interpolation, schannel] = mat_anim;

      if (schannel.empty()) continue;

      extensions_used_.insert("KHR_texture_transform");

      extensions_used_.insert("KHR_animation_pointer");

      tinygltf::AnimationChannel& channel = animation.channels.emplace_back();

      std::string target = "/materials/" + std::to_string(mat_id) +
                           "/pbrMetallicRoughness/baseColorTexture/extensions/"
                           "KHR_texture_transform/offset";

      tinygltf::Value::Object khr_pointer = {{"pointer", tinygltf::Value(target)}};

      channel.target_extensions["KHR_animation_pointer"] = tinygltf::Value(khr_pointer);

      channel.target_node = -1;

      channel.target_path = "pointer";

      channel.sampler = animation.samplers.size();

      tinygltf::AnimationSampler& sampler = animation.samplers.emplace_back();

      sampler.interpolation = interpolation == SInterpolationMode::kConstant ? "STEP" : "LINEAR";

      std::vector<float> keys;
      std::vector<Vec2<float>> values;

      keys.reserve(schannel.size());
      values.reserve(schannel.size());

      float min = std::numeric_limits<float>::max();

      float max = std::numeric_limits<float>::lowest();

      for (auto& key : schannel) {
        float time = key.time / 30.0f;

        if (time < min) min = time;

        if (time > max) max = time;

        keys.push_back(time);

        values.push_back({key.value.x, key.value.y * flip_v});
      }

      // A temporary workaround for Blender;
      // Channels with identical keyframes might be discarded
      if (values.size() == 2 && values[0] == values[1]) {
        values[1].x += 0.101f;
        values[1].y += 0.101f;
      }

      sampler.input = WriteData(keys, TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_FLOAT, 0);

      tinygltf::Accessor& accessor = gltf_model_.accessors.at(sampler.input);

      accessor.minValues = {min};
      accessor.maxValues = {max};

      sampler.output = WriteData(values, TINYGLTF_TYPE_VEC2, TINYGLTF_COMPONENT_TYPE_FLOAT, 0);
    }
  }

  void ProcessLight(const SLight& slight) {
    extensions_used_.insert("KHR_lights_punctual");

    gltf_model_.scenes.back().nodes.push_back(gltf_model_.nodes.size());

    tinygltf::Node& light_node = gltf_model_.nodes.emplace_back();

    light_node.light = gltf_model_.lights.size();

    light_node.name = slight.name;

    tinygltf::Light& light = gltf_model_.lights.emplace_back();

    light.name = slight.name;

    auto color = utl::color::SRGBToLinear(slight.color);

    light.color = {color.r, color.g, color.b};

    light.type = "directional";

    glm::mat4 transform{1.0f};

    light.intensity = 10.0f * 683.0f * 0.7f;

    transform = slight.GetTransform();

    transform[3][1] = 300.0f;

    light_node.matrix = GLMMatToGLTF(transform);

    if (slight.extras.IsObject() && !slight.extras.IsEmpty()) {
      light_node.extras = SValueToGLTFValue(slight.extras);
    }
  }

  void ProcessPosition() {
    if (sasset_.positions.empty()) return;

    std::size_t position_root_id = gltf_model_.nodes.size();

    gltf_model_.scenes.back().nodes.push_back(position_root_id);

    tinygltf::Node& position_root = gltf_model_.nodes.emplace_back();

    position_root.name = "positions";

    for (auto& sposition : sasset_.positions) {
      int position_id = gltf_model_.nodes.size();

      tinygltf::Node& position = gltf_model_.nodes.emplace_back();

      position.name = sposition.name;

      position.scale = {sposition.scale.x, sposition.scale.y, sposition.scale.z};

      position.rotation = {sposition.rotation.x, sposition.rotation.y, sposition.rotation.z, sposition.rotation.w};

      position.translation = {sposition.translation.x, sposition.translation.y, sposition.translation.z};

      gltf_model_.nodes.at(position_root_id).children.push_back(position_id);

      if (sposition.extras.IsObject() && !sposition.extras.IsEmpty())
        position.extras = SValueToGLTFValue(sposition.extras);
      else
        position.extras = tinygltf::Value(tinygltf::Value::Object());

      auto& extras = position.extras.Get<tinygltf::Value::Object>();

      extras["id"] = tinygltf::Value((int)sposition.id);
    }
  }

  void ProcessAttachment(const SAttachment& sattach) {
    std::size_t new_node_id = gltf_model_.nodes.size();

    tinygltf::Node& new_node = gltf_model_.nodes.emplace_back();

    new_node.scale = {sattach.scale.x, sattach.scale.y, sattach.scale.z};

    new_node.rotation = {sattach.rotation.x, sattach.rotation.y, sattach.rotation.z, sattach.rotation.w};

    new_node.translation = {sattach.translation.x, sattach.translation.y, sattach.translation.z};

    new_node.name = "external_attachment_" + utl::string::PrependZero(sattach.id);

    new_node.extras = tinygltf::Value(tinygltf::Value::Object{{"id", tinygltf::Value((int)sattach.id)}});

    tinygltf::Node& joint = gltf_model_.nodes.at(bone_joint_id_.at(sattach.bone));

    joint.children.push_back(new_node_id);
  }

  void Export() {
    gltf_model_.asset.generator = "NSUNI/NSLAR library " NNL_GIT_HASH;
    gltf_model_.asset.copyright =
        "This asset may be subject to copyright protection. "
        "Unauthorized use, distribution, or modification of this asset may "
        "violate copyright laws.";

    tinygltf::Scene& scene = gltf_model_.scenes.emplace_back();

    if (sasset_.extras.IsObject() && !sasset_.extras.IsEmpty()) {
      scene.extras = SValueToGLTFValue(sasset_.extras);
    }

    scene.name = "Scene";

    gltf_model_.defaultScene = 0;

    tinygltf::Value::Array variants;

    for (auto& variant : sasset_.model.material_variants) {
      variants.push_back(tinygltf::Value(tinygltf::Value::Object{{"name", tinygltf::Value(variant)}}));
    }

    if (!variants.empty()) {
      extensions_used_.insert("KHR_materials_variants");
      gltf_model_.extensions["KHR_materials_variants"] =
          tinygltf::Value(tinygltf::Value::Object{{"variants", tinygltf::Value(variants)}});
    }

    const bool export_armature =
        !sasset_.animations.empty() || sasset_.model.skeleton.roots.size() > 1 ||
        (!sasset_.model.skeleton.roots.empty() && !sasset_.model.skeleton.roots.at(0).children.empty());

    const bool is_camera = [&]() -> bool {
      const auto& roots = sasset_.model.skeleton.roots;
      return sasset_.model.meshes.size() <= 1 && roots.size() == 1 && roots.at(0).children.size() == 3 &&
             roots.at(0).children.at(0).children.empty() && roots.at(0).children.at(1).children.empty() &&
             roots.at(0).children.at(2).children.empty() && sasset_.textures.empty();
    }();

    if (export_armature && !is_camera) {
      tinygltf::Skin& skin = gltf_model_.skins.emplace_back();

      int armature_node_id = gltf_model_.nodes.size();

      scene.nodes.push_back(armature_node_id);

      tinygltf::Node& armature_node = gltf_model_.nodes.emplace_back();

      armature_node.name = "Armature";

      for (auto& root : sasset_.model.skeleton.roots) {
        int root_id = gltf_model_.nodes.size();

        bone_joint_id_[num_bones_++] = root_id;

        joint_pos_[root_id] = skin.joints.size();

        gltf_model_.nodes.at(armature_node_id).children.push_back(root_id);

        skin.joints.push_back(root_id);

        gltf_model_.nodes.emplace_back();

        ProcessBone(root_id, root);
      }

      skin.inverseBindMatrices = WriteData(inverse_bind_, TINYGLTF_TYPE_MAT4, TINYGLTF_COMPONENT_TYPE_FLOAT, 0);

      for (const SAttachment& sattach : sasset_.model.attachments) {
        ProcessAttachment(sattach);
      }
    }

    if (export_armature && is_camera) {
      bone_joint_id_[num_bones_++] = -1;

      for (auto& cam : sasset_.model.skeleton.roots.at(0).children) {
        bone_joint_id_[num_bones_++] = gltf_model_.nodes.size();

        scene.nodes.push_back(gltf_model_.nodes.size());

        tinygltf::Node& node = gltf_model_.nodes.emplace_back();

        node.name = cam.name;

        if (node.name == "VFX2") {
          node.camera = gltf_model_.cameras.size();

          tinygltf::Camera& cam = gltf_model_.cameras.emplace_back();

          cam.type = "perspective";

          cam.perspective.yfov = glm::radians(30.0);
          cam.perspective.znear = 1.0f;
          cam.perspective.zfar = 1000.0f;
        }
      }
    }

    if (export_armature) {
      for (std::size_t i = 0; i < sasset_.animations.size(); i++) {
        ProcessAnimation(i);
      }
    }

    if (!is_camera) {
      for (std::size_t i = 0; i < sasset_.model.meshes.size(); i++) {
        ProcessMesh(i);
      }

      for (std::size_t i = 0; i < sasset_.model.materials.size(); i++) {
        ProcessMaterial(i);
      }

      if (sasset_.textures.size() < (std::size_t)(max_texture_id_ + 1)) {
        NNL_LOG_WARN("some textures are missing: " + utl::filesys::u8string(path_));
        texture_placeholder_ = STexture::GenerateChessPattern();
        texture_placeholder_.name = "placeholder_";
      }

      for (std::size_t i = 0; i < std::max<std::size_t>(sasset_.textures.size(), (max_texture_id_ + 1)); i++) {
        ProcessTexture(i);
      }

      for (std::size_t i = 0; i < sasset_.visibility_animations.size(); i++) {
        ProcessVisibilityAnimation(i);
      }

      for (std::size_t i = 0; i < sasset_.model.uv_animations.size(); i++) {
        ProcessUVAnimation(i);
      }

      for (auto& slight : sasset_.lights) {
        ProcessLight(slight);
      }

      ProcessPosition();
    }

    gltf_model_.extensionsUsed = std::vector(extensions_used_.begin(), extensions_used_.end());

    if (blob_.Len() != 0) {
      tinygltf::Buffer& buffer = gltf_model_.buffers.emplace_back();

      buffer.data = std::move(blob_);
    }

    if (!path_.empty()) {
      bool success =
          gltf_ctx_.WriteGltfSceneToFile(&gltf_model_, utl::filesys::u8string(path_), false, false, true, true);

      if (!success) NNL_THROW(RuntimeError(NNL_SRCTAG("gltf: exporting failed: " + utl::filesys::u8string(path_))));
    } else if (buffer_ != nullptr) {
      std::ostringstream oss;
      gltf_ctx_.WriteGltfSceneToStream(&gltf_model_, oss, true, true);
      auto str = oss.str();
      buffer_->resize(str.size());
      std::memcpy(buffer_->data(), str.data(), str.size());
    }
  }
};

void SAsset3D::ExportGLB(const std::filesystem::path& path, bool flip, bool pack_textures) const {
  GLTFExporter(*this, path, flip, pack_textures);
}

Buffer SAsset3D::ExportGLB(bool flip) const {
  Buffer buffer;

  GLTFExporter(*this, &buffer, flip, true);

  return buffer;
}

}  // namespace nnl
