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

#include "nakama-cpp/data/NUser.h"

namespace Nakama {

    struct NAKAMA_API NGroupUser
    {
        // The group role status.
        enum class State {
            SUPERADMIN   = 0,   // The user is a superadmin with full control of the group.
            ADMIN        = 1,   // The user is an admin with additional privileges.
            MEMBER       = 2,   // The user is a regular member.
            JOIN_REQUEST = 3    // The user has requested to join the group
        };

        NUser user;            // User.
        State state;           // Their relationship to the group.
    };

}
