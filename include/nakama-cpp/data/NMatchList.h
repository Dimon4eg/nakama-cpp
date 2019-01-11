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

#include "nakama-cpp/data/NMatch.h"
#include <vector>
#include <memory>

namespace Nakama {

    // A list of realtime matches.
    struct NAKAMA_API NMatchList
    {
        std::vector<NMatch> matches;   // A number of matches corresponding to a list operation.
    };

    using NMatchListPtr = std::shared_ptr<NMatchList>;
}
