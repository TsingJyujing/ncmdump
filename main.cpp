#include "ncmcrypt.h"
#include "spdlog/spdlog.h"
#include <stdexcept>
#include "argparse.hpp"
#include <algorithm>
#include <cctype>
#include <string>

const std::vector<std::string> possibleFileExtensions{"mp3", "flac"};

void process_file(const std::string &filename, const bool skip, const bool rm) {
    try {
        if (skip) {
            // Check if .flac/.mp3 already existed.
            for (const auto &fileExtension: possibleFileExtensions) {
                std::filesystem::path convertedPath = filename;
                convertedPath.replace_extension("." + fileExtension);
                if (std::filesystem::exists(convertedPath)) {
                    spdlog::debug("File {} skipped caused by result file already existed.", filename);
                    return;
                }
            }
        }
        NeteaseCrypt crypt(filename);
        crypt.Dump();
        crypt.FixMetadata();
        if (rm) {
            // Remove data after converted successfully
            std::filesystem::remove(filename);
        }
    } catch (std::invalid_argument &e) {
        spdlog::error("Processing file {} which caused by {}", filename, e.what());
    } catch (...) {
        spdlog::error("Unknown exception occurred while processing file {}", filename);
    }
}


int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("ncmdump");

    program.add_argument("command")
            .help("The command you'd like to run: [file|scan]");

    program.add_argument("path")
            .help("Path of the file/dir");

    program.add_argument("--rm")
            .help("Remove NCM file if it's successfully dumped.")
            .default_value(false)
            .implicit_value(true);

    program.add_argument("--skip")
            .help("Skip writing if converted file already existed.")
            .default_value(false)
            .implicit_value(true);

    program.add_argument("--verbose")
            .help("Print detail logs")
            .default_value(false)
            .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        spdlog::error("Error while parsing from CLI: {}", err.what());
        std::cout << program; // Print usage out
        exit(0);
    }

    if (program["--verbose"] == true) {
        spdlog::set_level(spdlog::level::debug);
        spdlog::debug("Using verbose mode.");
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    const bool skip = program["--skip"] == true;
    const bool rm = program["--rm"] == true;
    auto path = program.get<std::string>("path");
    auto command = program.get<std::string>("command");

    if (command == "file") {
        process_file(path, skip, rm);
    } else if (command == "scan") {
        for (auto &p: std::filesystem::recursive_directory_iterator(path)) {
            const auto &filepath = p.path();
            std::string ext = filepath.extension().string();
            std::transform(
                    ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return std::tolower(c); }
            );
            if (ext == ".ncm") {
                spdlog::info("Found NCM file: {}", filepath.string());
                process_file(filepath, skip, rm);
            } else {
                spdlog::debug("Skip file: {}", filepath.string());
            }
        }
    } else {
        spdlog::error("Unknown command {}, please input file/scan.", command);
    }
    return 0;
}