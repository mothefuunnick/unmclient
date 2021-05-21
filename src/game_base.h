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

#ifndef GAME_BASE_H
#define GAME_BASE_H

#include "gameslot.h"

//
// CBaseGame
//

class CTCPServer;
class CGameProtocol;
class CPotentialPlayer;
class CGamePlayer;
class CDBGamePlayer;
class CMap;
class CSaveGame;
class CReplay;
class CIncomingJoinPlayer;
class CIncomingAction;
class CIncomingChatPlayer;
class CIncomingMapSize;

class CBaseGame
{
public:
    CUNM *m_UNM;

protected:
    CTCPServer *m_Socket;                               // listening socket
    CGameProtocol *m_Protocol;                          // game protocol
    std::vector<CPotentialPlayer *> m_Potentials;       // vector of potential players (connections that haven't sent a W3GS_REQJOIN packet yet)
    std::vector<CGamePlayer *> m_DeletedPlayers;        // vector of deleted players
    std::queue<CIncomingAction *> m_Actions;            // queue of actions to be sent
    std::vector<std::string> m_Reserved;                // vector of player names with reserved slots (from the !hold command)
    std::vector<std::string> m_ReservedRealms;          // vector of player realms with reserved slots (from the !hold command)
    std::vector<unsigned char> m_ReservedS;             // vector of slot nr for the reserved players
    std::set<std::string> m_IPBlackList;                     // set of IP addresses to blacklist from joining (todotodo: convert to uint32_t for efficiency)
    std::vector<CGameSlot> m_EnforceSlots;              // vector of slots to force players to use (used with saved games)
    std::vector<PIDPlayer> m_EnforcePlayers;            // vector of pids to force players to use (used with saved games)
    std::vector<std::string> m_GameNames;
    CMap *m_Map;                                        // map data (this is a pointer to global data)
    CSaveGame *m_SaveGame;                              // savegame data (this is a pointer to global data)
    CReplay *m_Replay;                                  // replay
    bool m_SaveReplays;                                 // save replays or not
    bool m_Exiting;                                     // set to true and this class will be deleted next update
    uint16_t m_HostPort;                                // the port to host games on
    unsigned char m_GameState;                          // game state, public or private
    unsigned char m_VirtualHostPID;                     // virtual host's PID
    unsigned char m_ObsPlayerPID;                       // the obs player PID (if present)
    std::string m_ObsPlayerName;                        // the obs player name (if present)
    std::vector<unsigned char> m_FakePlayers;           // the fake player's PIDs (if present)
    std::vector<std::string> m_FakePlayersNames;        // the fake player's names (if present)
    std::vector<BYTEARRAY> m_FakePlayersActSelection;   // the fake player's action of select (if present)
    std::vector<std::string> m_FakePlayersOwner;        // the fake player's names (if present)
    std::vector<uint32_t> m_FakePlayersStatus;          // the fake player's status
    bool m_FakePlayersCopyUsed;
	unsigned char m_GProxyEmptyActions;
    std::string m_GameName;                             // game name
    std::string m_LastGameName;                         // last game name (the previous game name before it was rehosted)
    std::string m_OriginalGameName;                     // game name
    std::string m_VirtualHostName;                      // virtual host's name
    std::string m_OwnerName;                            // name of the player who owns this game (should be considered an admin)
    std::string m_DefaultOwner;                         // name of the player who wrote !owner or is given owner right by other in LOBBY only(should be considered as a TEMPORARY admin)
    std::string m_DefaultOwnerLower;                    // in lower case
    std::string m_BannedNames;                          // name of the player who was just banned
    std::string m_CreatorName;                          // name of the player who created this game
    std::string m_CreatorServer;                        // battle.net server the player who created this game was on
    std::string m_AnnounceMessage;                      // a message to be sent every m_AnnounceInterval seconds
    std::string m_StatString;                           // the stat std::string when the game started (used when saving replays)
    std::string m_KickVotePlayer;                       // the player to be kicked with the currently running kick vote
    std::string m_HCLCommandString;                     // the "HostBot Command Library" command std::string, used to pass a limited amount of data to specially designed maps
    uint32_t m_RandomSeed;                              // the random seed sent to the Warcraft III clients
    uint32_t m_HostCounter;                             // a unique game number
    uint32_t m_CurrentGameCounter;
    uint32_t m_GameCounter;
    uint32_t m_EntryKey;                                // random entry key for LAN, used to prove that a player is actually joining from LAN
    uint32_t m_Latency;                                 // the number of ms to wait between sending action packets (we queue any received during this time)
	uint32_t m_DynamicLatency;
	bool m_UseDynamicLatency;
	bool m_DynamicLatencyLagging;
	bool m_DynamicLatencyBotLagging;
	uint32_t m_DynamicLatencyMaxSync;
	uint32_t m_DynamicLatencyLastTicks;
	uint32_t m_DynamicLatencyConsolePrint;
	uint32_t m_LastAdminJoinAndFullTicks;
    uint32_t m_SyncLimit;                               // the maximum number of packets a player can fall out of sync before starting the lag screen
    uint32_t m_SyncCounter;                             // the number of actions sent so far (for determining if anyone is lagging)
    uint32_t m_GameTicks;                               // ingame ticks
    uint32_t m_CreationTime;                            // GetTime when the game was created
    uint32_t m_LastPingTime;                            // GetTime when the last ping was sent
    uint32_t m_LastDownloadTicks;                       // GetTicks when the last map download cycle was performed
    uint32_t m_AllowDownloads;                          // allow map downloads or not
    uint32_t m_DownloadCounter;                         // # of map bytes downloaded in the last second
    uint32_t m_LastDownloadCounterResetTicks;           // GetTicks when the download counter was last reset
    uint32_t m_LastAnnounceTime;                        // GetTime when the last announce message was sent
    uint32_t m_LastGameAnnounceTime;                    // GetTime when the last game announce message was sent
    uint32_t m_AnnounceInterval;                        // how many seconds to wait between sending the m_AnnounceMessage
    uint32_t m_LastAutoStartTime;                       // the last time we tried to auto start the game
    uint32_t m_AutoStartPlayers;                        // auto start the game when there are this many players or more
    uint32_t m_LastCountDownTicks;                      // GetTicks when the last countdown message was sent
    uint32_t m_CountDownCounter;                        // the countdown is finished when this reaches zero
    uint32_t m_StartedLoadingTicks;                     // GetTicks when the game started loading
    uint32_t m_StartPlayers;                            // number of players when the game started
    uint32_t m_LastLoadInGameResetTime;                 // GetTime when the "lag" screen was last reset when using load-in-game
    uint32_t m_LastActionSentTicks;                     // GetTicks when the last action packet was sent
    uint32_t m_LastActionLateBy;                        // the number of ticks we were late sending the last action packet by
    uint32_t m_StartedLaggingTime;                      // GetTime when the last lag screen started
    uint32_t m_LastLagScreenTime;                       // GetTime when the last lag screen was active (continuously updated)
    uint32_t m_LastLagScreenResetTime;
    uint32_t m_LastReservedSeen;                        // GetTime when the last reserved player was seen in the lobby
    uint32_t m_StartedKickVoteTime;                     // GetTime when the kick vote was started
    uint32_t m_StartedVoteStartTime;                    // GetTime when the votestart was started
    uint32_t m_GameOverTime;                            // GetTime when the game was over
    uint32_t m_LastPlayerLeaveTicks;                    // GetTicks when the most recent player left the game
    uint32_t m_ActuallyPlayerBeforeStartPrintDelay;     // Actually delay for print WaitingForPlayersBeforeStart messages
    uint32_t m_ActuallyStillDownloadingASPrintDelay;    // Actually delay for print StillDownloading messages
    uint32_t m_ActuallyNotSpoofCheckedASPrintDelay;     // Actually delay for print NotSpoofChecked messages
    uint32_t m_ActuallyNotPingedASPrintDelay;           // Actually delay for print NotPinged messages
    bool m_SlotInfoChanged;                             // if the slot info has changed and hasn't been sent to the players yet (optimization)
    uint32_t m_GameLoadedTime;                          // GetTime when the game was loaded
    uint32_t m_LastRedefinitionTotalLaggingTime;        // GetTime when total lagging time for each player was redefinition
    bool m_GameLoadedMessage;                           // GameLoad message shown
    uint32_t m_DownloadInfoTime;                        // GetTime when we last showed download info
    uint32_t m_SwitchTime;                              // GetTime when someone issued -switch
    bool m_Switched;                                    // if someone has switched
    uint32_t m_SwitchNr;                                // How many have accepted
    unsigned char m_SwitchS;                            // Who is switching
    unsigned char m_SwitchT;                            // Target of the switch
    unsigned char m_MapObservers;
    std::string m_AnnouncedPlayer;
    uint32_t m_LastDLCounterTicks;                      // GetTicks when the DL counter was last reset
    uint32_t m_DLCounter;                               // how many map download bytes have been sent since the last reset (for limiting download speed)
    bool m_RefreshMessages;                             // if we should display "game refreshed..." messages or not
    bool m_MuteAll;                                     // if we should stop forwarding ingame chat messages targeted for all players or not
    bool m_CountDownStarted;                            // if the game start countdown has started or not
	bool m_StartVoteStarted;
    bool m_GameLoading;                                 // if the game is currently loading or not
    bool m_GameLoaded;                                  // if the game has loaded or not
    bool m_LoadInGame;                                  // if the load-in-game feature is enabled or not
    bool m_Desynced;                                    // if the game has desynced or not
    bool m_Lagging;                                     // if the lag screen is active or not
    bool m_AutoSave;                                    // if we should auto save the game before someone disconnects
    bool m_LocalAdminMessages;                          // if local admin messages should be relayed or not
	bool m_FPEnable;
	uint32_t m_MoreFPs;
	uint32_t m_LastProcessedTicks;
	uint32_t m_UpdateDelay;
	uint32_t m_UpdateDelay2;
    bool m_DropIfDesync;
    std::string m_Servers;
    std::string m_FPHCLUser;
    uint32_t m_FPHCLTime;
    uint32_t m_FPHCLNumber;
    bool m_NonAdminCommandsGame;
    uint32_t m_LastGameUpdateGUITime;
    BYTEARRAY m_MapFlags;
    BYTEARRAY m_MapWidth;
    BYTEARRAY m_MapHeight;
    BYTEARRAY m_MapCRC;
    BYTEARRAY m_MapGameFlags;
    uint32_t m_LastRefreshTime;                         // for unm server we need have refresh counter for every game

public:
    uint32_t m_RehostTime;                              // GetTime when we issued !rehost
    bool m_Pause;
    bool m_PauseEnd;
    uint32_t m_LagScreenTime;                           // GetTime when the last lag screen was activated
    std::vector<CGamePlayer *> m_Players;               // vector of players
    std::vector<CGameSlot> m_Slots;                     // vector of slots
	bool m_ShowRealSlotCount;
    uint32_t m_LastPauseResetTime;
    uint32_t m_PlayersatStart;                          // how many players were at game start
    bool m_GameEndCountDownStarted;                     // if the game end countdown has started or not
    uint32_t m_GameEndCountDownCounter;                 // the countdown is finished when this reaches zero
    uint32_t m_GameEndLastCountDownTicks;               // GetTicks when the last countdown message was sent
    uint32_t m_Team1;                                   // Players in team 1
    uint32_t m_Team2;                                   // Players in team 2
    uint32_t m_Team3;                                   // Players in team 3
    uint32_t m_Team4;                                   // Players in team 4
    uint32_t m_TeamDiff;                                // Difference between teams (in players number)
    bool m_GameOverCanceled;                            // cancel game over timer
    bool m_GameOverDiffCanceled;                        // disable game over on team difference
    std::vector<std::string> m_Muted;                   // muted player list
    std::vector<std::string> m_CensorMuted;             // muted player list
    std::vector<uint32_t> m_CensorMutedTime;            // muted player list
    std::vector<uint32_t> m_CensorMutedSeconds;
	uint32_t m_CensorMutedLastTime;
	bool m_DetourAllMessagesToAdmins;
	bool m_NormalCountdown;
    bool m_ErrorListing;
	bool m_HCL;
    bool m_Listen;                                      // are we listening?
	bool m_OwnerJoined;
    BYTEARRAY m_ListenUser;                             // who is listening?
    uint32_t m_LastSlotsUnoccupied;                     // how many slots are not occupied
    bool m_AllSlotsOccupied;                            // if we're full
    bool m_AllSlotsAnnounced;                           // announced or not
    bool m_DownloadOnlyMode;                            // ignore lobby time limit, kick people having the map
    unsigned char m_LastPlayerJoined;                   // last player who joined
    bool m_FunCommandsGame;                             // allow use fun commands or not
	bool m_autoinsultlobby;
    uint32_t m_LastMars;                                // time when last mars was issued
    uint32_t m_LastPub;                                 // time when last pub was issued
    uint32_t m_LastNoMapPrint;                          // time when message "doesn't have the map and map downloads are disabled" was printed
    uint32_t m_SlotsOccupiedTime;                       // GetTime when we got full.
    uint32_t m_LastLeaverTicks;                         // GetTicks of the laster leaver
    std::string m_Server;                                    // Server we're on
	bool m_IntergameChat;
	bool m_DontDeleteVirtualHost;
	bool m_DontDeletePenultimatePlayer;
	uint32_t m_NumSpoofOfline;
    std::string m_NameSpoofOfline;
    std::string m_GetMapType;
    std::string m_GetMapPath;
    std::string m_GetMapName;
    std::string m_GetMapTypeLower;
    std::string m_GetMapPathLower;
	bool m_DotA;
	uint32_t m_GetMapNumPlayers;
	uint32_t m_GetMapNumTeams;
	unsigned char m_GetMapGameType;
    BYTEARRAY m_MapGameType;
    CBaseGame( CUNM *nUNM, CMap *nMap, CSaveGame *nSaveGame, uint16_t nHostPort, unsigned char nGameState, std::string nGameName, std::string nOwnerName, std::string nCreatorName, std::string nCreatorServer );
	virtual ~CBaseGame( );

    virtual std::vector<CGameSlot> GetEnforceSlots( )
	{
		return m_EnforceSlots;
	}
    virtual std::vector<PIDPlayer> GetEnforcePlayers( )
	{
		return m_EnforcePlayers;
	}
	virtual CSaveGame *GetSaveGame( )
	{
		return m_SaveGame;
	}
    virtual CMap *GetMap( )
    {
        return m_Map;
    }
	virtual uint16_t GetHostPort( )
	{
		return m_HostPort;
	}
	virtual unsigned char GetGameState( )
	{
		return m_GameState;
	}
	virtual unsigned char GetGProxyEmptyActions( )
	{
		return m_GProxyEmptyActions;
	}
    virtual std::string GetGameName( )
	{
		return m_GameName;
	}
    virtual std::string GetValidGameName( std::string server );
    virtual std::string GetBannedNames( )
	{
		return m_BannedNames;
	}
    virtual void AddBannedNames( std::string name );
    virtual std::string GetLastGameName( )
	{
		return m_LastGameName;
	}
    virtual void SetGameName( std::string nGameName );
	virtual void SetHostCounter( uint32_t nHostCounter )
	{
		m_HostCounter = nHostCounter;
	}
    virtual void SetHCL( std::string nHCL )
	{
		m_HCLCommandString = nHCL;
	}
	virtual uint32_t GetGameLoadedTime( )
	{
		return m_GameLoadedTime;
	}
	virtual uint32_t GetGameNr( );
    virtual std::string GetOriginalName( )
	{
		return m_OriginalGameName;
	}
    virtual std::string GetVirtualHostName( )
	{
		return m_VirtualHostName;
    }
    virtual std::string GetOwnerName( );
    virtual std::string GetLastOwnerName( );
    virtual std::string GetOwnerNames( )
	{
		return m_OwnerName;
	}
    virtual std::string GetDefaultOwnerName( )
	{
		return m_DefaultOwner;
	}
    virtual std::string GetCreatorName( )
	{
		return m_CreatorName;
	}
    virtual std::string GetHCL( )
	{
		return m_HCLCommandString;
	}
    virtual uint32_t GetGameTicks( )
    {
        return m_GameTicks;
    }
	virtual uint32_t GetCreationTime( )
	{
		return m_CreationTime;
	}
    virtual std::string GetCreatorServer( )
	{
		return m_CreatorServer;
	}
	virtual uint32_t GetHostCounter( )
	{
		return m_HostCounter;
	}
    virtual uint32_t GetCurrentGameCounter( )
    {
        return m_CurrentGameCounter;
    }
    virtual uint32_t GetGameCounter( )
    {
        return m_GameCounter;
    }
	virtual uint32_t GetLastLagScreenTime( )
	{
		return m_LastLagScreenTime;
    }
	virtual bool GetRefreshMessages( )
	{
		return m_RefreshMessages;
	}
	virtual bool GetCountDownStarted( )
	{
		return m_CountDownStarted;
	}
	virtual bool GetGameLoading( )
	{
		return m_GameLoading;
	}
	virtual bool GetGameLoaded( )
	{
		return m_GameLoaded;
	}
	virtual bool GetLagging( )
	{
		return m_Lagging;
	}
	virtual bool GetPause( )
	{
		return m_Pause;
	}
    virtual bool GetExiting( )
    {
        return m_Exiting;
    }
	virtual uint32_t GetSyncCounter( )
	{
		return m_SyncCounter;
	}
    virtual std::vector<CGamePlayer*> GetPlayers( )
	{
		return m_Players;
	}
    virtual void SetEnforceSlots( std::vector<CGameSlot> nEnforceSlots )
	{
		m_EnforceSlots = nEnforceSlots;
	}
    virtual void SetEnforcePlayers( std::vector<PIDPlayer> nEnforcePlayers )
	{
		m_EnforcePlayers = nEnforcePlayers;
	}
	virtual void SetExiting( bool nExiting )
	{
		m_Exiting = nExiting;
	}
	virtual void SetAutoStartPlayers( uint32_t nAutoStartPlayers )
	{
		m_AutoStartPlayers = nAutoStartPlayers;
	}

	virtual uint32_t GetNextTimedActionTicks( );
	virtual uint32_t GetSlotsOccupied( );
	virtual uint32_t GetNumSlotsT1( );
	virtual uint32_t GetNumSlotsT2( );
	virtual uint32_t GetSlotsOccupiedT1( );
	virtual uint32_t GetSlotsOccupiedT2( );
	virtual uint32_t GetSlotsOpen( );
	virtual uint32_t GetSlotsOpenT1( );
	virtual uint32_t GetSlotsOpenT2( );
	virtual uint32_t GetSlotsClosed( );
	virtual uint32_t GetNumPlayers( );
	virtual uint32_t GetNumFakePlayers( );
	virtual bool ObsPlayer( );
	virtual bool DownloadInProgress( );
	virtual uint32_t GetNumHumanPlayers( );
    virtual std::string GetDescription( );

	// processing functions

	virtual unsigned int SetFD( void *fd, void *send_fd, int *nfds );
	virtual bool Update( void *fd, void *send_fd );
	virtual void UpdatePost( void *send_fd );

	// generic functions to send packets to players

	virtual void Send( CGamePlayer *player, BYTEARRAY data );
	virtual void SendFromBuffer( CGamePlayer *player, BYTEARRAY data );
	virtual void Send( unsigned char PID, BYTEARRAY data );
	virtual void Send( BYTEARRAY PIDs, BYTEARRAY data );
	virtual void SendAll( BYTEARRAY data );
	virtual void SendAlly( unsigned char PID, BYTEARRAY data );
	virtual void SendEnemy( unsigned char PID, BYTEARRAY data );
	virtual void SendAdmin( BYTEARRAY data );

	// functions to send packets to players

    virtual void SendChat0( unsigned char fromPID, CGamePlayer *player, std::string message, bool privatemsg );
    virtual void SendChat( unsigned char fromPID, CGamePlayer *player, std::string message );
    virtual void SendChat( unsigned char fromPID, unsigned char toPID, std::string message );
    virtual void SendChat( CGamePlayer *fromplayer, CGamePlayer *toplayer, std::string message );
    virtual void SendChat( CGamePlayer *player, unsigned char toPID, std::string message );
    virtual void SendChat( CGamePlayer *player, std::string message );
    virtual void SendChat( unsigned char toPID, std::string message );
    virtual void SendChat2( unsigned char fromPID, CGamePlayer *player, std::string message );
    virtual void SendChat2( unsigned char fromPID, unsigned char toPID, std::string message );
    virtual void SendChat2( CGamePlayer *fromplayer, CGamePlayer *toplayer, std::string message );
    virtual void SendChat2( CGamePlayer *player, unsigned char toPID, std::string message );
    virtual void SendChat2( CGamePlayer *player, std::string message );
    virtual void SendChat2( unsigned char toPID, std::string message );
    virtual void SendAllChat( unsigned char fromPID, std::string message );
    virtual void SendAllyChat( unsigned char fromPID, std::string message );
    virtual void SendAdminChat( unsigned char fromPID, std::string message );
    virtual void SendAllChat( std::string message );
    virtual void SendAllChat( CGamePlayer *player, std::string message );
    virtual void SendLocalAdminChat( std::string message );
    virtual void SendAllyChat( std::string message );
    virtual void SendAdminChat( std::string message );
	virtual void SendAllSlotInfo( );
	virtual void SendVirtualHostPlayerInfo( CGamePlayer *player );
	virtual void SendFakePlayerInfo( CGamePlayer *player );
	virtual void SendObservePlayerInfo( CGamePlayer *player );
	virtual void SendAllActions( );
	virtual void SendWelcomeMessage( CGamePlayer *player, uint32_t HostCounterID );

	// events
	// note: these are only called while iterating through the m_Potentials or m_Players vectors
	// therefore you can't modify those vectors and must use the player's m_DeleteMe member to flag for deletion

	virtual void EventPlayerDeleted( CGamePlayer *player );
	virtual void EventPlayerDisconnectTimedOut( CGamePlayer *player );
	virtual void EventPlayerDisconnectPlayerError( CGamePlayer *player );
	virtual void EventPlayerDisconnectSocketError( CGamePlayer *player );
	virtual void EventPlayerDisconnectConnectionClosed( CGamePlayer *player );
    virtual void EventPlayerJoined( CPotentialPlayer *potential, CIncomingJoinPlayer *joinPlayer );
	virtual void EventPlayerLeft( CGamePlayer *player, uint32_t reason );
	virtual void EventPlayerLoaded( CGamePlayer *player );
	virtual void EventPlayerAction( CGamePlayer *player, CIncomingAction *action );
	virtual void EventPlayerKeepAlive( CGamePlayer *player );
	virtual void EventPlayerChatToHost( CGamePlayer *player, CIncomingChatPlayer *chatPlayer );
    virtual bool EventPlayerBotCommand( CGamePlayer *player, std::string command, std::string payload );
	virtual void EventPlayerChangeTeam( CGamePlayer *player, unsigned char team );
	virtual void EventPlayerChangeColour( CGamePlayer *player, unsigned char colour );
	virtual void EventPlayerChangeRace( CGamePlayer *player, unsigned char race );
	virtual void EventPlayerChangeHandicap( CGamePlayer *player, unsigned char handicap );
	virtual void EventPlayerDropRequest( CGamePlayer *player );
	virtual void EventPlayerMapSize( CGamePlayer *player, CIncomingMapSize *mapSize );
	virtual void EventPlayerPongToHost( CGamePlayer *player );

	// these events are called outside of any iterations

	virtual void EventGameStarted( );
	virtual void EventGameLoaded( );

	// other functions

    virtual void SetAnnounce( uint32_t interval, std::string message );
	virtual unsigned char GetSIDFromPID( unsigned char PID );
    virtual QString GetColoredNameFromPID( unsigned char PID );
	virtual CGamePlayer *GetPlayerFromPID( unsigned char PID );
	virtual CGamePlayer *GetPlayerFromSID( unsigned char SID );
    virtual CGamePlayer *GetPlayerFromName( std::string name, bool sensitive );
	virtual bool OwnerInGame( );
    virtual uint32_t GetPlayerFromNamePartial( std::string name, CGamePlayer **player );
    virtual CGamePlayer *GetPlayerFromNamePartial( std::string name );
	virtual CGamePlayer *GetPlayerFromColour( unsigned char colour );
    virtual QStringList GetGameDataInit( );
    virtual QStringList GetGameData( );
    virtual QList<QStringList> GetPlayersData( );
	virtual unsigned char GetNewPID( );
	virtual unsigned char GetNewColour( );
	virtual BYTEARRAY GetPIDs( );
	virtual BYTEARRAY GetAllyPIDs( unsigned char PID );
	virtual BYTEARRAY GetPIDs( unsigned char excludePID );
	virtual BYTEARRAY GetAdminPIDs( );
	virtual BYTEARRAY GetRootAdminPIDs( );
	virtual BYTEARRAY GetRootAdminPIDs( unsigned char omitpid );
	virtual BYTEARRAY AddRootAdminPIDs( BYTEARRAY PIDs, unsigned char omitpid );
	virtual BYTEARRAY Silence( BYTEARRAY PIDs );
	virtual unsigned char GetHostPID( );
	virtual unsigned char GetHostPIDForMessageFromBuffer( unsigned char PID );
	virtual unsigned char GetEmptySlot( bool reserved );
	virtual unsigned char GetEmptySlotForFakePlayers( );
	virtual unsigned char GetEmptySlotForObsPlayer( );
	virtual unsigned char GetEmptySlotAdmin( bool reserved );
	virtual unsigned char GetEmptySlot( unsigned char team, unsigned char PID );
	virtual void SwapSlots( unsigned char SID1, unsigned char SID2 );
	virtual void OpenSlot( unsigned char SID, bool kick );
	virtual void CloseSlot( unsigned char SID, bool kick );
	virtual void ComputerSlot( unsigned char SID, unsigned char skill, bool kick );
    virtual void ColourSlot( unsigned char SID, unsigned char colour, bool force = false );
	virtual void OpenAllSlots( );
	virtual void CloseAllSlots( );
	virtual void ShuffleSlots( );
    virtual void AddToSpoofed( std::string server, std::string name, bool sendMessage );
    virtual void AddToReserved( std::string name, std::string server, unsigned char nr );
    virtual void DelFromReserved( std::string name );
    virtual void DelTempOwner( std::string name );
    virtual void DelSpoofOfline( std::string name );
    virtual void DelBlacklistedName( std::string name );
    virtual void AddGameName( std::string name );
	virtual void AutoSetHCL( );
    virtual bool IsGameName( std::string name );
    virtual bool IsOwner( std::string name );
    virtual bool IsDefaultOwner( std::string name );
    virtual std::string CurrentOwners( );
    virtual bool IsSpoofOfline( std::string name );
    virtual bool IsBlacklisted( std::string blacklist, std::string name );
    virtual bool IsReserved( std::string name, std::string server );
	virtual bool IsDownloading( );
	virtual void StartCountDown( bool force );
	virtual void StartCountDownAuto( bool requireSpoofChecks );
    virtual void StopPlayers( std::string reason );
    virtual void StopLaggers( std::string reason );
    virtual void StopLagger( std::string reason );
	virtual void CreateVirtualHost( );
	virtual void DeleteVirtualHost( );
    virtual void CreateFakePlayer( std::string name );
    virtual void CreateObsPlayer( std::string name );
	virtual void CreateInitialFakePlayers( );
	virtual void DeleteFakePlayer( );
	virtual void DeleteObsPlayer( );
	virtual void DeleteAFakePlayer( );
    virtual bool IsMuted( std::string name );
    virtual void AddToMuted( std::string name );
    virtual void AddToCensorMuted( std::string name );
    virtual uint32_t IsCensorMuted( std::string name );
	virtual void ProcessCensorMuted( );
    virtual void DelFromCensorMuted( std::string name );
    virtual void DelFromMuted( std::string name );
    virtual unsigned char GetTeam( unsigned char SID );
	virtual void AddToListen( unsigned char user );
	virtual void DelFromListen( unsigned char user );
	virtual bool IsListening( unsigned char user );
	virtual void ReCalculateTeams( );
    virtual void InitSwitching( unsigned char PID, std::string nr );
	virtual void SwitchAccept( unsigned char PID );
	virtual void SwitchDeny( unsigned char PID );
    virtual unsigned char ReservedSlot( std::string name );
    virtual void ResetReservedSlot( std::string name );
	virtual uint32_t CountDownloading( );
	virtual uint32_t ShowDownloading( );
    virtual uint32_t DownloaderNr( std::string name );
	virtual void SwapSlotsS( unsigned char SID1, unsigned char SID2 );
    virtual bool IsClanFullMember( std::string name, std::string server );
    virtual std::string CustomReason( std::string reason );
    virtual std::string GetGameInfo( );
    virtual std::string GetCurrentDate( );
	virtual void SetDynamicLatency( );
	virtual bool ValidPID( unsigned char PID );
    virtual CGamePlayer *GetPlayerFromPID2( unsigned char PID );
    virtual CGamePlayer *GetPlayerFromSID2( unsigned char SID );
    virtual void GameRefreshOnUNMServer( CBNET *bnet );

    // Commands

    virtual void CommandLobbyBan( CGamePlayer *player, std::string Payload );
};

#endif
