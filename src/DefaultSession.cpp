/**
 * Copyright 2017 The Nakama Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "DefaultSession.h"
#include "nakama-cpp/StrUtil.h"
#include <chrono>

namespace Nakama {

using namespace std;
using namespace std::chrono;

string getJsonField(const string& json, const string& field_name)
{
    string result;

    auto pos = json.find("\"" + field_name + "\"");
    if (pos != string::npos)
    {
        auto pos1 = json.find("\"", pos);
        if (pos1 != string::npos)
        {
            ++pos1;
            auto pos2 = json.find("\"", pos1);
            if (pos2 != string::npos)
            {
                --pos2;
                result = json.substr(pos1, pos2 - pos1);
            }
        }
    }

    return result;
}

DefaultSession::DefaultSession(const std::string & token, bool created)
{
    milliseconds ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch());

    _token = token;
    _created = created;
    _create_time = ms.count();

    // Hack decode JSON payload from JWT
    // first we narrow down to the segment between the first two '.'
    int dotIndex1 = token.find('.');
    int dotIndex2 = token.find('.', dotIndex1 + 1);
    std::string payload = token.substr(dotIndex1 + 1, dotIndex2 - dotIndex1 - 1);

    // the segment is base64 encoded, so decode it...
    std::string json = base64_decode(payload);

    // now we have some json to parse.
    // e.g.: {"exp":1489862293,"uid":"3c01e3ee-878a-4ec4-8923-40d51a86f91f"}
    string exp_str = getJsonField(json, "exp");
    if (!exp_str.empty())
    {
        _expire_time = std::atol(exp_str.c_str()) * 1000L;
    }

    _username = getJsonField(json, "usn");
    _user_id = getJsonField(json, "uid");
}

const std::string & DefaultSession::getAuthToken()
{
    return _token;
}

bool DefaultSession::isCreated()
{
    return _created;
}

const std::string & DefaultSession::getUsername()
{
    return _username;
}

const std::string & DefaultSession::getUserId()
{
    return _user_id;
}

uint64_t DefaultSession::getCreateTime()
{
    return _create_time;
}

uint64_t DefaultSession::getExpireTime()
{
    return _expire_time;
}

bool DefaultSession::isExpired()
{
    milliseconds ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch());

    return isExpired(ms.count());
}

bool DefaultSession::isExpired(uint64_t now)
{
    return now >= _expire_time;
}

NSessionPtr restore(const std::string& token)
{
    return NSessionPtr(new DefaultSession(token, false));
}

}
