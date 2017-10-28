#include "md5.h"
#include <openssl/md5.h>

static char NumberToHex(unsigned char val) {
    if (val < 10)
        return char('0' + val);
    else
        return char('a' + val - 10);
}

std::string CalcMD5(const std::string &data) {
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, data.c_str(), data.size());
    unsigned char digest[MD5_DIGEST_LENGTH] = {};
    MD5_Final(digest, &ctx);
    std::string res;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        res += NumberToHex((digest[i] >> 4) & 0xF);
        res += NumberToHex(digest[i] & 0xF);
    }
    return res;
}

