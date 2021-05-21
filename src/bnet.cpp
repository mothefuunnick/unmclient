/*

    Copyright 2010 Trevor Hogan

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/

#include "unm.h"
#include "util.h"
#include "config.h"
#include "language.h"
#include "socket.h"
#include "commandpacket.h"
#include "bncsutilinterface.h"
#include "bnetprotocol.h"
#include "unmprotocol.h"
#include "bnet.h"
#include "map.h"
#include "packed.h"
#include "savegame.h"
#include "replay.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "game_base.h"
#include "bnetbot.h"

#ifdef WIN32
#include <ws2tcpip.h>
#endif

using namespace std;

extern CLanguage *gLanguage;
extern string gLanguageFile;
extern ExampleObject *gToLog;
extern bool LogonDataRunAH;
extern bool AuthorizationPassed;
extern string gAltThreadState;
extern bool gExitProcessStarted;
extern bool gRestartAfterExiting;

//
// CBNET
//

CBNET::CBNET( CUNM *nUNM, string nServer, uint16_t nPort, uint32_t nTimeWaitingToReconnect, string nServerAlias, string nCDKeyROC, string nCDKeyTFT, string nCountryAbbrev, string nCountry, uint32_t nLocaleID, string nUserName, string nUserPassword, string nFirstChannel, uint32_t nHostName, string nHostNameCustom, string nCommandTrigger, bool nHoldFriends, bool nHoldClan, unsigned char nWar3Version, BYTEARRAY nEXEVersion, BYTEARRAY nEXEVersionHash, string nEXEInfo, string nPasswordHashType, string nPVPGNRealmName, bool nSpamBotFixBnetCommands, bool nSpamSubBot, bool nSpamSubBotChat, uint32_t nMainBotID, int32_t nMainBotChatID, uint32_t nSpamBotOnlyPM, uint32_t nSpamBotMessagesSendDelay, uint32_t nSpamBotMode, uint32_t nSpamBotNet, uint32_t nPacketSendDelay, uint32_t nMessageLength, uint32_t nMaxMessageLength, uint32_t nAllowAutoRehost, uint32_t  nAllowAutoRefresh, bool nAllowManualRehost, bool nAllowUseCustomMapGameType, bool nAllowUseCustomUpTime, uint32_t nCustomMapGameType, uint32_t nCustomUpTime, uint32_t nAutoRehostDelay, uint32_t nRefreshDelay, uint32_t nHostDelay, uint32_t nPacketDelaySmall, uint32_t nPacketDelayMedium, uint32_t nPacketDelayBig, bool nRunIccupLoader, bool nDisableMuteCommandOnBnet, uint32_t nDisableDefaultNotifyMessagesInGUI, uint32_t nServerType, bool nReconnect, uint32_t nHostCounterID )
{
    m_NewConnect = false;
    m_NewServer = string( );
    m_NewLogin = string( );
    m_NewPassword = string( );
    m_NewRunAH = false;
    m_UNM = nUNM;
    m_Server = nServer;
    m_ServerType = nServerType;
    m_Reconnect = nReconnect;
    m_Socket = new CTCPClient( );
    m_GetLastReceivePacketTicks = 0;
    m_LastGetPublicListTime = 0;
    m_LastGetPublicListTimeFull = 0;
    m_LastGetPublicListTimeGeneral = 0;
    m_InChat = false;
    m_InGame = false;
    m_LowerServer = m_Server;
    transform( m_LowerServer.begin( ), m_LowerServer.end( ), m_LowerServer.begin( ), static_cast<int(*)(int)>(tolower) );

    if( !nServerAlias.empty( ) )
        m_ServerAlias = nServerAlias;
    else if( m_ServerType == 1 )
        m_ServerAlias = "UNMServer";
    else if( m_UNM->IsIccupServer( m_LowerServer ) )
        m_ServerAlias = "iCCup";
    else if( m_LowerServer == "useast.battle.net" )
        m_ServerAlias = "USEast";
    else if( m_LowerServer == "uswest.battle.net" )
        m_ServerAlias = "USWest";
    else if( m_LowerServer == "asia.battle.net" )
        m_ServerAlias = "Asia";
    else if( m_LowerServer == "europe.battle.net" )
        m_ServerAlias = "Europe";
    else if( m_LowerServer == "server.eurobattle.net" || m_LowerServer == "eurobattle.net" )
        m_ServerAlias = "EuroBattle";
    else if( m_LowerServer == "europe.warcraft3.eu" )
        m_ServerAlias = "W3EU";
    else if( m_LowerServer == "battle.lp.ro" )
        m_ServerAlias = "BattleRo";
    else if( m_LowerServer == "200.51.203.231" )
        m_ServerAlias = "OMBU";
    else if( m_LowerServer == "83.209.79.68" )
        m_ServerAlias = "Giddo";
    else if( m_LowerServer == "pvpgn.onligamez.ru" )
        m_ServerAlias = "OZ Game";
    else if( m_LowerServer == "rubattle.net" )
        m_ServerAlias = "RuBattle";
    else if( m_LowerServer == "playground.ru" )
        m_ServerAlias = "Playground";
    else
        m_ServerAlias = m_Server;

    m_UserName = nUserName;
    m_UserNameLower = m_UserName;
    transform( m_UserNameLower.begin( ), m_UserNameLower.end( ), m_UserNameLower.begin( ), static_cast<int(*)(int)>(tolower) );
    m_UserPassword = nUserPassword;

    if( m_UNM->IsIccupServer( m_LowerServer ) )
        transform( m_UserPassword.begin( ), m_UserPassword.end( ), m_UserPassword.begin( ), static_cast<int(*)(int)>(tolower) );

    if( m_ServerType == 0 )
    {
        m_ServerAliasWithUserName = "[BNET: " + m_ServerAlias + " (" + m_UserName + ")]";
        m_UNMProtocol = nullptr;
        m_Protocol = new CBNETProtocol( m_UNM, m_ServerAliasWithUserName );
        m_BNCSUtil = new CBNCSUtilInterface( nUserName, nUserPassword );
    }
    else if( m_ServerType == 1 )
    {
        m_ServerAliasWithUserName = "[UNMSERVER: " + m_ServerAlias + " (" + m_UserName + ")]";
        m_UNMProtocol = new CUNMProtocol( );
        m_Protocol = nullptr;
        m_BNCSUtil = nullptr;
    }
    else
    {
        m_ServerAliasWithUserName = "[BNET: " + m_ServerAlias + " (" + m_UserName + ")]";
        m_UNMProtocol = nullptr;
        m_Protocol = nullptr;
        m_BNCSUtil = nullptr;
    }

    m_SpamBotFixBnetCommands = nSpamBotFixBnetCommands;
    m_SpamSubBot = nSpamSubBot;
    m_SpamSubBotChat = nSpamSubBotChat;
    m_MainBotID = nMainBotID;
    m_MainBotChatID = nMainBotChatID,
    m_SpamBotOnlyPM = nSpamBotOnlyPM;
    m_SpamBotMessagesSendDelay = nSpamBotMessagesSendDelay;
    m_SpamBotMode = nSpamBotMode;
    m_SpamBotNet = nSpamBotNet;

    if( m_UNM->m_ComputerName == "unm" )
        m_ComputerName = nUserName;
    else
        m_ComputerName = m_UNM->m_ComputerName;

    m_Exiting = false;
    m_TimeWaitingToReconnect = nTimeWaitingToReconnect;
    m_Port = nPort;
    m_RunIccupLoader = nRunIccupLoader;
    m_DisableMuteCommandOnBnet = nDisableMuteCommandOnBnet;
    m_DisableDefaultNotifyMessagesInGUI = nDisableDefaultNotifyMessagesInGUI;
    m_CountryAbbrev = nCountryAbbrev;
    m_Country = nCountry;
    m_LocaleID = nLocaleID;
    m_CDKeyROC = nCDKeyROC;
    m_CDKeyTFT = nCDKeyTFT;

    if( m_ServerType == 0 )
    {
        // remove dashes from CD keys and convert to uppercase

        m_CDKeyROC.erase( remove( m_CDKeyROC.begin( ), m_CDKeyROC.end( ), '-' ), m_CDKeyROC.end( ) );
        m_CDKeyTFT.erase( remove( m_CDKeyTFT.begin( ), m_CDKeyTFT.end( ), '-' ), m_CDKeyTFT.end( ) );
        transform( m_CDKeyROC.begin( ), m_CDKeyROC.end( ), m_CDKeyROC.begin( ), static_cast<int(*)(int)>(toupper) );
        transform( m_CDKeyTFT.begin( ), m_CDKeyTFT.end( ), m_CDKeyTFT.begin( ), static_cast<int(*)(int)>(toupper) );

        if( m_CDKeyROC.size( ) != 26 )
            CONSOLE_Print( m_ServerAliasWithUserName + " warning - your ROC CD key is not 26 characters long and is probably invalid" );

        if( m_UNM->m_TFT && m_CDKeyTFT.size( ) != 26 )
            CONSOLE_Print( m_ServerAliasWithUserName + " warning - your TFT CD key is not 26 characters long and is probably invalid" );
    }

    m_FirstChannel = nFirstChannel;
    m_DefaultChannel = m_FirstChannel;
    m_HostName = nHostName;
    m_HostNameCustom = nHostNameCustom;
    m_CommandTrigger = '\0';
    m_CommandTrigger1 = '\0';
    m_CommandTrigger2 = '\0';
    m_CommandTrigger3 = '\0';
    m_CommandTrigger4 = '\0';
    m_CommandTrigger5 = '\0';
    m_CommandTrigger6 = '\0';
    m_CommandTrigger7 = '\0';
    m_CommandTrigger8 = '\0';
    m_CommandTrigger9 = '\0';
    m_CommandTrigger = nCommandTrigger[0];

    if( nCommandTrigger.size( ) >= 2 )
    {
        m_CommandTrigger1 = nCommandTrigger[1];

        if( nCommandTrigger.size( ) >= 3 )
        {
            m_CommandTrigger2 = nCommandTrigger[2];

            if( nCommandTrigger.size( ) >= 4 )
            {
                m_CommandTrigger3 = nCommandTrigger[3];

                if( nCommandTrigger.size( ) >= 5 )
                {
                    m_CommandTrigger4 = nCommandTrigger[4];

                    if( nCommandTrigger.size( ) >= 6 )
                    {
                        m_CommandTrigger5 = nCommandTrigger[5];

                        if( nCommandTrigger.size( ) >= 7 )
                        {
                            m_CommandTrigger6 = nCommandTrigger[6];

                            if( nCommandTrigger.size( ) >= 8 )
                            {
                                m_CommandTrigger7 = nCommandTrigger[7];

                                if( nCommandTrigger.size( ) >= 9 )
                                {
                                    m_CommandTrigger8 = nCommandTrigger[8];

                                    if( nCommandTrigger.size( ) >= 10 )
                                        m_CommandTrigger9 = nCommandTrigger[9];
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    m_War3Version = nWar3Version;
    m_EXEVersion = nEXEVersion;
    m_EXEVersionHash = nEXEVersionHash;
    m_EXEInfo = nEXEInfo;
    m_PasswordHashType = nPasswordHashType;
    m_PVPGNRealmName = nPVPGNRealmName;
    m_ReplyTarget = string( );
    m_PacketSendDelay = nPacketSendDelay;
    m_MaxMessageLength = nMaxMessageLength;
    m_MessageLength = nMessageLength;
    m_HostCounterID = nHostCounterID;
    m_LastDisconnectedTime = 0;
    m_LastConnectionAttemptTime = 0;
    m_LastNullTime = 0;
    m_LastOutPacketTicks = 0;
    m_LastOutPacketTicks3 = 0;
    m_LastOutPacketSize[0] = 0;
    m_LastOutPacketSize[1] = 0;
    m_LastOutPacketSize[2] = 0;
    m_LastMessageSendTicks = 0;
    m_MaxOutPacketsSize = 30;
    m_ExternalIP = string( );
    m_QuietRehost = true;
    m_FirstLogon = true;
    m_FirstConnect = true;
    m_FastReconnect = false;

    if( m_HostCounterID == 1 )
        m_FirstTryConnect = 0;
    else
        m_FirstTryConnect = 2;

    m_LastFriendListTime = 0;
    m_WaitingToConnect = true;
    m_GameStartedMessagePrinting = false;
    m_GlobalChat = false;
    m_FirstSuccessfulLogin = false;
    m_LoggedIn = false;
    m_HoldFriends = nHoldFriends;
    m_HoldClan = nHoldClan;
    m_IccupReconnect = false;
    m_IccupReconnectChannel = m_FirstChannel;
    m_AutoRehostDelay = nAutoRehostDelay;
    m_RefreshDelay = nRefreshDelay;
    m_HostDelay = nHostDelay;
    m_LastGameUncreateTime = m_UNM->TicksDifference( m_AutoRehostDelay );
    m_LastRefreshTime = m_UNM->TicksDifference( m_RefreshDelay );
    m_LastHostTime = m_UNM->TicksDifference( m_HostDelay );
    m_LastRehostTimeFix = 0;
    m_RehostTimeFix = 0;
    m_LastRefreshTimeFix = 0;
    m_RefreshTimeFix = 0;
    m_HostCounter = 0;
    m_AllowAutoRehost = nAllowAutoRehost;
    m_AllowAutoRefresh = nAllowAutoRefresh;
    m_AllowManualRehost = nAllowManualRehost;
    m_CustomMapGameType = nCustomMapGameType;
    m_CustomUpTime = nCustomUpTime;
    m_AllowUseCustomMapGameType = nAllowUseCustomMapGameType;
    m_AllowUseCustomUpTime = nAllowUseCustomUpTime;
    m_GameName = string( );
    m_GameIsHosted = false;
    m_GameIsReady = false;
    m_GameNameAlreadyChanged = false;
    m_ResetUncreateTime = true;
    m_PacketDelaySmall = nPacketDelaySmall;
    m_PacketDelayMedium = nPacketDelayMedium;
    m_PacketDelayBig = nPacketDelayBig;
    m_WaitResponse = false;
    m_LoginErrorMessage = string( );
    m_IncorrectPasswordAttemptsNumber = 0;
    m_MyChannel = true;
    m_HostPacketSentTime = GetTime( );
    m_HostPacketSent = false;
    m_LogonSuccessfulTest = false;
    m_LogonSuccessfulTestTime = GetTime( );
    m_LastRequestSearchGame = 0;
    m_HideFirstEmptyMessage = false;
    m_LastSearchBotTicks = 0;
}

CBNET::~CBNET( )
{
    delete m_Socket;

    while( !m_Packets.empty( ) )
    {
        delete m_Packets.front( );
        m_Packets.pop( );
    }

    if( m_ServerType == 0 )
    {
        delete m_Protocol;
        delete m_BNCSUtil;
    }
    else if( m_ServerType == 1 )
        delete m_UNMProtocol;

    for( vector<CIncomingFriendList *>::iterator i = m_Friends.begin( ); i != m_Friends.end( ); i++ )
        delete *i;

    for( vector<CIncomingClanList *>::iterator i = m_Clans.begin( ); i != m_Clans.end( ); i++ )
        delete *i;

    if( !m_SpamSubBot )
    {
        for( vector<CIncomingGameHost *>::iterator i = m_GPGames.begin( ); i != m_GPGames.end( ); i++ )
        {
            if( ( *i )->GetBroadcasted( ) )
                m_UNM->m_UDPSocket->Broadcast( 6112, m_UNM->m_GameProtocol->SEND_W3GS_DECREATEGAME( ( *i )->GetUniqueGameID( ) ), "[GPROXY: " + ( *i )->GetGameName( ) + "]" );
        }

        for( vector<CIncomingGameHost *>::iterator i = m_GPGames.begin( ); i != m_GPGames.end( ); i++ )
            delete *i;

        m_GPGames.clear( );
    }
}

BYTEARRAY CBNET::GetUniqueName( )
{
    if( m_ServerType == 0 )
        return m_Protocol->GetUniqueName( );

    return BYTEARRAY( );
}

unsigned int CBNET::SetFD( void *fd, void *send_fd, int *nfds )
{
    unsigned int NumFDs = 0;

    if( !m_Socket->HasError( ) && m_Socket->GetConnected( ) )
    {
        m_Socket->SetFD( (fd_set *)fd, (fd_set *)send_fd, nfds );
        NumFDs++;
    }

    return NumFDs;
}

bool CBNET::Update( void *fd, void *send_fd )
{
    //
    // update commands from GUI
    //

    for( uint32_t i = 0; i < m_GamelistRequestQueue.size( ); )
    {
        bool gamefound = false;

        for( vector<CIncomingGameHost *>::iterator j = m_UNM->m_BNETs[m_MainBotID]->m_GPGames.begin( ); j != m_UNM->m_BNETs[m_MainBotID]->m_GPGames.end( ); j++ )
        {
            if( (*j)->GetUniqueGameID( ) == m_GamelistRequestQueue[i] )
            {
                gamefound = true;
                (*j)->SetBroadcasted( true );
                break;
            }
        }

        if( !gamefound )
            emit gToLog->deleteGame( m_GamelistRequestQueue[i] );

        m_GamelistRequestQueue.erase( m_GamelistRequestQueue.begin( ) );
    }

    for( uint32_t i = 0; i < m_QueueSearchBotPre.size( ); )
    {
        if( !m_QueueSearchBotPre[i].empty( ) )
        {
            bool NewBotName = true;

            for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.size( ); i++ )
            {
                if( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot[i] == m_QueueSearchBotPre[i] )
                {
                    NewBotName = false;
                    break;
                }
            }

            if( NewBotName )
            {
                m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.push_back( m_QueueSearchBotPre[i] );
                m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBotBool.push_back( false );
            }
        }

        m_QueueSearchBotPre.erase( m_QueueSearchBotPre.begin( ) );
    }

    for( uint32_t i = 0; i < m_QueueChatCommandQueue.size( ); )
    {
        if( !m_QueueChatCommandUserQueue[i].empty( ) )
            QueueChatCommand( m_QueueChatCommandQueue[i], m_QueueChatCommandUserQueue[i], m_QueueChatCommandWhisperQueue[i] );
        else
            QueueChatCommand( m_QueueChatCommandQueue[i], m_QueueChatCommandWhisperQueue[i] );

        m_QueueChatCommandQueue.erase( m_QueueChatCommandQueue.begin( ) );
        m_QueueChatCommandUserQueue.erase( m_QueueChatCommandUserQueue.begin( ) );
        m_QueueChatCommandWhisperQueue.erase( m_QueueChatCommandWhisperQueue.begin( ) );
    }

    for( uint32_t i = 0; i < m_UNMChatCommandQueue.size( ); )
    {
        UNMChatCommand( m_UNMChatCommandQueue[i] );
        m_UNMChatCommandQueue.erase( m_UNMChatCommandQueue.begin( ) );
    }

    if( GetLoggedIn( ) )
    {
        if( m_ServerType == 0 )
        {
            // refresh friends list every 5 minutes

            if( m_LastFriendListTime == 0 || GetTime( ) >= m_LastFriendListTime + 300 )
            {
                SendGetFriendsList( );
                m_LastFriendListTime = GetTime( );
            }
        }

        while( !m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame.empty( ) )
        {
            if( GetTime( ) - m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGameTime[0] >= 59 )
            {
                m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame.erase( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame.begin( ) );
                m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGameTime.erase( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGameTime.begin( ) );
            }
            else
                break;
        }

        while( !m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot.empty( ) )
        {
            if( GetTime( ) - m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBotTime[0] >= 10 )
            {
                m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot.erase( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot.begin( ) );
                m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBotTime.erase( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBotTime.begin( ) );
            }
            else
                break;
        }
    }

    if( m_NewConnect )
        ChangeLogonDataReal( m_NewServer, m_NewLogin, m_NewPassword, m_NewRunAH );

    if( m_ServerType == 0 )
        BNETServerSocketUpdate( fd, send_fd );
    else if( m_ServerType == 1 )
        UNMServerSocketUpdate( fd, send_fd );

    return m_Exiting;
}

void CBNET::ExtractPackets( )
{
    // extract as many packets as possible from the socket's receive buffer and put them in the m_Packets queue

    string *RecvBuffer = m_Socket->GetBytes( );
    BYTEARRAY Bytes = UTIL_CreateByteArray( (unsigned char *)RecvBuffer->c_str( ), RecvBuffer->size( ) );

    // a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

    while( Bytes.size( ) >= 4 )
    {
        // byte 0 is always 255

        if( Bytes[0] == BNET_HEADER_CONSTANT )
        {
            // bytes 2 and 3 contain the length of the packet

            uint16_t Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

            if( Length >= 4 )
            {
                if( Bytes.size( ) >= Length )
                {
                    m_Packets.push( new CCommandPacket( BNET_HEADER_CONSTANT, Bytes[1], BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) );
                    *RecvBuffer = RecvBuffer->substr( Length );
                    Bytes = BYTEARRAY( Bytes.begin( ) + Length, Bytes.end( ) );
                }
                else
                    return;
            }
            else
            {
                CONSOLE_Print( m_ServerAliasWithUserName + " error - received invalid packet from battle.net (bad length), disconnecting" );
                m_Socket->Disconnect( );
                return;
            }
        }
        else
        {
            CONSOLE_Print( m_ServerAliasWithUserName + " error - received invalid packet from battle.net (bad header constant), disconnecting" );
            m_Socket->Disconnect( );
            return;
        }
    }
}

void CBNET::ProcessPackets( )
{
    // process all the received packets in the m_Packets queue
    // this normally means sending some kind of response

    while( !m_Packets.empty( ) )
    {
        CCommandPacket *Packet = m_Packets.front( );
        m_Packets.pop( );

        if( Packet->GetPacketType( ) == BNET_HEADER_CONSTANT )
        {
            switch( Packet->GetID( ) )
            {
                case CBNETProtocol::SID_NULL:
                // warning: we do not respond to nullptr packets with a nullptr packet of our own
                // this is because PVPGN servers are programmed to respond to nullptr packets so it will create a vicious cycle of useless traffic
                // official battle.net servers do not respond to nullptr packets

                m_Protocol->RECEIVE_SID_NULL( Packet->GetData( ) );
                break;

                case CBNETProtocol::SID_GETADVLISTEX:
                {
                    vector<CIncomingGameHost *> Games = m_Protocol->RECEIVE_SID_GETADVLISTEX( Packet->GetData( ) );

                    // check for reliable games
                    // GHost++ uses specific invalid map dimensions (1984) to indicated reliable games

                    uint32_t GamesReceived = Games.size( );
                    uint32_t OldReliableGamesReceived = 0;
                    uint32_t NewReliableGamesReceived = 0;

                    for( vector<CIncomingGameHost *>::iterator i = Games.begin( ); i != Games.end( ); )
                    {
                        // filter game names

                        string MapNameLower = ( *i )->GetMapPath( );
                        string HostNameLower = ( *i )->GetHostName( );
                        transform( MapNameLower.begin( ), MapNameLower.end( ), MapNameLower.begin( ), (int(*)(int))tolower );
                        transform( HostNameLower.begin( ), HostNameLower.end( ), HostNameLower.begin( ), (int(*)(int))tolower );

                        if( ( MapNameLower.find( "dota v6.83s.w3x" ) != string::npos && m_UNM->IsIccupServer( m_LowerServer ) ) || ( HostNameLower.size( ) > 10 && ( HostNameLower.substr( 0, 11 ) == "iccup.dota." || HostNameLower.substr( 0, 10 ) == "iccup.ums." ) ) || ( ( *i )->GetIPString( ) == "185.60.44.131" && ( ( *i )->GetPort( ) == 11014 || ( *i )->GetPort( ) == 11015 ) ) )
                        {
                            delete *i;
                            i = Games.erase( i );
                            continue;
                        }

                        if( AddGame( *i ) )
                        {
                            NewReliableGamesReceived++;
                            i++;
                        }
                        else
                        {
                            OldReliableGamesReceived++;
                            delete *i;
                            i = Games.erase( i );
                        }
                    }

                    if( Games.size( ) == 1 )
                    {
                        string GameName = Games[0]->GetGameName( );

                        if( !GameName.empty( ) )
                        {
                            bool NewGameName = true;

                            for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame.size( ); i++ )
                            {
                                if( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame[i] == GameName )
                                {
                                    NewGameName = false;
                                    break;
                                }
                            }

                            if( NewGameName )
                            {
                                m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGameTime.push_back( GetTime( ) );
                                m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame.push_back( GameName );
                            }
                        }
                    }

                    if( !m_UNM->m_LogReduction && GamesReceived > 0 )
                        CONSOLE_Print( m_ServerAliasWithUserName + " sifted game list, found " + UTIL_ToString( OldReliableGamesReceived + NewReliableGamesReceived ) + "/" + UTIL_ToString( GamesReceived ) + " reliable games (" + UTIL_ToString( OldReliableGamesReceived ) + " duplicates)" );

                    BYTEARRAY teststatstring;
                    BYTEARRAY teststatstring2;
                    teststatstring.push_back( 129 ); teststatstring.push_back( 3 ); teststatstring.push_back( 73 ); teststatstring.push_back( 7 ); teststatstring.push_back( 1 ); teststatstring.push_back( 1 ); teststatstring.push_back( 193 ); teststatstring.push_back( 7 ); teststatstring.push_back( 189 ); teststatstring.push_back( 193 ); teststatstring.push_back( 7 ); teststatstring.push_back( 87 ); teststatstring.push_back( 161 ); teststatstring.push_back( 47 ); teststatstring.push_back( 3 ); teststatstring.push_back( 77 ); teststatstring.push_back( 203 ); teststatstring.push_back( 97 ); teststatstring.push_back( 113 ); teststatstring.push_back( 115 ); teststatstring.push_back( 93 ); teststatstring.push_back( 69 ); teststatstring.push_back( 111 ); teststatstring.push_back( 119 ); teststatstring.push_back( 25 ); teststatstring.push_back( 111 ); teststatstring.push_back( 109 ); teststatstring.push_back( 111 ); teststatstring.push_back( 97 ); teststatstring.push_back( 101 ); teststatstring.push_back( 93 ); teststatstring.push_back( 67 ); teststatstring.push_back( 79 ); teststatstring.push_back( 111 ); teststatstring.push_back( 115 ); teststatstring.push_back( 115 ); teststatstring.push_back( 33 ); teststatstring.push_back( 67 ); teststatstring.push_back( 97 ); teststatstring.push_back( 117 ); teststatstring.push_back( 169 ); teststatstring.push_back( 117 ); teststatstring.push_back( 109 ); teststatstring.push_back( 101 ); teststatstring.push_back( 33 ); teststatstring.push_back( 49 ); teststatstring.push_back( 47 ); teststatstring.push_back( 49 ); teststatstring.push_back( 81 ); teststatstring.push_back( 49 ); teststatstring.push_back( 33 ); teststatstring.push_back( 103 ); teststatstring.push_back( 105 ); teststatstring.push_back( 121 ); teststatstring.push_back( 101 ); teststatstring.push_back( 101 ); teststatstring.push_back( 169 ); teststatstring.push_back( 33 ); teststatstring.push_back( 99 ); teststatstring.push_back( 121 ); teststatstring.push_back( 33 ); teststatstring.push_back( 99 ); teststatstring.push_back( 115 ); teststatstring.push_back( 97 ); teststatstring.push_back( 153 ); teststatstring.push_back( 119 ); teststatstring.push_back( 47 ); teststatstring.push_back( 119 ); teststatstring.push_back( 51 ); teststatstring.push_back( 121 ); teststatstring.push_back( 1 ); teststatstring.push_back( 117 ); teststatstring.push_back( 77 ); teststatstring.push_back( 111 ); teststatstring.push_back( 109 ); teststatstring.push_back( 49 ); teststatstring.push_back( 1 ); teststatstring.push_back( 1 ); teststatstring.push_back( 43 ); teststatstring.push_back( 149 ); teststatstring.push_back( 157 ); teststatstring.push_back( 229 ); teststatstring.push_back( 111 ); teststatstring.push_back( 9 ); teststatstring.push_back( 217 ); teststatstring.push_back( 179 ); teststatstring.push_back( 241 ); teststatstring.push_back( 209 ); teststatstring.push_back( 47 ); teststatstring.push_back( 39 ); teststatstring.push_back( 53 ); teststatstring.push_back( 167 ); teststatstring.push_back( 99 ); teststatstring.push_back( 187 ); teststatstring.push_back( 195 ); teststatstring.push_back( 57 ); teststatstring.push_back( 13 ); teststatstring.push_back( 123 ); teststatstring.push_back( 83 ); teststatstring.push_back( 143 ); teststatstring.push_back( 249 );
                    teststatstring2.push_back( 129 ); teststatstring2.push_back( 3 ); teststatstring2.push_back( 73 ); teststatstring2.push_back( 7 ); teststatstring2.push_back( 1 ); teststatstring2.push_back( 1 ); teststatstring2.push_back( 193 ); teststatstring2.push_back( 7 ); teststatstring2.push_back( 245 ); teststatstring2.push_back( 193 ); teststatstring2.push_back( 7 ); teststatstring2.push_back( 93 ); teststatstring2.push_back( 19 ); teststatstring2.push_back( 237 ); teststatstring2.push_back( 209 ); teststatstring2.push_back( 77 ); teststatstring2.push_back( 203 ); teststatstring2.push_back( 97 ); teststatstring2.push_back( 113 ); teststatstring2.push_back( 115 ); teststatstring2.push_back( 93 ); teststatstring2.push_back( 69 ); teststatstring2.push_back( 111 ); teststatstring2.push_back( 119 ); teststatstring2.push_back( 25 ); teststatstring2.push_back( 111 ); teststatstring2.push_back( 109 ); teststatstring2.push_back( 111 ); teststatstring2.push_back( 97 ); teststatstring2.push_back( 101 ); teststatstring2.push_back( 93 ); teststatstring2.push_back( 87 ); teststatstring2.push_back( 215 ); teststatstring2.push_back( 97 ); teststatstring2.push_back( 109 ); teststatstring2.push_back( 113 ); teststatstring2.push_back( 105 ); teststatstring2.push_back( 115 ); teststatstring2.push_back( 105 ); teststatstring2.push_back( 115 ); teststatstring2.push_back( 179 ); teststatstring2.push_back( 109 ); teststatstring2.push_back( 33 ); teststatstring2.push_back( 79 ); teststatstring2.push_back( 101 ); teststatstring2.push_back( 119 ); teststatstring2.push_back( 33 ); teststatstring2.push_back( 71 ); teststatstring2.push_back( 35 ); teststatstring2.push_back( 101 ); teststatstring2.push_back( 111 ); teststatstring2.push_back( 33 ); teststatstring2.push_back( 119 ); teststatstring2.push_back( 55 ); teststatstring2.push_back( 47 ); teststatstring2.push_back( 49 ); teststatstring2.push_back( 155 ); teststatstring2.push_back( 53 ); teststatstring2.push_back( 47 ); teststatstring2.push_back( 119 ); teststatstring2.push_back( 51 ); teststatstring2.push_back( 121 ); teststatstring2.push_back( 1 ); teststatstring2.push_back( 117 ); teststatstring2.push_back( 141 ); teststatstring2.push_back( 111 ); teststatstring2.push_back( 109 ); teststatstring2.push_back( 49 ); teststatstring2.push_back( 51 ); teststatstring2.push_back( 1 ); teststatstring2.push_back( 1 ); teststatstring2.push_back( 9 ); teststatstring2.push_back( 255 ); teststatstring2.push_back( 149 ); teststatstring2.push_back( 97 ); teststatstring2.push_back( 69 ); teststatstring2.push_back( 141 ); teststatstring2.push_back( 49 ); teststatstring2.push_back( 1 ); teststatstring2.push_back( 23 ); teststatstring2.push_back( 203 ); teststatstring2.push_back( 81 ); teststatstring2.push_back( 217 ); teststatstring2.push_back( 75 ); teststatstring2.push_back( 53 ); teststatstring2.push_back( 127 ); teststatstring2.push_back( 97 ); teststatstring2.push_back( 65 ); teststatstring2.push_back( 57 ); teststatstring2.push_back( 157 ); teststatstring2.push_back( 63 ); teststatstring2.push_back( 39 ); teststatstring2.push_back( 113 ); teststatstring2.push_back( 143 );
                    AddGame( new CIncomingGameHost( m_UNM, m_ServerAliasWithUserName, 1, 0, 65536, 11014, "185.60.44.131", 4, 43, "Boss Battle", 11, 33558282, teststatstring ) );
                    AddGame( new CIncomingGameHost( m_UNM, m_ServerAliasWithUserName, 1, 0, 65536, 11015, "185.60.44.131", 4, 43, "[VNG] VampNewGen", 11, 33558282, teststatstring2 ) );
                    emit gToLog->gameListTablePosFix( m_AllBotIDs );
                    break;
                }
                case CBNETProtocol::SID_ENTERCHAT:
                if( m_Protocol->RECEIVE_SID_ENTERCHAT( Packet->GetData( ) ) )
                {
                    m_OutPackets3.push_back( m_Protocol->SEND_SID_JOINCHANNEL( m_FirstChannel ) );
                    m_OutPackets3Channel.push_back( m_FirstChannel );
                    m_InChat = true;
                    m_InGame = false;

                    if( m_UNM->m_CurrentGame && m_GameIsHosted && m_OutPackets2.empty( ) && ( m_AllowAutoRehost == 0 || m_RefreshDelay >= 300000 ) && ( m_AllowAutoRefresh == 2 || m_AllowAutoRefresh == 3 ) )
                    {
                        m_GameIsHosted = false;
                        m_GameIsReady = true;
                    }

                    if( m_UNM->m_GameStartedMessagePrinting && m_GameStartedMessagePrinting && m_UNM->m_Games.size( ) > 0 && !m_SpamSubBot )
                    {
                        QueueChatCommand( gLanguage->GameIsStarted( m_UNM->m_Games.back( )->GetGameInfo( ) ) );
                        m_GameStartedMessagePrinting = false;
                    }
                }

                break;

                case CBNETProtocol::SID_CHATEVENT:
                {
                    CIncomingChatEvent *ChatEvent = m_Protocol->RECEIVE_SID_CHATEVENT( Packet->GetData( ) );

                    if( ChatEvent )
                    {
                        if( m_IccupReconnect )
                        {
                            if( !AuthorizationPassed && m_HostCounterID == 1 )
                                emit gToLog->logonSuccessful( );
                            else if( !m_FirstLogon )
                                emit gToLog->bnetReconnect( m_HostCounterID );

                            CONSOLE_Print( m_ServerAliasWithUserName + " iCCup reconnect successful" );
                            m_FirstLogon = false;
                            m_IccupReconnect = false;
                            m_MyChannel = true;
                            m_GameStartedMessagePrinting = false;
                            m_LoggedIn = true;
                            m_Socket->PutBytes( m_Protocol->SEND_SID_FRIENDSLIST( ) );
                            m_Socket->PutBytes( m_Protocol->SEND_SID_CLANMEMBERLIST( ) );
                            m_LastGameUncreateTime = m_UNM->TicksDifference( m_AutoRehostDelay );
                            m_LastRefreshTime = m_UNM->TicksDifference( m_RefreshDelay );
                            m_LastHostTime = m_UNM->TicksDifference( m_HostDelay );
                            m_LastOutPacketTicks = 0;
                            m_LastOutPacketTicks3 = 0;
                            m_HostPacketSentTime = GetTime( );
                            m_HostPacketSent = false;
                            m_GameIsHosted = false;
                            m_GameIsReady = true;
                            m_ResetUncreateTime = true;
                            m_GameNameAlreadyChanged = false;
                            m_LastGetPublicListTime = 0;
                            m_LastGetPublicListTimeFull = 0;
                            m_LastRequestSearchGame = 0;
                            m_HideFirstEmptyMessage = true;
                            m_OutPackets.clear( );
                            m_OutPackets2.clear( );
                            m_OutPackets3.clear( );
                            m_OutPackets3Channel.clear( );
                            m_QueueSearchBotPre.clear( );

                            if( ChatEvent->GetChatEvent( ) != CBNETProtocol::EID_CHANNEL || ChatEvent->GetMessage( ) != m_IccupReconnectChannel )
                            {
                                m_OutPackets3.push_back( m_Protocol->SEND_SID_JOINCHANNEL( m_IccupReconnectChannel ) );
                                m_OutPackets3Channel.push_back( m_IccupReconnectChannel );
                            }
                        }

                        ProcessChatEvent( ChatEvent );
                    }

                    delete ChatEvent;
                    ChatEvent = nullptr;
                    break;
                }
                case CBNETProtocol::SID_CHECKAD:
                m_Protocol->RECEIVE_SID_CHECKAD( Packet->GetData( ) );
                break;

                case CBNETProtocol::SID_STARTADVEX3:
                {
                    if( m_Protocol->RECEIVE_SID_STARTADVEX3( Packet->GetData( ) ) )
                    {
                        m_InChat = false;
                        m_InGame = true;
                        m_UNM->EventBNETGameRefreshed( this );
                    }
                    else
                        m_UNM->EventBNETGameRefreshFailed( this );

                    m_HostPacketSent = false;
                    break;
                }
                case CBNETProtocol::SID_PING:
                m_Socket->PutBytes( m_Protocol->SEND_SID_PING( m_Protocol->RECEIVE_SID_PING( Packet->GetData( ) ) ) );
                break;

                case CBNETProtocol::SID_AUTH_INFO:
                if( m_Protocol->RECEIVE_SID_AUTH_INFO( Packet->GetData( ) ) )
                {
                    if( m_BNCSUtil->HELP_SID_AUTH_CHECK( m_UNM->m_TFT, m_UNM->m_Warcraft3Path, m_CDKeyROC, m_CDKeyTFT, m_Protocol->GetValueStringFormulaString( ), m_Protocol->GetIX86VerFileNameString( ), m_Protocol->GetClientToken( ), m_Protocol->GetServerToken( ) ) )
                    {
                        // override the exe information generated by bncsutil if specified in the config file
                        // apparently this is useful for pvpgn users

                        if( m_EXEVersion.size( ) == 4 )
                        {
                            CONSOLE_Print( m_ServerAliasWithUserName + " using custom exe version bnet_custom_exeversion = " + UTIL_ToString( m_EXEVersion[0] ) + " " + UTIL_ToString( m_EXEVersion[1] ) + " " + UTIL_ToString( m_EXEVersion[2] ) + " " + UTIL_ToString( m_EXEVersion[3] ) );
                            m_BNCSUtil->SetEXEVersion( m_EXEVersion );
                        }
                        else
                            CONSOLE_Print( m_ServerAliasWithUserName + " using exe version = " + m_BNCSUtil->GetEXEVersionStr( ) );

                        if( m_EXEVersionHash.size( ) == 4 )
                        {
                            CONSOLE_Print( m_ServerAliasWithUserName + " using custom exe version hash bnet_custom_exeversionhash = " + UTIL_ToString( m_EXEVersionHash[0] ) + " " + UTIL_ToString( m_EXEVersionHash[1] ) + " " + UTIL_ToString( m_EXEVersionHash[2] ) + " " + UTIL_ToString( m_EXEVersionHash[3] ) );
                            m_BNCSUtil->SetEXEVersionHash( m_EXEVersionHash );
                        }
                        else
                            CONSOLE_Print( m_ServerAliasWithUserName + " using exe version hash = " + m_BNCSUtil->GetEXEVersionHashStr( ) );

                        if( !m_EXEInfo.empty( ) )
                        {
                            CONSOLE_Print( m_ServerAliasWithUserName + " using custom exe info bnet_custom_exeinfo = " + m_EXEInfo );
                            m_BNCSUtil->SetEXEInfo( m_EXEInfo );
                        }
                        else
                            CONSOLE_Print( m_ServerAliasWithUserName + " using exe info = " + m_BNCSUtil->GetEXEInfo( ) );

                        if( m_UNM->m_TFT )
                            CONSOLE_Print( m_ServerAliasWithUserName + " attempting to auth as Warcraft III: The Frozen Throne" );
                        else
                            CONSOLE_Print( m_ServerAliasWithUserName + " attempting to auth as Warcraft III: Reign of Chaos" );

                        if( m_UNM->IsIccupServer( m_LowerServer ) && !m_RunIccupLoader )
                            m_Socket->PutBytes( m_Protocol->SEND_ICCUP_ANTIHACK_PRELOGIN_DATA( ) );

                        m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_CHECK( m_ServerAlias, m_UNM->m_TFT, m_Protocol->GetClientToken( ), m_BNCSUtil->GetEXEVersion( ), m_BNCSUtil->GetEXEVersionHash( ), m_BNCSUtil->GetKeyInfoROC( ), m_BNCSUtil->GetKeyInfoTFT( ), m_BNCSUtil->GetEXEInfo( ), m_ComputerName ) );
                    }
                    else
                    {
                        CONSOLE_Print( m_ServerAliasWithUserName + " logon failed - bncsutil key hash failed (check your Warcraft 3 path and cd keys), disconnecting" );
                        m_LoginErrorMessage = " Ошибка хэша ключа bncsutil (проверьте путь к Warcraft 3 и используемые CD keys)";

                        m_Socket->Disconnect( );
                        delete Packet;
                        return;
                    }
                }

                break;

                case CBNETProtocol::SID_AUTH_CHECK:
                if( m_Protocol->RECEIVE_SID_AUTH_CHECK( Packet->GetData( ) ) )
                {
                    // cd keys accepted

                    CONSOLE_Print( m_ServerAliasWithUserName + " cd keys accepted" );
                    m_BNCSUtil->HELP_SID_AUTH_ACCOUNTLOGON( );
                    m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_ACCOUNTLOGON( m_ServerAlias, m_BNCSUtil->GetClientKey( ), m_UserName ) );
                }
                else
                {
                    // cd keys not accepted

                    switch( UTIL_ByteArrayToUInt32( m_Protocol->GetKeyState( ), false ) )
                    {
                        case CBNETProtocol::KR_ROC_KEY_IN_USE:
                        CONSOLE_Print( m_ServerAliasWithUserName + " logon failed - ROC CD key in use by user [" + m_Protocol->GetKeyStateDescription( ) + "], disconnecting" );
                        m_LoginErrorMessage = " Ваш ROC CD key уже используется пользователем [" + m_Protocol->GetKeyStateDescription( ) + "]";
                        break;
                        case CBNETProtocol::KR_TFT_KEY_IN_USE:
                        CONSOLE_Print( m_ServerAliasWithUserName + " logon failed - TFT CD key in use by user [" + m_Protocol->GetKeyStateDescription( ) + "], disconnecting" );
                        m_LoginErrorMessage = " Ваш TFT CD key уже используется пользователем [" + m_Protocol->GetKeyStateDescription( ) + "]";
                        break;
                        case CBNETProtocol::KR_OLD_GAME_VERSION:
                        CONSOLE_Print( m_ServerAliasWithUserName + " logon failed - game version is too old, disconnecting" );
                        m_LoginErrorMessage = " Вы используете устаревший патч, обновите версию вашей игры.";
                        break;
                        case CBNETProtocol::KR_INVALID_VERSION:
                        CONSOLE_Print( m_ServerAliasWithUserName + " logon failed - game version is invalid, disconnecting" );
                        m_LoginErrorMessage = " Вы используете невалидный патч, возможно следует откатить версию вашей игры для успешного соединения с сервером.";
                        break;
                        default:
                        CONSOLE_Print( m_ServerAliasWithUserName + " logon failed - cd keys not accepted, disconnecting" );
                        m_LoginErrorMessage = " Вы используете невалидные CD keys.";
                        break;
                    }

                    m_Socket->Disconnect( );
                    delete Packet;
                    return;
                }

                break;

                case CBNETProtocol::SID_AUTH_ACCOUNTLOGON:
                if( m_Protocol->RECEIVE_SID_AUTH_ACCOUNTLOGON( Packet->GetData( ) ) )
                {
                    CONSOLE_Print( m_ServerAliasWithUserName + " username [" + m_UserName + "] accepted" );

                    if( m_PasswordHashType == "pvpgn" )
                    {
                        // pvpgn logon

                        CONSOLE_Print( m_ServerAliasWithUserName + " using pvpgn logon type (for pvpgn servers only)" );
                        m_BNCSUtil->HELP_PvPGNPasswordHash( m_UserPassword );
                        m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_ACCOUNTLOGONPROOF( m_ServerAlias, m_BNCSUtil->GetPvPGNPasswordHash( ) ) );
                    }
                    else
                    {
                        // battle.net logon

                        CONSOLE_Print( m_ServerAliasWithUserName + " using battle.net logon type (for official battle.net servers only)" );
                        m_BNCSUtil->HELP_SID_AUTH_ACCOUNTLOGONPROOF( m_Protocol->GetSalt( ), m_Protocol->GetServerPublicKey( ) );
                        m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_ACCOUNTLOGONPROOF( m_ServerAlias, m_BNCSUtil->GetM1( ) ) );
                    }
                }
                else
                {
                    CONSOLE_Print( m_ServerAliasWithUserName + " logon failed - invalid username, disconnecting" );
                    m_LoginErrorMessage = " Вы указали неверное имя пользователя.";
                    m_Socket->Disconnect( );
                    delete Packet;
                    return;
                }

                break;

                case CBNETProtocol::SID_AUTH_ACCOUNTLOGONPROOF:
                if( m_Protocol->RECEIVE_SID_AUTH_ACCOUNTLOGONPROOF( Packet->GetData( ) ) )
                {
                    m_WaitResponse = false;
                    m_FirstTryConnect = 2;

                    // logon successful

                    if( !AuthorizationPassed && m_HostCounterID == 1 )
                    {
                        emit gToLog->logonSuccessful( );

                        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                        {
                            ( *i )->m_BnetBots.push_back( new CBNETBot( this, "etm.chat", "ETM", string( ), string( ), "http://trikiman.ru/maps", "https://vk.com/trikiman" ) );
                            emit gToLog->addBnetBot( ( *i )->GetHostCounterID( ), "ETM" );
                        }
                    }
                    else if( !m_FirstLogon )
                        emit gToLog->bnetReconnect( m_HostCounterID );

                    if( m_UNM->m_LocalSocket )
                        m_UNM->SendLocalChat( "Connected to battle.net." );

                    CONSOLE_Print( m_ServerAliasWithUserName + " logon successful" );
                    m_FirstLogon = false;
                    m_MyChannel = true;
                    m_LogonSuccessfulTest = true;
                    m_LogonSuccessfulTestTime = GetTime( );
                    m_FirstSuccessfulLogin = true;
                    m_IccupReconnect = false;
                    m_GameStartedMessagePrinting = false;
                    m_LoggedIn = true;
                    m_Socket->PutBytes( m_Protocol->SEND_SID_NETGAMEPORT( m_UNM->m_HostPort ) );
                    m_Socket->PutBytes( m_Protocol->SEND_SID_ENTERCHAT( ) );
                    m_Socket->PutBytes( m_Protocol->SEND_SID_FRIENDSLIST( ) );
                    m_Socket->PutBytes( m_Protocol->SEND_SID_CLANMEMBERLIST( ) );
                    m_LastGameUncreateTime = m_UNM->TicksDifference( m_AutoRehostDelay );
                    m_LastRefreshTime = m_UNM->TicksDifference( m_RefreshDelay );
                    m_LastHostTime = m_UNM->TicksDifference( m_HostDelay );
                    m_LastOutPacketTicks = 0;
                    m_LastOutPacketTicks3 = 0;
                    m_HostPacketSentTime = GetTime( );
                    m_HostPacketSent = false;
                    m_GameIsHosted = false;
                    m_GameIsReady = true;
                    m_ResetUncreateTime = true;
                    m_GameNameAlreadyChanged = false;
                    m_LastGetPublicListTime = 0;
                    m_LastGetPublicListTimeFull = 0;
                    m_LastRequestSearchGame = 0;
                    m_HideFirstEmptyMessage = true;
                    m_OutPackets.clear( );
                    m_OutPackets2.clear( );
                    m_OutPackets3.clear( );
                    m_OutPackets3Channel.clear( );
                    m_QueueSearchBotPre.clear( );
                }
                else
                {
                    CONSOLE_Print( m_ServerAliasWithUserName + " logon failed - invalid password, disconnecting" );

                    if( m_LowerServer != "server.eurobattle.net" && m_LowerServer != "eurobattle.net" && m_IncorrectPasswordAttemptsNumber < 3 )
                        m_IncorrectPasswordAttemptsNumber++;

                    if( m_IncorrectPasswordAttemptsNumber == 3 )
                        m_IncorrectPasswordAttemptsNumber = 5;

                    if( m_IncorrectPasswordAttemptsNumber != 5 && m_LowerServer != "server.eurobattle.net" && m_LowerServer != "eurobattle.net" )
                        m_LoginErrorMessage = " Вы указали неверный пароль. Внимание: у вас есть 3 попытки чтобы ввести верный пароль, за исчерпание этого лимита вы получите временный бан! (" + UTIL_ToString( m_IncorrectPasswordAttemptsNumber ) + "/3)";
                    else
                        m_LoginErrorMessage = " Вы указали неверный пароль.";

                    // try to figure out if the user might be using the wrong logon type since too many people are confused by this

                    string Server = m_Server;
                    transform( Server.begin( ), Server.end( ), Server.begin( ), static_cast<int(*)(int)>(tolower) );

                    if( m_PasswordHashType == "pvpgn" && ( Server == "useast.battle.net" || Server == "uswest.battle.net" || Server == "asia.battle.net" || Server == "europe.battle.net" ) )
                    {
                        CONSOLE_Print( m_ServerAliasWithUserName + " it looks like you're trying to connect to a battle.net server using a pvpgn logon type, check your config file's \"BATTLE.NET CONFIGURATION\" section" );
                        m_LoginErrorMessage = " Похоже, что вы пытаетесь подключиться к серверу battle.net, используя тип входа pvpgn, проверьте раздел \"BATTLE.NET CONFIGURATION\" вашего конфигурационного файла (unm.cfg)";
                    }
                    else if( m_PasswordHashType != "pvpgn" && ( Server != "useast.battle.net" && Server != "uswest.battle.net" && Server != "asia.battle.net" && Server != "europe.battle.net" ) )
                    {
                        CONSOLE_Print( m_ServerAliasWithUserName + " it looks like you're trying to connect to a pvpgn server using a battle.net logon type, check your config file's \"BATTLE.NET CONFIGURATION\" section" );
                        m_LoginErrorMessage = " Похоже, что вы пытаетесь подключиться к серверу pvpgn, используя тип входа battle.net, проверьте раздел \"BATTLE.NET CONFIGURATION\" вашего конфигурационного файла (unm.cfg)";
                    }

                    m_Socket->Disconnect( );
                    delete Packet;
                    return;
                }

                break;

                case CBNETProtocol::SID_WARDEN:
                {
                    CONSOLE_Print( m_ServerAliasWithUserName + " warning - received warden packet but no BNLS server is available, you will be kicked from battle.net soon" );
                    break;
                }
                case CBNETProtocol::SID_FRIENDSLIST:
                {
                vector<CIncomingFriendList *> Friends = m_Protocol->RECEIVE_SID_FRIENDSLIST( Packet->GetData( ) );

                for( vector<CIncomingFriendList *> ::iterator i = m_Friends.begin( ); i != m_Friends.end( ); i++ )
                    delete *i;

                m_Friends = Friends;
                break;
                }
                case CBNETProtocol::SID_CLANMEMBERLIST:
                {
                vector<CIncomingClanList *> Clans = m_Protocol->RECEIVE_SID_CLANMEMBERLIST( Packet->GetData( ) );

                for( vector<CIncomingClanList *> ::iterator i = m_Clans.begin( ); i != m_Clans.end( ); i++ )
                    delete *i;

                m_Clans = Clans;
                m_ClanRecruits.erase( m_ClanRecruits.begin( ), m_ClanRecruits.end( ) );
                m_ClanPeons.erase( m_ClanPeons.begin( ), m_ClanPeons.end( ) );
                m_ClanGrunts.erase( m_ClanGrunts.begin( ), m_ClanGrunts.end( ) );
                m_ClanShamans.erase( m_ClanShamans.begin( ), m_ClanShamans.end( ) );
                m_ClanChieftains.erase( m_ClanChieftains.begin( ), m_ClanChieftains.end( ) );
                m_ClanOnline.erase( m_ClanOnline.begin( ), m_ClanOnline.end( ) );
                ClanOnlineNumber = 0;

                // sort by clan ranks into appropriate vectors and status (if they are online)
                // ClanChieftains = string with names, used for -clanlist command I have while m_ClanChieftains is a vector used for IsClanChieftain( string user )

                for( vector<CIncomingClanList *> ::iterator i = m_Clans.begin( ); i != m_Clans.end( ); i++ )
                {
                    string Name = ( *i )->GetName( );
                    transform( Name.begin( ), Name.end( ), Name.begin( ), static_cast<int(*)(int)>(tolower) );

                    if( ( *i )->GetRank( ) == "Peon" )
                        m_ClanPeons.push_back( string( Name ) );
                    else if( ( *i )->GetRank( ) == "Recruit" )
                        m_ClanRecruits.push_back( string( Name ) );
                    else if( ( *i )->GetRank( ) == "Grunt" )
                        m_ClanGrunts.push_back( string( Name ) );
                    else if( ( *i )->GetRank( ) == "Shaman" )
                        m_ClanShamans.push_back( string( Name ) );
                    else if( ( *i )->GetRank( ) == "Chieftain" )
                        m_ClanChieftains.push_back( string( Name ) );
                    if( ( *i )->GetStatus( ) == "Online" )
                    {
                        m_ClanOnline.push_back( string( Name ) );
                        ClanOnlineNumber = ClanOnlineNumber + 1;
                    }
                }
                break;
                }
            }
        }
        delete Packet;
    }
}

//
// Whisper or Channel Commands
//

void CBNET::ProcessChatEvent( CIncomingChatEvent *chatEvent )
{
    CBNETProtocol::IncomingChatEvent Event = chatEvent->GetChatEvent( );
    bool Whisper = false;

    if( Event == CBNETProtocol::EID_WHISPER )
        Whisper = true;

    string User = chatEvent->GetUser( );
    bool guimsg = chatEvent->GetUserFlags( ) == 999;
    bool unmservermsg = ( m_ServerType == 1 && ( User.empty( ) || User == m_PVPGNRealmName ) );

    if( guimsg )
        User = m_UserName;
    else if( User.empty( ) )
        User = m_PVPGNRealmName;

    bool anotherusermsg = ( !guimsg && !unmservermsg );
    string UserLower = User;
    transform( UserLower.begin( ), UserLower.end( ), UserLower.begin( ), static_cast<int(*)(int)>(tolower) );
    string Message = chatEvent->GetMessage( );

    if( Event == CBNETProtocol::EID_WHISPER || Event == CBNETProtocol::EID_TALK || ( Event == CBNETProtocol::EID_ERROR && Message.find( User + ": " ) != string::npos ) )
    {
        if( Event == CBNETProtocol::EID_ERROR )
        {
            string::size_type ErrorMessageStart = Message.find( " " );
            Message = Message.substr( ErrorMessageStart + 1 );
        }

        if( Event == CBNETProtocol::EID_WHISPER )
        {
            if( !guimsg )
            {
                GUI_Print( 1, Message, User, 0 );

                if( anotherusermsg && ( UserLower != m_UserNameLower || m_ReplyTarget.empty( ) ) )
                    m_ReplyTarget = User;

                if( m_UNM->m_LocalSocket )
                    m_UNM->SendLocalChat( User + " whispers: " + Message );

                CONSOLE_Print( m_ServerAliasWithUserName + " [WSPR] " + User + ": " + Message );

                if( Message.size( ) > 13 && Message.substr( 0, 13 ) == "Watched user " )
                {
                    string findstring = " has entered a Warcraft III Frozen Throne game named ";
                    string::size_type GameStartPos = Message.find( findstring );

                    if( GameStartPos == string::npos )
                    {
                        findstring = " has entered a Warcraft III The Frozen Throne game named ";
                        GameStartPos = Message.find( findstring );
                    }

                    if( GameStartPos != string::npos )
                    {
                        string GameName = Message.substr( GameStartPos + findstring.size( ) );
                        string UserName = Message.substr( 13, Message.size( ) - GameName.size( ) - findstring.size( ) - 13 );
                        QueueChatCommand( "/unwatch " + UserName );

                        for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.size( ); i++ )
                        {
                            if( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot[i] == UserName )
                            {
                                m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.erase( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.begin( ) + static_cast<int32_t>(i) );
                                m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBotBool.erase( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBotBool.begin( ) + static_cast<int32_t>(i) );
                                break;
                            }
                        }

                        bool AlreadyInOldQueue = false;

                        for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot.size( ); i++ )
                        {
                            if( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot[i] == UserName )
                            {
                                m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBotTime[i] = GetTime( );
                                AlreadyInOldQueue = true;
                                break;
                            }
                        }

                        if( !AlreadyInOldQueue )
                        {
                            m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot.push_back( UserName );
                            m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBotTime.push_back( GetTime( ) );
                        }

                        if( GameName.size( ) > 1 && GameName.back( ) == '.' )
                            GameName.erase( GameName.end( ) - 1 );

                        if( GameName.size( ) > 2 && GameName.front( ) == '"' && GameName.back( ) == '"' )
                        {
                            GameName.erase( GameName.begin( ) );
                            GameName.erase( GameName.end( ) - 1 );
                        }

                        if( !GameName.empty( ) )
                        {
                            bool NewGameName = true;

                            for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame.size( ); i++ )
                            {
                                if( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame[i] == GameName )
                                {
                                    NewGameName = false;
                                    break;
                                }
                            }

                            if( NewGameName )
                            {
                                bool NewGameName2 = true;

                                for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame.size( ); i++ )
                                {
                                    if( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame[i] == GameName )
                                    {
                                        NewGameName2 = false;
                                        break;
                                    }
                                }

                                m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGameTime.push_back( GetTime( ) );
                                m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame.push_back( GameName );

                                if( NewGameName2 )
                                    m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame.push_back( GameName );
                            }
                        }
                    }
                }
                else if( ( Message.find( "Your friend " ) != string::npos || Message.find( "Your clanmate " ) != string::npos ) && ( Message.find( " has entered " ) != string::npos || Message.find( " has left " ) != string::npos ) && Message.size( ) > 6 && Message.find( " Warcraft III " ) == string::npos && Message.find( " Frozen Throne " ) == string::npos && Message.find( " Reign of Chaos " ) == string::npos && Message.find( " game named " ) == string::npos && Message.substr( Message.size( ) - 6 ) != " game." )
                {
                    if( Message.find( "Your friend " ) != string::npos )
                        SendGetFriendsList( );
                    else if( Message.find( "Your clanmate " ) != string::npos )
                        SendGetClanList( );
                }
                else
                {
                    for( vector<CBNETBot *>::iterator i = m_BnetBots.begin( ); i != m_BnetBots.end( ); i++ )
                    {
                        if( ( *i )->GetLoginLower( ) == UserLower )
                        {
                            if( !( *i )->GetInit( ) || !( *i )->GetLogon( ) )
                            {
                                if( Message == "[etm.chat] не имеет варнов." )
                                    ( *i )->Logon( true, 1 );
                                else if( Message == "У вас недостаточно прав для проверки чужих варнов." )
                                    ( *i )->Logon( true, 0 );
                                else
                                    ( *i )->Logon( false );
                            }
                            else if( ( *i )->GetWaitForResponse( ) )
                            {
                                if( ( *i )->GetResponseType( ) == 1 ) // !map (последующие сообщения)
                                    ( *i )->AddMaps( Message, false );
                                else if( ( *i )->GetResponseType( ) == 2 ) // !map (первое сообщение)
                                {
                                    if( Message == "Карт не найдено" || Message == "Карт не найдено." )
                                        ( *i )->MapNotFound( );
                                    else if( Message.substr( 0, 12 ) == "Карты: " )
                                        ( *i )->AddMaps( Message.substr( 12 ), true );
                                    else if( Message.substr( 0, 52 ) == "Загрузка файла конфигурации" )
                                        ( *i )->SetCurrentMap( Message.substr( 52 ) );
                                }
                                else if( ( *i )->GetResponseType( ) == 3 ) // !pub
                                    ( *i )->CreateGame( Message );
                                else // !dm (= 4)
                                    ( *i )->MapUpload( Message );
                            }

                            break;
                        }
                    }
                }
            }
        }
        else
        {
            if( !m_SpamSubBotChat )
                if( m_GlobalChat )
                    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                        if( !( *i )->m_SpamSubBotChat )
                            if( m_ServerAlias != ( *i )->GetServerAlias( ) )
                                ( *i )->QueueChatCommand( m_ServerAlias + " " + User + ": " + Message );

            if( !m_SpamSubBotChat )
                for( vector<CBaseGame *> ::iterator i = m_UNM->m_Games.begin( ); i != m_UNM->m_Games.end( ); i++ )
                    if( ( *i )->m_IntergameChat )
                        ( *i )->SendAllChat( "[" + User + "]: " + Message );

            if( m_UNM->m_CurrentGame )
                if( m_UNM->m_CurrentGame->m_IntergameChat )
                    m_UNM->m_CurrentGame->SendAllChat( "[" + User + "]: " + Message );

            GUI_Print( 0, Message, User, 0 );
            CONSOLE_Print( m_ServerAliasWithUserName + " [LOCAL] " + User + ": " + Message );
        }

        if( !guimsg )
        {
            string GameName = string( );
            string BotName = string( );

            if( Message.substr( 0, 27 ) == "Создание игры [" || Message.substr( 0, 46 ) == "Создание публичной игры [" || Message.substr( 0, 46 ) == "Создание приватной игры [" )
            {
                string::size_type BotStartPos = Message.find( "] на боте [" );

                if( BotStartPos != string::npos )
                {
                    string::size_type OwnerStartPos = Message.find( "]. Владелец" );

                    if( OwnerStartPos != string::npos )
                    {
                        if( Message.substr( 0, 27 ) == "Создание игры [" )
                            GameName = Message.substr( 27, BotStartPos - 27 );
                        else
                            GameName = Message.substr( 46, BotStartPos - 46 );

                        BotName = Message.substr( BotStartPos + 17, OwnerStartPos - ( BotStartPos + 17 ) );
                    }
                    else
                    {
                        if( Message.substr( 0, 27 ) == "Создание игры [" )
                            GameName = Message.substr( 27, BotStartPos - 27 );
                        else
                            GameName = Message.substr( 46, BotStartPos - 46 );

                        BotName = Message.substr( BotStartPos + 17 );
                        string::size_type BotEndPos = BotName.rfind( "]" );

                        if( BotEndPos != string::npos )
                        {
                            if( BotEndPos > ( m_UNM->IsIccupServer( m_LowerServer ) ? 14 : 15 ) ) // max botname length = 15
                               BotEndPos = BotName.find( "]" );

                            if( BotEndPos > ( m_UNM->IsIccupServer( m_LowerServer ) ? 14 : 15 ) )
                                BotName = string( );
                            else
                                BotName = BotName.substr( 0, BotEndPos );
                        }
                        else
                            BotName = string( );
                    }
                }
                else
                {
                    string::size_type OwnerStartPos = Message.find( "]. Владелец" );

                    if( OwnerStartPos != string::npos )
                    {
                        if( Message.substr( 0, 27 ) == "Создание игры [" )
                            GameName = Message.substr( 27, OwnerStartPos - 27 );
                        else
                            GameName = Message.substr( 46, OwnerStartPos - 46 );
                    }
                    else
                    {
                        if( Message.substr( 0, 27 ) == "Создание игры [" )
                            GameName = Message.substr( 27 );
                        else
                            GameName = Message.substr( 46 );

                        string::size_type GameNameEndPos = GameName.rfind( "]" );

                        if( GameNameEndPos != string::npos )
                        {
                            if( GameNameEndPos > 31 ) // max gamename length = 31
                               GameNameEndPos = GameName.find( "]" );

                            if( GameNameEndPos > 31 )
                                GameName = string( );
                            else
                                GameName = Message.substr( 0, GameNameEndPos );
                        }
                        else
                            GameName = string( );
                    }
                }
            }
            else if( Message.substr( 0, 12 + m_UserName.size( ) ) == m_UserName + ", игра [" || Message.substr( 0, 31 + m_UserName.size( ) ) == m_UserName + ", публичная игра [" || Message.substr( 0, 31 + m_UserName.size( ) ) == m_UserName + ", приватная игра [" )
            {
                string::size_type GameNameEndPos = Message.find( "] создана на боте [" );

                if( GameNameEndPos != string::npos )
                {
                    if( Message.substr( 0, 12 + m_UserName.size( ) ) == m_UserName + ", игра [" )
                        GameName = Message.substr( 12 + m_UserName.size( ), GameNameEndPos - ( 12 + m_UserName.size( ) ) );
                    else
                        GameName = Message.substr( 31 + m_UserName.size( ), GameNameEndPos - ( 31 + m_UserName.size( ) ) );

                    BotName = Message.substr( GameNameEndPos + 32 );
                    string::size_type BotEndPos = BotName.rfind( "]" );

                    if( BotEndPos != string::npos )
                    {
                        if( BotEndPos > ( m_UNM->IsIccupServer( m_LowerServer ) ? 14 : 15 ) ) // max botname length = 15
                           BotEndPos = BotName.find( "]" );

                        if( BotEndPos > ( m_UNM->IsIccupServer( m_LowerServer ) ? 14 : 15 ) )
                            BotName = string( );
                        else
                            BotName = BotName.substr( 0, BotEndPos );
                    }
                    else
                        BotName = string( );
                }
                else
                {
                    GameNameEndPos = Message.find( "] создана" );

                    if( GameNameEndPos != string::npos )
                    {
                        if( Message.substr( 0, 12 + m_UserName.size( ) ) == m_UserName + ", игра [" )
                            GameName = Message.substr( 12 + m_UserName.size( ), GameNameEndPos - ( 12 + m_UserName.size( ) ) );
                        else
                            GameName = Message.substr( 31 + m_UserName.size( ), GameNameEndPos - ( 31 + m_UserName.size( ) ) );
                    }
                }
            }
            else if( Message.substr( 0, 10 ) == "Игра [" || Message.substr( 0, 29 ) == "Публичная игра [" || Message.substr( 0, 29 ) == "Приватная игра [" )
            {
                string::size_type GameNameEndPos = Message.find( "] создана на боте [" );

                if( GameNameEndPos != string::npos )
                {
                    if( Message.substr( 0, 10 ) == m_UserName + ", игра [" )
                        GameName = Message.substr( 10, GameNameEndPos - ( 10 ) );
                    else
                        GameName = Message.substr( 29, GameNameEndPos - ( 29 ) );

                    BotName = Message.substr( GameNameEndPos + 32 );
                    string::size_type BotEndPos = BotName.rfind( "]" );

                    if( BotEndPos != string::npos )
                    {
                        if( BotEndPos > ( m_UNM->IsIccupServer( m_LowerServer ) ? 14 : 15 ) ) // max botname length = 15
                           BotEndPos = BotName.find( "]" );

                        if( BotEndPos > ( m_UNM->IsIccupServer( m_LowerServer ) ? 14 : 15 ) )
                            BotName = string( );
                        else
                            BotName = BotName.substr( 0, BotEndPos );
                    }
                    else
                        BotName = string( );
                }
                else
                {
                    GameNameEndPos = Message.find( "] создана" );

                    if( GameNameEndPos != string::npos )
                    {
                        if( Message.substr( 0, 10 ) == m_UserName + ", игра [" )
                            GameName = Message.substr( 10, GameNameEndPos - ( 10 ) );
                        else
                            GameName = Message.substr( 29, GameNameEndPos - ( 29 ) );
                    }
                }
            }

            if( !GameName.empty( ) )
            {
                if( GameName.size( ) > 31 )
                    GameName = GameName.substr( 0, 31 );

                if( BotName.empty( ) || BotName.size( ) > ( m_UNM->IsIccupServer( m_LowerServer ) ? 14 : 15 ) )
                {
                    if( UserLower != "etm.chat" )
                        BotName = User;
                    else
                        BotName = string( );
                }

                if( !BotName.empty( ) )
                {
                    bool NewBotName = true;

                    for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot.size( ); i++ )
                    {
                        if( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot[i] == BotName )
                        {
                            NewBotName = false;
                            break;
                        }
                    }

                    if( NewBotName )
                    {
                        for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.size( ); i++ )
                        {
                            if( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot[i] == BotName )
                            {
                                NewBotName = false;
                                break;
                            }
                        }

                        if( NewBotName )
                        {
                            m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.push_back( BotName );
                            m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBotBool.push_back( false );
                        }
                    }
                }

                if( UserLower.substr( 0, 4 ) != "etm." )
                {
                    bool NewGameName = true;

                    for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame.size( ); i++ )
                    {
                        if( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame[i] == GameName )
                        {
                            NewGameName = false;
                            break;
                        }
                    }

                    if( NewGameName )
                    {
                        for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame.size( ); i++ )
                        {
                            if( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame[i] == GameName )
                            {
                                NewGameName = false;
                                break;
                            }
                        }

                        // не добавляем в m_OldQueueSearchGame, потому как сейчас нет уверености что игра с таким названием дейсвтительно существует
                        // мы добавим в m_OldQueueSearchGame позже, в случае если такая игра действительно есть (если мы получим эту игру)

                        if( NewGameName )
                            m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame.push_back( GameName );
                    }
                }
            }

            if( Event != CBNETProtocol::EID_WHISPER )
            {
                for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                {
                    if( ( *i )->GetServer( ) == m_Server && ( *i )->GetLoggedIn( ) && ( *i )->GetName( ) == User )
                        return;
                }
            }
        }

        // handle spoof checking for current game
        // this case covers whispers - we assume that anyone who sends a whisper to the bot with message "s" should be considered spoof checked
        // note that this means you can whisper "s" even in a public game to manually spoofcheck if the /whereis fails

        if( Event == CBNETProtocol::EID_WHISPER && m_UNM->m_CurrentGame && !guimsg )
        {
            if( Message == "s" || Message == "sc" || Message == "spoof" || Message == "check" || Message == "spoofcheck" || Message == "cm" || Message == "checkme" )
                m_UNM->m_CurrentGame->AddToSpoofed( m_Server, User, true );
            else if( Message.find( m_UNM->m_CurrentGame->GetGameName( ) ) != string::npos && m_ServerType == 0 )
            {
                // look for messages like "entered a Warcraft III The Frozen Throne game called XYZ"
                // we don't look for the English part of the text anymore because we want this to work with multiple languages
                // it's a pretty safe bet that anyone whispering the bot with a message containing the game name is a valid spoofcheck

                if( m_PasswordHashType == "pvpgn" && User == m_PVPGNRealmName )
                {
                    // the equivalent pvpgn message is: [PvPGN Realm] Your friend abc has entered a Warcraft III Frozen Throne game named "xyz".

                    vector<string> Tokens = UTIL_Tokenize( Message, ' ' );

                    if( Tokens.size( ) >= 3 )
                        m_UNM->m_CurrentGame->AddToSpoofed( m_Server, Tokens[2], false );
                }
                else
                    m_UNM->m_CurrentGame->AddToSpoofed( m_Server, User, false );
            }
        }

        // handle bot commands

        if( Message == "?trigger" )
            QueueChatCommand( gLanguage->CommandTrigger( string( 1, m_CommandTrigger ) ), User, Whisper );
        else if( !Message.empty( ) && ( Message[0] == m_CommandTrigger || Message[0] == m_CommandTrigger1 || Message[0] == m_CommandTrigger2 || Message[0] == m_CommandTrigger3 || Message[0] == m_CommandTrigger4 || Message[0] == m_CommandTrigger5 || Message[0] == m_CommandTrigger6 || Message[0] == m_CommandTrigger7 || Message[0] == m_CommandTrigger8 || Message[0] == m_CommandTrigger9 ) )
        {
            // extract the command trigger, the command, and the payload
            // e.g. "!say hello world" -> command: "say", payload: "hello world"

            string CommandTrigger = Message.substr( 0, 1 );
            string Command;
            string Payload = string( );
            string PayloadLower = string( );
            string::size_type PayloadStart = Message.find( " " );

            if( PayloadStart != string::npos )
            {
                if( PayloadStart == 1 )
                {
                    string MessageTest = Message.substr( 1 );
                    string::size_type CommandStart = MessageTest.find_first_not_of( " " );

                    if( CommandStart != string::npos )
                    {
                        string CommandTest = MessageTest.substr( 0, CommandStart );
                        MessageTest = MessageTest.substr( CommandStart );
                        string::size_type PayloadStart2 = MessageTest.find( " " );

                        if( PayloadStart2 != string::npos )
                        {
                            Command = CommandTest + MessageTest.substr( 0, PayloadStart2 );
                            Payload = MessageTest.substr( PayloadStart2 + 1 );
                            PayloadLower = Payload;
                        }
                        else
                            Command = Message.substr( 1 );
                    }
                    else
                        Command = Message.substr( 1 );
                }
                else
                {
                    Command = Message.substr( 1, PayloadStart - 1 );
                    Payload = Message.substr( PayloadStart + 1 );
                    PayloadLower = Payload;
                }
            }
            else
                Command = Message.substr( 1 );

            Command = UTIL_StringToLower( Command );

            if( !PayloadLower.empty( ) )
                PayloadLower = UTIL_StringToLower( PayloadLower );

            if( guimsg || unmservermsg )
            {
                if( guimsg )
                    CONSOLE_Print( m_ServerAliasWithUserName + " вы использовали команду [" + Message + "]" );
                else
                    CONSOLE_Print( m_ServerAliasWithUserName + " " + m_PVPGNRealmName + " отправил запрос на выполнение команды [" + Message + "]" );

                //
                // !ADDIPTOBLACKLIST
                // !ADDIPBLACKLIST
                // !ADDIPTOBL
                // !ADDIPBL
                // !BLACKLISTEDIP
                // !IPBLACKLISTED
                // !BLACKLISTIP
                // !IPBLACKLIST
                // !IPTOBLACKLIST
                // !IPTOBL
                // !IPBL
                // !BLIP
                //

                if( Command == "addiptoblacklist" || Command == "addipblacklist" || Command == "addiptobl" || Command == "addipbl" || Command == "blacklistedip" || Command == "ipblacklisted" || Command == "blacklistip" || Command == "ipblacklist" || Command == "iptoblacklist" || Command == "iptobl" || Command == "ipbl" || Command == "blip" )
                {
                    if( m_UNM->m_IPBlackListFile.empty( ) )
                        QueueChatCommand( gLanguage->CommandDisabled( CommandTrigger + Command ), User, Whisper );
                    else if( Payload.empty( ) )
                        QueueChatCommand( "Укажите IP, который следует внести в блэклист, например: " + CommandTrigger + Command + " 192.168.0.1", User, Whisper );
                    else if( !m_UNM->IPIsValid( Payload ) )
                        QueueChatCommand( "Не удалось добавить [" + Payload + "] в блэклист, указан некорректный IP!", User, Whisper );
                    else
                    {
                        ifstream in;

#ifdef WIN32
                        in.open( UTIL_UTF8ToCP1251( m_UNM->m_IPBlackListFile.c_str( ) ) );
#else
                        in.open( m_UNM->m_IPBlackListFile );
#endif

                        if( in.fail( ) )
                            QueueChatCommand( "Не удалось открыть файл [" + m_UNM->m_IPBlackListFile + "]", User, Whisper );
                        else
                        {
                            string Line;

                            while( !in.eof( ) )
                            {
                                getline( in, Line );
                                Line = UTIL_FixFileLine( Line, true );

                                // ignore blank lines and comments

                                if( Line.empty( ) || Line[0] == '#' )
                                    continue;

                                // ignore lines that don't look like IP addresses

                                if( Line.find_first_not_of( "1234567890." ) != string::npos )
                                    continue;

                                if( Line == Payload )
                                {
                                    QueueChatCommand( "IP [" + Payload + "] уже имеется в блэклисте!", User, Whisper );
                                    return;
                                }
                            }

                            QueueChatCommand( "IP [" + Payload + "] был успешно добавлен в блэклист!", User, Whisper );
                            CONSOLE_Print( m_ServerAliasWithUserName + " admin [" + User + "] added IP [" + Payload + "] to blacklist file" );
                            std::ofstream in2;
#ifdef WIN32
                            in2.open( UTIL_UTF8ToCP1251( m_UNM->m_IPBlackListFile.c_str( ) ), ios::app );
#else
                            in2.open( m_UNM->m_IPBlackListFile, ios::app );
#endif
                            in2 << Payload << endl;
                            in2.close( );
                        }

                        in.close( );
                    }
                }

                //
                // !GAMECHAT
                // !CHATGAME
                //

                else if( Command == "gamechat" || Command == "chatgame" )
                {
                    uint32_t GameNumber;
                    string Switcher = string( );
                    stringstream SS;
                    SS << Payload;
                    SS >> GameNumber;

                    if( SS.fail( ) )
                        QueueChatCommand( "bad input #1 to [" + Command + "] command", User, Whisper );
                    else
                    {
                        if( SS.eof( ) )
                        {
                            if( GameNumber <= m_UNM->m_Games.size( ) && GameNumber >= 1 )
                            {
                                if( !m_UNM->m_Games[GameNumber - 1]->m_IntergameChat )
                                {
                                    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                                    {
                                        if( !( *i )->m_SpamSubBotChat )
                                            ( *i )->QueueChatCommand( "Чат между игрой №" + UTIL_ToString( GameNumber ) + " и каналом включен!" );
                                    }

                                    m_UNM->m_Games[GameNumber - 1]->SendAllChat( "[" + User + "]: Чат между этой игрой и каналом включен!" );
                                    m_UNM->m_Games[GameNumber - 1]->m_IntergameChat = true;
                                }
                                else
                                {
                                    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                                    {
                                        if( !( *i )->m_SpamSubBotChat )
                                            ( *i )->QueueChatCommand( "Чат между игрой №" + UTIL_ToString( GameNumber ) + " и каналом отключен!" );
                                    }

                                    m_UNM->m_Games[GameNumber - 1]->SendAllChat( "[" + User + "]: Чат между этой игрой и каналом отключен!" );
                                    m_UNM->m_Games[GameNumber - 1]->m_IntergameChat = false;
                                }
                            }
                            else
                                QueueChatCommand( gLanguage->GameNumberDoesntExist( UTIL_ToString( GameNumber ) ), User, Whisper );
                        }
                        else
                        {
                            getline( SS, Switcher );
                            string::size_type Start = Switcher.find_first_not_of( " " );

                            if( Start != string::npos )
                                Switcher = Switcher.substr( Start );

                            if( GameNumber <= m_UNM->m_Games.size( ) && GameNumber >= 1 )
                            {
                                if( Switcher == "on" || Switcher == "вкл" )
                                {
                                    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                                    {
                                        if( !( *i )->m_SpamSubBotChat )
                                            ( *i )->QueueChatCommand( "Чат между игрой №" + UTIL_ToString( GameNumber ) + " и каналом включен!" );
                                    }

                                    m_UNM->m_Games[GameNumber - 1]->SendAllChat( "[" + User + "]: Чат между этой игрой и каналом включен!" );
                                    m_UNM->m_Games[GameNumber - 1]->m_IntergameChat = true;
                                }
                                else if( Switcher == "off" || Switcher == "выкл" )
                                {
                                    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                                    {
                                        if( !( *i )->m_SpamSubBotChat )
                                            ( *i )->QueueChatCommand( "Чат между игрой №" + UTIL_ToString( GameNumber ) + " и каналом отключен!" );
                                    }

                                    m_UNM->m_Games[GameNumber - 1]->SendAllChat( "[" + User + "]: Чат между этой игрой и каналом отключен!" );
                                    m_UNM->m_Games[GameNumber - 1]->m_IntergameChat = false;
                                }
                                else
                                {
                                    if( !m_UNM->m_Games[GameNumber - 1]->m_IntergameChat )
                                    {
                                        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                                        {
                                            if( !( *i )->m_SpamSubBotChat )
                                                ( *i )->QueueChatCommand( "Чат между игрой №" + UTIL_ToString( GameNumber ) + " и каналом включен!" );
                                        }

                                        m_UNM->m_Games[GameNumber - 1]->SendAllChat( "[" + User + "]: Чат между этой игрой и каналом включен!" );
                                        m_UNM->m_Games[GameNumber - 1]->m_IntergameChat = true;
                                    }
                                    else
                                    {
                                        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                                        {
                                            if( !( *i )->m_SpamSubBotChat )
                                                ( *i )->QueueChatCommand( "Чат между игрой №" + UTIL_ToString( GameNumber ) + " и каналом отключен!" );
                                        }

                                        m_UNM->m_Games[GameNumber - 1]->SendAllChat( "[" + User + "]: Чат между этой игрой и каналом отключен!" );
                                        m_UNM->m_Games[GameNumber - 1]->m_IntergameChat = false;
                                    }
                                }
                            }
                            else
                                QueueChatCommand( gLanguage->GameNumberDoesntExist( UTIL_ToString( GameNumber ) ), User, Whisper );
                        }
                    }
                }

                //
                // !CHAT
                // !BNETCHAT
                // !CHATBNET
                //

                else if( Command == "chat" || Command == "bnetchat" || Command == "chatbnet" )
                {
                    if( PayloadLower == "on" )
                    {
                        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                        {
                            if( !( *i )->m_GlobalChat )
                            {
                                ( *i )->m_GlobalChat = true;

                                if( !( *i )->m_SpamSubBotChat )
                                    ( *i )->QueueChatCommand( "Межсерверный чат включен" );
                            }
                        }
                    }
                    else if( PayloadLower == "off" )
                    {
                        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                        {
                            if( ( *i )->m_GlobalChat )
                            {
                                ( *i )->m_GlobalChat = false;

                                if( !( *i )->m_SpamSubBotChat )
                                    ( *i )->QueueChatCommand( "Межсерверный чат отключен" );
                            }
                        }
                    }
                    else if( !m_GlobalChat )
                    {
                        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                        {
                            if( !( *i )->m_GlobalChat )
                            {
                                ( *i )->m_GlobalChat = true;

                                if( !( *i )->m_SpamSubBotChat )
                                    ( *i )->QueueChatCommand( "Межсерверный чат включен" );
                            }
                        }
                    }
                    else if( m_GlobalChat )
                    {
                        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                        {
                            if( ( *i )->m_GlobalChat )
                            {
                                ( *i )->m_GlobalChat = false;

                                if( !( *i )->m_SpamSubBotChat )
                                    ( *i )->QueueChatCommand( "Межсерверный чат отключен" );
                            }
                        }
                    }
                }

                //
                // !DLMAP
                // !DM
                // !DOWNLOADMAP
                // !DOWNLOAD
                // !СКАЧАТЬ
                //

                else if( Command == "dlmap" || Command == "dm" || Command == "downloadmap" || Command == "download" || Command == "скачать" )
                {
                    if( Payload.empty( ) || PayloadLower == "help" )
                        QueueChatCommand( "Укажите прямую ссылку для скачивания: " + CommandTrigger + Command + " <URL>", User, Whisper );
                    else if( gAltThreadState == "@LogBackup" || gAltThreadState == "@ThreadFinished" )
                        QueueChatCommand( "Пожалуйста, подождите несколько секунд...", User, Whisper );
                    else if( !gAltThreadState.empty( ) )
                        QueueChatCommand( "Сейчас уже производится скачивание карты, начатое админом [" + gAltThreadState + "]", User, Whisper );
                    else
                    {
                        gAltThreadState = User;
                        string CommandWithTrigger = CommandTrigger + Command;
                        QList<QString> options;
                        options.append( QString::fromUtf8( Payload.c_str( ) ) );
                        options.append( QString::fromUtf8( m_UNM->m_MapPath.c_str( ) ) );
                        options.append( QString::fromUtf8( CommandWithTrigger.c_str( ) ) );
                        options.append( UTIL_ToQString( m_HostCounterID - 1 ) );
                        options.append( QString::fromUtf8( User.c_str( ) ) );

                        if( Whisper )
                            options.append( "1" );
                        else
                            options.append( "0" );

                        QueueChatCommand( "Началась загрузка карты...", User, Whisper );
                        emit gToLog->downloadMap( options );
                    }
                }

                //
                // !END
                //

                else if( Command == "end" && !Payload.empty( ) )
                {
                    // todotodo: what if a game ends just as you're typing this command and the numbering changes?

                    uint32_t GameNumber = UTIL_ToUInt32( Payload ) - 1;

                    if( GameNumber < m_UNM->m_Games.size( ) )
                    {
                        QueueChatCommand( gLanguage->EndingGame( m_UNM->m_Games[GameNumber]->GetDescription( ) ), User, Whisper );
                        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_UNM->m_Games[GameNumber]->GetGameName( ) ) + "] будет завершена через 5 секунд администратором", 6, m_UNM->m_Games[GameNumber]->GetCurrentGameCounter( ) );
                        m_UNM->m_Games[GameNumber]->SendAllChat( "Игра завершится через 5 секунд" );
                        m_UNM->m_Games[GameNumber]->m_GameEndCountDownStarted = true;
                        m_UNM->m_Games[GameNumber]->m_GameEndCountDownCounter = 5;
                        m_UNM->m_Games[GameNumber]->m_GameEndLastCountDownTicks = GetTicks( );
                    }
                    else
                        QueueChatCommand( gLanguage->GameNumberDoesntExist( Payload ), User, Whisper );
                }

                //
                // !ENFORCESG
                //

                else if( Command == "enforcesg" && !Payload.empty( ) )
                {
                    // only load files in the current directory just to be safe

                    if( Payload.find( "/" ) != string::npos || Payload.find( "\\" ) != string::npos )
                        QueueChatCommand( gLanguage->UnableToLoadReplaysOutside( ), User, Whisper );
                    else
                    {
                        string File = m_UNM->m_ReplayPath + Payload;

                        if( !UTIL_FileExists( File ) && UTIL_FileExists( File + ".w3g" ) )
                            File += ".w3g";

                        if( UTIL_FileExists( File ) )
                        {
                            QueueChatCommand( gLanguage->LoadingReplay( File ), User, Whisper );
                            CReplay *Replay = new CReplay( );
                            Replay->Load( File, false );
                            Replay->ParseReplay( false );
                            m_UNM->m_EnforcePlayers = Replay->GetPlayers( );
                            delete Replay;
                        }
                        else
                            QueueChatCommand( gLanguage->UnableToLoadReplayDoesntExist( File ), User, Whisper );
                    }
                }

                //
                // !EXIT
                // !QUIT
                //

                else if( Command == "exit" || Command == "quit" )
                {
                    if( PayloadLower == "nice" )
                        m_UNM->m_ExitingNice = true;
                    else if( PayloadLower == "force" )
                        m_Exiting = true;
                    else
                    {
                        if( m_UNM->m_CurrentGame || !m_UNM->m_Games.empty( ) )
                            QueueChatCommand( gLanguage->AtLeastOneGameActiveUseForceToShutdown( CommandTrigger ), User, Whisper );
                        else
                            m_Exiting = true;
                    }
                }

                //
                // !RESTART
                // !REBOOT
                //

                else if( Command == "restart" || Command == "reboot" )
                {
                    bool now = false;
                    bool safe = false;
                    bool wait = false;
                    bool yes = false;

                    if( Payload == "safe" || Payload == "безопасно" )
                        safe = true;
                    else if( Payload == "yes" || Payload == "да" )
                        yes = true;
                    else if( Payload == "now" || Payload == "сейчас" )
                        now = true;
                    else if( Payload == "wait" || Payload == "подождать" )
                        wait = true;


                    if( gExitProcessStarted )
                    {
                        if( !now )
                        {
                            QueueChatCommand( "Уже начался процесс перезапуска программы. Клиент перезапустится по завершению всех процессов.", User, Whisper );
                            QueueChatCommand( "Введите " + CommandTrigger + Command + " now - чтобы перезапустить прямо сейчас.", User, Whisper );
                        }
                        else
                        {
                            gExitProcessStarted = true;
                            gRestartAfterExiting = true;
                            emit gToLog->restartUNM( true );
                        }
                    }
                    else if( m_UNM == nullptr )
                    {
                        if( !now && !yes )
                            QueueChatCommand( "Похоже, что-то пошло не так... Введите " + CommandTrigger + Command + " yes - чтобы перезапустить клиент прямо сейчас.", User, Whisper );
                        else
                        {
                            gExitProcessStarted = true;
                            gRestartAfterExiting = true;
                            emit gToLog->restartUNM( true );
                        }
                    }
                    else if( !m_UNM->m_Games.empty( ) )
                    {
                        if( !now && !safe && !yes && !wait )
                        {
                            QueueChatCommand( "Имеются начатые игры в процессе! Пезапустить клиент немедленно?", User, Whisper );
                            QueueChatCommand( "Введите " + CommandTrigger + Command + " yes - чтобы перезапустить прямо сейчас.", User, Whisper );
                            QueueChatCommand( "Введите " + CommandTrigger + Command + " wait - чтобы произвести автоматический перезапуск после завершения текущих игр.", User, Whisper );
                        }
                        else if( now || yes )
                        {
                            gExitProcessStarted = true;
                            gRestartAfterExiting = true;
                            emit gToLog->restartUNM( false );
                            m_UNM->m_Exiting = true;
                        }
                        else
                        {
                            gExitProcessStarted = true;
                            gRestartAfterExiting = true;
                            emit gToLog->restartUNM( false );
                            m_UNM->m_ExitingNice = true;
                        }
                    }
                    else if( m_UNM->m_CurrentGame )
                    {
                        if( !now && !safe && !yes )
                        {
                            QueueChatCommand( "Имеется неначатая игра процессе! Пезапустить клиент немедленно?", User, Whisper );
                            QueueChatCommand( "Введите " + CommandTrigger + Command + " yes - чтобы перезапустить.", User, Whisper );
                            QueueChatCommand( "Введите " + CommandTrigger + Command + " safe - чтобы перезапустить безопасно.", User, Whisper );
                        }
                        else if( now || yes )
                        {
                            gExitProcessStarted = true;
                            gRestartAfterExiting = true;
                            emit gToLog->restartUNM( false );
                            m_UNM->m_Exiting = true;
                        }
                        else
                        {
                            gExitProcessStarted = true;
                            gRestartAfterExiting = true;
                            emit gToLog->restartUNM( false );
                            m_UNM->m_ExitingNice = true;
                        }
                    }
                    else
                    {
                        if( !now && !safe && !yes )
                        {
                            QueueChatCommand( "Введите " + CommandTrigger + Command + " yes - чтобы перезапустить клиент.", User, Whisper );
                            QueueChatCommand( "Введите " + CommandTrigger + Command + " safe - чтобы перезапустить клиент безопасно.", User, Whisper );
                        }
                        else if( now || yes )
                        {
                            gExitProcessStarted = true;
                            gRestartAfterExiting = true;
                            emit gToLog->restartUNM( false );
                            m_UNM->m_Exiting = true;
                        }
                        else
                        {
                            gExitProcessStarted = true;
                            gRestartAfterExiting = true;
                            emit gToLog->restartUNM( false );
                            m_UNM->m_ExitingNice = true;
                        }
                    }
                }

                //
                // !GETCLAN
                // !GC
                //

                else if( Command == "getclan" || Command == "gc" )
                {
                    SendGetClanList( );
                    QueueChatCommand( gLanguage->UpdatingClanList( ), User, Whisper );
                }

                //
                // !GETFRIENDS
                // !GFS
                //

                else if( Command == "getfriends" || Command == "gfs" )
                {
                    SendGetFriendsList( );
                    QueueChatCommand( gLanguage->UpdatingFriendsList( ), User, Whisper );
                }

                //
                // !GETGAME
                // !GG
                //

                else if( ( Command == "getgame" || Command == "gg" ) && !Payload.empty( ) )
                {
                    uint32_t GameNumber = UTIL_ToUInt32( Payload ) - 1;

                    if( PayloadLower == "0" || PayloadLower == "lobby" )
                    {
                        if( m_UNM->m_CurrentGame )
                            QueueChatCommand( m_UNM->m_CurrentGame->GetGameInfo( ), User, Whisper );
                        else if( PayloadLower == "lobby" )
                            QueueChatCommand( "Сейчас нет игры в лобби!", User, Whisper );
                        else if( PayloadLower == "0" )
                            QueueChatCommand( gLanguage->GameNumberDoesntExist( Payload ), User, Whisper );
                    }
                    else if( GameNumber < m_UNM->m_Games.size( ) )
                        QueueChatCommand( m_UNM->m_Games[GameNumber]->GetGameInfo( ), User, Whisper );
                    else
                        QueueChatCommand( gLanguage->GameNumberDoesntExist( Payload ), User, Whisper );
                }

                //
                // !GETGAMES
                // !GGS
                //

                else if( Command == "getgames" || Command == "ggs" )
                {
                    if( m_PasswordHashType == "pvpgn" )
                    {
                        string s;
                        string st;

                        for( uint32_t i = 0; i < m_UNM->m_Games.size( ); i++ )
                            s = s + " " + m_UNM->m_Games[i]->GetGameInfo( );

                        if( m_UNM->m_CurrentGame )
                            s = s + "; " + gLanguage->GameIsInTheLobby( m_UNM->m_CurrentGame->GetDescription( ), UTIL_ToString( m_UNM->m_Games.size( ) ), UTIL_ToString( m_UNM->m_MaxGames ) );

                        if( !s.empty( ) && s.front( ) == ' ' )
                            s.erase( s.begin( ) );

                        if( s.size( ) <= m_MessageLength )
                            QueueChatCommand( s, User, Whisper );
                        else
                            QueueChatCommand( s, User, true );
                    }
                    else if( m_UNM->m_CurrentGame )
                    {
                        string s;
                        string st;
                        st = gLanguage->GameIsInTheLobby( m_UNM->m_CurrentGame->GetDescription( ), UTIL_ToString( m_UNM->m_Games.size( ) ), UTIL_ToString( m_UNM->m_MaxGames ) );

                        if( st.size( ) <= m_MessageLength )
                            QueueChatCommand( st, User, Whisper );
                        else
                            QueueChatCommand( st, User, true );
                    }
                    else
                    {
                        string s;
                        string st;
                        st = gLanguage->ThereIsNoGameInTheLobby( UTIL_ToString( m_UNM->m_Games.size( ) ), UTIL_ToString( m_UNM->m_MaxGames ) );


                        if( st.size( ) <= m_MessageLength )
                            QueueChatCommand( st, User, Whisper );
                        else
                            QueueChatCommand( st, User, true );
                    }
                }

                //
                // !CGAMES
                // !CURRENTGAMES
                // !CG
                //

                else if( Command == "cgames" || Command == "currentgames" || Command == "cg" )
                {
                    int itr = 0;

                    if( m_UNM->m_Games.empty( ) && m_UNM->m_CurrentGame == nullptr )
                    {
                        QueueChatCommand( "No games in progress.", User, Whisper );
                        return;
                    }
                    else
                    {
                        string s = UTIL_ToString( m_UNM->m_Games.size( ) ) + " games atm: ";

                        if( m_UNM->m_CurrentGame )
                        {
                            string Names;

                            for( vector<CGamePlayer *> ::iterator i = m_UNM->m_CurrentGame->m_Players.begin( ); i != m_UNM->m_CurrentGame->m_Players.end( ); i++ )
                            {
                                if( itr == 0 )
                                    Names = ( *i )->GetName( );
                                else
                                    Names = ( *i )->GetName( ) + ", " + Names;

                                itr++;
                            }

                            itr = 0;
                            s += "Lobby [" + m_UNM->m_CurrentGame->GetOwnerName( ) + "][" + m_UNM->IncGameNrDecryption( m_UNM->m_CurrentGame->GetGameName( ) ) + "] " + Names + ";";
                            QueueChatCommand( s, User, Whisper );
                        }

                        s.clear( );
                        string Names2;

                        for( uint32_t i = 0; i < m_UNM->m_Games.size( ); i++ )
                        {
                            if( m_UNM->m_Games[i] )
                            {
                                for( vector<CGamePlayer *> ::iterator q = m_UNM->m_Games[i]->m_Players.begin( ); q != m_UNM->m_Games[i]->m_Players.end( ); q++ )
                                {
                                    if( itr == 0 )
                                        Names2 = ( *q )->GetName( );
                                    else
                                        Names2 = ( *q )->GetName( ) + ", " + Names2;

                                    itr++;
                                }

                                uint32_t glt = m_UNM->m_Games[i]->GetGameLoadedTime( );

                                if( m_UNM->m_Games[i]->GetGameLoadedTime( ) == 0 )
                                    glt = GetTime( );

                                s += UTIL_ToString( i + 1 ) + "- [" + m_UNM->m_Games[i]->GetOwnerName( ) + "][" + m_UNM->IncGameNrDecryption( m_UNM->m_Games[i]->GetGameName( ) ) + "] (" + UTIL_ToString( ( GetTime( ) - glt ) / 60 ) + " mins) " + Names2;

                            }

                            Names2.clear( );
                        }

                        if( !s.empty( ) )
                        {
                            uint32_t MaxMessageLength2x = m_MaxMessageLength * 2;

                            if( s.size( ) <= m_MessageLength )
                                QueueChatCommand( s, User, Whisper );
                            else if( s.size( ) <= m_MaxMessageLength )
                                QueueChatCommand( s, User, true );
                            else if( s.size( ) <= MaxMessageLength2x )
                            {
                                string st;
                                st = UTIL_SubStrFix( s, m_MaxMessageLength, 0 );
                                s = UTIL_SubStrFix( s, 0, m_MaxMessageLength );
                                QueueChatCommand( s, User, true );
                                QueueChatCommand( st, User, true );
                            }
                            else
                            {
                                string st;
                                string str;
                                str = UTIL_SubStrFix( s, MaxMessageLength2x, 0 );
                                st = UTIL_SubStrFix( s, m_MaxMessageLength, m_MaxMessageLength );
                                s = UTIL_SubStrFix( s, 0, m_MaxMessageLength );
                                QueueChatCommand( s, User, true );
                                QueueChatCommand( st, User, true );
                                QueueChatCommand( str, User, true );
                            }
                        }
                    }
                }


                //
                // !REHOSTDELAY
                // !REFRESHDELAY
                // !RD
                //

                else if( Command == "rehostdelay" || Command == "refreshdelay" || Command == "rd" )
                {
                    if( Payload.empty( ) )
                        QueueChatCommand( "rehostdelay is set to " + UTIL_ToString( m_RefreshDelay ), User, Whisper );
                    else
                    {
                        m_RefreshDelay = UTIL_ToUInt32( Payload );
                        QueueChatCommand( "rehostdelay set to " + Payload, User, Whisper );
                    }
                }

                //
                // !HOSTSG
                // !HSG
                //

                else if( ( Command == "hostsg" || Command == "hsg" ) && !Payload.empty( ) )
                {
                    string GameName = Payload;
                    GameName = UTIL_SubStrFix( GameName, 0, 31 );

                    if( GameName.length( ) > 0 && GameName.length( ) < m_UNM->m_MinLengthGN )
                        QueueChatCommand( "Слишком короткое название (минимум " + UTIL_ToString( m_UNM->m_MinLengthGN ) + " симв.)", User, Whisper );
                    else if( m_UNM->m_FixGameNameForIccup && ( GameName.find( "aptb" ) != string::npos || GameName.find( "rdtb" ) != string::npos || GameName.find( "artb" ) != string::npos || GameName.find( "sdtb" ) != string::npos ) )
                        QueueChatCommand( "Название игры не может содержать 'aptb' 'rdtb' 'artb' 'sdtb'", User, Whisper );
                    else
                        m_UNM->CreateGame( m_UNM->m_Map, GAME_PRIVATE, true, GameName, User, User, m_Server, Whisper, string( ) );
                }

                //
                // !HOST
                //

                else if( Command == "host" )
                {
                    string Code;
                    string GameName;
                    stringstream SS;
                    SS << Payload;
                    SS >> Code;

                    if( SS.fail( ) )
                        QueueChatCommand( "Укажите код карты", User, Whisper );
                    else
                    {
                        if( SS.eof( ) )
                            QueueChatCommand( "Укажите название игры!", User, Whisper );
                        else
                        {
                            getline( SS, GameName );

                            if( GameName.size( ) > 0 && GameName[0] == ' ' )
                                GameName = GameName.substr( 1 );

                            GameName = UTIL_SubStrFix( GameName, 0, 31 );

                            if( GameName.find_first_not_of( " " ) == string::npos )
                            {
                                QueueChatCommand( "Не указано название игры", User, Whisper );
                                return;
                            }
                            else if( GameName.size( ) < m_UNM->m_MinLengthGN )
                            {
                                QueueChatCommand( "Слишком короткое название (минимум " + UTIL_ToString( m_UNM->m_MinLengthGN ) + " симв.)", User, Whisper );
                                return;
                            }
                            else if( Code.find_first_not_of( " " ) == string::npos )
                            {
                                QueueChatCommand( "Не указан код карты", User, Whisper );
                                return;
                            }

                            if( m_UNM->m_AppleIcon && !m_UNM->m_GameNameChar.empty( ) )
                            {
                                GameName = m_UNM->m_GameNameChar + " " + GameName;
                                GameName = UTIL_SubStrFix( GameName, 0, 31 );
                            }

                            if( m_UNM->m_FixGameNameForIccup && ( GameName.find( "aptb" ) != string::npos || GameName.find( "rdtb" ) != string::npos || GameName.find( "artb" ) != string::npos || GameName.find( "sdtb" ) != string::npos ) )
                            {
                                QueueChatCommand( "Название игры не может содержать 'aptb' 'rdtb' 'artb' 'sdtb'", User, Whisper );
                                return;
                            }

                            string Config = m_UNM->HostMapCode( Code );
                            bool load = false;
                            bool random = false;

                            if( Config.empty( ) )
                                QueueChatCommand( "Не указан код карты", User, Whisper );
                            else if( Config == "Invalid map code" )
                                QueueChatCommand( "Указан некорректный код карты.", User, Whisper );
                            else if( Config == "Invalid map config" )
                                QueueChatCommand( "Файл конфигурации карты не верный.", User, Whisper );
                            else
                            {
                                if( Config.size( ) > 11 && Config.substr( Config.size( ) - 11 ) == ".cfg random" )
                                {
                                    Config = Config.substr( 0, Config.size( ) - 7 );
                                    random = true;
                                }

                                try
                                {
                                    std::filesystem::path MapCFGPath( m_UNM->m_MapCFGPath );
                                    string Pattern = Config;
                                    Pattern = UTIL_StringToLower( Pattern );

                                    if( !std::filesystem::exists( MapCFGPath ) )
                                    {
                                        CONSOLE_Print( m_ServerAliasWithUserName + " error listing map configs - map config path doesn't exist" );
                                        QueueChatCommand( gLanguage->ErrorListingMapConfigs( ), User, Whisper );
                                    }
                                    else
                                    {
                                        string FilePath;
                                        string File;
                                        string Stem;
                                        string FoundMapConfigs;
                                        uint32_t Matches = 0;
                                        m_UNM->MapsScan( true, Pattern, FilePath, File, Stem, FoundMapConfigs, Matches );

                                        if( Matches == 0 )
                                            QueueChatCommand( "Файл конфигурации карты не верный.", User, Whisper );
                                        else if( Matches == 1 )
                                        {
                                            load = true;
                                            m_UNM->MapLoad( FilePath, File, Stem, string( ), true );
                                        }
                                        else
                                            QueueChatCommand( "Файл конфигурации карты не верный.", User, Whisper );
                                    }
                                }
                                catch( const exception &ex )
                                {
                                    CONSOLE_Print( m_ServerAliasWithUserName + " error listing map configs - caught exception [" + ex.what( ) + "]" );
                                    QueueChatCommand( gLanguage->ErrorListingMapConfigs( ), User, Whisper );
                                }
                            }
                            if( load )
                            {
                                if( Config.size( ) > 4 && Config.substr( Config.size( ) - 4 ) == ".cfg" )
                                    Config = Config.substr( 0, Config.size( ) - 4 );

                                if( random )
                                    Config = Config + " random";

                                m_UNM->CreateGame( m_UNM->m_Map, GAME_PUBLIC, false, GameName, User, User, m_Server, Whisper, Config );
                                m_UNM->m_TempGameNameChar = m_UNM->m_GameNameChar;

                                if( m_UNM->m_ResetGameNameChar )
                                    m_UNM->m_GameNameChar = m_UNM->m_DefaultGameNameChar;
                            }
                        }
                    }
                }

                //
                // !DELMAPCFG (delete config file)
                // !DELETEMAPCFG
                // !REMOVEMAPCFG
                // !DMAPCFG
                // !MAPCFGDEL
                // !MAPCFGDELETE
                // !MAPCFGD
                // !MAPCFGREMOVE
                //

                else if( Command == "delmapcfg" || Command == "deletemapcfg" || Command == "removemapcfg" || Command == "dmapcfg" || Command == "mapcfgdel" || Command == "mapcfgdelete" || Command == "mapcfgd" || Command == "mapcfgremove" )
                {
                    // we have to be careful here because we didn't copy the map data when creating the game (there's only one global copy)
                    // therefore if we change the map data while a game is in the lobby everything will get screwed up
                    // the easiest solution is to simply reject the command if a game is in the lobby

                    if( m_UNM->m_CurrentGame )
                    {
                        QueueChatCommand( gLanguage->UnableToLoadConfigFileGameInLobby( CommandTrigger ), User, Whisper );
                        return;
                    }

                    if( Payload.empty( ) )
                        QueueChatCommand( gLanguage->CurrentlyLoadedMapCFGIs( m_UNM->m_Map->GetCFGFile( ) ), User, Whisper );
                    else
                    {
                        try
                        {
                            std::filesystem::path MapCFGPath( m_UNM->m_MapCFGPath );
                            string Pattern = Payload;
                            Pattern = UTIL_StringToLower( Pattern );

                            if( !std::filesystem::exists( MapCFGPath ) )
                            {
                                CONSOLE_Print( m_ServerAliasWithUserName + " error listing map configs - map config path doesn't exist" );
                                QueueChatCommand( gLanguage->ErrorListingMapConfigs( ), User, Whisper );
                            }
                            else
                            {
                                string FilePath;
                                string File;
                                string Stem;
                                string FoundMapConfigs;
                                uint32_t Matches = 0;
                                m_UNM->MapsScan( true, Pattern, FilePath, File, Stem, FoundMapConfigs, Matches );

                                if( Matches == 0 )
                                    QueueChatCommand( gLanguage->NoMapConfigsFound( ), User, Whisper );
                                else if( Matches == 1 )
                                {
                                    std::wstring WFilePath = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(FilePath);

                                    if( _wremove( WFilePath.c_str( ) ) != 0 )
                                        QueueChatCommand( "Error deleting " + FilePath, User, Whisper );
                                    else
                                        QueueChatCommand( "Config " + FilePath + " deleted!", User, Whisper );
                                }
                                else
                                    QueueChatCommand( gLanguage->FoundMapConfigs( FoundMapConfigs ), User, Whisper );
                            }
                        }
                        catch( const exception &ex )
                        {
                            CONSOLE_Print( m_ServerAliasWithUserName + " error listing map configs - caught exception [" + ex.what( ) + "]" );
                            QueueChatCommand( gLanguage->ErrorListingMapConfigs( ), User, Whisper );
                        }
                    }
                }

                //
                // !LOADMAP (load config file)
                // !LOADM
                // !LMAP
                // !LM
                // !MAPLOAD
                // !MAPL
                // !MLOAD
                // !ML
                // !CONFIGCREATE
                // !CREATECONFIG
                // !CFGCREATE
                // !CREATECFG
                //

                else if( Command == "loadmap" || Command == "loadm" || Command == "lmap" || Command == "lm" || Command == "mapload" || Command == "mapl" || Command == "mload" || Command == "ml" || Command == "configcreate" || Command == "createconfig" || Command == "cfgcreate" || Command == "createcfg" )
                {
                    if( Payload.empty( ) )
                        QueueChatCommand( gLanguage->CurrentlyLoadedMapCFGIs( m_UNM->m_Map->GetCFGFile( ) ), User, Whisper );
                    else
                    {
                        if( Payload.find( "/" ) != string::npos || Payload.find( "\\" ) != string::npos )
                            QueueChatCommand( gLanguage->UnableToLoadConfigFilesOutside( ), User, Whisper );
                        else
                        {
                            try
                            {
                                std::filesystem::path MapPath( m_UNM->m_MapPath );
                                string Pattern = Payload;
                                Pattern = UTIL_StringToLower( Pattern );

                                if( !std::filesystem::exists( MapPath ) )
                                {
                                    CONSOLE_Print( m_ServerAliasWithUserName + " error listing maps - map path doesn't exist" );
                                    QueueChatCommand( gLanguage->ErrorListingMaps( ), User, Whisper );
                                }
                                else
                                {
                                    string FilePath;
                                    string File;
                                    string Stem;
                                    string FoundMaps;
                                    uint32_t Matches = 0;
                                    m_UNM->MapsScan( false, Pattern, FilePath, File, Stem, FoundMaps, Matches );

                                    if( Matches == 0 )
                                        QueueChatCommand( gLanguage->NoMapsFound( ), User, Whisper );
                                    else if( Matches == 1 )
                                    {
                                        if( m_UNM->m_AllowAutoRenameMaps )
                                            m_UNM->MapAutoRename( FilePath, File, m_ServerAliasWithUserName + " " );

                                        string FileCFG = m_UNM->CheckMapConfig( Stem + ".cfg", true );

                                        if( !FileCFG.empty( ) && UTIL_FileExists( FileCFG ) )
                                        {
                                            // we have to be careful here because we didn't copy the map data when creating the game (there's only one global copy)
                                            // therefore if we change the map data while a game is in the lobby everything will get screwed up
                                            // the easiest solution is to simply reject the command if a game is in the lobby

                                            if( m_UNM->m_CurrentGame )
                                            {
                                                QueueChatCommand( gLanguage->UnableToLoadConfigFileGameInLobby( CommandTrigger ), User, Whisper );
                                                return;
                                            }

                                            CConfig MapCFG;
                                            m_UNM->m_Map->SetLogAll( true );
                                            MapCFG.Set( "map_path", "Maps\\Download\\" + File );
                                            MapCFG.Set( "map_localpath", File );
                                            m_UNM->m_Map->Load( &MapCFG, File, FilePath );
                                            m_UNM->m_Map->SetLogAll( false );
                                            QueueChatCommand( gLanguage->LoadingConfigFile( Stem + ".cfg" ), User, Whisper );
                                        }
                                        else
                                        {
                                            CConfig MapCFG;
                                            m_UNM->m_Map->SetLogAll( true );
                                            MapCFG.Set( "map_path", "Maps\\Download\\" + File );
                                            MapCFG.Set( "map_localpath", File );
                                            m_UNM->m_Map->Load( &MapCFG, File, FilePath );
                                            m_UNM->m_Map->SetLogAll( false );
                                            QueueChatCommand( gLanguage->LoadingConfigFile( Stem + ".cfg" ), User, Whisper );
                                        }
                                    }
                                    else
                                        QueueChatCommand( gLanguage->FoundMaps( FoundMaps ), User, Whisper );
                                }
                            }
                            catch( const exception &ex )
                            {
                                CONSOLE_Print( m_ServerAliasWithUserName + " error listing maps - caught exception [" + ex.what( ) + "]" );
                                QueueChatCommand( gLanguage->ErrorListingMaps( ), User, Whisper );
                            }
                        }
                    }
                }

                //
                // !LOADSG
                // !LSG
                //

                else if( ( Command == "loadsg" || Command == "lsg" ) && !Payload.empty( ) )
                {
                    // only load files in the current directory just to be safe

                    if( Payload.find( "/" ) != string::npos || Payload.find( "\\" ) != string::npos )
                        QueueChatCommand( gLanguage->UnableToLoadSaveGamesOutside( ), User, Whisper );
                    else
                    {
                        string File = m_UNM->m_ReplayPath + Payload;
                        string FileNoPath = Payload;

                        if( !UTIL_FileExists( File ) && UTIL_FileExists( File + ".w3z" ) )
                        {
                            File += ".w3z";
                            FileNoPath += ".w3z";
                        }

                        if( UTIL_FileExists( File ) )
                        {
                            if( m_UNM->m_CurrentGame )
                                QueueChatCommand( gLanguage->UnableToLoadSaveGameGameInLobby( ), User, Whisper );
                            else
                            {
                                QueueChatCommand( gLanguage->LoadingSaveGame( File ), User, Whisper );

                                if( m_UNM->m_SaveGame != nullptr )
                                {
                                    delete m_UNM->m_SaveGame;
                                    m_UNM->m_SaveGame = nullptr;
                                }

                                m_UNM->m_SaveGame = new CSaveGame( );
                                m_UNM->m_SaveGame->Load( File, false );
                                m_UNM->m_SaveGame->ParseSaveGame( );
                                m_UNM->m_SaveGame->SetFileName( File );
                                m_UNM->m_SaveGame->SetFileNameNoPath( FileNoPath );
                            }
                        }
                        else
                            QueueChatCommand( gLanguage->UnableToLoadSaveGameDoesntExist( File ), User, Whisper );
                    }
                }

                //
                // !RENAMEMAPS (rename all maps)
                // !RENAMEMAP
                // !RENAMEALLMAPS
                // !RENAMEALLMAP
                // !RMAP
                // !RAM
                // !RM
                // !FIXMAPS
                // !FIXMAP
                // !FIXALLMAPS
                // !FIXALLMAP
                // !FMAP
                // !FAM
                // !FM
                //

                else if( Command == "renamemaps" || Command == "renamemap" || Command == "renameallmaps" || Command == "renameallmap" || Command == "rmap" || Command == "ram" || Command == "rm" || Command == "fixmaps" || Command == "fixmap" || Command == "fixallmaps" || Command == "fixallmap" || Command == "fmap" || Command == "fam" || Command == "fm" )
                {
                    try
                    {
                        using convert_typeX = std::codecvt_utf8<wchar_t>;
                        std::wstring_convert<convert_typeX, wchar_t> converterX;
                        std::filesystem::path Path( m_UNM->m_MapPath );
                        std::filesystem::recursive_directory_iterator EndIterator;
                        string File;
                        string FilePath;
                        string FileNewName;
                        string is;
                        QString QFile;
                        QString QFilePath;
                        QString QFileExt;
                        QString QFileNewName;
                        QString QFileNewNameTest;
                        QString iqs;
                        uint32_t in;
                        uint32_t FileNameEnd;
                        uint32_t QFileSizeTest;
                        uint32_t QFileExtSize;
                        uint32_t AllMapsCounter = 0;
                        uint32_t RenamedMaps = 0;
                        uint32_t RenamingErrors = 0;

                        for( std::filesystem::recursive_directory_iterator i( Path ); i != EndIterator; i++ )
                        {
                            if( !is_directory( i->status( ) ) && ( i->path( ).extension( ) == ".w3x" || i->path( ).extension( ) == ".w3m" ) )
                            {
                                AllMapsCounter++;
                                File = converterX.to_bytes( i->path( ).filename( ) );
                                FilePath = converterX.to_bytes( i->path( ) );
                                QFile = QString::fromUtf8( File.c_str( ) );

                                if( QFile.size( ) <= 39 )
                                    continue;

                                QFilePath = QString::fromUtf8( FilePath.c_str( ) );

                                if( QFilePath.size( ) < QFile.size( ) )
                                    continue;

                                FileNameEnd = 0;
                                QFileSizeTest = QFile.size( ) - 1;

                                for( uint32_t n = QFileSizeTest; n < QFile.size( ); n-- )
                                {
                                    if( QFile[n] == "." )
                                    {
                                        FileNameEnd = n;
                                        break;
                                    }
                                    else if ( n == 0 )
                                        break;
                                }

                                if( FileNameEnd < 10 || ( QFileSizeTest - FileNameEnd ) > 24 )
                                    FileNameEnd = QFileSizeTest;

                                QFileExt = QString( );

                                if( FileNameEnd < QFileSizeTest )
                                    QFileExt = QFile.mid( FileNameEnd );

                                QFileExtSize = QFileExt.size( );
                                QFileNewName = QFile.mid( 0, 39 - QFileExtSize ) + QFileExt;
                                QFileNewNameTest = QFileNewName.mid( 0, 35 - QFileExtSize );
                                FileNewName = QFileNewName.toStdString( );
                                is = string( );
                                iqs = QString( );
                                in = 1;

                                while( UTIL_FileExists( FilePath.substr( 0, FilePath.size( ) - File.size( ) ) + FileNewName ) && in <= 99 )
                                {
                                    if( in == 10 )
                                        QFileNewNameTest = QFileNewNameTest.mid( 0, QFileNewNameTest.size( ) - 1 );

                                    is = " (" + UTIL_ToString( in ) + ")";
                                    iqs = QString::fromUtf8( is.c_str( ) );
                                    QFileNewName = QFileNewNameTest + iqs + QFileExt;
                                    FileNewName = QFileNewName.toStdString( );
                                    in++;
                                }

                                if( in >= 100 )
                                {
                                    RenamingErrors++;
                                    CONSOLE_Print( m_ServerAliasWithUserName + " AutoRenameMap: ошибка при переименовании файла [" + FilePath + "]" );
                                }
                                else if( UTIL_FileRename( FilePath, FilePath.substr( 0, FilePath.size( ) - File.size( ) ) + FileNewName ) )
                                {
                                    CONSOLE_Print( m_ServerAliasWithUserName + " AutoRenameMap: файл [" + File + "] успешно переименован в [" + FileNewName + "]" );
                                    FilePath = FilePath.substr( 0, FilePath.size( ) - File.size( ) ) + FileNewName;
                                    File = FileNewName;
                                    RenamedMaps++;
                                }
                                else
                                {
                                    RenamingErrors++;
                                    CONSOLE_Print( m_ServerAliasWithUserName + " AutoRenameMap: ошибка при переименовании файла [" + FilePath + "]" );
                                }
                            }
                        }

                        if( RenamedMaps > 0 || RenamingErrors > 0 )
                            QueueChatCommand( "Карт всего: [" + UTIL_ToString( AllMapsCounter ) + "], успешно переименовано проблемных карт: [" + UTIL_ToString( RenamedMaps ) + "], ошибок переименования: [" + UTIL_ToString( RenamingErrors ) + "]", User, Whisper );
                        else
                            QueueChatCommand( "Карт всего: [" + UTIL_ToString( AllMapsCounter ) + "], проблемных карт не найдено.", User, Whisper );
                    }
                    catch( const exception &ex )
                    {
                        CONSOLE_Print( m_ServerAliasWithUserName + " error listing maps - caught exception [" + ex.what( ) + "]" );
                        QueueChatCommand( gLanguage->ErrorListingMaps( ), User, Whisper );
                    }
                }

                //
                // !MAP (load map file)
                // !LOAD (load config file)
                // !КАРТА
                //

                else if( Command == "map" || Command == "карта" || Command == "load" )
                {
                    if( Payload.empty( ) )
                        QueueChatCommand( gLanguage->CurrentlyLoadedMapCFGIs( m_UNM->m_Map->GetCFGFile( ) ), User, Whisper );
                    else if( Command == "map" || Command == "карта" )
                    {
                        try
                        {
                            std::filesystem::path MapPath( m_UNM->m_MapPath );
                            string Pattern = Payload;
                            Pattern = UTIL_StringToLower( Pattern );

                            if( !std::filesystem::exists( MapPath ) )
                            {
                                CONSOLE_Print( m_ServerAliasWithUserName + " error listing maps - map path doesn't exist" );
                                QueueChatCommand( gLanguage->ErrorListingMaps( ), User, Whisper );
                            }
                            else
                            {
                                string FilePath;
                                string File;
                                string Stem;
                                string FoundMaps;
                                uint32_t Matches = 0;
                                m_UNM->MapsScan( false, Pattern, FilePath, File, Stem, FoundMaps, Matches );

                                if( Matches == 0 )
                                    QueueChatCommand( gLanguage->NoMapsFound( ), User, Whisper );
                                else if( Matches == 1 )
                                {
                                    m_UNM->MapLoad( FilePath, File, Stem, m_ServerAliasWithUserName, false );
                                    QueueChatCommand( gLanguage->LoadingConfigFile( File ), User, Whisper );
                                }
                                else
                                    QueueChatCommand( gLanguage->FoundMaps( FoundMaps ), User, true );
                            }
                        }
                        catch( const exception &ex )
                        {
                            CONSOLE_Print( m_ServerAliasWithUserName + " error listing maps - caught exception [" + ex.what( ) + "]" );
                            QueueChatCommand( gLanguage->ErrorListingMaps( ), User, Whisper );
                        }
                    }
                    else
                    {
                        try
                        {
                            std::filesystem::path MapCFGPath( m_UNM->m_MapCFGPath );
                            string Pattern = Payload;
                            Pattern = UTIL_StringToLower( Pattern );

                            if( !std::filesystem::exists( MapCFGPath ) )
                            {
                                CONSOLE_Print( m_ServerAliasWithUserName + " error listing map configs - map config path doesn't exist" );
                                QueueChatCommand( gLanguage->ErrorListingMapConfigs( ), User, Whisper );
                            }
                            else
                            {
                                string FilePath;
                                string File;
                                string Stem;
                                string FoundMapConfigs;
                                uint32_t Matches = 0;
                                m_UNM->MapsScan( true, Pattern, FilePath, File, Stem, FoundMapConfigs, Matches );

                                if( Matches == 0 )
                                    QueueChatCommand( gLanguage->NoMapConfigsFound( ), User, Whisper );
                                else if( Matches == 1 )
                                {
                                    QueueChatCommand( gLanguage->LoadingConfigFile( File ), User, Whisper );
                                    m_UNM->MapLoad( FilePath, File, Stem, string( ), true );
                                }
                                else
                                    QueueChatCommand( gLanguage->FoundMapConfigs( FoundMapConfigs ), User, true );
                            }
                        }
                        catch( const exception &ex )
                        {
                            CONSOLE_Print( m_ServerAliasWithUserName + " error listing map configs - caught exception [" + ex.what( ) + "]" );
                            QueueChatCommand( gLanguage->ErrorListingMapConfigs( ), User, Whisper );
                        }
                    }
                }

                //
                // !DELMAP (delete map file)
                // !DELETEMAP
                // !REMOVEMAP
                // !DMAP
                // !MAPDEL
                // !MAPDELETE
                // !MAPD
                // !MAPREMOVE
                //

                else if( Command == "delmap" || Command == "deletemap" || Command == "removemap" || Command == "dmap" || Command == "mapdel" || Command == "mapdelete" || Command == "mapd" || Command == "mapremove" )
                {
                    if( Payload.empty( ) )
                        QueueChatCommand( gLanguage->CurrentlyLoadedMapCFGIs( m_UNM->m_Map->GetCFGFile( ) ), User, Whisper );
                    else
                    {
                        try
                        {
                            std::filesystem::path MapPath( m_UNM->m_MapPath );
                            string Pattern = Payload;
                            Pattern = UTIL_StringToLower( Pattern );

                            if( !std::filesystem::exists( MapPath ) )
                            {
                                CONSOLE_Print( m_ServerAliasWithUserName + " error listing maps - map path doesn't exist" );
                                QueueChatCommand( gLanguage->ErrorListingMaps( ), User, Whisper );
                            }
                            else
                            {
                                string FilePath;
                                string File;
                                string Stem;
                                string FoundMaps;
                                uint32_t Matches = 0;
                                m_UNM->MapsScan( false, Pattern, FilePath, File, Stem, FoundMaps, Matches );

                                if( Matches == 0 )
                                    QueueChatCommand( gLanguage->NoMapsFound( ), User, Whisper );
                                else if( Matches == 1 )
                                {
                                    std::wstring WFilePath = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(FilePath);

                                    if( _wremove( WFilePath.c_str( ) ) != 0 )
                                        QueueChatCommand( "Error deleting " + FilePath, User, Whisper );
                                    else
                                        QueueChatCommand( "Map " + FilePath + " deleted!", User, Whisper );
                                }
                                else
                                    QueueChatCommand( gLanguage->FoundMaps( FoundMaps ), User, Whisper );
                            }
                        }
                        catch( const exception &ex )
                        {
                            CONSOLE_Print( m_ServerAliasWithUserName + " error listing maps - caught exception [" + ex.what( ) + "]" );
                            QueueChatCommand( gLanguage->ErrorListingMaps( ), User, Whisper );
                        }
                    }
                }

                //
                // !PREFIX
                // !CHAR
                // !ПРЕФИКС
                //

                else if( Command == "prefix" || Command == "char" || Command == "префикс" )
                {
                    if( !m_UNM->m_AppleIcon )
                    {
                        QueueChatCommand( "Команда отключена администратором", User, Whisper );
                        return;
                    }

                    if( Payload.empty( ) || PayloadLower == "check" )
                    {
                        if( Command == "prefix" )
                        {
                            if( m_UNM->m_GameNameChar.empty( ) )
                                QueueChatCommand( "Префикс не задан (" + CommandTrigger + "prefix list - список префиксов)", User, Whisper );
                            else
                                QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar + " (" + CommandTrigger + "prefix list - список префиксов)", User, Whisper );
                        }
                        else if( Command == "char" )
                        {
                            if( m_UNM->m_GameNameChar.empty( ) )
                                QueueChatCommand( "Префикс не задан (" + CommandTrigger + "char list - список префиксов)", User, Whisper );
                            else
                                QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar + " (" + CommandTrigger + "char list - список префиксов)", User, Whisper );
                        }
                        else if( Command == "префикс" )
                        {
                            if( m_UNM->m_GameNameChar.empty( ) )
                                QueueChatCommand( "Префикс не задан (" + CommandTrigger + "префикс <список> - список префиксов)", User, Whisper );
                            else
                                QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar + " (" + CommandTrigger + "префикс <список> - список префиксов)", User, Whisper );
                        }
                    }
                    else if( PayloadLower == "off" || PayloadLower == "disable" || PayloadLower == "убрать" || PayloadLower == "0" )
                    {
                        m_UNM->m_GameNameChar = string( );
                        QueueChatCommand( "Префикс удален", User, Whisper );
                    }
                    else if( PayloadLower == "1" )
                    {
                        if( m_UNM->m_GameNameChar == "♠" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "♠";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "2" )
                    {
                        if( m_UNM->m_GameNameChar == "♣" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "♣";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "3" )
                    {
                        if( m_UNM->m_GameNameChar == "♥" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "♥";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "4" )
                    {
                        if( m_UNM->m_GameNameChar == "♦" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "♦";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "5" )
                    {
                        if( m_UNM->m_GameNameChar == "◊" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "◊";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "6" )
                    {
                        if( m_UNM->m_GameNameChar == "○" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "○";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "7" )
                    {
                        if( m_UNM->m_GameNameChar == "●" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "●";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "8" )
                    {
                        if( m_UNM->m_GameNameChar == "◄" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "◄";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "9" )
                    {
                        if( m_UNM->m_GameNameChar == "▼" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "▼";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "10" )
                    {
                        if( m_UNM->m_GameNameChar == "►" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "►";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "11" )
                    {
                        if( m_UNM->m_GameNameChar == "▲" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "▲";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "12" )
                    {
                        if( m_UNM->m_GameNameChar == "■" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "■";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "13" )
                    {
                        if( m_UNM->m_GameNameChar == "▪" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "▪";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "14" )
                    {
                        if( m_UNM->m_GameNameChar == "▫" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "▫";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "15" )
                    {
                        if( m_UNM->m_GameNameChar == "¤" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "¤";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "16" )
                    {
                        if( m_UNM->m_GameNameChar == "£" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "£";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "17" )
                    {
                        if( m_UNM->m_GameNameChar == "§" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "§";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "18" )
                    {
                        if( m_UNM->m_GameNameChar == "→" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "→";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "19" )
                    {
                        if( m_UNM->m_GameNameChar == "♀" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "♀";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "20" )
                    {
                        if( m_UNM->m_GameNameChar == "♂" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "♂";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "21" )
                    {
                        if( m_UNM->m_GameNameChar == "æ" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "æ";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "22" )
                    {
                        if( m_UNM->m_GameNameChar == "ζ" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "ζ";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "23" )
                    {
                        if( m_UNM->m_GameNameChar == "☻" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "☻";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "24" )
                    {
                        if( m_UNM->m_GameNameChar == "☺" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "☺";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "25" )
                    {
                        if( m_UNM->m_GameNameChar == "©" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "©";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else if( PayloadLower == "26" )
                    {
                        if( m_UNM->m_GameNameChar == "®" )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = "®";
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                    else
                        QueueChatCommand( "1: ♠ 2: ♣ 3: ♥ 4: ♦ 5: ◊ 6: ○ 7: ● 8: ◄ 9: ▼ 10: ► 11: ▲ 12: ■ 13: ▪ 14: ▫ 15: ¤ 16: £ 17: § 18: → 19: ♀ 20: ♂21: æ 22: ζ 23: ☻ 24:☺ 25:© 26:®", User, Whisper );
                }

                //
                // !PREFIXCUSTOM
                // !CHARCUSTOM
                // !CUSTOMPREFIX
                // !CUSTOMCHAR
                // !SETPREFIX
                // !SETCHAR
                //

                else if( Command == "prefixcustom" || Command == "charcustom" || Command == "customprefix" || Command == "customchar" || Command == "setprefix" || Command == "setchar" )
                {
                    if( !m_UNM->m_AppleIcon )
                    {
                        QueueChatCommand( "Команда отключена администратором", User, Whisper );
                        return;
                    }

                    if( Payload.empty( ) || PayloadLower == "check" )
                    {
                        if( m_UNM->m_GameNameChar.empty( ) )
                            QueueChatCommand( "Префикс не задан", User, Whisper );
                        else
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                    }
                    else if( PayloadLower == "off" || PayloadLower == "disable" || PayloadLower == "убрать" || PayloadLower == "0" )
                    {
                        m_UNM->m_GameNameChar = string( );
                        QueueChatCommand( "Префикс удален", User, Whisper );
                    }
                    else
                    {
                        if( Payload == m_UNM->m_GameNameChar )
                            QueueChatCommand( "Текущий префикс: " + m_UNM->m_GameNameChar, User, Whisper );
                        else
                        {
                            m_UNM->m_GameNameChar = Payload;
                            QueueChatCommand( "Префикс изменен: " + m_UNM->m_GameNameChar, User, Whisper );
                        }
                    }
                }

                //
                // !PRIV (host private game)
                // !PR
                // !PRI
                //

                else if( ( Command == "priv" || Command == "pr" || Command == "pri" ) && !Payload.empty( ) )
                {
                    string GameName = Payload;
                    GameName = UTIL_SubStrFix( GameName, 0, 31 );

                    if( m_UNM->m_FixGameNameForIccup && ( GameName.find( "aptb" ) != string::npos || GameName.find( "rdtb" ) != string::npos || GameName.find( "artb" ) != string::npos || GameName.find( "sdtb" ) != string::npos ) )
                    {
                        QueueChatCommand( "Название игры не может содержать 'aptb' 'rdtb' 'artb' 'sdtb'", User, Whisper );
                        return;
                    }

                    if( Command == "pri" )
                        m_UNM->CreateGame( m_UNM->m_Map, static_cast<unsigned char>(m_UNM->m_gamestateinhouse), false, GameName, User, User, m_Server, Whisper, string( ) );
                    else
                        m_UNM->CreateGame( m_UNM->m_Map, GAME_PRIVATE, false, GameName, User, User, m_Server, Whisper, string( ) );
                }

                //
                // !PUB (host public game)
                // !P
                //

                else if( Command == "pub" || Command == "p" )
                {
                    if( m_UNM->m_LastGameName.empty( ) && Payload.empty( ) )
                    {
                        QueueChatCommand( "Укажите название игры!", User, Whisper );
                        return;
                    }

                    if( Payload.size( ) < m_UNM->m_MinLengthGN && !Payload.empty( ) )
                    {
                        QueueChatCommand( "Слишком короткое название (минимум " + UTIL_ToString( m_UNM->m_MinLengthGN ) + " симв.)", User, Whisper );
                        return;
                    }

                    string GameName;

                    if( m_UNM->m_AppleIcon )
                    {
                        if( m_UNM->m_GameNameChar.empty( ) )
                        {
                            if( Payload.empty( ) )
                                GameName = m_UNM->m_LastGameName;
                            else
                                GameName = Payload;
                        }
                        else
                        {
                            if( Payload.empty( ) )
                            {
                                if( m_UNM->m_TempGameNameChar.empty( ) )
                                    GameName = m_UNM->m_GameNameChar + " " + m_UNM->m_LastGameName;
                                else
                                {
                                    if( m_UNM->m_TempGameNameChar == m_UNM->m_GameNameChar )
                                        GameName = m_UNM->m_LastGameName;
                                    else
                                    {
                                        QueueChatCommand( "Укажите название игры!", User, Whisper );
                                        return;
                                    }
                                }
                            }
                            else
                                GameName = m_UNM->m_GameNameChar + " " + Payload;
                        }
                    }
                    else
                        GameName = ( Payload.empty( ) ) ? m_UNM->m_LastGameName : Payload;

                    GameName = UTIL_SubStrFix( GameName, 0, 31 );

                    if( m_UNM->m_FixGameNameForIccup && ( GameName.find( "aptb" ) != string::npos || GameName.find( "rdtb" ) != string::npos || GameName.find( "artb" ) != string::npos || GameName.find( "sdtb" ) != string::npos ) )
                    {
                        QueueChatCommand( "Название игры не может содержать 'aptb' 'rdtb' 'artb' 'sdtb'", User, Whisper );
                        return;
                    }

                    string GameNr = string( );
                    uint32_t idx = 0;
                    uint32_t Nr = 0;

                    if( Payload.empty( ) )
                    {
                        string s = m_UNM->GetRehostChar( );

                        if( m_UNM->m_SlotInfoInGameName == 1 )
                        {
                            if( GameName.find( " [10/10]" ) != string::npos || GameName.find( " [10/11]" ) != string::npos || GameName.find( " [10/12]" ) != string::npos || GameName.find( " [11/10]" ) != string::npos || GameName.find( " [11/11]" ) != string::npos || GameName.find( " [11/12]" ) != string::npos || GameName.find( " [12/10]" ) != string::npos || GameName.find( " [12/11]" ) != string::npos || GameName.find( " [12/12]" ) != string::npos )
                                GameName = GameName.substr( 0, GameName.size( ) - 8 );
                            else if( GameName.find( " [0/10]" ) != string::npos || GameName.find( " [0/11]" ) != string::npos || GameName.find( " [0/12]" ) != string::npos || GameName.find( " [1/10]" ) != string::npos || GameName.find( " [1/11]" ) != string::npos || GameName.find( " [1/12]" ) != string::npos || GameName.find( " [2/10]" ) != string::npos || GameName.find( " [2/11]" ) != string::npos || GameName.find( " [2/12]" ) != string::npos || GameName.find( " [3/10]" ) != string::npos || GameName.find( " [3/11]" ) != string::npos || GameName.find( " [3/12]" ) != string::npos || GameName.find( " [4/10]" ) != string::npos || GameName.find( " [4/11]" ) != string::npos || GameName.find( " [4/12]" ) != string::npos || GameName.find( " [5/10]" ) != string::npos || GameName.find( " [5/11]" ) != string::npos || GameName.find( " [5/12]" ) != string::npos || GameName.find( " [6/10]" ) != string::npos || GameName.find( " [6/11]" ) != string::npos || GameName.find( " [6/12]" ) != string::npos || GameName.find( " [7/10]" ) != string::npos || GameName.find( " [7/11]" ) != string::npos || GameName.find( " [7/12]" ) != string::npos || GameName.find( " [8/10]" ) != string::npos || GameName.find( " [8/11]" ) != string::npos || GameName.find( " [8/12]" ) != string::npos || GameName.find( " [9/10]" ) != string::npos || GameName.find( " [9/11]" ) != string::npos || GameName.find( " [9/12]" ) != string::npos || GameName.find( " [10/1]" ) != string::npos || GameName.find( " [10/2]" ) != string::npos || GameName.find( " [10/3]" ) != string::npos || GameName.find( " [10/4]" ) != string::npos || GameName.find( " [10/5]" ) != string::npos || GameName.find( " [10/6]" ) != string::npos || GameName.find( " [10/7]" ) != string::npos || GameName.find( " [10/8]" ) != string::npos || GameName.find( " [10/9]" ) != string::npos || GameName.find( " [11/1]" ) != string::npos || GameName.find( " [11/2]" ) != string::npos || GameName.find( " [11/3]" ) != string::npos || GameName.find( " [11/4]" ) != string::npos || GameName.find( " [11/5]" ) != string::npos || GameName.find( " [11/6]" ) != string::npos || GameName.find( " [11/7]" ) != string::npos || GameName.find( " [11/8]" ) != string::npos || GameName.find( " [11/9]" ) != string::npos || GameName.find( " [12/1]" ) != string::npos || GameName.find( " [12/2]" ) != string::npos || GameName.find( " [12/3]" ) != string::npos || GameName.find( " [12/4]" ) != string::npos || GameName.find( " [12/5]" ) != string::npos || GameName.find( " [12/6]" ) != string::npos || GameName.find( " [12/7]" ) != string::npos || GameName.find( " [12/8]" ) != string::npos || GameName.find( " [12/9]" ) != string::npos )
                                GameName = GameName.substr( 0, GameName.size( ) - 7 );
                            else if( GameName.find( " [0/1]" ) != string::npos || GameName.find( " [0/2]" ) != string::npos || GameName.find( " [0/3]" ) != string::npos || GameName.find( " [0/4]" ) != string::npos || GameName.find( " [0/5]" ) != string::npos || GameName.find( " [0/6]" ) != string::npos || GameName.find( " [0/7]" ) != string::npos || GameName.find( " [0/8]" ) != string::npos || GameName.find( " [0/9]" ) != string::npos || GameName.find( " [1/1]" ) != string::npos || GameName.find( " [1/2]" ) != string::npos || GameName.find( " [1/3]" ) != string::npos || GameName.find( " [1/4]" ) != string::npos || GameName.find( " [1/5]" ) != string::npos || GameName.find( " [1/6]" ) != string::npos || GameName.find( " [1/7]" ) != string::npos || GameName.find( " [1/8]" ) != string::npos || GameName.find( " [1/9]" ) != string::npos || GameName.find( " [2/1]" ) != string::npos || GameName.find( " [2/2]" ) != string::npos || GameName.find( " [2/3]" ) != string::npos || GameName.find( " [2/4]" ) != string::npos || GameName.find( " [2/5]" ) != string::npos || GameName.find( " [2/6]" ) != string::npos || GameName.find( " [2/7]" ) != string::npos || GameName.find( " [2/8]" ) != string::npos || GameName.find( " [2/9]" ) != string::npos || GameName.find( " [3/1]" ) != string::npos || GameName.find( " [3/2]" ) != string::npos || GameName.find( " [3/3]" ) != string::npos || GameName.find( " [3/4]" ) != string::npos || GameName.find( " [3/5]" ) != string::npos || GameName.find( " [3/6]" ) != string::npos || GameName.find( " [3/7]" ) != string::npos || GameName.find( " [3/8]" ) != string::npos || GameName.find( " [3/9]" ) != string::npos || GameName.find( " [4/1]" ) != string::npos || GameName.find( " [4/2]" ) != string::npos || GameName.find( " [4/3]" ) != string::npos || GameName.find( " [4/4]" ) != string::npos || GameName.find( " [4/5]" ) != string::npos || GameName.find( " [4/6]" ) != string::npos || GameName.find( " [4/7]" ) != string::npos || GameName.find( " [4/8]" ) != string::npos || GameName.find( " [4/9]" ) != string::npos || GameName.find( " [5/1]" ) != string::npos || GameName.find( " [5/2]" ) != string::npos || GameName.find( " [5/3]" ) != string::npos || GameName.find( " [5/4]" ) != string::npos || GameName.find( " [5/5]" ) != string::npos || GameName.find( " [5/6]" ) != string::npos || GameName.find( " [5/7]" ) != string::npos || GameName.find( " [5/8]" ) != string::npos || GameName.find( " [5/9]" ) != string::npos || GameName.find( " [6/1]" ) != string::npos || GameName.find( " [6/2]" ) != string::npos || GameName.find( " [6/3]" ) != string::npos || GameName.find( " [6/4]" ) != string::npos || GameName.find( " [6/5]" ) != string::npos || GameName.find( " [6/6]" ) != string::npos || GameName.find( " [6/7]" ) != string::npos || GameName.find( " [6/8]" ) != string::npos || GameName.find( " [6/9]" ) != string::npos || GameName.find( " [7/1]" ) != string::npos || GameName.find( " [7/2]" ) != string::npos || GameName.find( " [7/3]" ) != string::npos || GameName.find( " [7/4]" ) != string::npos || GameName.find( " [7/5]" ) != string::npos || GameName.find( " [7/6]" ) != string::npos || GameName.find( " [7/7]" ) != string::npos || GameName.find( " [7/8]" ) != string::npos || GameName.find( " [7/9]" ) != string::npos || GameName.find( " [8/1]" ) != string::npos || GameName.find( " [8/2]" ) != string::npos || GameName.find( " [8/3]" ) != string::npos || GameName.find( " [8/4]" ) != string::npos || GameName.find( " [8/5]" ) != string::npos || GameName.find( " [8/6]" ) != string::npos || GameName.find( " [8/7]" ) != string::npos || GameName.find( " [8/8]" ) != string::npos || GameName.find( " [8/9]" ) != string::npos || GameName.find( " [9/1]" ) != string::npos || GameName.find( " [9/2]" ) != string::npos || GameName.find( " [9/3]" ) != string::npos || GameName.find( " [9/4]" ) != string::npos || GameName.find( " [9/5]" ) != string::npos || GameName.find( " [9/6]" ) != string::npos || GameName.find( " [9/7]" ) != string::npos || GameName.find( " [9/8]" ) != string::npos || GameName.find( " [9/9]" ) != string::npos )
                                GameName = GameName.substr( 0, GameName.size( ) - 6 );
                        }

                        idx = GameName.length( ) - 1;

                        if( idx >= 2 )
                        {
                            if( GameName.at( idx - 2 ) == s[0] )
                                idx = idx - 1;
                            else if( GameName.at( idx - 1 ) != s[0] )
                                idx = 0;
                        }

                        // idx = 0, no Game Nr found in gamename

                        if( idx == 0 )
                        {
                            if( GameName.size( ) > 28 )
                                GameName = UTIL_SubStrFix( GameName, 0, 28 );

                            GameNr = "0";
                            GameName = GameName + " " + s[0];
                        }
                        else
                        {
                            GameNr = GameName.substr( idx, GameName.length( ) - idx );
                            GameName = GameName.substr( 0, idx );
                            uint32_t GameNrTest = UTIL_ToUInt32( GameNr );

                            if( GameNrTest >= m_UNM->m_MaxHostCounter && m_UNM->m_MaxHostCounter > 0 )
                                GameNrTest = 1;
                            else
                                GameNrTest = GameNrTest + 1;

                            if( GameName.size( ) + UTIL_ToString( GameNrTest ).size( ) > 31 )
                            {
                                if( GameName.size( ) >= 2 && GameName.substr( GameName.size( ) - 2 ) == " " + s )
                                    GameName.erase( GameName.end( ) - 2, GameName.end( ) );
                                else if( GameName.size( ) >= 1 && GameName.back( ) == s[0] )
                                    GameName.erase( GameName.end( ) - 1 );

                                GameName = UTIL_SubStrFix( GameName, 0, 29 - UTIL_ToString( GameNrTest ).size( ) );
                                GameName = GameName + " " + s[0];
                            }
                        }

                        stringstream SS;
                        SS << GameNr;
                        SS >> Nr;
                        Nr++;

                        if( Nr > m_UNM->m_MaxHostCounter && m_UNM->m_MaxHostCounter > 0 )
                            Nr = 1;

                        GameNr = UTIL_ToString( Nr );
                        GameName = GameName + GameNr;
                    }

                    m_UNM->CreateGame( m_UNM->m_Map, GAME_PUBLIC, false, GameName, User, User, m_Server, Whisper, string( ) );
                    m_UNM->m_TempGameNameChar = m_UNM->m_GameNameChar;

                    if( m_UNM->m_ResetGameNameChar )
                        m_UNM->m_GameNameChar = m_UNM->m_DefaultGameNameChar;
                }

                //
                // !REPLAYS
                // !NOREPLAYS
                // !SAVEREPLAYS
                // !REPLAYSSAVE
                //

                else if( Command == "replays" || Command == "noreplays" || Command == "savereplays" || Command == "replayssave" )
                {
                    m_UNM->m_SaveReplays = !m_UNM->m_SaveReplays;

                    if( m_UNM->m_SaveReplays )
                        QueueChatCommand( "Replay save enabled", User, Whisper );
                    else
                        QueueChatCommand( "Replay save disabled", User, Whisper );
                }

                //
                // !SAYGAME
                // !SG
                //

                else if( ( Command == "saygame" || Command == "sg" ) && !Payload.empty( ) )
                {
                    // extract the game number and the message
                    // e.g. "3 hello everyone" -> game number: "3", message: "hello everyone"

                    uint32_t GameNumber;
                    string Message;
                    stringstream SS;
                    SS << Payload;
                    SS >> GameNumber;

                    if( SS.fail( ) )
                        QueueChatCommand( "bad input #1 to [" + Command + "] command", User, Whisper );
                    else
                    {
                        if( SS.eof( ) )
                            QueueChatCommand( "missing input #2 to [" + Command + "] command", User, Whisper );
                        else
                        {
                            getline( SS, Message );

                            if( Message.size( ) > 0 && Message[0] == ' ' )
                                Message = Message.substr( 1 );

                            if( Message.empty( ) )
                                QueueChatCommand( "missing input #2 to [" + Command + "] command", User, Whisper );
                            else if( GameNumber - 1 < m_UNM->m_Games.size( ) )
                            {
                                m_UNM->m_Games[GameNumber - 1]->SendAllChat( User + ": " + Message );
                                QueueChatCommand( "Ваше сообщение отправлено всем игрокам в игре №" + UTIL_ToString( GameNumber ) + ".", User, Whisper );
                            }
                            else
                                QueueChatCommand( gLanguage->GameNumberDoesntExist( UTIL_ToString( GameNumber ) ), User, Whisper );
                        }
                    }
                }

                //
                // !SAYGAMES
                // !SGS
                //

                else if( ( Command == "saygames" || Command == "sgs" ) && !Payload.empty( ) )
                {
                    if( m_UNM->m_CurrentGame )
                        m_UNM->m_CurrentGame->SendAllChat( Payload );

                    for( vector<CBaseGame *> ::iterator i = m_UNM->m_Games.begin( ); i != m_UNM->m_Games.end( ); i++ )
                        ( *i )->SendAllChat( User + ": " + Payload );

                    QueueChatCommand( "Ваше сообщение отправлено игрокам во всех играх.", User, Whisper );
                }

                //
                // !HOME
                //

                else if( Command == "home" )
                    QueueChatCommand( "/channel " + m_FirstChannel );

                //
                // !RELOAD
                // !RCFG
                // !RELOADCONFING
                // !RELOADCFG
                //

                else if( Command == "reload" || Command == "rcfg" || Command == "reloadconfig" || Command == "reloadcfg" )
                {
                    m_UNM->ReloadConfig( nullptr );
                    QueueChatCommand( "unm.cfg перезагружен!", User, Whisper );
                }

                //
                // !GETNAMES
                // !GNS
                //

                else if( Command == "getnames" || Command == "gns" )
                {
                    string GameList = "Lobby: ";

                    if( m_UNM->m_CurrentGame )
                        GameList += m_UNM->m_CurrentGame->GetGameName( );
                    else
                        GameList += "-";

                    GameList += "; Started:";
                    uint32_t Index = 0;

                    while( Index < m_UNM->m_Games.size( ) )
                    {
                        GameList += " [" + UTIL_ToString( Index + 1 ) + "] " + m_UNM->m_Games[Index]->GetGameName( );
                        Index++;
                    }

                    QueueChatCommand( GameList, User, Whisper );
                }

                //
                // !GET
                //

                else if( Command == "get" && !Payload.empty( ) )
                {
                    string line;
                    uint32_t idx = 0;
                    uint32_t commidx = 0;
                    vector<string> newcfg;
                    string Value = string( );
                    string comment = string( );
                    string VarName = Payload;
                    bool varfound = false;
                    bool firstpart = false;
                    bool commentfound = false;

                    // read config file
                    std::ifstream myfile( "unm.cfg" );

                    if( myfile.is_open( ) )
                    {
                        while( !myfile.eof( ) )
                        {
                            getline( myfile, line );
                            line = UTIL_FixFileLine( line );

                            if( line.substr( 0, VarName.size( ) + 1 ) == VarName + " " )
                            {
                                string::size_type Split = line.find( "=" );

                                if( Split != string::npos )
                                {
                                    string::size_type ValueStart = line.find_first_not_of( " ", Split + 1 );
                                    string::size_type ValueEnd = line.size( );

                                    if( ValueStart != string::npos )
                                        Value = line.substr( ValueStart, ValueEnd - ValueStart );

                                    varfound = true;

                                    // search for variable comment
                                    if( idx > 2 )
                                    {
                                        commidx = idx - 1;

                                        if( newcfg[commidx].front( ) == '#' )
                                            firstpart = true;
                                        else if( newcfg[commidx - 1].front( ) == '#' )
                                        {
                                            commidx = idx - 2;

                                            if( commidx >= 1 )
                                                firstpart = true;
                                        }

                                        if( firstpart && ( newcfg[commidx - 1].front( ) == ' ' || newcfg[commidx - 1].empty( ) ) )
                                        {
                                            commentfound = true;
                                            comment = newcfg[commidx];
                                        }
                                    }
                                }
                            }

                            newcfg.push_back( line );
                            idx++;
                        }

                        myfile.close( );
                    }

                    if( !varfound || VarName.find( "password" ) != string::npos )
                    {
                        QueueChatCommand( VarName + " is not a config variable!", User, Whisper );
                        return;
                    }

                    string ValComment = Value;

                    if( commentfound )
                    {
                        ValComment += " - " + comment;
                        ValComment = ValComment.substr( 0, 599-Payload.size( ) );
                    }

                    QueueChatCommand( Payload + "=" + ValComment, User, Whisper );

                }

                //
                // !SET
                //

                else if( Command == "set" && !Payload.empty( ) )
                {
                    // get the var and value from payload, ex: !set bot_log g.txt

                    string VarName;
                    string Value = string( );
                    stringstream SS;
                    SS << Payload;
                    SS >> VarName;

                    if( !SS.eof( ) )
                    {
                        getline( SS, Value );

                        if( Value.size( ) > 0 && Value[0] == ' ' )
                            Value = Value.substr( 1 );
                    }

                    if( VarName.empty( ) || Value.empty( ) )
                    {
                        QueueChatCommand( gLanguage->IncorrectCommandSyntax( ), User, Whisper );
                        return;
                    }

                    string line;
                    uint32_t idx = 0;
                    uint32_t commidx = 0;
                    vector<string> newcfg;
                    string comment;
                    bool varfound = false;
                    bool firstpart = false;
                    bool commentfound = false;

                    // reading the old config file and replacing the variable if it exists
                    std::ifstream myfile( "unm.cfg" );

                    if( myfile.is_open( ) )
                    {
                        while( !myfile.eof( ) )
                        {
                            getline( myfile, line );
                            line = UTIL_FixFileLine( line );

                            if( line.substr( 0, VarName.size( ) + 1 ) == VarName + " " )
                            {
                                newcfg.push_back( VarName + " = " + Value );
                                varfound = true;

                                // search for variable comment
                                if( idx > 2 )
                                {
                                    commidx = idx - 1;

                                    if( newcfg[commidx].front( ) == '#' )
                                        firstpart = true;
                                    else if( newcfg[commidx - 1].front( ) == '#' )
                                    {
                                        commidx = idx - 2;

                                        if( commidx >= 1 )
                                            firstpart = true;
                                    }

                                    if( firstpart && ( newcfg[commidx - 1].front( ) == ' ' || newcfg[commidx - 1].empty( ) ) )
                                    {
                                        commentfound = true;
                                        comment = newcfg[commidx];
                                    }
                                }
                            }
                            else
                                newcfg.push_back( line );

                            idx++;
                        }

                        myfile.close( );
                    }

                    if( !varfound || VarName.find( "password" ) != string::npos )
                    {
                        QueueChatCommand( VarName + " is not a config variable!", User, Whisper );
                        return;
                    }

                    string ValComment = Value;

                    if( commentfound )
                    {
                        ValComment += " - " + comment;
                        ValComment = ValComment.substr( 0, 599-VarName.size( ) );
                    }

                    QueueChatCommand( VarName + "=" + ValComment, User, Whisper );

                    while( !newcfg.empty( ) && newcfg.back( ).empty( ) )
                        newcfg.erase( newcfg.end( ) - 1 );

                    // writing new config file

                    std::ofstream myfile2;
                    string unmcfg = "unm.cfg";
#ifdef WIN32
                    myfile2.open( UTIL_UTF8ToCP1251( unmcfg.c_str( ) ) );
#else
                    myfile2.open( unmcfg );
#endif
                    for( uint32_t i = 0; i < newcfg.size( ); i++ )
                        myfile2 << newcfg[i] << endl;

                    myfile2.close( );
                    m_UNM->ReloadConfig( nullptr );
                }

                //
                // !SETNEW
                //

                else if( Command == "setnew" && !Payload.empty( ) )
                {
                    // get the var and value from payload, ex: !set bot_log g.txt

                    string VarName;
                    string Value = string( );
                    stringstream SS;
                    SS << Payload;
                    SS >> VarName;

                    if( !SS.eof( ) )
                    {
                        getline( SS, Value );

                        if( Value.size( ) > 0 && Value[0] == ' ' )
                            Value = Value.substr( 1 );
                    }

                    if( VarName.empty( ) || Value.empty( ) || VarName.find( "password" ) != string::npos )
                    {
                        QueueChatCommand( gLanguage->IncorrectCommandSyntax( ), User, Whisper );
                        return;
                    }

                    string line;
                    uint32_t idx = 0;
                    uint32_t commidx = 0;
                    vector<string> newcfg;
                    string comment;
                    bool varfound = false;
                    bool firstpart = false;
                    bool commentfound = false;

                    // reading the old config file and replacing the variable if it exists
                    std::ifstream myfile( "unm.cfg" );

                    if( myfile.is_open( ) )
                    {
                        while( !myfile.eof( ) )
                        {
                            getline( myfile, line );
                            line = UTIL_FixFileLine( line );

                            if( line.substr( 0, VarName.size( ) + 1 ) == VarName + " " )
                            {
                                newcfg.push_back( VarName + " = " + Value );
                                varfound = true;

                                // search for variable comment
                                if( idx > 2 )
                                {
                                    commidx = idx - 1;

                                    if( newcfg[commidx].front( ) == '#' )
                                        firstpart = true;
                                    else if( newcfg[commidx - 1].front( ) == '#' )
                                    {
                                        commidx = idx - 2;

                                        if( commidx >= 1 )
                                            firstpart = true;
                                    }

                                    if( firstpart && ( newcfg[commidx - 1].front( ) == ' ' || newcfg[commidx - 1].empty( ) ) )
                                    {
                                        commentfound = true;
                                        comment = newcfg[commidx];
                                    }
                                }
                            }
                            else
                                newcfg.push_back( line );

                            idx++;
                        }

                        myfile.close( );
                    }

                    if( !varfound )
                    {
                        newcfg.insert( newcfg.begin( ), VarName + " = " + Value );
                        newcfg.insert( newcfg.begin( ), string( ) );
                    }

                    string ValComment = Value;

                    if( commentfound )
                    {
                        ValComment += " - " + comment;
                        ValComment = ValComment.substr( 0, 599-VarName.size( ) );
                    }

                    QueueChatCommand( VarName + "=" + ValComment, User, Whisper );

                    while( !newcfg.empty( ) && newcfg.back( ).empty( ) )
                        newcfg.erase( newcfg.end( ) - 1 );

                    // writing new config file

                    std::ofstream myfile2;
                    string unmcfg = "unm.cfg";
#ifdef WIN32
                    myfile2.open( UTIL_UTF8ToCP1251( unmcfg.c_str( ) ) );
#else
                    myfile2.open( unmcfg );
#endif
                    for( uint32_t i = 0; i < newcfg.size( ); i++ )
                        myfile2 << newcfg[i] << endl;

                    myfile2.close( );
                    m_UNM->ReloadConfig( nullptr );
                }

                //
                // !UNHOST
                // !UH
                //

                else if( Command == "unhost" || Command == "uh" )
                {
                    if( m_UNM->m_CurrentGame )
                    {
                        if( m_UNM->m_CurrentGame->GetCountDownStarted( ) )
                            QueueChatCommand( gLanguage->UnableToUnhostGameCountdownStarted( m_UNM->m_CurrentGame->GetDescription( ) ), User, Whisper );
                        else
                        {
                            QueueChatCommand( gLanguage->UnhostingGame( m_UNM->m_CurrentGame->GetDescription( ) ), User, Whisper );
                            m_UNM->m_CurrentGame->SetExiting( true );
                        }
                    }
                    else
                        QueueChatCommand( gLanguage->UnableToUnhostGameNoGameInLobby( ), User, Whisper );
                }

                //
                // !LANGUAGE
                // !LANG
                // !L
                //

                else if( Command == "language" || Command == "lang" || Command == "l" )
                {
                    delete gLanguage;
                    gLanguage = new CLanguage( gLanguageFile );
                    QueueChatCommand( "Language.cfg перезагружен!", User, Whisper );
                }

                //
                // !DATE
                // !TIME
                //

                else if( Command == "date" || Command == "time" )
                {
                    struct tm * timeinfo;
                    char buffer[150];
                    string sDate;
                    time_t Now = time( nullptr );
                    timeinfo = localtime( &Now );
                    strftime( buffer, 150, "Date: %d %B %Y Time: %H:%M:%S", timeinfo );
                    sDate = buffer;
                    QueueChatCommand( sDate, User, true );
                }

                //
                // !GAMES
                // !INFOGAMES
                // !PLAYERS
                //

                else if( Command == "games" || Command == "infogames" || Command == "players" )
                {
                    uint32_t NowOnline = 0;

                    for( uint32_t i = 0; i < m_UNM->m_Games.size( ); i++ )
                        NowOnline = NowOnline + m_UNM->m_Games[i]->GetNumHumanPlayers( );

                    if( m_UNM->m_CurrentGame )
                        NowOnline = NowOnline + m_UNM->m_CurrentGame->GetNumHumanPlayers( );

                    QueueChatCommand( "Активных игр [" + UTIL_ToString( m_UNM->m_Games.size( ) ) + "] → Игроков онлайн [" + UTIL_ToString( NowOnline ) + "]", User, Whisper );
                }

                //
                // !IMAGE
                // !PICTURE
                // !IMG
                // !PIC
                //

                else if( Command == "image" || Command == "picture" || Command == "img" || Command == "pic" )
                {
                    string file;
#ifdef WIN32
                    file = "text_files\\text_images\\";
#else
                    file = "text_files/text_images/";
#endif

                    if( Payload.empty( ) )
                        file += "image.txt";
                    else if( Payload.find_first_not_of( "0123456789" ) != string::npos )
                    {
                        QueueChatCommand( "Некорректный ввод команды, используйте \"" + CommandTrigger + Command + " <ImageNumber>\"", User, Whisper );
                        return;
                    }
                    else
                    {
                        uint32_t filenumber;
                        stringstream SS;
                        SS << Payload;
                        SS >> filenumber;
                        if( SS.fail( ) )
                        {
                            QueueChatCommand( "Некорректный ввод команды, используйте \"" + CommandTrigger + Command + " <ImageNumber>\"", User, Whisper );
                            return;
                        }
                        else
                            file += "image" + UTIL_ToString( filenumber ) + ".txt";
                    }

                    // read from file if available

                    std::ifstream in;
#ifdef WIN32
                    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
                    in.open( file );
#endif
                    if( !in.fail( ) )
                    {
                        // don't print more than 200 lines

                        uint32_t Count = 0;
                        string Line;

                        while( !in.eof( ) && Count < 200 )
                        {
                            getline( in, Line );
                            Line = UTIL_FixFileLine( Line );

                            if( Line.empty( ) )
                            {
                                if( !in.eof( ) )
                                {
                                    ++m_MaxOutPacketsSize;
                                    QueueChatCommand( " " );
                                }
                            }
                            else
                            {
                                ++m_MaxOutPacketsSize;
                                QueueChatCommand( Line );
                            }

                            ++Count;
                        }
                    }
                    else
                        QueueChatCommand( "Не удалось открыть файл [" + file + "]", User, true );

                    in.close( );
                }

                //
                // !WIMAGE
                // !WPICTURE
                // !WIMG
                // !WPIC
                //

                else if( Command == "wimage" || Command == "wpicture" || Command == "wimg" || Command == "wpic" )
                {
                    if( Payload.empty( ) )
                    {
                        QueueChatCommand( "Некорректный ввод команды, используйте \"" + CommandTrigger + Command + " <Username> <ImageNumber>\"", User, Whisper );
                        return;
                    }

                    string file;
#ifdef WIN32
                    file = "text_files\\text_images\\";
#else
                    file = "text_files/text_images/";
#endif

                    string usernametosend;
                    uint32_t filenumber;
                    stringstream SS;
                    SS << Payload;
                    SS >> usernametosend;

                    if( SS.fail( ) )
                    {
                        QueueChatCommand( "Некорректный ввод команды, используйте \"" + CommandTrigger + Command + " <Username> <ImageNumber>\"", User, Whisper );
                        return;
                    }
                    else
                    {
                        if( usernametosend == Payload || ( Payload.size( ) > usernametosend.size( ) && Payload.substr( usernametosend.size( ) ).find_first_not_of( " " ) == string::npos ) )
                            file += "image.txt";
                        else if( SS.eof( ) )
                        {
                            QueueChatCommand( "Некорректный ввод команды, используйте \"" + CommandTrigger + Command + " <Username> <ImageNumber>\"", User, Whisper );
                            return;
                        }
                        else
                        {
                            SS >> filenumber;

                            if( SS.fail( ) )
                            {
                                QueueChatCommand( "Некорректный ввод команды, используйте \"" + CommandTrigger + Command + " <Username> <ImageNumber>\"", User, Whisper );
                                return;
                            }
                            else
                                file += "image" + UTIL_ToString( filenumber ) + ".txt";
                        }
                    }

                    // read from file if available

                    std::ifstream in;
#ifdef WIN32
                    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
                    in.open( file );
#endif
                    if( !in.fail( ) )
                    {
                        // don't print more than 200 lines

                        uint32_t Count = 0;
                        string Line;

                        while( !in.eof( ) && Count < 200 )
                        {
                            getline( in, Line );
                            Line = UTIL_FixFileLine( Line );

                            if( Line.front( ) == ' ' )
                                Line = "|" + Line;

                            if( !Line.empty( ) )
                            {
                                ++m_MaxOutPacketsSize;
                                QueueChatCommand( "/w " + usernametosend + " " + Line );
                            }

                            ++Count;
                        }
                    }
                    else
                        QueueChatCommand( "Не удалось открыть файл [" + file + "]", User, true );

                    in.close( );
                }

                //
                // !BUTILOCHKA
                // !BUT
                // !БУТЫЛОЧКА
                // !БУТ
                //

                else if( Command == "butilochka" || Command == "but" || Command == "бутылочка" || Command == "бут" )
                {
                    vector<string> randoms;
                    string nam1 = User;
                    string nam2 = Payload;
                    string nam3 = string( );
                    string victim = GetPlayerFromNamePartial( Payload );

                    // if no user is specified, randomize one != with the user giving the command

                    if( nam2.empty( ) || victim.empty( ) )
                    {
                        for( uint32_t i = 0; i != m_ChannelUsers.size( ); i++ )
                        {
                            if( m_ChannelUsers[i] != nam1 )
                                randoms.push_back( m_ChannelUsers[i] );
                        }

                        if( randoms.empty( ) )
                        {
                            if( nam1 != m_UserName )
                                randoms.push_back( m_UserName );

                            if( nam1 != m_UNM->m_VirtualHostName )
                                randoms.push_back( m_UNM->m_VirtualHostName );

                            if( randoms.empty( ) )
                                randoms.push_back( m_UserName );
                        }

                        std::random_device rand_dev;
                        std::mt19937 rng( rand_dev( ) );
                        shuffle( randoms.begin( ), randoms.end( ), rng );
                        nam2 = randoms[0];
                        randoms.clear( );
                    }
                    else
                        nam2 = victim;

                    for( uint32_t i = 0; i != m_ChannelUsers.size( ); i++ )
                    {
                        if( m_ChannelUsers[i] != nam1 && m_ChannelUsers[i] != nam2 )
                            randoms.push_back( m_ChannelUsers[i] );
                    }

                    if( randoms.empty( ) )
                    {
                        if( nam1 != m_UserName && nam2 != m_UserName )
                            randoms.push_back( m_UserName );

                        if( nam1 != m_UNM->m_VirtualHostName && nam2 != m_UNM->m_VirtualHostName )
                            randoms.push_back( m_UNM->m_VirtualHostName );

                        if( randoms.empty( ) )
                            randoms.push_back( m_UserName );
                    }

                    std::random_device rand_dev;
                    std::mt19937 rng( rand_dev( ) );
                    shuffle( randoms.begin( ), randoms.end( ), rng );
                    nam3 = randoms[0];
                    string msg = m_UNM->GetBut( );
                    UTIL_Replace( msg, "$USER$", nam1 );
                    UTIL_Replace( msg, "$VICTIM$", nam2 );
                    UTIL_Replace( msg, "$RANDOM$", nam3 );
                    QueueChatCommand( msg );
                }

                //
                // !MARS
                //

                else if( Command == "mars" )
                {
                    vector<string> randoms;
                    string nam1 = User;
                    string nam2 = Payload;
                    string nam3 = string( );
                    string victim = GetPlayerFromNamePartial( Payload );

                    // if no user is specified, randomize one != with the user giving the command

                    if( nam2.empty( ) || victim.empty( ) )
                    {
                        for( uint32_t i = 0; i != m_ChannelUsers.size( ); i++ )
                        {
                            if( m_ChannelUsers[i] != nam1 )
                                randoms.push_back( m_ChannelUsers[i] );
                        }

                        if( randoms.empty( ) )
                        {
                            if( nam1 != m_UserName )
                                randoms.push_back( m_UserName );

                            if( nam1 != m_UNM->m_VirtualHostName )
                                randoms.push_back( m_UNM->m_VirtualHostName );

                            if( randoms.empty( ) )
                                randoms.push_back( m_UserName );
                        }

                        std::random_device rand_dev;
                        std::mt19937 rng( rand_dev( ) );
                        shuffle( randoms.begin( ), randoms.end( ), rng );
                        nam2 = randoms[0];
                        randoms.clear( );
                    }
                    else
                        nam2 = victim;

                    for( uint32_t i = 0; i != m_ChannelUsers.size( ); i++ )
                    {
                        if( m_ChannelUsers[i] != nam1 && m_ChannelUsers[i] != nam2 )
                            randoms.push_back( m_ChannelUsers[i] );
                    }

                    if( randoms.empty( ) )
                    {
                        if( nam1 != m_UserName && nam2 != m_UserName )
                            randoms.push_back( m_UserName );

                        if( nam1 != m_UNM->m_VirtualHostName && nam2 != m_UNM->m_VirtualHostName )
                            randoms.push_back( m_UNM->m_VirtualHostName );

                        if( randoms.empty( ) )
                            randoms.push_back( m_UserName );
                    }

                    std::random_device rand_dev;
                    std::mt19937 rng( rand_dev( ) );
                    shuffle( randoms.begin( ), randoms.end( ), rng );
                    nam3 = randoms[0];
                    string msg = m_UNM->GetMars( );
                    UTIL_Replace( msg, "$USER$", nam1 );
                    UTIL_Replace( msg, "$VICTIM$", nam2 );
                    UTIL_Replace( msg, "$RANDOM$", nam3 );
                    QueueChatCommand( msg );
                }

                //
                // !SLAP
                //

                else if( Command == "slap" )
                {
                    vector<string> randoms;
                    string nam1 = User;
                    string nam2 = Payload;
                    string nam3 = string( );
                    string victim = GetPlayerFromNamePartial( Payload );

                    // if no user is specified, randomize one != with the user giving the command

                    if( nam2.empty( ) || victim.empty( ) )
                    {
                        for( uint32_t i = 0; i != m_ChannelUsers.size( ); i++ )
                        {
                            if( m_ChannelUsers[i] != nam1 )
                                randoms.push_back( m_ChannelUsers[i] );
                        }

                        if( randoms.empty( ) )
                        {
                            if( nam1 != m_UserName )
                                randoms.push_back( m_UserName );

                            if( nam1 != m_UNM->m_VirtualHostName )
                                randoms.push_back( m_UNM->m_VirtualHostName );

                            if( randoms.empty( ) )
                                randoms.push_back( m_UserName );
                        }

                        std::random_device rand_dev;
                        std::mt19937 rng( rand_dev( ) );
                        shuffle( randoms.begin( ), randoms.end( ), rng );
                        nam2 = randoms[0];
                        randoms.clear( );
                    }
                    else
                        nam2 = victim;

                    for( uint32_t i = 0; i != m_ChannelUsers.size( ); i++ )
                    {
                        if( m_ChannelUsers[i] != nam1 && m_ChannelUsers[i] != nam2 )
                            randoms.push_back( m_ChannelUsers[i] );
                    }

                    if( randoms.empty( ) )
                    {
                        if( nam1 != m_UserName && nam2 != m_UserName )
                            randoms.push_back( m_UserName );

                        if( nam1 != m_UNM->m_VirtualHostName && nam2 != m_UNM->m_VirtualHostName )
                            randoms.push_back( m_UNM->m_VirtualHostName );

                        if( randoms.empty( ) )
                            randoms.push_back( m_UserName );
                    }

                    std::random_device rand_dev;
                    std::mt19937 rng( rand_dev( ) );
                    shuffle( randoms.begin( ), randoms.end( ), rng );
                    nam3 = randoms[0];
                    string msg = m_UNM->GetSlap( );
                    UTIL_Replace( msg, "$USER$", nam1 );
                    UTIL_Replace( msg, "$VICTIM$", nam2 );
                    UTIL_Replace( msg, "$RANDOM$", nam3 );
                    QueueChatCommand( msg );
                }

                //
                // !ROLL
                //

                else if( Command == "roll" )
                {
                    if( Payload.empty( ) )
                        QueueChatCommand( User + " rolled " + UTIL_ToString( UTIL_GenerateRandomNumberUInt32( 1, 100 ) ) + " из 100", User, Whisper );
                    else if( Payload.find_first_not_of( "0123456789" ) == string::npos && UTIL_ToUInt64( Payload ) >= 1 )
                        QueueChatCommand( User + " rolled " + UTIL_ToString( UTIL_GenerateRandomNumberUInt64( 1, UTIL_ToUInt64( Payload ) ) ) + " из " + UTIL_ToString( UTIL_ToUInt64( Payload ) ), User, Whisper );
                    else
                        QueueChatCommand( User + " rolled " + UTIL_ToString( UTIL_GenerateRandomNumberUInt32( 1, 100 ) ) + " из 100, for \"" + Payload + "\"", User, Whisper );
                }

                //
                // !УТК
                // !УТКА
                // !DUCK
                // !UTK
                // !YTK
                //

                else if( Command == "утк" || Command == "утка" || Command == "duck" || Command == "utk" || Command == "ytk" )
                {

                    if( PayloadLower == "2" || PayloadLower == "specular" || PayloadLower == "reversed" || PayloadLower == "mirror" )
                    {
                        QueueChatCommand( "    __( ^ )>" );
                        QueueChatCommand( "~(_____)" );
                        QueueChatCommand( "      |_ |_" );
                    }
                    else
                    {
                        QueueChatCommand( "<( ^ )__" );
                        QueueChatCommand( " (_____)~" );
                        QueueChatCommand( "  _/_/" );
                    }
                }

                //
                // !КТУ
                // !УТК2
                // !УТКА2
                // !KCUD
                // !DUCK2
                // !KTU
                // !KTY
                // !UTK2
                // !YTK2
                //

                else if( Command == "кту" || Command == "утк2" || Command == "утка2" || Command == "kcud" || Command == "duck2" || Command == "ktu" || Command == "kty" || Command == "utk2" || Command == "ytk2" )
                {
                    QueueChatCommand( "    __( ^ )>" );
                    QueueChatCommand( "~(_____)" );
                    QueueChatCommand( "      |_ |_" );
                }

                //
                // !МЫШЬ
                // !MOUSE
                //

                else if( Command == "мышь" || Command == "mouse" )
                {
                    if( PayloadLower == "2" || PayloadLower == "specular" || PayloadLower == "reversed" || PayloadLower == "mirror" )
                        QueueChatCommand( "<\"__)~" );
                    else
                        QueueChatCommand( "~(__\">" );
                }

                //
                // !МЫШЬ2
                // !MOUSE2
                //

                else if( Command == "мышь2" || Command == "mouse2" )
                {
                    QueueChatCommand( "<\"__)~" );
                }

                //
                // !ЗАЯЦ
                // !ЗАЙ
                // !ЗАЯ
                // !ЗАЙЧИК
                // !RABBIT
                // !BUNNY
                // !HARE
                //

                else if( Command == "заяц" || Command == "зай" || Command == "зая" || Command == "зайчик" || Command == "rabbit" || Command == "bunny" || Command == "hare" )
                {
                    if( PayloadLower == "2" || PayloadLower == "3" || PayloadLower == "два" || PayloadLower == "три" || PayloadLower == "two" || PayloadLower == "three" )
                    {
                        QueueChatCommand( "---/)/)---(\\.../)---(\\(\\" );
                        QueueChatCommand( "--(':'=)---(=';'=)---(=':')" );
                        QueueChatCommand( "(\")(\")..)-(\").--.(\")-(..(\")(\")" );
                    }
                    else
                    {
                        QueueChatCommand( "(\\__/)" );
                        QueueChatCommand( " (='.'=)" );
                        QueueChatCommand( "(\")_(\")" );
                    }
                }

                //
                // !ЗАЙЦЫ
                // !ЗАИ
                // !ЗАЙИ
                // !ЗАЙЧИКИ
                // !RABBITS
                // !BUNNYS
                // !HARES
                //

                else if( Command == "зайцы" || Command == "заи" || Command == "зайи" || Command == "зайчики" || Command == "rabbits" || Command == "bunnys" || Command == "hares" )
                {
                    QueueChatCommand( "---/)/)---(\\.../)---(\\(\\" );
                    QueueChatCommand( "--(':'=)---(=';'=)---(=':')" );
                    QueueChatCommand( "(\")(\")..)-(\").--.(\")-(..(\")(\")" );
                }

                //
                // !БЕЛКА
                // !БЕЛОЧКА
                // !SQUIRREL
                //

                else if( Command == "белка" || Command == "белочка" || Command == "squirrel" )
                {
                    QueueChatCommand( "( \\" );
                    QueueChatCommand( "(_ \\  ( '>" );
                    QueueChatCommand( "   ) \\/_)=" );
                    QueueChatCommand( "   (_(_ )_" );
                }

                //
                // !КРАБ
                // !КРАБИК
                // !CRAB
                //

                else if( Command == "краб" || Command == "крабик" || Command == "crab" )
                {
                    QueueChatCommand( "(\\/)_0_0_(\\/)" );
                }

                //
                // !VERSION
                // !ABOUT
                // !V
                //

                else if( Command == "version" || Command == "about" || Command == "v" )
                    QueueChatCommand( "UNM bot v" + m_UNM->m_Version + " (created by motherfuunnick)", User, true );
            }
            else
                CONSOLE_Print( m_ServerAliasWithUserName + " юзер [" + User + "] отправил запрос на выполнение команды [" + Message + "]" );
        }
    }
    else if( Event == CBNETProtocol::EID_BROADCAST )
    {
        GUI_Print( 5, Message, string( ), 0 );
        CONSOLE_Print( m_ServerAliasWithUserName + " broadcast: [" + Message + "]" );
    }
    else if( Event == CBNETProtocol::EID_CHANNEL )
    {
        // keep track of current channel so we can rejoin it after hosting a game

        if( !m_UNM->m_LogReduction )
            CONSOLE_Print( m_ServerAliasWithUserName + " joined channel [" + Message + "]" );

        bool changechannel = true;

        for( uint32_t i = 0; i != m_OutPackets3Channel.size( ); i++ )
        {
            if( !m_OutPackets3Channel[i].empty( ) )
            {
                changechannel = false;
                break;
            }
        }

        if( changechannel )
        {
            m_CurrentChannel = Message;
            m_MyChannel = true;
        }

        ChangeChannel( false );
    }
    else if( Event == CBNETProtocol::EID_CHANNELFULL )
    {
        if( !m_UNM->m_LogReduction )
            CONSOLE_Print( m_ServerAliasWithUserName + " channel is full" );
    }
    else if( Event == CBNETProtocol::EID_CHANNELDOESNOTEXIST )
    {
        if( !m_UNM->m_LogReduction )
            CONSOLE_Print( m_ServerAliasWithUserName + " channel does not exist" );
    }
    else if( Event == CBNETProtocol::EID_CHANNELRESTRICTED )
    {
        if( !m_UNM->m_LogReduction )
            CONSOLE_Print( m_ServerAliasWithUserName + " channel restricted" );
    }
    else if( Event == CBNETProtocol::EID_SHOWUSER )
    {
        AddChannelUser( User );
    }
    else if( Event == CBNETProtocol::EID_USERFLAGS )
    {
    }
    else if( Event == CBNETProtocol::EID_JOIN )
    {
        if( m_ServerType == 0 )
        {
            if( !m_UNM->m_LogReduction )
                CONSOLE_Print( m_ServerAliasWithUserName + " user [" + User + "] joined channel " + m_CurrentChannel );

            AddChannelUser( User );
        }
    }
    else if( Event == CBNETProtocol::EID_LEAVE )
    {
        if( !m_UNM->m_LogReduction )
            CONSOLE_Print( m_ServerAliasWithUserName + " user [" + User + "] leaves channel " + m_CurrentChannel );

        DelChannelUser( User );
    }
    else if( Event == CBNETProtocol::EID_INFO || ( Event == CBNETProtocol::EID_ERROR && Message == "Unknown user." && m_ServerType == 0 ) )
    {
        m_LogonSuccessfulTest = false;

        if( m_ServerType == 0 )
        {
            // are we tracking a user by issuing /whereis commands ever few seconds?

            if( m_UNM->IsIccupServer( m_LowerServer ) && ( Message == "Hello " + m_UserName + ", welcome to . . ." || Message.find( "The Abyss WarCraft 3 / DotA Server!" ) != string::npos || Message.find( "Current users online:" ) != string::npos || Message.find( "See the lastest news at:" ) != string::npos ) )
            {
                string CountryAbbrevLower = m_CountryAbbrev;
                string CountryLower = m_Country;
                transform( CountryAbbrevLower.begin( ), CountryAbbrevLower.end( ), CountryAbbrevLower.begin( ), static_cast<int(*)(int)>(tolower) );
                transform( CountryLower.begin( ), CountryLower.end( ), CountryLower.begin( ), static_cast<int(*)(int)>(tolower) );

                if( m_LocaleID == 25 || m_LocaleID == 1049 || m_LocaleID == 1092 || m_LocaleID == 1157 || m_LocaleID == 1133 || m_LocaleID == 2073 || m_LocaleID == 4096 || CountryAbbrevLower == "ru" || CountryAbbrevLower == "rus" || CountryAbbrevLower == "russia" || CountryLower == "ru" || CountryLower == "rus" || CountryLower == "russia" )
                {
                    if( Message == "Hello " + m_UserName + ", welcome to . . ." )
                        Message = "Привет, " + m_UserName + "! Добро пожаловать на . . .";
                    else if( Message.find( "See the lastest news at:" ) != string::npos )
                        Message = "Последние новости можно найти на сайте:";
                    else
                    {
                        string spacestring = "   ";
                        uint32_t spacecounttest = 0;

                        while( spacecounttest < m_UserName.size( ) )
                        {
                            spacestring += " ";
                            spacecounttest++;
                        }

                        if( Message.find( "The Abyss WarCraft 3 / DotA Server!" ) != string::npos )
                            Message = spacestring + "WarCraft 3 / DotA Сервер The Abyss!";
                        else if( Message.find( "Current users online:" ) != string::npos )
                        {
                            string::size_type OnlineStart = Message.find( ":" );
                            string onlinestring;

                            if( OnlineStart != string::npos )
                            {
                                onlinestring = Message.substr( OnlineStart + 1 );

                                if( !onlinestring.empty( ) && onlinestring.back( ) == ';' )
                                    onlinestring.erase( onlinestring.end( ) - 1 );

                                if( onlinestring.empty( ) )
                                    return;
                                else
                                    Message = spacestring + "Сейчас онлайн: " + onlinestring;
                            }
                        }
                    }
                }
            }

            if( Event == CBNETProtocol::EID_ERROR && Message == "Unknown user." )
                GUI_Print( 5, Message, string( ), 1 );
            else if( Message.find( " motd:" ) != string::npos )
            {
                m_HideFirstEmptyMessage = false;

                if( m_DisableDefaultNotifyMessagesInGUI == 0 || ( m_DisableDefaultNotifyMessagesInGUI == 2 && !m_SpamSubBotChat ) )
                    GUI_Print( 5, Message, string( ), 0 );
            }
            else if( Message == "you are now tempOP for this channel" )
            {
                if( m_DisableDefaultNotifyMessagesInGUI == 0 || ( m_DisableDefaultNotifyMessagesInGUI == 2 && !m_SpamSubBotChat ) )
                    GUI_Print( 5, Message, string( ), 0 );
            }
            else if( Message != "Antihack Launcher connected" || !m_UNM->IsIccupServer( m_LowerServer ) )
            {

                if( m_HideFirstEmptyMessage )
                {
                    if( Message.find_first_not_of( " " ) != string::npos )
                    {
                        m_HideFirstEmptyMessage = false;
                        GUI_Print( 5, Message, string( ), 0 );
                    }

                }
                else
                    GUI_Print( 5, Message, string( ), 0 );
            }

            CONSOLE_Print( m_ServerAliasWithUserName + " [INFO] " + Message );

            /*if( Message.size( ) >= 16 + m_UserName.size( ) && Message.substr( 0, 16 + m_UserName.size( ) ) == m_UserName + " has been kicked" )
            {
                m_OutPackets3.push_back( m_Protocol->SEND_SID_JOINCHANNEL( m_CurrentChannel ) );
                m_OutPackets3Channel.push_back( m_CurrentChannel );

                if( Message.size( ) > 17 + m_UserName.size( ) )
                    QueueChatCommand( "[" + Message.substr( 20 + m_UserName.size( ), Message.substr( 20 + m_UserName.size( ) ).size( ) - 1 ) + "] выстрелил в [" + m_UserName + "], но промахнулся" );
                else
                    QueueChatCommand( "Кто-то выстрелил в [" + m_UserName + "], но промахнулся" );
            }*/

            if( m_UNM->m_CurrentGame && m_UNM->m_SpoofChecks != 0 && ( Message.find( "User was last seen on" ) != string::npos || Message == "Unknown user." ) )
                m_UNM->m_CurrentGame->m_NumSpoofOfline++;

            // extract the first word which we hope is the username
            // this is not necessarily true though since info messages also include channel MOTD's and such

            string UserName;
            string::size_type Split = Message.find( " " );

            if( Split != string::npos )
                UserName = Message.substr( 0, Split );
            else
                UserName = Message.substr( 0 );

            bool checkingame = false;

            if( Message.find( "is using Warcraft III The Frozen Throne in game" ) != string::npos || Message.find( "is using Warcraft III Frozen Throne and is currently in  game" ) != string::npos || Message.find( "is using Warcraft III Frozen Throne and is currently in game" ) != string::npos )
            {
                checkingame = true;

                for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.size( ); i++ )
                {
                    if( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot[i] == UserName )
                    {
                        m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.erase( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.begin( ) + static_cast<int32_t>(i) );
                        m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBotBool.erase( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBotBool.begin( ) + static_cast<int32_t>(i) );
                        break;
                    }
                }

                bool AlreadyInOldQueue = false;

                for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot.size( ); i++ )
                {
                    if( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot[i] == UserName )
                    {
                        m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBotTime[i] = GetTime( );
                        AlreadyInOldQueue = true;
                        break;
                    }
                }

                if( !AlreadyInOldQueue )
                {
                    m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot.push_back( UserName );
                    m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBotTime.push_back( GetTime( ) );
                }

                string GameName = Message.substr( Message.find( "game" ) + 4 );

                if( !GameName.empty( ) && GameName.front( ) == ' ' )
                    GameName.erase( GameName.begin( ) );

                if( GameName.size( ) > 1 && GameName.back( ) == '.' )
                    GameName.erase( GameName.end( ) - 1 );

                if( GameName.size( ) > 14 && GameName.substr( GameName.size( ) - 14 ) == " (not started)" )
                    GameName = GameName.substr( 0, GameName.size( ) - 14 );
                else if( GameName.substr( GameName.size( ) - 6 ) == " ago)." )
                {
                    string::size_type GameEndPos = GameName.rfind( " (started " );

                    if( GameEndPos != string::npos )
                        GameName = GameName.substr( 0, GameEndPos );
                }

                if( GameName.size( ) > 2 && GameName.front( ) == '"' && GameName.back( ) == '"' )
                {
                    GameName.erase( GameName.begin( ) );
                    GameName.erase( GameName.end( ) - 1 );
                }

                if( !GameName.empty( ) )
                {
                    bool NewGameName = true;

                    for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame.size( ); i++ )
                    {
                        if( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame[i] == GameName )
                        {
                            NewGameName = false;
                            break;
                        }
                    }

                    if( NewGameName )
                    {
                        bool NewGameName2 = true;

                        for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame.size( ); i++ )
                        {
                            if( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame[i] == GameName )
                            {
                                NewGameName2 = false;
                                break;
                            }
                        }

                        m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGameTime.push_back( GetTime( ) );
                        m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame.push_back( GameName );

                        if( NewGameName2 )
                            m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame.push_back( GameName );
                    }
                }
            }

            // handle spoof checking for current game
            // this case covers whois results which are used when hosting a public game (we send out a "/whereis <player>" for each player)
            // at all times you can still /w the bot with "spoofcheck" to manually spoof check

            if( m_UNM->m_CurrentGame && m_UNM->m_SpoofChecks != 0 && m_UNM->m_CurrentGame->GetPlayerFromName( UserName, false ) )
            {
                if( checkingame || ( m_UNM->m_SpoofCheckIfGameNameIsIndefinite && Split != string::npos && ( Message.substr( Split + 1 ) == "is using Warcraft III The Frozen Throne." || Message.substr( Split + 1 ) == "is using Warcraft III Frozen Throne." || Message.substr( Split + 1 ) == "is using Warcraft III The Frozen Throne" || Message.substr( Split + 1 ) == "is using Warcraft III Frozen Throne" ) ) )
                {
                    bool IsInOriginalGame = false;

                    if( Message.find( m_UNM->m_CurrentGame->GetOriginalName( ) ) != string::npos )
                        IsInOriginalGame = true;

                    bool IsInLastGames = m_UNM->m_CurrentGame->IsGameName( Message );
                    m_UNM->m_CurrentGame->DelSpoofOfline( UserName );

                    if( Message.find( m_UNM->m_CurrentGame->GetGameName( ) ) != string::npos || IsInOriginalGame || IsInLastGames || ( m_UNM->m_SpoofCheckIfGameNameIsIndefinite && Split != string::npos && ( Message.substr( Split + 1 ) == "is using Warcraft III The Frozen Throne." || Message.substr( Split + 1 ) == "is using Warcraft III Frozen Throne." || Message.substr( Split + 1 ) == "is using Warcraft III The Frozen Throne" || Message.substr( Split + 1 ) == "is using Warcraft III Frozen Throne" ) ) )
                        m_UNM->m_CurrentGame->AddToSpoofed( m_Server, UserName, false );
                    else
                        m_UNM->m_CurrentGame->SendAllChat( gLanguage->SpoofDetectedIsInAnotherGame( UserName ) );
                }
                else if( Message.find( "is using Warcraft III The Frozen Throne in the channel" ) != string::npos || Message.find( "is using Warcraft III The Frozen Throne in channel" ) != string::npos || Message.find( "is using Warcraft III Frozen Throne and is currently in channel" ) != string::npos )
                {
                    m_UNM->m_CurrentGame->DelSpoofOfline( UserName );
                    m_UNM->m_CurrentGame->SendAllChat( gLanguage->SpoofDetectedIsNotInGame( UserName ) );
                }
                else if( Message.find( "is using Warcraft III The Frozen Throne in a private channel" ) != string::npos )
                {
                    m_UNM->m_CurrentGame->DelSpoofOfline( UserName );
                    m_UNM->m_CurrentGame->SendAllChat( gLanguage->SpoofDetectedIsInPrivateChannel( UserName ) );
                }
                else if( Message.find( "unknown user" ) != string::npos || Message.find( "Unknown user" ) != string::npos || Message.find( "was last seen on" ) != string::npos )
                {
                    m_UNM->m_CurrentGame->DelSpoofOfline( UserName );
                    m_UNM->m_CurrentGame->SendAllChat( gLanguage->SpoofPossibleIsUnavailable( UserName ) );
                }
                else if( Message.find( "is using Warcraft III The Frozen Throne" ) != string::npos || Message.find( "is using Warcraft III Frozen Throne" ) != string::npos )
                {
                    m_UNM->m_CurrentGame->DelSpoofOfline( UserName );
                    m_UNM->m_CurrentGame->SendAllChat( gLanguage->SpoofPossibleIsAway( UserName ) );

                    for( vector<CGamePlayer *> ::iterator i = m_UNM->m_CurrentGame->m_Players.begin( ); i != m_UNM->m_CurrentGame->m_Players.end( ); i++ )
                    {
                        if( ( *i )->GetName( ) == UserName )
                            m_UNM->m_CurrentGame->SendChat( ( *i )->GetPID( ), gLanguage->SpoofCheckByWhispering( m_UserName ) );
                    }
                }
                else if( Message.find( "AT Preparation" ) != string::npos || Message.find( "is unavailable" ) != string::npos || Message.find( "unavailable (" ) != string::npos )
                {
                    m_UNM->m_CurrentGame->DelSpoofOfline( UserName );
                    m_UNM->m_CurrentGame->SendAllChat( gLanguage->SpoofPossibleIsRefusingMessages( UserName ) );
                }
                else if( Message.find( "is refusing messages" ) != string::npos || Message.find( "refusing messages (" ) != string::npos || Message.find( "is away" ) != string::npos || Message.find( "away (" ) != string::npos )
                    m_UNM->m_CurrentGame->DelSpoofOfline( UserName );
                else
                    m_UNM->m_CurrentGame->DelSpoofOfline( UserName );
            }
        }
        else
        {
            GUI_Print( 5, Message, string( ), 0 );
            CONSOLE_Print( m_ServerAliasWithUserName + " [INFO] " + Message );
        }
    }
    else if( Event == CBNETProtocol::EID_ERROR )
    {
        m_LogonSuccessfulTest = false;
        GUI_Print( 5, Message, string( ), 1 );

        if( m_ServerType == 0 )
        {
            if( Message == "Connection to The Abyss (iCCup) has been lost, taking attempt to reconnect.." )
            {
                if( !m_IccupReconnect )
                {
                    ChangeChannel( AuthorizationPassed );
                    m_IccupReconnect = true;
                    m_IccupReconnectChannel = m_CurrentChannel;
                }
            }
        }

        CONSOLE_Print( m_ServerAliasWithUserName + " [ERROR] " + Message );
    }
    else if( Event == CBNETProtocol::EID_EMOTE )
    {
        CONSOLE_Print( m_ServerAliasWithUserName + " [EMOTE] " + User + ": " + Message );

        if( User.find_first_not_of( " " ) != string::npos )
            GUI_Print( 5, User + " (EMOTE): " + Message, string( ), 2 );
        else
            GUI_Print( 5, "EMOTE: " + Message, string( ), 2 );
    }
}

void CBNET::SendGetFriendsList( )
{
    if( GetLoggedIn( ) )
    {
        if( m_ServerType == 0 )
            m_Socket->PutBytes( m_Protocol->SEND_SID_FRIENDSLIST( ) );
    }
}

void CBNET::SendGetClanList( )
{
    if( GetLoggedIn( ) )
    {
        if( m_ServerType == 0 )
            m_Socket->PutBytes( m_Protocol->SEND_SID_CLANMEMBERLIST( ) );
    }
}

void CBNET::QueueEnterChat( )
{
    if( GetLoggedIn( ) )
    {
        if( m_ServerType == 0 )
            m_OutPackets2.push_back( m_Protocol->SEND_SID_ENTERCHAT( ) );
        else
        {
            if( m_UNM->m_GameStartedMessagePrinting && m_GameStartedMessagePrinting && m_UNM->m_Games.size( ) > 0 && !m_SpamSubBot )
            {
                QueueChatCommand( gLanguage->GameIsStarted( m_UNM->m_Games.back( )->GetGameInfo( ) ) );
                m_GameStartedMessagePrinting = false;
            }
        }
    }
}

void CBNET::SendChatCommand( string chatCommand )
{
    if( chatCommand.empty( ) )
        return;

    if( GetLoggedIn( ) )
    {
        if( m_LowerServer.find( "rubattle" ) != string::npos || m_LowerServer.find( "playground" ) != string::npos )
        {
            UTIL_Replace2( chatCommand, "iccup", "iсcup" );
            UTIL_Replace2( chatCommand, "eurobat", "еurobat" );

            if( chatCommand.find( "айкап" ) != string::npos )
                UTIL_Replace( chatCommand, "айкап", "aйкап" );
        }

        if( chatCommand.size( ) > m_MessageLength )
            chatCommand = UTIL_SubStrFix( chatCommand, 0, m_MessageLength );

        if( chatCommand.size( ) > m_MaxMessageLength )
            chatCommand = UTIL_SubStrFix( chatCommand, 0, m_MaxMessageLength );

        CONSOLE_Print( m_ServerAliasWithUserName + " [IMMEDIATE] " + chatCommand );

        if( m_ServerType == 0 )
            m_Socket->PutBytes( m_Protocol->SEND_SID_CHATCOMMAND( chatCommand, 255, 0 ) );
        else if( m_ServerType == 1 )
            m_Socket->PutBytes( m_UNMProtocol->SEND_CHAT_MESSAGE( chatCommand, 255, 0 ) );
    }
}

void CBNET::QueueChatCommand( string chatCommand )
{
    QueueChatCommand( chatCommand, false );
}

void CBNET::QueueChatCommand( string chatCommand, bool manual )
{
    if( chatCommand.empty( ) || !GetLoggedIn( ) )
        return;

    if( m_LowerServer.find( "rubattle" ) != string::npos || m_LowerServer.find( "playground" ) != string::npos )
    {
        UTIL_Replace2( chatCommand, "iccup", "iсcup" );
        UTIL_Replace2( chatCommand, "eurobat", "еurobat" );

        if( chatCommand.find( "айкап" ) != string::npos )
            UTIL_Replace( chatCommand, "айкап", "aйкап" );
    }

    if( chatCommand.size( ) > m_MaxMessageLength )
        chatCommand = UTIL_SubStrFix( chatCommand, 0, m_MaxMessageLength );

    if( m_OutPackets.size( ) > 15 && m_OutPackets.size( ) <= m_MaxOutPacketsSize && m_PacketSendDelay != 4 )
        CONSOLE_Print( m_ServerAliasWithUserName + " packet queue warning - there are already " + UTIL_ToString( m_OutPackets.size( ) ) + " packets waiting to be sent!" );
    if( m_OutPackets.size( ) > m_MaxOutPacketsSize && m_PacketSendDelay != 4 )
        CONSOLE_Print( m_ServerAliasWithUserName + " packet queue DISCARDING - there are already " + UTIL_ToString( m_OutPackets.size( ) ) + " packets waiting to be sent!" );
    else
    {
        string msg;
        string User = string( );
        int i = 0;
        bool onemoretime = false;
        bool first = true;
        bool firsttest = true;

        if( chatCommand.size( ) >= 3 && chatCommand.substr( 0, 3 ) == "/w " )
            chatCommand = "/w " + chatCommand.substr( 3 );
        else if( chatCommand.size( ) >= 3 && chatCommand.substr( 0, 3 ) == "/m " )
            chatCommand = "/w " + chatCommand.substr( 3 );
        else if( chatCommand.size( ) >= 9 && chatCommand.substr( 0, 9 ) == "/whisper " )
            chatCommand = "/w " + chatCommand.substr( 9 );
        else if( chatCommand.size( ) >= 5 && chatCommand.substr( 0, 5 ) == "/msg " )
            chatCommand = "/w " + chatCommand.substr( 5 );

        do
        {
            if( chatCommand.size( ) >= 6 && chatCommand.substr( 0, 3 ) == "/w " )
            {
                string chatCommand2 = chatCommand.substr( 3 );
                string::size_type chatCommand5 = chatCommand2.find( " " );
                string chatCommand3;

                if( chatCommand5 != string::npos )
                {
                    chatCommand3 = chatCommand2.substr( 0, chatCommand5 );

                    if( chatCommand3.size( ) > 15 )
                    {
                        chatCommand3 = UTIL_SubStrFix( chatCommand3, 0, 15 );
                        chatCommand = "/w " + chatCommand3 + chatCommand2.substr( chatCommand5 );
                    }

                    User = chatCommand3;
                }
                else
                {
                    chatCommand3 = chatCommand2;

                    if( chatCommand3.size( ) > 197 )
                        chatCommand = "/w " + UTIL_SubStrFix( chatCommand3, 0, 197 );
                }

                string chatCommand4 = "/w " + chatCommand3 + " ";
                uint32_t chatCommand4Size = m_MessageLength - chatCommand4.size( );

                if( chatCommand.size( ) > m_MessageLength && firsttest )
                {
                    uint32_t sizex;

                    if( chatCommand.size( ) > m_MessageLength )
                        sizex = chatCommand.size( ) - m_MessageLength;
                    else
                        sizex = 0;

                    uint32_t timex = 0;

                    while( sizex > 0 )
                    {
                        timex++;

                        if( sizex > m_MessageLength )
                            sizex = sizex - m_MessageLength;
                        else
                            sizex = 0;
                    }

                    if( timex > 0 )
                    {
                        uint32_t chatCommand4Sizextest = chatCommand4.size( )*timex;
                        uint32_t chatCommand4Sizex = m_MaxMessageLength - chatCommand4Sizextest;

                        if( chatCommand.size( ) > chatCommand4Sizex )
                            chatCommand = chatCommand.substr( 0, chatCommand4Sizex );
                    }

                    firsttest = false;
                }

                if( chatCommand.size( ) <= m_MessageLength )
                    msg = chatCommand;
                else if( !first )
                {
                    msg = UTIL_SubStrFix( chatCommand, i, chatCommand4Size );
                    msg = chatCommand4 + msg;

                    if( chatCommand.size( ) - i > chatCommand4Size )
                    {
                        i = i + chatCommand4Size;
                        onemoretime = true;
                    }
                    else
                        onemoretime = false;
                }
                else
                {
                    msg = UTIL_SubStrFix( chatCommand, i, m_MessageLength );

                    if( chatCommand.size( ) - i > m_MessageLength )
                    {
                        i = i + m_MessageLength;
                        onemoretime = true;
                    }
                    else
                        onemoretime = false;
                }
            }
            else if( chatCommand.size( ) >= 5 && chatCommand.substr( 0, 4 ) == "/me " )
            {
                if( chatCommand.size( ) > 104 && firsttest )
                {
                    uint32_t sizex;

                    if( chatCommand.size( ) > 104 )
                        sizex = chatCommand.size( ) - 104;
                    else
                        sizex = 0;

                    uint32_t timex = 0;

                    while( sizex > 0 )
                    {
                        timex++;

                        if( sizex > 104 )
                            sizex = sizex - 104;
                        else
                            sizex = 0;
                    }

                    if( timex > 0 )
                    {
                        uint32_t chatCommand4Sizextest = 4 * timex;
                        uint32_t chatCommand4Sizex = m_MaxMessageLength - chatCommand4Sizextest;

                        if( chatCommand.size( ) > chatCommand4Sizex )
                            chatCommand = chatCommand.substr( 0, chatCommand4Sizex );
                    }

                    firsttest = false;
                }

                if( chatCommand.size( ) <= 104 )
                    msg = chatCommand;
                else if( !first )
                {
                    msg = UTIL_SubStrFix( chatCommand, i, 100 );
                    msg = "/me " + msg;

                    if( chatCommand.size( ) - i > 100 )
                    {
                        i = i + 100;
                        onemoretime = true;
                    }
                    else
                        onemoretime = false;
                }
                else
                {
                    msg = UTIL_SubStrFix( chatCommand, i, 104 );

                    if( chatCommand.size( ) - i > 104 )
                    {
                        i = i + 104;
                        onemoretime = true;
                    }
                    else
                        onemoretime = false;
                }
            }
            else
            {
                msg = UTIL_SubStrFix( chatCommand, i, m_MessageLength );

                if( chatCommand.size( ) - i > m_MessageLength )
                {
                    i = i + m_MessageLength;
                    onemoretime = true;
                }
                else
                    onemoretime = false;
            }

            first = false;
            string messageforqstr;

            // сообщения помещаются в очередь отправки в функции GUI_Print, для оптимизации и краткости кода

            if( !m_DisableMuteCommandOnBnet || msg.size( ) < 5 || msg.substr( 0, 5 ) != "/mute" )
            {
                if( manual )
                    GUI_Print( 2, msg, User, 2 );
                else
                    GUI_Print( 2, msg, User, 1 );
            }
            else
                GUI_Print( 2, msg, User, 0 );
        }
        while( onemoretime );
    }
}

void CBNET::QueueChatCommand( string chatCommand, string user, bool whisper )
{
    if( chatCommand.empty( ) || !GetLoggedIn( ) || ( whisper && user.empty( ) ) )
        return;

    if( m_LowerServer.find( "rubattle" ) != string::npos || m_LowerServer.find( "playground" ) != string::npos )
    {
        UTIL_Replace2( chatCommand, "iccup", "iсcup" );
        UTIL_Replace2( chatCommand, "eurobat", "еurobat" );

        if( chatCommand.find( "айкап" ) != string::npos )
            UTIL_Replace( chatCommand, "айкап", "aйкап" );
    }

    if( chatCommand.size( ) > m_MaxMessageLength )
        chatCommand = UTIL_SubStrFix( chatCommand, 0, m_MaxMessageLength );

    // if whisper is true send the chat command as a whisper to user, otherwise just queue the chat command

    if( whisper )
    {
        uint32_t chatCommandSizeTest = 4 + user.size( );
        uint32_t chatCommandSize = m_MessageLength - chatCommandSizeTest;
        uint32_t sizex = chatCommand.size( );
        uint32_t timex = 0;

        while( sizex > 0 )
        {
            timex++;
            if( sizex > m_MessageLength )
                sizex = sizex - m_MessageLength;
            else
                sizex = 0;
        }

        if( timex > 0 )
        {
            uint32_t chatCommandSizexTest = chatCommandSizeTest * timex;
            uint32_t chatCommandSizex = m_MaxMessageLength - chatCommandSizexTest;

            if( chatCommand.size( ) > chatCommandSizex )
                chatCommand = chatCommand.substr( 0, chatCommandSizex );
        }

        string msg;
        int i = 0;
        bool onemoretime = false;

        do
        {
            msg = UTIL_SubStrFix( chatCommand, i, chatCommandSize );

            if( chatCommand.size( ) - i > chatCommandSize )
            {
                i = i + chatCommandSize;
                onemoretime = true;
            }
            else
                onemoretime = false;

            if( user == m_UserName )
                QueueChatCommand( "/w " + user + " " + msg );
            else
                QueueChatCommand( "/w " + user + " " + msg );

        }
        while( onemoretime );
    }
    else
        QueueChatCommand( chatCommand );
}

void CBNET::QueueGameCreate( )
{
    if( GetLoggedIn( ) )
    {
        if( !m_CurrentChannel.empty( ) )
            m_FirstChannel = m_CurrentChannel;
    }
}

void CBNET::QueueGameRefresh( unsigned char state, string gameName, string creatorName, string ownerName, CMap *map, CSaveGame *saveGame, uint32_t hostCounter, uint32_t upTime )
{
    if( GetLoggedIn( ) && map )
    {
        string hostName = string( );

        if( m_HostName == 0 )
        {
            BYTEARRAY UniqueName = m_Protocol->GetUniqueName( );
            hostName = string( UniqueName.begin( ), UniqueName.end( ) );
        }
        else if( m_HostName == 1 )
            hostName = m_UNM->m_VirtualHostName;
        else if( m_HostName == 2 )
            hostName = creatorName;
        else if( m_HostName == 3 )
            hostName = ownerName;
        else if( m_HostName == 4 )
            hostName = m_HostNameCustom;

        if( hostName.empty( ) )
        {
            BYTEARRAY UniqueName = m_Protocol->GetUniqueName( );
            hostName = string( UniqueName.begin( ), UniqueName.end( ) );
        }

        // construct the correct SID_STARTADVEX3 packet

        BYTEARRAY MapGameType;

        // construct a fixed host counter which will be used to identify players from this realm
        // the fixed host counter's 8 most significant bits will contain a 8 bit ID (0-255)
        // the rest of the fixed host counter will contain the 24 least significant bits of the actual host counter
        // since we're destroying 8 bits of information here the actual host counter should not be greater than 2^24 which is a reasonable assumption
        // when a player joins a game we can obtain the ID from the received host counter
        // note: LAN broadcasts use an ID of 0, battle.net refreshes use an ID of 1-255, the rest are unused

        uint32_t FixedHostCounter = ( hostCounter & 0x00FFFFFF ) | ( m_HostCounterID << 24 );

        // overwrite MapGameType if allowed

        if( saveGame )
        {
            MapGameType.push_back( 0 );
            MapGameType.push_back( 10 );
            MapGameType.push_back( 0 );
            MapGameType.push_back( 0 );
        }
        else if( m_AllowUseCustomMapGameType )
            MapGameType = UTIL_CreateByteArray( static_cast<uint32_t>(m_CustomMapGameType), false );
        else
        {
            MapGameType.push_back( map->GetMapGameType( ) );
            MapGameType.push_back( 32 );
            MapGameType.push_back( 73 );
            MapGameType.push_back( 0 );
        }

        // overwrite UpTime if allowed

        uint32_t UpTime = upTime;

        if( m_AllowUseCustomUpTime )
            UpTime = m_CustomUpTime;

        if( m_Reconnect )
        {
            // use an invalid map width/height to indicate reconnectable games

            BYTEARRAY MapWidth;
            BYTEARRAY MapHeight;
            MapWidth.push_back( 192 );
            MapHeight.push_back( 192 );
            MapWidth.push_back( 7 );
            MapHeight.push_back( 7 );
            m_OutPackets2.push_back( m_Protocol->SEND_SID_STARTADVEX3( m_ServerAlias, state, MapGameType, map->GetMapGameFlags( ), MapWidth, MapHeight, gameName, hostName, UpTime, saveGame ? ( "Save\\Multiplayer\\" + saveGame->GetFileNameNoPath( ) ) : map->GetMapPath( ), saveGame ? saveGame->GetMagicNumber( ) : map->GetMapCRC( ), map->GetMapSHA1( ), FixedHostCounter ) );
        }
        else
            m_OutPackets2.push_back( m_Protocol->SEND_SID_STARTADVEX3( m_ServerAlias, state, MapGameType, map->GetMapGameFlags( ), saveGame ? UTIL_CreateByteArray( (uint16_t)0, false ) : map->GetMapWidth( ), saveGame ? UTIL_CreateByteArray( (uint16_t)0, false ) : map->GetMapHeight( ), gameName, hostName, UpTime, saveGame ? ( "Save\\Multiplayer\\" + saveGame->GetFileNameNoPath( ) ) : map->GetMapPath( ), saveGame ? saveGame->GetMagicNumber( ) : map->GetMapCRC( ), map->GetMapSHA1( ), FixedHostCounter ) );
    }
}

void CBNET::QueueGameRefreshUNM( uint32_t GameNumber, bool GameLoading, bool GameLoaded, uint16_t HostPort, BYTEARRAY MapWidth, BYTEARRAY MapHeight, BYTEARRAY MapGameType, BYTEARRAY MapGameFlags, BYTEARRAY MapCRC, uint32_t HostCounter, uint32_t EntryKey, uint32_t UpTime, string GameName, string GameOwner, string MapPath, string HostName, string HostIP, string TGAImage, string MapPlayers, string MapDescription, string MapName, string Stats, unsigned char SlotsOpen, vector<CGameSlot> &GameSlots, vector<CGamePlayer *> players )
{
    m_OutPackets2.push_back( m_UNMProtocol->SEND_GAME_REFRESH( GameNumber, m_UNM->m_LCPings, m_UNM->m_TFT, m_War3Version, GameLoading, GameLoaded, HostPort, MapWidth, MapHeight, MapGameType, MapGameFlags, MapCRC, HostCounter, EntryKey, UpTime, GameName, GameOwner, MapPath, HostName, HostIP, TGAImage, MapPlayers, MapDescription, MapName, Stats, SlotsOpen, GameSlots, players ) );
}

void CBNET::QueueGameUncreate( uint32_t GameNumber )
{
    if( GetLoggedIn( ) )
    {
        if( m_ServerType == 0 )
            m_OutPackets2.push_back( m_Protocol->SEND_SID_STOPADV( ) );
        else
            m_OutPackets2.push_back( m_UNMProtocol->SEND_GAME_UNHOST( GameNumber ) );
    }
}

void CBNET::QueueChatCommandDirect( string command, unsigned char manual, unsigned char botid )
{
    if( m_ServerType == 0 )
        m_OutPackets.push_back( m_Protocol->SEND_SID_CHATCOMMAND( command, manual, botid ) );
    else if( m_ServerType == 1 )
        m_OutPackets.push_back( m_UNMProtocol->SEND_CHAT_MESSAGE( command, manual, botid ) );
}

void CBNET::UnqueueChatCommand( string chatCommand )
{
    if( chatCommand.empty( ) )
        return;

    BYTEARRAY PacketToUnqueue;
    uint32_t Unqueued = 0;

    if( m_ServerType == 0 )
        PacketToUnqueue = m_Protocol->SEND_SID_CHATCOMMAND( chatCommand, 255, 0 );
    else if( m_ServerType == 1 )
        PacketToUnqueue = m_UNMProtocol->SEND_CHAT_MESSAGE( chatCommand, 255, 0 );

    for( uint32_t i = 0; i < m_OutPackets.size( ); )
    {
        PacketToUnqueue.back( ) = m_OutPackets[i].back( );

        if( m_OutPackets[i] == PacketToUnqueue )
        {
            Unqueued++;
            m_OutPackets.erase( m_OutPackets.begin( ) + i );
        }
        else
            i++;
    }

    if( Unqueued > 0 )
        CONSOLE_Print( m_ServerAliasWithUserName + " unqueued " + UTIL_ToString( Unqueued ) + " chat command packets" );
}

void CBNET::ClearQueueGameRehost( )
{
    m_OutPackets2.clear( );
}

bool CBNET::IsFriend( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    string lowername;

    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
    {
        if( ( *it )->GetServer( ) == m_Server )
        {
            for( vector<CIncomingFriendList *> ::iterator i = ( *it )->m_Friends.begin( ); i != ( *it )->m_Friends.end( ); i++ )
            {
                lowername = ( *i )->GetAccount( );
                transform( lowername.begin( ), lowername.end( ), lowername.begin( ), static_cast<int(*)(int)>(tolower) );

                if( lowername == name )
                    return true;
            }
        }
    }

    return false;
}

bool CBNET::IsClanMember( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
    {
        if( ( *it )->GetServer( ) == m_Server )
        {
            for( vector<CIncomingClanList *> ::iterator i = ( *it )->m_Clans.begin( ); i != ( *it )->m_Clans.end( ); i++ )
            {
                if( ( *i )->GetName( ) == name )
                    return true;
            }
        }
    }

    return false;
}

bool CBNET::IsClanRecruit( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
    {
        if( ( *it )->GetServer( ) == m_Server )
        {
            for( vector<string> ::iterator i = ( *it )->m_ClanRecruits.begin( ); i != ( *it )->m_ClanRecruits.end( ); i++ )
            {
                if( *i == name )
                    return true;
            }
        }
    }

    return false;
}

bool CBNET::IsClanPeon( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
    {
        if( ( *it )->GetServer( ) == m_Server )
        {
            for( vector<string> ::iterator i = ( *it )->m_ClanPeons.begin( ); i != ( *it )->m_ClanPeons.end( ); i++ )
            {
                if( *i == name )
                    return true;
            }
        }
    }

    return false;
}

bool CBNET::IsClanGrunt( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
    {
        if( ( *it )->GetServer( ) == m_Server )
        {
            for( vector<string> ::iterator i = ( *it )->m_ClanGrunts.begin( ); i != ( *it )->m_ClanGrunts.end( ); i++ )
            {
                if( *i == name )
                    return true;
            }
        }
    }

    return false;
}

bool CBNET::IsClanShaman( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
    {
        if( ( *it )->GetServer( ) == m_Server )
        {
            for( vector<string> ::iterator i = ( *it )->m_ClanShamans.begin( ); i != ( *it )->m_ClanShamans.end( ); i++ )
            {
                if( *i == name )
                    return true;
            }
        }
    }

    return false;
}

bool CBNET::IsClanChieftain( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
    {
        if( ( *it )->GetServer( ) == m_Server )
        {
            for( vector<string> ::iterator i = ( *it )->m_ClanChieftains.begin( ); i != ( *it )->m_ClanChieftains.end( ); i++ )
            {
                if( *i == name )
                    return true;
            }
        }
    }

    return false;
}

bool CBNET::IsClanFullMember( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
    {
        if( ( *it )->GetServer( ) == m_Server )
        {
            for( vector<CIncomingClanList *> ::iterator i = ( *it )->m_Clans.begin( ); i != ( *it )->m_Clans.end( ); i++ )
            {
                if( ( *i )->GetName( ) == name && !( *it )->IsClanRecruit( name ) )
                    return true;
            }
        }
    }

    return false;
}

void CBNET::HoldFriends( CBaseGame *game )
{
    if( game )
    {
        for( vector<CIncomingFriendList *> ::iterator i = m_Friends.begin( ); i != m_Friends.end( ); i++ )
            game->AddToReserved( ( *i )->GetAccount( ), m_Server, 255 );
    }
}

void CBNET::HoldClan( CBaseGame *game )
{
    if( game )
    {
        for( vector<CIncomingClanList *> ::iterator i = m_Clans.begin( ); i != m_Clans.end( ); i++ )
            game->AddToReserved( ( *i )->GetName( ), m_Server, 255 );
    }
}

void CBNET::UNMChatCommand( string Message )
{
    CIncomingChatEvent *chatCommand = new CIncomingChatEvent( CBNETProtocol::EID_WHISPER, 999, 0, m_UserName, Message );
    ProcessChatEvent( chatCommand );
    delete chatCommand;
}

string CBNET::GetPlayerFromNamePartial( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    uint32_t Matches = 0;
    string player = string( );

    // try to match each player with the passed string (e.g. "Varlock" would be matched with "lock")

    for( uint32_t i = 0; i != m_ChannelUsers.size( ); i++ )
    {
        string TestName;
        TestName = m_ChannelUsers[i];
        transform( TestName.begin( ), TestName.end( ), TestName.begin( ), static_cast<int(*)(int)>(tolower) );

        if( TestName.find( name ) != string::npos )
        {
            Matches++;
            player = m_ChannelUsers[i];

            // if the name matches exactly stop any further matching

            if( TestName == name )
            {
                Matches = 1;
                break;
            }
        }
    }
    if( Matches == 1 )
        return player;
    else
        return string( );
}

bool CBNET::GetLoggedIn( )
{
    if( m_LoggedIn && !m_IccupReconnect )
        return true;
    else
        return false;
}

void CBNET::ChangeLogonData( string Server, string Login, string Password, bool RunAH )
{
    m_NewConnect = true;
    m_NewServer = Server;
    m_NewLogin = Login;
    m_NewPassword = Password;
    m_NewRunAH = RunAH;
}

void CBNET::ChangeLogonDataReal( string Server, string Login, string Password, bool RunAH )
{
    m_NewConnect = false;
    m_NewServer = string( );
    m_NewLogin = string( );
    m_NewPassword = string( );
    m_NewRunAH = false;

    if( Server != m_Server )
    {
        m_Server = Server;
        m_LowerServer = m_Server;
        transform( m_LowerServer.begin( ), m_LowerServer.end( ), m_LowerServer.begin( ), static_cast<int(*)(int)>(tolower) );

        if( m_UNM->IsIccupServer( m_LowerServer ) )
            m_ServerAlias = "iCCup";
        else if( m_LowerServer == "useast.battle.net" )
            m_ServerAlias = "USEast";
        else if( m_LowerServer == "uswest.battle.net" )
            m_ServerAlias = "USWest";
        else if( m_LowerServer == "asia.battle.net" )
            m_ServerAlias = "Asia";
        else if( m_LowerServer == "europe.battle.net" )
            m_ServerAlias = "Europe";
        else if( m_LowerServer == "server.eurobattle.net" || m_LowerServer == "eurobattle.net" )
            m_ServerAlias = "EuroBattle";
        else if( m_LowerServer == "europe.warcraft3.eu" )
            m_ServerAlias = "W3EU";
        else if( m_LowerServer == "battle.lp.ro" )
            m_ServerAlias = "BattleRo";
        else if( m_LowerServer == "200.51.203.231" )
            m_ServerAlias = "OMBU";
        else
            m_ServerAlias = m_Server;
    }

    m_UserName = Login;

    if( m_ServerType == 0 )
    {
        m_ServerAliasWithUserName = "[BNET: " + m_ServerAlias + " (" + m_UserName + ")]";
        m_Protocol->SetServer( m_ServerAliasWithUserName );
    }
    else if( m_ServerType == 1 )
        m_ServerAliasWithUserName = "[UNMSERVER: " + m_ServerAlias + " (" + m_UserName + ")]";

    QString usernameqstr = QString::fromUtf8( m_UserName.c_str( ) );
    emit gToLog->changeBnetUsername( m_HostCounterID, usernameqstr );
    m_UserNameLower = m_UserName;
    transform( m_UserNameLower.begin( ), m_UserNameLower.end( ), m_UserNameLower.begin( ), static_cast<int(*)(int)>(tolower) );
    m_UserPassword = Password;
    m_RunIccupLoader = RunAH;
    m_FirstConnect = true;
    m_FirstTryConnect = 0;

    if( m_UNM->IsIccupServer( m_LowerServer ) )
        transform( m_UserPassword.begin( ), m_UserPassword.end( ), m_UserPassword.begin( ), static_cast<int(*)(int)>(tolower) );

    m_ServerIP = string( );
    m_BNCSUtil->Reset( m_UserName, m_UserPassword );
    m_Socket->Reset( );
}

void CBNET::AddChannelUser( string name )
{
    if( !IsChannelUser( name ) )
    {
        m_ChannelUsers.push_back( name );
        QString messageqstr = QString::fromUtf8( name.c_str( ) );
        emit gToLog->addChannelUser( messageqstr, m_HostCounterID );
    }
}

void CBNET::DelChannelUser( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( uint32_t i = 0; i != m_ChannelUsers.size( ); i++ )
    {
        string testname = m_ChannelUsers[i];
        transform( testname.begin( ), testname.end( ), testname.begin( ), static_cast<int(*)(int)>(tolower) );

        if( testname == name )
        {
            QString messageqstr = QString::fromUtf8( m_ChannelUsers[i].c_str( ) );
            emit gToLog->removeChannelUser( messageqstr, m_HostCounterID );
            m_ChannelUsers.erase( m_ChannelUsers.begin( ) + static_cast<int32_t>(i) );
            return;
        }
    }
}

void CBNET::ChangeChannel( bool disconnect )
{
    m_ChannelUsers.clear( );

    if( disconnect )
        emit gToLog->bnetDisconnect( m_HostCounterID );
    else
        emit gToLog->clearChannelUser( m_HostCounterID );
}

bool CBNET::IsChannelUser( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( uint32_t i = 0; i != m_ChannelUsers.size( ); i++ )
    {
        string testname = m_ChannelUsers[i];
        transform( testname.begin( ), testname.end( ), testname.begin( ), static_cast<int(*)(int)>(tolower) );

        if( testname == name )
            return true;
    }

    return false;
}

QString CBNET::GetCommandTrigger( )
{
    string CommandTriggerstr = string( 1, m_CommandTrigger );
    return QString::fromUtf8( CommandTriggerstr.c_str( ) );
}

void CBNET::GUI_Print( uint32_t type, string message, string user, uint32_t option )
{
    if( type == 0 ) // чужое сообщение на канале
    {
        if( !m_SpamSubBotChat || m_MyChannel )
        {
            // мейнакки видят всегда, спамакки только если находятся на своих каналах, дубляж не идет, всегда подтверждение печати  (либо не нуждается)

            bool isbotacc = false;
            string msg = user + ": " + message;
            string userLower = user;
            transform( userLower.begin( ), userLower.end( ), userLower.begin( ), static_cast<int(*)(int)>(tolower) );

            for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
            {
                if( ( *i )->GetServer( ) == m_Server && ( *i )->m_SpamBotNet == m_SpamBotNet && ( *i )->GetLoggedIn( ) && ( *i )->GetLowerName( ) == userLower )
                {
                    isbotacc = true;
                    break;
                }
            }

            if( isbotacc )
            {
                msg = msg + "5";
                emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString::fromUtf8( message.c_str( ) ), m_HostCounterID, -1 );
            }
            else
            {
                msg = msg + "0";
                emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString( ), m_HostCounterID, -1 );
            }
        }
    }
    else if( type == 1 ) // чужое сообщение в лс
    {
        // все акки видят всегда, дубляж не идет, всегда подтверждение печати (либо не нуждается)

        bool isbotacc = false;
        string msg = "From " + user + ": " + message;
        string userLower = user;
        transform( userLower.begin( ), userLower.end( ), userLower.begin( ), static_cast<int(*)(int)>(tolower) );

        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
        {
            if( ( *i )->GetServer( ) == m_Server && ( *i )->m_SpamBotNet == m_SpamBotNet && ( *i )->GetLoggedIn( ) && ( *i )->GetLowerName( ) == userLower )
            {
                isbotacc = true;
                break;
            }
        }

        if( isbotacc )
        {
            msg = msg + "6";
            emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString::fromUtf8( message.c_str( ) ), m_HostCounterID, m_MainBotChatID );
        }
        else
        {
            msg = msg + "2";
            emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString( ), m_HostCounterID, m_MainBotChatID );
        }
    }
    else if( type == 2 ) // печать сообщений
    {
        // вывод идет всегда, без дубляжя, всегда предварительная печать
        // если функция вызывается в мейнакке, то сообщения адресованные в лс ИГРОКАМ помечаются, чтобы на этапе отправки, в случае если сообщение будет отправлено с спамакка - оно дублировалось для мейнакка

        string msg = string( );

        if( option != 0 ) // обычное сообщение
        {
            bool toChat = false;
            string wmsg = string( );

            if( user.find_first_not_of( " " ) != string::npos )
            {
                bool isbotacc = false;
                string userLower = user;
                transform( userLower.begin( ), userLower.end( ), userLower.begin( ), static_cast<int(*)(int)>(tolower) );

                if( message.size( ) > 4 + user.size( ) )
                    wmsg = message.substr( 4 + user.size( ) );

                for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                {
                    if( ( *i )->GetServer( ) == m_Server && ( *i )->m_SpamBotNet == m_SpamBotNet && ( *i )->GetLoggedIn( ) && ( *i )->GetLowerName( ) == userLower )
                    {
                        isbotacc = true;
                        break;
                    }
                }

                if( isbotacc )
                {
                    if( wmsg.find_first_not_of( " " ) != string::npos )
                    {
                        toChat = true;
                        msg = "To " + user + ": " + wmsg + "1";
                        string::size_type wmsgStart = wmsg.find_first_not_of( " " );

                        if( wmsgStart != string::npos )
                            wmsg = wmsg.substr( wmsgStart );
                    }
                    else
                    {
                        msg = m_UserName + ": " + message + "1";
                        wmsg = message;
                    }

                    QueueChatCommandDirect( message, (option == 1 ? 255 : 253), GetHostCounterID( ) );
                }
                else if( wmsg.find_first_not_of( " " ) != string::npos )
                {
                    toChat = true;
                    msg = "To " + user + ": " + wmsg + "1";
                    string::size_type wmsgStart = wmsg.find_first_not_of( " " );

                    if( wmsgStart != string::npos )
                        wmsg = wmsg.substr( wmsgStart );

                    if( !m_SpamSubBotChat )
                        QueueChatCommandDirect( message, (option == 1 ? 255 : 253), GetHostCounterID( ) );
                    else
                        QueueChatCommandDirect( message, (option == 1 ? 254 : 252), GetHostCounterID( ) );
                }
                else
                {
                    msg = m_UserName + ": " + message + "1";
                    wmsg = message;
                    QueueChatCommandDirect( message, (option == 1 ? 255 : 253), GetHostCounterID( ) );
                }
            }
            else
            {
                msg = m_UserName + ": " + message + "1";
                wmsg = message;
                QueueChatCommandDirect( message, (option == 1 ? 255 : 253), GetHostCounterID( ) );
            }

            if( toChat || message.empty( ) || message.front( ) != '/' )
                emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString::fromUtf8( wmsg.c_str( ) ), m_HostCounterID, -1 );
        }
        else // попытка отправки сообщения "/mute ..." при запрещенном использовании данной bnet-команды
        {
            msg = "Сообщение не отправлено: " + message + "4";
            emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString( ), m_HostCounterID, -1 );
        }
    }
    else if( type == 3 || type == 4 ) // отправка сообщений
    {
        // мейнакки видят
        // либо подтверждение печати, либо сообщения которые не нуждаются в подверждении
        // type 4 - сообщение спамакка (сформированно) в лс другому юзеру
        // type 3 - остальные сообщения
        // option = номер акка, на котором сформировано сообщение

        bool toChat = false;
        string msg = string( );
        string msgtype = "5";
        bool check = false;
        bool check2 = false;
        bool pm = false;
        string specmsg = string( );

        if( option != GetHostCounterID( ) )
            msgtype = "0";
        else
            specmsg = message;

        if( ( message.size( ) > 5 && message.substr( 0, 3 ) == "/w " ) || ( message.size( ) > 5 && message.substr( 0, 3 ) == "/m " ) || ( message.size( ) > 11 && message.substr( 0, 9 ) == "/whisper " ) || ( message.size( ) > 7 && message.substr( 0, 5 ) == "/msg " ) )
        {
            string messageTest;
            string ToUser;
            string::size_type MessageStart = message.find( " " );

            if( MessageStart != string::npos )
            {
                messageTest = message.substr( MessageStart + 1 );
                MessageStart = messageTest.find( " " );

                if( MessageStart != string::npos )
                {
                    ToUser = messageTest.substr( 0, MessageStart );
                    messageTest = messageTest.substr( MessageStart + 1 );

                    if( ToUser.find_first_not_of( " " ) != string::npos && messageTest.find_first_not_of( " " ) != string::npos )
                    {
                        pm = true;
                        bool isbotacc = false;
                        string userLower = ToUser;
                        uint32_t botid = 0;
                        transform( userLower.begin( ), userLower.end( ), userLower.begin( ), static_cast<int(*)(int)>(tolower) );

                        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                        {
                            if( ( *i )->GetServer( ) == m_Server && ( *i )->m_SpamBotNet == m_SpamBotNet && ( *i )->GetLoggedIn( ) && ( *i )->GetLowerName( ) == userLower )
                            {
                                isbotacc = true;
                                botid = ( *i )->GetHostCounterID( );
                                break;
                            }
                        }

                        if( !isbotacc )
                        {
                            if( type == 4 )
                                check = true;

                            toChat = true;
                            msg = "To " + ToUser + ": " + messageTest + "6";
                            string::size_type specmsgStart = messageTest.find_first_not_of( " " );

                            if( specmsgStart != string::npos )
                                specmsg = messageTest.substr( specmsgStart );
                            else
                                specmsg = messageTest;
                        }
                        else if( m_UNM->m_BNETs[option-1]->m_SpamSubBotChat && option != botid )
                        {
                            toChat = true;
                            check2 = true;
                            msg = "To " + ToUser + ": " + messageTest + "6";
                            string::size_type specmsgStart = messageTest.find_first_not_of( " " );

                            if( specmsgStart != string::npos )
                                specmsg = messageTest.substr( specmsgStart );
                            else
                                specmsg = messageTest;
                        }
                    }
                    else
                        msg = m_UserName + ": " + message + msgtype;
                }
                else
                    msg = m_UserName + ": " + message + msgtype;
            }
            else
                msg = m_UserName + ": " + message + msgtype;
        }
        else
            msg = m_UserName + ": " + message + msgtype;

        if( !msg.empty( ) && ( toChat || message.empty( ) || message.front( ) != '/' ) )
        {
            if( !pm )
                emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString::fromUtf8( specmsg.c_str( ) ), m_HostCounterID, -1 );
            else if( check2 )
                emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString::fromUtf8( specmsg.c_str( ) ), option, -1 );
            else if( check )
            {
                if( m_MainBotChatID == -1 )
                    emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString::fromUtf8( specmsg.c_str( ) ), option, m_HostCounterID );
                else
                    emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString::fromUtf8( specmsg.c_str( ) ), option, m_MainBotChatID );
            }
            else
                emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString::fromUtf8( specmsg.c_str( ) ), m_HostCounterID, m_MainBotChatID );
        }
    }
    else if( type == 5 )
    {
        // прочие сообщения: 0 - уведомления, 1 - предупреждения, 2 - emote

        string msg = string( );

        if( option == 0 )
            msg = message + "3";
        else if( option == 1 )
            msg = message + "4";
        else
            msg = message + "8";

        emit gToLog->toChat( QString::fromUtf8( msg.c_str( ) ), QString( ), m_HostCounterID, -1 );
    }
}

void CBNET::BNETServerSocketUpdate( void *fd, void *send_fd )
{
    // we return at the end of each if statement so we don't have to deal with errors related to the order of the if statements
    // that means it might take a few ms longer to complete a task involving multiple steps (in this case, reconnecting) due to blocking or sleeping
    // but it's not a big deal at all, maybe 100ms in the worst possible case (based on a 50ms blocking time)

    if( m_Socket->HasError( ) )
    {
        // the socket has an error

        CONSOLE_Print( m_ServerAliasWithUserName + " disconnected from battle.net due to socket error" );

        if( m_Socket->GetError( ) == ECONNRESET && GetTime( ) - m_LastConnectionAttemptTime < 65 )
            CONSOLE_Print( m_ServerAliasWithUserName + " warning - you are probably temporarily IP banned from battle.net" );

        if( m_UNM->m_LocalSocket )
            m_UNM->SendLocalChat( "Disconnected from battle.net." );

        if( m_FirstTryConnect == 0 )
        {
            CONSOLE_Print( m_ServerAliasWithUserName + " waiting 3 seconds to reconnect" );
            m_FastReconnect = true;
            m_LastDisconnectedTime = GetTime( ) - m_TimeWaitingToReconnect + 3;
        }
        else
        {
            if( m_HostCounterID == 1 && !AuthorizationPassed )
            {
                string messageforqstr;

                if( m_UNM->IsIccupServer( m_LowerServer ) && LogonDataRunAH )
                    messageforqstr = "Не удалось выполнить вход, ошибка сокета. Возможно, следует обновить анти-хак (файл iccwc3.icc)";
                else
                    messageforqstr = "Не удалось выполнить вход, ошибка сокета.";

                QString messageqstr = QString::fromUtf8( messageforqstr.c_str( ) );
                emit gToLog->logonFailed( messageqstr );
            }

            CONSOLE_Print( m_ServerAliasWithUserName + " waiting " + UTIL_ToString( m_TimeWaitingToReconnect ) + " seconds to reconnect" );
            m_LastDisconnectedTime = GetTime( );
        }

        m_BNCSUtil->Reset( m_UserName, m_UserPassword );
        m_Socket->Reset( );
        m_LoggedIn = false;
        m_InChat = false;
        m_InGame = false;
        m_LogonSuccessfulTest = false;
        ChangeChannel( AuthorizationPassed );
        m_WaitingToConnect = true;
        m_FirstTryConnect++;
        ResetGameList( );
        return;
    }

    if( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && !m_WaitingToConnect )
    {
        if( m_HostCounterID == 1 && !AuthorizationPassed )
        {
            string messageforqstr = "Не удалось выполнить вход." + m_LoginErrorMessage;

            if( m_LoginErrorMessage.empty( ) && m_LowerServer != "server.eurobattle.net" && m_LowerServer != "eurobattle.net" )
            {
                if( m_IncorrectPasswordAttemptsNumber == 2 )
                {
                    m_IncorrectPasswordAttemptsNumber++;
                    messageforqstr += " Вы указали неверный пароль. У вас было 3 попытки чтобы ввести верный пароль. Вы исчерпали данный лимит и вероятно получили временный бан! (" + UTIL_ToString( m_IncorrectPasswordAttemptsNumber ) + "/3)";
                }
                else if( m_IncorrectPasswordAttemptsNumber == 3 )
                {
                    m_IncorrectPasswordAttemptsNumber++;
                    messageforqstr += " Похоже мы получили временный бан за исчерпания попыток ввести верный пароль.";
                }
                else if( m_IncorrectPasswordAttemptsNumber == 4 )
                    messageforqstr += " Похоже мы получили временный бан за исчерпания попыток ввести верный пароль.";
            }

            if( m_IncorrectPasswordAttemptsNumber == 3 )
                messageforqstr = "0" + messageforqstr;
            else if( m_IncorrectPasswordAttemptsNumber == 2 )
                messageforqstr = "1" + messageforqstr;

            QString messageqstr = QString::fromUtf8( messageforqstr.c_str( ) );
            emit gToLog->logonFailed( messageqstr );
        }

        // the socket was disconnected

        if( m_UNM->m_LocalSocket )
            m_UNM->SendLocalChat( "Disconnected from battle.net." );

        CONSOLE_Print( m_ServerAliasWithUserName + " disconnected from battle.net" );
        CONSOLE_Print( m_ServerAliasWithUserName + " waiting " + UTIL_ToString( m_TimeWaitingToReconnect ) + " seconds to reconnect" );
        m_BNCSUtil->Reset( m_UserName, m_UserPassword );
        m_Socket->Reset( );
        m_LastDisconnectedTime = GetTime( );
        m_LoggedIn = false;
        m_InChat = false;
        m_InGame = false;
        m_LogonSuccessfulTest = false;
        ChangeChannel( AuthorizationPassed );
        m_WaitingToConnect = true;
        m_FirstTryConnect = 2;
        ResetGameList( );
        return;
    }

    // сбрасываем соединение, если что-то идет не так

    if( AuthorizationPassed && m_Socket->GetConnected( ) && GetLoggedIn( ) && ( ( m_HostPacketSent && GetTime( ) > m_HostPacketSentTime + 9 ) || ( m_LogonSuccessfulTest && GetTime( ) > m_LogonSuccessfulTestTime + 9 ) ) )
    {
        if( m_HostPacketSent && GetTime( ) > m_HostPacketSentTime + 9 )
            CONSOLE_Print( m_ServerAliasWithUserName + " соединение было сброшено (нет ответа на host packet)" );
        else
            CONSOLE_Print( m_ServerAliasWithUserName + " соединение было сброшено (нет ответа на chatevent packet)" );

        m_LogonSuccessfulTest = false;
        m_HostPacketSent = false;
        m_FirstConnect = true;
        m_BNCSUtil->Reset( m_UserName, m_UserPassword );
        m_Socket->Reset( );
        m_LoggedIn = false;
        m_InChat = false;
        m_InGame = false;
        ChangeChannel( AuthorizationPassed );
        m_WaitingToConnect = true;
        m_FirstTryConnect++;

        if( m_UNM->m_LocalSocket )
            m_UNM->SendLocalChat( "Disconnected from battle.net." );

        ResetGameList( );
    }

    if( m_Socket->GetConnected( ) )
    {
        // the connection attempt timed out (5 seconds)

        if( m_WaitResponse && GetTime( ) - m_LastConnectionAttemptTime >= 5 )
        {
            CONSOLE_Print( m_ServerAliasWithUserName + " connect timed out" );

            if( m_UNM->IsIccupServer( m_LowerServer ) && ( ( m_RunIccupLoader && m_HostCounterID != 1 ) || ( LogonDataRunAH && m_HostCounterID == 1 ) ) )
                CONSOLE_Print( m_ServerAliasWithUserName + " не удалось присоединиться к IccReconnectLoader.exe - возможно запущено несколько экземпляров использующих общий порт" );

            if( m_FirstTryConnect == 0 )
            {
                CONSOLE_Print( m_ServerAliasWithUserName + " waiting 3 seconds to reconnect" );
                m_FastReconnect = true;
                m_LastDisconnectedTime = GetTime( ) - m_TimeWaitingToReconnect + 3;
            }
            else
            {
                if( m_HostCounterID == 1 && !AuthorizationPassed )
                {
                    string messageforqstr;

                    if( m_UNM->IsIccupServer( m_LowerServer ) && LogonDataRunAH )
                        messageforqstr = "Не удалось выполнить вход, ошибка сокета. Возможно запущено несколько экземпляров анти-хака (IccReconnectLoader.exe), использующих общий порт.";
                    else
                        messageforqstr = "Не удалось выполнить вход, время ожидания ответа от сервера истекло.";

                    QString messageqstr = QString::fromUtf8( messageforqstr.c_str( ) );
                    emit gToLog->logonFailed( messageqstr );
                }

                CONSOLE_Print( m_ServerAliasWithUserName + " waiting " + UTIL_ToString( m_TimeWaitingToReconnect ) + " seconds to reconnect" );
                m_LastDisconnectedTime = GetTime( );
            }

            m_Socket->Reset( );
            m_WaitingToConnect = true;
            m_WaitResponse = false;
            m_LogonSuccessfulTest = false;
            m_FirstTryConnect++;
            return;
        }

        // the socket is connected and everything appears to be working properly

        m_Socket->DoRecv( (fd_set *)fd, m_ServerAliasWithUserName );
        ExtractPackets( );
        ProcessPackets( );

        // update bnet bots

        for( vector<CBNETBot *>::iterator i = m_BnetBots.begin( ); i != m_BnetBots.end( ); i++ )
            ( *i )->Update( );

        // request the public game list

//        if( AuthorizationPassed && !m_UNM->m_LocalSocket )
        if( AuthorizationPassed && GetLoggedIn( ) )
        {
            if( !m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame.empty( ) )
            {
                if( GetTime( ) - m_LastGetPublicListTime >= m_OutPackets.size( ) + m_LastRequestSearchGame )
                {
                    QueueGetGameList( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame[0] );
                    m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame.erase( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame.begin( ) );
                    m_LastGetPublicListTime = GetTime( );
                }
            }
            else if( GetTime( ) - m_LastGetPublicListTime >= 3 && GetTime( ) - m_LastGetPublicListTimeGeneral >= 1 && m_LastRequestSearchGame % 2 == 0 )
            {
                uint32_t m_GetListDelay = 1 + ( m_UNM->m_BNETs[m_MainBotID]->m_GPGames.size( ) / 4 ) + ( ( ( m_OutPackets.size( ) / ceil( (double)(m_AllChatBotIDs.size( )) / 2 ) ) + m_OutPackets3.size( ) ) * 5 ) + ( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.size( ) * 5 );

                if( m_GetListDelay < 3 )
                    m_GetListDelay = 3;
                else if( m_GetListDelay == 3 )
                    m_GetListDelay = 4;

                if( m_LastRequestSearchGame > 0 )
                    m_GetListDelay = m_GetListDelay * m_LastRequestSearchGame;

                // m_LastRequestSearchGame

                if( GetTime( ) - m_LastGetPublicListTime >= m_GetListDelay || GetTime( ) - m_LastGetPublicListTimeFull >= 60 )
                {
                    // request 20 games (note: it seems like 20 is the maximum, requesting more doesn't result in more results returned)

                    QueueGetGameList( 20 );
                    m_LastGetPublicListTime = GetTime( );
                    m_LastGetPublicListTimeFull = GetTime( );

                    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                    {
                        if( ( *i )->GetServer( ) == m_Server )
                            ( *i )->m_LastGetPublicListTimeGeneral = GetTime( );
                    }
                }
            }

            if( !m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.empty( ) && GetTicks( ) - m_LastSearchBotTicks > 1100 )
            {
                m_LastSearchBotTicks = GetTicks( );
                bool AlreadyInOldQueue = false;

                for( uint32_t i = 0; i < m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot.size( ); i++ )
                {
                    if( m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot[i] == m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot[0] )
                    {
                        m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBotTime[i] = GetTime( );
                        AlreadyInOldQueue = true;
                        break;
                    }
                }

                if( !AlreadyInOldQueue )
                {
                    m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot.push_back( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot[0] );
                    m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBotTime.push_back( GetTime( ) );
                }

                if( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBotBool[0] )
                {
                    QueueChatCommand( "/watch " + m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot[0] );
                    m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.erase( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.begin( ) );
                    m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBotBool.erase( m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBotBool.begin( ) );
                }
                else
                {
                    QueueChatCommand( "/whois " + m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot[0] );
                    m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBotBool[0] = true;
                }
            }
        }

        while( !m_OutPackets.empty( ) )
        {
            bool SafeOnlyPMCheck = true;
            string channel1;
            string channel2;
            string OutPacket = string( );
            bool msgcommand = false;
            bool msgm = false;
            bool msgw = false;
            bool msgmsg = false;
            bool msgwhisper = false;
            bool msgwhere = false;
            bool msgwhereis = false;
            bool msgme = false;

            if( m_OutPackets.front( ).size( ) > 5 && m_OutPackets.front( )[1] == 14 )
                OutPacket = string( m_OutPackets.front( ).begin( ) + 4, m_OutPackets.front( ).end( ) - 1 );

            if( m_SpamBotMode > 0 )
            {
                if( !OutPacket.empty( ) && OutPacket.front( ) == '/' )
                {
                    msgcommand = true;
                    string OutPacketLower = OutPacket;
                    transform( OutPacketLower.begin( ), OutPacketLower.end( ), OutPacketLower.begin( ), static_cast<int(*)(int)>(tolower) );
                    string Command = string( );
                    string Payload1 = string( );
                    string Payload2 = string( );
                    string::size_type PayloadStart = OutPacketLower.find( " " );

                    if( PayloadStart != string::npos )
                    {
                        Command = OutPacketLower.substr( 0, PayloadStart );
                        Payload1 = OutPacketLower.substr( PayloadStart + 1 );

                        while( !Payload1.empty( ) && Payload1.front( ) == ' ' )
                            Payload1.erase( Payload1.begin( ) );

                        if( !Payload1.empty( ) )
                        {
                            PayloadStart = Payload1.find( " " );

                            if( PayloadStart != string::npos )
                            {
                                Payload2 = Payload1.substr( PayloadStart + 1 );
                                Payload1 = Payload1.substr( 0, PayloadStart );

                                while( !Payload2.empty( ) && Payload2.front( ) == ' ' )
                                    Payload2.erase( Payload2.begin( ) );
                            }
                        }
                    }
                    else
                        Command = OutPacketLower;

                    if( Command == "/m" )
                    {
                        if( !Payload1.empty( ) && !Payload2.empty( ) )
                            msgm = true;
                    }
                    else if( Command == "/w" )
                    {
                        if( !Payload1.empty( ) && !Payload2.empty( ) )
                            msgw = true;
                    }
                    else if( Command == "/msg" )
                    {
                        if( !Payload1.empty( ) && !Payload2.empty( ) )
                            msgmsg = true;
                    }
                    else if( Command == "/whisper" )
                    {
                        if( !Payload1.empty( ) && !Payload2.empty( ) )
                            msgwhisper = true;
                    }
                    else if( Command == "/where" )
                    {
                        if( !Payload1.empty( ) )
                            msgwhere = true;
                    }
                    else if( Command == "/whereis" )
                    {
                        if( !Payload1.empty( ) )
                            msgwhereis = true;
                    }
                    else if( Command == "/me" )
                    {
                        if( !Payload1.empty( ) )
                            msgme = true;
                    }
                }

                if( !OutPacket.empty( ) && ( !m_SpamSubBotChat || m_OutPackets.front( )[0] < 254 ) && m_SpamBotMode > 0 && m_SpamBotNet > 0 )
                    channel1 = GetCurrentChannel( );

                if( !OutPacket.empty( ) && m_SpamBotOnlyPM == 3 && m_SpamBotMode == 1 && m_SpamBotNet > 0 && ( !msgcommand || ( !msgm && !msgw && !msgmsg && !msgwhisper && !msgwhere && !msgwhereis ) ) )
                {
                    SafeOnlyPMCheck = false;

                    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                    {
                        if( ( *i )->GetServer( ) == m_Server )
                        {
                            if( ( *i )->GetLoggedIn( ) && ( *i )->m_SpamBotNet == m_SpamBotNet && ( ( *i )->m_SpamBotOnlyPM == 0 || ( *i )->m_SpamBotOnlyPM == 4 || ( ( *i )->m_SpamBotOnlyPM == 1 && ( *i )->GetCurrentChannel( ) == GetCurrentChannel( ) ) ) )
                                SafeOnlyPMCheck = true;

                            if( ( channel1.empty( ) || ( *i )->GetCurrentChannel( ) == GetCurrentChannel( ) ) && ( *i )->GetLoggedIn( ) && ( *i )->m_SpamBotNet == m_SpamBotNet && !( *i )->m_SpamSubBotChat )
                                channel1 = ( *i )->GetCurrentChannel( );
                        }
                    }
                }
                else if( !OutPacket.empty( ) && channel1.empty( ) && m_SpamBotMode == 1 && m_SpamBotNet > 0 )
                {
                    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                    {
                        if( ( *i )->GetServer( ) == m_Server && ( channel1.empty( ) || ( *i )->GetCurrentChannel( ) == GetCurrentChannel( ) ) && ( *i )->GetLoggedIn( ) && ( *i )->m_SpamBotNet == m_SpamBotNet && !( *i )->m_SpamSubBotChat )
                            channel1 = ( *i )->GetCurrentChannel( );
                    }
                }

                if( channel1.empty( ) )
                    channel1 = GetCurrentChannel( );
            }

            if( !OutPacket.empty( ) && m_SpamBotMode > 0 && m_SpamBotNet > 0 && SafeOnlyPMCheck && ( m_SpamBotMode == 1 || ( msgcommand && ( msgm || msgw || msgmsg || msgwhisper || msgwhere || msgwhereis ) ) ) && ( !m_SpamBotFixBnetCommands || !msgcommand || msgm || msgw || msgmsg || msgwhisper || msgwhere || msgwhereis || msgme ) )
            {
                if( GetTicks( ) - m_LastMessageSendTicks >= m_SpamBotMessagesSendDelay || m_SpamBotMessagesSendDelay == 0 )
                {
                    bool FullCycle = true;

                    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                    {
                        if( ( *i )->GetServer( ) == m_Server && ( *i )->m_SpamBotNet == m_SpamBotNet )
                        {
                            channel2 = ( *i )->GetCurrentChannel( );

                            if( ( ( *i )->m_PacketSendDelay == 4 || ( ( *i )->m_OutPackets3.empty( ) && GetTicks( ) - ( *i )->m_LastOutPacketTicks3 >= 1100 ) ) && ( *i )->GetLoggedIn( ) && ( m_SpamBotMode == 2 || ( *i )->m_SpamBotOnlyPM == 0 || ( *i )->m_SpamBotOnlyPM == 4 || ( ( *i )->m_SpamBotOnlyPM == 1 && channel2 == GetCurrentChannel( ) ) || ( ( *i )->m_SpamBotOnlyPM == 2 && ( *i )->GetHostCounterID( ) == GetHostCounterID( ) ) || ( msgcommand && ( msgm || msgw || msgmsg || msgwhisper || msgwhere || msgwhereis ) ) ) )
                            {
                                bool needchangechannel = ( ( *i )->m_SpamBotOnlyPM == 4 && channel1 != channel2 && ( !msgcommand || msgme ) );

                                if( m_UNM->m_CurrentGame && needchangechannel && ( *i )->m_GameIsHosted )
                                    continue;

                                // check if at least one packet is waiting to be sent and if we've waited long enough to prevent flooding
                                // this formula has changed many times but currently we wait 1 second if the last packet was "small", 3.5 seconds if it was "medium", and 4 seconds if it was "big"

                                uint32_t WaitTicks = 0;

                                if( ( *i )->m_PacketSendDelay == 3 )
                                {
                                    if( ( ( OutPacket.size( ) > 4 && OutPacket.substr( 0, 4 ) == "/me " ) || m_OutPackets.front( ).size( ) >= 100 ) && ( *i )->m_LastOutPacketSize[0] < 100 && ( *i )->m_LastOutPacketSize[1] < 100 )
                                    {
                                        if( OutPacket.size( ) > 4 && OutPacket.substr( 0, 4 ) == "/me " )
                                            ( *i )->m_LastOutPacketSize[2] = m_MessageLength;
                                        else
                                            ( *i )->m_LastOutPacketSize[2] = m_OutPackets.front( ).size( );
                                    }
                                    else
                                        ( *i )->m_LastOutPacketSize[2] = ( *i )->m_LastOutPacketSize[0];

                                    if( ( *i )->m_LastOutPacketSize[2] < 10 )
                                        WaitTicks = 1100; // 1000 ?
                                    else if( ( *i )->m_LastOutPacketSize[2] < 100 )
                                        WaitTicks = 1100; // 1000 ?
                                    else
                                        WaitTicks = 3100; // 3000 ?

                                    if( GetTicks( ) - ( *i )->m_LastOutPacketTicks >= WaitTicks )
                                        ( *i )->m_LastOutPacketSize[0] = ( *i )->m_LastOutPacketSize[2];
                                }
                                else if( ( *i )->m_PacketSendDelay == 2 )
                                {
                                    if( ( *i )->m_LastOutPacketSize[0] < 10 )
                                        WaitTicks = 1000;
                                    else if( ( *i )->m_LastOutPacketSize[0] < 25 )
                                        WaitTicks = 1100;
                                    else if( ( *i )->m_LastOutPacketSize[0] < 50 )
                                        WaitTicks = 1250;
                                    else if( ( *i )->m_LastOutPacketSize[0] < 75 )
                                        WaitTicks = 1450;
                                    else if( ( *i )->m_LastOutPacketSize[0] < 100 )
                                        WaitTicks = 1700;
                                    else if( ( *i )->m_LastOutPacketSize[0] < 125 )
                                        WaitTicks = 2000;
                                    else if( ( *i )->m_LastOutPacketSize[0] < 150 )
                                        WaitTicks = 2400;
                                    else if( ( *i )->m_LastOutPacketSize[0] < 175 )
                                        WaitTicks = 2900;
                                    else if( ( *i )->m_LastOutPacketSize[0] <= m_MessageLength )
                                        WaitTicks = 3450;
                                    else
                                        WaitTicks = 3600;
                                }
                                else if( ( *i )->m_PacketSendDelay == 1 )
                                {
                                    if( ( ( OutPacket.size( ) > 4 && OutPacket.substr( 0, 4 ) == "/me " ) || m_OutPackets.front( ).size( ) >= 100 ) && ( *i )->m_LastOutPacketSize[0] < 100 && ( *i )->m_LastOutPacketSize[1] < 100 )
                                    {
                                        if( OutPacket.size( ) > 4 && OutPacket.substr( 0, 4 ) == "/me " )
                                            ( *i )->m_LastOutPacketSize[2] = m_MessageLength;
                                        else
                                            ( *i )->m_LastOutPacketSize[2] = m_OutPackets.front( ).size( );
                                    }
                                    else
                                        ( *i )->m_LastOutPacketSize[2] = ( *i )->m_LastOutPacketSize[0];

                                    if( ( *i )->m_LastOutPacketSize[2] < 10 )
                                        WaitTicks = m_PacketDelaySmall;
                                    else if( ( *i )->m_LastOutPacketSize[2] < 100 )
                                        WaitTicks = m_PacketDelayMedium;
                                    else
                                        WaitTicks = m_PacketDelayBig;

                                    if( GetTicks( ) - ( *i )->m_LastOutPacketTicks >= WaitTicks )
                                        ( *i )->m_LastOutPacketSize[0] = ( *i )->m_LastOutPacketSize[2];
                                }
                                else if( ( *i )->m_PacketSendDelay == 0 )
                                {
                                    if( ( *i )->m_LastOutPacketSize[0] < 10 )
                                        WaitTicks = m_PacketDelaySmall;
                                    else if( ( *i )->m_LastOutPacketSize[0] < 100 )
                                        WaitTicks = m_PacketDelayMedium;
                                    else
                                        WaitTicks = m_PacketDelayBig;
                                }

                                if( GetTicks( ) - ( *i )->m_LastOutPacketTicks >= WaitTicks || WaitTicks == 0 )
                                {
                                    if( m_MaxOutPacketsSize > 30 )
                                        --m_MaxOutPacketsSize;

                                    if( needchangechannel )
                                    {
                                        ( *i )->m_OutPackets3.push_back( m_Protocol->SEND_SID_JOINCHANNEL( channel1 ) );
                                        ( *i )->m_OutPackets3Channel.push_back( channel1 );
                                        ( *i )->m_OutPackets3.push_back( m_OutPackets.front( ) );
                                        ( *i )->m_OutPackets3Channel.push_back( string( ) );
                                        ( *i )->m_OutPackets3.push_back( m_Protocol->SEND_SID_JOINCHANNEL( channel2 ) );
                                        ( *i )->m_OutPackets3Channel.push_back( channel2 );
                                        CONSOLE_Print( ( *i )->m_ServerAliasWithUserName + " QUE: " + OutPacket );
                                    }
                                    else
                                    {
                                        ( *i )->m_OutPackets3.push_back( m_OutPackets.front( ) );
                                        ( *i )->m_OutPackets3Channel.push_back( string( ) );
                                        CONSOLE_Print( ( *i )->m_ServerAliasWithUserName + " QUE: " + OutPacket );
                                    }

                                    ( *i )->m_LastOutPacketSize[1] = ( *i )->m_LastOutPacketSize[0];

                                    if( OutPacket.size( ) > 4 && OutPacket.substr( 0, 4 ) == "/me " )
                                        ( *i )->m_LastOutPacketSize[0] = m_MessageLength;
                                    else
                                        ( *i )->m_LastOutPacketSize[0] = m_OutPackets.front( ).size( );

                                    m_OutPackets.pop_front( );
                                    ( *i )->m_LastOutPacketTicks = GetTicks( );

                                    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                                    {
                                        if( ( *it )->GetServer( ) == ( *i )->GetServer( ) && ( *it )->m_SpamBotNet == ( *i )->m_SpamBotNet )
                                            ( *it )->m_LastMessageSendTicks = GetTicks( );
                                    }

                                    if( i + 1 != m_UNM->m_BNETs.end( ) || ( *i )->m_PacketSendDelay == 4 )
                                        FullCycle = false;

                                    break;
                                }
                            }
                        }
                    }

                    if( FullCycle )
                        break;
                }
                else
                    break;
            }
            else
            {
                // check if at least one packet is waiting to be sent and if we've waited long enough to prevent flooding
                // this formula has changed many times but currently we wait 1 second if the last packet was "small", 3.5 seconds if it was "medium", and 4 seconds if it was "big"

                uint32_t WaitTicks = 0;

                if( m_PacketSendDelay == 3 )
                {
                    if( ( ( OutPacket.size( ) > 4 && OutPacket.substr( 0, 4 ) == "/me " ) || m_OutPackets.front( ).size( ) >= 100 ) && m_LastOutPacketSize[0] < 100 && m_LastOutPacketSize[1] < 100 )
                    {
                        if( OutPacket.size( ) > 4 && OutPacket.substr( 0, 4 ) == "/me " )
                            m_LastOutPacketSize[0] = m_MessageLength;
                        else
                            m_LastOutPacketSize[0] = m_OutPackets.front( ).size( );
                    }

                    if( m_LastOutPacketSize[0] < 10 )
                        WaitTicks = 1100; // 1000 ?
                    else if( m_LastOutPacketSize[0] < 100 )
                        WaitTicks = 1100; // 1000 ?
                    else
                        WaitTicks = 3100; // 3000 ?
                }
                else if( m_PacketSendDelay == 2 )
                {
                    if( m_LastOutPacketSize[0] < 10 )
                        WaitTicks = 1000;
                    else if( m_LastOutPacketSize[0] < 25 )
                        WaitTicks = 1100;
                    else if( m_LastOutPacketSize[0] < 50 )
                        WaitTicks = 1250;
                    else if( m_LastOutPacketSize[0] < 75 )
                        WaitTicks = 1450;
                    else if( m_LastOutPacketSize[0] < 100 )
                        WaitTicks = 1700;
                    else if( m_LastOutPacketSize[0] < 125 )
                        WaitTicks = 2000;
                    else if( m_LastOutPacketSize[0] < 150 )
                        WaitTicks = 2400;
                    else if( m_LastOutPacketSize[0] < 175 )
                        WaitTicks = 2900;
                    else if( m_LastOutPacketSize[0] <= m_MessageLength )
                        WaitTicks = 3450;
                    else
                        WaitTicks = 3600;
                }
                else if( m_PacketSendDelay == 1 )
                {
                    if( ( ( OutPacket.size( ) > 4 && OutPacket.substr( 0, 4 ) == "/me " ) || m_OutPackets.front( ).size( ) >= 100 ) && m_LastOutPacketSize[0] < 100 && m_LastOutPacketSize[1] < 100 )
                    {
                        if( OutPacket.size( ) > 4 && OutPacket.substr( 0, 4 ) == "/me " )
                            m_LastOutPacketSize[0] = m_MessageLength;
                        else
                            m_LastOutPacketSize[0] = m_OutPackets.front( ).size( );
                    }

                    if( m_LastOutPacketSize[0] < 10 )
                        WaitTicks = m_PacketDelaySmall;
                    else if( m_LastOutPacketSize[0] < 100 )
                        WaitTicks = m_PacketDelayMedium;
                    else
                        WaitTicks = m_PacketDelayBig;
                }
                else if( m_PacketSendDelay == 0 )
                {
                    if( m_LastOutPacketSize[0] < 10 )
                        WaitTicks = m_PacketDelaySmall;
                    else if( m_LastOutPacketSize[0] < 100 )
                        WaitTicks = m_PacketDelayMedium;
                    else
                        WaitTicks = m_PacketDelayBig;
                }

                if( GetTicks( ) - m_LastOutPacketTicks >= WaitTicks || WaitTicks == 0 )
                {
                    if( m_MaxOutPacketsSize > 30 )
                        --m_MaxOutPacketsSize;

                    if( !OutPacket.empty( ) )
                        CONSOLE_Print( m_ServerAliasWithUserName + " QUE: " + OutPacket );

                    m_OutPackets3.push_back( m_OutPackets.front( ) );
                    m_OutPackets3Channel.push_back( string( ) );
                    m_LastOutPacketSize[1] = m_LastOutPacketSize[0];

                    if( OutPacket.size( ) > 4 && OutPacket.substr( 0, 4 ) == "/me " )
                        m_LastOutPacketSize[0] = m_MessageLength;
                    else
                        m_LastOutPacketSize[0] = m_OutPackets.front( ).size( );

                    m_OutPackets.pop_front( );
                    m_LastOutPacketTicks = GetTicks( );

                    if( m_SpamBotMode > 0 && m_SpamBotNet > 0 )
                    {
                        for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                        {
                            if( ( *it )->GetServer( ) == m_Server && ( *it )->m_SpamBotNet == m_SpamBotNet )
                                ( *it )->m_LastMessageSendTicks = GetTicks( );
                        }
                    }
                }
                else
                    break;
            }
        }

        ProcessGameRefreshOnBnet( );

        // отправляем пакеты рехоста/рефреша игр

        while( !m_OutPackets2.empty( ) && ( m_HostDelay == 0 || GetTicks( ) - m_LastHostTime >= m_HostDelay ) )
        {
            if( m_OutPackets2.front( ).size( ) > 1 )
            {
                if( m_OutPackets2.front( )[1] == 2 )
                {
                    m_GameIsReady = false;
                    m_GameIsHosted = false;
                }
                else if( m_OutPackets2.front( )[1] == 10 )
                {
                    m_GameIsReady = true;
                    m_GameIsHosted = false;
                    m_LastGameUncreateTime = GetTicks( );
                }
                else if( m_OutPackets2.front( )[1] == 28 )
                {
                    if( m_ResetUncreateTime )
                    {
                        m_LastGameUncreateTime = GetTicks( ) - m_HostDelay;
                        m_ResetUncreateTime = false;
                    }

                    m_GameIsReady = true;
                    m_GameIsHosted = true;
                    m_LastRefreshTime = GetTicks( );
                    m_HostPacketSentTime = GetTime( );
                    m_HostPacketSent = true;
                }

                m_LastHostTime = GetTicks( );
                m_Socket->PutBytes( m_OutPackets2.front( ) );
            }
            else
            {
                m_GameIsReady = true;
                m_GameIsHosted = false;
                m_ResetUncreateTime = true;
                m_LastGameUncreateTime = GetTicks( ) - m_HostDelay;
                m_LastRefreshTime = m_UNM->TicksDifference( m_RefreshDelay );
            }

            m_OutPackets2.pop_front( );
        }

        // send packets

        while( !m_OutPackets3.empty( ) && ( m_PacketSendDelay == 4 || m_OutPackets3Channel.front( ).empty( ) || GetTicks( ) - m_LastOutPacketTicks3 > 1100 ) )
        {
            if( !m_OutPackets3Channel.front( ).empty( ) )
            {
                if( !m_UNM->m_LogReduction )
                    CONSOLE_Print( m_ServerAliasWithUserName + " joining channel [" + m_OutPackets3Channel.front( ) + "]" );

                if( m_OutPackets3Channel.front( ) != m_CurrentChannel )
                    m_MyChannel = false;

                m_LastOutPacketTicks3 = GetTicks( );

                if( m_UNM->m_CurrentGame && m_GameIsHosted && m_OutPackets2.empty( ) && ( m_AllowAutoRehost == 0 || m_RefreshDelay >= 300000 ) && ( m_AllowAutoRefresh == 2 || m_AllowAutoRefresh == 3 ) )
                {
                    m_GameIsHosted = false;
                    m_GameIsReady = true;
                }
            }
            else if( m_OutPackets3.front( ).size( ) > 5 && m_OutPackets3.front( )[1] == 14 )
            {
                bool MarkedPacket = false;
                uint32_t botid = 0;

                if( m_OutPackets3.front( )[0] == 254 || m_OutPackets3.front( )[0] == 252 )
                    MarkedPacket = true;

                if( m_OutPackets3.front( )[0] != BNET_HEADER_CONSTANT )
                    m_OutPackets3.front( )[0] = BNET_HEADER_CONSTANT;

                if( m_OutPackets3.front( )[m_OutPackets3.front( ).size( ) - 1] != 0 )
                {
                    botid = m_OutPackets3.front( )[m_OutPackets3.front( ).size( ) - 1];
                    m_OutPackets3.front( )[m_OutPackets3.front( ).size( ) - 1] = 0;
                }

                string OutPacket = string( m_OutPackets3.front( ).begin( ) + 4, m_OutPackets3.front( ).end( ) - 1 );

                if( !MarkedPacket )
                    GUI_Print( 3, OutPacket, string( ), botid );
                else
                    GUI_Print( 4, OutPacket, string( ), botid );
            }
            else if( m_OutPackets3.front( ).size( ) >= 23 && m_OutPackets3.front( )[1] == 9 )
            {
                if( m_OutPackets3.front( ).size( ) == 23 )
                    m_LastRequestSearchGame -= 1;
                else
                    m_LastRequestSearchGame -= 2;
            }

            m_Socket->PutBytes( m_OutPackets3.front( ) );
            m_OutPackets3.pop_front( );
            m_OutPackets3Channel.pop_front( );
        }

        // send a null packet every 60 seconds to detect disconnects

        if( GetTime( ) - m_LastNullTime >= 60 && GetTicks( ) - m_LastOutPacketTicks >= 60000 )
        {
            m_OutPackets3.push_back( m_Protocol->SEND_SID_NULL( ) );
            m_OutPackets3Channel.push_back( string( ) );
            m_LastNullTime = GetTime( );
        }

        m_Socket->DoSend( (fd_set *)send_fd, m_ServerAliasWithUserName );
        return;
    }

    if( m_Socket->GetConnecting( ) )
    {
        // we are currently attempting to connect to battle.net

        if( m_Socket->CheckConnect( ) )
        {
            // the connection attempt completed

            CONSOLE_Print( m_ServerAliasWithUserName + " connected" );
            m_Socket->PutBytes( m_Protocol->SEND_PROTOCOL_INITIALIZE_SELECTOR( ) );
            m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_INFO( m_War3Version, m_UNM->m_TFT, m_LocaleID, m_CountryAbbrev, m_Country ) );
            m_Socket->DoSend( (fd_set *)send_fd, m_ServerAliasWithUserName );
            m_LastNullTime = GetTime( );
            m_LastGameUncreateTime = m_UNM->TicksDifference( m_AutoRehostDelay );
            m_LastRefreshTime = m_UNM->TicksDifference( m_RefreshDelay );
            m_LastHostTime = m_UNM->TicksDifference( m_HostDelay );
            m_LastOutPacketTicks = 0;
            m_LastOutPacketTicks3 = 0;
            m_HostPacketSentTime = GetTime( );
            m_HostPacketSent = false;
            m_LogonSuccessfulTest = false;
            m_GameIsHosted = false;
            m_GameIsReady = false;
            m_ResetUncreateTime = true;
            m_GameNameAlreadyChanged = false;
            m_WaitResponse = true;
            m_LastGetPublicListTime = 0;
            m_LastGetPublicListTimeFull = 0;
            m_LastRequestSearchGame = 0;
            m_HideFirstEmptyMessage = true;
            m_OutPackets.clear( );
            m_OutPackets2.clear( );
            m_OutPackets3.clear( );
            m_OutPackets3Channel.clear( );
            m_QueueSearchBotPre.clear( );
            return;
        }
        else if( GetTime( ) - m_LastConnectionAttemptTime >= 5 )
        {
            // the connection attempt timed out (5 seconds)

            CONSOLE_Print( m_ServerAliasWithUserName + " connect timed out" );

            if( m_FirstTryConnect <= 1 )
            {
                CONSOLE_Print( m_ServerAliasWithUserName + " waiting 3 seconds to reconnect" );
                m_FastReconnect = true;
                m_LastDisconnectedTime = GetTime( ) - m_TimeWaitingToReconnect + 3;
            }
            else
            {
                if( m_HostCounterID == 1 && !AuthorizationPassed )
                {
                    string messageforqstr;
                    messageforqstr = "Не удалось выполнить вход, время ожидания ответа от сервера истекло.";
                    QString messageqstr = QString::fromUtf8( messageforqstr.c_str( ) );
                    emit gToLog->logonFailed( messageqstr );
                }

                CONSOLE_Print( m_ServerAliasWithUserName + " waiting " + UTIL_ToString( m_TimeWaitingToReconnect ) + " seconds to reconnect" );
                m_LastDisconnectedTime = GetTime( );
            }

            m_Socket->Reset( );
            m_WaitingToConnect = true;
            m_LogonSuccessfulTest = false;
            m_FirstTryConnect++;
            return;
        }
    }

    if( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && ( m_FirstConnect || ( ( AuthorizationPassed || m_FastReconnect ) && GetTime( ) >= m_LastDisconnectedTime && GetTime( ) - m_LastDisconnectedTime >= m_TimeWaitingToReconnect ) ) )
    {
        // attempt to connect to battle.net

        m_FirstConnect = false;
        m_FastReconnect = false;
        m_LoginErrorMessage = string( );

        if( !m_UNM->m_BindAddress.empty( ) )
            CONSOLE_Print( m_ServerAliasWithUserName + " attempting to bind to address [" + m_UNM->m_BindAddress + "]" );

        if( m_UNM->IsIccupServer( m_LowerServer ) && m_RunIccupLoader )
            CONSOLE_Print( m_ServerAliasWithUserName + " connecting to server [" + m_Server + "] on port " + UTIL_ToString( m_Port ) );
        else
            CONSOLE_Print( m_ServerAliasWithUserName + " connecting to server [" + m_Server + "] on port 6112" );

        if( m_ServerIP.empty( ) )
        {
            if( m_UNM->IsIccupServer( m_LowerServer ) && m_RunIccupLoader )
                m_Socket->Connect( m_UNM->m_BindAddress, "127.0.0.1", m_Port, m_ServerAliasWithUserName );
            else if( m_UNM->IsIccupServer( m_LowerServer ) )
                m_Socket->Connect( m_UNM->m_BindAddress, "178.218.214.114", 6112, m_ServerAliasWithUserName );
            else if( m_LowerServer.find( "rubattle" ) != string::npos )
                m_Socket->Connect( m_UNM->m_BindAddress, "rubattle.net", 6112, m_ServerAliasWithUserName );
            else if( m_LowerServer.find( "playground" ) != string::npos )
                m_Socket->Connect( m_UNM->m_BindAddress, "playground.ru", 6112, m_ServerAliasWithUserName );
            else if( m_LowerServer.find( "eurobattle" ) != string::npos )
                m_Socket->Connect( m_UNM->m_BindAddress, "server.eurobattle.net", 6112, m_ServerAliasWithUserName );
            else if( m_LowerServer.find( "it-ground" ) != string::npos || m_LowerServer.find( "itground" ) != string::npos )
                m_Socket->Connect( m_UNM->m_BindAddress, "bnet.it-ground.net", 6112, m_ServerAliasWithUserName );
            else
                m_Socket->Connect( m_UNM->m_BindAddress, m_Server, 6112, m_ServerAliasWithUserName );

            if( !m_Socket->HasError( ) )
            {
                m_ServerIP = m_Socket->GetIPString( );
                CONSOLE_Print( m_ServerAliasWithUserName + " resolved and cached server IP address " + m_ServerIP );
            }
        }
        else
        {
            // use cached server IP address since resolving takes time and is blocking

            CONSOLE_Print( m_ServerAliasWithUserName + " using cached server IP address " + m_ServerIP );

            if( m_UNM->IsIccupServer( m_LowerServer ) && m_RunIccupLoader )
                m_Socket->Connect( m_UNM->m_BindAddress, m_ServerIP, m_Port, m_ServerAliasWithUserName );
            else
                m_Socket->Connect( m_UNM->m_BindAddress, m_ServerIP, 6112, m_ServerAliasWithUserName );
        }

        m_WaitingToConnect = false;
        m_LastConnectionAttemptTime = GetTime( );
    }

    return;
}

void CBNET::UNMServerSocketUpdate( void *fd, void *send_fd )
{
    // we return at the end of each if statement so we don't have to deal with errors related to the order of the if statements
    // that means it might take a few ms longer to complete a task involving multiple steps (in this case, reconnecting) due to blocking or sleeping
    // but it's not a big deal at all, maybe 100ms in the worst possible case (based on a 50ms blocking time)

    if( m_Socket->HasError( ) )
    {
        // the socket has an error

        CONSOLE_Print( m_ServerAliasWithUserName + " disconnected from UNM server due to socket error" );

        if( m_FirstTryConnect == 0 )
        {
            CONSOLE_Print( m_ServerAliasWithUserName + " waiting 3 seconds to reconnect" );
            m_FastReconnect = true;
            m_LastDisconnectedTime = GetTime( ) - m_TimeWaitingToReconnect + 3;
        }
        else
        {
            if( m_HostCounterID == 1 && !AuthorizationPassed )
            {
                string messageforqstr = "Не удалось выполнить вход, ошибка сокета.";
                emit gToLog->logonFailed( QString::fromUtf8( messageforqstr.c_str( ) ) );
            }

            CONSOLE_Print( m_ServerAliasWithUserName + " waiting " + UTIL_ToString( m_TimeWaitingToReconnect ) + " seconds to reconnect" );
            m_LastDisconnectedTime = GetTime( );
        }

        m_Socket->Reset( );
        m_LoggedIn = false;
        m_WaitingToConnect = true;
        m_FirstTryConnect++;
        return;
    }

    if( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && !m_WaitingToConnect )
    {
        if( m_HostCounterID == 1 && !AuthorizationPassed )
        {
            string messageforqstr = "Не удалось выполнить вход." + m_LoginErrorMessage;
            emit gToLog->logonFailed( QString::fromUtf8( messageforqstr.c_str( ) ) );
        }

        // the socket was disconnected

        CONSOLE_Print( m_ServerAliasWithUserName + " disconnected from UNM server" );
        CONSOLE_Print( m_ServerAliasWithUserName + " waiting " + UTIL_ToString( m_TimeWaitingToReconnect ) + " seconds to reconnect" );
        m_Socket->Reset( );
        m_LastDisconnectedTime = GetTime( );
        m_LoggedIn = false;
        m_WaitingToConnect = true;
        m_FirstTryConnect = 2;
        return;
    }

    if( m_Socket->GetConnected( ) )
    {
        // the connection attempt timed out (5 seconds)

        if( m_WaitResponse && GetTime( ) - m_LastConnectionAttemptTime >= 5 )
        {
            CONSOLE_Print( m_ServerAliasWithUserName + " connect timed out" );

            if( m_UNM->IsIccupServer( m_LowerServer ) && ( ( m_RunIccupLoader && m_HostCounterID != 1 ) || ( LogonDataRunAH && m_HostCounterID == 1 ) ) )
                CONSOLE_Print( m_ServerAliasWithUserName + " не удалось присоединиться к IccReconnectLoader.exe - возможно запущено несколько экземпляров использующих общий порт" );

            if( m_FirstTryConnect == 0 )
            {
                CONSOLE_Print( m_ServerAliasWithUserName + " waiting 3 seconds to reconnect" );
                m_FastReconnect = true;
                m_LastDisconnectedTime = GetTime( ) - m_TimeWaitingToReconnect + 3;
            }
            else
            {
                if( m_HostCounterID == 1 && !AuthorizationPassed )
                {
                    string messageforqstr;

                    if( m_UNM->IsIccupServer( m_LowerServer ) && LogonDataRunAH )
                        messageforqstr = "Не удалось выполнить вход, ошибка сокета. Возможно запущено несколько экземпляров анти-хака (IccReconnectLoader.exe), использующих общий порт.";
                    else
                        messageforqstr = "Не удалось выполнить вход, время ожидания ответа от сервера истекло.";

                    QString messageqstr = QString::fromUtf8( messageforqstr.c_str( ) );
                    emit gToLog->logonFailed( messageqstr );
                }

                CONSOLE_Print( m_ServerAliasWithUserName + " waiting " + UTIL_ToString( m_TimeWaitingToReconnect ) + " seconds to reconnect" );
                m_LastDisconnectedTime = GetTime( );
            }

            m_Socket->Reset( );
            m_WaitingToConnect = true;
            m_WaitResponse = false;
            m_FirstTryConnect++;
            return;
        }

        // the socket is connected and everything appears to be working properly

        m_Socket->DoRecv( (fd_set *)fd, m_ServerAliasWithUserName );
        ExtractPacketsUNM( );
        ProcessPacketsUNM( );

        while( !m_OutPackets.empty( ) )
        {
            string OutPacket = string( );

            if( m_OutPackets.front( ).size( ) > 5 && m_OutPackets.front( )[1] == 14 )
                OutPacket = string( m_OutPackets.front( ).begin( ) + 4, m_OutPackets.front( ).end( ) - 1 );

            // check if at least one packet is waiting to be sent and if we've waited long enough to prevent flooding
            // this formula has changed many times but currently we wait 1 second if the last packet was "small", 3.5 seconds if it was "medium", and 4 seconds if it was "big"

            uint32_t WaitTicks = 500;

            if( GetTicks( ) - m_LastOutPacketTicks >= WaitTicks )
            {
                if( m_MaxOutPacketsSize > 30 )
                    --m_MaxOutPacketsSize;

                if( !OutPacket.empty( ) )
                    CONSOLE_Print( m_ServerAliasWithUserName + " [QUEUE] " + OutPacket );

                m_Socket->PutBytes( m_OutPackets.front( ) );
                m_OutPackets.pop_front( );
                m_LastOutPacketTicks = GetTicks( );

                if( m_SpamBotMode > 0 && m_SpamBotNet > 0 )
                {
                    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                    {
                        if( ( *it )->GetServer( ) == m_Server && ( *it )->m_SpamBotNet == m_SpamBotNet )
                            ( *it )->m_LastMessageSendTicks = GetTicks( );
                    }
                }
            }
            else
                break;
        }

        ProcessGameRefreshOnUNMServer( );

        // отправляем пакеты рехоста/рефреша игр

        while( !m_OutPackets2.empty( ) && ( m_HostDelay == 0 || GetTicks( ) - m_LastHostTime >= m_HostDelay ) )
        {
            if( m_OutPackets2.front( ).size( ) > 1 && m_OutPackets2.front( )[1] == 7 )
                m_LastRefreshTime = GetTicks( );

            m_LastHostTime = GetTicks( );
            m_Socket->PutBytes( m_OutPackets2.front( ) );
            m_OutPackets2.pop_front( );
        }

        m_Socket->DoSend( (fd_set *)send_fd, m_ServerAliasWithUserName );
        return;
    }

    if( m_Socket->GetConnecting( ) )
    {
        // we are currently attempting to connect to UNM server

        if( m_Socket->CheckConnect( ) )
        {
            // the connection attempt completed

            CONSOLE_Print( m_ServerAliasWithUserName + " connected" );
            m_Socket->PutBytes( m_UNMProtocol->SEND_CLIENT_INFO( ) );
            m_Socket->DoSend( (fd_set *)send_fd, m_ServerAliasWithUserName );
            m_LastNullTime = GetTime( );
            m_LastOutPacketTicks = GetTicks( );
            m_LastOutPacketTicks3 = GetTicks( );
            m_LastHostTime = m_UNM->TicksDifference( m_HostDelay );
            m_WaitResponse = true;
            m_OutPackets.clear( );
            m_OutPackets2.clear( );
            m_OutPackets3.clear( );
            m_OutPackets3Channel.clear( );
            return;
        }
        else if( GetTime( ) - m_LastConnectionAttemptTime >= 5 )
        {
            // the connection attempt timed out (5 seconds)

            CONSOLE_Print( m_ServerAliasWithUserName + " connect timed out" );

            if( m_FirstTryConnect <= 1 )
            {
                CONSOLE_Print( m_ServerAliasWithUserName + " waiting 3 seconds to reconnect" );
                m_FastReconnect = true;
                m_LastDisconnectedTime = GetTime( ) - m_TimeWaitingToReconnect + 3;
            }
            else
            {
                if( m_HostCounterID == 1 && !AuthorizationPassed )
                {
                    string messageforqstr;
                    messageforqstr = "Не удалось выполнить вход, время ожидания ответа от сервера истекло.";
                    QString messageqstr = QString::fromUtf8( messageforqstr.c_str( ) );
                    emit gToLog->logonFailed( messageqstr );
                }

                CONSOLE_Print( m_ServerAliasWithUserName + " waiting " + UTIL_ToString( m_TimeWaitingToReconnect ) + " seconds to reconnect" );
                m_LastDisconnectedTime = GetTime( );
            }

            m_Socket->Reset( );
            m_WaitingToConnect = true;
            m_FirstTryConnect++;
            return;
        }
    }

    if( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && ( m_FirstConnect || ( ( AuthorizationPassed || m_FastReconnect ) && GetTime( ) >= m_LastDisconnectedTime && GetTime( ) - m_LastDisconnectedTime >= m_TimeWaitingToReconnect ) ) )
    {
        // attempt to connect to UNM server

        m_FirstConnect = false;
        m_FastReconnect = false;
        m_LoginErrorMessage = string( );

        if( !m_UNM->m_BindAddress.empty( ) )
            CONSOLE_Print( m_ServerAliasWithUserName + " attempting to bind to address [" + m_UNM->m_BindAddress + "]" );

        CONSOLE_Print( m_ServerAliasWithUserName + " connecting to server [" + m_Server + "] on port " + UTIL_ToString( m_Port ) );

        if( m_ServerIP.empty( ) )
        {
            m_Socket->Connect( m_UNM->m_BindAddress, m_Server, m_Port, m_ServerAliasWithUserName );

            if( !m_Socket->HasError( ) )
            {
                m_ServerIP = m_Socket->GetIPString( );
                CONSOLE_Print( m_ServerAliasWithUserName + " resolved and cached server IP address " + m_ServerIP );
            }
        }
        else
        {
            // use cached server IP address since resolving takes time and is blocking

            CONSOLE_Print( m_ServerAliasWithUserName + " using cached server IP address " + m_ServerIP );
            m_Socket->Connect( m_UNM->m_BindAddress, m_ServerIP, m_Port, m_ServerAliasWithUserName );
        }

        m_WaitingToConnect = false;
        m_LastConnectionAttemptTime = GetTime( );
    }

    return;
}

void CBNET::ExtractPacketsUNM( )
{
    // extract as many packets as possible from the socket's receive buffer and put them in the m_Packets queue

    string *RecvBuffer = m_Socket->GetBytes( );
    BYTEARRAY Bytes = UTIL_CreateByteArray( (unsigned char *)RecvBuffer->c_str( ), RecvBuffer->size( ) );

    // a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

    while( Bytes.size( ) >= 4 )
    {
        // byte 0 is always 255

        if( Bytes[0] == UNM_HEADER_CONSTANT )
        {
            // bytes 2 and 3 (or 2, 3, 4 and 5) contain the length of the packet

            uint32_t Length;

            if( Bytes[1] == CUNMProtocol::GET_MAPLIST || Bytes[1] == CUNMProtocol::SET_MAPLIST || Bytes[1] == CUNMProtocol::ADD_MAPLIST || Bytes[1] == CUNMProtocol::REMOVE_MAPLIST )
                Length = UTIL_ByteArrayToUInt32( Bytes, false, 2 );
            else
                Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

            if( Length >= 4 )
            {
                if( Bytes.size( ) >= Length )
                {
                    m_Packets.push( new CCommandPacket( UNM_HEADER_CONSTANT, Bytes[1], BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) );
                    *RecvBuffer = RecvBuffer->substr( Length );
                    Bytes = BYTEARRAY( Bytes.begin( ) + Length, Bytes.end( ) );
                }
                else
                    return;
            }
            else
            {
                CONSOLE_Print( m_ServerAliasWithUserName + " error - received invalid packet from battle.net (bad length), disconnecting" );
                m_Socket->Disconnect( );
                return;
            }
        }
        else
        {
            CONSOLE_Print( m_ServerAliasWithUserName + " error - received invalid packet from battle.net (bad header constant), disconnecting" );
            m_Socket->Disconnect( );
            return;
        }
    }
}

void CBNET::ProcessPacketsUNM( )
{
    // process all the received packets in the m_Packets queue
    // this normally means sending some kind of response

    while( !m_Packets.empty( ) )
    {
        CCommandPacket *Packet = m_Packets.front( );
        m_Packets.pop( );

        if( Packet->GetPacketType( ) == UNM_HEADER_CONSTANT )
        {
            switch( Packet->GetID( ) )
            {
                case CUNMProtocol::PING_FROM_HOST:
                {
                    m_GetLastReceivePacketTicks = GetTicks( );
                    m_Socket->PutBytes( m_UNMProtocol->SEND_PONG_TO_HOST( m_UNMProtocol->RECEIVE_PING_FROM_HOST_RESPONSE( Packet->GetData( ) ) ) );
                    break;
                    }
                    case CUNMProtocol::CLIENT_INFO_RESPONSE:
                    {
                    m_GetLastReceivePacketTicks = GetTicks( );
                    unsigned char ReceiveClientInfo = m_UNMProtocol->RECEIVE_CLIENT_INFO_RESPONSE( Packet->GetData( ) );

                    if( ReceiveClientInfo <= 1 )
                    {
                        if( ReceiveClientInfo == 0 )
                            CONSOLE_Print( m_ServerAliasWithUserName + " Проверка версии прошла успешно, используется последняя версия протокола" );
                        else
                            CONSOLE_Print( m_ServerAliasWithUserName + " Проверка версии прошла успешно, рекомендуется обновить версию протокола" );

                        m_Socket->PutBytes( m_UNMProtocol->SEND_SMART_LOGON_BOT( 1, 0, m_UNM->m_HostPort, m_UserName, m_UserPassword, string( ), string( ), string( ) ) );
                    }
                    else
                    {
                        if( ReceiveClientInfo == 2 )
                            m_LoginErrorMessage = "Для подключения к серверу необходимо обновить версию протокола";
                        else
                            m_LoginErrorMessage = "Неизвестная ошибка при проверке версии протокола, подключение к серверу невозможно";

                        CONSOLE_Print( m_ServerAliasWithUserName + " " + m_LoginErrorMessage );
                        m_Socket->Disconnect( );
                        delete Packet;
                        return;
                    }

                    break;
                }
                case CUNMProtocol::SMART_LOGON_BOT_RESPONSE:
                {
                    m_GetLastReceivePacketTicks = GetTicks( );
                    unsigned char SmartLogonBot = m_UNMProtocol->RECEIVE_SMART_LOGON_BOT_RESPONSE( Packet->GetData( ) );

                    if( SmartLogonBot == SMART_LOGON_RESPONSE_SUCCESS )
                    {
                        m_WaitResponse = false;
                        m_FirstTryConnect = 2;

                        // logon successful

                        if( !AuthorizationPassed && m_HostCounterID == 1 )
                            emit gToLog->logonSuccessful( );
                        else if( !m_FirstLogon )
                            emit gToLog->bnetReconnect( m_HostCounterID );

                        CONSOLE_Print( m_ServerAliasWithUserName + " Авторизация прошла успешно" );
                        m_FirstLogon = false;
                        m_MyChannel = true;
                        m_FirstSuccessfulLogin = true;
                        m_GameStartedMessagePrinting = false;
                        m_LoggedIn = true;
                        m_LastOutPacketTicks = GetTicks( );
                        m_LastOutPacketTicks3 = GetTicks( );
                        m_LastHostTime = m_UNM->TicksDifference( m_HostDelay );
                        m_OutPackets.clear( );
                        m_OutPackets2.clear( );
                        m_OutPackets3.clear( );
                        m_OutPackets3Channel.clear( );
                    }
                    else
                    {
                        if( SmartLogonBot == SMART_LOGON_RESPONSE_DB_ERROR )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, ошибка при чтении данных из бд";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_WRONG_PASSWORD )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, указан неправильный пароль";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_BOTNAME_ALREADY_TAKEN )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, указанный псевдоним в настоящее время уже используется другим ботом";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_INVALID_BOTNAME )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, указан недопустимый псевдоним бота";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_INVALID_LOGIN )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, указан недопустимый логин";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_INVALID_PASSWORD )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, указан недопустимый пароль";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_INVALID_IP )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, указан недопустимый IP";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_INVALID_PORT )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, указан недопустимый порт";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_INVALID_MAPLISTNAME )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, указан недопустимый маплист";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_DENIED_ACCESS_TO_USE_PUBLIC_BOT )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, у вас недостаточно прав на подключение бота как публичного, попробуйте изменить тип на приватный (параметр BotType)";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_DENIED_ACCESS_TO_CREATE_GAME )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, у вас недостаточно прав на подключение бота с возможностью обработки запросов на создание игр, попробуйте изменить параметр CanCreateGame";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_DENIED_ACCESS_TO_USE_THIS_IP )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, у вас недостаточно прав на использование произвольного IP адреса, попробуйте изменить параметр IP (укажите свой настоящий IP или оставьте поле пустым)";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_DENIED_ACCESS_TO_USE_THIS_BOTNAME )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, у вас недостаточно прав на использование данного псевдонима для бота, попробуйте изменить параметр BotName (укажите свой логин или оставьте поле пустым)";
                        else if( SmartLogonBot == SMART_LOGON_RESPONSE_DENIED_ACCESS_TO_USE_THIS_MAPLISTNAME )
                            m_LoginErrorMessage = "Не удалось пройти авторизацию, у вас недостаточно прав на использование данного маплиста, попробуйте изменить параметр MapListName (укажите свой логин или оставьте поле пустым)";
                        else
                            m_LoginErrorMessage = "Неизвестная ошибка во время авторизации, подключение к серверу невозможно";

                        CONSOLE_Print( m_ServerAliasWithUserName + " " + m_LoginErrorMessage );
                        m_Socket->Disconnect( );
                        delete Packet;
                        return;
                    }

                    break;
                }
                case CUNMProtocol::GAME_REFRESH_RESPONSE:
                {
                    uint32_t GameNumber;
                    uint32_t GameCounter;
                    unsigned char result = m_UNMProtocol->RECEIVE_GAME_REFRESH_RESPONSE( Packet->GetData( ), GameNumber, GameCounter );

                    if( result <= 2 )
                        CONSOLE_Print( "Игра номер [" + UTIL_ToString( GameNumber ) + "] успешно обновлена на UNM Server" );
                    else
                        CONSOLE_Print( "Не удалось обновить игру [" + UTIL_ToString( GameNumber ) + "] на UNM Server" );

                    break;
                }
                case CUNMProtocol::CHAT_EVENT_RESPONSE:
                {
                    m_GetLastReceivePacketTicks = GetTicks( );
                    CIncomingChatEvent *ChatEvent = nullptr;
                    ChatEvent = m_UNMProtocol->RECEIVE_CHAT_EVENT_RESPONSE( Packet->GetData( ) );

                    if( ChatEvent )
                        ProcessChatEvent( ChatEvent );

                    delete ChatEvent;
                    ChatEvent = nullptr;
                    break;
                }
            }
        }

        delete Packet;
    }
}

void CBNET::ProcessGameRefreshOnBnet( )
{
    // refresh our current game in bnet
    // don't refresh if we're autohosting locally only

    if( m_UNM->m_CurrentGame && m_UNM->m_CurrentGame->m_RehostTime == 0 && !m_UNM->m_CurrentGame->GetGameLoading( ) && !m_UNM->m_CurrentGame->GetGameLoaded( ) && GetServerType( ) == 0 && m_GameIsReady && GetLoggedIn( ) && !GetUniqueName( ).empty( ) )
    {
        // send uncreate game packet every bnet_autorehostdelay ms
        // send refresh game packet every bnet_refreshdelay ms
        // мне отсылаем одновременно 2 пакета (если это даже позволено настройками для данного bnet-сервера) - потому что мы не знаем в каком порядке их обработает сервер если их отправить без минимальной задержки

        if( m_UNM->m_CurrentGame->GetGameState( ) == GAME_PUBLIC && m_GameIsHosted && m_AllowAutoRehost > 0 && ( m_AutoRehostDelay == 0 || GetTicks( ) - m_LastGameUncreateTime >= m_AutoRehostDelay ) && ( m_RehostTimeFix == 0 || GetTicks( ) - m_LastRehostTimeFix >= m_RehostTimeFix ) )
        {
            ClearQueueGameRehost( );

            if( m_AllowAutoRehost == 2 )
                QueueGameUncreate( );

            QueueEnterChat( );
            m_GameIsReady = false;

            if( m_RehostTimeFix != 0 )
            {
                for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                {
                    if( ( *it )->m_RehostTimeFix != 0 && ( *it )->GetServer( ) == GetServer( ) )
                        ( *it )->m_LastRehostTimeFix = GetTicks( );
                }
            }
        }
        else if( ( m_UNM->m_CurrentGame->GetGameState( ) == GAME_PUBLIC || !m_GameIsHosted ) && GetTicks( ) >= m_LastRefreshTime && !m_GameName.empty( ) && ( ( m_AllowAutoRefresh == 1 && ( m_RefreshDelay == 0 || GetTicks( ) - m_LastRefreshTime >= m_RefreshDelay ) ) || ( m_AllowAutoRefresh == 2 && !m_GameIsHosted ) || ( m_AllowAutoRefresh == 3 && !m_GameIsHosted && ( m_RefreshDelay == 0 || GetTicks( ) - m_LastRefreshTime >= m_RefreshDelay ) ) ) && ( m_RefreshTimeFix == 0 || GetTicks( ) - m_LastRefreshTimeFix >= m_RefreshTimeFix ) )
        {
            if( !m_GameNameAlreadyChanged )
            {
                m_UNM->m_CurrentGame->SetGameName( m_UNM->IncGameNr( m_UNM->m_CurrentGame->GetGameName( ), m_SpamSubBot, ( m_UNM->m_GameNameCharHide == 3 && m_SpamSubBot && m_AllowAutoRehost > 0 && m_AllowAutoRefresh > 0 && m_RefreshDelay < 300000 ), m_UNM->m_CurrentGame->GetNumHumanPlayers( ), m_UNM->m_CurrentGame->GetNumHumanPlayers( ) + m_UNM->m_CurrentGame->GetSlotsOpen( ) ) );
                m_UNM->m_HostCounter++;

                if( m_UNM->m_MaxHostCounter > 0 && m_UNM->m_HostCounter > m_UNM->m_MaxHostCounter )
                    m_UNM->m_HostCounter = 1;

                m_UNM->m_CurrentGame->SetHostCounter( m_UNM->m_HostCounter );
                m_UNM->m_CurrentGame->AddGameName( m_UNM->m_CurrentGame->GetGameName( ) );
                m_GameName = m_UNM->m_CurrentGame->GetGameName( );
                m_HostCounter = m_UNM->m_CurrentGame->GetHostCounter( );

                // всякое бывает :D

                uint32_t trycount = 0;
                bool needrename = false;

                while( trycount < m_AllBotIDs.size( ) )
                {
                    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                    {
                        if( ( *it )->GetLoggedIn( ) && ( *it )->GetServer( ) == GetServer( ) && ( *it )->m_AllowAutoRefresh > 0 && ( *it )->m_HostCounterID != m_HostCounterID && ( *it )->m_GameName == m_GameName )
                        {
                            needrename = true;
                            break;
                        }
                    }

                    if( needrename )
                    {
                        m_UNM->m_CurrentGame->SetGameName( m_UNM->IncGameNr( m_UNM->m_CurrentGame->GetGameName( ), m_SpamSubBot, ( m_UNM->m_GameNameCharHide == 3 && m_SpamSubBot && m_AllowAutoRehost > 0 && m_AllowAutoRefresh > 0 && m_RefreshDelay < 300000 ), m_UNM->m_CurrentGame->GetNumHumanPlayers( ), m_UNM->m_CurrentGame->GetNumHumanPlayers( ) + m_UNM->m_CurrentGame->GetSlotsOpen( ) ) );
                        m_UNM->m_HostCounter++;

                        if( m_UNM->m_MaxHostCounter > 0 && m_UNM->m_HostCounter > m_UNM->m_MaxHostCounter )
                            m_UNM->m_HostCounter = 1;

                        m_UNM->m_CurrentGame->SetHostCounter( m_UNM->m_HostCounter );
                        m_UNM->m_CurrentGame->AddGameName( m_UNM->m_CurrentGame->GetGameName( ) );
                        m_GameName = m_UNM->m_CurrentGame->GetGameName( );
                        m_HostCounter = m_UNM->m_CurrentGame->GetHostCounter( );
                        needrename = false;
                    }
                    else
                        break;

                    trycount++;
                }
            }
            else
            {
                for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                {
                    if( ( *it )->GetServer( ) == GetServer( ) )
                        ( *it )->m_GameNameAlreadyChanged = false;
                }
            }

            QueueGameRefresh( m_UNM->m_CurrentGame->GetGameState( ), m_GameName, m_UNM->m_CurrentGame->GetCreatorName( ), m_UNM->m_CurrentGame->GetDefaultOwnerName( ), m_UNM->m_CurrentGame->GetMap( ), m_UNM->m_CurrentGame->GetSaveGame( ), m_HostCounter, GetTime( ) - m_UNM->m_CurrentGame->GetCreationTime( ) );
            m_GameIsReady = false;

            // if we're creating a private game we don't need to send any game refresh messages so we can rejoin the chat immediately
            // unfortunately this doesn't work on PVPGN servers because they consider an enterchat message to be a gameuncreate message when in a game
            // so don't rejoin the chat if we're using PVPGN

            if( m_UNM->m_CurrentGame->GetGameState( ) != GAME_PUBLIC && GetPasswordHashType( ) != "pvpgn" )
                QueueEnterChat( );

            // only print the "game refreshed" message if we actually refreshed on at least one battle.net server

            if( m_UNM->m_CurrentGame->GetRefreshMessages( ) )
            {
                for( vector<CGamePlayer *> ::iterator it = m_UNM->m_CurrentGame->m_Players.begin( ); it != m_UNM->m_CurrentGame->m_Players.end( ); it++ )
                {
                    if( ( *it )->GetJoinedRealm( ) == GetServer( ) )
                        m_UNM->m_CurrentGame->SendChat( ( *it )->GetPID( ), gLanguage->GameRefreshed( ) );
                }
            }

            if( m_RefreshTimeFix != 0 )
            {
                for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                {
                    if( ( *it )->m_RefreshTimeFix != 0 && ( *it )->GetServer( ) == GetServer( ) )
                        ( *it )->m_LastRefreshTimeFix = GetTicks( );
                }
            }
        }
    }
}

void CBNET::ProcessGameRefreshOnUNMServer( )
{
    if( m_UNM->m_CurrentGame )
        m_UNM->m_CurrentGame->GameRefreshOnUNMServer( this );

    for( vector<CBaseGame *> ::iterator i = m_UNM->m_Games.begin( ); i != m_UNM->m_Games.end( ); i++ )
        ( *i )->GameRefreshOnUNMServer( this );
}

void CBNET::QueueGetGameList( uint32_t numGames )
{
    if( m_ServerType == 0 )
    {
        if( GetLoggedIn( ) )
        {
            m_OutPackets.push_back( m_Protocol->SEND_SID_GETADVLISTEX( string( ), numGames ) );
            m_LastRequestSearchGame++;
        }
    }
}

void CBNET::QueueGetGameList( string gameName )
{
    if( m_ServerType == 0 )
    {
        if( GetLoggedIn( ) )
        {
            m_OutPackets.push_back( m_Protocol->SEND_SID_GETADVLISTEX( gameName, 1 ) );
            m_LastRequestSearchGame += 2;
        }
    }
}

void CBNET::QueueJoinGame( string gameName )
{
    if( m_ServerType == 0 )
    {
        if( GetLoggedIn( ) )
        {
            m_OutPackets.push_back( m_Protocol->SEND_SID_NOTIFYJOIN( gameName ) );
            m_InChat = false;
            m_InGame = true;
        }
    }
}

CIncomingGameHost *CBNET::GetGame( uint32_t GameID )
{
    for( vector<CIncomingGameHost *>::iterator i = m_UNM->m_BNETs[m_MainBotID]->m_GPGames.begin( ); i != m_UNM->m_BNETs[m_MainBotID]->m_GPGames.end( ); i++ )
    {
        if( ( *i )->GetUniqueGameID( ) == GameID )
            return *i;
    }

    return nullptr;
}

bool CBNET::AddGame( CIncomingGameHost *game )
{
    QStringList GameInfoInit;
    GameInfoInit.append( QString::fromUtf8( UTIL_SubStrFix( game->GetGameName( ), 0, 0 ).c_str( ) ) );
    GameInfoInit.append( QString::fromUtf8( game->GetMapPathName( ).c_str( ) ) );
    GameInfoInit.append( QString::fromUtf8( game->GetHostName( ).c_str( ) ) );
    GameInfoInit.append( QString::fromUtf8( game->GetHostName( ).c_str( ) ) );

    // check for duplicates and rehosted games

    for( vector<CIncomingGameHost *>::iterator i = m_UNM->m_BNETs[m_MainBotID]->m_GPGames.begin( ); i != m_UNM->m_BNETs[m_MainBotID]->m_GPGames.end( ); i++ )
    {
        if( game->GetIPString( ) == ( *i )->GetIPString( ) && game->GetPort( ) == ( *i )->GetPort( ) )
        {
            // если игра (ip+port) уже имеется в списке - мы обновляем ее
            // при этом, если игра транслировалась в лан, и новый полученый statsring отличается от старого - мы отправляем в LAN пакет удаления игры
            // также пакет удаления игры из LAN будет отправляться, если новое название игры отличается от старого не только rehost counter'ом (функция UTIL_CompareGameNames возвращает false)
            // если в новом пакете игры карта отличается от той что была - мы прекращаем ее транслировать в лан
            // все эти проверки, отправка запроса на удаление игры из LAN, а также само обновления данных игры происходит в функции UpdateGame

            emit gToLog->updateGameToList( m_AllBotIDs, ( *i )->GetUniqueGameID( ), GameInfoInit, ( *i )->UpdateGame( game ) );
            return false;
        }
    }

    m_UNM->m_UniqueGameID++;
    game->SetUniqueGameID( m_UNM->m_UniqueGameID );
    emit gToLog->addGameToList( m_AllBotIDs, game->GetUniqueGameID( ), game->GetGProxy( ), game->GetGameIcon( ), GameInfoInit );
    m_UNM->m_BNETs[m_MainBotID]->m_GPGames.push_back( game );
    return true;
}

void CBNET::ResetGameList( )
{
    bool GameListReset = true;

    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
    {
        if( ( *i )->GetServer( ) == m_Server && ( *i )->GetLoggedIn( ) )
        {
            GameListReset = false;
            break;
        }
    }

    if( GameListReset )
    {
        m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchGame.clear( );
        m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGame.clear( );
        m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchGameTime.clear( );
        m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBot.clear( );
        m_UNM->m_BNETs[m_MainBotID]->m_QueueSearchBotBool.clear( );
        m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBot.clear( );
        m_UNM->m_BNETs[m_MainBotID]->m_OldQueueSearchBotTime.clear( );

        // на самом деле нет смысла очищать список игр при дисконекте - список очиститься в процессе сам, пока что в нем можно сохранить актуальные игры
/*
        for( vector<CIncomingGameHost *>::iterator i = m_UNM->m_BNETs[m_MainBotID]->m_GPGames.begin( ); i != m_UNM->m_BNETs[m_MainBotID]->m_GPGames.end( ); )
        {
            // don't forget to remove it from the LAN list first

            if( ( *i )->GetBroadcasted( ) )
                m_UNM->m_UDPSocket->Broadcast( 6112, m_UNM->m_GameProtocol->SEND_W3GS_DECREATEGAME( ( *i )->GetUniqueGameID( ) ), "[GPROXY: " + ( *i )->GetGameName( ) + "]" );

            emit gToLog->deleteGameFromList( m_UNM->m_BNETs[m_MainBotID]->m_AllBotIDs, ( *i )->GetUniqueGameID( ), m_UNM->m_CurrentGameID != ( *i )->GetUniqueGameID( ) ? 0 : 1 );
            delete *i;
            i = m_UNM->m_BNETs[m_MainBotID]->m_GPGames.erase( i );
            continue;
        }*/
    }
}

void CBNET::GetBnetBot( string botName, uint32_t &botStatus, bool &canMap, bool &supMap, bool &canPub, bool &canPrivate, bool &canInhouse, bool &canPubBy, bool &canDM, bool &supDM, string &statusString, string &description, string &accessName, string &site, string &vk, string &map, string &botLogin, vector<string> &maps )
{
    for( vector<CBNETBot *>::iterator i = m_BnetBots.begin( ); i != m_BnetBots.end( ); i++ )
    {
        if( botName == ( *i )->GetName( ) )
        {
            if( !( *i )->GetInit( ) )
                botStatus = 1;
            else if( !( *i )->GetLogon( ) )
                botStatus = 2;
            else if( ( *i )->GetResponseType( ) > 1 )
                botStatus = 3;
            else
                botStatus = 4;

            canMap = ( *i )->GetCanMap( );
            supMap = ( *i )->GetSupMap( );
            canPub = ( *i )->GetCanPub( );
            canPrivate = ( *i )->GetCanPrivate( );
            canInhouse = ( *i )->GetCanInhouse( );
            canPubBy = ( *i )->GetCanPubBy( );
            canDM = ( *i )->GetCanDM( );
            supDM = ( *i )->GetSupDM( );
            statusString = ( *i )->GetStatusString( );
            description = ( *i )->GetDescription( );
            accessName = ( *i )->GetAccessUpString( );
            site = ( *i )->GetSite( );
            vk = ( *i )->GetVK( );
            map = ( *i )->GetCurrentMap( );
            botLogin = ( *i )->GetLogin( );
            maps = ( *i )->GetMapList( );
            return;
        }
    }

    botStatus = 0;
}

void CBNET::AddCommandOnBnetBot( string botName, string command, uint32_t type )
{
    for( vector<CBNETBot *>::iterator i = m_BnetBots.begin( ); i != m_BnetBots.end( ); i++ )
    {
        if( botName == ( *i )->GetName( ) )
        {
            ( *i )->AddCommand( command, type );
            return;
        }
    }
}
