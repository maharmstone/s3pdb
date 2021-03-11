#include <iostream>
#include <fstream>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <string>
#include <filesystem>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>
#include "pdb.h"

using namespace std;

static const string_view access_key = "AKIAXYIHB2JMQD2MMCVX"; // FIXME
static const string_view secret_key = "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"; // FIXME

class memory_map {
public:
    memory_map(const filesystem::path& fn) { // FIXME - Windows equivalent
        size = file_size(fn);

        auto fd = open(fn.string().c_str(), O_RDONLY);
        if (fd == -1)
            throw formatted_error("open failed (error {})", errno);

        addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (addr == MAP_FAILED) {
            auto err = errno;

            close(fd);

            throw formatted_error("mmap failed (error {})", err);
        }

        close(fd);
    }

    ~memory_map() {
        munmap(addr, size);
    }

    string_view data() const {
        return {(const char*)addr, size};
    }

private:
    void* addr;
    size_t size;
};

static string gzip(const filesystem::path& fn, unsigned int level) {
    int ret;
    z_stream strm;
    string out;

    // FIXME - handle incompressible files

    memory_map m(fn);

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    ret = deflateInit2(&strm, level, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK)
        throw formatted_error("deflateInit failed (error {})", ret);

    try {
        auto sv = m.data();

        out.resize(sv.length());

        strm.avail_in = (unsigned int)sv.length();
        strm.next_in = (uint8_t*)sv.data();
        strm.avail_out = (unsigned int)out.length();
        strm.next_out = (uint8_t*)out.data();

        do {
            ret = deflate(&strm, Z_FINISH);

            if (ret < 0)
                throw formatted_error("deflate failed (error {})", ret);
        } while (strm.avail_in > 0 && strm.avail_out > 0);

        out.resize((char*)strm.next_out - (char*)out.data());
    } catch (...) {
        deflateEnd(&strm);
        throw;
    }

    deflateEnd(&strm);

    return out;
}

static void upload(Aws::S3::S3Client& s3_client, const string_view& bucket,
                   const filesystem::path& filename, string& object_name) {
    // FIXME - option not to compress?

    auto comp = gzip(filename, Z_DEFAULT_COMPRESSION);

    pdb p(filename);

    auto info = p.get_info();

    object_name = fmt::format("{}/{:08X}{:04X}{:04X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:X}/{}",
                             filename.filename().u8string(),
                             *(uint32_t*)&info.guid[0], *(uint16_t*)&info.guid[4], *(uint16_t*)&info.guid[6],
                             (uint8_t)info.guid[8], (uint8_t)info.guid[9], (uint8_t)info.guid[10], (uint8_t)info.guid[11],
                             (uint8_t)info.guid[12], (uint8_t)info.guid[13], (uint8_t)info.guid[14], (uint8_t)info.guid[15],
                             info.age, filename.filename().u8string());

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(Aws::String(bucket));
    request.SetKey(Aws::String(object_name));
    request.SetContentType("application/octet-stream");
    request.SetContentEncoding("gzip");

//     auto input_data = make_shared<fstream>(filename, ios_base::in | ios_base::binary);

    auto input_data = make_shared<stringstream>(stringstream::in | stringstream::out | stringstream::binary);
    input_data->write(comp.data(), comp.length());

    request.SetBody(input_data);

    auto outcome = s3_client.PutObject(request);

    if (!outcome.IsSuccess())
        throw formatted_error("S3 PutObject failed: {}", outcome.GetError().GetMessage());
}

int main (int argc, char* argv[]) {
    bool failed = false;

    if (argc < 2) {
        fmt::print(stderr, "Usage: s3pdb <pdb-file>\n");
        return 1;
    }

    try {
        Aws::SDKOptions options;
        Aws::InitAPI(options);

        string bucket_name = "symbols.burntcomma.com"; // FIXME
        string region = "eu-west-2"; // FIXME

        Aws::Client::ClientConfiguration config;

        config.region = Aws::String(region);

        Aws::Auth::AWSCredentials credentials;

        credentials.SetAWSAccessKeyId(Aws::String(access_key));
        credentials.SetAWSSecretKey(Aws::String(secret_key));

        Aws::S3::S3Client s3_client(credentials, config);

        for (int i = 1; i < argc; i++) {
            bool success = false;
            string path;

            try {
                upload(s3_client, bucket_name, argv[i], path);
                success = true;
            } catch (const exception& e) {
                fmt::print(stderr, "Error uploading {}: {}\n", argv[i], e.what());
                failed = true;
            }

            if (success)
                fmt::print("Uploaded {} as {}.\n", argv[i], path);
        }

        Aws::ShutdownAPI(options);
    } catch (const exception& e) {
        fmt::print(stderr, "{}\n", e.what());
        return 1;
    }

    return failed ? 1 : 0;
}
