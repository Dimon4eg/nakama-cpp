/*
* Copyright 2018 The Nakama Authors
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

#include "DataHelper.h"

using namespace std;

namespace Nakama {

void assign(uint64_t& time, const google::protobuf::Timestamp& data)
{
    time = data.seconds() * 1000;
}

void assign(NAccount& account, const nakama::api::Account& data)
{
    assign(account.user.id, data.user().id());
    assign(account.custom_id, data.custom_id());
    assign(account.email, data.email());
    assign(account.verify_time, data.verify_time());
    assign(account.wallet, data.wallet());
    assign(account.user, data.user());
    assign(account.devices, data.devices());
}

void assign(NUser& user, const nakama::api::User& data)
{
    assign(user.id, data.id());
    assign(user.username, data.username());
    assign(user.displayName, data.display_name());
    assign(user.avatarUrl, data.avatar_url());
    assign(user.lang, data.lang_tag());
    assign(user.location, data.location());
    assign(user.timeZone, data.timezone());
    assign(user.metadata, data.metadata());
    assign(user.facebookId, data.facebook_id());
    assign(user.googleId, data.google_id());
    assign(user.gameCenterId, data.gamecenter_id());
    assign(user.steamId, data.steam_id());
    assign(user.online, data.online());
    assign(user.edge_count, data.edge_count());
    assign(user.createdAt, data.create_time());
    assign(user.updatedAt, data.update_time());
}

void assign(NAccountDevice & device, const nakama::api::AccountDevice & data)
{
    assign(device.id, data.id());
}

}
