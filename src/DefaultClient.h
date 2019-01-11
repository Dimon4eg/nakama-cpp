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

#pragma once

#include "nakama-cpp/ClientInterface.h"
#include "nakama-cpp/DefaultClient.h"
#include "api/github.com/heroiclabs/nakama/apigrpc/apigrpc.grpc.pb.h"
#include <set>

namespace Nakama {

    struct RpcRequest
    {
        grpc::ClientContext context;
        grpc::Status status;
        std::function<void()> successCallback;
        ErrorCallback errorCallback;
    };

    /**
     * A client to interact with Nakama server.
     * Don't use it directly, use `createDefaultClient` instead.
     */
    class DefaultClient : public ClientInterface
    {
    public:
        DefaultClient(const DefaultClientParameters& parameters);
        ~DefaultClient();

        void disconnect() override;

        void tick() override;

        void authenticateDevice(
            const std::string& id,
            const opt::optional<std::string>& username,
            const opt::optional<bool>& create,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void authenticateEmail(
            const std::string& email,
            const std::string& password,
            const std::string& username,
            bool create,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void authenticateFacebook(
            const std::string& accessToken,
            const std::string& username,
            bool create,
            bool importFriends,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void authenticateGoogle(
            const std::string& accessToken,
            const std::string& username,
            bool create,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void authenticateGameCenter(
            const std::string& playerId,
            const std::string& bundleId,
            NTimestamp timestampSeconds,
            const std::string& salt,
            const std::string& signature,
            const std::string& publicKeyUrl,
            const std::string& username,
            bool create,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void authenticateCustom(
            const std::string& id,
            const std::string& username,
            bool create,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void authenticateSteam(
            const std::string& token,
            const std::string& username,
            bool create,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void linkFacebook(
            NSessionPtr session,
            const std::string& accessToken,
            const opt::optional<bool>& importFriends,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void linkEmail(
            NSessionPtr session,
            const std::string& email,
            const std::string& password,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void linkDevice(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void linkGoogle(
            NSessionPtr session,
            const std::string& accessToken,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void linkGameCenter(
            NSessionPtr session,
            const std::string& playerId,
            const std::string& bundleId,
            NTimestamp timestampSeconds,
            const std::string& salt,
            const std::string& signature,
            const std::string& publicKeyUrl,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void linkSteam(
            NSessionPtr session,
            const std::string& token,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void linkCustom(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void unlinkFacebook(
            NSessionPtr session,
            const std::string& accessToken,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void unlinkEmail(
            NSessionPtr session,
            const std::string& email,
            const std::string& password,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void unlinkGoogle(
            NSessionPtr session,
            const std::string& accessToken,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void unlinkGameCenter(
            NSessionPtr session,
            const std::string& playerId,
            const std::string& bundleId,
            NTimestamp timestampSeconds,
            const std::string& salt,
            const std::string& signature,
            const std::string& publicKeyUrl,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void unlinkSteam(
            NSessionPtr session,
            const std::string& token,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void unlinkDevice(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void unlinkCustom(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void importFacebookFriends(
            NSessionPtr session,
            const std::string& token,
            const opt::optional<bool>& reset,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void getAccount(
            NSessionPtr session,
            std::function<void(const NAccount&)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) override;

        void updateAccount(
            NSessionPtr session,
            const opt::optional<std::string>& username,
            const opt::optional<std::string>& displayName,
            const opt::optional<std::string>& avatarUrl,
            const opt::optional<std::string>& langTag,
            const opt::optional<std::string>& location,
            const opt::optional<std::string>& timezone,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void getUsers(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames,
            const std::vector<std::string>& facebookIds,
            std::function<void(const NUsers&)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void addFriends(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void deleteFriends(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void blockFriends(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void listFriends(
            NSessionPtr session,
            std::function<void(NFriendsPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void createGroup(
            NSessionPtr session,
            const std::string& name,
            const std::string& description,
            const std::string& avatarUrl,
            const std::string& langTag,
            bool open,
            std::function<void(const NGroup&)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void deleteGroup(
            NSessionPtr session,
            const std::string& groupId,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void addGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            const std::vector<std::string>& ids,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void listGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            std::function<void(NGroupUserListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void kickGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            const std::vector<std::string>& ids,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void joinGroup(
            NSessionPtr session,
            const std::string& groupId,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void leaveGroup(
            NSessionPtr session,
            const std::string& groupId,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void listGroups(
            NSessionPtr session,
            const std::string& name,
            int limit,
            const std::string& cursor,
            std::function<void(NGroupListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void listUserGroups(
            NSessionPtr session,
            std::function<void(NUserGroupListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void listUserGroups(
            NSessionPtr session,
            const std::string& userId,
            std::function<void(NUserGroupListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void promoteGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            const std::vector<std::string>& ids,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void updateGroup(
            NSessionPtr session,
            const std::string& groupId,
            const opt::optional<std::string>& name,
            const opt::optional<std::string>& description,
            const opt::optional<std::string>& avatarUrl,
            const opt::optional<std::string>& langTag,
            const opt::optional<bool>& open,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void listLeaderboardRecords(
            NSessionPtr session,
            const std::string& leaderboardId,
            const std::vector<std::string>& ownerIds,
            const opt::optional<int>& limit,
            const opt::optional<std::string>& cursor,
            std::function<void(NLeaderboardRecordListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void listLeaderboardRecordsAroundOwner(
            NSessionPtr session,
            const std::string& leaderboardId,
            const std::string& ownerId,
            const opt::optional<int>& limit,
            std::function<void(NLeaderboardRecordListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void writeLeaderboardRecord(
            NSessionPtr session,
            const std::string& leaderboardId,
            int64_t score,
            const opt::optional<int64_t>& subscore,
            const opt::optional<std::string>& metadata,
            std::function<void(NLeaderboardRecord)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void writeTournamentRecord(
            NSessionPtr session,
            const std::string& tournamentId,
            int64_t score,
            const opt::optional<int64_t>& subscore,
            const opt::optional<std::string>& metadata,
            std::function<void(NLeaderboardRecord)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void deleteLeaderboardRecord(
            NSessionPtr session,
            const std::string& leaderboardId,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void listMatches(
            NSessionPtr session,
            const opt::optional<int>& min_size,
            const opt::optional<int>& max_size,
            const opt::optional<int>& limit,
            const opt::optional<std::string>& label,
            const opt::optional<bool>& authoritative,
            std::function<void(NMatchListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void listNotifications(
            NSessionPtr session,
            const opt::optional<int>& limit,
            const opt::optional<std::string>& cacheableCursor,
            std::function<void(NNotificationListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

        void deleteNotifications(
            NSessionPtr session,
            const std::vector<std::string>& notificationIds,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override;

        void listChannelMessages(
            NSessionPtr session,
            const std::string& channelId,
            const opt::optional<int>& limit,
            const opt::optional<std::string>& cursor,
            const opt::optional<bool>& forward,
            std::function<void(NChannelMessageListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override;

    private:
        RpcRequest* createRpcRequest(NSessionPtr session);
        void onResponse(void* tag, bool ok);

    private:
        std::unique_ptr< nakama::api::Nakama::Stub> _stub;
        grpc::CompletionQueue _cq;
        std::string _basicAuthMetadata;
        std::set<RpcRequest*> _requests;
    };
}
