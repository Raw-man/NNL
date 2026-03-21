# Contribution Guidelines

Thank you for your interest in contributing to this project!
Please take a look at the guidelines below before you get started.

## Reporting an issue

Before opening a new issue, please make sure it has not already been reported. 
Provide a clear description and, for bugs, include steps to reproduce the behavior, 
ideally with a minimal example.

## Assets and Proprietary Code

Do **not** include any *proprietary* assets in your contributions. This includes:
- **Binary files:** EBOOT.BIN, .PRX modules, game data archives.
- **Copyrighted assets:** Textures, models, sounds.
- **Decompiled code:** Do not copy/paste raw output from decompilers.

## Naming Conventions

The *naming* conventions used in this project are similar to those suggested by the Google C++ Style Guide:

- **Files:** `snake_case.hpp`
- **Classes / Structs:** `PascalCase`
- **Functions:** `PascalCase`
- **Local Variables / Struct Members:** `snake_case`
- **Class Members:** `snake_case_` (with a trailing underscore)
- **Constants/ Enumerators:** `kPascalCase`
- **Namespaces:** `snake_case`
- **Macros:** `ALL_CAPS`

**Exceptions:** `snake_case()` may be used for simple accessors or features that imitate standard C++ library components.

## Code Formatting

To maintain a consistent style, please use the provided `.clang-format` file before submitting any changes. 

Many modern IDEs can be configured to use the project's .clang-format config automatically on save.
You can also apply the style manually via the command line:

`clang-format -style=file:/path/to/the/custom_config_file -i your_file.cpp`

