/**
 * @file nnl.hpp
 * @brief An all-in-one header for the library.
 *
 */
#pragma once

#include "NNL/common/enum_flag.hpp"                         // IWYU pragma: export
#include "NNL/common/exception.hpp"                         // IWYU pragma: export
#include "NNL/common/fixed_type.hpp"                        // IWYU pragma: export
#include "NNL/common/io.hpp"                                // IWYU pragma: export
#include "NNL/common/logger.hpp"                            // IWYU pragma: export
#include "NNL/common/panic.hpp"                             // IWYU pragma: export
#include "NNL/common/src_info.hpp"                          // IWYU pragma: export
#include "NNL/game_asset/audio/adpcm.hpp"                   // IWYU pragma: export
#include "NNL/game_asset/audio/phd.hpp"                     // IWYU pragma: export
#include "NNL/game_asset/behavior/action.hpp"               // IWYU pragma: export
#include "NNL/game_asset/container/asset.hpp"               // IWYU pragma: export
#include "NNL/game_asset/container/collection.hpp"          // IWYU pragma: export
#include "NNL/game_asset/container/dig.hpp"                 // IWYU pragma: export
#include "NNL/game_asset/container/dig_entry.hpp"           // IWYU pragma: export
#include "NNL/game_asset/format.hpp"                        // IWYU pragma: export
#include "NNL/game_asset/interaction/colbox.hpp"            // IWYU pragma: export
#include "NNL/game_asset/interaction/collision.hpp"         // IWYU pragma: export
#include "NNL/game_asset/interaction/shadow_collision.hpp"  // IWYU pragma: export
#include "NNL/game_asset/visual/animation.hpp"              // IWYU pragma: export
#include "NNL/game_asset/visual/fog.hpp"                    // IWYU pragma: export
#include "NNL/game_asset/visual/lit.hpp"                    // IWYU pragma: export
#include "NNL/game_asset/visual/minimap.hpp"                // IWYU pragma: export
#include "NNL/game_asset/visual/model.hpp"                  // IWYU pragma: export
#include "NNL/game_asset/visual/render.hpp"                 // IWYU pragma: export
#include "NNL/game_asset/visual/text.hpp"                   // IWYU pragma: export
#include "NNL/game_asset/visual/texture.hpp"                // IWYU pragma: export
#include "NNL/game_asset/visual/visanimation.hpp"           // IWYU pragma: export
#include "NNL/game_asset/world/posd.hpp"                    // IWYU pragma: export
#include "NNL/simple_asset/sanimation.hpp"                  // IWYU pragma: export
#include "NNL/simple_asset/sasset3d.hpp"                    // IWYU pragma: export
#include "NNL/simple_asset/saudio.hpp"                      // IWYU pragma: export
#include "NNL/simple_asset/smodel.hpp"                      // IWYU pragma: export
#include "NNL/simple_asset/stexture.hpp"                    // IWYU pragma: export
#include "NNL/simple_asset/svalue.hpp"                      // IWYU pragma: export
#include "NNL/utility/array3d.hpp"                          // IWYU pragma: export
#include "NNL/utility/color.hpp"                            // IWYU pragma: export
#include "NNL/utility/data.hpp"                             // IWYU pragma: export
#include "NNL/utility/filesys.hpp"                          // IWYU pragma: export
#include "NNL/utility/math.hpp"                             // IWYU pragma: export
#include "NNL/utility/static_set.hpp"                       // IWYU pragma: export
#include "NNL/utility/static_vector.hpp"                    // IWYU pragma: export
#include "NNL/utility/string.hpp"                           // IWYU pragma: export
#include "NNL/utility/trait.hpp"                            // IWYU pragma: export
#include "NNL/utility/utf8.hpp"                             // IWYU pragma: export

#ifdef __DOXYGEN__

/**
 * \defgroup Utils Utilities
 */

/**
 * \defgroup Common Common
 */

/**
 * \defgroup SimpleAssets Simple Assets
 *
 * A set of data structures for representing
 * and managing 3D models, audio, and textures. Those structures serve as a bridge
 * between common exchange formats and internal engine-specific data.
 */

/**
 * \defgroup GameAssets Game Assets
 */

/**
 * \defgroup Containers Containers
 * @ingroup GameAssets
 *
 * @brief Provides functions and structures for managing various game data
 * containers.
 */

/**
 * \defgroup Visual Visual
 * @ingroup GameAssets
 *
 * @brief Provides functions and structures for managing visual elements,
 * including models and textures.
 */

/**
 * \defgroup Interaction Interaction
 * @ingroup GameAssets
 *
 * @brief Provides functions and structures for managing how assets respond to
 * player input or collisions.
 */

/**
 * \defgroup World World
 * @ingroup GameAssets
 *
 * @brief Provides functions and structures for managing environment layout,
 * and object placement.
 */

/**
 * \defgroup Behavior Behavior
 * @ingroup GameAssets
 *
 * @brief Provides functions and structures for managing game logic, rules, and
 * AI scripts.
 */

/**
 * \defgroup Audio Audio
 * @ingroup GameAssets
 *
 * @brief Provides functions and structures for managing sound banks, audio
 * encoding.
 */

#endif
