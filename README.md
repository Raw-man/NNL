# NNL: A library for modding NSUNI and NSLAR

## Description

NNL is a C++ library dedicated to the modding of two PlayStation Portable titles:

*   **Naruto Shippuden: Ultimate Ninja Impact**
*   **Naruto Shippuden: Legends: Akatsuki Rising**

## Features 

**NNL provides an API for:**
-   **Archive Management**:
Unpack and repack game archives.
-   **Asset Processing**:
Parse and manipulate game-native formats, such as 3D models and textures.
- **Format Conversion**:
Export assets to and import from standard exchange formats, such as **glTF** and **PNG**.

## Integration (CMake)

Add the following to your CMakeLists.txt:

```CMake
include(FetchContent)

# 1. Configure the library
set(NNL_ENABLE_LOGGING ON)

FetchContent_Declare(
    NNL
    GIT_REPOSITORY https://github.com/Raw-man/NNL.git
    GIT_TAG        main
)

FetchContent_MakeAvailable(NNL)

# 2. Link against the alias target
target_link_libraries(${PROJECT_NAME} PUBLIC NNL::NNL)
```

## Building

Ensure you have the following installed:

- Git
- Ninja or Make
- C++17 compiler (GCC 13+, Clang 16+, or MSVC 19.28+)
- CMake 3.20+
- Doxygen 1.14+ (optional, for documentation) 

Run the following commands:

```bash
git clone https://github.com/Raw-man/NNL.git
cd NNL
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```
Once built, you can run the tests. Note: you must provide the main asset archive from NSLAR (ULUS10447):

```bash
cd build
ctest --output-on-failure
```
## Documentation

To generate the local HTML API reference, run the custom documentation target after the build step:

```bash
cmake --build build --target doc
```

## Usage
This example shows how to extract Naruto's asset from the game archives and convert the data
into a standard GLB file.

```C++
#include <iostream>
#include "NNL/nnl.hpp"

int main() {
   using namespace nnl;

  // Initialize the path to the main data archive from NSLAR (ULUS10447).
  // Note: Archives from NSUNI require decryption before processing;
  // see https://gbatemp.net/threads/pgd-encrypter.432029/ 

  std::filesystem::path cfc_dig_path = std::filesystem::u8path("0000.BIN");

  if(!std::filesystem::exists(cfc_dig_path)){
     std::cerr << "Error: The file does not exist: " << std::filesystem::absolute(cfc_dig_path) << std::endl;
     return 1;
  }

  // Validate file type. The format namespace lists all currently known asset formats.
  // dig::IsOfType() can also be used.
  if (format::Detect(cfc_dig_path) != format::kDig)
  { 
      std::cerr << "Error: Invalid file format" << std::endl;
      return 1;
  }

  // Parse the main archive structure. cfc is an std::vector of raw binary archive entries.
  dig::Dig cfc = dig::Import(cfc_dig_path);

  // For this example, let's access the archive that stores Naruto.
  const std::size_t naruto_archive_index = 8;

  const dig::FileRecord& naruto_rec = cfc.at(naruto_archive_index);

  // Extract and decompress the record. Note: see FileRecord::is_compressed
  Buffer decomp_buf = dig::Decompress(naruto_rec.buffer, naruto_rec.decompressed_size);

  // The entries that the main archive stores are nested archives as well.

  dig_entry::DigEntry cfc_ent = dig_entry::Import(decomp_buf);

  // Access the entry that stores the 3D model.
  const std::size_t naruto_3d_asset_index = 0;

  Buffer& naruto_bin_3d_asset = cfc_ent.at(naruto_3d_asset_index);

  // That entry is a container that stores closely related parts of a complete asset
  asset::Asset ast_container = asset::Import(naruto_bin_3d_asset);

  // Extract individual components
  Buffer& bin_nrt_mdl = ast_container.at(asset::Asset3D::kModel);
  Buffer& bin_nrt_tex = ast_container.at(asset::Asset3D::kTextureContainer);
  Buffer& bin_nrt_anim = ast_container.at(asset::Asset3D::kAnimationContainer);
  Buffer& bin_nrt_vis_anim = ast_container.at(asset::Asset3D::kVisanimationContainer);
  Buffer& bin_nrt_action_conf = ast_container.at(asset::Asset3D::kActionConfig);

  model::Model nrt_mdl = model::Import(bin_nrt_mdl);
  texture::TextureContainer nrt_tex = texture::Import(bin_nrt_tex);
  animation::AnimationContainer nrt_anim = animation::Import(bin_nrt_anim);
  visanimation::AnimationContainer nrt_vis_anim = visanimation::Import(bin_nrt_vis_anim);
  action::ActionConfig nrt_action_conf = action::Import(bin_nrt_action_conf);

  // The reverse operation:
  // auto bin_tex_re = texture::Export(nrt_tex);

  // Transform game-specific assets into a simplified representation for easier manipulation.
  SAsset3D simple_ast;

  simple_ast.model = model::Convert(nrt_mdl);
  simple_ast.textures = texture::Convert(nrt_tex);
  simple_ast.animations = animation::Convert(nrt_anim);
  simple_ast.visibility_animations = visanimation::Convert(nrt_vis_anim);

  // The reverse operation:
  // auto params = texture::GenerateConvertParam(nrt_tex);
  // auto tex = texture::Convert(std::move(simple_ast.textures), params);

  auto names = action::GetAnimationNames(nrt_action_conf);

  for (std::size_t i = 0; i < simple_ast.animations.size(); i++) {
    auto& sanim = simple_ast.animations.at(i);

    if (!names[i].empty()) sanim.name += "_" + *names[i].begin();
  }
  
  // Save the asset to a standard format (GLB).
  std::filesystem::path out_path = std::filesystem::u8path("naruto.glb");

  simple_ast.ExportGLB(out_path);

  // The reverse operation:
  // SAsset3D simple_ast_re = SAsset3D::Import(out_path);

  return 0;
}
```
