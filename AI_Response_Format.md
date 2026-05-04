# AI Response Format – Coding Phase

When you have finished negotiation and planning and are ready to deliver code, your **entire output** must consist of one or more file blocks exactly as described below.  
No introductory text, no explanations, no markdown headings, and nothing after the final block.

## Block Format

```
##FILE <relative-path>
##CONTENT
<file content>
##END
```

- **`##FILE`** starts a new file entry.
- `<relative-path>` is the path relative to the project root (e.g., `src/main.cpp`).
- The `##CONTENT` line must appear immediately after the `##FILE` line.
- Every line between `##CONTENT` and `##END` is the file’s content, written exactly as given.
- **`##END`** marks the end of the file entry. It must appear on its own line.
- Exactly **one blank line** separates the `##END` of one block and the next `##FILE` line.

## Example

```
##FILE src/main.cpp
##CONTENT
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
##END

##FILE include/config.hpp
##CONTENT
#pragma once
constexpr int VERSION = 1;
##END

##FILE README.md
##CONTENT
# My Project
This project is awesome.
##END
```

## Critical Rules

1. **Only file blocks.** If explanation is needed, put it in a `.md` file block.
2. **No trailing text.** The last line must be `##END` of the final block.
3. **No leading empty lines.** Start with the first `##FILE`.
4. **Relative paths only.** No absolute paths or `..`.
5. **Avoid using `##FILE`, `##CONTENT`, or `##END` inside the file content.** If unavoidable, add a comment to break the pattern (e.g., `## END`).
