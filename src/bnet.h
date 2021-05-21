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

#ifndef BNET_H
#define BNET_H

#include "gameslot.h"

//
// CBNET
//

class CTCPClient;
class CCommandPacket;
class CBNCSUtilInterface;
class CBNETProtocol;
class CUNMProtocol;
class CIncomingFriendList;
class CIncomingClanList;
class CIncomingChatEvent;
class CGamePlayer;
class CBNETBot;

class CBNET
{
public:
    CUNM *m_UNM;

private:
    CTCPClient *m_Socket;                                           // the connection to battle.net
    CBNETProtocol *m_Protocol;                                      // battle.net protocol
    CUNMProtocol *m_UNMProtocol;
    std::queue<CCommandPacket *> m_Packets;                              // queue of incoming packets
    CBNCSUtilInterface *m_BNCSUtil;                                 // the interface to the bncsutil library (used for logging into battle.net)
    std::deque<BYTEARRAY> m_OutPackets;                                  // queue of outgoing packets to be sent (to prevent getting kicked for flooding)
    std::deque<BYTEARRAY> m_OutPackets2;                                 // queue of outgoing immediate packets to be sent (to prevent getting kicked for flooding)
    std::deque<BYTEARRAY> m_OutPackets3;                                 // final queue of outgoing packets to be sent (to prevent getting kicked for flooding)
    std::deque<std::string> m_OutPackets3Channel;                             // info about packets in final queue of outgoing (channel)
    std::vector<CIncomingFriendList *> m_Friends;                        // vector of friends
    std::vector<CIncomingClanList *> m_Clans;                            // vector of clan members
    std::vector<std::string> m_ClanRecruits;
    std::vector<std::string> m_ClanPeons;
    std::vector<std::string> m_ClanGrunts;
    std::vector<std::string> m_ClanShamans;
    std::vector<std::string> m_ClanChieftains;
    std::vector<std::string> m_ClanOnline;
    std::vector<std::string> m_ChannelUsers;
	uint32_t ClanOnlineNumber;
    bool m_Exiting;                                                 // set to true and this class will be deleted next update
    std::string m_Server;                                                // battle.net server to connect
	std::string m_LowerServer;
    uint32_t m_TimeWaitingToReconnect;                              // the waiting time until the next reconnect in seconds (if disconnected)
    std::string m_ServerIP;                                              // battle.net/pvpgn server to connect to (the IP address so we don't have to resolve it every time we connect)
    std::string m_ServerAlias;                                           // battle.net/pvpgn server alias (short name, e.g. "USEast")
    std::string m_CDKeyROC;                                              // ROC CD key
    std::string m_CDKeyTFT;                                              // TFT CD key
    std::string m_CountryAbbrev;                                         // country abbreviation
    std::string m_Country;                                               // country
    uint32_t m_LocaleID;                                            // see: http://msdn.microsoft.com/en-us/library/0h88fahh%28VS.85%29.aspx
    std::string m_UserName;                                              // battle.net/pvpgn username
    std::string m_UserNameLower;                                         // battle.net/pvpgn username in lower case
    std::string m_UserPassword;                                          // battle.net/pvpgn password
    std::string m_FirstChannel;                                          // the first chat channel to join upon entering chat (note: we hijack this to store the last channel when entering a game)
    std::string m_CurrentChannel;                                        // the current chat channel
    std::string m_DefaultChannel;                                        // the default chat channel
    uint32_t m_HostName;                                            // host name
    std::string m_HostNameCustom;                                        // host custom name
    char m_CommandTrigger;                                          // the character prefix to identify commands
    char m_CommandTrigger1;                                         // the character prefix to identify commands
    char m_CommandTrigger2;                                         // the character prefix to identify commands
    char m_CommandTrigger3;                                         // the character prefix to identify commands
    char m_CommandTrigger4;                                         // the character prefix to identify commands
    char m_CommandTrigger5;                                         // the character prefix to identify commands
    char m_CommandTrigger6;                                         // the character prefix to identify commands
    char m_CommandTrigger7;                                         // the character prefix to identify commands
    char m_CommandTrigger8;                                         // the character prefix to identify commands
    char m_CommandTrigger9;                                         // the character prefix to identify commands
    unsigned char m_War3Version;                                    // custom warcraft 3 version for PvPGN users
    BYTEARRAY m_EXEVersion;                                         // custom exe version for PvPGN users
    BYTEARRAY m_EXEVersionHash;                                     // custom exe version hash for PvPGN users
    std::string m_EXEInfo;                                               // custom exe info for PvPGN users
    std::string m_PasswordHashType;                                      // password hash type for PvPGN users
    std::string m_PVPGNRealmName;                                        // realm name for PvPGN users (for mutual friend spoofchecks)
    uint32_t m_MaxMessageLength;                                    // maximum message length for PvPGN users
    uint32_t m_MessageLength;                                       // message length for PvPGN users
    uint32_t m_HostCounterID;                                       // the host counter ID to identify players from this realm
    uint32_t m_LastDisconnectedTime;                                // GetTime when we were last disconnected from battle.net
    uint32_t m_LastConnectionAttemptTime;                           // GetTime when we last attempted to connect to battle.net
    uint32_t m_LastNullTime;                                        // GetTime when the last null packet was sent for detecting disconnects
    uint32_t m_LastOutPacketTicks;                                  // GetTicks when the last packet was sent for the m_OutPackets queue
    uint32_t m_LastOutPacketTicks3;                                 // GetTicks when the last packet was sent for the m_OutPackets queue
    uint32_t m_LastOutPacketSize[3];                                // The size of the last sent packet
    uint32_t m_MaxOutPacketsSize;                                   // The maximum number of packets in the queue
    bool m_FastReconnect;
    uint32_t m_FirstTryConnect;
    bool m_FirstSuccessfulLogin;
    bool m_WaitingToConnect;                                        // if we're waiting to reconnect to battle.net after being disconnected
    bool m_LoggedIn;                                                // if we've logged into battle.net or not
    bool m_HoldFriends;                                             // whether to auto hold friends when creating a game or not
    bool m_HoldClan;                                                // whether to auto hold clan members when creating a game or not
	bool m_IccupReconnect;
	std::string m_IccupReconnectChannel;
	uint32_t m_PacketDelaySmall;
	uint32_t m_PacketDelayMedium;
	uint32_t m_PacketDelayBig;
	uint32_t m_LastMessageSendTicks;
    bool m_WaitResponse;
    std::string m_LoginErrorMessage;
    uint32_t m_IncorrectPasswordAttemptsNumber;
    std::string m_ComputerName;
    uint32_t m_ServerType;
    bool m_Reconnect;
    uint32_t m_GetLastReceivePacketTicks;
    uint32_t m_LastGetPublicListTime;
    uint32_t m_LastGetPublicListTimeFull;
    uint32_t m_LastGetPublicListTimeGeneral;
    bool m_InChat;									// if we've entered chat or not (but we're not necessarily in a chat channel yet)
    bool m_InGame;
    bool m_NewConnect;
    std::string m_NewServer;
    std::string m_NewLogin;
    std::string m_NewPassword;
    bool m_NewRunAH;
    std::vector<std::string> m_QueueSearchGame;
    std::vector<std::string> m_OldQueueSearchGame;
    std::vector<uint32_t> m_OldQueueSearchGameTime;
    std::vector<std::string> m_QueueSearchBot;
    std::vector<bool> m_QueueSearchBotBool;
    std::vector<std::string> m_OldQueueSearchBot;
    std::vector<uint32_t> m_OldQueueSearchBotTime;
    uint32_t m_LastRequestSearchGame;
    uint32_t m_LastSearchBotTicks;
    bool m_HideFirstEmptyMessage;

public:
    CBNET( CUNM *nUNM, std::string nServer, uint16_t nPort, uint32_t nTimeWaitingToReconnect, std::string nServerAlias, std::string nCDKeyROC, std::string nCDKeyTFT, std::string nCountryAbbrev, std::string nCountry, uint32_t nLocaleID, std::string nUserName, std::string nUserPassword, std::string nFirstChannel, uint32_t nHostName, std::string nHostNameCustom, std::string nCommandTrigger, bool nHoldFriends, bool nHoldClan, unsigned char nWar3Version, BYTEARRAY nEXEVersion, BYTEARRAY nEXEVersionHash, std::string nEXEInfo, std::string nPasswordHashType, std::string nPVPGNRealmName, bool nSpamBotFixBnetCommands, bool nSpamSubBot, bool nSpamSubBotChat, uint32_t nMainBotID, int32_t nMainBotChatID, uint32_t nSpamBotOnlyPM, uint32_t nSpamBotMessagesSendDelay, uint32_t nSpamBotMode, uint32_t nSpamBotNet, uint32_t nPacketSendDelay, uint32_t nMessageLength, uint32_t nMaxMessageLength, uint32_t nAllowAutoRehost, uint32_t  nAllowAutoRefresh, bool nAllowManualRehost, bool nAllowUseCustomMapGameType, bool nAllowUseCustomUpTime, uint32_t nCustomMapGameType, uint32_t nCustomUpTime, uint32_t nAutoRehostDelay, uint32_t nRefreshDelay, uint32_t nHostDelay, uint32_t nPacketDelaySmall, uint32_t nPacketDelayMedium, uint32_t nPacketDelayBig, bool nRunIccupLoader, bool nDisableMuteCommandOnBnet, uint32_t nDisableDefaultNotifyMessagesInGUI, uint32_t nServerType, bool nReconnect, uint32_t nHostCounterID );
	~CBNET( );

    std::vector<CBNETBot *> m_BnetBots;
    std::vector<std::string> m_QueueSearchBotPre;
    std::string m_ServerAliasWithUserName;                               // battle.net/pvpgn server alias with user name
    QList<unsigned int> m_AllBotIDs;
    std::vector<unsigned int> m_AllChatBotIDs;
    std::vector<CIncomingGameHost *> m_GPGames;
    std::string m_ExternalIP;
    bool m_LogonSuccessfulTest;
    uint32_t m_LogonSuccessfulTestTime;
    uint32_t m_HostPacketSentTime;
    bool m_HostPacketSent;
    bool m_MyChannel;
    std::string m_ReplyTarget;
    bool m_QuietRehost;
    bool m_FirstLogon;
    bool m_FirstConnect;
	uint32_t m_CustomMapGameType;
	uint32_t m_CustomUpTime;
	uint32_t m_LastGameUncreateTime;
	uint32_t m_LastRehostTimeFix;
	uint32_t m_RehostTimeFix;
    uint32_t m_LastRefreshTimeFix;
    uint32_t m_RefreshTimeFix;
	uint32_t m_LastRefreshTime;
	uint32_t m_LastHostTime;
	uint32_t m_HostCounter;
	uint32_t m_AutoRehostDelay;
	uint32_t m_RefreshDelay;
	uint32_t m_HostDelay;
    std::string m_GameName;
	bool m_GameIsHosted;
	bool m_GameIsReady;
    bool m_GameNameAlreadyChanged;
    bool m_ResetUncreateTime;
    bool m_RunIccupLoader;
    bool m_DisableMuteCommandOnBnet;
    uint32_t m_DisableDefaultNotifyMessagesInGUI;
    uint16_t m_Port;                                                // the port connection to battle.net server
	uint32_t m_AllowAutoRehost;
	uint32_t m_AllowAutoRefresh;
	bool m_AllowManualRehost;
	bool m_AllowUseCustomMapGameType;
	bool m_AllowUseCustomUpTime;
    uint32_t m_LastFriendListTime;                                  // GetTime when we got the friend list
	uint32_t m_PacketSendDelay;
    uint32_t m_MainBotID;
    int32_t m_MainBotChatID;
	uint32_t m_SpamBotMessagesSendDelay;
	uint32_t m_SpamBotNet;
	uint32_t m_SpamBotMode;
	uint32_t m_SpamBotOnlyPM;
	bool m_SpamBotFixBnetCommands;
	bool m_SpamSubBot;
    bool m_SpamSubBotChat;
	bool m_GameStartedMessagePrinting;
	bool m_GlobalChat;
    std::vector<uint32_t> m_GamelistRequestQueue;
    std::vector<std::string> m_QueueChatCommandQueue;
    std::vector<std::string> m_QueueChatCommandUserQueue;
    std::vector<bool> m_QueueChatCommandWhisperQueue;
    std::vector<std::string> m_UNMChatCommandQueue;

	bool GetExiting( )
	{
		return m_Exiting;
	}
	std::string GetServer( )
	{
		return m_Server;
	}
    uint32_t GetServerType( )
    {
        return m_ServerType;
    }
	std::string GetLowerServer( )
	{
		return m_LowerServer;
	}
	std::string GetServerAlias( )
	{
		return m_ServerAlias;
	}
	std::string GetCDKeyROC( )
	{
		return m_CDKeyROC;
	}
	std::string GetCDKeyTFT( )
	{
		return m_CDKeyTFT;
	}
    std::string GetName( )
    {
        return m_UserName;
    }
    std::string GetLowerName( )
    {
        return m_UserNameLower;
    }
	std::string GetFirstChannel( )
	{
		return m_FirstChannel;
	}
	std::string GetCurrentChannel( )
	{
		return m_CurrentChannel;
	}
	std::string GetPasswordHashType( )
	{
		return m_PasswordHashType;
	}
	uint32_t GetHostCounterID( )
	{
		return m_HostCounterID;
	}
	bool GetHoldFriends( )
	{
		return m_HoldFriends;
	}
	bool GetHoldClan( )
	{
		return m_HoldClan;
	}
    uint32_t GetTimeWaitingToReconnect( )
    {
        return m_TimeWaitingToReconnect;
    }
    uint32_t GetMessageLength( )
    {
        return m_MessageLength;
    }

	bool GetLoggedIn( );
	BYTEARRAY GetUniqueName( );
    QString GetCommandTrigger( );
    void GUI_Print( uint32_t type, std::string message, std::string user, uint32_t option );

	// processing functions

	unsigned int SetFD( void *fd, void *send_fd, int *nfds );
	bool Update( void *fd, void *send_fd );
	void ExtractPackets( );
	void ProcessPackets( );
    void ExtractPacketsUNM( );
    void ProcessPacketsUNM( );
	void ProcessChatEvent( CIncomingChatEvent *chatEvent );

	// functions to send packets to battle.net

	void SendGetFriendsList( );
	void SendGetClanList( );
	void QueueEnterChat( );
	void SendChatCommand( std::string chatCommand );
	void QueueChatCommand( std::string chatCommand );
    void QueueChatCommand( std::string chatCommand, bool manual );
	void QueueChatCommand( std::string chatCommand, std::string user, bool whisper );
    void QueueChatCommandDirect( std::string command, unsigned char manual, unsigned char botid );
    void QueueGameCreate( );
    void QueueGameRefresh( unsigned char state, std::string gameName, std::string creatorName, std::string ownerName, CMap *map, CSaveGame *saveGame, uint32_t hostCounter, uint32_t upTime );
    void QueueGameRefreshUNM( uint32_t GameNumber, bool GameLoading, bool GameLoaded, uint16_t HostPort, BYTEARRAY MapWidth, BYTEARRAY MapHeight, BYTEARRAY MapGameType, BYTEARRAY MapGameFlags, BYTEARRAY MapCRC, uint32_t HostCounter, uint32_t EntryKey, uint32_t UpTime, std::string GameName, std::string GameOwner, std::string MapPath, std::string HostName, std::string HostIP, std::string TGAImage, std::string MapPlayers, std::string MapDescription, std::string MapName, std::string Stats, unsigned char SlotsOpen, std::vector<CGameSlot> &GameSlots, std::vector<CGamePlayer *> players );
	void QueueGameUncreate( uint32_t GameNumber = 0 );
	void UnqueueChatCommand( std::string chatCommand );
    void ClearQueueGameRehost( );

	// other functions

    void AddChannelUser( std::string name );
    void DelChannelUser( std::string name );
    void ChangeChannel( bool disconnect = false );
    bool IsChannelUser( std::string name );
    void ChangeLogonData( std::string Server, std::string Login, std::string Password, bool RunAH );
    void ChangeLogonDataReal( std::string Server, std::string Login, std::string Password, bool RunAH );
	void HoldFriends( CBaseGame *game );
	void HoldClan( CBaseGame *game );
	bool IsFriend( std::string name );
	bool IsClanMember( std::string name );
	bool IsClanChieftain( std::string name );
	bool IsClanShaman( std::string name );
	bool IsClanGrunt( std::string name );
	bool IsClanPeon( std::string name );
	bool IsClanRecruit( std::string name );
	bool IsClanFullMember( std::string name );
    void UNMChatCommand( std::string Message );
	std::string GetPlayerFromNamePartial( std::string name );
	void BNETServerSocketUpdate( void *fd, void *send_fd );
	void UNMServerSocketUpdate( void *fd, void *send_fd );
    void ProcessGameRefreshOnBnet( );
    void ProcessGameRefreshOnUNMServer( );
    void QueueGetGameList( uint32_t numGames );
    void QueueGetGameList( std::string gameName );
    void QueueJoinGame( std::string gameName );
    bool GetInChat( )
    {
        return m_InChat;
    }
    bool GetInGame( )
    {
        return m_InGame;
    }
    CIncomingGameHost *GetGame( uint32_t GameID );
    bool AddGame( CIncomingGameHost *game );
    void ResetGameList( );
    void GetBnetBot( std::string botName, uint32_t &botStatus, bool &canMap, bool &supMap, bool &canPub, bool &canPrivate, bool &canInhouse, bool &canPubBy, bool &canDM, bool &supDM, std::string &statusString, std::string &description, std::string &accessName, std::string &site, std::string &vk, std::string &map, std::string &botLogin, std::vector<std::string> &maps );
    void AddCommandOnBnetBot( std::string botName, std::string command, uint32_t type );
};

#endif
