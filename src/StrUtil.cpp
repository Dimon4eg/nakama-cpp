/*
 * Copyright 2019 The Nakama Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nakama-cpp/StrUtil.h"
#include <google/protobuf/stubs/strutil.h>

namespace Nakama {

using namespace std;

std::string base64Encode(const Base64Buffer& buffer)
{
    std::string base64str;

    google::protobuf::Base64Escape((const unsigned char *)buffer.data(), static_cast<int>(buffer.size()), &base64str, true);

    return base64str;
}

Base64Buffer base64Decode(const std::string& base64str)
{
    Base64Buffer buffer;

    google::protobuf::Base64Unescape(base64str, &buffer);

    return buffer;
}

string urlEncode(const string & str)
{
    string result;
    const char* chars = str.c_str();
    char bufHex[10];
    size_t len = str.size();

    for (size_t i = 0; i < len; i++)
    {
        char c = chars[i];
        // uncomment this if you want to encode spaces with +
        /*if (c==' ') new_str += '+';
        else */if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            result += c;
        else
        {
            sprintf(bufHex, "%X", c);

            if ((uint8_t)c < 16)
                result += "%0";
            else
                result += "%";

            result += bufHex;
        }
    }

    return result;
}

bool isStringStartsWith(const string & str, const string & prefix)
{
    bool res = false;

    if (str.size() >= prefix.size())
    {
        res = (str.compare(0, prefix.size(), prefix) == 0);
    }

    return res;
}

} // namespace Nakama
