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

#pragma once

#include "nakama-cpp/NTypes.h"
#include <vector>

namespace Nakama {

    /// An object within the storage engine.
    struct NAKAMA_API NStorageObject
    {
        std::string collection;                ///< The collection which stores the object.
        std::string key;                       ///< The key of the object within the collection.
        std::string user_id;                   ///< The user owner of the object.
        std::string value;                     ///< The value of the object.
        std::string version;                   ///< The version hash of the object.
        int32_t permission_read = 0;           ///< The read access permissions for the object.
        int32_t permission_write = 0;          ///< The write access permissions for the object.
        NTimestamp create_time = 0;            ///< The UNIX time when the object was created.
        NTimestamp update_time = 0;            ///< The UNIX time when the object was last updated.
    };

    using NStorageObjects = std::vector<NStorageObject>;
}
