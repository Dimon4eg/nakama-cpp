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

#include "realtime/RtClientTestBase.h"
#include "test_serverConfig.h"

namespace Nakama {
namespace Test {

NRtClientProtocol NRtClientTest::protocol = NRtClientProtocol::Protobuf;

void NRtClientTest::runTest()
{
    createWorkingClient();

    listener.setConnectCallback([this]()
    {
        if (onRtConnect)
            onRtConnect();
        else
            stopTest();
    });

    listener.setDisconnectCallback([this]()
    {
        if (!isDone())
        {
            std::cout << "Disconnected!" << std::endl;
            stopTest();
        }
    });

    listener.setErrorCallback([this](const NRtError& error)
    {
        stopTest();
    });

    listener.setStatusPresenceCallback([](const NStatusPresenceEvent& event)
    {
        for (auto& presence : event.joins)
        {
            std::cout << "Joined User ID: " << presence.userId << " Username: " << presence.username << " Status: " << presence.status << std::endl;
        }

        for (auto& presence : event.leaves)
        {
            std::cout << "Left User ID: " << presence.userId << " Username: " << presence.username << " Status: " << presence.status << std::endl;
        }
    });

    listener.setChannelMessageCallback([](const NChannelMessage& message)
    {
        std::cout << "Received a message on channel " << message.channelId << std::endl;
        std::cout << "Message content: " << message.content << std::endl;
    });

    auto successCallback = [this](NSessionPtr session)
    {
        this->session = session;

        std::cout << "session token: " << session->getAuthToken() << std::endl;

        rtClient = client->createRtClient(SERVER_HTTP_PORT);

        rtClient->setListener(&listener);

        rtClient->connect(session, true, protocol);
    };

    client->authenticateDevice("mytestdevice0000", opt::nullopt, true, successCallback);

    NTest::runTest();
}

void NRtClientTest::tick()
{
    NTest::tick();

    if (rtClient)
    {
        rtClient->tick();
    }
}

} // namespace Test
} // namespace Nakama
