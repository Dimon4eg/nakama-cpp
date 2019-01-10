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

#include "DefaultClient.h"
#include "nakama-cpp/StrUtil.h"
#include "DefaultSession.h"
#include "DataHelper.h"
#include <grpc++/create_channel.h>
#include <sstream>

using namespace std;

namespace Nakama {

ClientPtr createDefaultClient(const DefaultClientParameters& parameters)
{
    ClientPtr client(new DefaultClient(parameters));
    return client;
}

DefaultClient::DefaultClient(const DefaultClientParameters& parameters)
{
    std::string target = parameters.host + ":" + std::to_string(parameters.port);

    auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

    _stub = nakama::api::Nakama::NewStub(channel);

    _basicAuthMetadata = "Basic " + base64_encode(parameters.serverKey + ":");
}

DefaultClient::~DefaultClient()
{
    disconnect();
}

void DefaultClient::disconnect()
{
    _cq.Shutdown();
}

void DefaultClient::tick()
{
    bool ok;
    void* tag;
    bool continueLoop = true;
    gpr_timespec timespec;

    timespec.clock_type = GPR_TIMESPAN;
    timespec.tv_sec = 0;
    timespec.tv_nsec = 0;

    do {
        switch (_cq.AsyncNext(&tag, &ok, timespec))
        {
        case grpc::CompletionQueue::SHUTDOWN:
            std::cerr << "The completion queue unexpectedly shutdown." << std::endl;
            continueLoop = false;
            break;

        case grpc::CompletionQueue::GOT_EVENT:
            std::cout << "call completed " << (int)(tag) << " ok: " << ok << std::endl;
            onResponse(tag, ok);
            break;

        case grpc::CompletionQueue::TIMEOUT:
            continueLoop = false;
            break;
        }
    } while (continueLoop);
}

RpcRequest * DefaultClient::createRpcRequest(NSessionPtr session)
{
    RpcRequest* rpcRequest = new RpcRequest();

    if (session)
    {
        rpcRequest->context.AddMetadata("authorization", "Bearer " + session->getAuthToken());
    }
    else
    {
        rpcRequest->context.AddMetadata("authorization", _basicAuthMetadata);
    }

    _requests.emplace(rpcRequest);
    return rpcRequest;
}

void DefaultClient::onResponse(void * tag, bool ok)
{
    auto it = _requests.find((RpcRequest*)tag);

    if (it != _requests.end())
    {
        RpcRequest* reqStatus = *it;

        if (ok)
        {
            if (reqStatus->status.ok())
            {
                if (reqStatus->successCallback)
                {
                    reqStatus->successCallback();
                }
            }
            else if (reqStatus->errorCallback)
            {
                std::stringstream ss;

                ss << "grpc call failed" << std::endl;
                ss << "code: "    << reqStatus->status.error_code() << std::endl;
                ss << "message: " << reqStatus->status.error_message() << std::endl;
                ss << "details: " << reqStatus->status.error_details();

                reqStatus->errorCallback(NError(
                    ss.str(),
                    ErrorCode::GrpcCallFailed
                ));
            }
        }
        else if (reqStatus->errorCallback)
        {
            reqStatus->errorCallback(NError(
                "grpc call failed",
                ErrorCode::GrpcCallFailed
            ));
        }

        delete reqStatus;
        _requests.erase(it);
    }
    else
    {
        std::cout << "DefaultClient::onResponse: not found tag " << tag << std::endl;
    }
}

void DefaultClient::authenticateDevice(
    const std::string& id,
    const opt::optional<std::string>& username,
    const opt::optional<bool>& create,
    std::function<void(NSessionPtr)> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(nullptr);
    auto sessionData(make_shared<nakama::api::Session>());

    if (successCallback)
    {
        rpcRequest->successCallback = [sessionData, successCallback]()
        {
            NSessionPtr session(new DefaultSession(sessionData->token(), sessionData->created()));
            successCallback(session);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::AuthenticateDeviceRequest req;

    req.mutable_account()->set_id(id);

    if (username)
        req.set_username(*username);

    if (create)
        req.mutable_create()->set_value(*create);

    auto responseReader = _stub->AsyncAuthenticateDevice(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*sessionData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::authenticateEmail(
    const std::string & email,
    const std::string & password,
    const std::string & username,
    bool create,
    std::function<void(NSessionPtr)> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(nullptr);
    auto sessionData(make_shared<nakama::api::Session>());

    if (successCallback)
    {
        rpcRequest->successCallback = [sessionData, successCallback]()
        {
            NSessionPtr session(new DefaultSession(sessionData->token(), sessionData->created()));
            successCallback(session);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::AuthenticateEmailRequest req;

    if (!email.empty())
        req.mutable_account()->set_email(email);

    req.mutable_account()->set_password(password);

    if (!username.empty())
        req.set_username(username);

    req.mutable_create()->set_value(create);

    auto responseReader = _stub->AsyncAuthenticateEmail(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*sessionData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::authenticateFacebook(
    const std::string & accessToken,
    const std::string & username,
    bool create,
    bool importFriends,
    std::function<void(NSessionPtr)> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(nullptr);
    auto sessionData(make_shared<nakama::api::Session>());

    if (successCallback)
    {
        rpcRequest->successCallback = [sessionData, successCallback]()
        {
            NSessionPtr session(new DefaultSession(sessionData->token(), sessionData->created()));
            successCallback(session);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::AuthenticateFacebookRequest req;

    req.mutable_account()->set_token(accessToken);

    if (!username.empty())
        req.set_username(username);

    req.mutable_create()->set_value(create);
    req.mutable_sync()->set_value(importFriends);

    auto responseReader = _stub->AsyncAuthenticateFacebook(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*sessionData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::authenticateGoogle(
    const std::string & accessToken,
    const std::string & username,
    bool create,
    std::function<void(NSessionPtr)> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(nullptr);
    auto sessionData(make_shared<nakama::api::Session>());

    if (successCallback)
    {
        rpcRequest->successCallback = [sessionData, successCallback]()
        {
            NSessionPtr session(new DefaultSession(sessionData->token(), sessionData->created()));
            successCallback(session);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::AuthenticateGoogleRequest req;

    req.mutable_account()->set_token(accessToken);

    if (!username.empty())
        req.set_username(username);

    req.mutable_create()->set_value(create);

    auto responseReader = _stub->AsyncAuthenticateGoogle(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*sessionData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::authenticateGameCenter(
    const std::string & playerId,
    const std::string & bundleId,
    uint64_t timestampSeconds,
    const std::string & salt,
    const std::string & signature,
    const std::string & publicKeyUrl,
    const std::string & username,
    bool create,
    std::function<void(NSessionPtr)> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(nullptr);
    auto sessionData(make_shared<nakama::api::Session>());

    if (successCallback)
    {
        rpcRequest->successCallback = [sessionData, successCallback]()
        {
            NSessionPtr session(new DefaultSession(sessionData->token(), sessionData->created()));
            successCallback(session);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::AuthenticateGameCenterRequest req;

    req.mutable_account()->set_player_id(playerId);
    req.mutable_account()->set_bundle_id(bundleId);
    req.mutable_account()->set_timestamp_seconds(timestampSeconds);
    req.mutable_account()->set_salt(salt);
    req.mutable_account()->set_signature(signature);
    req.mutable_account()->set_public_key_url(publicKeyUrl);

    if (!username.empty())
        req.set_username(username);

    req.mutable_create()->set_value(create);

    auto responseReader = _stub->AsyncAuthenticateGameCenter(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*sessionData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::authenticateCustom(
    const std::string & id,
    const std::string & username,
    bool create,
    std::function<void(NSessionPtr)> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(nullptr);
    auto sessionData(make_shared<nakama::api::Session>());

    if (successCallback)
    {
        rpcRequest->successCallback = [sessionData, successCallback]()
        {
            NSessionPtr session(new DefaultSession(sessionData->token(), sessionData->created()));
            successCallback(session);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::AuthenticateCustomRequest req;

    req.mutable_account()->set_id(id);

    if (!username.empty())
        req.set_username(username);

    req.mutable_create()->set_value(create);

    auto responseReader = _stub->AsyncAuthenticateCustom(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*sessionData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::authenticateSteam(
    const std::string & token,
    const std::string & username,
    bool create,
    std::function<void(NSessionPtr)> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(nullptr);
    auto sessionData(make_shared<nakama::api::Session>());

    if (successCallback)
    {
        rpcRequest->successCallback = [sessionData, successCallback]()
        {
            NSessionPtr session(new DefaultSession(sessionData->token(), sessionData->created()));
            successCallback(session);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::AuthenticateSteamRequest req;

    req.mutable_account()->set_token(token);

    if (!username.empty())
        req.set_username(username);

    req.mutable_create()->set_value(create);

    auto responseReader = _stub->AsyncAuthenticateSteam(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*sessionData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::getAccount(
    NSessionPtr session,
    std::function<void(const NAccount&)> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(session);
    auto accoutData(make_shared<nakama::api::Account>());

    if (successCallback)
    {
        rpcRequest->successCallback = [accoutData, successCallback]()
        {
            NAccount account;
            assign(account, *accoutData);
            successCallback(account);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    auto responseReader = _stub->AsyncGetAccount(&rpcRequest->context, {}, &_cq);

    responseReader->Finish(&(*accoutData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::getUsers(
    NSessionPtr session,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& usernames,
    const std::vector<std::string>& facebookIds,
    std::function<void(const NUsers&)> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(session);
    auto usersData(make_shared<nakama::api::Users>());

    if (successCallback)
    {
        rpcRequest->successCallback = [usersData, successCallback]()
        {
            NUsers users;
            assign(users, *usersData);
            successCallback(users);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::GetUsersRequest req;

    for (auto& id : ids)
    {
        req.mutable_ids()->Add()->assign(id);
    }

    for (auto& username : usernames)
    {
        req.mutable_usernames()->Add()->assign(username);
    }

    for (auto& facebookId : facebookIds)
    {
        req.mutable_facebook_ids()->Add()->assign(facebookId);
    }

    auto responseReader = _stub->AsyncGetUsers(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*usersData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::addFriends(
    NSessionPtr session,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& usernames,
    std::function<void()> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(session);

    rpcRequest->successCallback = successCallback;
    rpcRequest->errorCallback = errorCallback;

    nakama::api::AddFriendsRequest req;

    for (auto& id : ids)
    {
        req.mutable_ids()->Add()->assign(id);
    }

    for (auto& username : usernames)
    {
        req.mutable_usernames()->Add()->assign(username);
    }

    auto responseReader = _stub->AsyncAddFriends(&rpcRequest->context, req, &_cq);

    responseReader->Finish(nullptr, &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::deleteFriends(
    NSessionPtr session,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& usernames,
    std::function<void()> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(session);

    rpcRequest->successCallback = successCallback;
    rpcRequest->errorCallback = errorCallback;

    nakama::api::DeleteFriendsRequest req;

    for (auto& id : ids)
    {
        req.mutable_ids()->Add()->assign(id);
    }

    for (auto& username : usernames)
    {
        req.mutable_usernames()->Add()->assign(username);
    }

    auto responseReader = _stub->AsyncDeleteFriends(&rpcRequest->context, req, &_cq);

    responseReader->Finish(nullptr, &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::blockFriends(
    NSessionPtr session,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& usernames,
    std::function<void()> successCallback,
    ErrorCallback errorCallback)
{
    RpcRequest* rpcRequest = createRpcRequest(session);

    rpcRequest->successCallback = successCallback;
    rpcRequest->errorCallback = errorCallback;

    nakama::api::BlockFriendsRequest req;

    for (auto& id : ids)
    {
        req.mutable_ids()->Add()->assign(id);
    }

    for (auto& username : usernames)
    {
        req.mutable_usernames()->Add()->assign(username);
    }

    auto responseReader = _stub->AsyncBlockFriends(&rpcRequest->context, req, &_cq);

    responseReader->Finish(nullptr, &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::listFriends(NSessionPtr session, std::function<void(NFriendsPtr)> successCallback, ErrorCallback errorCallback)
{
    RpcRequest* rpcRequest = createRpcRequest(session);
    auto data(make_shared<nakama::api::Friends>());

    if (successCallback)
    {
        rpcRequest->successCallback = [data, successCallback]()
        {
            NFriendsPtr friends(new NFriends());
            assign(*friends, *data);
            successCallback(friends);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    auto responseReader = _stub->AsyncListFriends(&rpcRequest->context, {}, &_cq);

    responseReader->Finish(&(*data), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::createGroup(
    NSessionPtr session,
    const std::string & name,
    const std::string & description,
    const std::string & avatarUrl,
    const std::string & langTag,
    bool open,
    std::function<void(const NGroup&)> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(session);
    auto groupData(make_shared<nakama::api::Group>());

    if (successCallback)
    {
        rpcRequest->successCallback = [groupData, successCallback]()
        {
            NGroup group;
            assign(group, *groupData);
            successCallback(group);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::CreateGroupRequest req;

    req.set_name(name);

    if (!description.empty())
        req.set_description(description);

    if (!avatarUrl.empty())
        req.set_avatar_url(avatarUrl);

    if (!langTag.empty())
        req.set_lang_tag(langTag);

    req.set_open(open);

    auto responseReader = _stub->AsyncCreateGroup(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*groupData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::deleteGroup(
    NSessionPtr session,
    const std::string & groupId,
    std::function<void()> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(session);

    rpcRequest->successCallback = successCallback;
    rpcRequest->errorCallback = errorCallback;

    nakama::api::DeleteGroupRequest req;

    req.set_group_id(groupId);

    auto responseReader = _stub->AsyncDeleteGroup(&rpcRequest->context, req, &_cq);

    responseReader->Finish(nullptr, &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::addGroupUsers(
    NSessionPtr session,
    const std::string & groupId,
    const std::vector<std::string>& ids,
    std::function<void()> successCallback,
    ErrorCallback errorCallback
)
{
    RpcRequest* rpcRequest = createRpcRequest(session);

    rpcRequest->successCallback = successCallback;
    rpcRequest->errorCallback = errorCallback;

    nakama::api::AddGroupUsersRequest req;

    req.set_group_id(groupId);

    for (auto& id : ids)
    {
        req.add_user_ids(id);
    }

    auto responseReader = _stub->AsyncAddGroupUsers(&rpcRequest->context, req, &_cq);

    responseReader->Finish(nullptr, &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::listGroupUsers(NSessionPtr session, const std::string & groupId, std::function<void(NGroupUserListPtr)> successCallback, ErrorCallback errorCallback)
{
    RpcRequest* rpcRequest = createRpcRequest(session);
    auto groupData(make_shared<nakama::api::GroupUserList>());

    if (successCallback)
    {
        rpcRequest->successCallback = [groupData, successCallback]()
        {
            NGroupUserListPtr users(new NGroupUserList());
            assign(*users, *groupData);
            successCallback(users);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::ListGroupUsersRequest req;

    req.set_group_id(groupId);

    auto responseReader = _stub->AsyncListGroupUsers(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*groupData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::kickGroupUsers(NSessionPtr session, const std::string & groupId, const std::vector<std::string>& ids, std::function<void()> successCallback, ErrorCallback errorCallback)
{
    RpcRequest* rpcRequest = createRpcRequest(session);

    rpcRequest->successCallback = successCallback;
    rpcRequest->errorCallback = errorCallback;

    nakama::api::KickGroupUsersRequest req;

    req.set_group_id(groupId);

    for (auto& id : ids)
    {
        req.add_user_ids(id);
    }

    auto responseReader = _stub->AsyncKickGroupUsers(&rpcRequest->context, req, &_cq);

    responseReader->Finish(nullptr, &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::joinGroup(NSessionPtr session, const std::string & groupId, std::function<void()> successCallback, ErrorCallback errorCallback)
{
    RpcRequest* rpcRequest = createRpcRequest(session);

    rpcRequest->successCallback = successCallback;
    rpcRequest->errorCallback = errorCallback;

    nakama::api::JoinGroupRequest req;

    req.set_group_id(groupId);

    auto responseReader = _stub->AsyncJoinGroup(&rpcRequest->context, req, &_cq);

    responseReader->Finish(nullptr, &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::leaveGroup(NSessionPtr session, const std::string & groupId, std::function<void()> successCallback, ErrorCallback errorCallback)
{
    RpcRequest* rpcRequest = createRpcRequest(session);

    rpcRequest->successCallback = successCallback;
    rpcRequest->errorCallback = errorCallback;

    nakama::api::LeaveGroupRequest req;

    req.set_group_id(groupId);

    auto responseReader = _stub->AsyncLeaveGroup(&rpcRequest->context, req, &_cq);

    responseReader->Finish(nullptr, &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::listGroups(NSessionPtr session, const std::string & name, int limit, const std::string & cursor, std::function<void(NGroupListPtr)> successCallback, ErrorCallback errorCallback)
{
    RpcRequest* rpcRequest = createRpcRequest(session);
    auto groupData(make_shared<nakama::api::GroupList>());

    if (successCallback)
    {
        rpcRequest->successCallback = [groupData, successCallback]()
        {
            NGroupListPtr groups(new NGroupList());
            assign(*groups, *groupData);
            successCallback(groups);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::ListGroupsRequest req;

    req.set_name(name);

    if (limit > 0)
        req.mutable_limit()->set_value(limit);

    if (!cursor.empty())
        req.set_cursor(cursor);

    auto responseReader = _stub->AsyncListGroups(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*groupData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::listUserGroups(NSessionPtr session, std::function<void(NUserGroupListPtr)> successCallback, ErrorCallback errorCallback)
{
    listUserGroups(session, "", successCallback, errorCallback);
}

void DefaultClient::listUserGroups(NSessionPtr session, const std::string & userId, std::function<void(NUserGroupListPtr)> successCallback, ErrorCallback errorCallback)
{
    RpcRequest* rpcRequest = createRpcRequest(session);
    auto groupData(make_shared<nakama::api::UserGroupList>());

    if (successCallback)
    {
        rpcRequest->successCallback = [groupData, successCallback]()
        {
            NUserGroupListPtr groups(new NUserGroupList());
            assign(*groups, *groupData);
            successCallback(groups);
        };
    }
    rpcRequest->errorCallback = errorCallback;

    nakama::api::ListUserGroupsRequest req;

    if (!userId.empty())
        req.set_user_id(userId);

    auto responseReader = _stub->AsyncListUserGroups(&rpcRequest->context, req, &_cq);

    responseReader->Finish(&(*groupData), &rpcRequest->status, (void*)rpcRequest);
}

void DefaultClient::promoteGroupUsers(NSessionPtr session, const std::string & groupId, const std::vector<std::string>& ids, std::function<void()> successCallback, ErrorCallback errorCallback)
{
    RpcRequest* rpcRequest = createRpcRequest(session);

    rpcRequest->successCallback = successCallback;
    rpcRequest->errorCallback = errorCallback;

    nakama::api::PromoteGroupUsersRequest req;

    req.set_group_id(groupId);

    for (auto& id : ids)
    {
        req.add_user_ids(id);
    }

    auto responseReader = _stub->AsyncPromoteGroupUsers(&rpcRequest->context, req, &_cq);

    responseReader->Finish(nullptr, &rpcRequest->status, (void*)rpcRequest);
}

}
