/* Copyright (c) Mark Harmstone 2021
 *
 * This file is part of s3pdb.
 *
 * s3pdb is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public Licence as published by
 * the Free Software Foundation, either version 2 of the Licence, or
 * (at your option) any later version.
 *
 * s3pdb is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public Licence
 * along with s3pdb. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <fmt/format.h>

#define PDB_MAGIC "Microsoft C/C++ MSF 7.00\r\n\x1a\x44\x53\0\0"

struct pdb_superblock {
    char magic[sizeof(PDB_MAGIC)];
    uint32_t block_size;
    uint32_t free_block_map;
    uint32_t num_blocks;
    uint32_t num_directory_bytes;
    uint32_t unknown;
    uint32_t block_map_addr;
};

struct stream {
    uint32_t size = 0;
    std::vector<uint32_t> addresses;
};

enum class pdb_stream_version : uint32_t {
    pdb_stream_version_vc2 = 19941610,
    pdb_stream_version_vc4 = 19950623,
    pdb_stream_version_vc41 = 19950814,
    pdb_stream_version_vc50 = 19960307,
    pdb_stream_version_vc98 = 19970604,
    pdb_stream_version_vc70dep = 19990604,
    pdb_stream_version_vc70 = 20000404,
    pdb_stream_version_vc80 = 20030901,
    pdb_stream_version_vc110 = 20091201,
    pdb_stream_version_vc140 = 20140508,
};

struct pdb_info_header {
    enum pdb_stream_version version;
    uint32_t signature;
    uint32_t age;
    uint8_t guid[16];
};

struct pdb_info {
    uint8_t guid[16];
    uint32_t age;
};

class pdb {
public:
    pdb(const std::filesystem::path& fn);
    pdb_info get_info();

private:
    std::string read_block(uint32_t addr);
    std::string read_stream(unsigned int num);

    pdb_superblock super;
    std::ifstream f;
    std::vector<stream> stream_list;
};

class formatted_error : public std::exception {
public:
    template<typename T, typename... Args>
    formatted_error(const T& s, Args&&... args) {
        msg = fmt::format(s, std::forward<Args>(args)...);
    }

    const char* what() const noexcept {
        return msg.c_str();
    }

private:
    std::string msg;
};
