# NES-Talos

NES-Talos: High-speed C++ CLI to scaffold projects from browser-AI output. Atomic file generation with sub-second execution. Zero dependencies.

## Why NES-Talos?
Browser-based LLMs cannot access your local filesystem, forcing developers into a tedious copy-paste workflow. NES-Talos bridges this gap. It reads structured streams from your terminal and executes atomic filesystem writes, allowing you to scaffold complex project structures in milliseconds.

## Features
* **Zero-Dependency:** Written in standard C++17/20. No `cmake`, no `vcpkg`, no bloat.
* **Atomic Writes:** Uses a "write-to-temp-then-rename" pattern to ensure files are never left in a corrupted or half-written state.
* **Sub-Second Performance:** Implements a deterministic finite-state machine with a precomputed transition table for branch-free dispatch.
* **Security:** Strict path sanitization prevents directory traversal attacks.

## Getting Started

### 1. Compile
The project includes a `Makefile`. Simply run:
```bash
make
```

### 2. Instruct the AI
To ensure the AI generates output that NES-Talos can parse, provide it with the AI_Response_Format.md file included in this repository. Use this in your system prompt or as an initial context message.

### 3. Usage
Generate: Ask your AI for a scaffold, ensuring it follows the format defined in AI_Response_Format.md.

Copy: Copy the AI's output to your clipboard.

Execute: Run the tool from your terminal:

Bash
```
./nes-talos

# Or 'talos' if you have aliased the binary
```

Paste: Paste the AI output into the terminal and press Enter.

NES-Talos will immediately process the blocks and scaffold the files into your current directory.
