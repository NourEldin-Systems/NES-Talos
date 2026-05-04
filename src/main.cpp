/**
 * @file main.cpp
 * @brief Entry point for the NES‑Talos scaffolding CLI.
 *
 * Reads AI‑formatted scaffold data from stdin, parses it, and writes the
 * corresponding file tree to disk. The project root can be given as a
 * command‑line argument; if omitted, the current working directory is used.
 *
 * When stdin is a terminal, a brief instruction is printed to guide the user
 * (paste the data, then press Ctrl+Z + Enter on Windows, or Ctrl+D otherwise).
 *
 * @example
 *   nes-talos my_project < scaffold_data.txt
 *   nes-talos                           # uses current directory + terminal paste
 */

#include "nes-talos/parser.hpp"

#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <memory_resource>

#ifdef _WIN32
#include <io.h>
#define ISATTY _isatty
#else
#include <unistd.h>
#define ISATTY isatty
#endif

int main(int argc, char *argv[])
{
  std::filesystem::path projectRoot;
  if (argc == 1)
  {
    projectRoot = std::filesystem::current_path();
  }
  else if (argc == 2)
  {
    projectRoot = std::filesystem::path(argv[1]);
  }
  else
  {
    std::cerr << "Usage: " << (argc > 0 ? argv[0] : "nes-talos")
              << " [project-root]\n"
              << "  project-root   Target directory (default: current directory)\n";
    return EXIT_FAILURE;
  }

  // If user is pasting manually, show a short help message.
  if (ISATTY(0))
  {
    std::clog << "Paste your scaffold data below, then press ";
#ifdef _WIN32
    std::clog << "Ctrl+Z and Enter";
#else
    std::clog << "Ctrl+D";
#endif
    std::clog << " to finish.\n";
  }

  try
  {
    nes_talos::ScaffoldParser parser(projectRoot);
    std::pmr::string inputLine;
    inputLine.reserve(1024);

    while (std::getline(std::cin, inputLine))
    {
      parser.feedLine(inputLine);
      inputLine.clear();
    }

    parser.finalize();
    return EXIT_SUCCESS;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
}