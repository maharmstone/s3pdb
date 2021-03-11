#include <iostream>
#include <fstream>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <string>
#include <filesystem>
#include "pdb.h"

using namespace std;

static const string_view access_key = "AKIAXYIHB2JMQD2MMCVX"; // FIXME
static const string_view secret_key = "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"; // FIXME

static void upload(Aws::S3::S3Client& s3_client, const string_view& bucket,
                   const filesystem::path& filename, string& object_name) {
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

    auto input_data = make_shared<fstream>(filename, ios_base::in | ios_base::binary);

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
