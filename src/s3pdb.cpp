#include <iostream>
#include <fstream>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <string>
#include <filesystem>

using namespace std;

static const string_view access_key = "AKIAXYIHB2JMQD2MMCVX"; // FIXME
static const string_view secret_key = "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"; // FIXME

static void upload(Aws::S3::S3Client& s3_client, const string_view& bucket, const filesystem::path& filename) {
    if (!exists(filename))
        throw runtime_error(filename.u8string() + " does not exist.");

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(Aws::String(bucket));
    request.SetKey(Aws::String(filename.filename().u8string())); // FIXME - prepend GUID etc.

    auto input_data = make_shared<fstream>(filename, ios_base::in | ios_base::binary);

    request.SetBody(input_data);

    auto outcome = s3_client.PutObject(request);

    if (!outcome.IsSuccess())
        throw runtime_error("S3 PutObject failed: " + string(outcome.GetError().GetMessage()));
}

int main (int argc, char* argv[]) {
    bool failed = false;

    if (argc < 2) {
        fprintf(stderr, "Usage: s3pdb <pdb-file>\n");
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

            try {
                upload(s3_client, bucket_name, argv[i]);
                success = true;
            } catch (const exception& e) {
                fprintf(stderr, "Error uploading %s: %s\n", argv[i], e.what());
                failed = true;
            }

            if (success)
                printf("Uploaded %s.\n", argv[i]);
        }

        Aws::ShutdownAPI(options);
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return 1;
    }

    return failed ? 1 : 0;
}
