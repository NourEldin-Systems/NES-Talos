/**
 * @file parser.hpp
 * @brief Deterministic finite‑state machine parser for the NES‑Talos scaffold format.
 *
 * The parser processes the AI response format defined in AI_Response_Format.md.
 * It uses a zero‑copy, line‑oriented scanner with a 3‑state machine and a
 * precomputed transition table for branch‑free dispatch.
 *
 * All file content is accumulated in std::pmr::string to enable custom
 * memory resources when needed.
 *
 * @see AI_Response_Format.md
 */

#ifndef NES_TALOS_PARSER_HPP
#define NES_TALOS_PARSER_HPP

#include <filesystem>
#include <string_view>
#include <memory_resource>

namespace nes_talos
{

  /**
   * @brief States of the line‑by‑line parser.
   */
  enum class ParserState : uint8_t
  {
    Idle,          ///< Waiting for a ##FILE directive.
    GotPath,       ///< Saw ##FILE, waiting for ##CONTENT.
    CollectContent ///< After ##CONTENT, accumulating file content until ##END.
  };

  /**
   * @brief Parses AI‑formatted scaffold input and writes files to disk.
   *
   * Usage:
   * @code
   *   ScaffoldParser parser("my_project");
   *   for (std::string line; std::getline(std::cin, line); ) {
   *       parser.feedLine(line);
   *   }
   *   parser.finalize(); // prints summary
   * @endcode
   *
   * The parser validates path sanity (no absolute paths, no “..” traversal)
   * and writes each file atomically using a temporary file + rename.
   *
   * @remarks The state machine is deliberately kept minimal – only three states
   *          and five token types – to ensure fast, predictable execution.
   */
  class ScaffoldParser
  {
  public:
    /**
     * @brief Constructs a parser targeting the given project root.
     *
     * @param projectRoot  Absolute or relative path to the existing directory
     *                     where the generated files will be placed.
     *
     * @throws std::runtime_error if the path does not exist or is not a directory.
     */
    explicit ScaffoldParser(std::filesystem::path projectRoot);

    /// Not copyable.
    ScaffoldParser(const ScaffoldParser &) = delete;
    ScaffoldParser &operator=(const ScaffoldParser &) = delete;

    /**
     * @brief Processes one input line.
     *
     * The line must not contain a trailing newline (as produced by std::getline).
     *
     * @param line  A non‑null‑terminated view of the line.
     *
     * @throws std::runtime_error if a syntax error is detected.
     */
    void feedLine(std::string_view line);

    /**
     * @brief Finalises parsing. Must be called after EOF.
     *
     * Verifies that no block is left open and prints a creation summary.
     *
     * @throws std::runtime_error if the parser is not in Idle state.
     */
    void finalize();

    /**
     * @brief Returns the number of files successfully written.
     *
     * @returns Number of files created during this session.
     */
    int filesCreated() const { return mFilesCreated; }

  private:
    std::filesystem::path mProjectRoot;
    ParserState mState{ParserState::Idle};
    std::pmr::string mCurrentPath;    ///< Relative path of the file being built.
    std::pmr::string mCurrentContent; ///< Accumulated file body.
    int mLineNumber{0};               ///< 1‑based line counter for error messages.
    int mFilesCreated{0};             ///< Success counter.

    /**
     * @brief Token categories recognised by the scanner.
     */
    enum class TokenType : uint8_t
    {
      FileDirective, ///< "##FILE <path>"
      ContentMarker, ///< "##CONTENT"
      EndDirective,  ///< "##END"
      Blank,         ///< Line consisting solely of whitespace.
      Other          ///< Content lines or unexpected input.
    };

    /**
     * @brief Classifies a line into a token type.
     *
     * Leading whitespace is trimmed before classification, so lines like
     * "  ##FILE …" are still recognised. A line consisting entirely of
     * whitespace is mapped to TokenType::Blank.
     *
     * @param line  Raw input line.
     * @returns     The token type.
     */
    static TokenType classifyToken(std::string_view line) noexcept;

    /**
     * @brief Writes the accumulated content to disk and resets internal buffers.
     *
     * @throws std::runtime_error, std::system_error on I/O or path errors.
     */
    void flushCurrentFile();

    /**
     * @brief Writes a single file atomically, creating parent directories as needed.
     *
     * Uses the classic “write to temp, then rename” pattern to guarantee that
     * existing files are never left in a half‑written state.
     *
     * @param relativePath  Path relative to mProjectRoot.
     * @param content       Complete file body.
     *
     * @throws std::runtime_error      If the path is invalid (absolute, contains “..”, etc.).
     * @throws std::system_error       On filesystem errors.
     */
    void writeFileAtomic(const std::filesystem::path &relativePath,
                         const std::pmr::string &content);

    /**
     * @brief Security check: refuses paths that attempt to escape mProjectRoot.
     *
     * @param relativePath  Path to validate.
     *
     * @throws std::runtime_error if the path is absolute, empty, contains “..”,
     *         or has a leading slash.
     */
    void validatePathSanity(const std::filesystem::path &relativePath) const;
  };

} // namespace nes_talos

#endif