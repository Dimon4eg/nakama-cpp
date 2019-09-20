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

#include "nakama-cpp/NClientInterface.h"
#include "nakama-c/ClientFactory.h"
#include "nakama-cpp-c-wrapper/Impl/NSessionWrapperImpl.h"
#include "nakama-cpp-c-wrapper/Impl/NDataHelperWrapperImpl.h"
#include <unordered_map>

NAKAMA_NAMESPACE_BEGIN

#define NOT_IMPLEMENTED

    /**
     * C++ wrapper for C client.
     */
    class NClientWrapper : public NClientInterface
    {
    public:
        const NClientReqData INVALID_REQ_ID = 0;

        NClientWrapper(::NClient cClient, const NClientParameters& parameters)
            : _cClient(cClient), _host(parameters.host), _ssl(parameters.ssl)
        {
            NClient_setUserData(cClient, this);
        }

        ~NClientWrapper()
        {
            ::destroyNakamaClient(_cClient);
        }

        static NClientWrapper* getWrapper(::NClient cClient)
        {
            return (NClientWrapper*)NClient_getUserData(cClient);
        }

        static ::NSession getCSession(const NSessionPtr& session)
        {
            return ((NSessionWrapper*)session.get())->getCSession();
        }

        static void OnDefaultErrorStatic(::NClient cClient, const tNError* cError)
        {
            getWrapper(cClient)->OnDefaultError(cError);
        }

        void OnDefaultError(const tNError* cError)
        {
            if (_defaultErrorCallback)
            {
                NError error;
                assign(error, cError);
                _defaultErrorCallback(error);
            }
        }

        void setErrorCallback(ErrorCallback errorCallback) override
        {
            _defaultErrorCallback = errorCallback;
            ::NClient_setErrorCallback(_cClient, &NClientWrapper::OnDefaultErrorStatic);
        }

        void setUserData(void* userData) override
        {
            _userData = userData;
        }

        void* getUserData() const override
        {
            return _userData;
        }

        void disconnect() override
        {
            ::NClient_disconnect(_cClient);
        }

        void tick() override
        {
            ::NClient_tick(_cClient);
        }

        NRtClientPtr createRtClient(int32_t port, NRtTransportPtr transport) override
        {
            RtClientParameters parameters;

            parameters.host = _host;
            parameters.port = port;
            parameters.ssl = _ssl;

            return createRtClient(parameters, transport);
        }
        
        NRtClientPtr createRtClient(const RtClientParameters& parameters, NRtTransportPtr transport) override
        {
            NOT_IMPLEMENTED
            return nullptr;
        }

        static void authenticateOkStatic(::NClient cClient, ::NClientReqData reqData, ::NSession cSession)
        {
            getWrapper(cClient)->authenticateOk(cSession, reqData);
        }

        void authenticateOk(::NSession cSession, ::NClientReqData reqData)
        {
            if (reqData != INVALID_REQ_ID)
            {
                auto it = _authSuccessCallbacks.find(reqData);
                if (it != _authSuccessCallbacks.end())
                {
                    NSessionPtr session = NSessionWrapper::create(cSession);
                    it->second(session);
                    _authSuccessCallbacks.erase(it);
                }
            }
        }

        static void reqErrorStatic(::NClient cClient, ::NClientReqData reqData, const ::tNError* cError)
        {
            getWrapper(cClient)->reqError(reqData, cError);
        }

        void reqError(::NClientReqData reqData, const ::tNError* cError)
        {
            if (reqData != INVALID_REQ_ID)
            {
                auto it = _reqErrorCallbacks.find(reqData);
                if (it != _reqErrorCallbacks.end())
                {
                    NError error;
                    assign(error, cError);
                    it->second(error);
                    _reqErrorCallbacks.erase(it);
                }
            }
        }

        static void reqOkEmptyStatic(::NClient cClient, ::NClientReqData reqData)
        {
            getWrapper(cClient)->reqOkEmpty(reqData);
        }

        void reqOkEmpty(::NClientReqData reqData)
        {
            if (reqData != INVALID_REQ_ID)
            {
                auto it = _reqOkEmptyCallbacks.find(reqData);
                if (it != _reqOkEmptyCallbacks.end())
                {
                    it->second();
                }
            }
        }

        NClientReqData getNextReqId()
        {
            ++_curReqId;
            if (_curReqId == (uintptr_t)INVALID_REQ_ID)
                ++_curReqId;
            return (NClientReqData)_curReqId;
        }

        void authenticateDevice(
            const std::string& id,
            const opt::optional<std::string>& username,
            const opt::optional<bool>& create,
            const NStringMap& vars,
            std::function<void (NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _authSuccessCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NStringMap cVars = toCNStringMap(vars);

            ::NClient_authenticateDevice(_cClient,
                id.c_str(),
                username ? username.value().c_str() : nullptr,
                create ? *create : false,
                cVars,
                reqId,
                &NClientWrapper::authenticateOkStatic,
                &NClientWrapper::reqErrorStatic);

            ::NStringMap_destroy(cVars);
        }

        void authenticateEmail(
            const std::string& email,
            const std::string& password,
            const std::string& username,
            bool create,
            const NStringMap& vars,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _authSuccessCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NStringMap cVars = toCNStringMap(vars);

            ::NClient_authenticateEmail(_cClient,
                email.c_str(),
                password.c_str(),
                !username.empty() ? username.c_str() : nullptr,
                create,
                cVars,
                reqId,
                &NClientWrapper::authenticateOkStatic,
                &NClientWrapper::reqErrorStatic);

            ::NStringMap_destroy(cVars);
        }

        void authenticateFacebook(
            const std::string& accessToken,
            const std::string& username,
            bool create,
            bool importFriends,
            const NStringMap& vars,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _authSuccessCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NStringMap cVars = toCNStringMap(vars);

            ::NClient_authenticateFacebook(_cClient,
                accessToken.c_str(),
                !username.empty() ? username.c_str() : nullptr,
                create,
                importFriends,
                cVars,
                reqId,
                &NClientWrapper::authenticateOkStatic,
                &NClientWrapper::reqErrorStatic);

            ::NStringMap_destroy(cVars);
        }

        void authenticateGoogle(
            const std::string& accessToken,
            const std::string& username,
            bool create,
            const NStringMap& vars,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _authSuccessCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NStringMap cVars = toCNStringMap(vars);

            ::NClient_authenticateGoogle(_cClient,
                accessToken.c_str(),
                !username.empty() ? username.c_str() : nullptr,
                create,
                cVars,
                reqId,
                &NClientWrapper::authenticateOkStatic,
                &NClientWrapper::reqErrorStatic);

            ::NStringMap_destroy(cVars);
        }

        void authenticateGameCenter(
            const std::string& playerId,
            const std::string& bundleId,
            NTimestamp timestampSeconds,
            const std::string& salt,
            const std::string& signature,
            const std::string& publicKeyUrl,
            const std::string& username,
            bool create,
            const NStringMap& vars,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _authSuccessCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NStringMap cVars = toCNStringMap(vars);

            ::NClient_authenticateGameCenter(_cClient,
                playerId.c_str(),
                bundleId.c_str(),
                timestampSeconds,
                salt.c_str(),
                signature.c_str(),
                publicKeyUrl.c_str(),
                !username.empty() ? username.c_str() : nullptr,
                create,
                cVars,
                reqId,
                &NClientWrapper::authenticateOkStatic,
                &NClientWrapper::reqErrorStatic);

            ::NStringMap_destroy(cVars);
        }

        void authenticateCustom(
            const std::string& id,
            const std::string& username,
            bool create,
            const NStringMap& vars,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _authSuccessCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NStringMap cVars = toCNStringMap(vars);

            ::NClient_authenticateCustom(_cClient,
                id.c_str(),
                !username.empty() ? username.c_str() : nullptr,
                create,
                cVars,
                reqId,
                &NClientWrapper::authenticateOkStatic,
                &NClientWrapper::reqErrorStatic);

            ::NStringMap_destroy(cVars);
        }

        void authenticateSteam(
            const std::string& token,
            const std::string& username,
            bool create,
            const NStringMap& vars,
            std::function<void(NSessionPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _authSuccessCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NStringMap cVars = toCNStringMap(vars);

            ::NClient_authenticateSteam(_cClient,
                token.c_str(),
                !username.empty() ? username.c_str() : nullptr,
                create,
                cVars,
                reqId,
                &NClientWrapper::authenticateOkStatic,
                &NClientWrapper::reqErrorStatic);

            ::NStringMap_destroy(cVars);
        }

        void linkFacebook(
            NSessionPtr session,
            const std::string& accessToken,
            const opt::optional<bool>& importFriends,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_linkFacebook(_cClient,
                getCSession(session),
                accessToken.c_str(),
                importFriends ? *importFriends : false,
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void linkEmail(
            NSessionPtr session,
            const std::string& email,
            const std::string& password,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_linkEmail(_cClient,
                getCSession(session),
                email.c_str(),
                password.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void linkDevice(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_linkDevice(_cClient,
                getCSession(session),
                id.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void linkGoogle(
            NSessionPtr session,
            const std::string& accessToken,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_linkGoogle(_cClient,
                getCSession(session),
                accessToken.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

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
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_linkGameCenter(_cClient,
                getCSession(session),
                playerId.c_str(),
                bundleId.c_str(),
                timestampSeconds,
                salt.c_str(),
                signature.c_str(),
                publicKeyUrl.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void linkSteam(
            NSessionPtr session,
            const std::string& token,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_linkSteam(_cClient,
                getCSession(session),
                token.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void linkCustom(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_linkCustom(_cClient,
                getCSession(session),
                id.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void unlinkFacebook(
            NSessionPtr session,
            const std::string& accessToken,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_unlinkFacebook(_cClient,
                getCSession(session),
                accessToken.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void unlinkEmail(
            NSessionPtr session,
            const std::string& email,
            const std::string& password,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_unlinkEmail(_cClient,
                getCSession(session),
                email.c_str(),
                password.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void unlinkGoogle(
            NSessionPtr session,
            const std::string& accessToken,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_unlinkGoogle(_cClient,
                getCSession(session),
                accessToken.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

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
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_unlinkGameCenter(_cClient,
                getCSession(session),
                playerId.c_str(),
                bundleId.c_str(),
                timestampSeconds,
                salt.c_str(),
                signature.c_str(),
                publicKeyUrl.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void unlinkSteam(
            NSessionPtr session,
            const std::string& token,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_unlinkSteam(_cClient,
                getCSession(session),
                token.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void unlinkDevice(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_unlinkDevice(_cClient,
                getCSession(session),
                id.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void unlinkCustom(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_unlinkCustom(_cClient,
                getCSession(session),
                id.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void importFacebookFriends(
            NSessionPtr session,
            const std::string& token,
            const opt::optional<bool>& reset,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_importFacebookFriends(_cClient,
                getCSession(session),
                token.c_str(),
                reset ? *reset : false,
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void getAccount(
            NSessionPtr session,
            std::function<void(const NAccount&)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkAccountCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_getAccount(_cClient,
                getCSession(session),
                reqId,
                &NClientWrapper::reqOkAccountStatic,
                errorCallback  ? &NClientWrapper::reqErrorStatic : nullptr);
        }

        static void reqOkAccountStatic(::NClient cClient, ::NClientReqData reqData, const sNAccount* cAccount)
        {
            getWrapper(cClient)->reqOkAccount(reqData, cAccount);
        }

        void reqOkAccount(::NClientReqData reqData, const sNAccount* cAccount)
        {
            if (reqData != INVALID_REQ_ID)
            {
                auto it = _reqOkAccountCallbacks.find(reqData);
                if (it != _reqOkAccountCallbacks.end())
                {
                    NAccount account;
                    assign(account, cAccount);
                    it->second(account);
                    _reqOkAccountCallbacks.erase(it);
                }
            }
        }

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
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_updateAccount(_cClient,
                getCSession(session),
                username ? username->c_str() : nullptr,
                displayName ? displayName->c_str() : nullptr,
                avatarUrl ? avatarUrl->c_str() : nullptr,
                langTag ? langTag->c_str() : nullptr,
                location ? location->c_str() : nullptr,
                timezone ? timezone->c_str() : nullptr,
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        static void reqOkUsersStatic(::NClient cClient, ::NClientReqData reqData, const sNUsers* cUsers)
        {
            getWrapper(cClient)->reqOkUsers(reqData, cUsers);
        }

        void reqOkUsers(::NClientReqData reqData, const sNUsers* cUsers)
        {
            if (reqData != INVALID_REQ_ID)
            {
                auto it = _reqOkUsersCallbacks.find(reqData);
                if (it != _reqOkUsersCallbacks.end())
                {
                    NUsers users;
                    assign(users, cUsers);
                    it->second(users);
                    _reqOkUsersCallbacks.erase(it);
                }
            }
        }

        void getUsers(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames,
            const std::vector<std::string>& facebookIds,
            std::function<void(const NUsers&)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkUsersCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            const char** idsArray = nullptr;
            const char** usernamesArray = nullptr;
            const char** facebookIdsArray = nullptr;

            if (ids.size() > 0)
            {
                idsArray = new const char* [ids.size()];

                for (size_t i = 0; i < ids.size(); ++i)
                {
                    idsArray[i] = ids[i].c_str();
                }
            }

            if (usernames.size() > 0)
            {
                usernamesArray = new const char* [usernames.size()];

                for (size_t i = 0; i < usernames.size(); ++i)
                {
                    usernamesArray[i] = usernames[i].c_str();
                }
            }

            if (facebookIds.size() > 0)
            {
                facebookIdsArray = new const char* [facebookIds.size()];

                for (size_t i = 0; i < facebookIds.size(); ++i)
                {
                    facebookIdsArray[i] = facebookIds[i].c_str();
                }
            }

            ::NClient_getUsers(_cClient,
                getCSession(session),
                idsArray,
                (uint16_t)ids.size(),
                usernamesArray,
                (uint16_t)usernames.size(),
                facebookIdsArray,
                (uint16_t)facebookIds.size(),
                reqId,
                &NClientWrapper::reqOkUsersStatic,
                &NClientWrapper::reqErrorStatic);

            delete[] idsArray;
            delete[] usernamesArray;
            delete[] facebookIdsArray;
        }

        void addFriends(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            const char** idsArray = nullptr;
            const char** usernamesArray = nullptr;

            if (ids.size() > 0)
            {
                idsArray = new const char* [ids.size()];

                for (size_t i = 0; i < ids.size(); ++i)
                {
                    idsArray[i] = ids[i].c_str();
                }
            }

            if (usernames.size() > 0)
            {
                usernamesArray = new const char* [usernames.size()];

                for (size_t i = 0; i < usernames.size(); ++i)
                {
                    usernamesArray[i] = usernames[i].c_str();
                }
            }

            ::NClient_addFriends(_cClient,
                getCSession(session),
                idsArray,
                (uint16_t)ids.size(),
                usernamesArray,
                (uint16_t)usernames.size(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);

            delete[] idsArray;
            delete[] usernamesArray;
        }

        void deleteFriends(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            const char** idsArray = nullptr;
            const char** usernamesArray = nullptr;

            if (ids.size() > 0)
            {
                idsArray = new const char* [ids.size()];

                for (size_t i = 0; i < ids.size(); ++i)
                {
                    idsArray[i] = ids[i].c_str();
                }
            }

            if (usernames.size() > 0)
            {
                usernamesArray = new const char* [usernames.size()];

                for (size_t i = 0; i < usernames.size(); ++i)
                {
                    usernamesArray[i] = usernames[i].c_str();
                }
            }

            ::NClient_deleteFriends(_cClient,
                getCSession(session),
                idsArray,
                (uint16_t)ids.size(),
                usernamesArray,
                (uint16_t)usernames.size(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);

            delete[] idsArray;
            delete[] usernamesArray;
        }

        void blockFriends(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            const char** idsArray = nullptr;
            const char** usernamesArray = nullptr;

            if (ids.size() > 0)
            {
                idsArray = new const char* [ids.size()];

                for (size_t i = 0; i < ids.size(); ++i)
                {
                    idsArray[i] = ids[i].c_str();
                }
            }

            if (usernames.size() > 0)
            {
                usernamesArray = new const char* [usernames.size()];

                for (size_t i = 0; i < usernames.size(); ++i)
                {
                    usernamesArray[i] = usernames[i].c_str();
                }
            }

            ::NClient_blockFriends(_cClient,
                getCSession(session),
                idsArray,
                (uint16_t)ids.size(),
                usernamesArray,
                (uint16_t)usernames.size(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);

            delete[] idsArray;
            delete[] usernamesArray;
        }

        static void reqOkFriendListStatic(::NClient cClient, ::NClientReqData reqData, const sNFriendList* cFriendList)
        {
            getWrapper(cClient)->reqOkFriendList(reqData, cFriendList);
        }

        void reqOkFriendList(::NClientReqData reqData, const sNFriendList* cFriendList)
        {
            if (reqData != INVALID_REQ_ID)
            {
                auto it = _reqOkFriendListCallbacks.find(reqData);
                if (it != _reqOkFriendListCallbacks.end())
                {
                    NFriendListPtr friendList(new NFriendList());
                    assign(*friendList, cFriendList);
                    it->second(friendList);
                    _reqOkFriendListCallbacks.erase(it);
                }
            }
        }

        void listFriends(
            NSessionPtr session,
            const opt::optional<int32_t>& limit,
            const opt::optional<NFriend::State>& state,
            const std::string& cursor,
            std::function<void(NFriendListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkFriendListCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_listFriends(_cClient,
                getCSession(session),
                limit ? &(*limit) : nullptr,
                state ? (const eFriendState*)&(*state) : nullptr,
                cursor.empty() ? nullptr : cursor.c_str(),
                reqId,
                &NClientWrapper::reqOkFriendListStatic,
                &NClientWrapper::reqErrorStatic);
        }

        static void reqOkGroupStatic(::NClient cClient, ::NClientReqData reqData, const sNGroup* cGroup)
        {
            getWrapper(cClient)->reqOkGroup(reqData, cGroup);
        }

        void reqOkGroup(::NClientReqData reqData, const sNGroup* cGroup)
        {
            if (reqData != INVALID_REQ_ID)
            {
                auto it = _reqOkGroupCallbacks.find(reqData);
                if (it != _reqOkGroupCallbacks.end())
                {
                    NGroup group;
                    assign(group, cGroup);
                    it->second(group);
                    _reqOkGroupCallbacks.erase(it);
                }
            }
        }

        void createGroup(
            NSessionPtr session,
            const std::string& name,
            const std::string& description,
            const std::string& avatarUrl,
            const std::string& langTag,
            bool open,
            const opt::optional<int32_t>& maxCount,
            std::function<void(const NGroup&)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkGroupCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_createGroup(_cClient,
                getCSession(session),
                name.c_str(),
                description.c_str(),
                avatarUrl.c_str(),
                langTag.c_str(),
                open,
                maxCount ? &(*maxCount) : nullptr,
                reqId,
                &NClientWrapper::reqOkGroupStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void deleteGroup(
            NSessionPtr session,
            const std::string& groupId,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_deleteGroup(_cClient,
                getCSession(session),
                groupId.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void addGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            const std::vector<std::string>& ids,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            const char** idsArray = nullptr;

            if (ids.size() > 0)
            {
                idsArray = new const char* [ids.size()];

                for (size_t i = 0; i < ids.size(); ++i)
                {
                    idsArray[i] = ids[i].c_str();
                }
            }

            ::NClient_addGroupUsers(_cClient,
                getCSession(session),
                groupId.c_str(),
                idsArray,
                (uint16_t)ids.size(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);

            delete[] idsArray;
        }

        static void reqOkGroupUserListStatic(::NClient cClient, ::NClientReqData reqData, const sNGroupUserList* cGroupUserList)
        {
            getWrapper(cClient)->reqOkGroupUserList(reqData, cGroupUserList);
        }

        void reqOkGroupUserList(::NClientReqData reqData, const sNGroupUserList* cGroupUserList)
        {
            if (reqData != INVALID_REQ_ID)
            {
                auto it = _reqOkGroupUserListCallbacks.find(reqData);
                if (it != _reqOkGroupUserListCallbacks.end())
                {
                    NGroupUserListPtr groupUserList(new NGroupUserList());
                    assign(*groupUserList, cGroupUserList);
                    it->second(groupUserList);
                    _reqOkGroupUserListCallbacks.erase(it);
                }
            }
        }

        void listGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            const opt::optional<int32_t>& limit,
            const opt::optional<NFriend::State>& state,
            const std::string& cursor,
            std::function<void(NGroupUserListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkGroupUserListCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_listGroupUsers(_cClient,
                getCSession(session),
                groupId.c_str(),
                limit ? &(*limit) : nullptr,
                state ? (const eFriendState*)&(*state) : nullptr,
                cursor.empty() ? nullptr : cursor.c_str(),
                reqId,
                &NClientWrapper::reqOkGroupUserListStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void kickGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            const std::vector<std::string>& ids,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            const char** idsArray = nullptr;

            if (ids.size() > 0)
            {
                idsArray = new const char* [ids.size()];

                for (size_t i = 0; i < ids.size(); ++i)
                {
                    idsArray[i] = ids[i].c_str();
                }
            }

            ::NClient_kickGroupUsers(_cClient,
                getCSession(session),
                groupId.c_str(),
                idsArray,
                (uint16_t)ids.size(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);

            delete[] idsArray;
        }

        void joinGroup(
            NSessionPtr session,
            const std::string& groupId,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_joinGroup(_cClient,
                getCSession(session),
                groupId.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void leaveGroup(
            NSessionPtr session,
            const std::string& groupId,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_leaveGroup(_cClient,
                getCSession(session),
                groupId.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        static void reqOkGroupListStatic(::NClient cClient, ::NClientReqData reqData, const sNGroupList* cGroupList)
        {
            getWrapper(cClient)->reqOkGroupList(reqData, cGroupList);
        }

        void reqOkGroupList(::NClientReqData reqData, const sNGroupList* cGroupList)
        {
            if (reqData != INVALID_REQ_ID)
            {
                auto it = _reqOkGroupListCallbacks.find(reqData);
                if (it != _reqOkGroupListCallbacks.end())
                {
                    NGroupListPtr groupList(new NGroupList());
                    assign(*groupList, cGroupList);
                    it->second(groupList);
                    _reqOkGroupListCallbacks.erase(it);
                }
            }
        }

        void listGroups(
            NSessionPtr session,
            const std::string& name,
            int32_t limit,
            const std::string& cursor,
            std::function<void(NGroupListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkGroupListCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_listGroups(_cClient,
                getCSession(session),
                name.c_str(),
                &limit,
                cursor.empty() ? nullptr : cursor.c_str(),
                reqId,
                &NClientWrapper::reqOkGroupListStatic,
                &NClientWrapper::reqErrorStatic);
        }

        static void reqOkUserGroupListStatic(::NClient cClient, ::NClientReqData reqData, const sNUserGroupList* cUserGroupList)
        {
            getWrapper(cClient)->reqOkUserGroupList(reqData, cUserGroupList);
        }

        void reqOkUserGroupList(::NClientReqData reqData, const sNUserGroupList* cUserGroupList)
        {
            if (reqData != INVALID_REQ_ID)
            {
                auto it = _reqOkUserGroupListCallbacks.find(reqData);
                if (it != _reqOkUserGroupListCallbacks.end())
                {
                    NUserGroupListPtr userGroupList(new NUserGroupList());
                    assign(*userGroupList, cUserGroupList);
                    it->second(userGroupList);
                    _reqOkUserGroupListCallbacks.erase(it);
                }
            }
        }

        void listUserGroups(
            NSessionPtr session,
            const opt::optional<int32_t>& limit,
            const opt::optional<NFriend::State>& state,
            const std::string& cursor,
            std::function<void(NUserGroupListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            this->listUserGroups(session, "", limit, state, cursor, successCallback, errorCallback);
        }

        void listUserGroups(
            NSessionPtr session,
            const std::string& userId,
            const opt::optional<int32_t>& limit,
            const opt::optional<NFriend::State>& state,
            const std::string& cursor,
            std::function<void(NUserGroupListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkUserGroupListCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_listUserGroups(_cClient,
                getCSession(session),
                userId.empty() ? nullptr : userId.c_str(),
                limit ? &(*limit) : nullptr,
                state ? (const eFriendState*) & (*state) : nullptr,
                cursor.empty() ? nullptr : cursor.c_str(),
                reqId,
                &NClientWrapper::reqOkUserGroupListStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void promoteGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            const std::vector<std::string>& ids,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            const char** idsArray = nullptr;

            if (ids.size() > 0)
            {
                idsArray = new const char* [ids.size()];

                for (size_t i = 0; i < ids.size(); ++i)
                {
                    idsArray[i] = ids[i].c_str();
                }
            }

            ::NClient_promoteGroupUsers(_cClient,
                getCSession(session),
                groupId.c_str(),
                idsArray,
                (uint16_t)ids.size(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);

            delete[] idsArray;
        }

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
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_updateGroup(_cClient,
                getCSession(session),
                groupId.c_str(),
                name ? name->c_str() : nullptr,
                description ? description->c_str() : nullptr,
                avatarUrl ? avatarUrl->c_str() : nullptr,
                langTag ? langTag->c_str() : nullptr,
                open ? &(*open) : nullptr,
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        static void reqOkLeaderboardRecordListStatic(::NClient cClient, ::NClientReqData reqData, const sNLeaderboardRecordList* cList)
        {
            getWrapper(cClient)->reqOkLeaderboardRecordList(reqData, cList);
        }

        void reqOkLeaderboardRecordList(::NClientReqData reqData, const sNLeaderboardRecordList* cList)
        {
            if (reqData != INVALID_REQ_ID)
            {
                auto it = _reqOkLeaderboardRecordListCallbacks.find(reqData);
                if (it != _reqOkLeaderboardRecordListCallbacks.end())
                {
                    NLeaderboardRecordListPtr list(new NLeaderboardRecordList());
                    assign(*list, cList);
                    it->second(list);
                    _reqOkLeaderboardRecordListCallbacks.erase(it);
                }
            }
        }

        void listLeaderboardRecords(
            NSessionPtr session,
            const std::string& leaderboardId,
            const std::vector<std::string>& ownerIds,
            const opt::optional<int32_t>& limit,
            const opt::optional<std::string>& cursor,
            std::function<void(NLeaderboardRecordListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkLeaderboardRecordListCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            const char** ownerIdsArray = nullptr;

            if (ownerIds.size() > 0)
            {
                ownerIdsArray = new const char* [ownerIds.size()];

                for (size_t i = 0; i < ownerIds.size(); ++i)
                {
                    ownerIdsArray[i] = ownerIds[i].c_str();
                }
            }

            ::NClient_listLeaderboardRecords(_cClient,
                getCSession(session),
                leaderboardId.c_str(),
                ownerIdsArray,
                (uint16_t)ownerIds.size(),
                limit ? *limit : 0,
                cursor ? (*cursor).c_str() : nullptr,
                reqId,
                &NClientWrapper::reqOkLeaderboardRecordListStatic,
                &NClientWrapper::reqErrorStatic);

            delete[] ownerIdsArray;
        }

        void listLeaderboardRecordsAroundOwner(
            NSessionPtr session,
            const std::string& leaderboardId,
            const std::string& ownerId,
            const opt::optional<int32_t>& limit,
            std::function<void(NLeaderboardRecordListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkLeaderboardRecordListCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_listLeaderboardRecordsAroundOwner(_cClient,
                getCSession(session),
                leaderboardId.c_str(),
                ownerId.c_str(),
                limit ? *limit : 0,
                reqId,
                &NClientWrapper::reqOkLeaderboardRecordListStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void writeLeaderboardRecord(
            NSessionPtr session,
            const std::string& leaderboardId,
            int64_t score,
            const opt::optional<int64_t>& subscore,
            const opt::optional<std::string>& metadata,
            std::function<void(NLeaderboardRecord)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void writeTournamentRecord(
            NSessionPtr session,
            const std::string& tournamentId,
            int64_t score,
            const opt::optional<int64_t>& subscore,
            const opt::optional<std::string>& metadata,
            std::function<void(NLeaderboardRecord)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void deleteLeaderboardRecord(
            NSessionPtr session,
            const std::string& leaderboardId,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_deleteLeaderboardRecord(_cClient,
                getCSession(session),
                leaderboardId.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void listMatches(
            NSessionPtr session,
            const opt::optional<int32_t>& min_size,
            const opt::optional<int32_t>& max_size,
            const opt::optional<int32_t>& limit,
            const opt::optional<std::string>& label,
            const opt::optional<bool>& authoritative,
            std::function<void(NMatchListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void listNotifications(
            NSessionPtr session,
            const opt::optional<int32_t>& limit,
            const opt::optional<std::string>& cacheableCursor,
            std::function<void(NNotificationListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void deleteNotifications(
            NSessionPtr session,
            const std::vector<std::string>& notificationIds,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            const char** notificationIdsArray = nullptr;

            if (notificationIds.size() > 0)
            {
                notificationIdsArray = new const char*[notificationIds.size()];

                for (size_t i = 0; i < notificationIds.size(); ++i)
                {
                    notificationIdsArray[i] = notificationIds[i].c_str();
                }
            }

            ::NClient_deleteNotifications(_cClient,
                getCSession(session),
                notificationIdsArray,
                (uint16_t)notificationIds.size(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);

            delete[] notificationIdsArray;
        }

        void listChannelMessages(
            NSessionPtr session,
            const std::string& channelId,
            const opt::optional<int32_t>& limit,
            const opt::optional<std::string>& cursor,
            const opt::optional<bool>& forward,
            std::function<void(NChannelMessageListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void listTournaments(
            NSessionPtr session,
            const opt::optional<uint32_t>& categoryStart,
            const opt::optional<uint32_t>& categoryEnd,
            const opt::optional<uint32_t>& startTime,
            const opt::optional<uint32_t>& endTime,
            const opt::optional<int32_t>& limit,
            const opt::optional<std::string>& cursor,
            std::function<void(NTournamentListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void listTournamentRecords(
            NSessionPtr session,
            const std::string& tournamentId,
            const opt::optional<int32_t>& limit,
            const opt::optional<std::string>& cursor,
            const std::vector<std::string>& ownerIds,
            std::function<void(NTournamentRecordListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void listTournamentRecordsAroundOwner(
            NSessionPtr session,
            const std::string& tournamentId,
            const std::string& ownerId,
            const opt::optional<int32_t>& limit,
            std::function<void(NTournamentRecordListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void joinTournament(
            NSessionPtr session,
            const std::string& tournamentId,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::NClient_joinTournament(_cClient,
                getCSession(session),
                tournamentId.c_str(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);
        }

        void listStorageObjects(
            NSessionPtr session,
            const std::string& collection,
            const opt::optional<int32_t>& limit,
            const opt::optional<std::string>& cursor,
            std::function<void(NStorageObjectListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void listUsersStorageObjects(
            NSessionPtr session,
            const std::string& collection,
            const std::string& userId,
            const opt::optional<int32_t>& limit,
            const opt::optional<std::string>& cursor,
            std::function<void(NStorageObjectListPtr)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void writeStorageObjects(
            NSessionPtr session,
            const std::vector<NStorageObjectWrite>& objects,
            std::function<void(const NStorageObjectAcks&)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void readStorageObjects(
            NSessionPtr session,
            const std::vector<NReadStorageObjectId>& objectIds,
            std::function<void(const NStorageObjects&)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

        void deleteStorageObjects(
            NSessionPtr session,
            const std::vector<NDeleteStorageObjectId>& objectIds,
            std::function<void()> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NClientReqData reqId = INVALID_REQ_ID;

            if (successCallback || errorCallback)
            {
                reqId = getNextReqId();
                if (successCallback) _reqOkEmptyCallbacks.emplace(reqId, successCallback);
                if (errorCallback) _reqErrorCallbacks.emplace(reqId, errorCallback);
            }

            ::sNDeleteStorageObjectId* objectIdsArray = nullptr;

            if (objectIds.size() > 0)
            {
                objectIdsArray = new ::sNDeleteStorageObjectId[objectIds.size()];

                for (size_t i =0; i < objectIds.size(); ++i)
                {
                    const auto& obj = objectIds[i];
                    objectIdsArray[i].collection = obj.collection.c_str();
                    objectIdsArray[i].key        = obj.key.c_str();
                    objectIdsArray[i].version    = obj.version.c_str();
                }
            }

            ::NClient_deleteStorageObjects(_cClient,
                getCSession(session),
                objectIdsArray,
                (uint16_t)objectIds.size(),
                reqId,
                &NClientWrapper::reqOkEmptyStatic,
                &NClientWrapper::reqErrorStatic);

            delete[] objectIdsArray;
        }

        void rpc(
            NSessionPtr session,
            const std::string& id,
            const opt::optional<std::string>& payload,
            std::function<void(const NRpc&)> successCallback,
            ErrorCallback errorCallback
        ) override
        {
            NOT_IMPLEMENTED
        }

    protected:
        std::string _host;
        bool _ssl;
        ::NClient _cClient = nullptr;
        ErrorCallback _defaultErrorCallback;
        void* _userData = nullptr;
        uintptr_t _curReqId = (uintptr_t)INVALID_REQ_ID;
        std::unordered_map<NClientReqData, std::function<void(NSessionPtr)>> _authSuccessCallbacks;
        std::unordered_map<NClientReqData, ErrorCallback> _reqErrorCallbacks;
        std::unordered_map<NClientReqData, std::function<void()>> _reqOkEmptyCallbacks;
        std::unordered_map<NClientReqData, std::function<void(const NAccount&)>> _reqOkAccountCallbacks;
        std::unordered_map<NClientReqData, std::function<void(const NUsers&)>> _reqOkUsersCallbacks;
        std::unordered_map<NClientReqData, std::function<void(NFriendListPtr)>> _reqOkFriendListCallbacks;
        std::unordered_map<NClientReqData, std::function<void(const NGroup&)>> _reqOkGroupCallbacks;
        std::unordered_map<NClientReqData, std::function<void(NGroupUserListPtr)>> _reqOkGroupUserListCallbacks;
        std::unordered_map<NClientReqData, std::function<void(NGroupListPtr)>> _reqOkGroupListCallbacks;
        std::unordered_map<NClientReqData, std::function<void(NUserGroupListPtr)>> _reqOkUserGroupListCallbacks;
        std::unordered_map<NClientReqData, std::function<void(NLeaderboardRecordListPtr)>> _reqOkLeaderboardRecordListCallbacks;
    };

NAKAMA_NAMESPACE_END
