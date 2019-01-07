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

#pragma once

#include <string>

namespace Nakama {

    struct NAKAMA_API NGroup
    {
        std::string id;            // The id of a group.
        std::string creator_id;    // The id of the user who created the group.
        std::string name;          // The unique name of the group.
        std::string description;   // A description for the group.
        std::string lang;          // The language expected to be a tag which follows the BCP-47 spec.
        std::string metadata;      // Additional information stored as a JSON object.
        std::string avatar_url;    // A URL for an avatar image.
        bool open = false;         // Anyone can join open groups, otherwise only admins can accept members.
        int32_t edge_count = 0;    // The current count of all members in the group.
        int32_t max_count = 0;     // The maximum number of members allowed.
        uint64_t create_time = 0;  // The UNIX time when the group was created.
        uint64_t update_time = 0;  // The UNIX time when the group was last updated.
    };

}
