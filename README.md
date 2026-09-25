# RNT

**RNT (Rename with Texteditor)** is a batch renaming tool that lets you edit file and directory names in a text editor.

Instead of providing its own rename editor, RNT opens the target directory as a text representation in an external editor. Edit the names and hierarchy, save and close the editor, and RNT applies the resulting changes to the filesystem.

> RNT is designed around the idea that you should be free to use the text editor you already like.

## Features

- Batch rename multiple files and directories at once
- Rename files and directories by editing plain text
- Move entries between directories by changing their indentation
- Use external editors such as VS Code or Notepad++
- Detect collisions in the final directory state before applying changes
- Resolve cyclic renames by using temporary paths
- Preserve entry identity with internal `EntryId`s, so reordering lines does not change which file an edit refers to
- Currently focused on Windows, with a codebase intended to remain portable where practical

## How it works

When RNT starts, it scans the target directory recursively and generates a temporary text file.

For example:

```text
\\		"file-a.txt"
:		"file-b.txt"
*		"docs"/
?			"old.txt"
```

The leading characters are the internal entry IDs. They are not filenames and should normally be left untouched.

The quoted text is the entry name. A trailing `/` marks a directory.

To rename `file-a.txt` to `renamed.txt`, simply change:

```text
\\		"file-a.txt"
```

to:

```text
\\		"renamed.txt"
```

The same mechanism can be used to move entries by changing their position in the hierarchy.

### Cyclic renames

RNT can handle cycles such as:

```text
A -> B
B -> C
C -> A
```

These cannot be performed directly with ordinary sequential `rename()` calls because one destination is temporarily occupied. RNT detects the dependency cycle and uses a temporary path so the operations can be completed safely:

```text
C -> temporary
B -> C
A -> B
temporary -> A
```

## Usage

Run RNT without arguments to operate on the current directory:

```text
rnt
```

Or specify a target directory:

```text
rnt <directory>
```

RNT launches the configured editor and waits for it to finish. After the editor closes, RNT parses the edited text, validates the final state, generates filesystem operations, and executes them.

## Editing rules

Keep these rules in mind while editing the generated text:

- Keep the entry ID at the beginning of each line.
- Keep each name inside double quotes.
- Directories are represented by a trailing `/` after the closing quote.
- Do not add or delete entries. The number of entries must remain unchanged.
- Changing an entry from a file to a directory, or vice versa, is not supported.
- Duplicate final paths are rejected before any filesystem operation is executed.

The indentation is used to describe directory hierarchy. The spacing is used to keep the ID and name visually aligned. The hierarchy is represented by indentation, so changing the indentation can change the parent directory.

## Editor configuration

RNT uses a JSON configuration file for the selected text editor.

The default editor definitions are stored in `rnt.cfg.json`:

```json
{
	"editor": [
		{
			"name": "Notepad++",
			"path": "C:/Program Files/Notepad++/notepad++.exe",
			"arg": "-multiInst -nosession -notabbar"
		},
		{
			"name": "VSCode",
			"path": "C:/Users/yy981/AppData/Local/Programs/Microsoft VS Code/Code.exe",
			"arg": "--wait"
		}
	]
}
```

On first launch, RNT asks which editor to use and creates its user configuration file. An editor not included in the default list can be added to `rnt.cfg.json`.

The editor configuration contains:

| Field | Description |
|---|---|
| `name` | Display name shown during setup |
| `path` | Executable path |
| `arg` | Arguments passed before the temporary text file path |

## Build

### Requirements

- C++23 compiler
- CMake 3.30 or newer
- Ninja or another supported CMake generator
- Git

RNT uses SDL3 as a CMake package. The SDL3 source is included as a Git submodule under `external/SDL`, but the top-level `CMakeLists.txt` does not build it automatically.

Because of this, **SDL3 must be configured and installed into `external/install/` before configuring RNT itself.**

### 1. Initialize the SDL3 submodule

If you cloned the repository without submodules, run:

```text
git submodule update --init --recursive
```

### 2. Build and install SDL3

For a Release build:

```text
cmake -S external/SDL -B external/build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=external/install/release
cmake --build external/build/release --target install
```

For a Debug build:

```text
cmake -S external/SDL -B external/build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=external/install/debug
cmake --build external/build/debug --target install
```

The main project selects the corresponding install directory automatically based on `CMAKE_BUILD_TYPE`.

### 3. Configure and build RNT

After SDL3 has been installed, configure the main project:

```text
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The resulting executable is:

```text
build/rnt.exe
```

### One-time build summary

A clean Release build therefore consists of:

```text
git submodule update --init --recursive
cmake -S external/SDL -B external/build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=external/install/release
cmake --build external/build/release --target install
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Project structure

```text
src/
├── core/
│   ├── config.*      Configuration handling
│   ├── dir.h         Recursive directory scanning
│   ├── fs.h          Filesystem operation definitions
│   ├── launcher.h    External editor process launching
│   ├── solver.*      Rename dependency/cycle resolution
│   └── texter.*      Text representation and parsing
├── def/
│   ├── entry.h       Entry / EntryId definitions
│   └── fs.h          Filesystem operation definitions
└── util/
    ├── fs.h          Temporary directory handling
    └── json.*         JSON file I/O
```

The main processing flow is roughly:

```text
Directory scan
    ↓
Generate editable text
    ↓
Open external text editor
    ↓
Parse edited text
    ↓
Validate final paths
    ↓
Build rename/move operations
    ↓
Resolve dependencies and cycles
    ↓
Execute std::filesystem::rename()
```

## Current limitations

RNT is still under active development. The current implementation has several intentional limitations:

- File/directory type changes are not supported.
- Creating or deleting entries is not supported.
- The final path of every entry must remain unique.
- The text format is intended for RNT-generated files and is not a general-purpose rename script format.
- The current process execution and filesystem implementation is centered on `std::filesystem` and SDL3.
- The `useCliArg` setting is currently stored during setup but is not yet used by the main program.
- `rnt.cfg.json` is currently read as a project/working-directory file.
- Cross-platform support is not yet the primary target.

## License

RNT is released under the [MIT License](LICENSE.txt).

Copyright (c) 2026 yy981
