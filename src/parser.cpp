/**
 * @file parser.cpp
 * @brief Implementation of the ScaffoldParser state machine.
 *
 * The core of NES‑Talos. It translates AI‑generated block directives into
 * real filesystem operations with atomic writes and path sanitisation.
 */

#include "nes-talos/parser.hpp"

#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <iostream>

namespace nes_talos
{

  // ── Marker constants ─────────────────────────
  constexpr std::string_view FILE_DIRECTIVE = "##FILE ";
  constexpr std::string_view CONTENT_MARKER = "##CONTENT";
  constexpr std::string_view END_DIRECTIVE = "##END";

  /**
   * @brief Checks whether a string view starts with a given prefix.
   *
   * Equivalent to C++20 std::string_view::starts_with, provided for compatibility.
   */
  [[nodiscard]] inline bool startsWith(std::string_view sv, std::string_view prefix) noexcept
  {
    return sv.size() >= prefix.size() && sv.compare(0, prefix.size(), prefix) == 0;
  }

  // ── Token classification ─────────────────────
  ScaffoldParser::TokenType ScaffoldParser::classifyToken(std::string_view line) noexcept
  {
    auto pos = line.find_first_not_of(" \t\r\n");
    if (pos == std::string_view::npos)
    {
      return TokenType::Blank;
    }
    std::string_view trimmed = line.substr(pos);
    if (startsWith(trimmed, FILE_DIRECTIVE))
      return TokenType::FileDirective;
    if (trimmed == CONTENT_MARKER)
      return TokenType::ContentMarker;
    if (trimmed == END_DIRECTIVE)
      return TokenType::EndDirective;
    return TokenType::Other;
  }

  // ── State transition table ───────────────────
  using NextState = int8_t;
  static constexpr NextState kInvalid = -1;

  /**
   * The transition table maps (currentState, tokenType) → nextState.
   * Dimensions: [3 states][5 token types].
   *
   * State indices: 0 = Idle, 1 = GotPath, 2 = CollectContent.
   * Token indices: 0 = FileDirective, 1 = ContentMarker, 2 = EndDirective,
   *                3 = Blank, 4 = Other.
   */
  static constexpr NextState TransitionTable[3][5] = {
      //            FileDir,  ContMrk, EndDir,  Blank,  Other
      /* Idle      */ {1, kInvalid, kInvalid, 0, kInvalid},
      /* GotPath   */ {kInvalid, 2, kInvalid, 1, kInvalid},
      /* Collect   */ {kInvalid, 2, 0, 2, 2}};

  // ── Constructor ──────────────────────────────
  ScaffoldParser::ScaffoldParser(std::filesystem::path projectRoot)
      : mProjectRoot(std::move(projectRoot))
  {
    std::error_code ec;
    if (!std::filesystem::exists(mProjectRoot, ec) || !std::filesystem::is_directory(mProjectRoot, ec))
    {
      throw std::runtime_error("Project root does not exist or is not a directory: " + mProjectRoot.string());
    }
    mCurrentPath.reserve(256);
    mCurrentContent.reserve(4096);
  }

  // ── Main parse loop ──────────────────────────
  void ScaffoldParser::feedLine(std::string_view line)
  {
    ++mLineNumber;

    const TokenType token = classifyToken(line);
    const NextState next = TransitionTable[static_cast<int>(mState)][static_cast<int>(token)];

    if (next == kInvalid)
    {
      std::ostringstream oss;
      oss << "Line " << mLineNumber << ": unexpected token '" << line
          << "' in state " << static_cast<int>(mState);
      throw std::runtime_error(oss.str());
    }

    // ── Critical: flush BEFORE leaving CollectContent so files are written ──
    if (mState == ParserState::CollectContent && token == TokenType::EndDirective)
    {
      flushCurrentFile();
    }

    // ── Capture path when we are about to enter GotPath ──
    if (next == static_cast<int>(ParserState::GotPath) && token == TokenType::FileDirective)
    {
      auto pos = line.find_first_not_of(" \t\r\n");
      std::string_view trimmed = (pos == std::string_view::npos) ? line : line.substr(pos);
      mCurrentPath = trimmed.substr(FILE_DIRECTIVE.size());
    }

    // ── Advance state ──
    mState = static_cast<ParserState>(next);

    // ── State‑specific actions ──
    switch (mState)
    {
    case ParserState::CollectContent:
      if (token == TokenType::ContentMarker)
      {
        // Just entered CollectContent – reset content buffer.
        mCurrentContent.clear();
      }
      else if (token == TokenType::EndDirective)
      {
        // Handled before the state change; nothing extra needed.
      }
      else
      {
        // Append line to content, restoring the newline that getline stripped.
        mCurrentContent.append(line);
        mCurrentContent.append("\n");
      }
      break;
    default:
      break;
    }
  }

  // ── Finalisation ─────────────────────────────
  void ScaffoldParser::finalize()
  {
    if (mState != ParserState::Idle)
    {
      throw std::runtime_error("Unexpected end of input: parser is still inside a file block");
    }
    std::clog << "Done. " << mFilesCreated << " file"
              << (mFilesCreated != 1 ? "s" : "") << " generated.\n";
  }

  // ── Internal helpers ─────────────────────────
  void ScaffoldParser::flushCurrentFile()
  {
    if (mCurrentPath.empty())
    {
      throw std::runtime_error("Internal error: flushCurrentFile called with empty path");
    }
    writeFileAtomic(mCurrentPath, mCurrentContent);
  }

  void ScaffoldParser::writeFileAtomic(const std::filesystem::path &relativePath,
                                       const std::pmr::string &content)
  {
    validatePathSanity(relativePath);

    const std::filesystem::path fullPath = mProjectRoot / relativePath;
    const std::filesystem::path parentPath = fullPath.parent_path();

    std::error_code ec;
    std::filesystem::create_directories(parentPath, ec);
    if (ec)
    {
      throw std::system_error(ec, "Failed to create directory: " + parentPath.string());
    }

    std::filesystem::path tempPath = fullPath;
    tempPath += ".tmp";

    // Write to temporary file.
    {
      std::ofstream tempFile(tempPath, std::ios::binary | std::ios::trunc);
      if (!tempFile)
      {
        throw std::system_error(errno, std::generic_category(), "Cannot open temp file: " + tempPath.string());
      }
      tempFile.write(content.data(), content.size());
      if (!tempFile)
      {
        tempFile.close();
        std::filesystem::remove(tempPath, ec); // best effort cleanup
        throw std::system_error(errno, std::generic_category(), "Write failed to temp file");
      }
      tempFile.close();
    }

    // Atomic rename.
    std::filesystem::rename(tempPath, fullPath, ec);
    if (ec)
    {
      std::filesystem::remove(tempPath, ec); // cleanup stray temp file
      throw std::system_error(ec, "Failed to rename temp file to " + fullPath.string());
    }

    // Report creation.
    std::clog << "  created: " << relativePath.string() << " (" << content.size() << " bytes)\n";
    ++mFilesCreated;
  }

  void ScaffoldParser::validatePathSanity(const std::filesystem::path &relativePath) const
  {
    if (relativePath.empty())
    {
      throw std::runtime_error("Empty file path not allowed");
    }
    if (relativePath.is_absolute())
    {
      throw std::runtime_error("Absolute paths are not allowed: " + relativePath.string());
    }
    for (const auto &part : relativePath)
    {
      if (part == "..")
      {
        throw std::runtime_error("Path traversal ('..') detected: " + relativePath.string());
      }
    }
    if (relativePath.string().front() == '/')
    {
      throw std::runtime_error("Path must be relative without leading slash: " + relativePath.string());
    }
  }

}