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
#include "bnet.h"
#include "map.h"
#include "packed.h"
#include "savegame.h"
#include "replay.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "game_base.h"
#include "next_combination.h"

using namespace std;

extern CLanguage *gLanguage;
extern ExampleObject *gToLog;

//
// sorting classes
//

class CGamePlayerSortAscByPing
{
public:
    bool operator( ) ( CGamePlayer *Player1, CGamePlayer *Player2 ) const
    {
        return Player1->GetPing( false ) < Player2->GetPing( false );
    }
};

class CGamePlayerSortDescByPing
{
public:
    bool operator( ) ( CGamePlayer *Player1, CGamePlayer *Player2 ) const
    {
        return Player1->GetPing( false ) > Player2->GetPing( false );
    }
};

//
// CBaseGame
//

CBaseGame::CBaseGame( CUNM *nUNM, CMap *nMap, CSaveGame *nSaveGame, uint16_t nHostPort, unsigned char nGameState, string nGameName, string nOwnerName, string nCreatorName, string nCreatorServer ) :
    m_UNM( nUNM ),
    m_SaveGame( nSaveGame ),
    m_Replay( nullptr ),
    m_Exiting( false ),
    m_HostPort( nHostPort ),
    m_GameState( nGameState ),
    m_VirtualHostPID( 255 ),
    m_ObsPlayerPID( 255 ),
    m_ObsPlayerName( m_UNM->m_ObsPlayerName ),
    m_GProxyEmptyActions( 0 ),
    m_GameName( nGameName ),
    m_LastGameName( nGameName ),
    m_VirtualHostName( m_UNM->m_VirtualHostName ),
    m_OwnerName( nOwnerName ),
    m_CreatorName( nCreatorName ),
    m_CreatorServer( nCreatorServer ),
    m_HCLCommandString( nMap->GetMapDefaultHCL( ) ),
    m_RandomSeed( GetTicks( ) ),
    m_EntryKey( UTIL_GenerateRandomNumberUInt32( 0, RAND_MAX ) ),
    m_Latency( m_UNM->m_Latency ),
    m_SyncLimit( m_UNM->m_SyncLimit ),
    m_SyncCounter( 0 ),
    m_GameTicks( 0 ),
    m_CreationTime( GetTime( ) ),
    m_LastPingTime( GetTime( ) ),
    m_LastDownloadTicks( GetTime( ) ),
    m_DownloadCounter( 0 ),
    m_LastDownloadCounterResetTicks( GetTime( ) ),
    m_LastAnnounceTime( 0 ),
    m_LastGameAnnounceTime( 0 ),
    m_AnnounceInterval( 0 ),
    m_LastAutoStartTime( GetTime( ) ),
    m_AutoStartPlayers( 0 ),
    m_LastCountDownTicks( 0 ),
    m_CountDownCounter( 0 ),
    m_StartedLoadingTicks( 0 ),
    m_StartPlayers( 0 ),
    m_LastActionSentTicks( 0 ),
    m_LastActionLateBy( 0 ),
    m_StartedLaggingTime( 0 ),
    m_LastLagScreenTime( 0 ),
    m_LastLagScreenResetTime( 0 ),
    m_LastReservedSeen( GetTime( ) ),
    m_StartedKickVoteTime( 0 ),
    m_StartedVoteStartTime( 0 ),
    m_GameOverTime( 0 ),
    m_LastPlayerLeaveTicks( 0 ),
    m_SlotInfoChanged( false ),
    m_RefreshMessages( m_UNM->m_RefreshMessages ),
    m_MuteAll( false ),
    m_CountDownStarted( false ),
    m_GameLoading( false ),
    m_GameLoaded( false ),
    m_LoadInGame( nMap->GetMapLoadInGame( ) ),
    m_Lagging( false ),
    m_AutoSave( m_UNM->m_AutoSave ),
    m_LocalAdminMessages( m_UNM->m_LocalAdminMessages )
{
    m_UNM->m_CurrentGameCounter++;
    m_CurrentGameCounter = m_UNM->m_CurrentGameCounter;
    m_GameCounter = m_CurrentGameCounter;
    m_Socket = new CTCPServer( );
    m_Protocol = new CGameProtocol( m_UNM );
    m_Map = new CMap( *nMap );
    m_SaveReplays = m_UNM->m_SaveReplays;
    m_LastRefreshTime = 0;

    if( m_SaveReplays && !m_SaveGame )
        m_Replay = new CReplay( );

    m_NonAdminCommandsGame = m_UNM->m_NonAdminCommandsGame;
    m_LastProcessedTicks = 0;
    m_UNM->m_FPNamesLast.clear( );

    // wait time of 1 minute = 0 empty actions required
    // wait time of 2 minutes = 1 empty action required
    // etc...

    if( m_UNM->m_ReconnectWaitTime != 0 )
    {
        m_GProxyEmptyActions = static_cast<unsigned char>( m_UNM->m_ReconnectWaitTime - 1 );

        // clamp to 9 empty actions (10 minutes)

        if( m_GProxyEmptyActions > 9 )
            m_GProxyEmptyActions = 9;
    }

    m_UNM->m_CurrentGameName.clear( );
    m_GameNames.push_back( m_GameName );
    m_OriginalGameName = nGameName;
    m_DefaultOwner = ( m_OwnerName.empty( ) || m_OwnerName.size( ) > 15 ) ? m_VirtualHostName : m_OwnerName;
    m_DefaultOwnerLower = m_DefaultOwner;
    transform( m_DefaultOwnerLower.begin( ), m_DefaultOwnerLower.end( ), m_DefaultOwnerLower.begin( ), static_cast<int(*)(int)>(tolower) );
    m_Server = nCreatorServer;
    m_UNM->m_HostCounter++;

    if( m_UNM->m_MaxHostCounter > 0 && m_UNM->m_HostCounter > m_UNM->m_MaxHostCounter )
        m_UNM->m_HostCounter = 1;

    m_HostCounter = m_UNM->m_HostCounter;
    m_UseDynamicLatency = m_UNM->m_UseDynamicLatency;
    m_DynamicLatency = m_Latency;
    m_DynamicLatencyLastTicks = 0;
    m_DynamicLatencyConsolePrint = 0;
    m_DynamicLatencyMaxSync = 0;
    m_DetourAllMessagesToAdmins = m_UNM->m_DetourAllMessagesToAdmins;
    m_NormalCountdown = m_UNM->m_NormalCountdown;
    m_LastAdminJoinAndFullTicks = GetTicks( );
    m_LastDLCounterTicks = GetTime( );
    m_CensorMutedLastTime = GetTime( ) + 5;
    m_AllowDownloads = m_UNM->m_AllowDownloads;
    m_DLCounter = 0;
    m_LastPauseResetTime = 0;
    m_LagScreenTime = 0;
    m_GameLoadedTime = 0;
    m_LastRedefinitionTotalLaggingTime = 0;
    m_ActuallyPlayerBeforeStartPrintDelay = m_UNM->m_PlayerBeforeStartPrintDelay;
    m_ActuallyStillDownloadingASPrintDelay = m_UNM->m_StillDownloadingASPrintDelay;
    m_ActuallyNotSpoofCheckedASPrintDelay = m_UNM->m_NotSpoofCheckedASPrintDelay;
    m_ActuallyNotPingedASPrintDelay = m_UNM->m_NotPingedASPrintDelay;
    m_OwnerJoined = false;
    m_HCL = false;
    m_Listen = false;
    m_GameLoadedMessage = false;
    m_StartVoteStarted = false;
    m_GameEndCountDownStarted = false;
    m_Servers = string( );
    m_AnnouncedPlayer = string( );
    m_Desynced = false;
    m_Pause = false;
    m_PauseEnd = false;
    m_DynamicLatencyLagging = false;
    m_DynamicLatencyBotLagging = false;
    m_RehostTime = 0;
    m_ShowRealSlotCount = m_UNM->m_ShowRealSlotCount;
    m_SwitchTime = 0;
    m_SwitchNr = 0;
    m_Switched = false;
    m_GameOverCanceled = false;
    m_GameOverDiffCanceled = false;
    m_Team1 = 0;
    m_Team2 = 0;
    m_Team3 = 0;
    m_Team4 = 0;
    m_TeamDiff = 0;
    m_DropIfDesync = m_UNM->m_DropIfDesync;
    m_LastPlayerJoined = 255;
    m_FunCommandsGame = m_UNM->m_FunCommandsGame;
    m_autoinsultlobby = m_UNM->m_autoinsultlobby;
    m_LastMars = m_UNM->TimeDifference( 10 );
    m_LastPub = m_UNM->TimeDifference( m_UNM->m_PubGameDelay );
    m_LastNoMapPrint = m_UNM->TimeDifference( m_UNM->m_NoMapPrintDelay );
    m_AllSlotsOccupied = false;
    m_AllSlotsAnnounced = false;
    m_DownloadOnlyMode = false;
    m_IntergameChat = false;
    m_FPEnable = m_UNM->m_FakePlayersLobby;
    m_MoreFPs = m_UNM->m_MoreFPsLobby;
    m_DontDeleteVirtualHost = m_UNM->m_DontDeleteVirtualHost;
    m_DontDeletePenultimatePlayer = m_UNM->m_DontDeletePenultimatePlayer;
    m_FakePlayersCopyUsed = false;
    m_NumSpoofOfline = 0;
    m_NameSpoofOfline = string( );
    m_MapObservers = m_Map->GetMapObservers( );
    m_LastGameUpdateGUITime = GetTime( );

    if( m_SaveGame )
    {
        m_EnforceSlots = m_SaveGame->GetSlots( );
        m_Slots = m_SaveGame->GetSlots( );

        // the savegame slots contain player entries
        // we really just want the open/closed/computer entries
        // so open all the player slots

        for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
        {
            if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && ( *i ).GetComputer( ) == 0 )
            {
                ( *i ).SetPID( 0 );
                ( *i ).SetDownloadStatus( 255 );
                ( *i ).SetSlotStatus( SLOTSTATUS_OPEN );
            }
        }
    }
    else
        m_Slots = m_Map->GetSlots( );

    if( !m_UNM->m_IPBlackListFile.empty( ) )
    {
        ifstream in;

#ifdef WIN32
        in.open( UTIL_UTF8ToCP1251( m_UNM->m_IPBlackListFile.c_str( ) ) );
#else
        in.open( m_UNM->m_IPBlackListFile );
#endif

        if( in.fail( ) )
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] error loading IP blacklist file [" + m_UNM->m_IPBlackListFile + "]", 6, m_CurrentGameCounter );
        else
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] loading IP blacklist file [" + m_UNM->m_IPBlackListFile + "]", 6, m_CurrentGameCounter );
            string Line;

            while( !in.eof( ) )
            {
                getline( in, Line );
                Line = UTIL_FixFileLine( Line, true );

                // ignore blank lines and comments

                if( Line.empty( ) || Line[0] == '#' )
                    continue;

                // ignore lines that don't look like IP addresses

                if( Line.find_first_not_of( "1234567890." ) != string::npos || !m_UNM->IPIsValid( Line ) )
                    continue;

                m_IPBlackList.insert( Line );
            }

            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] loaded " + UTIL_ToString( m_IPBlackList.size( ) ) + " lines from IP blacklist file", 6, m_CurrentGameCounter );
        }

        in.close( );
    }

    m_ErrorListing = false;

    // start listening for connections

    if( !m_UNM->m_BindAddress.empty( ) )
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] attempting to bind to address [" + m_UNM->m_BindAddress + "]", 6, m_CurrentGameCounter );
    else
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] attempting to bind to all available addresses", 6, m_CurrentGameCounter );

    if( m_Socket->Listen( string( ), m_HostPort, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]" ) )
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] listening on port " + UTIL_ToString( m_HostPort ), 6, m_CurrentGameCounter );
    else
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] error listening on port " + UTIL_ToString( m_HostPort ), 6, m_CurrentGameCounter );
        m_Socket->Reset( );
        m_Exiting = true;
        m_ErrorListing = true;
    }

    m_FPHCLNumber = 0;
    m_FPHCLUser = string( );
    m_FPHCLTime = GetTime( );
    m_MapFlags.push_back( m_Map->GetMapFlags( ) );
    m_MapWidth = m_Map->GetMapWidth( );
    m_MapHeight = m_Map->GetMapHeight( );
    m_MapCRC = m_Map->GetMapCRC( );
    m_MapGameFlags = m_Map->GetMapGameFlags( );
    m_GetMapType = m_Map->GetMapType( );
    m_GetMapPath = m_Map->GetMapPath( );
    m_GetMapName = m_GetMapPath;

    string::size_type MapNameStart = m_GetMapName.rfind( "\\" );

    if( MapNameStart != string::npos )
        m_GetMapName = m_GetMapName.substr( MapNameStart + 1 );

    if( m_GetMapName.size( ) > 4 && ( m_GetMapName.substr( m_GetMapName.size( ) - 4 ) == ".w3x" || m_GetMapName.substr( m_GetMapName.size( ) - 4 ) == ".w3m" ) )
        m_GetMapName = m_GetMapName.substr( 0, m_GetMapName.size( ) - 4 );

    m_GetMapGameType = m_Map->GetMapGameType( );
    m_MapGameType.push_back( m_GetMapGameType );
    m_MapGameType.push_back( 0 );
    m_MapGameType.push_back( 0 );
    m_MapGameType.push_back( 0 );
    m_GetMapNumPlayers = m_Map->GetMapNumPlayers( );
    m_GetMapNumTeams = m_Map->GetMapNumTeams( );
    m_GetMapTypeLower = m_GetMapType;
    m_GetMapTypeLower = UTIL_StringToLower( m_GetMapTypeLower );
    m_GetMapPathLower = m_GetMapPath;
    m_GetMapPathLower = UTIL_StringToLower( m_GetMapPathLower );

    if( m_GetMapPathLower.find( "dota v" ) != string::npos && m_GetMapPathLower.find( "lod" ) == string::npos && m_GetMapPathLower.find( "omg" ) == string::npos && m_GetMapPathLower.find( "fun" ) == string::npos && m_GetMapPathLower.find( "mod" ) == string::npos )
        m_DotA = true;

    m_UpdateDelay = 50;
    uint32_t UpdateDelay = 50;

    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
    {
        if( ( *i )->GetServerType( ) == 0 )
        {
            UpdateDelay = ( *i )->m_AutoRehostDelay;

            if( UpdateDelay > ( *i )->m_RefreshDelay )
                UpdateDelay = ( *i )->m_RefreshDelay;

            if( UpdateDelay < ( *i )->m_HostDelay )
                UpdateDelay = ( *i )->m_HostDelay;

            if( ( *i )->m_RehostTimeFix > 0 && ( *i )->m_RehostTimeFix < UpdateDelay )
                UpdateDelay = ( *i )->m_RehostTimeFix;

            if( ( *i )->m_RefreshTimeFix > 0 && ( *i )->m_RefreshTimeFix < UpdateDelay )
                UpdateDelay = ( *i )->m_RefreshTimeFix;

            if( UpdateDelay < m_UpdateDelay )
                m_UpdateDelay = UpdateDelay;

            ( *i )->m_GameName = m_GameName;
            ( *i )->m_HostCounter = m_HostCounter;
            ( *i )->m_GameIsHosted = false;
            ( *i )->m_GameIsReady = true;
            ( *i )->m_ResetUncreateTime = true;

            if( !( *i )->m_SpamSubBot )
                ( *i )->m_GameNameAlreadyChanged = true;
            else
                ( *i )->m_GameNameAlreadyChanged = false;
        }
        else if( !( *i )->m_SpamBotMode )
        {
            ( *i )->m_GameName = m_GameName;
            ( *i )->m_HostCounter = m_HostCounter;
        }
    }

    if( m_UpdateDelay > 50 )
        m_UpdateDelay = 50;
    else if( m_UpdateDelay < 1 )
        m_UpdateDelay = 1;

    m_UpdateDelay2 = 50;

    if( m_UNM->m_CountDownMethod > 0 )
        m_UpdateDelay2 = 5;

    if( m_UpdateDelay < m_UpdateDelay2 )
        m_UpdateDelay2 = m_UpdateDelay;

    if( !m_UNM->m_ObsPlayerName.empty( ) && m_GetMapType.find( "obs" ) != string::npos && m_GetMapType.find( "noobs" ) == string::npos )
        CreateObsPlayer( string( ) );
}

CBaseGame::~CBaseGame( )
{
    emit gToLog->deleteGame( m_CurrentGameCounter );

    // save replay
    // todotodo: put this in a thread

    if( m_Replay && ( m_GameLoading || m_GameLoaded ) )
    {
        string GameName = m_UNM->IncGameNrDecryption( m_GameName );
        time_t Now = time( nullptr );
        char Time[17];
        memset( Time, 0, sizeof( char ) * 17 );
        strftime( Time, sizeof( char ) * 17, "%Y-%m-%d %H-%M", localtime( &Now ) );
        float iTime = (float)( GetTime( ) - m_GameLoadedTime ) / 60;

        if( m_DotA )
        {
            if( iTime > 2 )
                iTime -= 2;
            else
                iTime = 1;
        }

        string sTime = UTIL_ToString( iTime ) + "min";
        string s = m_UNM->GetRehostChar( );
        string str = m_UNM->m_InvalidReplayChars;

        if( GameName.size( ) > 5 )
        {
            string::size_type DPos = GameName.rfind( s[0] );

            if( DPos != string::npos && GameName.find( " " + s[0] ) != string::npos )
                GameName = GameName.substr( 0, DPos - 1 );
        }

        if( !str.empty( ) )
        {
            uint32_t f = 0;
            string sf;
            string::size_type found = GameName.find_first_of( str );

            while( f < str.size( ) && found != string::npos )
            {
                sf = str[f];
                UTIL_Replace( GameName, sf, string( ) );
                found = GameName.find_first_of( str );
                f++;
            }
        }

        m_Replay->BuildReplay( GameName, m_StatString, m_UNM->m_ReplayWar3Version, m_UNM->m_ReplayBuildNumber );
        string ReplayPath = m_UNM->m_ReplayPath;
        string ReplayName = string( );

        if( m_UNM->m_ReplaysByName )
            ReplayName = UTIL_FileSafeName( GameName + " " + sTime + " dated " + string( Time ) + " (1." + UTIL_ToString( m_UNM->m_ReplayWar3Version ) + ").w3g" );
        else
            ReplayName = UTIL_FileSafeName( "UNM " + sTime + " " + string( Time ) + " " + GameName + " (1." + UTIL_ToString( m_UNM->m_ReplayWar3Version ) + ").w3g" );

        ReplayName = ReplayPath + ReplayName;

        if( ReplayPath.size( ) > 1 && ( ReplayPath.substr( ReplayPath.size( ) - 1 ) == "\\" || ReplayPath.substr( ReplayPath.size( ) - 1 ) == "/" ) )
        {
            ReplayPath = ReplayPath.substr( 0, ReplayPath.size( ) - 1 );
            std::wstring WReplayPath = std::wstring_convert<std::codecvt_utf8<wchar_t>>( ).from_bytes(ReplayPath);

            if( !std::filesystem::exists( WReplayPath ) )
            {
                if( std::filesystem::create_directory( WReplayPath ) )
                    CONSOLE_Print( "[UNM] Была создана папка \"" + ReplayPath + "\"" );
                else
                    CONSOLE_Print( "[UNM] Не удалось создать папку \"" + ReplayPath + "\"" );
            }
        }

        m_Replay->Save( m_UNM->m_TFT, ReplayName );
    }

    delete m_Socket;
    delete m_Protocol;
    delete m_Map;
    delete m_Replay;

    for( vector<CPotentialPlayer *> ::iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); i++ )
        delete *i;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        delete *i;

    for( vector<CGamePlayer *> ::iterator i = m_DeletedPlayers.begin( ); i != m_DeletedPlayers.end( ); i++ )
        delete *i;

    while( !m_Actions.empty( ) )
    {
        delete m_Actions.front( );
        m_Actions.pop( );
    }

    m_Reserved.clear( );
    m_ReservedRealms.clear( );
    m_ReservedS.clear( );
    m_IPBlackList.clear( );
    m_EnforceSlots.clear( );
    m_EnforcePlayers.clear( );
    m_GameNames.clear( );
    m_FakePlayers.clear( );
    m_FakePlayersNames.clear( );
    m_FakePlayersActSelection.clear( );
    m_FakePlayersOwner.clear( );
    m_FakePlayersStatus.clear( );
    m_Players.clear( );
    m_Slots.clear( );
    m_Muted.clear( );
    m_CensorMuted.clear( );
    m_CensorMutedTime.clear( );
    m_CensorMutedSeconds.clear( );
}

string CBaseGame::GetValidGameName( string server )
{
    if( !m_UNM->IsBnetServer( server ) )
        return m_UNM->IncGameNrDecryption( m_GameName );

    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
    {
        if( !( *i )->m_SpamSubBot && ( *i )->GetServer( ) == server )
            return m_UNM->IncGameNrDecryption( ( *i )->m_GameName, true );
    }

    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
    {
        if( ( *i )->GetServer( ) == server )
            return m_UNM->IncGameNrDecryption( ( *i )->m_GameName, true );
    }

    return m_UNM->IncGameNrDecryption( m_GameName );
}

uint32_t CBaseGame::GetNextTimedActionTicks( )
{
    // return the number of ticks (ms) until the next "timed action", which for our purposes is the next game update
    // the main UNM loop will make sure the next loop update happens at or before this value
    // note: there's no reason this function couldn't take into account the game's other timers too but they're far less critical
    // warning: this function must take into account when actions are not being sent (e.g. during loading or lagging)

    if( m_Lagging )
        return 50;
    else if( !m_GameLoading && !m_GameLoaded )
    {
        if( m_CountDownStarted )
            return m_UpdateDelay2;
        else
            return m_UpdateDelay;
    }

    uint32_t TicksSinceLastUpdate = GetTicks( ) - m_LastActionSentTicks;

    if( m_LastActionLateBy > m_DynamicLatency || TicksSinceLastUpdate > m_DynamicLatency - m_LastActionLateBy )
        return 0;
    else
        return m_DynamicLatency - m_LastActionLateBy - TicksSinceLastUpdate;
}

uint32_t CBaseGame::GetSlotsOccupied( )
{
    uint32_t NumSlotsOccupied = 0;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED )
            NumSlotsOccupied++;
    }

    return NumSlotsOccupied;
}

uint32_t CBaseGame::GetNumSlotsT1( )
{
    uint32_t NumSlotsT1 = 0;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) != SLOTSTATUS_CLOSED && ( *i ).GetComputer( ) == 0 && ( *i ).GetTeam( ) == 0 )
            NumSlotsT1++;
    }

    return NumSlotsT1;
}

uint32_t CBaseGame::GetNumSlotsT2( )
{
    uint32_t NumSlotsT2 = 0;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) != SLOTSTATUS_CLOSED && ( *i ).GetComputer( ) == 0 && ( *i ).GetTeam( ) == 1 )
            NumSlotsT2++;
    }

    return NumSlotsT2;
}

uint32_t CBaseGame::GetSlotsOccupiedT1( )
{
    uint32_t NumSlotsOccupiedT1 = 0;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) != SLOTSTATUS_OPEN && ( *i ).GetTeam( ) == 0 )
            NumSlotsOccupiedT1++;
    }

    return NumSlotsOccupiedT1;
}

uint32_t CBaseGame::GetSlotsOccupiedT2( )
{
    uint32_t NumSlotsOccupiedT2 = 0;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) != SLOTSTATUS_OPEN && ( *i ).GetTeam( ) == 1 )
            NumSlotsOccupiedT2++;
    }

    return NumSlotsOccupiedT2;
}

uint32_t CBaseGame::GetSlotsOpenT1( )
{
    uint32_t NumSlotsOpenT1 = 0;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_OPEN && ( *i ).GetTeam( ) == 0 )
            NumSlotsOpenT1++;
    }

    return NumSlotsOpenT1;
}

uint32_t CBaseGame::GetSlotsOpenT2( )
{
    uint32_t NumSlotsOpenT2 = 0;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_OPEN && ( *i ).GetTeam( ) == 1 )
            NumSlotsOpenT2++;
    }

    return NumSlotsOpenT2;
}

uint32_t CBaseGame::GetSlotsClosed( )
{
    uint32_t NumSlotsClosed = 0;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_CLOSED )
            NumSlotsClosed++;
    }

    return NumSlotsClosed;
}

uint32_t CBaseGame::GetSlotsOpen( )
{
    uint32_t NumSlotsOpen = 0;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_OPEN )
            NumSlotsOpen++;
    }

    return NumSlotsOpen;
}

bool CBaseGame::ObsPlayer( )
{
    return m_ObsPlayerPID != 255;
}

bool CBaseGame::DownloadInProgress( )
{
    for( unsigned char i = 0; i < m_Slots.size( ); ++i )
    {
        if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetComputer( ) == 0 && m_Slots[i].GetDownloadStatus( ) < 100 )
            return true;
    }
    return false;
}

uint32_t CBaseGame::GetNumFakePlayers( )
{
    return m_FakePlayers.size( );
}

uint32_t CBaseGame::GetNumPlayers( )
{
    uint32_t n = ( m_ObsPlayerPID == 255 ) ? 0 : 1;
    return GetNumHumanPlayers( ) + m_FakePlayers.size( ) + n;
}

uint32_t CBaseGame::GetNumHumanPlayers( )
{
    uint32_t NumHumanPlayers = 0;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) )
            NumHumanPlayers++;
    }

    return NumHumanPlayers;
}

string CBaseGame::GetDescription( )
{
    string Description = m_GameName + " : " + m_DefaultOwner + " : " + UTIL_ToString( GetNumHumanPlayers( ) ) + "/" + UTIL_ToString( m_GameLoading || m_GameLoaded ? m_StartPlayers : m_Slots.size( ) );

    if( m_GameLoading || m_GameLoaded )
        Description += " : " + UTIL_ToString( ( m_GameTicks / 1000 ) / 60 ) + "m";
    else
        Description += " : " + UTIL_ToString( ( GetTime( ) - m_CreationTime ) / 60 ) + "m";

    return Description;
}

void CBaseGame::SetAnnounce( uint32_t interval, string message )
{
    m_AnnounceInterval = interval;
    m_AnnounceMessage = message;
    m_LastAnnounceTime = GetTime( );
}

unsigned int CBaseGame::SetFD( void *fd, void *send_fd, int *nfds )
{
    unsigned int NumFDs = 0;

    if( m_Socket )
    {
        m_Socket->SetFD( (fd_set *)fd, (fd_set *)send_fd, nfds );
        NumFDs++;
    }

    for( vector<CPotentialPlayer *> ::iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); i++ )
    {
        if( ( *i )->GetSocket( ) && !( *i )->GetSocket( )->HasError( ) && ( *i )->GetSocket( )->GetConnected( ) )
        {
            ( *i )->GetSocket( )->SetFD( (fd_set *)fd, (fd_set *)send_fd, nfds );
            NumFDs++;
        }
    }

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( ( *i )->GetSocket( ) && !( *i )->GetSocket( )->HasError( ) && ( *i )->GetSocket( )->GetConnected( ) )
        {
            ( *i )->GetSocket( )->SetFD( (fd_set *)fd, (fd_set *)send_fd, nfds );
            NumFDs++;
        }
    }

    return NumFDs;
}

bool CBaseGame::Update( void *fd, void *send_fd )
{
    if( !m_FPHCLUser.empty( ) && m_GameLoaded && GetTime( ) - m_FPHCLTime > 45 )
    {
        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( ( *i )->GetName( ) == m_FPHCLUser )
            {
                SendChat( ( *i )->GetPID( ), "Время вышло, вы не успели ввести команду для фейк-плеера. Чтобы попробовать заново - используйте " + string( 1, m_UNM->m_CommandTrigger ) + "fphcl" );
                m_FPHCLNumber = 0;
                m_FPHCLUser = string( );
                m_FPHCLTime = GetTime( );
                break;
            }
        }
    }

    // drop players who have not loaded if it's been a long time

    if( m_GameLoading && !m_GameLoaded && m_UNM->m_DenyMaxLoadTime != 0 && GetTicks( ) - m_StartedLoadingTicks > m_UNM->m_DenyMaxLoadTime * 1000 )
    {
        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( !( *i )->GetFinishedLoading( ) )
            {
                DelTempOwner( ( *i )->GetName( ) );
                SendAllChat( "[" + ( *i )->GetName( ) + "] выкинут за долгую загрузку карты (больше чем " + UTIL_ToString( m_UNM->m_DenyMaxLoadTime ) + " сек.)" );
                ( *i )->SetFinishedLoadingTicks( GetTicks( ) );
                ( *i )->SetDeleteMe( true );
                ( *i )->SetLeftReason( "load time is too long" );
                ( *i )->SetLeftCode( PLAYERLEAVE_LOST );
            }
        }
    }

    // update players

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); )
    {
        if( ( *i )->Update( fd ) )
        {
            if( m_GameLoading || m_GameLoaded )
            {
                CGamePlayer *DeletedPlayer = GetPlayerFromPID( ( *i )->GetPID( ) );
                m_DeletedPlayers.push_back( DeletedPlayer );
                emit gToLog->toGameChat( QString( ), QString( ), GetColoredNameFromPID( ( *i )->GetPID( ) ), 11, m_CurrentGameCounter );
            }
            else
                emit gToLog->toGameChat( QString( ), QString( ), GetColoredNameFromPID( ( *i )->GetPID( ) ), 10, m_CurrentGameCounter );

            EventPlayerDeleted( *i );

            if( !m_GameLoading && !m_GameLoaded )
                delete *i;

            i = m_Players.erase( i );
        }
        else
            i++;
    }

    for( vector<CPotentialPlayer *> ::iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); )
    {
        if( ( *i )->Update( fd ) )
        {
            // flush the socket (e.g. in case a rejection message is queued)

            if( ( *i )->GetSocket( ) )
            {
                if( !( *i )->GetName( ).empty( ) )
                    ( *i )->GetSocket( )->DoSend( (fd_set *)send_fd, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Player [" + ( *i )->GetName( ) + "] caused an error:" );
                else
                    ( *i )->GetSocket( )->DoSend( (fd_set *)send_fd, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Player caused an error:" );
            }

            delete *i;
            i = m_Potentials.erase( i );
        }
        else
            i++;
    }

    // create the virtual host player

    uint32_t SlotReq = 12;

    if( m_ShowRealSlotCount )
        SlotReq = m_Slots.size( );
    if( !m_GameLoading && !m_GameLoaded && GetSlotsOccupied( ) < SlotReq && !m_UNM->m_DetourAllMessagesToAdmins )
        CreateVirtualHost( );

    // ping every 5 seconds
    // changed this to ping during game loading as well to hopefully fix some problems with people disconnecting during loading
    // changed this to ping during the game as well

    if( GetTime( ) - m_LastPingTime >= 5 )
    {
        // note: we must send pings to players who are downloading the map because Warcraft III disconnects from the lobby if it doesn't receive a ping every ~90 seconds
        // so if the player takes longer than 90 seconds to download the map they would be disconnected unless we keep sending pings
        // todotodo: ignore pings received from players who have recently finished downloading the map

        SendAll( m_Protocol->SEND_W3GS_PING_FROM_HOST( ) );

        if( m_UNM->m_broadcastinlan )
        {
            // we also broadcast the game to the local network every 5 seconds so we hijack this timer for our nefarious purposes
            // however we only want to broadcast if the countdown hasn't started
            // see the !sendlan code later in this file for some more information about how this works
            // todotodo: should we send a game cancel message somewhere? we'll need to implement a host counter for it to work

            if( !m_CountDownStarted )
            {
                BYTEARRAY MapGameType;

                // construct a fixed host counter which will be used to identify players from this "realm" (i.e. LAN)
                // the fixed host counter's 8 most significant bits will contain a 8 bit ID (0-255)
                // the rest of the fixed host counter will contain the 24 least significant bits of the actual host counter
                // since we're destroying 8 bits of information here the actual host counter should not be greater than 2^24 which is a reasonable assumption
                // when a player joins a game we can obtain the ID from the received host counter
                // note: LAN broadcasts use an ID of 0, battle.net refreshes use an ID of 1-255, the rest are unused

                uint32_t FixedHostCounter = m_HostCounter & 0x00FFFFFF;

                // construct the correct W3GS_GAMEINFO packet

                uint32_t slotstotal = m_Slots.size( );
                uint32_t slotsopen = GetSlotsOpen( );
                string BroadcastHostName = string( );

                if( m_UNM->m_BroadcastHostName == 0 )
                    BroadcastHostName = m_VirtualHostName;
                else if( m_UNM->m_BroadcastHostName == 1 )
                    BroadcastHostName = m_CreatorName;
                else if( m_UNM->m_BroadcastHostName == 2 )
                    BroadcastHostName = m_DefaultOwner;
                else if( m_UNM->m_BroadcastHostName == 3 )
                    BroadcastHostName = m_UNM->m_BroadcastHostNameCustom;

                if( BroadcastHostName.find_first_not_of( " " ) == string::npos )
                    BroadcastHostName = m_VirtualHostName;

                if( !m_ShowRealSlotCount )
                {
                    slotsopen = 2;
                    slotstotal = m_Slots.size( );
                }

                if( m_SaveGame )
                {
                    MapGameType.push_back( 0 );
                    MapGameType.push_back( 2 );
                    MapGameType.push_back( 0 );
                    MapGameType.push_back( 0 );
                    BYTEARRAY MapWidth;
                    MapWidth.push_back( 0 );
                    MapWidth.push_back( 0 );
                    BYTEARRAY MapHeight;
                    MapHeight.push_back( 0 );
                    MapHeight.push_back( 0 );
                    m_UNM->m_UDPSocket->Broadcast( m_UNM->m_BroadcastPort, m_Protocol->SEND_W3GS_GAMEINFO( m_UNM->m_TFT, m_UNM->m_LANWar3Version, MapGameType, m_Map->GetMapGameFlags( ), MapWidth, MapHeight, m_GameName, BroadcastHostName, GetTime( ) - m_CreationTime, "Save\\Multiplayer\\" + m_SaveGame->GetFileNameNoPath( ), m_SaveGame->GetMagicNumber( ), slotstotal, slotsopen, m_HostPort, FixedHostCounter, m_EntryKey, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]" );
                    m_UNM->m_UDPSocket->SendTo( "127.0.0.1", m_UNM->m_BroadcastPort, m_Protocol->SEND_W3GS_GAMEINFO( m_UNM->m_TFT, m_UNM->m_LANWar3Version, MapGameType, m_Map->GetMapGameFlags( ), MapWidth, MapHeight, m_GameName, BroadcastHostName, GetTime( ) - m_CreationTime, "Save\\Multiplayer\\" + m_SaveGame->GetFileNameNoPath( ), m_SaveGame->GetMagicNumber( ), slotstotal, slotsopen, m_HostPort, FixedHostCounter, m_EntryKey, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]" );
                }
                else
                {
                    MapGameType.push_back( m_Map->GetMapGameType( ) );
                    MapGameType.push_back( 0 );
                    MapGameType.push_back( 0 );
                    MapGameType.push_back( 0 );
                    m_UNM->m_UDPSocket->Broadcast( m_UNM->m_BroadcastPort, m_Protocol->SEND_W3GS_GAMEINFO( m_UNM->m_TFT, m_UNM->m_LANWar3Version, MapGameType, m_Map->GetMapGameFlags( ), m_Map->GetMapWidth( ), m_Map->GetMapHeight( ), m_GameName, BroadcastHostName, GetTime( ) - m_CreationTime, m_Map->GetMapPath( ), m_Map->GetMapCRC( ), slotstotal, slotsopen, m_HostPort, FixedHostCounter, m_EntryKey, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]" );
                    m_UNM->m_UDPSocket->SendTo( "127.0.0.1", m_UNM->m_BroadcastPort, m_Protocol->SEND_W3GS_GAMEINFO( m_UNM->m_TFT, m_UNM->m_LANWar3Version, MapGameType, m_Map->GetMapGameFlags( ), m_Map->GetMapWidth( ), m_Map->GetMapHeight( ), m_GameName, BroadcastHostName, GetTime( ) - m_CreationTime, m_Map->GetMapPath( ), m_Map->GetMapCRC( ), slotstotal, slotsopen, m_HostPort, FixedHostCounter, m_EntryKey, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]" );
                }

                uint32_t slotsoccupied = ( slotsopen <= 0 ) ? slotstotal : ( slotstotal - slotsopen );
                m_UNM->m_UDPSocket->Broadcast( m_UNM->m_BroadcastPort, m_Protocol->SEND_W3GS_REFRESHGAME( FixedHostCounter, slotstotal, slotsoccupied ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]" );
            }
            else
            {
                uint32_t FixedHostCounter = m_HostCounter & 0x00FFFFFF;
                m_UNM->m_UDPSocket->Broadcast( m_UNM->m_BroadcastPort, m_Protocol->SEND_W3GS_DECREATEGAME( FixedHostCounter ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]" );
            }
        }

        m_LastPingTime = GetTime( );
    }

    // send more map data

    if( !m_GameLoading && !m_GameLoaded && GetTicks( ) - m_LastDownloadCounterResetTicks >= 1000 )
    {
        // hackhack: another timer hijack is in progress here
        // since the download counter is reset once per second it's a great place to update the slot info if necessary

        if( m_SlotInfoChanged )
            SendAllSlotInfo( );

        m_LastDownloadCounterResetTicks = GetTicks( );
    }

    // kick AFK players

    if( m_UNM->m_AFKtimer != 0 && m_GameLoaded && GetTicks( ) - m_LastProcessedTicks > 1000 )
    {
        m_LastProcessedTicks = GetTicks( );
        uint32_t TimeNow = GetTime( );
        uint32_t TimeLimit = m_UNM->m_AFKtimer;

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            uint32_t TimeActive = ( *i )->GetTimeActive( );

            if( TimeActive > 0 && ( ( TimeNow - TimeActive ) > TimeLimit ) )
            {
                string s;
                ( *i )->SetTimeActive( 0 );

                if( ( m_UNM->m_AFKkick == 2 || m_GetMapType.find( "noafk" ) != string::npos || m_GetMapType.find( "guard" ) != string::npos ) && m_UNM->m_AFKkick != 0 )
                {
                    s = ". И был автоматически кикнут!";
                    ( *i )->SetDeleteMe( true );

                    if( m_UNM->m_AFKtimer < 60 )
                        ( *i )->SetLeftReason( "был кикнут за афк больше " + UTIL_ToString( m_UNM->m_AFKtimer ) + " секунд" );
                    else
                        ( *i )->SetLeftReason( "был кикнут за афк больше " + UTIL_ToString( m_UNM->m_AFKtimer / 60 ) + " минут" );

                    ( *i )->SetLeftCode( PLAYERLEAVE_LOST );
                }

                if( m_UNM->m_AFKtimer < 60 )
                    SendAllChat( "Игрок " + ( *i )->GetName( ) + " отошел (АФК больше " + UTIL_ToString( m_UNM->m_AFKtimer ) + " секунд)" + s );
                else
                    SendAllChat( "Игрок " + ( *i )->GetName( ) + " отошел (АФК больше " + UTIL_ToString( m_UNM->m_AFKtimer / 60 ) + " минут)" + s );

                break;
            }
        }
    }

    // announce every m_AnnounceInterval seconds

    if( !m_GameLoading && ( m_GameLoaded || !m_CountDownStarted ) && !m_AnnounceMessage.empty( ) && GetTime( ) - m_LastAnnounceTime >= m_AnnounceInterval )
    {
        string msg = m_AnnounceMessage;
        string msgl = string( );
        string::size_type lstart;

        while( ( lstart = msg.find( "|" ) ) != string::npos )
        {
            msgl = msg.substr( 0, lstart );
            msg = msg.substr( lstart + 1, msg.length( ) - lstart - 1 );
            SendAllChat( msgl );
        }

        SendAllChat( msg );
        m_LastAnnounceTime = GetTime( );
    }

    // also announce every m_UNM->m_GameAnnounceInterval seconds

    if( !m_GameLoading && ( m_GameLoaded || !m_CountDownStarted ) && !m_UNM->m_Announce.empty( ) && ( m_UNM->m_GameAnnounce == 1 || ( m_UNM->m_GameAnnounce == 2 && !m_GameLoaded ) || ( m_UNM->m_GameAnnounce == 3 && m_GameLoaded ) ) && m_UNM->m_GameAnnounceInterval != 0 && GetTime( ) - m_LastGameAnnounceTime >= m_UNM->m_GameAnnounceInterval )
    {
        for( vector<string> ::iterator i = m_UNM->m_Announce.begin( ); i != m_UNM->m_Announce.end( ); i++ )
            SendAllChat( ( *i ) );

        m_LastGameAnnounceTime = GetTime( );
    }

    // kick players who don't spoof check within SpoofTime seconds when spoof checks are required and the game is autohosted

    if( !m_CountDownStarted && m_UNM->m_SpoofChecks > 0 && m_GameState == GAME_PUBLIC )
    {
        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( !( *i )->GetSpoofed( ) && GetTime( ) - ( *i )->GetJoinTime( ) >= 20 )
            {
                ( *i )->SetDeleteMe( true );
                ( *i )->SetLeftReason( gLanguage->WasKickedForNotSpoofChecking( ) );
                ( *i )->SetLeftCode( PLAYERLEAVE_LOBBY );
                OpenSlot( GetSIDFromPID( ( *i )->GetPID( ) ), false );
            }
        }
    }

    if( !m_CountDownStarted && m_UNM->m_SpoofChecks != 0 && m_GameState == GAME_PUBLIC && m_NumSpoofOfline > 0 )
    {
        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( m_NumSpoofOfline > 0 && !( *i )->GetSpoofed( ) && GetTime( ) - ( *i )->GetJoinTime( ) >= 3 && IsSpoofOfline( ( *i )->GetName( ) ) )
            {
                string UserName = ( *i )->GetName( );
                m_NumSpoofOfline--;
                DelSpoofOfline( UserName );
                SendAllChat( gLanguage->SpoofPossibleIsUnavailable( UserName ) );
            }
        }

        m_NumSpoofOfline = 0;
    }

    // try to auto start

    if( !m_CountDownStarted && m_AutoStartPlayers != 0 && GetTime( ) - m_LastAutoStartTime >= m_UNM->m_TryAutoStartDelay )
    {
        StartCountDownAuto( m_UNM->m_SpoofChecks > 0 );
        m_LastAutoStartTime = GetTime( );
    }

    // end game countdown every 1000 ms

    if( m_GameEndCountDownStarted && GetTicks( ) - m_GameEndLastCountDownTicks >= 1000 )
    {
        if( m_GameEndCountDownCounter > 0 )
        {
            // we use a countdown counter rather than a "finish countdown time" here because it might alternately round up or down the count
            // this sometimes resulted in a countdown of e.g. "6 5 3 2 1" during my testing which looks pretty dumb
            // doing it this way ensures it's always "5 4 3 2 1" but each interval might not be *exactly* the same length

            SendAllChat( UTIL_ToString( m_GameEndCountDownCounter ) + ". . ." );
            m_GameEndCountDownCounter--;
        }
        else if( !m_GameLoading && m_GameLoaded )
        {
            m_GameEndCountDownStarted = false;
            StopPlayers( "был отключен (игра была завершена)" );
        }

        m_GameEndLastCountDownTicks = GetTicks( );
    }

    if( m_CountDownStarted && !m_GameLoading && !m_GameLoaded )
    {
        // start game countdown every X ms

        // we use a countdown counter rather than a "finish countdown time" here because it might alternately round up or down the count
        // this sometimes resulted in a countdown of e.g. "6 5 3 2 1" during my testing which looks pretty dumb
        // doing it this way ensures it's always "5 4 3 2 1" but each interval might not be *exactly* the same length

        if( m_NormalCountdown )
        {
            if( GetTicks( ) - m_LastCountDownTicks >= 1200 )
            {
                if( m_CountDownCounter > 0 )
                    m_CountDownCounter--;
                else
                    EventGameStarted( );

                m_LastCountDownTicks = GetTicks( );
            }
        }
        else if( m_UNM->m_CountDownMethod == 0 )
        {
            if( GetTicks( ) - m_LastCountDownTicks >= 1000 )
            {
                if( m_CountDownCounter > 0 )
                {
                    SendAllChat( UTIL_ToString( m_CountDownCounter ) + ". . ." );
                    m_CountDownCounter--;
                }
                else
                    EventGameStarted( );

                m_LastCountDownTicks = GetTicks( );
            }
        }
        else if( m_UNM->m_CountDownMethod == 1 )
        {
            if( GetTicks( ) - m_LastCountDownTicks >= 25 )
            {
                if( m_CountDownCounter > 0 )
                {
                    // we use a countdown counter rather than a "finish countdown time" here because it might alternately round up or down the count
                    // this sometimes resulted in a countdown of e.g. "6 5 3 2 1" during my testing which looks pretty dumb
                    // doing it this way ensures it's always "5 4 3 2 1" but each interval might not be *exactly* the same length

                    if( m_CountDownCounter == 200 )
                    {
                        deque<string> AddToBufferMessages;

                        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                        {
                            if( ( *i )->m_BufferMessages.size( ) < 7 && !m_UNM->m_BotNamePrivatePrintJoin && m_UNM->IsBnetServer( ( *i )->GetJoinedRealm( ) ) )
                            {
                                string BotName = string( );

                                if( m_UNM->m_BotNameCustom.find_first_not_of( " " ) != string::npos )
                                    BotName = m_UNM->m_BotNameCustom;
                                else
                                {
                                    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                                    {
                                        if( !( *it )->m_SpamSubBot && ( *it )->GetServer( ) == ( *i )->GetJoinedRealm( ) )
                                        {
                                            BotName = ( *it )->GetName( );
                                            break;
                                        }
                                    }

                                    if( BotName.empty( ) )
                                    {
                                        for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                                        {
                                            if( ( *it )->GetHostCounterID( ) == ( *i )->GetJoinedRealmID( ) )
                                            {
                                                BotName = ( *it )->GetName( );
                                                break;
                                            }
                                        }
                                    }
                                }

                                if( !BotName.empty( ) )
                                    AddToBufferMessages.push_front( gLanguage->BotName( BotName ) );
                            }

                            if( AddToBufferMessages.size( ) + ( *i )->m_BufferMessages.size( ) < 7 && !m_UNM->m_BotChannelPrivatePrintJoin && m_UNM->IsBnetServer( ( *i )->GetJoinedRealm( ) ) )
                            {
                                string BotChannel = string( );

                                if( m_UNM->m_BotNameCustom.find_first_not_of( " " ) != string::npos )
                                    BotChannel = m_UNM->m_BotChannelCustom;
                                else
                                {
                                    for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                                    {
                                        if( !( *it )->m_SpamSubBot && ( *it )->GetServer( ) == ( *i )->GetJoinedRealm( ) )
                                        {
                                            BotChannel = ( *it )->GetFirstChannel( );
                                            break;
                                        }
                                    }

                                    if( BotChannel.empty( ) )
                                    {
                                        for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                                        {
                                            if( ( *it )->GetHostCounterID( ) == ( *i )->GetJoinedRealmID( ) )
                                            {
                                                BotChannel = ( *it )->GetFirstChannel( );
                                                break;
                                            }
                                        }
                                    }
                                }

                                if( !BotChannel.empty( ) )
                                    AddToBufferMessages.push_front( gLanguage->BotChannel( BotChannel ) );
                            }

                            if( AddToBufferMessages.size( ) + ( *i )->m_BufferMessages.size( ) < 7 && !m_UNM->m_InfoGamesPrivatePrintJoin )
                            {
                                uint32_t NowOnline = 0;

                                for( uint32_t i = 0; i < m_UNM->m_Games.size( ); i++ )
                                    NowOnline += m_UNM->m_Games[i]->GetNumHumanPlayers( );

                                if( m_UNM->m_CurrentGame )
                                    NowOnline += NowOnline + m_UNM->m_CurrentGame->GetNumHumanPlayers( );

                                AddToBufferMessages.push_front( "Активных игр [" + UTIL_ToString( m_UNM->m_Games.size( ) ) + "] → Игроков онлайн [" + UTIL_ToString( NowOnline ) + "]" );
                            }

                            if( AddToBufferMessages.size( ) + ( *i )->m_BufferMessages.size( ) < 7 && m_OwnerName.find_first_not_of( " " ) != string::npos && m_UNM->m_GameOwnerPrivatePrintJoin == 0 )
                            {
                                if( m_OwnerName == m_DefaultOwner )
                                    AddToBufferMessages.push_front( gLanguage->GameHost( m_DefaultOwner ) );
                                else if( m_OwnerName.find( " " ) == string::npos )
                                    AddToBufferMessages.push_front( gLanguage->GameOwner( m_OwnerName ) );
                                else
                                {
                                    string LastOwner = m_OwnerName;

                                    while( !LastOwner.empty( ) && LastOwner.back( ) == ' ' )
                                        LastOwner.erase( LastOwner.end( ) - 1 );

                                    string::size_type LastOwnerStart = LastOwner.rfind( " " );

                                    if( LastOwnerStart != string::npos )
                                        LastOwner = LastOwner.substr( LastOwnerStart + 1 );

                                    if( LastOwner == m_DefaultOwner )
                                        AddToBufferMessages.push_front( gLanguage->GameHost( m_DefaultOwner ) );
                                    else
                                        AddToBufferMessages.push_front( gLanguage->GameOwner( LastOwner ) );
                                }
                            }

                            if( AddToBufferMessages.size( ) + ( *i )->m_BufferMessages.size( ) < 7 && m_HCLCommandString.find_first_not_of( " " ) != string::npos && !m_UNM->m_GameModePrivatePrintJoin )
                                AddToBufferMessages.push_front( gLanguage->GameMode( m_HCLCommandString ) );

                            if( AddToBufferMessages.size( ) + ( *i )->m_BufferMessages.size( ) < 7 && m_GameName.find_first_not_of( " " ) != string::npos && !m_UNM->m_GameNamePrivatePrintJoin )
                                AddToBufferMessages.push_front( gLanguage->GameName( m_GameName ) );

                            if( AddToBufferMessages.size( ) + ( *i )->m_BufferMessages.size( ) < 7 && !m_UNM->m_PlayerBeforeStartPrivatePrintJoin && m_AutoStartPlayers > 0 )
                            {
                                if( ( m_UNM->m_PlayerBeforeStartPrintDelay > m_ActuallyPlayerBeforeStartPrintDelay + 2 ) || !m_UNM->m_PlayerBeforeStartPrivatePrintJoinSD || !m_UNM->m_PlayerBeforeStartPrint )
                                {
                                    uint32_t PNr = GetNumHumanPlayers( );

                                    if( PNr < m_AutoStartPlayers )
                                    {
                                        string s = gLanguage->WaitingForPlayersBeforeAutoStartVS( UTIL_ToString( m_AutoStartPlayers ), UTIL_ToString( m_AutoStartPlayers - PNr ), string( 1, m_UNM->m_CommandTrigger ) );
                                        AddToBufferMessages.push_front( s );
                                    }
                                }
                            }

                            for( uint32_t in = 0; in < AddToBufferMessages.size( ); in++ )
                                ( *i )->m_BufferMessages.push_front( m_Protocol->SEND_W3GS_CHAT_FROM_HOST_BUFFER( GetHostPID( ), UTIL_CreateByteArray( ( *i )->GetPID( ) ), 16, BYTEARRAY( ), AddToBufferMessages[in], "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                            AddToBufferMessages.clear( );
                        }
                    }

                    string poloska = string( );

                    while( poloska.size( ) < m_CountDownCounter )
                        poloska = poloska + ".";

                    while( poloska.size( ) < 200 )
                        poloska = ":" + poloska;

                    poloska = "[" + poloska.substr( 0, 100 ) + UTIL_ToString( ( 200 - m_CountDownCounter ) / 2 ) + "%" + poloska.substr( 100 ) + "]";

                    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    {
                        for( uint32_t in = 0; in < ( *i )->m_BufferMessages.size( ); in++ )
                            SendFromBuffer( *i, ( *i )->m_BufferMessages[in] );

                        Send( *i, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( ( *i )->GetPID( ), UTIL_CreateByteArray( ( *i )->GetPID( ) ), 16, BYTEARRAY( ), poloska, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                    }

                    m_CountDownCounter--;
                }
                else if( !m_GameLoading && !m_GameLoaded )
                    EventGameStarted( );

                m_LastCountDownTicks = GetTicks( );
            }
        }
    }

    // check if the lobby is "abandoned" and needs to be closed since it will never start

    if( !m_GameLoading && !m_GameLoaded && !m_CountDownStarted && !m_DownloadOnlyMode && m_AutoStartPlayers == 0 && m_UNM->m_LobbyTimeLimitMax > 0 )
    {
        // check if we've hit the time limit

        if( GetTime( ) - m_CreationTime >= m_UNM->m_LobbyTimeLimitMax )
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] is over (lobby time limit hit)", 6, m_CurrentGameCounter );

            for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
            {
                if( !( *i )->m_SpamSubBotChat )
                {
                    if( ( *i )->GetServer( ) == GetCreatorServer( ) && !( *i )->m_SpamSubBot )
                        ( *i )->QueueChatCommand( "GAME [" + m_UNM->IncGameNrDecryption( m_GameName ) + "] is over (lobby timelimit hit)", GetCreatorName( ), true );

                    ( *i )->QueueChatCommand( "GAME [" + m_UNM->IncGameNrDecryption( m_GameName ) + "] is over (lobby timelimit hit)" );
                }
            }

            m_Exiting = true;
        }
    }

    // check if the lobby is "abandoned" and needs to be closed since it will never start

    if( !m_GameLoading && !m_GameLoaded && !m_CountDownStarted && !m_DownloadOnlyMode && m_AutoStartPlayers == 0 && m_UNM->m_LobbyTimeLimit > 0 && ( m_AutoStartPlayers == 0 || m_AutoStartPlayers < GetNumHumanPlayers( ) ) )
    {
        // check if there's owner in the game

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( IsDefaultOwner( ( *i )->GetName( ) ) || IsOwner( ( *i )->GetName( ) ) )
                m_LastReservedSeen = GetTime( );
        }

        // check if we've hit the time limit

        if( GetTime( ) - m_LastReservedSeen >= m_UNM->m_LobbyTimeLimit )
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] is over (lobby time limit hit)", 6, m_CurrentGameCounter );

            for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
            {
                if( !( *i )->m_SpamSubBotChat )
                {
                    if( ( *i )->GetServer( ) == GetCreatorServer( ) && !( *i )->m_SpamSubBot )
                        ( *i )->QueueChatCommand( "GAME [" + m_UNM->IncGameNrDecryption( m_GameName ) + "] is over (lobby timelimit hit)", GetCreatorName( ), true );

                    ( *i )->QueueChatCommand( "GAME [" + m_UNM->IncGameNrDecryption( m_GameName ) + "] is over (lobby timelimit hit)" );
                }
            }

            m_Exiting = true;
        }
    }

    // check if the game is loaded

    if( m_GameLoading )
    {
        bool FinishedLoading = true;

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            FinishedLoading = ( *i )->GetFinishedLoading( );

            if( !FinishedLoading )
                break;
        }

        if( FinishedLoading )
            EventGameLoaded( );
        else
        {
            // reset the "lag" screen (the load-in-game screen) every 30 seconds

            if( m_LoadInGame && GetTime( ) - m_LastLagScreenResetTime >= 30 )
            {
                bool UsingGProxy = false;

                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    if( ( *i )->GetGProxy( ) )
                        UsingGProxy = true;
                }

                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    if( ( *i )->GetFinishedLoading( ) )
                    {
                        // stop the lag screen

                        for( vector<CGamePlayer *> ::iterator j = m_Players.begin( ); j != m_Players.end( ); j++ )
                        {
                            if( !( *j )->GetFinishedLoading( ) )
                                Send( *i, m_Protocol->SEND_W3GS_STOP_LAG( *j, true ) );
                        }

                        // send an empty update
                        // this resets the lag screen timer but creates a rather annoying problem
                        // in order to prevent a desync we must make sure every player receives the exact same "desyncable game data" (updates and player leaves) in the exact same order
                        // unfortunately we cannot send updates to players who are still loading the map, so we buffer the updates to those players (see the else clause a few lines down for the code)
                        // in addition to this we must ensure any player leave messages are sent in the exact same position relative to these updates so those must be buffered too

                        if( UsingGProxy && !( *i )->GetGProxy( ) )
                        {
                            // we must send empty actions to non-GProxy++ players
                            // GProxy++ will insert these itself so we don't need to send them to GProxy++ players
                            // empty actions are used to extend the time a player can use when reconnecting

                            for( unsigned char j = 0; j < m_GProxyEmptyActions; j++ )
                                Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
                        }

                        Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );

                        // start the lag screen

                        Send( *i, m_Protocol->SEND_W3GS_START_LAG( m_Players, true, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                    }
                    else
                    {
                        // buffer the empty update since the player is still loading the map

                        if( UsingGProxy && !( *i )->GetGProxy( ) )
                        {
                            // we must send empty actions to non-GProxy++ players
                            // GProxy++ will insert these itself so we don't need to send them to GProxy++ players
                            // empty actions are used to extend the time a player can use when reconnecting

                            for( unsigned char j = 0; j < m_GProxyEmptyActions; j++ )
                                ( *i )->AddLoadInGameData( m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
                        }

                        ( *i )->AddLoadInGameData( m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
                    }
                }

                // add actions to replay

                if( m_Replay )
                {
                    if( UsingGProxy )
                    {
                        for( unsigned char i = 0; i < m_GProxyEmptyActions; i++ )
                            m_Replay->AddTimeSlot( 0, queue<CIncomingAction *>( ) );
                    }

                    m_Replay->AddTimeSlot( 0, queue<CIncomingAction *>( ) );
                }

                m_LastLagScreenResetTime = GetTime( );
            }
        }
    }

    // keep track of the largest sync counter (the number of keepalive packets received by each player)
    // if anyone falls behind by more than m_SyncLimit keepalives we start the lag screen

    if( m_GameLoaded )
    {
        // check if anyone has started lagging
        // we consider a player to have started lagging if they're more than m_SyncLimit keepalives behind

        if( !m_Lagging )
        {
            string LaggingString;
            m_DynamicLatencyMaxSync = 0;
            uint32_t Sync = 0;

            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            {
                if( ( *i )->GetSyncCounter( ) >= m_SyncCounter )
                    Sync = 0;
                else
                    Sync = m_SyncCounter - ( *i )->GetSyncCounter( );

                if( Sync > m_DynamicLatencyMaxSync )
                    m_DynamicLatencyMaxSync = Sync;

                if( Sync > m_SyncLimit )
                {
                    // drop them immediately if they have already exceeded their total lagging time
                    if( m_UNM->m_DropWaitTimeSum > 0 && ( *i )->GetTotalLaggingTicks( ) > m_UNM->m_DropWaitTimeSum * 1000 )
                    {
                        ( *i )->SetDeleteMe( true );
                        ( *i )->SetLeftReason( gLanguage->TotalLaggingTimeLimit( ) );
                        ( *i )->SetLeftCode( PLAYERLEAVE_DISCONNECT );
                    }
                    else
                    {
                        ( *i )->SetLagging( true );
                        ( *i )->SetStartedLaggingTicks( GetTicks( ) );
                        m_Lagging = true;
                        m_DynamicLatencyLagging = true;
                        m_StartedLaggingTime = GetTime( );

                        if( LaggingString.empty( ) )
                            LaggingString = ( *i )->GetName( );
                        else
                            LaggingString += ", " + ( *i )->GetName( );
                    }
                }
            }

            if( m_Lagging )
            {
                // start the lag screen

                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] started lagging on [" + LaggingString + "]", 6, m_CurrentGameCounter );
                SendAll( m_Protocol->SEND_W3GS_START_LAG( m_Players, false, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                // reset everyone's drop vote

                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    ( *i )->SetDropVote( false );

                m_LastLagScreenResetTime = GetTime( );

                // get current time to use for the drop vote

                m_LagScreenTime = GetTime( );
            }
        }
        else if( !m_Pause )
        {
            string LaggingString;

            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            {
                if( !( *i )->GetLagging( ) && m_SyncCounter - ( *i )->GetSyncCounter( ) > m_SyncLimit )
                {
                    ( *i )->SetLagging( true );
                    ( *i )->SetStartedLaggingTicks( GetTicks( ) );

                    if( LaggingString.empty( ) )
                        LaggingString = ( *i )->GetName( );
                    else
                        LaggingString += ", " + ( *i )->GetName( );
                }
            }

            if( !LaggingString.empty( ) )
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] also started lagging on [" + LaggingString + "]", 6, m_CurrentGameCounter );
        }

        if( m_Lagging )
        {
            // check if anyone has stopped lagging normally
            // we consider a player to have stopped lagging if they're less than half m_SyncLimit keepalives behind

            bool Lagging = false;

            if( m_PauseEnd )
            {
                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    ( *i )->SetLagging( false );
                    ( *i )->SetStartedLaggingTicks( 0 );
                }

                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    ( *i )->SetLagging( true );
                    ( *i )->SetStartedLaggingTicks( GetTicks( ) );
                    ( *i )->Send( m_Protocol->SEND_W3GS_STOP_LAG( *i ) );
                    ( *i )->SetLagging( false );
                    ( *i )->SetStartedLaggingTicks( 0 );
                }
                m_Pause = false;
                m_PauseEnd = false;
            }
            else if( m_Pause )
            {
                if( GetTime( ) - m_LastPauseResetTime >= 60 )
                {
                    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    {
                        ( *i )->SetLagging( false );
                        ( *i )->SetStartedLaggingTicks( 0 );
                    }

                    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    {
                        ( *i )->SetLagging( true );
                        ( *i )->SetStartedLaggingTicks( GetTicks( ) );
                        ( *i )->Send( m_Protocol->SEND_W3GS_STOP_LAG( *i ) );
                        ( *i )->SetLagging( false );
                        ( *i )->SetStartedLaggingTicks( 0 );
                    }

                    m_StartedLaggingTime = GetTime( );

                    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    {
                        ( *i )->SetLagging( true );
                        ( *i )->SetStartedLaggingTicks( GetTicks( ) );
                        ( *i )->Send( m_Protocol->SEND_W3GS_START_LAG( m_Players, false, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                        ( *i )->SetLagging( false );
                        ( *i )->SetStartedLaggingTicks( 0 );
                    }

                    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    {
                        ( *i )->SetLagging( true );
                        ( *i )->SetStartedLaggingTicks( GetTicks( ) );
                    }

                    m_LastPauseResetTime = GetTime( );
                    m_LastLagScreenResetTime = GetTime( );
                    m_LagScreenTime = GetTime( );
                }

                Lagging = true;
            }
            else
            {
                bool UsingGProxy = false;

                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    if( ( *i )->GetGProxy( ) )
                        UsingGProxy = true;
                }

                uint32_t WaitTime = m_UNM->m_DropWaitTime;

                if( UsingGProxy && WaitTime > 0 )
                    WaitTime = ( m_GProxyEmptyActions + 1 ) * 60;

                if( WaitTime > 0 && GetTime( ) - m_StartedLaggingTime >= WaitTime )
                {
                    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    {
                        if( ( *i )->GetLagging( ) && GetTicks( ) - ( *i )->GetStartedLaggingTicks( ) >= WaitTime * 1000 )
                        {
                            ( *i )->SetDeleteMe( true );
                            ( *i )->SetLeftReason( gLanguage->WasAutomaticallyDroppedAfterSeconds( UTIL_ToString( WaitTime ) ) );
                            ( *i )->SetLeftCode( PLAYERLEAVE_DISCONNECT );
                        }
                    }
                }

                // we cannot allow the lag screen to stay up for more than ~65 seconds because Warcraft III disconnects if it doesn't receive an action packet at least this often
                // one (easy) solution is to simply drop all the laggers if they lag for more than 60 seconds
                // another solution is to reset the lag screen the same way we reset it when using load-in-game
                // this is required in order to give GProxy++ clients more time to reconnect

                if( GetTime( ) - m_LastLagScreenResetTime >= 60 )
                {
                    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    {
                        // stop the lag screen

                        for( vector<CGamePlayer *> ::iterator j = m_Players.begin( ); j != m_Players.end( ); j++ )
                        {
                            if( ( *j )->GetLagging( ) )
                                Send( *i, m_Protocol->SEND_W3GS_STOP_LAG( *j ) );
                        }

                        // send an empty update
                        // this resets the lag screen timer

                        if( UsingGProxy && !( *i )->GetGProxy( ) )
                        {
                            // we must send additional empty actions to non-GProxy++ players
                            // GProxy++ will insert these itself so we don't need to send them to GProxy++ players
                            // empty actions are used to extend the time a player can use when reconnecting

                            for( unsigned char j = 0; j < m_GProxyEmptyActions; j++ )
                                Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
                        }

                        Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );

                        // start the lag screen

                        Send( *i, m_Protocol->SEND_W3GS_START_LAG( m_Players, false, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                    }

                    // add actions to replay

                    if( m_Replay )
                    {
                        if( UsingGProxy )
                        {
                            for( unsigned char i = 0; i < m_GProxyEmptyActions; i++ )
                                m_Replay->AddTimeSlot( 0, queue<CIncomingAction *>( ) );
                        }

                        m_Replay->AddTimeSlot( 0, queue<CIncomingAction *>( ) );
                    }

                    m_LastLagScreenResetTime = GetTime( );
                }

                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    // stop the lag screen for this player

                    if( ( *i )->GetLagging( ) && m_SyncCounter - ( *i )->GetSyncCounter( ) <= ( ( m_SyncLimit * 4 ) / 5 ) )
                    {
                        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] stopped lagging on [" + ( *i )->GetName( ) + "]", 6, m_CurrentGameCounter );
                        SendAll( m_Protocol->SEND_W3GS_STOP_LAG( *i ) );

                        // update their total lagging time

                        if( m_UNM->m_DropWaitTimeSum > 0 && ( *i )->GetStartedLaggingTicks( ) > 0 )
                            ( *i )->SetTotalLaggingTicks( ( *i )->GetTotalLaggingTicks( ) + ( GetTicks( ) - ( *i )->GetStartedLaggingTicks( ) ) );

                        ( *i )->SetLagging( false );
                        ( *i )->SetStartedLaggingTicks( 0 );
                    }
                }

                // check if everyone has stopped lagging

                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    if( ( *i )->GetLagging( ) )
                        Lagging = true;
                }
            }

            m_Lagging = Lagging;

            // reset m_LastActionSentTicks because we want the game to stop running while the lag screen is up

            m_LastActionSentTicks = GetTicks( );

            // keep track of the last lag screen time so we can avoid timing out players

            m_LastLagScreenTime = GetTime( );
        }
    }

    // update dynamic latency

    if( m_UseDynamicLatency )
        SetDynamicLatency( );
    else if( m_Latency == 0 )
        m_DynamicLatency = 1;
    else
        m_DynamicLatency = m_Latency;

    // send actions every m_DynamicLatency milliseconds
    // actions are at the heart of every Warcraft 3 game but luckily we don't need to know their contents to relay them
    // we queue player actions in EventPlayerAction then just resend them in batches to all players here

    if( m_GameLoaded && !m_Lagging && ( GetTicks( ) - m_LastActionSentTicks >= m_DynamicLatency - m_LastActionLateBy || m_LastActionLateBy >= m_DynamicLatency ) )
        SendAllActions( );

    // expire the votekick

    if( !m_KickVotePlayer.empty( ) && GetTime( ) - m_StartedKickVoteTime >= 120 )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Время на голосование чтобы выкинуть игрока [" + m_KickVotePlayer + "] (120 сек.) истекло!", 6, m_CurrentGameCounter );
        SendAllChat( gLanguage->VoteKickExpired( m_KickVotePlayer ) );
        m_KickVotePlayer.clear( );
        m_StartedKickVoteTime = 0;
    }

    // expire the votestart

    if( m_StartedVoteStartTime != 0 && m_UNM->m_VoteStartDuration > 0 && GetTime( ) - m_StartedVoteStartTime >= m_UNM->m_VoteStartDuration )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Время на голосование за досрочный старт игры (" + UTIL_ToString( m_UNM->m_VoteStartDuration ) + " сек.) истекло!", 6, m_CurrentGameCounter );
        SendAllChat( "Время на голосование за досрочный старт игры (" + UTIL_ToString( m_UNM->m_VoteStartDuration ) + " сек.) истекло!" );
        m_StartedVoteStartTime = 0;
    }

    // subtract m_UNM->m_AllowLaggingTime from Player->GetTotalLaggingTicks for each player every m_UNM->m_AllowLaggingTimeInterval seconds, but only if game is loaded

    if( m_UNM->m_AllowLaggingTime > 0 && m_UNM->m_AllowLaggingTimeInterval > 0 && m_GameLoaded && GetTime( ) - m_LastRedefinitionTotalLaggingTime >= m_UNM->m_AllowLaggingTimeInterval )
    {
        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( ( *i )->GetTotalLaggingTicks( ) > m_UNM->m_AllowLaggingTime )
                ( *i )->SetTotalLaggingTicks( ( *i )->GetTotalLaggingTicks( ) - m_UNM->m_AllowLaggingTime );
            else if( ( *i )->GetTotalLaggingTicks( ) > 0 )
                ( *i )->SetTotalLaggingTicks( 0 );
        }

        m_LastRedefinitionTotalLaggingTime = GetTime( );
    }

    // show game start text
    // read from gameloaded.txt if available

    if( m_GameLoaded && !m_GameLoadedMessage && GetTime( ) >= m_GameLoadedTime + m_UNM->m_GameLoadedPrintout )
    {
        m_GameLoadedMessage = true;
        string file;
        ifstream in;

#ifdef WIN32
        file = "text_files\\gameloaded.txt";
        in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
        file = "text_files/gameloaded.txt";
        in.open( file );
#endif

        if( !in.fail( ) )
        {
            // don't print more than 30 lines

            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] loading file [" + file + "]", 6, m_CurrentGameCounter );
            uint32_t Count = 0;
            string Line;

            while( !in.eof( ) && Count < 30 )
            {
                getline( in, Line );
                Line = UTIL_FixFileLine( Line );

                if( Line.empty( ) )
                    SendAllChat( " " );
                else
                    SendAllChat( Line );

                if( in.eof( ) )
                    break;

                Count++;
            }
        }
        else
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] warning - unable to read file [" + file + "]", 6, m_CurrentGameCounter );

        in.close( );
    }

    // check if switch expired

    if( m_SwitchTime != 0 && GetTime( ) - m_SwitchTime > 60 )
    {
        m_SwitchTime = 0;
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Switch expired", 6, m_CurrentGameCounter );
    }

    // update game in GUI every 10 seconds

    if( GetTime( ) - m_LastGameUpdateGUITime > 5 )
    {
        m_LastGameUpdateGUITime = GetTime( );
        emit gToLog->updateGame( m_CurrentGameCounter, GetGameData( ), GetPlayersData( ) );
    }

    bool lessthanminpercent = false;
    bool lessthanminplayers = false;
    float remainingpercent = (float)m_Players.size( ) * 100 / (float)m_PlayersatStart;

    if( !m_GameOverCanceled )
    {
        // start the gameover timer if we have less than the minimum percent of players remaining.
        if( m_UNM->m_gameoverminpercent != 0 && !m_GameLoading && m_GameLoaded && remainingpercent < m_UNM->m_gameoverminpercent )
            lessthanminpercent = true;

        if( lessthanminpercent && m_GameOverTime == 0 )
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] gameover timer started (less than " + UTIL_ToString( m_UNM->m_gameoverminpercent ) + "% )" + " " + string( 1, m_UNM->m_CommandTrigger ) + "override to cancel", 6, m_CurrentGameCounter );
            SendAllChat( "Game over in 25 seconds, " + UTIL_ToString( remainingpercent ) + "% remaining ( < " + UTIL_ToString( m_UNM->m_gameoverminpercent ) + "% ) " + string( 1, m_UNM->m_CommandTrigger ) + "override to cancel" );
            m_GameOverTime = GetTime( );
        }

        // start the gameover timer if there's less than m_gameoverminplayers players left (and we had at least 1 leaver)

        if( m_UNM->m_gameoverminplayers != 0 && m_Players.size( ) < m_UNM->m_gameoverminplayers && m_PlayersatStart > m_Players.size( ) && !m_GameLoading && m_GameLoaded )
            lessthanminplayers = true;

        if( lessthanminplayers && m_GameOverTime == 0 )
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] gameover timer started (one player left)" + " " + string( 1, m_UNM->m_CommandTrigger ) + "override to cancel", 6, m_CurrentGameCounter );
            SendAllChat( "Game over in 25 seconds, " + UTIL_ToString( m_Players.size( ) ) + " remaining ( < " + UTIL_ToString( m_UNM->m_gameoverminplayers ) + " ), write " + string( 1, m_UNM->m_CommandTrigger ) + "or / " + string( 1, m_UNM->m_CommandTrigger ) + "override to cancel" );
            m_GameOverTime = GetTime( );
        }

        if( !m_GameLoading && m_GameLoaded && !m_GameOverDiffCanceled && !lessthanminplayers && !lessthanminpercent && ( m_Team1 < 2 || m_Team2 < 2 ) )
            m_GameOverDiffCanceled = true;

        // start the gameover timer if there's only one player left

        if( ( m_UNM->m_gameoverminplayers == 0 || m_UNM->m_gameoverminplayers > 2 ) && m_Players.size( ) == 1 && m_GameOverTime == 0 && ( m_GameLoading || m_GameLoaded ) )
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] gameover timer started (one player left)", 6, m_CurrentGameCounter );
            m_GameOverTime = GetTime( );
        }

        // start the gameover timer if team unbalance is greater than m_gameovermaxteamdifference
        // ex: m_gameovermaxteamdifference = 2, if one team has -3, start game over.

        if( !m_GameLoading && m_GameLoaded && !m_GameOverDiffCanceled && !m_Switched && !lessthanminplayers && !lessthanminpercent && m_GetMapNumTeams == 2 && m_UNM->m_gameovermaxteamdifference != 0 && m_Players.size( ) < m_PlayersatStart && m_Players.size( ) > 1 )
        {
            bool unbalanced = false;

            if( m_TeamDiff > m_UNM->m_gameovermaxteamdifference )
                unbalanced = true;

            if( m_GameOverTime != 0 && !unbalanced )
            {
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] gameover timer stoped (rebalanced team)", 6, m_CurrentGameCounter );
                SendAllChat( "Game over averted, team rebalanced" );
                m_GameOverTime = 0;
            }


            if( unbalanced && m_GameOverTime == 0 )
            {
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] gameover timer started (unbalanced team)", 6, m_CurrentGameCounter );
                SendAllChat( "Game over in 120 seconds, rebalance quickly!, max team difference is " + UTIL_ToString( m_UNM->m_gameovermaxteamdifference ) + " " + string( 1, m_UNM->m_CommandTrigger ) + "override to cancel" );
                m_GameOverTime = GetTime( ) + 95;
            }
        }

        // finish the gameover timer

        if( m_GameOverTime != 0 && GetTime( ) > m_GameOverTime + 25 )
        {
            bool AlreadyStopped = true;

            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            {
                if( !( *i )->GetDeleteMe( ) )
                {
                    AlreadyStopped = false;
                    break;
                }
            }

            if( !AlreadyStopped )
            {
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] is over (gameover timer finished)", 6, m_CurrentGameCounter );
                StopPlayers( "was disconnected (gameover timer finished)" );
            }
        }
    }

    // censor muted process

    ProcessCensorMuted( );

    // check how many slots are unoccupied and announce if needed

    if( m_UNM->m_LobbyAnnounceUnoccupied && !m_GameLoaded && !m_GameLoading && m_AutoStartPlayers == 0 && GetSlotsOpen( ) != m_LastSlotsUnoccupied )
    {
        m_LastSlotsUnoccupied = GetSlotsOpen( );

        if( m_LastSlotsUnoccupied == 1 || m_LastSlotsUnoccupied == 2 )
            SendAllChat( "+" + UTIL_ToString( m_LastSlotsUnoccupied ) );
    }

    // check if all slots are no longer occupied

    if( !m_GameLoaded && !m_GameLoading && m_AllSlotsOccupied && GetSlotsOpen( ) != 0 )
    {
        m_AllSlotsOccupied = false;
        m_AllSlotsAnnounced = false;
    }

    // if all slots occupied for 3 seconds, announce in the lobby

    if( !m_CountDownStarted && !m_GameLoaded && !m_GameLoading && GetSlotsOpen( ) == 0 && m_AllSlotsOccupied && !m_AllSlotsAnnounced && GetTime( ) - m_SlotsOccupiedTime > 3 )
    {
        m_AllSlotsAnnounced = true;
        string Pings = gLanguage->AdviseWhenLobbyFull( string( 1, m_UNM->m_CommandTrigger ) );
        uint32_t Ping;

        // copy the m_Players vector so we can sort by descending ping so it's easier to find players with high pings
        vector<CGamePlayer *> SortedPlayers = m_Players;
        sort( SortedPlayers.begin( ), SortedPlayers.end( ), CGamePlayerSortDescByPing( ) );

        for( vector<CGamePlayer *> ::iterator i = SortedPlayers.begin( ); i != SortedPlayers.end( ); i++ )
        {
            if( ( *i )->GetNumPings( ) > 0 )
            {
                Ping = ( *i )->GetPing( m_UNM->m_LCPings );
                Pings += UTIL_ToString( Ping ) + "ms";
            }
            else
                Pings += "N/A";

            if( i != SortedPlayers.end( ) - 1 )
                Pings += ", ";
        }

        SendAllChat( Pings );
    }

    // check if we're rehosting

    if( m_RehostTime > 0 && GetTime( ) - m_RehostTime >= 3 )
    {
        m_Exiting = true;
        m_UNM->m_newGame = true;
    }

    // end the game if there aren't any players left

    if( m_Players.empty( ) && ( m_GameLoading || m_GameLoaded ) )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] завершена (не осталось игроков)", 6, m_CurrentGameCounter );
        return true;
    }

    // accept new connections

    if( m_Socket )
    {
        CTCPSocket *NewSocket = m_Socket->Accept( (fd_set *)fd );

        if( NewSocket )
        {
            // check the IP blacklist

            if( m_IPBlackList.find( NewSocket->GetIPString( ) ) == m_IPBlackList.end( ) )
            {
                NewSocket->SetNoDelay( true );
                m_Potentials.push_back( new CPotentialPlayer( m_Protocol, this, NewSocket ) );
            }
            else
            {
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] rejected connection from [" + NewSocket->GetIPString( ) + "] due to blacklist", 6, m_CurrentGameCounter );
                delete NewSocket;
            }
        }

        if( m_Socket->HasError( ) )
            return true;
    }

    return m_Exiting;
}

void CBaseGame::UpdatePost( void *send_fd )
{
    // we need to manually call DoSend on each player now because CGamePlayer::Update doesn't do it
    // this is in case player 2 generates a packet for player 1 during the update but it doesn't get sent because player 1 already finished updating
    // in reality since we're queueing actions it might not make a big difference but oh well

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( ( *i )->GetSocket( ) )
            ( *i )->GetSocket( )->DoSend( (fd_set *)send_fd, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Player [" + ( *i )->GetName( ) + "] caused an error:" );
    }

    for( vector<CPotentialPlayer *> ::iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); i++ )
    {
        if( ( *i )->GetSocket( ) )
        {
            if( !( *i )->GetName( ).empty( ) )
                ( *i )->GetSocket( )->DoSend( (fd_set *)send_fd, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Player [" + ( *i )->GetName( ) + "] caused an error:" );
            else
                ( *i )->GetSocket( )->DoSend( (fd_set *)send_fd, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Player caused an error:" );
        }
    }
}

void CBaseGame::Send( CGamePlayer *player, BYTEARRAY data )
{
    if( player )
        player->Send( data );
}

void CBaseGame::SendFromBuffer( CGamePlayer *player, BYTEARRAY data )
{
    if( player )
    {
        uint32_t PID = 0;

        for( uint32_t i = 6; i <= data.size( ); i++ )
        {
            if( data[i] == '@' )
            {
                PID = i;
                data.erase( data.begin( ) + i );
                break;
            }
        }

        if( PID != 0 && data.size( ) > PID )
        {
            m_Protocol->AssignLength( data );

            if( !ValidPID( data[PID] ) )
                data[PID] = GetHostPIDForMessageFromBuffer( player->GetPID( ) );

            player->Send( data );
        }
    }
}

void CBaseGame::Send( unsigned char PID, BYTEARRAY data )
{
    Send( GetPlayerFromPID( PID ), data );
}

void CBaseGame::Send( BYTEARRAY PIDs, BYTEARRAY data )
{
    for( uint32_t i = 0; i < PIDs.size( ); i++ )
        Send( PIDs[i], data );
}

void CBaseGame::SendAlly( unsigned char PID, BYTEARRAY data )
{
    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( m_Slots[GetSIDFromPID( ( *i )->GetPID( ) )].GetTeam( ) == m_Slots[GetSIDFromPID( PID )].GetTeam( ) )
            ( *i )->Send( data );
    }
}

void CBaseGame::SendEnemy( unsigned char PID, BYTEARRAY data )
{
    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( m_Slots[GetSIDFromPID( ( *i )->GetPID( ) )].GetTeam( ) != m_Slots[GetSIDFromPID( PID )].GetTeam( ) )
            ( *i )->Send( data );
    }
}

void CBaseGame::SendAll( BYTEARRAY data )
{
    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        ( *i )->Send( data );
}

void CBaseGame::SendAdmin( BYTEARRAY data )
{
    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( IsOwner( ( *i )->GetName( ) ) || IsDefaultOwner( ( *i )->GetName( ) ) )
            ( *i )->Send( data );
    }
}

void CBaseGame::SendChat( unsigned char fromPID, CGamePlayer *player, string message )
{
    SendChat0( fromPID, player, message, true );
}

void CBaseGame::SendChat( unsigned char fromPID, unsigned char toPID, string message )
{
    SendChat( fromPID, GetPlayerFromPID( toPID ), message );
}

void CBaseGame::SendChat( CGamePlayer *fromplayer, CGamePlayer *toplayer, string message )
{
    SendChat( fromplayer->GetPID( ), toplayer, message );
}

void CBaseGame::SendChat( CGamePlayer *player, unsigned char toPID, string message )
{
    SendChat( player->GetPID( ), GetPlayerFromPID( toPID ), message );
}

void CBaseGame::SendChat( CGamePlayer *player, string message )
{
    if( m_GameLoaded || m_GameLoading )
        SendChat( player->GetPID( ), player, message );
    else
        SendChat( GetHostPID( ), player, message );
}

void CBaseGame::SendChat( unsigned char toPID, string message )
{
    if( m_GameLoaded || m_GameLoading )
        SendChat( toPID, toPID, message );
    else
        SendChat( GetHostPID( ), toPID, message );
}

void CBaseGame::SendChat0( unsigned char fromPID, CGamePlayer *player, string message, bool privatemsg )
{
    // send a private message to one player - it'll be marked [Private] in Warcraft 3

    if( m_DetourAllMessagesToAdmins && player && !IsOwner( player->GetName( ) ) && !IsDefaultOwner( player->GetName( ) ) )
         return;

    if( player )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] [Private chat (To " + player->GetName( ) + ")]: " + message, 6, m_CurrentGameCounter );
        emit gToLog->toGameChat( QString::fromUtf8( message.c_str( ) ), GetColoredNameFromPID( player->GetPID( ) ), GetColoredNameFromPID( fromPID ), 0, m_CurrentGameCounter );

        if( !m_GameLoading && !m_GameLoaded )
        {
            uint32_t i = 0;
            string msg;
            bool onemoretime = false;

            do
            {
                msg = UTIL_SubStrFix( message, i, 220 );

                if( message.size( ) - i > 220 )
                {
                    i = i + 220;
                    onemoretime = true;
                }
                else
                    onemoretime = false;

                Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 16, BYTEARRAY( ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                if( m_UNM->m_CountDownMethod == 1 )
                {
                    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    {
                        if( !( *i )->GetLeftMessageSent( ) && ( ( *i )->GetPID( ) == fromPID || ( ( *i )->GetPID( ) != fromPID && ( *i )->GetPID( ) == player->GetPID( ) ) ) )
                        {
                            ( *i )->m_BufferMessages.push_back( m_Protocol->SEND_W3GS_CHAT_FROM_HOST_BUFFER( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 16, BYTEARRAY( ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                            if( ( *i )->m_BufferMessages.size( ) > 7 )
                                ( *i )->m_BufferMessages.pop_front( );
                        }
                    }
                }
            }
            while( onemoretime );
        }
        else
        {
            uint32_t i = 0;
            string msg;
            bool onemoretime = false;

            // based on my limited testing it seems that the extra flags' first byte contains 3 plus the recipient's colour to denote a private message

            unsigned char ExtraFlags[] = { 3, 0, 0, 0 };
            unsigned char SID = GetSIDFromPID( player->GetPID( ) );

            if( SID < m_Slots.size( ) )
                ExtraFlags[0] = 3 + m_Slots[SID].GetColour( );

            do
            {
                msg = UTIL_SubStrFix( message, i, 120 );

                if( message.size( ) - i > 120 )
                {
                    i = i + 120;
                    onemoretime = true;
                }
                else
                    onemoretime = false;

                if( privatemsg )
                    Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 32, UTIL_CreateByteArray( ExtraFlags, 4 ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                else
                    Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 32, UTIL_CreateByteArray( (uint32_t)0, false ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
            }
            while( onemoretime );
        }
    }
}

void CBaseGame::SendChat2( unsigned char fromPID, CGamePlayer *player, string message )
{
    SendChat0( fromPID, player, message, false );
}

void CBaseGame::SendChat2( unsigned char fromPID, unsigned char toPID, string message )
{
    SendChat2( fromPID, GetPlayerFromPID( toPID ), message );
}

void CBaseGame::SendChat2( CGamePlayer *fromplayer, CGamePlayer *toplayer, string message )
{
    SendChat2( fromplayer->GetPID( ), toplayer, message );
}

void CBaseGame::SendChat2( CGamePlayer *player, unsigned char toPID, string message )
{
    SendChat2( player->GetPID( ), GetPlayerFromPID( toPID ), message );
}

void CBaseGame::SendChat2( CGamePlayer *player, string message )
{
    if( m_GameLoaded || m_GameLoading )
        SendChat2( player->GetPID( ), player, message );
    else
        SendChat2( GetHostPID( ), player, message );
}

void CBaseGame::SendChat2( unsigned char toPID, string message )
{
    if( m_GameLoaded || m_GameLoading )
        SendChat2( toPID, toPID, message );
    else
        SendChat2( GetHostPID( ), toPID, message );
}

void CBaseGame::SendAllChat( unsigned char fromPID, string message )
{
    // send a public message to all players - it'll be marked [All] in Warcraft 3

    if( GetNumHumanPlayers( ) > 0 )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] [All chat]: " + message, 6, m_CurrentGameCounter );
        emit gToLog->toGameChat( QString::fromUtf8( message.c_str( ) ), QString( ), GetColoredNameFromPID( fromPID ), 1, m_CurrentGameCounter );

        if( !m_GameLoading && !m_GameLoaded )
        {
            uint32_t i = 0;
            string msg;
            bool onemoretime = false;

            do
            {
                msg = UTIL_SubStrFix( message, i, 220 );

                if( message.size( ) - i > 220 )
                {
                    i = i + 220;
                    onemoretime = true;
                }
                else
                    onemoretime = false;

                // this is a lobby unm chat message

                if( m_DetourAllMessagesToAdmins )
                {
                    SendAdmin( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetAdminPIDs( ), 16, BYTEARRAY( ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                    if( m_UNM->m_CountDownMethod == 1 )
                    {
                        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                        {
                            if( !( *i )->GetLeftMessageSent( ) && ( IsOwner( ( *i )->GetName( ) ) ) )
                            {
                                ( *i )->m_BufferMessages.push_back( m_Protocol->SEND_W3GS_CHAT_FROM_HOST_BUFFER( fromPID, GetAdminPIDs( ), 16, BYTEARRAY( ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                                if( ( *i )->m_BufferMessages.size( ) > 7 )
                                    ( *i )->m_BufferMessages.pop_front( );
                            }
                        }
                    }
                }
                else
                {
                    SendAll( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetPIDs( ), 16, BYTEARRAY( ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                    if( m_UNM->m_CountDownMethod == 1 )
                    {
                        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                        {
                            if( !( *i )->GetLeftMessageSent( ) )
                            {
                                ( *i )->m_BufferMessages.push_back( m_Protocol->SEND_W3GS_CHAT_FROM_HOST_BUFFER( fromPID, GetPIDs( ), 16, BYTEARRAY( ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                                if( ( *i )->m_BufferMessages.size( ) > 7 )
                                    ( *i )->m_BufferMessages.pop_front( );
                            }
                        }
                    }
                }
            }
            while( onemoretime );
        }
        else
        {
            uint32_t i = 0;
            string msg;
            bool onemoretime = false;

            do
            {
                msg = UTIL_SubStrFix( message, i, 120 );

                if( message.size( ) - i > 120 )
                {
                    i = i + 120;
                    onemoretime = true;
                }
                else
                    onemoretime = false;

                // this is an ingame unm chat message

                if( m_GameLoaded )
                {
                    if( m_DetourAllMessagesToAdmins )
                        SendAdmin( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetAdminPIDs( ), 32, UTIL_CreateByteArray( (uint32_t)0, false ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                    else
                        SendAll( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetPIDs( ), 32, UTIL_CreateByteArray( (uint32_t)0, false ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                }
                else
                {
                    if( m_DetourAllMessagesToAdmins )
                    {
                        for( vector<CGamePlayer *> ::iterator p = m_Players.begin( ); p != m_Players.end( ); p++ )
                        {
                            if( IsOwner( ( *p )->GetName( ) ) )
                            {
                                if( ( *p )->GetFinishedLoading( ) )
                                    ( *p )->Send( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetAdminPIDs( ), 32, UTIL_CreateByteArray( (uint32_t)0, false ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                                else
                                {
                                    ( *p )->m_MessagesBeforeLoading.push_back( msg );
                                    ( *p )->m_MessagesAuthorsBeforeLoading.push_back( fromPID );
                                    ( *p )->m_MessagesFlagsBeforeLoading.push_back( 32 );
                                    ( *p )->m_MessagesExtraFlagsBeforeLoading.push_back( UTIL_CreateByteArray( (uint32_t)0, false ) );
                                }
                            }
                        }
                    }
                    else
                    {
                        for( vector<CGamePlayer *> ::iterator p = m_Players.begin( ); p != m_Players.end( ); p++ )
                        {
                            if( ( *p )->GetFinishedLoading( ) )
                                ( *p )->Send( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetPIDs( ), 32, UTIL_CreateByteArray( (uint32_t)0, false ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                            else
                            {
                                ( *p )->m_MessagesBeforeLoading.push_back( msg );
                                ( *p )->m_MessagesAuthorsBeforeLoading.push_back( fromPID );
                                ( *p )->m_MessagesFlagsBeforeLoading.push_back( 32 );
                                ( *p )->m_MessagesExtraFlagsBeforeLoading.push_back( UTIL_CreateByteArray( (uint32_t)0, false ) );
                            }
                        }
                    }
                }
            }
            while( onemoretime );

            if( m_Replay )
                m_Replay->AddChatMessage( fromPID, 32, 0, message );
        }
    }
}

void CBaseGame::SendAllyChat( unsigned char fromPID, string message )
{
    // send a public message to ally players - it'll be marked [Ally] in Warcraft 3

    if( GetNumPlayers( ) > 0 )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] [Ally Chat (Team " + UTIL_ToString( m_Slots[GetSIDFromPID( fromPID )].GetTeam( ) + 1 ) + ")]: " + message, 6, m_CurrentGameCounter );
        emit gToLog->toGameChat( QString::fromUtf8( message.c_str( ) ), "(Team " + UTIL_ToQString( m_Slots[GetSIDFromPID( fromPID )].GetTeam( ) + 1 ) + ")", GetColoredNameFromPID( fromPID ), 2, m_CurrentGameCounter );

        if( !m_GameLoading && !m_GameLoaded )
        {
            int i = 0;
            string msg;
            bool onemoretime = false;

            do
            {
                msg = UTIL_SubStrFix( message, i, 220 );

                if( message.size( ) - i > 220 )
                {
                    i = i + 220;
                    onemoretime = true;
                }
                else
                    onemoretime = false;

                // this is a lobby unm chat message
                unsigned char ExtraFlags[] = { 1, 0, 0, 0 };
                SendAlly( fromPID, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetAllyPIDs( fromPID ), 16, UTIL_CreateByteArray( ExtraFlags, 4 ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                unsigned char team = m_Slots[GetSIDFromPID( fromPID )].GetTeam( );

                if( m_UNM->m_CountDownMethod == 1 )
                {
                    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    {
                        if( !( *i )->GetLeftMessageSent( ) && ( ( *i )->GetPID( ) == fromPID || ( ( *i )->GetPID( ) != fromPID && GetTeam( GetSIDFromPID( ( *i )->GetPID( ) ) ) == team ) ) )
                        {
                            ( *i )->m_BufferMessages.push_back( m_Protocol->SEND_W3GS_CHAT_FROM_HOST_BUFFER( fromPID, GetAllyPIDs( fromPID ), 16, UTIL_CreateByteArray( ExtraFlags, 4 ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                            if( ( *i )->m_BufferMessages.size( ) > 7 )
                                ( *i )->m_BufferMessages.pop_front( );
                        }
                    }
                }
            }
            while( onemoretime );
        }
        else
        {
            int i = 0;
            string msg;
            bool onemoretime = false;

            do
            {
                msg = UTIL_SubStrFix( message, i, 120 );

                if( message.size( ) - i > 120 )
                {
                    i = i + 120;
                    onemoretime = true;
                }
                else
                    onemoretime = false;

                // this is an ingame unm chat message, print it to the console
                unsigned char ExtraFlags[] = { 1, 0, 0, 0 };
                SendAlly( fromPID, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetAllyPIDs( fromPID ), 32, UTIL_CreateByteArray( ExtraFlags, 4 ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
            }
            while( onemoretime );

            if( m_Replay )
                m_Replay->AddChatMessage( fromPID, 32, 0, message );
        }
    }
}

void CBaseGame::SendAdminChat( unsigned char fromPID, string message )
{
    // send a public message to admin players - it'll be marked [Ally] in Warcraft 3

    if( GetNumPlayers( ) > 0 )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] [Admin chat]: " + message, 6, m_CurrentGameCounter );
        emit gToLog->toGameChat( QString::fromUtf8( message.c_str( ) ), QString( ), GetColoredNameFromPID( fromPID ), 3, m_CurrentGameCounter );

        if( !m_GameLoading && !m_GameLoaded )
        {
            int i = 0;
            string msg;
            bool onemoretime = false;

            do
            {
                msg = UTIL_SubStrFix( message, i, 220 );

                if( message.size( ) - i > 220 )
                {
                    i = i + 220;
                    onemoretime = true;
                }
                else
                    onemoretime = false;

                // this is a lobby unm chat message

                unsigned char ExtraFlags[] = { 1, 0, 0, 0 };
                SendAdmin( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetAdminPIDs( ), 16, UTIL_CreateByteArray( ExtraFlags, 4 ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                if( m_UNM->m_CountDownMethod == 1 )
                {
                    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    {
                        if( !( *i )->GetLeftMessageSent( ) && ( IsOwner( ( *i )->GetName( ) ) ) )
                        {
                            ( *i )->m_BufferMessages.push_back( m_Protocol->SEND_W3GS_CHAT_FROM_HOST_BUFFER( fromPID, GetAdminPIDs( ), 16, UTIL_CreateByteArray( ExtraFlags, 4 ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                            if( ( *i )->m_BufferMessages.size( ) > 7 )
                                ( *i )->m_BufferMessages.pop_front( );
                        }
                    }
                }
            }
            while( onemoretime );
        }
        else
        {
            int i = 0;
            string msg;
            bool onemoretime = false;

            do
            {
                msg = UTIL_SubStrFix( message, i, 120 );

                if( message.size( ) - i > 120 )
                {
                    i = i + 120;
                    onemoretime = true;
                }
                else
                    onemoretime = false;

                // this is an ingame unm chat message, print it to the console
                unsigned char ExtraFlags[] = { 1, 0, 0, 0 };
                SendAdmin( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetAdminPIDs( ), 32, UTIL_CreateByteArray( ExtraFlags, 4 ), msg, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
            }
            while( onemoretime );

            if( m_Replay )
                m_Replay->AddChatMessage( fromPID, 32, 0, message );
        }
    }
}

void CBaseGame::SendAllChat( string message )
{
    SendAllChat( GetHostPID( ), message );
}

void CBaseGame::SendAllChat( CGamePlayer *player, string message )
{
    SendAllChat( player->GetPID( ), message );
}

void CBaseGame::SendAllyChat( string message )
{
    SendAllyChat( GetHostPID( ), message );
}

void CBaseGame::SendAdminChat( string message )
{
    SendAdminChat( GetHostPID( ), message );
}

void CBaseGame::SendLocalAdminChat( string message )
{
    if( !m_LocalAdminMessages )
        return;

    // send a message to LAN/local players who are admins
    // at the time of this writing it is only possible for the game owner to meet this criteria because being an admin requires spoof checking
    // this is mainly used for relaying battle.net whispers, chat messages, and emotes to these players

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( ( *i )->GetSpoofed( ) && IsOwner( ( *i )->GetName( ) ) && ( UTIL_IsLanIP( ( *i )->GetExternalIP( ) ) || UTIL_IsLocalIP( ( *i )->GetExternalIP( ), m_UNM->m_LocalAddresses ) ) )
        {
            if( m_VirtualHostPID != 255 )
                SendChat( m_VirtualHostPID, *i, message );
            else
            {
                // make the chat message originate from the recipient since it's not going to be logged to the replay

                SendChat( ( *i )->GetPID( ), *i, message );
            }
        }
    }
}

void CBaseGame::SendAllSlotInfo( )
{
    if( !m_GameLoading && !m_GameLoaded )
    {
        SendAll( m_Protocol->SEND_W3GS_SLOTINFO( m_Slots, m_RandomSeed, m_Map->GetMapGameType( ) == GAMETYPE_CUSTOM ? 3 : 0, static_cast<unsigned char>(m_Map->GetMapNumPlayers( )) ) );
        m_SlotInfoChanged = false;
    }
}

void CBaseGame::SendVirtualHostPlayerInfo( CGamePlayer *player )
{
    if( m_VirtualHostPID == 255 )
        return;

    BYTEARRAY IP;
    IP.push_back( 0 );
    IP.push_back( 0 );
    IP.push_back( 0 );
    IP.push_back( 0 );
    Send( player, m_Protocol->SEND_W3GS_PLAYERINFO( m_VirtualHostPID, m_VirtualHostName, IP, IP, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
}

void CBaseGame::SendObservePlayerInfo( CGamePlayer *player )
{
    if( m_ObsPlayerPID == 255 )
        return;

    BYTEARRAY IP;
    IP.push_back( 0 );
    IP.push_back( 0 );
    IP.push_back( 0 );
    IP.push_back( 0 );
    Send( player, m_Protocol->SEND_W3GS_PLAYERINFO( m_ObsPlayerPID, m_ObsPlayerName, IP, IP, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
}

void CBaseGame::SendFakePlayerInfo( CGamePlayer *player )
{
    if( m_FakePlayers.empty( ) )
        return;

    BYTEARRAY IP;
    IP.push_back( 0 );
    IP.push_back( 0 );
    IP.push_back( 0 );
    IP.push_back( 0 );
    uint32_t fpnubmer = 0;

    for( vector<unsigned char> ::iterator i = m_FakePlayers.begin( ); i != m_FakePlayers.end( ); ++i )
    {
        string s;

        if( m_FakePlayersNames.size( ) > fpnubmer )
            s = UTIL_SubStrFix( m_FakePlayersNames[fpnubmer], 0, 15 );
        else
            s = UTIL_SubStrFix( m_UNM->GetFPName( ), 0, 15 );

        if( s.empty( ) )
            s = "Wizard[" + UTIL_ToString( *i ) + "]";

        Send( player, m_Protocol->SEND_W3GS_PLAYERINFO( *i, s, IP, IP, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
        fpnubmer++;
    }
}

void CBaseGame::SendAllActions( )
{
    bool UsingGProxy = false;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( ( *i )->GetGProxy( ) )
        {
            UsingGProxy = true;
            break;
        }
    }

    m_GameTicks += m_DynamicLatency;

    if( UsingGProxy )
    {
        // we must send empty actions to non-GProxy++ players
        // GProxy++ will insert these itself so we don't need to send them to GProxy++ players
        // empty actions are used to extend the time a player can use when reconnecting

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( !( *i )->GetGProxy( ) )
            {
                for( unsigned char j = 0; j < m_GProxyEmptyActions; j++ )
                    Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
            }
        }

        // add actions to replay

        if( m_Replay )
        {
            for( unsigned char i = 0; i < m_GProxyEmptyActions; i++ )
                m_Replay->AddTimeSlot( 0, queue<CIncomingAction *>( ) );
        }
    }

    if( !GetPause( ) )
        m_SyncCounter++;

    // we aren't allowed to send more than 1460 bytes in a single packet but it's possible we might have more than that many bytes waiting in the queue

    if( !m_Actions.empty( ) )
    {
        // we use a "sub actions queue" which we keep adding actions to until we reach the size limit
        // start by adding one action to the sub actions queue

        queue<CIncomingAction *> SubActions;
        CIncomingAction *Action = m_Actions.front( );
        m_Actions.pop( );
        SubActions.push( Action );
        uint32_t SubActionsLength = Action->GetLength( );

        while( !m_Actions.empty( ) )
        {
            Action = m_Actions.front( );
            m_Actions.pop( );

            // check if adding the next action to the sub actions queue would put us over the limit (1452 because the INCOMING_ACTION and INCOMING_ACTION2 packets use an extra 8 bytes)

            if( SubActionsLength + Action->GetLength( ) > 1452 )
            {
                // we'd be over the limit if we added the next action to the sub actions queue
                // so send everything already in the queue and then clear it out
                // the W3GS_INCOMING_ACTION2 packet handles the overflow but it must be sent *before* the corresponding W3GS_INCOMING_ACTION packet

                SendAll( m_Protocol->SEND_W3GS_INCOMING_ACTION2( SubActions ) );

                if( m_Replay )
                    m_Replay->AddTimeSlot2( SubActions );

                while( !SubActions.empty( ) )
                {
                    delete SubActions.front( );
                    SubActions.pop( );
                }

                SubActionsLength = 0;
            }

            SubActions.push( Action );
            SubActionsLength += Action->GetLength( );
        }

        SendAll( m_Protocol->SEND_W3GS_INCOMING_ACTION( SubActions, (uint16_t)m_DynamicLatency ) );

        if( m_Replay )
            m_Replay->AddTimeSlot( (uint16_t)m_DynamicLatency, SubActions );

        while( !SubActions.empty( ) )
        {
            delete SubActions.front( );
            SubActions.pop( );
        }
    }
    else
    {
        SendAll( m_Protocol->SEND_W3GS_INCOMING_ACTION( m_Actions, (uint16_t)m_DynamicLatency ) );

        if( m_Replay )
            m_Replay->AddTimeSlot( (uint16_t)m_DynamicLatency, m_Actions );
    }

    uint32_t ActualSendInterval = GetTicks( ) - m_LastActionSentTicks;
    uint32_t ExpectedSendInterval = m_DynamicLatency - m_LastActionLateBy;
    m_LastActionLateBy = ActualSendInterval - ExpectedSendInterval;

    if( m_LastActionLateBy > m_DynamicLatency )
    {
        // something is going terribly wrong - UNM is probably starved of resources
        // print a message because even though this will take more resources it should provide some information to the administrator for future reference
        // other solutions - dynamically modify the latency, request higher priority, terminate other games, ???

        if( m_UNM->m_WarnLatency )
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] warning - the latency is " + UTIL_ToString( m_DynamicLatency ) + "ms but the last update was late by " + UTIL_ToString( m_LastActionLateBy ) + "ms", 6, m_CurrentGameCounter );

        m_LastActionLateBy = m_DynamicLatency;
        m_DynamicLatencyBotLagging = true;
    }

    m_LastActionSentTicks = GetTicks( );
}

void CBaseGame::SendWelcomeMessage( CGamePlayer *player, uint32_t HostCounterID )
{
    if( ( m_UNM->m_BotNamePrivatePrintJoin || m_UNM->m_BotChannelPrivatePrintJoin ) && HostCounterID != 0 )
    {
        string BotServer = string( );
        string BotName = string( );
        string BotChannel = string( );

        if( m_UNM->m_BotNameCustom.find_first_not_of( " " ) != string::npos )
            BotName = m_UNM->m_BotNameCustom;

        if( m_UNM->m_BotNameCustom.find_first_not_of( " " ) != string::npos )
            BotChannel = m_UNM->m_BotChannelCustom;

        if( ( BotName.empty( ) && m_UNM->m_BotNamePrivatePrintJoin ) || ( BotChannel.empty( ) && m_UNM->m_BotChannelPrivatePrintJoin ) )
        {
            for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
            {
                if( ( *i )->GetHostCounterID( ) == HostCounterID )
                {
                    BotServer = ( *i )->GetServer( );
                    break;
                }
            }

            if( !BotServer.empty( ) )
            {
                for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                {
                    if( !( *i )->m_SpamSubBot && ( *i )->GetServer( ) == BotServer )
                    {
                        if( BotName.empty( ) )
                            BotName = ( *i )->GetName( );

                        if( BotChannel.empty( ) )
                            BotChannel = ( *i )->GetFirstChannel( );

                        break;
                    }
                }

                if( ( BotName.empty( ) && m_UNM->m_BotNamePrivatePrintJoin ) || ( BotChannel.empty( ) && m_UNM->m_BotChannelPrivatePrintJoin ) )
                {
                    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                    {
                        if( ( *i )->GetHostCounterID( ) == HostCounterID )
                        {
                            if( BotName.empty( ) )
                                BotName = ( *i )->GetName( );

                            if( BotChannel.empty( ) )
                                BotChannel = ( *i )->GetFirstChannel( );

                            break;
                        }
                    }
                }
            }
        }

        if( !BotName.empty( ) && m_UNM->m_BotNamePrivatePrintJoin )
            SendChat( player, gLanguage->BotName( BotName ) );

        if( !BotChannel.empty( ) && m_UNM->m_BotChannelPrivatePrintJoin )
            SendChat( player, gLanguage->BotChannel( BotChannel ) );
    }

    if( m_UNM->m_InfoGamesPrivatePrintJoin )
    {
        uint32_t NowOnline = 0;

        for( uint32_t i = 0; i < m_UNM->m_Games.size( ); i++ )
            NowOnline += m_UNM->m_Games[i]->GetNumHumanPlayers( );

        if( m_UNM->m_CurrentGame )
            NowOnline += m_UNM->m_CurrentGame->GetNumHumanPlayers( );

        SendChat( player, "Активных игр [" + UTIL_ToString( m_UNM->m_Games.size( ) ) + "] → Игроков онлайн [" + UTIL_ToString( NowOnline ) + "]" );
    }

    if( m_GameName.find_first_not_of( " " ) != string::npos && m_UNM->m_GameNamePrivatePrintJoin )
        SendChat( player, gLanguage->GameName( m_GameName ) );

    if( m_OwnerName.find_first_not_of( " " ) != string::npos && m_UNM->m_GameOwnerPrivatePrintJoin != 0 )
    {
        if( m_OwnerName == m_DefaultOwner )
            SendChat( player, gLanguage->GameHost( m_DefaultOwner ) );
        else if( m_OwnerName.find( " " ) == string::npos )
            SendChat( player, gLanguage->GameOwner( m_OwnerName ) );
        else if( m_UNM->m_GameOwnerPrivatePrintJoin == 2 )
            SendChat( player, gLanguage->GameOwners( m_OwnerName ) );
        else
        {
            string LastOwner = m_OwnerName;

            while( !LastOwner.empty( ) && LastOwner.back( ) == ' ' )
                LastOwner.erase( LastOwner.end( ) - 1 );

            string::size_type LastOwnerStart = LastOwner.rfind( " " );

            if( LastOwnerStart != string::npos )
                LastOwner = LastOwner.substr( LastOwnerStart + 1 );

            if( LastOwner == m_DefaultOwner )
                SendChat( player, gLanguage->GameHost( m_DefaultOwner ) );
            else
                SendChat( player, gLanguage->GameOwner( LastOwner ) );
        }
    }

    if( m_HCLCommandString.find_first_not_of( " " ) != string::npos && m_UNM->m_GameModePrivatePrintJoin )
        SendChat( player, gLanguage->GameMode( m_HCLCommandString ) );

    if( m_UNM->m_PlayerBeforeStartPrivatePrintJoin && m_AutoStartPlayers > 0 )
    {
        if( ( m_UNM->m_PlayerBeforeStartPrintDelay > m_ActuallyPlayerBeforeStartPrintDelay + 2 ) || !m_UNM->m_PlayerBeforeStartPrivatePrintJoinSD || !m_UNM->m_PlayerBeforeStartPrint )
        {
            uint32_t PNr = GetNumHumanPlayers( );

            if( PNr < m_AutoStartPlayers )
            {
                string s = gLanguage->WaitingForPlayersBeforeAutoStartVS( UTIL_ToString( m_AutoStartPlayers ), UTIL_ToString( m_AutoStartPlayers - PNr ), string( 1, m_UNM->m_CommandTrigger ) );
                SendChat( player, s );
            }
        }
    }

    for( vector<string> ::iterator i = m_UNM->m_Welcome.begin( ); i != m_UNM->m_Welcome.end( ); i++ )
        SendChat( player, ( *i ) );
}

void CBaseGame::EventPlayerDeleted( CGamePlayer *player )
{
    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] deleting player [" + player->GetName( ) + "] (" + player->GetExternalIPString( ) + "): " + player->GetLeftReason( ), 6, m_CurrentGameCounter );
    bool Pause = false;
    player->SetLeftTime( m_GameTicks / 1000 );

    if( IsListening( player->GetPID( ) ) )
        DelFromListen( player->GetPID( ) );

    if( m_GameLoaded )
    {
        if( player->GetName( ) == m_FPHCLUser )
        {
            m_FPHCLNumber = 0;
            m_FPHCLUser = string( );
            m_FPHCLTime = GetTime( );
        }

        bool checkfpcopyused = false;

        for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
        {
            if( m_FakePlayersOwner[i] == player->GetName( ) )
            {
                m_FakePlayersOwner[i] = string( );
                m_FakePlayersStatus[i] = 0;
                m_FakePlayersActSelection[i].clear( );
            }
            else if( !m_FakePlayersOwner[i].empty( ) )
                checkfpcopyused = true;
        }

        m_FakePlayersCopyUsed = checkfpcopyused;
    }

    if( !m_GameLoading && !m_GameLoaded && m_UNM->m_SpoofChecks != 0 )
    {
        if( IsSpoofOfline( player->GetName( ) ) )
            DelSpoofOfline( player->GetName( ) );

        uint32_t numspoofofline = 0;

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( IsSpoofOfline( ( *i )->GetName( ) ) )
                numspoofofline++;
        }

        if( m_NumSpoofOfline > numspoofofline )
            m_NumSpoofOfline = numspoofofline;
    }

    // resume game before player deleteted if the game is paused

    if( m_Pause )
    {
        if( !m_PauseEnd )
            Pause = true;

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            ( *i )->SetLagging( false );
            ( *i )->SetStartedLaggingTicks( 0 );
        }

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            ( *i )->SetLagging( true );
            ( *i )->SetStartedLaggingTicks( GetTicks( ) );
            ( *i )->Send( m_Protocol->SEND_W3GS_STOP_LAG( *i ) );
            ( *i )->SetLagging( false );
            ( *i )->SetStartedLaggingTicks( 0 );
        }

        m_Pause = false;
        m_PauseEnd = false;
    }

    // remove any queued spoofcheck messages for this player

    if( player->GetWhoisSent( ) && m_UNM->IsBnetServer( player->GetJoinedRealm( ) ) && player->GetSpoofedRealm( ).empty( ) )
    {
        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
        {
            if( ( *i )->GetServer( ) == player->GetJoinedRealm( ) )
            {
                // hackhack: there must be a better way to do this

                ( *i )->UnqueueChatCommand( "/whereis " + player->GetName( ) );
                ( *i )->UnqueueChatCommand( "/w " + player->GetName( ) + " " + gLanguage->SpoofCheckByReplying( ) );
            }
        }
    }

    m_LastPlayerLeaveTicks = GetTicks( );

    // in some cases we're forced to send the left message early so don't send it again

    if( player->GetLeftMessageSent( ) )
        return;

    player->SetLeftMessageSent( true );

    while( !player->m_MessagesBeforeLoading.empty( ) )
    {
        player->m_MessagesBeforeLoading.pop_front( );
        player->m_MessagesAuthorsBeforeLoading.pop_front( );
        player->m_MessagesFlagsBeforeLoading.pop_front( );
        player->m_MessagesExtraFlagsBeforeLoading.pop_front( );
    }

    ReCalculateTeams( );

    if( player->GetLagging( ) )
        SendAll( m_Protocol->SEND_W3GS_STOP_LAG( player ) );

    // autosave

    if( m_GameLoaded && player->GetLeftCode( ) == PLAYERLEAVE_DISCONNECT && m_AutoSave )
    {
        string SaveGameName = UTIL_FileSafeName( "UNM AutoSave " + m_UNM->IncGameNrDecryption( m_GameName ) + " (" + player->GetName( ) + ").w3z" );
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] auto saving [" + SaveGameName + "] before player drop, shortened send interval = " + UTIL_ToString( GetTicks( ) - m_LastActionSentTicks ), 6, m_CurrentGameCounter );
        BYTEARRAY CRC;
        BYTEARRAY Action;
        Action.push_back( 6 );
        UTIL_AppendByteArray( Action, SaveGameName );
        m_Actions.push( new CIncomingAction( player->GetPID( ), CRC, Action ) );

        // todotodo: with the new latency system there needs to be a way to send a 0-time action

        SendAllActions( );
    }

    if( m_GameLoading && m_LoadInGame )
    {
        // we must buffer player leave messages when using "load in game" to prevent desyncs
        // this ensures the player leave messages are correctly interleaved with the empty updates sent to each player

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( ( *i )->GetFinishedLoading( ) )
            {
                if( !player->GetFinishedLoading( ) )
                    Send( *i, m_Protocol->SEND_W3GS_STOP_LAG( player ) );

                Send( *i, m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( player->GetPID( ), player->GetLeftCode( ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
            }
            else
                ( *i )->AddLoadInGameData( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( player->GetPID( ), player->GetLeftCode( ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
        }
    }
    else
    {
        // tell everyone about the player leaving or turn leave player to fakeplayer

        if( !m_GameLoading && m_GameLoaded && m_Players.size( ) == 2 && m_DontDeletePenultimatePlayer && m_VirtualHostPID == 255 && m_ObsPlayerPID == 255 && m_FakePlayers.empty( ) )
        {
            if( player->GetLeftReason( ) == gLanguage->HasLostConnectionPlayerError( player->GetErrorString( ) ) || player->GetLeftReason( ) == gLanguage->HasLostConnectionSocketError( player->GetSocket( )->GetErrorString( ) ) || player->GetLeftReason( ) == gLanguage->HasLostConnectionClosedByRemoteHost( ) || player->GetLeftReason( ) == gLanguage->HasLeftVoluntarily( ) )
            {
                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    if( IsOwner( ( *i )->GetName( ) ) )
                        SendChat2( GetHostPID( ), ( *i )->GetPID( ), player->GetName( ) + " " + player->GetLeftReason( ) + " и был заменен фейк-плеером (юзай " + string( 1, m_UNM->m_CommandTrigger ) + "kickfp чтобы кикнуть)" );
                    else
                        SendChat2( GetHostPID( ), ( *i )->GetPID( ), player->GetName( ) + " " + gLanguage->HasLeftTheGame( ) + " и был заменен фейк-плеером." );
                }
            }
            else
            {
                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    if( IsOwner( ( *i )->GetName( ) ) )
                        SendChat2( GetHostPID( ), ( *i )->GetPID( ), player->GetName( ) + " " + player->GetLeftReason( ) + " и был заменен фейк-плеером (юзай " + string( 1, m_UNM->m_CommandTrigger ) + "kickfp чтобы кикнуть)" );
                    else
                        SendChat2( GetHostPID( ), ( *i )->GetPID( ), player->GetName( ) + " " + player->GetLeftReason( ) + " и был заменен фейк-плеером." );
                }
            }

            BYTEARRAY fpaction;
            m_FakePlayers.push_back( player->GetPID( ) );
            m_FakePlayersNames.push_back( player->GetName( ) );
            m_FakePlayersActSelection.push_back( fpaction );
            m_FakePlayersOwner.push_back( string( ) );
            m_FakePlayersStatus.push_back( 0 );
        }
        else
        {
            if( m_GameLoaded && ( player->GetLeftReason( ) == gLanguage->HasLostConnectionPlayerError( player->GetErrorString( ) ) || player->GetLeftReason( ) == gLanguage->HasLostConnectionSocketError( player->GetSocket( )->GetErrorString( ) ) || player->GetLeftReason( ) == gLanguage->HasLostConnectionClosedByRemoteHost( ) || player->GetLeftReason( ) == gLanguage->HasLeftVoluntarily( ) ) )
            {
                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    if( IsOwner( ( *i )->GetName( ) ) )
                        SendChat2( GetHostPID( ), ( *i )->GetPID( ), player->GetName( ) + " " + player->GetLeftReason( ) );
                    else
                        SendChat2( GetHostPID( ), ( *i )->GetPID( ), player->GetName( ) + " " + gLanguage->HasLeftTheGame( ) );
                }
            }
            else if( m_GameLoaded )
                SendAllChat( player->GetName( ) + " " + player->GetLeftReason( ) );

            SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( player->GetPID( ), player->GetLeftCode( ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
        }
    }

    if( !m_GameLoading && !m_GameLoaded )
    {
        if( m_UNM->m_PlayerBeforeStartPrintLeave && m_AutoStartPlayers > 0 )
        {
            uint32_t PNr = GetNumHumanPlayers( );

            if( PNr < m_AutoStartPlayers )
            {
                string s = gLanguage->WaitingForPlayersBeforeAutoStartVS( UTIL_ToString( m_AutoStartPlayers ), UTIL_ToString( m_AutoStartPlayers - PNr ), string( 1, m_UNM->m_CommandTrigger ) );
                SendAllChat( s );
            }
        }
    }

    // set the replay's host PID and name to the last player to leave the game
    // this will get overwritten as each player leaves the game so it will eventually be set to the last player

    if( m_Replay && ( m_GameLoading || m_GameLoaded ) )
    {
        m_Replay->SetHostPID( player->GetPID( ) );
        m_Replay->SetHostName( player->GetName( ) );

        // add leave message to replay

        if( m_GameLoading && !m_LoadInGame )
            m_Replay->AddLeaveGameDuringLoading( 1, player->GetPID( ), player->GetLeftCode( ) );
        else
            m_Replay->AddLeaveGame( 1, player->GetPID( ), player->GetLeftCode( ) );
    }

    // abort the countdown if there was one in progress

    if( !m_NormalCountdown && m_CountDownStarted && !m_GameLoading && !m_GameLoaded )
    {
        SendAllChat( gLanguage->CountDownAborted( ) );
        m_CountDownStarted = false;
    }

    // abort the votekick

    if( !m_KickVotePlayer.empty( ) )
        SendAllChat( gLanguage->VoteKickCancelled( m_KickVotePlayer ) );

    m_KickVotePlayer.clear( );
    m_StartedKickVoteTime = 0;

    // abort the votestart

    if( m_StartedVoteStartTime != 0 )
    {
        if( m_UNM->m_CancelVoteStartIfLeave || ( GetNumHumanPlayers( ) < m_UNM->m_VoteStartMinPlayers ) )
        {
            SendAllChat( "Голосование за досрочный старт отменено!" );
            m_StartedVoteStartTime = 0;
        }
        else
        {
            uint32_t VotesNeeded;
            uint32_t Votes = 0;

            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
            {
                if( !( *i )->GetLeftMessageSent( ) && ( *i )->GetStartVote( ) )
                    ++Votes;
            }

            if( m_UNM->m_VoteStartPercentalVoting )
                VotesNeeded = (uint32_t)ceil( float( float( GetNumHumanPlayers( ) * m_UNM->m_VoteStartPercent ) / 100 ) );
            else
                VotesNeeded = m_UNM->m_VoteStartMinPlayers;

            if( Votes >= VotesNeeded )
            {
                if( m_UNM->m_StartAfterLeaveIfEnoughVotes )
                {
                    StartCountDown( true );
                    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] голосование набрало необходимое число голосов, запуск игры", 6, m_CurrentGameCounter );
                }
                else
                    SendAllChat( "Набрано необходимое число голосов для старта, однако кто-то ливнул! Требуется подтверждение старта (" + string( 1, m_UNM->m_CommandTrigger ) + "vs) от любого из игроков" );
            }
        }
    }

    // again pause game if the game was unpaused before player deleted

    if( Pause )
    {
        m_Pause = true;
        m_PauseEnd = false;
        m_Lagging = true;
        m_StartedLaggingTime = GetTime( );

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            ( *i )->SetLagging( true );
            ( *i )->SetStartedLaggingTicks( GetTicks( ) );
            ( *i )->Send( m_Protocol->SEND_W3GS_START_LAG( m_Players, false, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
            ( *i )->SetLagging( false );
            ( *i )->SetStartedLaggingTicks( 0 );
        }

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            ( *i )->SetLagging( true );
            ( *i )->SetStartedLaggingTicks( GetTicks( ) );
        }

        m_LastPauseResetTime = GetTime( );
        m_LastLagScreenResetTime = GetTime( );
        m_LagScreenTime = GetTime( );
    }
}

void CBaseGame::EventPlayerDisconnectTimedOut( CGamePlayer *player )
{
    if( player->GetGProxy( ) && m_GameLoaded )
    {
        if( !player->GetGProxyDisconnectNoticeSent( ) )
        {
            SendAllChat( player->GetName( ) + " " + gLanguage->HasLostConnectionTimedOutGProxy( ) + "." );
            player->SetGProxyDisconnectNoticeSent( true );
        }

        if( GetTime( ) - player->GetLastGProxyWaitNoticeSentTime( ) >= 20 )
        {
            uint32_t TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60 - ( GetTime( ) - m_StartedLaggingTime );

            if( TimeRemaining > ( (uint32_t)m_GProxyEmptyActions + 1 ) * 60 )
                TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60;

            SendAllChat( player->GetPID( ), gLanguage->WaitForReconnectSecondsRemain( UTIL_ToString( TimeRemaining ) ) );
            player->SetLastGProxyWaitNoticeSentTime( GetTime( ) );
        }

        return;
    }

    // not only do we not do any timeouts if the game is lagging, we allow for an additional grace period of 5 seconds
    // this is because Warcraft 3 stops sending packets during the lag screen
    // so when the lag screen finishes we would immediately disconnect everyone if we didn't give them some extra time

    if( GetTime( ) - m_LastLagScreenTime >= 5 )
    {
        player->SetDeleteMe( true );
        player->SetLeftReason( gLanguage->HasLostConnectionTimedOut( ) );
        player->SetLeftCode( PLAYERLEAVE_DISCONNECT );

        if( !m_GameLoading && !m_GameLoaded )
            OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
    }
}

void CBaseGame::EventPlayerDisconnectPlayerError( CGamePlayer *player )
{
    // at the time of this comment there's only one player error and that's when we receive a bad packet from the player
    // since TCP has checks and balances for data corruption the chances of this are pretty slim

    player->SetDeleteMe( true );
    player->SetLeftReason( gLanguage->HasLostConnectionPlayerError( player->GetErrorString( ) ) );
    player->SetLeftCode( PLAYERLEAVE_DISCONNECT );

    if( !m_GameLoading && !m_GameLoaded )
        OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
}

void CBaseGame::EventPlayerDisconnectSocketError( CGamePlayer *player )
{
    if( player->GetGProxy( ) && m_GameLoaded )
    {
        if( !player->GetGProxyDisconnectNoticeSent( ) )
        {
            SendAllChat( player->GetName( ) + " " + gLanguage->HasLostConnectionSocketErrorGProxy( player->GetSocket( )->GetErrorString( ) ) + "." );
            player->SetGProxyDisconnectNoticeSent( true );
        }

        if( GetTime( ) - player->GetLastGProxyWaitNoticeSentTime( ) >= 20 )
        {
            uint32_t TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60 - ( GetTime( ) - m_StartedLaggingTime );

            if( TimeRemaining > ( (uint32_t)m_GProxyEmptyActions + 1 ) * 60 )
                TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60;

            SendAllChat( player->GetPID( ), gLanguage->WaitForReconnectSecondsRemain( UTIL_ToString( TimeRemaining ) ) );
            player->SetLastGProxyWaitNoticeSentTime( GetTime( ) );
        }

        return;
    }

    player->SetDeleteMe( true );
    player->SetLeftReason( gLanguage->HasLostConnectionSocketError( player->GetSocket( )->GetErrorString( ) ) );
    player->SetLeftCode( PLAYERLEAVE_DISCONNECT );

    if( !m_GameLoading && !m_GameLoaded )
        OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
}

void CBaseGame::EventPlayerDisconnectConnectionClosed( CGamePlayer *player )
{
    if( player->GetGProxy( ) && m_GameLoaded )
    {
        if( !player->GetGProxyDisconnectNoticeSent( ) )
        {
            SendAllChat( player->GetName( ) + " " + gLanguage->HasLostConnectionClosedByRemoteHostGProxy( ) + "." );
            player->SetGProxyDisconnectNoticeSent( true );
        }

        if( GetTime( ) - player->GetLastGProxyWaitNoticeSentTime( ) >= 20 )
        {
            uint32_t TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60 - ( GetTime( ) - m_StartedLaggingTime );

            if( TimeRemaining > ( (uint32_t)m_GProxyEmptyActions + 1 ) * 60 )
                TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60;

            SendAllChat( player->GetPID( ), gLanguage->WaitForReconnectSecondsRemain( UTIL_ToString( TimeRemaining ) ) );
            player->SetLastGProxyWaitNoticeSentTime( GetTime( ) );
        }

        return;
    }

    player->SetDeleteMe( true );
    player->SetLeftReason( gLanguage->HasLostConnectionClosedByRemoteHost( ) );
    player->SetLeftCode( PLAYERLEAVE_DISCONNECT );

    if( !m_GameLoading && !m_GameLoaded )
        OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
}

void CBaseGame::EventPlayerJoined( CPotentialPlayer *potential, CIncomingJoinPlayer *joinPlayer )
{
    // identify their joined realm
    // this is only possible because when we send a game refresh via LAN or battle.net we encode an ID value in the 8 most significant bits of the host counter
    // the client sends the host counter when it joins so we can extract the ID value here
    // note: this is not a replacement for spoof checking since it doesn't verify the player's name and it can be spoofed anyway

    uint32_t HostCounterID = joinPlayer->GetHostCounter( ) >> 24;
    string JoinedRealm = "LAN";

    // we use an ID value of 0 to denote joining via LAN

    if( HostCounterID != 0 )
    {
        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
        {
            if( ( *i )->GetHostCounterID( ) == HostCounterID )
                JoinedRealm = ( *i )->GetServer( );
        }
    }

    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "|" + JoinedRealm + "] joining the game", 6, m_CurrentGameCounter );

    if( ( m_NormalCountdown && m_CountDownStarted ) || m_GameLoading || m_GameLoaded )
    {
        potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_FULL ) );
        potential->SetDeleteMe( true );
        return;
    }

    // check if the new player's name is the same as the virtual host name

    if( joinPlayer->GetName( ) == m_VirtualHostName )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is trying to join the game with the virtual host name", 6, m_CurrentGameCounter );
        potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_FULL ) );
        potential->SetDeleteMe( true );
        return;
    }

    // check if the new player's name is already taken

    if( GetPlayerFromName( joinPlayer->GetName( ), false ) )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is trying to join the game, but that name is already taken", 6, m_CurrentGameCounter );
        potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_FULL ) );
        potential->SetDeleteMe( true );
        return;
    }

    bool Owner = IsOwner( joinPlayer->GetName( ) );
    bool DefaultOwner = IsDefaultOwner( joinPlayer->GetName( ) );
    bool Reserved = IsReserved( joinPlayer->GetName( ), JoinedRealm ) || Owner || DefaultOwner;

    // check if the new player's name is empty or too long

    if( joinPlayer->GetName( ).empty( ) || joinPlayer->GetName( ).size( ) > 15 )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is trying to join the game with an invalid name of length " + UTIL_ToString( joinPlayer->GetName( ).size( ) ), 6, m_CurrentGameCounter );
        potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_FULL ) );
        potential->SetDeleteMe( true );
        return;
    }

    // check if the new player's name is invalid

    if( m_UNM->m_RejectColoredName && joinPlayer->GetName( ).find_first_of( " !\"|" ) != string::npos )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is trying to join the game with an invalid/colored name", 6, m_CurrentGameCounter );
        SendAllChat( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is trying to join the game with an invalid/colored name" );
        potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_FULL ) );
        potential->SetDeleteMe( true );
        return;
    }

    // check if the new player's name is blacklisted

    if( !Reserved && IsBlacklisted( m_BannedNames, joinPlayer->GetName( ) ) )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is trying to join the game but name was blacklisted", 6, m_CurrentGameCounter );
        potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_FULL ) );
        potential->SetDeleteMe( true );
        return;
    }

    potential->GetExternalIPString( );
    unsigned char SID = 255;
    unsigned char EnforcePID = 255;
    CGameSlot EnforceSlot( 255, 0, 0, 0, 0, 0, 0 );

    // try to find a slot

    unsigned char EnforceSID = 0;

    if( m_SaveGame )
    {
        // in a saved game we enforce the player layout and the slot layout
        // unfortunately we don't know how to extract the player layout from the saved game so we use the data from a replay instead
        // the !enforcesg command defines the player layout by parsing a replay

        for( vector<PIDPlayer> ::iterator i = m_EnforcePlayers.begin( ); i != m_EnforcePlayers.end( ); i++ )
        {
            if( ( *i ).second == joinPlayer->GetName( ) )
                EnforcePID = ( *i ).first;
        }

        for( vector<CGameSlot> ::iterator i = m_EnforceSlots.begin( ); i != m_EnforceSlots.end( ); i++ )
        {
            if( ( *i ).GetPID( ) == EnforcePID )
            {
                EnforceSlot = *i;
                break;
            }

            EnforceSID++;
        }

        if( EnforcePID == 255 || EnforceSlot.GetPID( ) == 255 || EnforceSID >= m_Slots.size( ) )
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is trying to join the game but isn't in the enforced list", 6, m_CurrentGameCounter );
            potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_FULL ) );
            potential->SetDeleteMe( true );
            return;
        }

        SID = EnforceSID;
    }
    else
    {
        // try to find an empty slot

        SID = GetEmptySlot( false );

        // check if the player is an admin or root admin on any connected realm for determining reserved status
        // we can't just use the spoof checked realm like in EventPlayerBotCommand because the player hasn't spoof checked yet

        if( SID == 255 && Reserved )
        {
            // a reserved player is trying to join the game but it's full, try to find a reserved slot

            if( Owner )
                SID = GetEmptySlotAdmin( true );
            else
                SID = GetEmptySlot( true );

            if( SID != 255 )
            {
                CGamePlayer *KickedPlayer = GetPlayerFromSID( SID );

                if( KickedPlayer )
                {
                    KickedPlayer->SetDeleteMe( true );
                    KickedPlayer->SetLeftReason( gLanguage->WasKickedForReservedPlayer( joinPlayer->GetName( ) ) );
                    KickedPlayer->SetLeftCode( PLAYERLEAVE_LOBBY );

                    // send a playerleave message immediately since it won't normally get sent until the player is deleted which is after we send a playerjoin message
                    // we don't need to call OpenSlot here because we're about to overwrite the slot data anyway

                    SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( KickedPlayer->GetPID( ), KickedPlayer->GetLeftCode( ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                    KickedPlayer->SetLeftMessageSent( true );
                }
            }
        }

        if( SID == 255 && DefaultOwner )
        {
            // the owner player is trying to join the game but it's full and we couldn't even find a reserved slot, kick the player in the lowest numbered slot
            // updated this to try to find a player slot so that we don't end up kicking a computer

            SID = 0;

            for( unsigned char i = 0; i < m_Slots.size( ); i++ )
            {
                if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetComputer( ) == 0 )
                {
                    SID = i;
                    break;
                }
            }

            CGamePlayer *KickedPlayer = GetPlayerFromSID( SID );

            if( KickedPlayer )
            {
                KickedPlayer->SetDeleteMe( true );
                KickedPlayer->SetLeftReason( gLanguage->WasKickedForOwnerPlayer( joinPlayer->GetName( ) ) );
                KickedPlayer->SetLeftCode( PLAYERLEAVE_LOBBY );

                // send a playerleave message immediately since it won't normally get sent until the player is deleted which is after we send a playerjoin message
                // we don't need to call OpenSlot here because we're about to overwrite the slot data anyway

                SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( KickedPlayer->GetPID( ), KickedPlayer->GetLeftCode( ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                KickedPlayer->SetLeftMessageSent( true );
            }
        }
    }

    if( SID >= m_Slots.size( ) )
    {
        if( Owner && GetTicks( ) - m_LastAdminJoinAndFullTicks > 5000 )
        {
            m_LastAdminJoinAndFullTicks = GetTicks( );
            SendAllChat( "Owner " + joinPlayer->GetName( ) + " is trying to join but the lobby is full..." );
        }

        potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_FULL ) );
        potential->SetDeleteMe( true );
        return;
    }

    if( Owner )
        m_OwnerJoined = true;

    // check if the owner joined and place him on the first slot.

    if( DefaultOwner && ( m_UNM->m_PlaceAdminsHigher == 1 || ( m_UNM->m_PlaceAdminsHigher == 2 && !m_DotA ) || ( m_UNM->m_PlaceAdminsHigher == 3 && m_DotA ) ) )
    {
        unsigned char SSID = 255;

        for( unsigned char i = 0; i < m_Slots.size( ); i++ )
        {
            if( m_Slots[i].GetComputer( ) == 0 )
            {
                SSID = i;
                break;
            }
        }

        if( SSID != SID && SSID != 255 )
        {
            SwapSlots( SID, SSID );
            SID = SSID;
        }
    }

    // auto delete fake player(s)

    if( m_FPEnable )
    {
        if( ( GetSlotsOpen( ) < 2 || GetNumHumanPlayers( ) > 3 ) && !m_FakePlayers.empty( ) )
            DeleteAFakePlayer( ); // we delete a fp when the lobby has < 2 open slots or > 3 human players
        else if( m_MoreFPs != 0 && GetSlotsOpen( ) > 3 && ( m_MoreFPs == 2 || ( m_MoreFPs == 1 && GetNumHumanPlayers( ) < 4 ) || ( GetNumHumanPlayers( ) < 4 && m_FakePlayers.size( ) < 3 ) ) )
            CreateFakePlayer( string( ) ); // we only allow a maximum of 3 fps to be added, no fp added once there're >=4 human ppls in lobby, the number of them declines gradually
    }

    // check to see if we have a specific slot reserved for this player

    unsigned char sn = ReservedSlot( joinPlayer->GetName( ) );

    if( sn != 255 && sn != SID && sn < m_Slots.size( ) )
    {
        if( m_Slots[sn].GetComputer( ) == 0 )
        {
            SwapSlots( SID, sn );
            SID = sn;
            ResetReservedSlot( joinPlayer->GetName( ) );
        }
        else
            ResetReservedSlot( joinPlayer->GetName( ) );
    }

    // we have a slot for the new player
    // make room for them by deleting the virtual host player if we have to

    uint32_t SlotReq = 11;

    if( m_ShowRealSlotCount )
        SlotReq = m_Slots.size( ) - 1;

    if( GetSlotsOccupied( ) >= SlotReq || EnforcePID == m_VirtualHostPID )
        DeleteVirtualHost( );

    // turning the CPotentialPlayer into a CGamePlayer is a bit of a pain because we have to be careful not to close the socket
    // this problem is solved by setting the socket to nullptr before deletion and handling the nullptr case in the destructor
    // we also have to be careful to not modify the m_Potentials vector since we're currently looping through it

    CGamePlayer *Player = nullptr;
    Player = new CGamePlayer( potential, m_SaveGame ? EnforcePID : GetNewPID( ), HostCounterID, JoinedRealm, joinPlayer->GetName( ), joinPlayer->GetInternalIP( ), Reserved );

    // consider LAN players to have already spoof checked since they can't
    // since so many people have trouble with this feature we now use the JoinedRealm to determine LAN status

    if( !m_UNM->IsBnetServer( JoinedRealm ) )
    {
        Player->SetSpoofed( true );
        Player->SetSpoofedRealm( JoinedRealm );
    }

    Player->SetSID( SID );
    Player->SetWhoisShouldBeSent( m_UNM->m_SpoofChecks == 1 || ( m_UNM->m_SpoofChecks == 2 && ( Owner || DefaultOwner ) ) );
    m_Players.push_back( Player );
    potential->SetSocket( nullptr );
    potential->SetDeleteMe( true );

    if( m_SaveGame )
        m_Slots[SID] = EnforceSlot;
    else
    {
        if( m_Map->GetMapGameType( ) == GAMETYPE_CUSTOM )
            m_Slots[SID] = CGameSlot( Player->GetPID( ), 255, SLOTSTATUS_OCCUPIED, 0, m_Slots[SID].GetTeam( ), m_Slots[SID].GetColour( ), m_Slots[SID].GetRace( ) );
        else
        {
            m_Slots[SID] = CGameSlot( Player->GetPID( ), 255, SLOTSTATUS_OCCUPIED, 0, 12, 12, 96 );

            // try to pick a team and colour
            // make sure there aren't too many other players already

            unsigned char NumOtherPlayers = 0;

            for( unsigned char i = 0; i < m_Slots.size( ); i++ )
            {
                if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetTeam( ) != 12 )
                    NumOtherPlayers++;
            }

            if( NumOtherPlayers < m_Map->GetMapNumPlayers( ) )
            {
                if( SID < m_Map->GetMapNumPlayers( ) )
                    m_Slots[SID].SetTeam( SID );
                else
                    m_Slots[SID].SetTeam( 0 );

                m_Slots[SID].SetColour( GetNewColour( ) );
            }
        }
    }

    // send slot info to the new player
    // the SLOTINFOJOIN packet also tells the client their assigned PID and that the join was successful

    Player->Send( m_Protocol->SEND_W3GS_SLOTINFOJOIN( Player->GetPID( ), Player->GetSocket( )->GetPort( ), Player->GetExternalIP( ), m_Slots, m_RandomSeed, m_Map->GetMapGameType( ) == GAMETYPE_CUSTOM ? 3 : 0, static_cast<unsigned char>( m_Map->GetMapNumPlayers( ) ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

    // send virtual host info and fake players info (if present) to the new player

    SendVirtualHostPlayerInfo( Player );
    SendFakePlayerInfo( Player );
    SendObservePlayerInfo( Player );
    BYTEARRAY BlankIP;
    BlankIP.push_back( 0 );
    BlankIP.push_back( 0 );
    BlankIP.push_back( 0 );
    BlankIP.push_back( 0 );
    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + Player->GetName( ) + "] joined the game", 6, m_CurrentGameCounter );

    if( m_UNM->m_PlayerServerPrintJoin )
    {
        string JoinPlayerName = Player->GetName( );

        if( !m_UNM->IsBnetServer( JoinedRealm ) )
            SendAllChat( "Игрок "  + JoinPlayerName + " забежал с •" + JoinedRealm + "•" );
        else
            SendAllChat( "Игрок " + JoinPlayerName + " забежал с сервера •" + JoinedRealm + "•" );
    }

    if( m_UNM->m_PlayerBeforeStartPrintJoin && m_AutoStartPlayers > 0 )
    {
        uint32_t PNr = GetNumHumanPlayers( );

        if( PNr < m_AutoStartPlayers )
        {
            string s = gLanguage->WaitingForPlayersBeforeAutoStartVS( UTIL_ToString( m_AutoStartPlayers ), UTIL_ToString( m_AutoStartPlayers - PNr ), string( 1, m_UNM->m_CommandTrigger ) );
            SendAllChat( s );
        }
    }

    // recalculate nr of players in each team + difference in players nr.

    ReCalculateTeams( );

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) && *i != Player )
        {
            // send info about the new player to every other player

            string s;

            if( ( *i )->GetSocket( ) )
            {
                if( m_UNM->m_PrefixName )
                {
                    s = Player->GetJoinedRealm( );
                    string st = s;
                    transform( st.begin( ), st.end( ), st.begin( ), static_cast<int(*)(int)>(tolower) );

                    if( st.empty( ) || st == "lan" )
                        s = "LAN";
                    else if( st.size( ) >= 6 && st.substr( 0, 6 ) == "garena" )
                        s = "Ga";
                    else if( st.find( "eurobattle" ) != string::npos )
                        s = "EB";
                    else if( st.find( "warcraft3.eu" ) != string::npos )
                        s = "w3eu";
                    else if( m_UNM->IsIccupServer( st ) )
                        s = "IC";
                    else if( st.find( "playground" ) != string::npos || st.find( "rubattle" ) != string::npos )
                        s = "PG";
                    else if( st.find( "it-ground" ) != string::npos )
                        s = "IT";
                    else
                        s = UTIL_SubStrFix( s, 0, 2 );

                    s = "[" + s + "]" + Player->GetName( );
                    s = UTIL_SubStrFix( s, 0, 15 );
                }
                else
                    s = Player->GetName( );

                if( m_UNM->m_HideIPAddresses )
                    ( *i )->Send( m_Protocol->SEND_W3GS_PLAYERINFO( Player->GetPID( ), s, BlankIP, BlankIP, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                else
                    ( *i )->Send( m_Protocol->SEND_W3GS_PLAYERINFO( Player->GetPID( ), s, Player->GetExternalIP( ), Player->GetInternalIP( ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
            }

            // send info about every other player to the new player

            if( m_UNM->m_PrefixName )
            {
                s = ( *i )->GetJoinedRealm( );
                string st = s;
                transform( st.begin( ), st.end( ), st.begin( ), static_cast<int(*)(int)>(tolower) );

                if( st.empty( ) || st == "lan" )
                    s = "LAN";
                else if( st.size( ) >= 6 && st.substr( 0, 6 ) == "garena" )
                    s = "Ga";
                else if( st.find( "eurobattle" ) != string::npos )
                    s = "EB";
                else if( st.find( "warcraft3.eu" ) != string::npos )
                    s = "w3eu";
                else if( m_UNM->IsIccupServer( st ) )
                    s = "IC";
                else if( st.find( "playground" ) != string::npos || st.find( "rubattle" ) != string::npos )
                    s = "PG";
                else if( st.find( "it-ground" ) != string::npos )
                    s = "IT";
                else
                    s = UTIL_SubStrFix( s, 0, 2 );

                s = "[" + s + "]" + ( *i )->GetName( );
                s = UTIL_SubStrFix( s, 0, 15 );
            }
            else
                s = ( *i )->GetName( );

            if( m_UNM->m_HideIPAddresses )
                Player->Send( m_Protocol->SEND_W3GS_PLAYERINFO( ( *i )->GetPID( ), s, BlankIP, BlankIP, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
            else
                Player->Send( m_Protocol->SEND_W3GS_PLAYERINFO( ( *i )->GetPID( ), s, ( *i )->GetExternalIP( ), ( *i )->GetInternalIP( ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
        }
    }

    // check for multiple ip usage

    vector<string>IPs;
    bool sayit = false;
    vector<string>Players;
    string IP1, IP2;
    bool pp1, pp2;
    bool IPfound;
    string::size_type pfound;
    vector<string> ::iterator p;

    if( Player->GetExternalIPString( ) != "127.0.0.1" )
    {
        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            IP1 = ( *i )->GetExternalIPString( );

            for( vector<CGamePlayer *> ::iterator j = m_Players.begin( ); j != m_Players.end( ); j++ )
            {
                // compare with all the other players except himself.
                if( ( *i ) != ( *j ) )
                {
                    IP2 = ( *j )->GetExternalIPString( );

                    // matching IP
                    if( IP1 == IP2 )
                    {
                        if( Player->GetName( ) == ( *i )->GetName( ) || Player->GetName( ) == ( *j )->GetName( ) )
                            sayit = true;

                        // already in the list of matching IPs?
                        pp1 = false;
                        pp2 = false;
                        IPfound = false;
                        p = Players.begin( );

                        if( IPs.size( ) > 0 )
                        {
                            for( vector<string> ::iterator s = IPs.begin( ); s != IPs.end( ); s++ )
                            {
                                if( ( *s ) == IP1 )
                                {
                                    // already in the list, check too see if the players are in it
                                    IPfound = true;
                                    pfound = ( *p ).find( ( *j )->GetName( ) );

                                    if( pfound != string::npos )
                                        pp2 = true;

                                    pfound = ( *p ).find( ( *i )->GetName( ) );

                                    if( pfound != string::npos )
                                        pp1 = true;

                                    if( !pp1 )
                                        ( *p ) += "=" + ( *i )->GetName( );

                                    if( !pp2 )
                                        ( *p ) += "=" + ( *j )->GetName( );
                                }

                                p++;
                            }
                        }

                        // the IP is not yet in the list, add it together with the two player names

                        if( !IPfound )
                        {
                            IPs.push_back( IP1 );
                            Players.push_back( ( *i )->GetName( ) + "=" + ( *j )->GetName( ) );
                        }
                    }
                }
            }
        }
    }

    // multiple IP usage was found, list it if last player who joined is part of them

    if( Player->GetExternalIPString( ) != "127.0.0.1" )
    {
        if( IPs.size( ) > 0 && sayit )
        {
            string IPString;
            p = Players.begin( );
            IPString = "Одинаковые IPs: ";

            for( vector<string> ::iterator s = IPs.begin( ); s != IPs.end( ); s++ )
            {
                IPString += ( *p );

                if( s != IPs.end( ) - 1 )
                    IPString += ", ";

                p++;
            }

            SendAllChat( IPString );
        }
    }

    // show country and pings when every slot has been occupied

    if( GetSlotsOpen( ) == 0 && !m_AllSlotsOccupied )
    {
        m_AllSlotsOccupied = true;
        m_AllSlotsAnnounced = false;
        m_SlotsOccupiedTime = GetTime( );
    }

    // update LastPlayerJoined

    m_LastPlayerJoined = Player->GetPID( );

    // send a map check packet to the new player

    Player->Send( m_Protocol->SEND_W3GS_MAPCHECK( m_Map->GetMapPath( ), m_Map->GetMapSize( ), m_Map->GetMapInfo( ), m_Map->GetMapCRC( ), m_Map->GetMapSHA1( ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

    // send slot info to everyone, so the new player gets this info twice but everyone else still needs to know the new slot layout

    SendAllSlotInfo( );

    // send a welcome message

    SendWelcomeMessage( Player, HostCounterID );

    // check for multiple IP usage

    if( m_UNM->m_CheckMultipleIPUsage )
    {
        string Others;
        uint32_t numOthers = 0;

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
        {
            if( Player != *i && Player->GetExternalIPString( ) == ( *i )->GetExternalIPString( ) )
            {
                if( Others.empty( ) )
                    Others = ( *i )->GetName( );
                else
                    Others += ", " + ( *i )->GetName( );

                numOthers++;
            }
        }

        // kick all of the players if they have more than eight connections
        // battle.net servers only allow eight connections for IP address (and nine is just too much!)

        if( m_UNM->m_DenyMaxIPUsage != 0 && numOthers >= m_UNM->m_DenyMaxIPUsage )
        {
            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
            {
                if( Player != *i && Player->GetExternalIPString( ) == ( *i )->GetExternalIPString( ) )
                {
                    ( *i )->SetDeleteMe( true );
                    ( *i )->SetLeftCode( PLAYERLEAVE_LOBBY );
                    OpenSlot( GetSIDFromPID( ( *i )->GetPID( ) ), false );
                }
            }

            Player->SetDeleteMe( true );
            Player->SetLeftCode( PLAYERLEAVE_LOBBY );
            OpenSlot( GetSIDFromPID( Player->GetPID( ) ), false );
            return;
        }
    }

    // abort the countdown if there was one in progress

    if( m_CountDownStarted && !m_GameLoading && !m_GameLoaded )
    {
        SendAllChat( gLanguage->CountDownAborted( ) );
        m_CountDownStarted = false;
    }

    emit gToLog->toGameChat( QString( ), QString( ), GetColoredNameFromPID( Player->GetPID( ) ), 9, m_CurrentGameCounter );
}


void CBaseGame::EventPlayerLeft( CGamePlayer *player, uint32_t reason )
{
    // this function is only called when a player leave packet is received, not when there's a socket error, kick, etc...

    m_LastLeaverTicks = GetTicks( );
    player->SetDeleteMe( true );

    if( reason == PLAYERLEAVE_GPROXY )
        player->SetLeftReason( gLanguage->WasUnrecoverablyDroppedFromGProxy( ) );
    else
        player->SetLeftReason( gLanguage->HasLeftVoluntarily( ) );

    player->SetLeftCode( PLAYERLEAVE_LOST );
    ReCalculateTeams( );

    if( !m_GameLoading && !m_GameLoaded )
        OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
}

void CBaseGame::EventPlayerLoaded( CGamePlayer *player )
{
    if( !m_GameLoading )
        return;

    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + player->GetName( ) + "] finished loading in " + UTIL_ToString( (float)( player->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000, 2 ) + " seconds", 6, m_CurrentGameCounter );
    player->SetTimeActive( GetTime( ) );

    if( m_LoadInGame )
    {
        // send any buffered data to the player now
        // see the Update function for more information about why we do this
        // this includes player loaded messages, game updates, and player leave messages

        queue<BYTEARRAY> *LoadInGameData = player->GetLoadInGameData( );

        while( !LoadInGameData->empty( ) )
        {
            Send( player, LoadInGameData->front( ) );
            LoadInGameData->pop( );
        }

        // start the lag screen for the new player

        bool FinishedLoading = true;

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            FinishedLoading = ( *i )->GetFinishedLoading( );

            if( !FinishedLoading )
                break;
        }

        if( !FinishedLoading )
            Send( player, m_Protocol->SEND_W3GS_START_LAG( m_Players, true, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

        // remove the new player from previously loaded players' lag screens

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( *i != player && ( *i )->GetFinishedLoading( ) )
                Send( *i, m_Protocol->SEND_W3GS_STOP_LAG( player ) );
        }

        while( !player->m_MessagesBeforeLoading.empty( ) )
        {
            CGamePlayer *fromPlayer = GetPlayerFromPID( player->m_MessagesAuthorsBeforeLoading[0] );

            if( fromPlayer )
            {
                BYTEARRAY PIDs;
                PIDs.push_back( player->GetPID( ) );
                Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( player->m_MessagesAuthorsBeforeLoading[0], PIDs, player->m_MessagesFlagsBeforeLoading[0], player->m_MessagesExtraFlagsBeforeLoading[0], player->m_MessagesBeforeLoading[0], "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
            }

            player->m_MessagesBeforeLoading.pop_front( );
            player->m_MessagesAuthorsBeforeLoading.pop_front( );
            player->m_MessagesFlagsBeforeLoading.pop_front( );
            player->m_MessagesExtraFlagsBeforeLoading.pop_front( );
        }

        // send a chat message to previously loaded players

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( *i != player && ( *i )->GetFinishedLoading( ) && m_UNM->m_PlayerFinishedLoadingPrinting )
                SendChat( *i, gLanguage->PlayerFinishedLoading( player->GetName( ) ) );
        }

        if( !FinishedLoading )
            SendChat( player, gLanguage->PleaseWaitPlayersStillLoading( ) );

        // if HCL was sent message first player not to set the map mode

        unsigned char Nr = 255;
        unsigned char Nrt;
        CGamePlayer *p = nullptr;

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            Nrt = GetSIDFromPID( ( *i )->GetPID( ) );

            if( Nrt < Nr )
            {
                Nr = Nrt;
                p = ( *i );
            }
        }

        if( m_HCL && p != nullptr && p == player && !m_HCLCommandString.empty( ) )
            SendChat( p->GetPID( ), "-" + m_HCLCommandString + " auto set, no need to type it in " + p->GetName( ) );
    }
    else
        SendAll( m_Protocol->SEND_W3GS_GAMELOADED_OTHERS( player->GetPID( ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
}

void CBaseGame::EventPlayerAction( CGamePlayer *player, CIncomingAction *action )
{
    if( !m_FPHCLUser.empty( ) && player && player->GetName( ) == m_FPHCLUser )
    {
        string fpdatastr = UTIL_ByteArrayToHexString( *action->GetAction( ) );

        if( fpdatastr.size( ) > 3 && fpdatastr.substr( 0, 3 ) == "60 " )
        {
            UTIL_Replace( fpdatastr, " 21 40 23 24 25 ", " " );
            BYTEARRAY fpdata, fpcrc;
            fpdata = UTIL_ExtractHexNumbers( fpdatastr );
            string fpname = string( );
            string checkstring = string( );
            string fpmode = string( );
            bool end = false;

            for( uint32_t i = 0; i < fpdata.size( ); i++ )
            {
                checkstring.push_back(fpdata[i]);

                if( !end && i > 8 )
                {
                    if( fpdata[i] == 0 )
                        end = true;
                    else
                        fpmode.push_back(fpdata[i]);
                }
            }

            transform( checkstring.begin( ), checkstring.end( ), checkstring.begin( ), static_cast<int(*)(int)>(tolower) );

            if( checkstring.find( "fphcl" ) == string::npos && checkstring.find( "hclfp" ) == string::npos && checkstring.find( "fpmode" ) == string::npos && checkstring.find( "fpmod" ) == string::npos && checkstring.find( "fakeplayerhcl" ) == string::npos && checkstring.find( "hclfakeplayer" ) == string::npos )
            {
                if( fpdata.size( ) > 10 )
                {
                    if( fpdata[1] == fpdata[5] && fpdata[2] == fpdata[6] )
                    {
                        unsigned char fpdatatwomin = 96;
                        unsigned char fpdatathreemin = 0;
                        unsigned char fpdatatwomax = 192;
                        unsigned char fpdatathreemax = 32;

                        if( fpdata[1] != 255 || fpdata[2] != 255 )
                        {
                            fpdatatwomin = fpdata[1];
                            fpdatathreemin = fpdata[2];
                            fpdatatwomax = fpdata[1];
                            fpdatathreemax = fpdata[2];
                            uint32_t xvar = ( ( player->GetSID( ) + 1 ) * 4 ) + 24;
                            uint32_t xvars = ( m_Slots.size( ) * 4 ) + 48;

                            if( fpdatatwomin < xvar )
                                fpdatatwomin = 0;
                            else
                                fpdatatwomin = fpdatatwomin - xvar;

                            if( xvars + fpdatatwomin >= 255 )
                                fpdatatwomax = 255;
                            else
                                fpdatatwomax = fpdatatwomin + xvars;

                            xvar = ( ( player->GetSID( ) + 1 ) * 2 ) + 12;
                            xvars = ( m_Slots.size( ) * 2 ) + 24;

                            if( fpdatathreemin < xvar )
                                fpdatathreemin = 0;
                            else
                                fpdatathreemin = fpdatathreemin - xvar;

                            if( xvars + fpdatathreemin >= 255 )
                                fpdatathreemax = 255;
                            else
                                fpdatathreemax = fpdatathreemin + xvars;
                        }

                        for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
                        {
                            if( m_FPHCLNumber == 12 || m_FPHCLNumber == i )
                            {
                                if( fpname.empty( ) )
                                    fpname = m_FakePlayersNames[i];

                                fpdata[1] = fpdatatwomin;
                                fpdata[2] = fpdatathreemin;
                                fpdata[3] = 0;
                                fpdata[4] = 0;
                                fpdata[5] = fpdatatwomin;
                                fpdata[6] = fpdatathreemin;
                                fpdata[7] = 0;
                                fpdata[8] = 0;

                                while( true )
                                {
                                    while( true )
                                    {
                                        m_Actions.push( new CIncomingAction( m_FakePlayers[i], fpcrc, fpdata ) );

                                        if( fpdata[1] == fpdatatwomax )
                                            break;
                                        else
                                        {
                                            fpdata[1]++;
                                            fpdata[5]++;
                                        }
                                    }

                                    if( fpdata[2] == fpdatathreemax )
                                        break;
                                    else
                                    {
                                        fpdata[2]++;
                                        fpdata[6]++;
                                        fpdata[1] = fpdatatwomin;
                                        fpdata[5] = fpdatatwomin;
                                    }
                                }
                            }
                        }

                        fpdata[1] = 255;
                        fpdata[2] = 255;
                        fpdata[3] = 255;
                        fpdata[4] = 255;
                        fpdata[5] = 255;
                        fpdata[6] = 255;
                        fpdata[7] = 255;
                        fpdata[8] = 255;

                        for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
                        {
                            if( m_FPHCLNumber == 12 || m_FPHCLNumber == i )
                            {
                                if( fpname.empty( ) )
                                    fpname = m_FakePlayersNames[i];

                                m_Actions.push( new CIncomingAction( m_FakePlayers[i], fpcrc, fpdata ) );
                            }
                        }
                    }
                    else
                    {
                        fpdata[1] -= player->GetSID( );
                        fpdata[5] -= player->GetSID( );
                        unsigned char fpdatatwo = fpdata[1];
                        unsigned char fpdatasix = fpdata[5];

                        for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
                        {
                            if( m_FPHCLNumber == 12 || m_FPHCLNumber == i )
                            {
                                if( fpname.empty( ) )
                                    fpname = m_FakePlayersNames[i];
                                else
                                {
                                    fpdata[1] = fpdatatwo;
                                    fpdata[5] = fpdatasix;
                                }

                                for( uint32_t n = 0; n < m_Slots.size( ); n++ )
                                {
                                    fpdata[1]++;
                                    fpdata[5]++;
                                    m_Actions.push( new CIncomingAction( m_FakePlayers[i], fpcrc, fpdata ) );
                                }
                            }
                        }
                    }
                }

                if( !fpmode.empty( ) )
                {
                    if( fpname.empty( ) )
                        SendChat( player->GetPID( ), "Не удалось отправить команду [" + fpmode + "] от имени фейк-плеера." );
                    else if( m_FPHCLNumber == 12 )
                        SendChat( player->GetPID( ), "Команда [" + fpmode + "] была отправлена от имени всех фейк-плееров." );
                    else
                        SendChat( player->GetPID( ), "Команда [" + fpmode + "] была отправлена от имени фейк-плеера [" + fpname + "]" );
                }
                else
                {
                    if( fpname.empty( ) )
                        SendChat( player->GetPID( ), "Не удалось отправить команду от имени фейк-плеера." );
                    else if( m_FPHCLNumber == 12 )
                        SendChat( player->GetPID( ), "Команда была отправлена от имени всех фейк-плееров." );
                    else
                        SendChat( player->GetPID( ), "Команда была отправлена от имени фейк-плеера [" + fpname + "]" );
                }

                m_FPHCLNumber = 0;
                m_FPHCLUser = string( );
                m_FPHCLTime = GetTime( );
            }
        }
    }

    if( !action->GetAction( )->empty( ) )
    {
        BYTEARRAY *ActionData = action->GetAction( );
        BYTEARRAY packet = *action->GetAction( );
        BYTEARRAY fpcrc;
        string playername;
        uint32_t PacketLength = ActionData->size( );

        if( m_FakePlayersCopyUsed && player )
        {
            for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
            {
                if( m_FakePlayersOwner[i] == player->GetName( ) )
                {
                    if( m_FakePlayersStatus[i] == 1 )
                    {
                        if( packet.size( ) > 1 && packet[0] == 22 )
                        {
                            m_FakePlayersActSelection[i] = packet;
                            m_FakePlayersStatus[i] = 2;
                            SendChat( player, "Юнит выбран! Для отмены юзай " + string( 1, m_UNM->m_CommandTrigger ) + "fpforget <" + UTIL_ToString( i + 1 ) + ">" );
                            m_Actions.push( new CIncomingAction( m_FakePlayers[i], fpcrc, packet ) );
                        }
                    }
                    else if( m_FakePlayersStatus[i] == 2 )
                    {
                        if( packet.size( ) > 1 && packet[0] == 22 )
                            m_Actions.push( new CIncomingAction( m_FakePlayers[i], fpcrc, m_FakePlayersActSelection[i] ) );
                        else
                            m_Actions.push( new CIncomingAction( m_FakePlayers[i], fpcrc, packet ) );
                    }
                }
            }
        }

        if( PacketLength > 0 )
        {
            bool PlayerActivity = false; // used for AFK detection
            uint32_t ActionSize = 0;
            uint32_t n = 0;
            uint32_t p = 0;
            unsigned int CurrentID = 255;
            unsigned int PreviousID = 255;
            bool Failed = false;
            bool Notified = false;

            while( n < PacketLength && !Failed )
            {
                PreviousID = CurrentID;
                CurrentID = ( *ActionData )[n];

                switch( CurrentID )
                {
                    case 0x00: Failed = true; break;
                    case 0x01: n += 1; break;
                    case 0x02: n += 1; break;
                    case 0x03: n += 2; break;
                    case 0x04: n += 1; break;
                    case 0x05: n += 1; break;
                    case 0x06:
                    Failed = true;
                    while( n < PacketLength )
                    {
                        if( ( *ActionData )[n] == 0 )
                        {
                            Failed = false;
                            break;
                        }
                        ++n;
                    }
                    ++n;

                    // notify everyone that a player is saving the game
                    if( !Notified )
                    {
                        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + player->GetName( ) + "] is saving the game", 6, m_CurrentGameCounter );
                        SendAllChat( gLanguage->PlayerIsSavingTheGame( player->GetName( ) ) );
                        Notified = true;
                    }
                    break;
                    case 0x07: n += 5; break;
                    case 0x08: Failed = true; break;
                    case 0x09: Failed = true; break;
                    case 0x10: n += 15; PlayerActivity = true; break;
                    case 0x11: n += 23; PlayerActivity = true; break;
                    case 0x12: n += 31; PlayerActivity = true; break;
                    case 0x13: n += 39; PlayerActivity = true; break;
                    case 0x14: n += 44; PlayerActivity = true; break;
                    case 0x15: Failed = true; break;
                    case 0x16:
                    case 0x17:
                    if( n + 4 > PacketLength )
                        Failed = true;
                    else
                    {
                        unsigned char i = ( *ActionData )[n + 2];
                        if( ( *ActionData )[n + 3] != 0x00 || i > 16 )
                            Failed = true;
                        else
                            n += ( 4 + ( i * 8 ) );
                    }
                    PlayerActivity = true;
                    break;
                    case 0x18: n += 3; break;
                    case 0x19: n += 13; break;
                    case 0x1A: n += 1; break;
                    case 0x1B: n += 10; PlayerActivity = true; break;
                    case 0x1C: n += 10; PlayerActivity = true; break;
                    case 0x1D: n += 9; break;
                    case 0x1E: n += 6; PlayerActivity = true; break;
                    case 0x1F: Failed = true; break;
                    case 0x20: Failed = true; break;
                    case 0x21: n += 9; break;
                    case 0x50: n += 6; break;
                    case 0x51:
                    n += 10;
                    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] resource trade detected by [" + player->GetName( ) + "]", 6, m_CurrentGameCounter );
                    playername = player->GetName( );
                    if( m_GetMapType.find( "guard" ) != string::npos || m_GetMapType.find( "notrade" ) != string::npos )
                    {
                        SendAllChat( "[" + playername + "] has used resource trade. Kicked as trading cheat is not allowed in non-trade map!" );
                        player->SetDeleteMe( true );
                        player->SetLeftReason( gLanguage->WasKickedByPlayer( "Anti-tradehack" ) );
                        player->SetLeftCode( PLAYERLEAVE_LOST );
                    }
                    else if( m_UNM->m_ResourceTradePrint )
                        SendAllChat( "[" + playername + "] has used resource trade." );
                    break;
                    case 0x52: Failed = true; break;
                    case 0x53: Failed = true; break;
                    case 0x54: Failed = true; break;
                    case 0x55: Failed = true; break;
                    case 0x56: Failed = true; break;
                    case 0x57: Failed = true; break;
                    case 0x58: Failed = true; break;
                    case 0x59: Failed = true; break;
                    case 0x5A: Failed = true; break;
                    case 0x5B: Failed = true; break;
                    case 0x5C: Failed = true; break;
                    case 0x5D: Failed = true; break;
                    case 0x5E: Failed = true; break;
                    case 0x5F: Failed = true; break;
                    case 0x60:
                    {
                        n += 9;
                        unsigned int j = 0;
                        Failed = true;
                        while( n < PacketLength && j < 128 )
                        {
                            if( ( *ActionData )[n] == 0 )
                            {
                                Failed = false;
                                break;
                            }
                            ++n;
                            ++j;
                        }
                        ++n;
                    }
                    break;
                    case 0x61: n += 1; PlayerActivity = true; break;
                    case 0x62: n += 13; break;
                    case 0x63: n += 9; break;
                    case 0x64: n += 9; break;
                    case 0x65: n += 9; break;
                    case 0x66: n += 1; PlayerActivity = true; break;
                    case 0x67: n += 1; PlayerActivity = true; break;
                    case 0x68: n += 13; break;
                    case 0x69: n += 17; break;
                    case 0x6A: n += 17; break;
                    case 0x6B: // used by W3MMD
                    {
                        ++n;
                        unsigned int j = 0;
                        while( n < PacketLength && j < 3 )
                        {
                            if( ( *ActionData )[n] == 0 )
                            {
                                ++j;
                            }
                            ++n;
                        }
                        n += 4;
                    }
                    break;
                    case 0x6C: Failed = true; break;
                    case 0x6D: Failed = true; break;
                    case 0x6E: Failed = true; break;
                    case 0x6F: Failed = true; break;
                    case 0x70: Failed = true; break;
                    case 0x71: Failed = true; break;
                    case 0x72: Failed = true; break;
                    case 0x73: Failed = true; break;
                    case 0x74: Failed = true; break;
                    case 0x75: n += 2; break;
                    default: Failed = true;
                }

                ActionSize = n - p;
                p = n;

                if( ActionSize > 1027 )
                    Failed = true;
            }

            if( Failed && m_UNM->m_WarningPacketValidationFailed )
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + player->GetName( ) + "] error: packet validation failed (" + UTIL_ToString( CurrentID ) + "," + UTIL_ToString( PreviousID ) + ")" + " (" + UTIL_ToString( n ) + "," + UTIL_ToString( PacketLength ) + ")", 6, m_CurrentGameCounter );

            if( PlayerActivity )
            {
                player->m_ActionsSum++;
                player->SetTimeActive( GetTime( ) );
            }
        }
    }

    m_Actions.push( action );
    BYTEARRAY *ActionData = action->GetAction( );

    if( ( m_GetMapType.find( "nopause" ) != string::npos || m_GetMapType.find( "guard" ) != string::npos ) && !ActionData->empty( ) && ( *ActionData )[0] == 1 )
    {
        BYTEARRAY CRC;
        BYTEARRAY Action;
        Action.push_back( 2 );

        if( ObsPlayer( ) )
            m_Actions.push( new CIncomingAction( m_ObsPlayerPID, CRC, Action ) );

        SendAllChat( "[Anti-Pause] " + player->GetName( ) + " tried to pause the game." );
    }

    if( m_GetMapType.find( "guard" ) != string::npos || m_GetMapType.find( "nosave" ) != string::npos )
    {
        CIncomingAction *Action = m_Actions.front( );

        if( m_Slots[GetSIDFromPID( Action->GetPID( ) )].GetTeam( ) != 12 )
        {
            CIncomingAction *Action = m_Actions.front( );

            if( ( *ActionData )[0] == 6 )
            {
                while( !m_Actions.empty( ) )
                {
                    Action = m_Actions.front( );
                    m_Actions.pop( );
                    SendAllChat( "[Anti-Save] " + GetPlayerFromPID( Action->GetPID( ) )->GetName( ) + " tried to save the game. Justice has served." );
                }
            }
        }
    }
}

void CBaseGame::EventPlayerKeepAlive( CGamePlayer *player )
{
    if( m_DropIfDesync )
    {
        // check for desyncs
        // however, it's possible that not every player has sent a checksum for this frame yet
        // first we verify that we have enough checksums to work with otherwise we won't know exactly who desynced

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( !( *i )->GetDeleteMe( ) && ( *i )->GetCheckSums( )->empty( ) )
                return;
        }

        // now we check for desyncs since we know that every player has at least one checksum waiting

        bool FoundPlayer = false;
        uint32_t FirstCheckSum = 0;

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( !( *i )->GetDeleteMe( ) )
            {
                FoundPlayer = true;
                FirstCheckSum = ( *i )->GetCheckSums( )->front( );
                break;
            }
        }

        if( !FoundPlayer )
            return;

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( !( *i )->GetDeleteMe( ) && ( *i )->GetCheckSums( )->front( ) != FirstCheckSum )
            {
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] desync detected", 6, m_CurrentGameCounter );
                SendAllChat( gLanguage->DesyncDetected( ) );

                // try to figure out who desynced
                // this is complicated by the fact that we don't know what the correct game state is so we let the players vote
                // put the players into bins based on their game state

                map<uint32_t, vector<unsigned char> > Bins;

                for( vector<CGamePlayer *> ::iterator j = m_Players.begin( ); j != m_Players.end( ); j++ )
                {
                    if( !( *j )->GetDeleteMe( ) )
                        Bins[( *j )->GetCheckSums( )->front( )].push_back( ( *j )->GetPID( ) );
                }

                uint32_t StateNumber = 1;
                map<uint32_t, vector<unsigned char> > ::iterator LargestBin = Bins.begin( );
                bool Tied = false;

                for( map<uint32_t, vector<unsigned char> > ::iterator j = Bins.begin( ); j != Bins.end( ); j++ )
                {
                    if( ( *j ).second.size( ) > ( *LargestBin ).second.size( ) )
                    {
                        LargestBin = j;
                        Tied = false;
                    }
                    else if( j != LargestBin && ( *j ).second.size( ) == ( *LargestBin ).second.size( ) )
                        Tied = true;

                    string Players;

                    for( vector<unsigned char> ::iterator k = ( *j ).second.begin( ); k != ( *j ).second.end( ); k++ )
                    {
                        CGamePlayer *Player = GetPlayerFromPID( *k );

                        if( Player )
                        {
                            if( Players.empty( ) )
                                Players = Player->GetName( );
                            else
                                Players += ", " + Player->GetName( );
                        }
                    }

                    SendAllChat( gLanguage->PlayersInGameState( UTIL_ToString( StateNumber ), Players ) );
                    StateNumber++;
                }

                FirstCheckSum = ( *LargestBin ).first;

                if( Tied )
                {
                    // there is a tie, which is unfortunate
                    // the most common way for this to happen is with a desync in a 1v1 situation
                    // this is not really unsolvable since the game shouldn't continue anyway so we just kick both players
                    // in a 2v2 or higher the chance of this happening is very slim
                    // however, we still kick every player because it's not fair to pick one or another group
                    // todotodo: it would be possible to split the game at this point and create a "new" game for each game state

                    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] can't kick desynced players because there is a tie, kicking all players instead", 6, m_CurrentGameCounter );
                    StopPlayers( gLanguage->WasDroppedDesync( ) );
                }
                else
                {
                    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] kicking desynced players", 6, m_CurrentGameCounter );

                    for( map<uint32_t, vector<unsigned char> > ::iterator j = Bins.begin( ); j != Bins.end( ); j++ )
                    {
                        // kick players who are NOT in the largest bin
                        // examples: suppose there are 10 players
                        // the most common case will be 9v1 (e.g. one player desynced and the others were unaffected) and this will kick the single outlier
                        // another (very unlikely) possibility is 8v1v1 or 8v2 and this will kick both of the outliers, regardless of whether their game states match

                        if( ( *j ).first != ( *LargestBin ).first )
                        {
                            for( vector<unsigned char> ::iterator k = ( *j ).second.begin( ); k != ( *j ).second.end( ); k++ )
                            {
                                CGamePlayer *Player = GetPlayerFromPID( *k );

                                if( Player )
                                {
                                    Player->SetDeleteMe( true );
                                    Player->SetLeftReason( gLanguage->WasDroppedDesync( ) );
                                    Player->SetLeftCode( PLAYERLEAVE_LOST );
                                }
                            }
                        }
                    }
                }

                // don't continue looking for desyncs, we already found one!

                break;
            }
        }

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( !( *i )->GetDeleteMe( ) )
                ( *i )->GetCheckSums( )->pop( );
        }

    }
    else
    {
        // check for desyncs
        // however, it's possible that not every player has sent a checksum for this frame yet
        // first we verify that we have enough checksums to work with otherwise we won't know exactly who desynced

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( ( *i )->GetCheckSums( )->empty( ) )
                return;
        }

        // now we check for desyncs since we know that every player has at least one checksum waiting

        uint32_t FirstCheckSum = player->GetCheckSums( )->front( );

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( !m_Desynced && ( *i )->GetCheckSums( )->front( ) != FirstCheckSum )
            {
                m_Desynced = true;
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] desync detected", 6, m_CurrentGameCounter );
                SendAllChat( gLanguage->DesyncDetected( ) );

                // try to figure out who desynced
                // this is complicated by the fact that we don't know what the correct game state is so we let the players vote
                // put the players into bins based on their game state

                map<uint32_t, vector<unsigned char> > Bins;

                for( vector<CGamePlayer *> ::iterator j = m_Players.begin( ); j != m_Players.end( ); j++ )
                    Bins[( *j )->GetCheckSums( )->front( )].push_back( ( *j )->GetPID( ) );

                uint32_t StateNumber = 1;
                map<uint32_t, vector<unsigned char> > ::iterator LargestBin = Bins.begin( );
                bool Tied = false;

                for( map<uint32_t, vector<unsigned char> > ::iterator j = Bins.begin( ); j != Bins.end( ); j++ )
                {
                    if( ( *j ).second.size( ) > ( *LargestBin ).second.size( ) )
                    {
                        LargestBin = j;
                        Tied = false;
                    }
                    else if( j != LargestBin && ( *j ).second.size( ) == ( *LargestBin ).second.size( ) )
                        Tied = true;

                    string Players;

                    for( vector<unsigned char> ::iterator k = ( *j ).second.begin( ); k != ( *j ).second.end( ); k++ )
                    {
                        CGamePlayer *Player = GetPlayerFromPID( *k );

                        if( Player )
                        {
                            if( Players.empty( ) )
                                Players = Player->GetName( );
                            else
                                Players += ", " + Player->GetName( );
                        }
                    }

                    SendAllChat( gLanguage->PlayersInGameState( UTIL_ToString( StateNumber ), Players ) );
                    StateNumber++;
                }
            }
        }

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            ( *i )->GetCheckSums( )->pop( );
    }
}

void CBaseGame::EventPlayerChatToHost( CGamePlayer *player, CIncomingChatPlayer *chatPlayer )
{
    if( chatPlayer->GetFromPID( ) == player->GetPID( ) )
    {
        if( chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_MESSAGE || chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_MESSAGEEXTRA )
        {
            // relay the chat message to other players

            bool Relay = true;

            if( IsMuted( player->GetName( ) ) || player->GetMuted( ) || IsCensorMuted( player->GetName( ) ) > 0 )
                Relay = false;

            BYTEARRAY ExtraFlags = chatPlayer->GetExtraFlags( );
            player->SetTimeActive( GetTime( ) );

            // calculate timestamp

            string MinString = UTIL_ToString( ( m_GameTicks / 1000 ) / 60 );
            string SecString = UTIL_ToString( ( m_GameTicks / 1000 ) % 60 );

            if( MinString.size( ) == 1 )
                MinString.insert( 0, "0" );

            if( SecString.size( ) == 1 )
                SecString.insert( 0, "0" );

            string playername = player->GetName( );
            string pushbackmsg = string( );
            bool isOwner = IsOwner( player->GetName( ) );
            bool isDefaultOwner = IsDefaultOwner( player->GetName( ) );
            string msg = chatPlayer->GetMessage( );
            string msglower = msg;
            msglower = UTIL_StringToLower( msglower );
            bool iscmd = false;
            string messagetype;

            if( !msg.empty( ) && ( msg[0] == m_UNM->m_CommandTrigger || msg[0] == m_UNM->m_CommandTrigger1 || msg[0] == m_UNM->m_CommandTrigger2 || msg[0] == m_UNM->m_CommandTrigger3 || msg[0] == m_UNM->m_CommandTrigger4 || msg[0] == m_UNM->m_CommandTrigger5 || msg[0] == m_UNM->m_CommandTrigger6 || msg[0] == m_UNM->m_CommandTrigger7 || msg[0] == m_UNM->m_CommandTrigger8 || msg[0] == m_UNM->m_CommandTrigger9 ) )
                iscmd = true;

            if( !ExtraFlags.empty( ) )
            {
                unsigned char fteam = m_Slots[GetSIDFromPID( chatPlayer->GetFromPID( ) )].GetTeam( );

                if( m_DotA && msglower.size( ) > 1 )
                {
                    bool blueplayer = false;

                    if( msglower.front( ) == '-' )
                    {
                        unsigned char Nr = 255;
                        unsigned char Nrt;
                        string sSlotNr;
                        string sName = " ";
                        CGamePlayer *p = nullptr;

                        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                        {
                            Nrt = GetSIDFromPID( ( *i )->GetPID( ) );

                            if( Nrt < Nr )
                            {
                                Nr = Nrt;
                                sName = ( *i )->GetName( );
                                p = ( *i );
                            }
                        }

                        sSlotNr = UTIL_ToString( Nr );

                        if( p && sName == playername )
                            blueplayer = true;
                    }

                    if( msglower.size( ) == 9 && msglower.substr( 0, 7 ) == "-switch" )
                        InitSwitching( chatPlayer->GetFromPID( ), msglower.substr( 8, 1 ) );

                    if( m_SwitchTime != 0 )
                    {
                        if( msglower == "-no" || msglower == "-switch cancel" )
                            SwitchDeny( chatPlayer->GetFromPID( ) );
                        else if( msglower == "-ok" || msglower == "-switch accept" )
                            SwitchAccept( chatPlayer->GetFromPID( ) );
                    }
                }

                string CheckValid = " ";

                if( !m_GameLoading && !m_GameLoaded )
                {
                    Relay = false;
                    CheckValid = " [This message was not sent! (wrong format)] ";
                }

                if( ExtraFlags[0] == 0 )
                {
                    // this is an ingame [All] message, print it to the console

                    messagetype = "All";
                    pushbackmsg = "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] (" + MinString + ":" + SecString + ") " + "[Team: " + UTIL_ToString( fteam + 1 ) + "] [All]" + CheckValid + "[" + playername + "]: " + msg;
                    CONSOLE_Print( pushbackmsg );
                    emit gToLog->toGameChat( QString::fromUtf8( msg.c_str( ) ), QString( ), GetColoredNameFromPID( player->GetPID( ) ), 4, m_CurrentGameCounter );

                    // don't relay ingame messages targeted for all players if we're currently muting all
                    // note that commands will still be processed even when muting all because we only stop relaying the messages, the rest of the function is unaffected

                    if( m_MuteAll && !isOwner && !isDefaultOwner )
                        Relay = false;
                }
                else if( ExtraFlags[0] == 1 )
                {
                    // this is an ingame [ally] message, print it to the console

                    messagetype = "Ally";
                    pushbackmsg = "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] (" + MinString + ":" + SecString + ") " + "[Team: " + UTIL_ToString( fteam + 1 ) + "] [Allies]" + CheckValid + "[" + playername + "]: " + msg;
                    CONSOLE_Print( pushbackmsg );
                    emit gToLog->toGameChat( QString::fromUtf8( msg.c_str( ) ), "(Team " + UTIL_ToQString( fteam + 1 ) + ")", GetColoredNameFromPID( player->GetPID( ) ), 5, m_CurrentGameCounter );
                }
                else if( ExtraFlags[0] == 2 )
                {
                    // this is an ingame [Obs/Ref] message, print it to the console

                    if( m_MapObservers == MAPOBS_REFEREES )
                        messagetype = "Referees";
                    else
                        messagetype = "Observers";

                    pushbackmsg = "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] (" + MinString + ":" + SecString + ") " + "[Team: " + UTIL_ToString( fteam + 1 ) + "] [" + messagetype + "]" + CheckValid + "[" + playername + "]: " + msg;
                    CONSOLE_Print( pushbackmsg );
                    emit gToLog->toGameChat( QString::fromUtf8( msg.c_str( ) ), QString::fromUtf8( messagetype.c_str( ) ), QString::fromUtf8( player->GetName( ).c_str( ) ), 6, m_CurrentGameCounter );
                }
                else
                {
                    // this is an ingame [private] message, print it to the console

                    string toplayername;
                    CGamePlayer *toPlayer = GetPlayerFromSID( ExtraFlags[0] - 3 );

                    if( toPlayer )
                    {
                        toplayername = toPlayer->GetName( );
                        emit gToLog->toGameChat( QString::fromUtf8( msg.c_str( ) ), GetColoredNameFromPID( toPlayer->GetPID( ) ), GetColoredNameFromPID( player->GetPID( ) ), 7, m_CurrentGameCounter );
                    }
                    else
                    {
                        toplayername = UTIL_ToString( ExtraFlags[0] - 2 ) + " slot";

                        if( ExtraFlags[0] - 2 > 0 && m_Slots.size( ) <= 255 && ExtraFlags[0] - 3 < m_Slots.size( ) )
                        {
                            string ColoredName = string( );

                            switch( m_Slots[ExtraFlags[0] - 3].GetColour( ) )
                            {
                                case 0:
                                ColoredName = "<font color = #ff0000>" + toplayername + "</font>";
                                break;
                                case 1:
                                ColoredName = "<font color = #0000ff>" + toplayername + "</font>";
                                break;
                                case 2:
                                ColoredName = "<font color = #00ffff>" + toplayername + "</font>";
                                break;
                                case 3:
                                ColoredName = "<font color = #800080>" + toplayername + "</font>";
                                break;
                                case 4:
                                ColoredName = "<font color = #ffff00>" + toplayername + "</font>";
                                break;
                                case 5:
                                ColoredName = "<font color = #ffa500>" + toplayername + "</font>";
                                break;
                                case 6:
                                ColoredName = "<font color = #00ff00>" + toplayername + "</font>";
                                break;
                                case 7:
                                ColoredName = "<font color = #ff00ff>" + toplayername + "</font>";
                                break;
                                case 8:
                                ColoredName = "<font color = #858585>" + toplayername + "</font>";
                                break;
                                case 9:
                                ColoredName = "<font color = #add8e6>" + toplayername + "</font>";
                                break;
                                case 10:
                                ColoredName = "<font color = #008000>" + toplayername + "</font>";
                                break;
                                case 11:
                                ColoredName = "<font color = #964B00>" + toplayername + "</font>";
                                break;
                                default:
                                ColoredName = "<font color = #000000>" + toplayername + "</font>";
                            }

                            emit gToLog->toGameChat( QString::fromUtf8( msg.c_str( ) ), QString::fromUtf8( ColoredName.c_str( ) ), GetColoredNameFromPID( player->GetPID( ) ), 7, m_CurrentGameCounter );
                        }
                        else
                            emit gToLog->toGameChat( QString::fromUtf8( msg.c_str( ) ), QString::fromUtf8( toplayername.c_str( ) ), GetColoredNameFromPID( player->GetPID( ) ), 8, m_CurrentGameCounter );
                    }

                    messagetype = "Private (to " + toplayername + ")";
                    pushbackmsg = "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] (" + MinString + ":" + SecString + ") " + "[Team: " + UTIL_ToString( fteam + 1 ) + "] [Private (to " + toplayername + ")]" + CheckValid + "[" + playername + "]: " + msg;
                    CONSOLE_Print( pushbackmsg );
                }

                if( !m_GameLoading && !m_GameLoaded )
                    messagetype += " - message was not sent (wrong format)";

                // add chat message to replay
                // this includes allied chat and private chat from both teams as long as it was relayed

                if( m_Replay && Relay && m_GameLoaded )
                    m_Replay->AddChatMessage( chatPlayer->GetFromPID( ), chatPlayer->GetFlag( ), UTIL_ByteArrayToUInt32( ExtraFlags, false ), msg );
            }
            else
            {
                if( m_GameLoading || m_GameLoaded )
                {
                    messagetype = "Unknown - message was not sent (wrong format)";
                    pushbackmsg = "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] [Lobby] [This message was not sent! (wrong format)] [" + playername + "]: " + msg;
                    CONSOLE_Print( pushbackmsg );
                    Relay = false;
                }
                else
                {
                    // this is a lobby message, print it to the console

                    messagetype = "Lobby";
                    pushbackmsg = "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] [Lobby] [" + playername + "]: " + msg;
                    CONSOLE_Print( pushbackmsg );
                }
            }

            // handle bot commands

            if( msglower == "?trigger" )
                SendChat( player, gLanguage->CommandTrigger( string( 1, m_UNM->m_CommandTrigger ) ) );
            else if( iscmd )
            {
                // extract the command trigger, the command, and the payload
                // e.g. "!say hello world" -> command: "say", payload: "hello world"

                string Command;
                string Payload;
                string::size_type PayloadStart = msglower.find( " " );

                if( PayloadStart != string::npos )
                {
                    Command = msglower.substr( 1, PayloadStart - 1 );
                    Payload = msg.substr( PayloadStart + 1 );
                }
                else
                    Command = msglower.substr( 1 );

                // don't allow EventPlayerBotCommand to veto a previous instruction to set Relay to false
                // so if Relay is already false (e.g. because the player is muted) then it cannot be forced back to true here

                if( EventPlayerBotCommand( player, Command, Payload ) )
                    Relay = false;
            }
            else if( !msg.empty( ) )
            {
                string str = m_UNM->m_InvalidTriggersGame;

                if( !str.empty( ) && msg.size( ) > 1 )
                {
                    for( uint32_t i = 0; i < str.length( ); i++ )
                    {
                        if( msg[0] == str[i] )
                        {
                            SendChat( player, "Wrong command trigger. Command starts with " + string( 1, m_UNM->m_CommandTrigger ) + " example: " + string( 1, m_UNM->m_CommandTrigger ) + "owner, " + string( 1, m_UNM->m_CommandTrigger ) + "start" );
                            break;
                        }
                    }
                }
            }

            BYTEARRAY PIDs;

            if( !iscmd || m_UNM->m_RelayChatCommands )
                PIDs = Silence( chatPlayer->GetToPIDs( ) );
            else
                PIDs = Silence( GetRootAdminPIDs( player->GetPID( ) ) );

            if( m_Listen && !ExtraFlags.empty( ) )
            {
                for( uint32_t i = 0; i != m_ListenUser.size( ); i++ )
                {
                    CGamePlayer *toPlayer = GetPlayerFromPID( m_ListenUser[i] );

                    if( toPlayer && m_ListenUser[i] != chatPlayer->GetFromPID( ) )
                    {
                        bool relaytoplayer = false;

                        if( Relay )
                        {
                            for( uint32_t p = 0; p != PIDs.size( ); p++ )
                            {
                                if( m_ListenUser[i] == PIDs[p] )
                                {
                                    relaytoplayer = true;
                                    break;
                                }
                            }
                        }

                        if( relaytoplayer && toPlayer->GetIsIgnoring( player->GetName( ) ) )
                            relaytoplayer = false;

                        if( !relaytoplayer )
                        {
                            BYTEARRAY ExtraFlagsTest = ExtraFlags;
                            BYTEARRAY ListenUserArray;
                            unsigned char fromPIDTest = chatPlayer->GetFromPID( );
                            ListenUserArray.push_back( m_ListenUser[i] );
                            string msgTest = msg;

                            if( ExtraFlagsTest[0] > 2 )
                                ExtraFlagsTest[0] = toPlayer->GetSID( ) + 3;

                            else if( m_MapObservers == MAPOBS_ALLOWED && GetTeam( player->GetSID( ) ) == 12 && GetTeam( toPlayer->GetSID( ) ) != 12 && ExtraFlagsTest[0] == 2 )
                            {
                                fromPIDTest = toPlayer->GetPID( );
                                msgTest = player->GetName( ) + ": " + msgTest;
                            }

                            msgTest = "(ПPOCЛУШKA) " + msgTest;

                            if( !m_GameLoading || toPlayer->GetFinishedLoading( ) ) // Send now
                                Send( m_ListenUser[i], m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPIDTest, ListenUserArray, chatPlayer->GetFlag( ), ExtraFlagsTest, msgTest, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                            else   // Send later
                            {
                                toPlayer->m_MessagesBeforeLoading.push_back( msgTest );
                                toPlayer->m_MessagesAuthorsBeforeLoading.push_back( fromPIDTest );
                                toPlayer->m_MessagesFlagsBeforeLoading.push_back( chatPlayer->GetFlag( ) );
                                toPlayer->m_MessagesExtraFlagsBeforeLoading.push_back( ExtraFlagsTest );
                            }
                        }
                    }
                }
            }

            if( Relay )
            {
                if( ( player->m_MsgAutoCorrect && ( m_UNM->m_MsgAutoCorrectMode == 2 || m_UNM->m_MsgAutoCorrectMode == 3 ) ) || m_UNM->m_MsgAutoCorrectMode == 1 )
                    msg = UTIL_MsgErrorCorrection( msg );

                string msg2;

                if( m_UNM->m_Censor )
                    msg2 = m_UNM->CensorMessage( msg );
                else
                    msg2 = msg;

                if( m_IntergameChat && !iscmd )
                {
                    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                    {
                        if( !( *i )->m_SpamSubBotChat )
                            ( *i )->QueueChatCommand( "[" + player->GetName( ) + "]: " + msg2 );
                    }
                }

                if( m_UNM->m_CountDownMethod == 1 && !m_GameLoading && !m_GameLoaded )
                {
                    player->m_BufferMessages.push_back( m_Protocol->SEND_W3GS_CHAT_FROM_HOST_BUFFER( chatPlayer->GetFromPID( ), PIDs, chatPlayer->GetFlag( ), ExtraFlags, msg2, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                    if( player->m_BufferMessages.size( ) > 7 )
                        player->m_BufferMessages.pop_front( );
                }

                for( uint32_t i = 0; i < PIDs.size( ); ++i )
                {
                    CGamePlayer *toPlayer = GetPlayerFromPID( PIDs[i] );

                    if( toPlayer && !toPlayer->GetIsIgnoring( player->GetName( ) ) )
                    {
                        if( !m_GameLoading || toPlayer->GetFinishedLoading( ) ) // Send now
                        {
                            Send( toPlayer, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( chatPlayer->GetFromPID( ), PIDs, chatPlayer->GetFlag( ), ExtraFlags, msg2, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                            if( m_UNM->m_CountDownMethod == 1 && !m_GameLoading && !m_GameLoaded && !toPlayer->GetLeftMessageSent( ) && toPlayer->GetPID( ) != player->GetPID( ) )
                            {
                                toPlayer->m_BufferMessages.push_back( m_Protocol->SEND_W3GS_CHAT_FROM_HOST_BUFFER( chatPlayer->GetFromPID( ), PIDs, chatPlayer->GetFlag( ), ExtraFlags, msg2, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                                if( toPlayer->m_BufferMessages.size( ) > 7 )
                                    toPlayer->m_BufferMessages.pop_front( );
                            }
                        }
                        else   // Send later
                        {
                            toPlayer->m_MessagesBeforeLoading.push_back( msg2 );
                            toPlayer->m_MessagesAuthorsBeforeLoading.push_back( chatPlayer->GetFromPID( ) );
                            toPlayer->m_MessagesFlagsBeforeLoading.push_back( chatPlayer->GetFlag( ) );
                            toPlayer->m_MessagesExtraFlagsBeforeLoading.push_back( ExtraFlags );
                        }
                    }
                }

                if( m_autoinsultlobby && !m_GameLoaded && !m_GameLoading && msg2 != msg )
                {
                    bool ok = true;

                    if( m_UNM->m_autoinsultlobbysafeadmins && ( isOwner || isDefaultOwner ) )
                        ok = false;

                    if( ok && GetTime( ) - m_LastMars >= 10 )
                    {
                        string nam1 = string( );
                        string nam2 = player->GetName( );
                        vector<string> randoms;
                        m_LastMars = GetTime( );
                        string msgmars = m_UNM->GetMars( );

                        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                        {
                            if( ( *i )->GetName( ) != nam2 && ( *i )->GetName( ) != nam1 )
                                randoms.push_back( ( *i )->GetName( ) );
                        }

                        if( randoms.empty( ) )
                        {
                            if( nam1 != m_VirtualHostName && nam2 != m_VirtualHostName )
                                randoms.push_back( m_VirtualHostName );
                            else
                                randoms.push_back( "unm" );
                        }

                        std::random_device rand_dev;
                        std::mt19937 rng( rand_dev( ) );
                        shuffle( randoms.begin( ), randoms.end( ), rng );
                        nam1 = randoms[0];
                        UTIL_Replace( msgmars, "$VICTIM$", nam2 );
                        UTIL_Replace( msgmars, "$USER$", nam1 );
                        UTIL_Replace( msgmars, "$RANDOM$", randoms[0] );
                        SendAllChat( msgmars );
                    }
                }

                // Censor

                string msg3;

                if( !m_UNM->m_CensorMute )
                    msg3 = msg;
                else if( m_UNM->m_Censor )
                    msg3 = msg2;
                else
                    msg2 = m_UNM->CensorMessage( msg );

                if( msg3 != msg )
                {
                    if( !isOwner || m_UNM->m_CensorMuteAdmins )
                        AddToCensorMuted( player->GetName( ) );
                }
            }
        }
        else if( chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_TEAMCHANGE && !m_CountDownStarted )
            EventPlayerChangeTeam( player, chatPlayer->GetByte( ) );
        else if( chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_COLOURCHANGE && !m_CountDownStarted )
            EventPlayerChangeColour( player, chatPlayer->GetByte( ) );
        else if( chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_RACECHANGE && !m_CountDownStarted )
            EventPlayerChangeRace( player, chatPlayer->GetByte( ) );
        else if( chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_HANDICAPCHANGE && !m_CountDownStarted )
            EventPlayerChangeHandicap( player, chatPlayer->GetByte( ) );
    }
}

bool CBaseGame::EventPlayerBotCommand( CGamePlayer *player, string command, string payload )
{
    // return true if the command itself should be hidden from other players

    return false;
}

void CBaseGame::EventPlayerChangeTeam( CGamePlayer *player, unsigned char team )
{
    // player is requesting a team change

    if( m_SaveGame )
        return;

    if( m_Map->GetMapGameType( ) == GAMETYPE_CUSTOM )
    {
        unsigned char oldSID = GetSIDFromPID( player->GetPID( ) );
        unsigned char newSID = GetEmptySlot( team, player->GetPID( ) );
        SwapSlots( oldSID, newSID );
    }
    else
    {
        if( team > 12 )
            return;

        if( team == 12 )
        {
            if( m_MapObservers != MAPOBS_ALLOWED && m_MapObservers != MAPOBS_REFEREES )
                return;
        }
        else
        {
            if( team >= m_Map->GetMapNumPlayers( ) )
                return;

            // make sure there aren't too many other players already

            unsigned char NumOtherPlayers = 0;

            for( unsigned char i = 0; i < m_Slots.size( ); i++ )
            {
                if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetTeam( ) != 12 && m_Slots[i].GetPID( ) != player->GetPID( ) )
                    NumOtherPlayers++;
            }

            if( NumOtherPlayers >= m_Map->GetMapNumPlayers( ) )
                return;
        }

        unsigned char SID = GetSIDFromPID( player->GetPID( ) );

        if( SID < m_Slots.size( ) )
        {
            m_Slots[SID].SetTeam( team );

            if( team == 12 )
            {
                // if they're joining the observer team give them the observer colour

                m_Slots[SID].SetColour( 12 );
            }
            else if( m_Slots[SID].GetColour( ) == 12 )
            {
                // if they're joining a regular team give them an unused colour

                m_Slots[SID].SetColour( GetNewColour( ) );
            }

            SendAllSlotInfo( );
        }
    }
}

void CBaseGame::EventPlayerChangeColour( CGamePlayer *player, unsigned char colour )
{
    // player is requesting a colour change

    if( m_SaveGame )
        return;

    if( colour > 11 )
        return;

    unsigned char SID = GetSIDFromPID( player->GetPID( ) );

    if( SID < m_Slots.size( ) )
    {
        // make sure the player isn't an observer

        if( m_Slots[SID].GetTeam( ) == 12 )
            return;

        ColourSlot( SID, colour );
    }
}

void CBaseGame::EventPlayerChangeRace( CGamePlayer *player, unsigned char race )
{
    // player is requesting a race change

    if( m_SaveGame )
        return;

    if( m_Map->GetMapFlags( ) & MAPFLAG_RANDOMRACES )
        return;

    if( race != SLOTRACE_HUMAN && race != SLOTRACE_ORC && race != SLOTRACE_NIGHTELF && race != SLOTRACE_UNDEAD && race != SLOTRACE_RANDOM )
        return;

    unsigned char SID = GetSIDFromPID( player->GetPID( ) );

    if( SID < m_Slots.size( ) )
    {
        m_Slots[SID].SetRace( race | SLOTRACE_FIXED );
        SendAllSlotInfo( );
    }
}

void CBaseGame::EventPlayerChangeHandicap( CGamePlayer *player, unsigned char handicap )
{
    // player is requesting a handicap change

    if( m_SaveGame )
        return;

    if( handicap != 50 && handicap != 60 && handicap != 70 && handicap != 80 && handicap != 90 && handicap != 100 )
        return;

    if( m_Map->GetMapType( ).find( "nochangehandicap" ) != string::npos )
        return;

    unsigned char SID = GetSIDFromPID( player->GetPID( ) );

    if( SID < m_Slots.size( ) )
    {
        m_Slots[SID].SetHandicap( handicap );
        SendAllSlotInfo( );
    }
}

void CBaseGame::EventPlayerDropRequest( CGamePlayer *player )
{
    uint32_t TimePassed = GetTime( ) - m_LagScreenTime;
    uint32_t TimeRemaining = m_UNM->m_DropVoteTime - TimePassed;
    double DropVotePercent = 0.01 * m_UNM->m_DropVotePercent;

    if( m_Lagging )
    {
        if( TimePassed < m_UNM->m_DropVoteTime )
            SendChat( player, "Пожалуйста, подождите еще " + UTIL_ToString( TimeRemaining ) + " сек. прежде чем голосовать за дроп" );
        else
        {
			player->SetDropVote( true );
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] player [" + player->GetName( ) + "] voted to drop laggers", 6, m_CurrentGameCounter );
            SendAllChat( gLanguage->PlayerVotedToDropLaggers( player->GetName( ) ) );

            // check if at least half the players voted to drop

            uint32_t Votes = 0;

            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            {
                if( ( *i )->GetDropVote( ) )
                    Votes++;
            }

            if( (float)Votes / m_Players.size( ) >= DropVotePercent )
            {
                if( m_UNM->m_UserCanDropAdmin )
                    StopLaggers( gLanguage->LaggedOutDroppedByVote( ) );
                else
                    StopLagger( gLanguage->LaggedOutDroppedByVote( ) );
            }
        }
    }
}

void CBaseGame::EventPlayerMapSize( CGamePlayer *player, CIncomingMapSize *mapSize )
{
    if( m_GameLoading || m_GameLoaded )
        return;

    // todotodo: the variable names here are confusing due to extremely poor design on my part

    uint32_t MapSize = UTIL_ByteArrayToUInt32( m_Map->GetMapSize( ), false );

    if( mapSize->GetSizeFlag( ) != 1 || mapSize->GetMapSize( ) != MapSize )
    {
        // the player doesn't have the map

        bool candownload = false;

        if( IsOwner( player->GetName( ) ) )
            candownload = true;

        if( ( m_AllowDownloads != 0 && m_AllowDownloads != 3 ) || candownload )
        {
            string *MapData = m_Map->GetMapData( );

            if( !MapData->empty( ) )
            {
                if( candownload || m_AllowDownloads == 1 || ( m_AllowDownloads == 2 && player->GetDownloadAllowed( ) ) )
                {
                    if( !player->GetDownloadStarted( ) && mapSize->GetSizeFlag( ) == 1 )
                    {
                        // inform the client that we are willing to send the map

                        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] map download started for player [" + player->GetName( ) + "]", 6, m_CurrentGameCounter );
                        Send( player, m_Protocol->SEND_W3GS_STARTDOWNLOAD( GetHostPID( ) ) );
                        player->SetDownloadStarted( true );
                        player->SetStartedDownloadingTicks( GetTicks( ) );
                    }
                    else
                    {
                        // send up to 50 pieces of the map at once so that the download goes faster
                        // if we wait for each MAPPART packet to be acknowledged by the client it'll take a long time to download
                        // this is because we would have to wait the round trip time (the ping time) between sending every 1442 bytes of map data
                        // doing it this way allows us to send at least 70 KB in each round trip interval which is much more reasonable
                        // the theoretical throughput is [70 KB * 1000 / ping] in KB/sec so someone with 100 ping (round trip ping, not LC ping) could download at 700 KB/sec

                        // how many players are currently downloading?

                        uint32_t downloaders = CountDownloading( );
                        bool queued = false;

                        // share between max allowed downloaders
                        if( m_UNM->m_maxdownloaders != 0 && downloaders > m_UNM->m_maxdownloaders )
                            downloaders = m_UNM->m_maxdownloaders;

                        // calculate shared rate in KB/s
                        float sharedRate = 1024 * 10;

                        if( m_UNM->m_totaldownloadspeed > 0 )
                            sharedRate = (float)m_UNM->m_totaldownloadspeed / (float)downloaders;

                        float Seconds = (float)( GetTicks( ) - player->GetStartedDownloadingTicks( ) ) / 1000;
                        double Rate = (float)player->GetLastMapPartSent( ) / 1024 / Seconds;
                        uint32_t iRate = (uint32_t)Rate;
                        uint32_t Percent = (uint32_t)( (float)player->GetLastMapPartSent( ) * 100 / (float)MapSize );

                        if( Percent < 0 )
                            Percent = 0;

                        uint32_t ETA = 0;
                        uint32_t TotalTime = 100;

                        if( Percent != 0 )
                        {
                            TotalTime = (uint32_t)(float)( ( 100 * Seconds / (float)Percent ) );
                            ETA = TotalTime - (uint32_t)Seconds;
                        }

                        uint32_t toSend = 1;
                        float setRate = (float)m_UNM->m_clientdownloadspeed;
                        uint32_t DownloadNr = DownloaderNr( player->GetName( ) );

                        if( setRate == 0 )
                            setRate = 2048;

                        // if shared rate is lower than client's allowed rate, use shared rate
                        if( setRate > sharedRate )
                            setRate = sharedRate;

                        // check if there are too many downloading, and set rate to 1 if true
                        if( m_UNM->m_maxdownloaders != 0 && DownloadNr > m_UNM->m_maxdownloaders )
                            queued = true;

                        if( queued )
                            setRate = 1;

                        // if the setrate (KB/s) is higher...send 50 pieces otherwise 1

                        if( player->GetLastMapPartSent( ) == 0 || Rate <= setRate )
                            toSend = 50;

                        string sDownloadInfo = string( );
                        sDownloadInfo = UTIL_ToString( iRate ) + " KB/s (" + UTIL_ToString( Percent ) + "% ";

                        if( !queued && ETA != 0 )
                            sDownloadInfo += "ETA: " + UTIL_ToString( ETA ) + " s ";
                        else if( queued )
                            sDownloadInfo += "queued";

                        sDownloadInfo += ")";
                        player->SetDownloadInfo( sDownloadInfo );

                        if( DownloadNr >= downloaders )
                            ShowDownloading( );

                        while( player->GetLastMapPartSent( ) < mapSize->GetMapSize( ) + 1442 * toSend && player->GetLastMapPartSent( ) < MapSize )
                        {
                            Send( player, m_Protocol->SEND_W3GS_MAPPART( GetHostPID( ), player->GetPID( ), player->GetLastMapPartSent( ), MapData, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                            player->SetLastMapPartSent( player->GetLastMapPartSent( ) + 1442 );
                        }
                    }
                }
            }
            else
            {
                player->SetDeleteMe( true );
                player->SetLeftReason( "doesn't have the map and there is no local copy of the map to send" );
                player->SetLeftCode( PLAYERLEAVE_LOBBY );
                OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
            }
        }
        else
        {
            CPotentialPlayer *potentialCopy = new CPotentialPlayer( m_Protocol, this, player->GetSocket( ) );
            potentialCopy->SetBanned( );

            SendChat( player, "You cannot join this game because you do not have the map." );
            SendChat( player, "If this is DotA, check http://getdota.com/ for your map update" );
            SendChat( player, "If this is not DotA, visit http://epicwar.com/ for map download" );
            player->SetSocket( nullptr );
            player->SetDeleteMe( true );
            player->SetLeftReason( "doesn't have the map and map downloads are disabled" );
            player->SetLeftCode( PLAYERLEAVE_LOBBY );
            OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
            if( GetTime( ) - m_LastNoMapPrint >= m_UNM->m_NoMapPrintDelay && m_UNM->m_NoMapPrintDelay != 0 )
            {
                m_LastNoMapPrint = GetTime( );
                SendAllChat( "Игрок " + player->GetName( ) + " был кикнут т.к. не имеет карты" );
            }
        }
    }
    else
    {
        if( player->GetDownloadStarted( ) && !player->GetDownloadFinished( ) )
        {
            // calculate download rate

            float Seconds = (float)( GetTicks( ) - player->GetStartedDownloadingTicks( ) ) / 1000;
            float Rate = (float)MapSize / 1024 / Seconds;
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] map download finished for player [" + player->GetName( ) + "] in " + UTIL_ToString( Seconds, 2 ) + " seconds", 6, m_CurrentGameCounter );

            if( m_UNM->m_ShowFinishDownloadingInfo )
                SendAllChat( gLanguage->PlayerDownloadedTheMap( player->GetName( ), UTIL_ToString( Seconds, 2 ), UTIL_ToString( Rate, 2 ) ) );

            player->SetDownloadFinished( true );
            player->SetFinishedDownloadingTime( GetTime( ) );

            if( m_DownloadOnlyMode )
                SendChat( player->GetPID( ), "This is a download only game, you should leave now" );
        }
        else if( m_DownloadOnlyMode )
        {
            player->SetDeleteMe( true );
            player->SetLeftReason( "does have the map and this is a download only game" );
            player->SetLeftCode( PLAYERLEAVE_LOBBY );
            OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
        }
    }

    unsigned char NewDownloadStatus = static_cast<unsigned char>( (float)mapSize->GetMapSize( ) / MapSize * 100 );
    unsigned char SID = GetSIDFromPID( player->GetPID( ) );

    if( NewDownloadStatus > 100 )
        NewDownloadStatus = 100;

    if( SID < m_Slots.size( ) )
    {
        // only send the slot info if the download status changed

        if( m_Slots[SID].GetDownloadStatus( ) != NewDownloadStatus )
        {
            m_Slots[SID].SetDownloadStatus( NewDownloadStatus );

            // we don't actually send the new slot info here
            // this is an optimization because it's possible for a player to download a map very quickly
            // if we send a new slot update for every percentage change in their download status it adds up to a lot of data
            // instead, we mark the slot info as "out of date" and update it only once in awhile (once per second when this comment was made)

            m_SlotInfoChanged = true;
        }
    }
}

void CBaseGame::EventPlayerPongToHost( CGamePlayer *player )
{
    // autokick players with excessive pings but only if they're not reserved and we've received at least 3 pings from them
    // also don't kick anyone if the game is loading or loaded - this could happen because we send pings during loading but we stop sending them after the game is loaded
    // see the Update function for where we send pings

    if( !m_GameLoading && !m_GameLoaded && !player->GetDeleteMe( ) && !player->GetReserved( ) && player->GetNumPings( ) >= 3 && player->GetPing( m_UNM->m_LCPings ) > m_UNM->m_AutoKickPing && ( m_Slots[GetSIDFromPID( player->GetPID( ) )].GetDownloadStatus( ) < 11 || m_Slots[GetSIDFromPID( player->GetPID( ) )].GetDownloadStatus( ) == 255 ) )
    {
        // send a chat message because we don't normally do so when a player leaves the lobby

        SendAllChat( gLanguage->AutokickingPlayerForExcessivePing( player->GetName( ), UTIL_ToString( player->GetPing( m_UNM->m_LCPings ) ) ) );
        player->SetDeleteMe( true );
        player->SetLeftReason( "автоматически выкинут из-за высокого пинга: " + UTIL_ToString( player->GetPing( m_UNM->m_LCPings ) ) );
        player->SetLeftCode( PLAYERLEAVE_LOBBY );
        OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
    }
}

void CBaseGame::EventGameStarted( )
{
    if( m_UNM->m_ShuffleSlotsOnStart )
        ShuffleSlots( );

    // remove the virtual host player

    DeleteVirtualHost( );

    // try add obs/fake player if the number of players in the game = 1 (if m_DontDeleteVirtualHost = true)

    if( m_DontDeleteVirtualHost && GetNumHumanPlayers( ) == 1 && m_ObsPlayerPID == 255 && m_FakePlayers.empty( ) )
        CreateObsPlayer( m_VirtualHostName );

    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] started loading with " + UTIL_ToString( GetNumHumanPlayers( ) ) + " players", 6, m_CurrentGameCounter );

    // reserve players from last game

    if( m_UNM->m_HoldPlayersForRMK  )
    {
        for( vector<CGamePlayer *> ::iterator j = m_Players.begin( ); j != m_Players.end( ); j++ )
        {
            m_UNM->m_PlayerNamesfromRMK.push_back( ( *j )->GetName( ) );
            m_UNM->m_PlayerServersfromRMK.push_back( ( *j )->GetJoinedRealm( ) );
        }

        CONSOLE_Print( "[UNM] reserving players from [" + m_UNM->IncGameNrDecryption( GetGameName( ) ) + "] for next game" );
    }

    if( m_autoinsultlobby )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Auto insult turned off because the game started", 6, m_CurrentGameCounter );
        m_autoinsultlobby = false;
    }

    if( m_StartedVoteStartTime != 0 )
        m_StartedVoteStartTime = 0;

    if( m_UNM->m_GameStartedMessagePrinting )
    {
        for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
        {
            if( !( *i )->m_SpamSubBot )
                ( *i )->m_GameStartedMessagePrinting = true;
        }
    }

    // encode the HCL command string in the slot handicaps
    // here's how it works:
    // the user inputs a command string to be sent to the map
    // it is almost impossible to send a message from the bot to the map so we encode the command string in the slot handicaps
    // this works because there are only 6 valid handicaps but Warcraft III allows the bot to set up to 256 handicaps
    // we encode the original (unmodified) handicaps in the new handicaps and use the remaining space to store a short message
    // only occupied slots deliver their handicaps to the map and we can send one character (from a list) per handicap
    // when the map finishes loading, assuming it's designed to use the HCL system, it checks if anyone has an invalid handicap
    // if so, it decodes the message from the handicaps and restores the original handicaps using the encoded values
    // the meaning of the message is specific to each map and the bot doesn't need to understand it
    // e.g. you could send game modes, # of rounds, level to start on, anything you want as long as it fits in the limited space available
    // note: if you attempt to use the HCL system on a map that does not support HCL the bot will drastically modify the handicaps
    // since the map won't automatically restore the original handicaps in this case your game will be ruined

    if( !m_HCLCommandString.empty( ) )
    {
        if( m_HCLCommandString.size( ) <= GetSlotsOccupied( ) )
        {
            string HCLChars = "abcdefghijklmnopqrstuvwxyz0123456789 -=,.";

            if( m_HCLCommandString.find_first_not_of( HCLChars ) == string::npos )
            {
                unsigned char EncodingMap[256];
                unsigned char j = 0;

                for( uint32_t i = 0; i < 256; i++ )
                {
                    // the following 7 handicap values are forbidden

                    if( j == 0 || j == 50 || j == 60 || j == 70 || j == 80 || j == 90 || j == 100 )
                        j++;

                    EncodingMap[i] = j++;
                }

                unsigned char CurrentSlot = 0;

                for( string::iterator si = m_HCLCommandString.begin( ); si != m_HCLCommandString.end( ); si++ )
                {
                    while( m_Slots[CurrentSlot].GetSlotStatus( ) != SLOTSTATUS_OCCUPIED )
                        CurrentSlot++;

                    unsigned char HandicapIndex = ( m_Slots[CurrentSlot].GetHandicap( ) - 50 ) / 10;
                    unsigned char CharIndex = static_cast<unsigned char>(HCLChars.find( *si ));
                    m_Slots[CurrentSlot++].SetHandicap( EncodingMap[HandicapIndex + CharIndex * 6] );
                }

                SendAllSlotInfo( );
                m_HCL = true;

                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] successfully encoded HCL command string [" + m_HCLCommandString + "]", 6, m_CurrentGameCounter );
            }
            else
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] encoding HCL command string [" + m_HCLCommandString + "] failed because it contains invalid characters", 6, m_CurrentGameCounter );
        }
        else
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] encoding HCL command string [" + m_HCLCommandString + "] failed because there aren't enough occupied slots", 6, m_CurrentGameCounter );
    }

    // send a final slot info update if necessary
    // this typically won't happen because we prevent the !start command from completing while someone is downloading the map
    // however, if someone uses !start force while a player is downloading the map this could trigger
    // this is because we only permit slot info updates to be flagged when it's just a change in download status, all others are sent immediately
    // it might not be necessary but let's clean up the mess anyway

    if( m_SlotInfoChanged )
        SendAllSlotInfo( );

    m_StartedLoadingTicks = GetTicks( );
    m_LastLagScreenResetTime = GetTime( );
    m_GameLoading = true;

    // since we use a fake countdown to deal with leavers during countdown the COUNTDOWN_START and COUNTDOWN_END packets are sent in quick succession
    // send a start countdown packet

    if( !m_NormalCountdown )
        SendAll( m_Protocol->SEND_W3GS_COUNTDOWN_START( ) );

    // send an end countdown packet

    SendAll( m_Protocol->SEND_W3GS_COUNTDOWN_END( ) );

    // send a game loaded packet for the obs player (if present)

    if( m_ObsPlayerPID != 255 )
        SendAll( m_Protocol->SEND_W3GS_GAMELOADED_OTHERS( m_ObsPlayerPID, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

    // send a game loaded packet for the fake player (if present)

    for( vector<unsigned char> ::iterator i = m_FakePlayers.begin( ); i != m_FakePlayers.end( ); ++i )
        SendAll( m_Protocol->SEND_W3GS_GAMELOADED_OTHERS( *i, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

    // record the starting number of players

    m_StartPlayers = GetNumHumanPlayers( );

    // close the listening socket

    delete m_Socket;
    m_Socket = nullptr;

    // delete any potential players that are still hanging around

    for( vector<CPotentialPlayer *> ::iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); i++ )
        delete *i;

    m_Potentials.clear( );


    // set initial values for replay

    if( m_Replay )
    {
        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            m_Replay->AddPlayer( ( *i )->GetPID( ), ( *i )->GetName( ) );

        for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
        {
            if( i < m_FakePlayersNames.size( ) )
                m_Replay->AddPlayer( m_FakePlayers[i], m_FakePlayersNames[i] );
        }

        if( m_ObsPlayerPID != 255 )
            m_Replay->AddPlayer( m_ObsPlayerPID, m_ObsPlayerName );

        m_Replay->SetSlots( m_Slots );
        m_Replay->SetRandomSeed( m_RandomSeed );
        m_Replay->SetSelectMode( m_Map->GetMapGameType( ) == GAMETYPE_CUSTOM ? 3 : 0 );
        m_Replay->SetStartSpotCount( static_cast<unsigned char>(m_Map->GetMapNumPlayers( )) );

        if( m_SaveGame )
        {
            uint32_t MapGameType = MAPGAMETYPE_SAVEDGAME;

            if( m_GameState == GAME_PRIVATE )
                MapGameType |= MAPGAMETYPE_PRIVATEGAME;

            m_Replay->SetMapGameType( MapGameType );
        }
        else
        {
            uint32_t MapGameType = m_Map->GetMapGameType( );
            MapGameType |= MAPGAMETYPE_UNKNOWN0;

            if( m_GameState == GAME_PRIVATE )
                MapGameType |= MAPGAMETYPE_PRIVATEGAME;

            m_Replay->SetMapGameType( MapGameType );
        }

        if( !m_Players.empty( ) )
        {
            // this might not be necessary since we're going to overwrite the replay's host PID and name everytime a player leaves

            m_Replay->SetHostPID( m_Players[0]->GetPID( ) );
            m_Replay->SetHostName( m_Players[0]->GetName( ) );
        }
    }

    // build a stat string for use when saving the replay
    // we have to build this now because the map data is going to be deleted

    BYTEARRAY StatString;
    UTIL_AppendByteArray( StatString, m_Map->GetMapGameFlags( ) );
    StatString.push_back( 0 );
    UTIL_AppendByteArray( StatString, m_Map->GetMapWidth( ) );
    UTIL_AppendByteArray( StatString, m_Map->GetMapHeight( ) );
    UTIL_AppendByteArray( StatString, m_Map->GetMapCRC( ) );
    UTIL_AppendByteArray( StatString, m_Map->GetMapPath( ) );
    UTIL_AppendByteArray( StatString, "UNM Bot" );
    StatString.push_back( 0 );
    UTIL_AppendByteArray( StatString, m_Map->GetMapSHA1( ) ); // note: in replays generated by Warcraft III it stores 20 zeros for the SHA1 instead of the real thing
    StatString = UTIL_EncodeStatString( StatString );
    m_StatString = string( StatString.begin( ), StatString.end( ) );

    // delete the map data

    delete m_Map;
    m_Map = nullptr;

    if( m_LoadInGame )
    {
        // buffer all the player loaded messages
        // this ensures that every player receives the same set of player loaded messages in the same order, even if someone leaves during loading
        // if someone leaves during loading we buffer the leave message to ensure it gets sent in the correct position but the player loaded message wouldn't get sent if we didn't buffer it now

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            for( vector<CGamePlayer *> ::iterator j = m_Players.begin( ); j != m_Players.end( ); j++ )
                ( *j )->AddLoadInGameData( m_Protocol->SEND_W3GS_GAMELOADED_OTHERS( ( *i )->GetPID( ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
        }
    }

    // move the game to the games in progress vector

    m_GameName = m_UNM->IncGameNrDecryption( m_GameName );
    m_UNM->m_CurrentGameName.clear( );
    m_UNM->m_CurrentGame = nullptr;
    m_UNM->m_Games.push_back( this );

    if( m_UNM->m_newGame )
    {
        m_UNM->CreateGame( m_Map, m_UNM->m_newGameGameState, false, m_UNM->m_newGameName, m_UNM->m_newGameUser, m_UNM->m_newGameUser, m_UNM->m_newGameServer, true, string( ) );
        m_UNM->m_newGame = false;
    }

    // and finally reenter battle.net chat and also fill a vector of servers

    m_Servers = string( );

    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
    {
        if( ( *i )->GetLoggedIn( ) && !UTIL_StringSearch( m_Servers, ( *i )->GetServer( ), 1 ) )
        {
            if( m_Servers.size( ) + ( *i )->GetServer( ).size( ) <= 510 )
                m_Servers += ( *i )->GetServer( ) + ", ";
        }

        ( *i )->QueueGameUncreate( );
        ( *i )->QueueEnterChat( );
    }

    if( m_Servers.size( ) > 2 )
        m_Servers = m_Servers.substr( 0, m_Servers.size( ) - 2 );
}

void CBaseGame::EventGameLoaded( )
{
    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] finished loading with " + UTIL_ToString( GetNumHumanPlayers( ) ) + " players", 6, m_CurrentGameCounter );
    unsigned char Nr = 255;
    unsigned char Nrt;
    CGamePlayer *p = nullptr;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        Nrt = GetSIDFromPID( ( *i )->GetPID( ) );

        if( Nrt < Nr )
        {
            Nr = Nrt;
            p = ( *i );
        }

        ( *i )->m_BufferMessages.clear( );
    }

    m_GameLoadedTime = GetTime( );
    m_LastRedefinitionTotalLaggingTime = GetTime( );
    m_LastActionSentTicks = GetTicks( );
    m_GameLoading = false;
    m_GameLoaded = true;

    // remember how many players were at game start

    m_PlayersatStart = m_Players.size( );

    // update slots - needed for switching detection

    for( unsigned char s = 0; s < m_Slots.size( ); s++ )
    {
        CGamePlayer *pp = GetPlayerFromSID( s );

        if( pp )
            pp->SetSID( s );
    }

    // if HCL was sent message first player not to set the map mode

    if( !m_LoadInGame && p != nullptr && !m_HCLCommandString.empty( ) )
        SendChat( p, "-" + m_HCLCommandString + " мод был выбран автоматически" );

    // if no HCL, message first player to set the map mode

    else if( m_UNM->m_manualhclmessage > 0 && p != nullptr && m_HCLCommandString.empty( ) )
    {
        string Modes = string( );

        if( m_GameName.find( "-" ) != string::npos )
        {
            uint32_t j = 0;
            uint32_t k = 0;
            uint32_t i = 0;
            string mode = string( );

            while( j < m_GameName.length( ) - 1 && ( m_GameName.find( "-", j ) != string::npos ) )
            {
                k = m_GameName.find( "-", j );
                i = m_GameName.find( " ", k );

                if( i == 0 )
                    i = m_GameName.length( ) - k + 1;
                else
                    i = i - k;

                mode = m_GameName.substr( k, i );

                if( !Modes.empty( ) && !mode.empty( ) )
                    Modes += " ";

                Modes += mode;
                j = k + i;
            }
        }

        if( !Modes.empty( ) )
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] " + p->GetName( ) + " должен ввести мод: " + Modes, 6, m_CurrentGameCounter );
            SendChat( p, p->GetName( ) + ", вы на первом слоте, введите мод \"" + Modes + "\" прямо сейчас!" );
        }
        else
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] " + p->GetName( ) + " должен ввести любой мод", 6, m_CurrentGameCounter );
            SendChat( p, p->GetName( ) + ", вы на первом слоте, введите любой мод прямо сейчас!" );
        }
    }

    // send shortest, longest, and personal load times to each player

    CGamePlayer *Shortest = nullptr;
    CGamePlayer *Longest = nullptr;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !Shortest || ( *i )->GetFinishedLoadingTicks( ) < Shortest->GetFinishedLoadingTicks( ) )
            Shortest = *i;

        if( !Longest || ( *i )->GetFinishedLoadingTicks( ) > Longest->GetFinishedLoadingTicks( ) )
            Longest = *i;
    }

    if( Shortest && Longest )
    {
        SendAllChat( gLanguage->ShortestLoadByPlayer( Shortest->GetName( ), UTIL_ToString( (float)( Shortest->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000, 2 ) ) );
        SendAllChat( gLanguage->LongestLoadByPlayer( Longest->GetName( ), UTIL_ToString( (float)( Longest->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000, 2 ) ) );
    }

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        SendChat( *i, gLanguage->YourLoadingTimeWas( UTIL_ToString( (float)( ( *i )->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000, 2 ) ) );
}

unsigned char CBaseGame::GetSIDFromPID( unsigned char PID )
{
    if( m_Slots.size( ) > 255 )
        return 255;

    for( unsigned char i = 0; i < m_Slots.size( ); i++ )
    {
        if( m_Slots[i].GetPID( ) == PID )
            return i;
    }

    return 255;
}

QString CBaseGame::GetColoredNameFromPID( unsigned char PID )
{
    if( m_Slots.size( ) > 255 )
        return QString( );

    for( unsigned char i = 0; i < m_Slots.size( ); i++ )
    {
        if( m_Slots[i].GetPID( ) == PID )
        {
            for( vector<CGamePlayer *> ::iterator n = m_Players.begin( ); n != m_Players.end( ); n++ )
            {
                if( !( *n )->GetLeftMessageSent( ) && ( *n )->GetPID( ) == PID )
                {
                    string ColoredName = string( );

                    switch( m_Slots[i].GetColour( ) )
                    {
                        case 0:
                        ColoredName = "<font color = #ff0000>" + ( *n )->GetName( ) + "</font>";
                        break;
                        case 1:
                        ColoredName = "<font color = #0000ff>" + ( *n )->GetName( ) + "</font>";
                        break;
                        case 2:
                        ColoredName = "<font color = #00ffff>" + ( *n )->GetName( ) + "</font>";
                        break;
                        case 3:
                        ColoredName = "<font color = #800080>" + ( *n )->GetName( ) + "</font>";
                        break;
                        case 4:
                        ColoredName = "<font color = #ffff00>" + ( *n )->GetName( ) + "</font>";
                        break;
                        case 5:
                        ColoredName = "<font color = #ffa500>" + ( *n )->GetName( ) + "</font>";
                        break;
                        case 6:
                        ColoredName = "<font color = #00ff00>" + ( *n )->GetName( ) + "</font>";
                        break;
                        case 7:
                        ColoredName = "<font color = #ff00ff>" + ( *n )->GetName( ) + "</font>";
                        break;
                        case 8:
                        ColoredName = "<font color = #858585>" + ( *n )->GetName( ) + "</font>";
                        break;
                        case 9:
                        ColoredName = "<font color = #add8e6>" + ( *n )->GetName( ) + "</font>";
                        break;
                        case 10:
                        ColoredName = "<font color = #008000>" + ( *n )->GetName( ) + "</font>";
                        break;
                        case 11:
                        ColoredName = "<font color = #964B00>" + ( *n )->GetName( ) + "</font>";
                        break;
                        default:
                        ColoredName = "<font color = #000000>" + ( *n )->GetName( ) + "</font>";
                    }

                    return QString::fromUtf8( ColoredName.c_str( ) );
                }
            }

            return QString( );
        }
    }

    return QString( );
}

CGamePlayer *CBaseGame::GetPlayerFromPID( unsigned char PID )
{
    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) && ( *i )->GetPID( ) == PID )
            return *i;
    }

    return nullptr;
}


CGamePlayer *CBaseGame::GetPlayerFromSID( unsigned char SID )
{
    if( SID < m_Slots.size( ) )
        return GetPlayerFromPID( m_Slots[SID].GetPID( ) );

    return nullptr;
}

CGamePlayer *CBaseGame::GetPlayerFromName( string name, bool sensitive )
{
    if( !sensitive )
        transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) )
        {
            string TestName = ( *i )->GetName( );

            if( !sensitive )
                transform( TestName.begin( ), TestName.end( ), TestName.begin( ), static_cast<int(*)(int)>(tolower) );

            if( TestName == name )
                return *i;
        }
    }

    return nullptr;
}

bool CBaseGame::OwnerInGame( )
{
    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( IsOwner( ( *i )->GetName( ) ) )
            return true;
    }

    return false;
}

uint32_t CBaseGame::GetPlayerFromNamePartial( string name, CGamePlayer **player )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    uint32_t Matches = 0;
    *player = nullptr;

    // try to match each player with the passed string (e.g. "Varlock" would be matched with "lock")

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) )
        {
            string TestName = ( *i )->GetName( );
            transform( TestName.begin( ), TestName.end( ), TestName.begin( ), static_cast<int(*)(int)>(tolower) );

            if( TestName.find( name ) != string::npos )
            {
                Matches++;
                *player = *i;

                // if the name matches exactly stop any further matching

                if( TestName == name )
                {
                    Matches = 1;
                    break;
                }
            }
        }
    }

    return Matches;
}

CGamePlayer *CBaseGame::GetPlayerFromNamePartial( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    CGamePlayer *player = nullptr;

    // try to match each player with the passed string (e.g. "Varlock" would be matched with "lock")

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) && ( *i )->GetNameLower( ).find( name ) != string::npos )
        {
            player = *i;

            // if the name matches exactly stop any further matching

            if( ( *i )->GetNameLower( ) == name )
                return player;
        }
    }

    return player;
}

CGamePlayer *CBaseGame::GetPlayerFromColour( unsigned char colour )
{
    for( unsigned char i = 0; i < m_Slots.size( ); i++ )
    {
        if( m_Slots[i].GetColour( ) == colour )
            return GetPlayerFromSID( i );
    }

    return nullptr;
}

QStringList CBaseGame::GetGameDataInit( )
{
    QStringList GameInfoInit;
    GameInfoInit.append( QString::fromUtf8( m_GameName.c_str( ) ) );
    GameInfoInit.append( QString::fromUtf8( m_GetMapName.c_str( ) ) );

    if( m_GameState == GAME_PUBLIC )
        GameInfoInit.append( "Публичная игра" );
    else if( m_GameState == GAME_PRIVATE )
        GameInfoInit.append( "Приватная игра" );
    else
        GameInfoInit.append( "Особая игра" );

    GameInfoInit.append( "Неначатая игра (лобби)" );
    GameInfoInit.append( "Открытая (админ-команды доступны всем админам)" );
    GameInfoInit.append( QString::fromUtf8( m_CreatorName.c_str( ) ) );
    GameInfoInit.append( QString::fromUtf8( m_OwnerName.c_str( ) ) );
    GameInfoInit.append( QString::fromUtf8( m_DefaultOwner.c_str( ) ) );
    GameInfoInit.append( QString::fromUtf8( m_CreatorServer.c_str( ) ) );
    struct tm * timeinfo;
    char buffer[150];
    string sDate;
    time_t Now = time( nullptr );
    timeinfo = localtime( &Now );
    strftime( buffer, 150, "%d.%m.%y %H:%M:%S", timeinfo );
    sDate = buffer;
    GameInfoInit.append( QString::fromUtf8( sDate.c_str( ) ) );
    GameInfoInit.append( "Статистика отключена" );
    return GameInfoInit;
}

QStringList CBaseGame::GetGameData( )
{
    QStringList GameInfo;
    GameInfo.append( QString::fromUtf8( m_UNM->IncGameNrDecryption( m_GameName ).c_str( ) ) );

    if( m_GameState == GAME_PUBLIC )
        GameInfo.append( "Публичная игра" );
    else if( m_GameState == GAME_PRIVATE )
        GameInfo.append( "Приватная игра" );
    else
        GameInfo.append( "Особая игра" );

    if( !m_GameLoading && !m_GameLoaded )
        GameInfo.append( "Неначатая игра (лобби)" );
    else if( m_GameLoading )
        GameInfo.append( "Неначатая игра (игроки загружают карту)" );
    else
        GameInfo.append( "Начатая игра" );

    GameInfo.append( "Открытая игра (админ-команды доступны только владельцам игры)" );
    GameInfo.append( QString::fromUtf8( m_OwnerName.c_str( ) ) );
    return GameInfo;
}

QList<QStringList> CBaseGame::GetPlayersData( )
{
    QStringList names;
    QStringList races;
    QStringList clans;
    QStringList colors;
    QStringList handicaps;
    QStringList statuses;
    QStringList pings;
    QStringList servers;
    QStringList accesses;
    QStringList levels;
    QStringList ips;
    QStringList countries;
    string name = string( );
    string race = string( );
    string clan = string( );
    string colour = string( );
    string handicap = string( );
    string status = string( );
    string ping = string( );
    string server = string( );
    string access = string( );
    string level = string( );
    string ip = string( );
    string country = string( );

    for( unsigned char i = 0; i < m_Slots.size( ); ++i )
    {
        name = string( );
        race = string( );
        clan = "Клан " + UTIL_ToString( m_Slots[i].GetTeam( ) + 1 );
        colour = string( );
        handicap = UTIL_ToString( m_Slots[i].GetHandicap( ) ) + "%";
        status = string( );
        ping = string( );
        server = string( );
        access = string( );
        level = string( );
        ip = string( );
        country = string( );

        switch( m_Slots[i].GetRace( ) )
        {
            case SLOTRACE_RANDOM:
            race = "Случайная раса";
            break;
            case SLOTRACE_UNDEAD:
            race = "Нежить";
            break;
            case SLOTRACE_HUMAN:
            race = "Альянс";
            break;
            case SLOTRACE_ORC:
            race = "Орда";
            break;
            case SLOTRACE_NIGHTELF:
            race = "Ночные эльфы";
            break;
            case 96:
            race = "Случайная раса";
            break;
            case 72:
            race = "Нежить";
            break;
            case 65:
            race = "Альянс";
            break;
            case 66:
            race = "Орда";
            break;
            case 68:
            race = "Ночные эльфы";
            break;
            case 64:
            race = "Случайная раса";
            break;
            default:
            race = "Случайная раса";
        }

        switch( m_Slots[i].GetColour( ) )
        {
            case 0:
            colour = "red";
            break;
            case 1:
            colour = "blue";
            break;
            case 2:
            colour = "teal";
            break;
            case 3:
            colour = "purple";
            break;
            case 4:
            colour = "yellow";
            break;
            case 5:
            colour = "orange";
            break;
            case 6:
            colour = "green";
            break;
            case 7:
            colour = "pink";
            break;
            case 8:
            colour = "gray";
            break;
            case 9:
            colour = "lightblue";
            break;
            case 10:
            colour = "darkgreen";
            break;
            case 11:
            colour = "brown";
            break;
            default:
            colour = "black";
        }

        if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetComputer( ) == 0 )
        {
            CGamePlayer *player = GetPlayerFromSID2( i );

            if( player )
            {
                name = player->GetName( );
                country = "unknown";
                level = "1 lvl";
                ping = UTIL_ToString( player->GetPing( m_UNM->m_LCPings ) ) + " ms";
                ip = player->GetExternalIPString( );

                if( !m_UNM->IsBnetServer( player->GetJoinedRealm( ) ) )
                    server = "LAN";
                else
                    server = player->GetJoinedRealm( );

                if( IsOwner( player->GetName( ) ) )
                    access = "owner";
                else
                    access = "user";

                if( !m_GameLoaded && !m_GameLoading )
                {
                    if( m_Slots[i].GetDownloadStatus( ) == 255 || m_Slots[i].GetDownloadStatus( ) == 100 )
                        status = "ожидает старта игры";
                    else
                        status = "качает карту: " + UTIL_ToString( m_Slots[i].GetDownloadStatus( ) ) + "%";
                }
                else if( m_GameLoading )
                {
                    if( !player->GetFinishedLoading( ) )
                        status = "грузит карту";
                    else
                        status = "ждет начала игры";
                }
                else if( player->GetLeftTime( ) > 0 )
                    status = "ливнул на " + UTIL_ToString( player->GetLeftTime( ) ) + " секунде, причина: " + player->GetLeftReason( );
                else
                    status = "играет";
            }
            else
                name = "Fakeplayer";
        }
        else if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED )
        {
            if( m_Slots[i].GetComputerType( ) == SLOTCOMP_EASY )
                name = "Комьютер (слабый)";
            else if( m_Slots[i].GetComputerType( ) == SLOTCOMP_NORMAL )
                name = "Комьютер (средний)";
            else if( m_Slots[i].GetComputerType( ) == SLOTCOMP_HARD )
                name = "Комьютер (сильный)";
            else
                name = "Комьютер";
        }
        else if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN )
            name = "Открыто";
        else if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_CLOSED )
            name = "Закрыто";

        names.append( QString::fromUtf8( name.c_str( ) ) );
        races.append( QString::fromUtf8( race.c_str( ) ) );
        clans.append( QString::fromUtf8( clan.c_str( ) ) );
        colors.append( QString::fromUtf8( colour.c_str( ) ) );
        handicaps.append( QString::fromUtf8( handicap.c_str( ) ) );
        statuses.append( QString::fromUtf8( status.c_str( ) ) );
        pings.append( QString::fromUtf8( ping.c_str( ) ) );
        servers.append( QString::fromUtf8( server.c_str( ) ) );
        accesses.append( QString::fromUtf8( access.c_str( ) ) );
        levels.append( QString::fromUtf8( level.c_str( ) ) );
        ips.append( QString::fromUtf8( ip.c_str( ) ) );
        countries.append( QString::fromUtf8( country.c_str( ) ) );
    }

    return { names, races, clans, colors, handicaps, statuses, pings, accesses, levels, servers, ips, countries };
}

unsigned char CBaseGame::GetNewPID( )
{
    // find an unused PID for a new player to use

    for( unsigned char TestPID = 1; TestPID < 255; TestPID++ )
    {
        if( TestPID == m_VirtualHostPID || TestPID == m_ObsPlayerPID )
            continue;

        bool InUse = false;

        for( vector<unsigned char> ::iterator i = m_FakePlayers.begin( ); i != m_FakePlayers.end( ); ++i )
        {
            if( *i == TestPID )
            {
                InUse = true;
                break;
            }
        }

        if( InUse )
            continue;

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( !( *i )->GetLeftMessageSent( ) && ( *i )->GetPID( ) == TestPID )
            {
                InUse = true;
                break;
            }
        }

        if( !InUse )
            return TestPID;
    }

    // this should never happen

    return 255;
}

unsigned char CBaseGame::GetNewColour( )
{
    // find an unused colour for a player to use

    for( unsigned char TestColour = 0; TestColour < 12; TestColour++ )
    {
        bool InUse = false;

        for( unsigned char i = 0; i < m_Slots.size( ); i++ )
        {
            if( m_Slots[i].GetColour( ) == TestColour )
            {
                InUse = true;
                break;
            }
        }

        if( !InUse )
            return TestColour;
    }

    // this should never happen

    return 12;
}

BYTEARRAY CBaseGame::GetAllyPIDs( unsigned char PID )
{
    BYTEARRAY result;
    unsigned char team = m_Slots[GetSIDFromPID( PID )].GetTeam( );

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) && GetTeam( GetSIDFromPID( ( *i )->GetPID( ) ) ) == team )
            result.push_back( ( *i )->GetPID( ) );
    }

    return result;
}

BYTEARRAY CBaseGame::GetAdminPIDs( )
{
    BYTEARRAY result;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( IsOwner( ( *i )->GetName( ) ) )
            result.push_back( ( *i )->GetPID( ) );
    }

    return result;
}

BYTEARRAY CBaseGame::Silence( BYTEARRAY PIDs )
{
    BYTEARRAY result;

    for( uint32_t j = 0; j < PIDs.size( ); j++ )
    {
        CGamePlayer *p = GetPlayerFromPID( PIDs[j] );

        if( p )
        {
            if( !p->GetSilence( ) )
                result.push_back( PIDs[j] );
        }
        else
            result.push_back( PIDs[j] );
    }
    return result;
}

BYTEARRAY CBaseGame::AddRootAdminPIDs( BYTEARRAY PIDs, unsigned char omitpid )
{
    BYTEARRAY result;
    unsigned char root;
    bool isadmin = false;

    for( uint32_t j = 0; j < PIDs.size( ); j++ )
        result.push_back( PIDs[j] );

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        isadmin = IsDefaultOwner( ( *i )->GetName( ) );
        root = ( *i )->GetPID( );

        if( !( *i )->GetLeftMessageSent( ) && isadmin && ( *i )->GetPID( ) != omitpid )
        {
            bool isAlready = false;

            for( uint32_t j = 0; j < PIDs.size( ); j++ )
            {
                if( root == PIDs[j] )
                    isAlready = true;
            }

            if( !isAlready )
                result.push_back( root );
        }
    }

    return result;
}

BYTEARRAY CBaseGame::GetRootAdminPIDs( unsigned char omitpid )
{
    BYTEARRAY result;
    bool isadmin = false;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        isadmin = IsDefaultOwner( ( *i )->GetName( ) );

        if( !( *i )->GetLeftMessageSent( ) && isadmin && ( *i )->GetPID( ) != omitpid )
            result.push_back( ( *i )->GetPID( ) );
    }

    return result;
}

BYTEARRAY CBaseGame::GetRootAdminPIDs( )
{
    BYTEARRAY result;
    bool isadmin = false;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        isadmin = IsDefaultOwner( ( *i )->GetName( ) );

        if( !( *i )->GetLeftMessageSent( ) && isadmin )
            result.push_back( ( *i )->GetPID( ) );
    }

    return result;
}

BYTEARRAY CBaseGame::GetPIDs( )
{
    BYTEARRAY result;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) )
            result.push_back( ( *i )->GetPID( ) );
    }

    return result;
}

BYTEARRAY CBaseGame::GetPIDs( unsigned char excludePID )
{
    BYTEARRAY result;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) && ( *i )->GetPID( ) != excludePID )
            result.push_back( ( *i )->GetPID( ) );
    }

    return result;
}

unsigned char CBaseGame::GetHostPID( )
{
    // return the player to be considered the host (it can be any player) - mainly used for sending text messages from the bot

    // try to find the virtual host player first
    if( m_VirtualHostPID != 255 )
        return m_VirtualHostPID;

    // try to find the obsplayer next
    if( m_ObsPlayerPID != 255 )
        return m_ObsPlayerPID;

    // try to find the fakeplayer next
    if( m_FakePlayers.size( ) >= 1 )
        return m_FakePlayers[0];

    // try to find the owner player next

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) && IsOwner( ( *i )->GetName( ) ) )
            return ( *i )->GetPID( );
    }

    // okay then, just use the first available player

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) )
            return ( *i )->GetPID( );
    }

    return 255;
}

unsigned char CBaseGame::GetHostPIDForMessageFromBuffer( unsigned char PID )
{
    // return the player to be considered the host (it can be any player) - mainly used for sending text messages from the bot

    // try to find the virtual host player first
    if( m_VirtualHostPID != 255 )
        return m_VirtualHostPID;

    // try to find the obsplayer next
    if( m_ObsPlayerPID != 255 )
        return m_ObsPlayerPID;

    // try to find the fakeplayer next
    if( m_FakePlayers.size( ) >= 1 )
        return m_FakePlayers[0];

    // okay then, just use PID our player
    return PID;
}

unsigned char CBaseGame::GetEmptySlotForObsPlayer( )
{
    if( m_Slots.size( ) > 255 )
        return 255;

    // look for an empty slot for a new fake player to occupy
    // if reserved is true then we're willing to use closed or occupied slots as long as it wouldn't displace a player with a reserved slot

    for( uint32_t i = m_Slots.size( ); i-- > 0; )
    {
        if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN )
            return static_cast<unsigned char>(i);
    }

    return 255;
}

unsigned char CBaseGame::GetEmptySlotForFakePlayers( )
{
    if( m_Slots.size( ) > 255 )
        return 255;

    // look for an empty slot for a new fake player to occupy
    // if reserved is true then we're willing to use closed or occupied slots as long as it wouldn't displace a player with a reserved slot

    for( uint32_t i = m_Slots.size( ); i-- > 0; )
    {
        if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN && m_Slots[i].GetTeam( ) != 12 )
            return static_cast<unsigned char>(i);
    }

    return 255;
}

unsigned char CBaseGame::GetEmptySlot( bool reserved )
{
    if( m_Slots.size( ) > 255 )
        return 255;

    if( m_SaveGame )
    {
        // unfortunately we don't know which slot each player was assigned in the savegame
        // but we do know which slots were occupied and which weren't so let's at least force players to use previously occupied slots

        vector<CGameSlot> SaveGameSlots = m_SaveGame->GetSlots( );

        for( unsigned char i = 0; i < m_Slots.size( ); i++ )
        {
            if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN && SaveGameSlots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && SaveGameSlots[i].GetComputer( ) == 0 )
                return i;
        }

        // don't bother with reserved slots in savegames
    }
    else
    {
        // look for an empty slot for a new player to occupy
        // if reserved is true then we're willing to use closed or occupied slots as long as it wouldn't displace a player with a reserved slot

        for( unsigned char i = 0; i < m_Slots.size( ); i++ )
        {
            if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN )
                return i;
        }

        if( reserved )
        {
            // no empty slots, but since player is reserved give them a closed slot

            for( unsigned char i = 0; i < m_Slots.size( ); i++ )
            {
                if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_CLOSED )
                    return i;
            }

            // no closed slots either, give them an occupied slot but not one occupied by another reserved player
            // first look for a player who is downloading the map and has the least amount downloaded so far

            unsigned char LeastDownloaded = 100;
            unsigned char LeastSID = 255;

            for( unsigned char i = 0; i < m_Slots.size( ); i++ )
            {
                CGamePlayer *Player = GetPlayerFromSID( i );

                if( Player && !Player->GetReserved( ) && m_Slots[i].GetDownloadStatus( ) < LeastDownloaded )
                {
                    LeastDownloaded = m_Slots[i].GetDownloadStatus( );
                    LeastSID = i;
                }
            }

            if( LeastSID != 255 )
                return LeastSID;

            // nobody who isn't reserved is downloading the map, just choose the first player who isn't reserved

            for( uint32_t i = m_Slots.size( ) - 1; i > 0; i-- )
            {
                CGamePlayer *Player = GetPlayerFromSID( static_cast<unsigned char>(i) );

                if( Player && !Player->GetReserved( ) )
                    return static_cast<unsigned char>(i);
            }
        }
    }

    return 255;
}

unsigned char CBaseGame::GetEmptySlotAdmin( bool reserved )
{
    if( m_Slots.size( ) > 255 )
        return 255;

    if( m_SaveGame )
    {
        // unfortunately we don't know which slot each player was assigned in the savegame
        // but we do know which slots were occupied and which weren't so let's at least force players to use previously occupied slots

        vector<CGameSlot> SaveGameSlots = m_SaveGame->GetSlots( );

        for( unsigned char i = 0; i < m_Slots.size( ); i++ )
        {
            if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN && SaveGameSlots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && SaveGameSlots[i].GetComputer( ) == 0 )
                return i;
        }

        // don't bother with reserved slots in savegames
    }
    else
    {
        // look for an empty slot for a new player to occupy
        // if reserved is true then we're willing to use closed or occupied slots as long as it wouldn't displace a player with a reserved slot

        for( unsigned char i = 0; i < m_Slots.size( ); i++ )
        {
            if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN )
                return i;
        }

        if( reserved )
        {
            // no empty slots, but since player is reserved give them a closed slot

            for( unsigned char i = 0; i < m_Slots.size( ); i++ )
            {
                if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_CLOSED )
                    return i;
            }

            // no closed slots either, give them an occupied slot but not one occupied by another reserved player
            // first look for a player who is downloading the map and has the least amount downloaded so far

            unsigned char LeastDownloaded = 100;
            unsigned char LeastSID = 255;

            for( unsigned char i = 0; i < m_Slots.size( ); i++ )
            {
                CGamePlayer *Player = GetPlayerFromSID( i );

                if( Player && !Player->GetReserved( ) && m_Slots[i].GetDownloadStatus( ) < LeastDownloaded )
                {
                    LeastDownloaded = m_Slots[i].GetDownloadStatus( );
                    LeastSID = i;
                }
            }

            if( LeastSID != 255 )
                return LeastSID;

            // nobody who isn't reserved is downloading the map, just choose the first player who isn't an admin

            for( uint32_t i = m_Slots.size( ) - 1; i > 0; i-- )
            {
                CGamePlayer *Player = GetPlayerFromSID( static_cast<unsigned char>(i) );
                bool isadmin = false;

                if( Player && ( IsOwner( Player->GetName( ) ) || IsDefaultOwner( Player->GetName( ) ) ) )
                    isadmin = true;

                if( Player && !isadmin )
                    return static_cast<unsigned char>(i);
            }
        }
    }

    return 255;
}

unsigned char CBaseGame::GetEmptySlot( unsigned char team, unsigned char PID )
{
    if( m_Slots.size( ) > 255 )
        return 255;

    // find an empty slot based on player's current slot

    unsigned char StartSlot = GetSIDFromPID( PID );

    if( StartSlot < m_Slots.size( ) )
    {
        if( m_Slots[StartSlot].GetTeam( ) != team )
        {
            // player is trying to move to another team so start looking from the first slot on that team
            // we actually just start looking from the very first slot since the next few loops will check the team for us

            StartSlot = 0;
        }

        if( m_SaveGame )
        {
            vector<CGameSlot> SaveGameSlots = m_SaveGame->GetSlots( );

            for( unsigned char i = StartSlot; i < m_Slots.size( ); i++ )
            {
                if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN && m_Slots[i].GetTeam( ) == team && SaveGameSlots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && SaveGameSlots[i].GetComputer( ) == 0 )
                    return i;
            }

            for( unsigned char i = 0; i < StartSlot; i++ )
            {
                if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN && m_Slots[i].GetTeam( ) == team && SaveGameSlots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && SaveGameSlots[i].GetComputer( ) == 0 )
                    return i;
            }
        }
        else
        {
            // find an empty slot on the correct team starting from StartSlot

            for( unsigned char i = StartSlot; i < m_Slots.size( ); i++ )
            {
                if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN && m_Slots[i].GetTeam( ) == team )
                    return i;
            }

            // didn't find an empty slot, but we could have missed one with SID < StartSlot
            // e.g. in the DotA case where I am in slot 4 (yellow), slot 5 (orange) is occupied, and slot 1 (blue) is open and I am trying to move to another slot

            for( unsigned char i = 0; i < StartSlot; i++ )
            {
                if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN && m_Slots[i].GetTeam( ) == team )
                    return i;
            }
        }
    }

    return 255;
}

void CBaseGame::SwapSlots( unsigned char SID1, unsigned char SID2 )
{
    if( SID1 < m_Slots.size( ) && SID2 < m_Slots.size( ) && SID1 != SID2 )
    {
        CGameSlot Slot1 = m_Slots[SID1];
        CGameSlot Slot2 = m_Slots[SID2];

        // update slots - needed for switching detection
        CGamePlayer *p1 = GetPlayerFromSID( SID1 );
        CGamePlayer *p2 = GetPlayerFromSID( SID2 );

        if( p1 )
            p1->SetSID( SID2 );

        if( p2 )
            p2->SetSID( SID1 );

        if( m_Map->GetMapGameType( ) != GAMETYPE_CUSTOM )
        {
            // regular game - swap everything

            m_Slots[SID1] = Slot2;
            m_Slots[SID2] = Slot1;
        }
        else
        {
            // custom game - don't swap the team, colour, or race

            m_Slots[SID1] = CGameSlot( Slot2.GetPID( ), Slot2.GetDownloadStatus( ), Slot2.GetSlotStatus( ), Slot2.GetComputer( ), Slot1.GetTeam( ), Slot1.GetColour( ), Slot1.GetRace( ), Slot2.GetComputerType( ), Slot2.GetHandicap( ) );
            m_Slots[SID2] = CGameSlot( Slot1.GetPID( ), Slot1.GetDownloadStatus( ), Slot1.GetSlotStatus( ), Slot1.GetComputer( ), Slot2.GetTeam( ), Slot2.GetColour( ), Slot2.GetRace( ), Slot1.GetComputerType( ), Slot1.GetHandicap( ) );
        }

        SendAllSlotInfo( );
    }
}

void CBaseGame::SwapSlotsS( unsigned char SID1, unsigned char SID2 )
{
    if( SID1 < m_Slots.size( ) && SID2 < m_Slots.size( ) && SID1 != SID2 )
    {
        CGameSlot Slot1 = m_Slots[SID1];
        CGameSlot Slot2 = m_Slots[SID2];

        if( m_GetMapGameType != GAMETYPE_CUSTOM )
        {
            // regular game - swap everything

            m_Slots[SID1] = Slot2;
            m_Slots[SID2] = Slot1;
        }
        else
        {
            // custom game - don't swap the team, colour, or race

            m_Slots[SID1] = CGameSlot( Slot2.GetPID( ), Slot2.GetDownloadStatus( ), Slot2.GetSlotStatus( ), Slot2.GetComputer( ), Slot1.GetTeam( ), Slot1.GetColour( ), Slot1.GetRace( ), Slot2.GetComputerType( ), Slot2.GetHandicap( ) );
            m_Slots[SID2] = CGameSlot( Slot1.GetPID( ), Slot1.GetDownloadStatus( ), Slot1.GetSlotStatus( ), Slot1.GetComputer( ), Slot2.GetTeam( ), Slot2.GetColour( ), Slot2.GetRace( ), Slot1.GetComputerType( ), Slot1.GetHandicap( ) );
        }
    }
}

void CBaseGame::OpenSlot( unsigned char SID, bool kick )
{
    if( SID < m_Slots.size( ) )
    {
        if( kick )
        {
            CGamePlayer *Player = GetPlayerFromSID( SID );

            if( Player )
            {
                Player->SetDeleteMe( true );
                Player->SetLeftReason( "was kicked when opening a slot" );
                Player->SetLeftCode( PLAYERLEAVE_LOBBY );
            }
        }

        CGameSlot Slot = m_Slots[SID];
        m_Slots[SID] = CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, Slot.GetTeam( ), Slot.GetColour( ), Slot.GetRace( ) );
        SendAllSlotInfo( );
    }
}

void CBaseGame::CloseSlot( unsigned char SID, bool kick )
{
    if( SID < m_Slots.size( ) )
    {
        if( kick )
        {
            CGamePlayer *Player = GetPlayerFromSID( SID );

            if( Player )
            {
                Player->SetDeleteMe( true );
                Player->SetLeftReason( "was kicked when closing a slot" );
                Player->SetLeftCode( PLAYERLEAVE_LOBBY );
            }
        }

        CGameSlot Slot = m_Slots[SID];
        m_Slots[SID] = CGameSlot( 0, 255, SLOTSTATUS_CLOSED, 0, Slot.GetTeam( ), Slot.GetColour( ), Slot.GetRace( ) );
        SendAllSlotInfo( );
    }
}

void CBaseGame::ComputerSlot( unsigned char SID, unsigned char skill, bool kick )
{
    if( SID < m_Slots.size( ) && skill < 3 )
    {
        if( kick )
        {
            CGamePlayer *Player = GetPlayerFromSID( SID );

            if( Player )
            {
                Player->SetDeleteMe( true );
                Player->SetLeftReason( "was kicked when creating a computer in a slot" );
                Player->SetLeftCode( PLAYERLEAVE_LOBBY );
            }
        }

        CGameSlot Slot = m_Slots[SID];
        m_Slots[SID] = CGameSlot( 0, 100, SLOTSTATUS_OCCUPIED, 1, Slot.GetTeam( ), Slot.GetColour( ), Slot.GetRace( ), skill, 100 );
        SendAllSlotInfo( );
    }
}

void CBaseGame::ColourSlot( unsigned char SID, unsigned char colour, bool force )
{
    if( SID < m_Slots.size( ) && colour < 12 )
    {
        // make sure the requested colour isn't already taken

        bool Taken = false;
        unsigned char TakenSID = 0;

        for( unsigned char i = 0; i < m_Slots.size( ); i++ )
        {
            if( m_Slots[i].GetColour( ) == colour )
            {
                TakenSID = i;
                Taken = true;
            }
        }

        if( Taken && ( m_Slots[TakenSID].GetSlotStatus( ) != SLOTSTATUS_OCCUPIED || force ) )
        {
            // the requested colour is currently "taken" by an unused (open or closed) slot
            // but we allow the colour to persist within a slot so if we only update the existing player's colour the unused slot will have the same colour
            // this isn't really a problem except that if someone then joins the game they'll receive the unused slot's colour resulting in a duplicate
            // one way to solve this (which we do here) is to swap the player's current colour into the unused slot

            m_Slots[TakenSID].SetColour( m_Slots[SID].GetColour( ) );
            m_Slots[SID].SetColour( colour );
            SendAllSlotInfo( );
        }
        else if( !Taken )
        {
            // the requested colour isn't used by ANY slot

            m_Slots[SID].SetColour( colour );
            SendAllSlotInfo( );
        }
    }
}

void CBaseGame::OpenAllSlots( )
{
    bool Changed = false;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_CLOSED )
        {
            ( *i ).SetSlotStatus( SLOTSTATUS_OPEN );
            Changed = true;
        }
    }

    if( Changed )
        SendAllSlotInfo( );
}

void CBaseGame::CloseAllSlots( )
{
    bool Changed = false;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_OPEN )
        {
            ( *i ).SetSlotStatus( SLOTSTATUS_CLOSED );
            Changed = true;
        }
    }

    if( Changed )
        SendAllSlotInfo( );
}

void CBaseGame::ShuffleSlots( )
{
    // we only want to shuffle the player slots
    // that means we need to prevent this function from shuffling the open/closed/computer slots too
    // so we start by copying the player slots to a temporary vector

    vector<CGameSlot> PlayerSlots;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && ( *i ).GetComputer( ) == 0 && ( *i ).GetColour( ) != 12 )
            PlayerSlots.push_back( *i );
    }

    // now we shuffle PlayerSlots

    if( m_Map->GetMapGameType( ) == GAMETYPE_CUSTOM )
    {
        // custom game
        // rather than rolling our own probably broken shuffle algorithm we use shuffle because it's guaranteed to do it properly
        // so in order to let shuffle do all the work we need a vector to operate on
        // unfortunately we can't just use PlayerSlots because the team/colour/race shouldn't be modified
        // so make a vector we can use

        vector<unsigned char> SIDs;

        for( unsigned char i = 0; i < PlayerSlots.size( ); i++ )
            SIDs.push_back( i );

        std::random_device rand_dev;
        std::mt19937 rng( rand_dev( ) );
        shuffle( SIDs.begin( ), SIDs.end( ), rng );

        // now put the PlayerSlots vector in the same order as the SIDs vector

        vector<CGameSlot> Slots;

        // as usual don't modify the team/colour/race

        for( unsigned char i = 0; i < SIDs.size( ); i++ )
            Slots.push_back( CGameSlot( PlayerSlots[SIDs[i]].GetPID( ), PlayerSlots[SIDs[i]].GetDownloadStatus( ), PlayerSlots[SIDs[i]].GetSlotStatus( ), PlayerSlots[SIDs[i]].GetComputer( ), PlayerSlots[i].GetTeam( ), PlayerSlots[i].GetColour( ), PlayerSlots[i].GetRace( ), PlayerSlots[i].GetComputerType( ), PlayerSlots[i].GetHandicap( ) ) );

        PlayerSlots = Slots;
    }
    else
    {
        // regular game
        // it's easy when we're allowed to swap the team/colour/race!

        std::random_device rand_dev;
        std::mt19937 rng( rand_dev( ) );
        shuffle( PlayerSlots.begin( ), PlayerSlots.end( ), rng );
    }

    // now we put m_Slots back together again

    vector<CGameSlot> ::iterator CurrentPlayer = PlayerSlots.begin( );
    vector<CGameSlot> Slots;

    for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
    {
        if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && ( *i ).GetComputer( ) == 0 && ( *i ).GetColour( ) != 12 )
        {
            Slots.push_back( *CurrentPlayer );
            CurrentPlayer++;
        }
        else
            Slots.push_back( *i );
    }

    m_Slots = Slots;

    // and finally tell everyone about the new slot configuration

    SendAllSlotInfo( );

    // update slots - needed for switching detection

    for( unsigned char s = 0; s < m_Slots.size( ); s++ )
    {
        CGamePlayer *p = GetPlayerFromSID( s );

        if( p )
            p->SetSID( s );
    }
}

void CBaseGame::AddToSpoofed( string server, string name, bool sendMessage )
{
    CGamePlayer *Player = GetPlayerFromName( name, false );

    if( Player )
    {
        Player->SetSpoofedRealm( server );
        Player->SetSpoofed( true );

        if( !m_UNM->IsSpoofedIP( name, Player->GetExternalIPString( ) ) )
            m_UNM->AddSpoofedIP( name, Player->GetExternalIPString( ) );

        if( sendMessage )
            SendAllChat( gLanguage->SpoofCheckAcceptedFor( server, name ) );
    }
}

bool CBaseGame::IsMuted( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( vector<string> ::iterator i = m_Muted.begin( ); i != m_Muted.end( ); i++ )
    {
        if( *i == name )
            return true;
    }

    return false;
}

uint32_t CBaseGame::IsCensorMuted( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( uint32_t i = 0; i != m_CensorMuted.size( ); i++ )
    {
        if( m_CensorMuted[i] == name )
            return m_CensorMutedSeconds[i] - ( GetTime( ) - m_CensorMutedTime[i] );
    }

    return 0;
}

void CBaseGame::DelFromMuted( string name )
{
    string nam = name;
    transform( nam.begin( ), nam.end( ), nam.begin( ), static_cast<int(*)(int)>(tolower) );

    for( vector<string> ::iterator i = m_Muted.begin( ); i != m_Muted.end( ); i++ )
    {
        if( *i == nam )
        {
            m_Muted.erase( i );
            return;
        }
    }
}

unsigned char CBaseGame::GetTeam( unsigned char SID )
{
    return m_Slots[SID].GetTeam( );
}

bool CBaseGame::IsListening( unsigned char user )
{
    for( uint32_t i = 0; i != m_ListenUser.size( ); i++ )
    {
        if( m_ListenUser[i] == user )
            return true;
    }

    return false;
}

void CBaseGame::SwitchDeny( unsigned char PID )
{
    m_SwitchTime = 0;
    CGamePlayer *p = GetPlayerFromPID( PID );
    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Switch canceled being denied by " + p->GetName( ), 6, m_CurrentGameCounter );
}

void CBaseGame::SwitchAccept( unsigned char PID )
{
    if( m_SwitchTime == 0 || ( GetTime( ) - m_SwitchTime ) > 60 )
        return;

    CGamePlayer *p = GetPlayerFromPID( PID );

    if( !p->m_Switchok )
    {
        p->m_Switchok = true;
        m_SwitchNr++;
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Switch accepted by " + p->GetName( ) + " " + UTIL_ToString( m_SwitchNr ) + "/" + UTIL_ToString( m_Players.size( ) - 1 ), 6, m_CurrentGameCounter );

        if( m_SwitchNr == m_Players.size( ) - 2 )
        {
            // disable gameoverteamdiff if switch is detected (until we handle switches)
            m_GameOverDiffCanceled = true;
            SwapSlotsS( m_SwitchS, m_SwitchT );
            m_Switched = true;
            ReCalculateTeams( );
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] Switch completed", 6, m_CurrentGameCounter );
            m_SwitchTime = 0;
        }
    }
}

void CBaseGame::InitSwitching( unsigned char PID, string nr )
{
    uint32_t CNr;

    CNr = UTIL_ToUInt32( nr );

    if( CNr < 1 || CNr>5 )
        return;

    if( m_SwitchTime != 0 && ( GetTime( ) - m_SwitchTime ) < 60 )
        return;

    unsigned char SID = GetSIDFromPID( PID );
    unsigned char TSID;

    if( m_Slots[SID].GetTeam( ) == 0 )
        TSID = static_cast<unsigned char>( CNr + 4 );
    else
        TSID = static_cast<unsigned char>( CNr - 1 );

    m_SwitchTime = GetTime( );
    m_SwitchNr = 0;
    CGamePlayer *p = GetPlayerFromPID( PID );
    m_SwitchS = SID;
    m_SwitchT = TSID;
    p->m_Switching = true;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        ( *i )->m_Switchok = false;
    }

    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] " + p->GetName( ) + " wants to switch", 6, m_CurrentGameCounter );
}

void CBaseGame::ReCalculateTeams( )
{
    unsigned char sid;
    unsigned char team;
    m_Team1 = 0;
    m_Team2 = 0;
    m_Team3 = 0;
    m_Team4 = 0;
    m_TeamDiff = 0;

    if( m_Players.empty( ) )
        return;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        // ignore players who left and didn't get deleted yet.
        if( *i )
        {
            if( !( *i )->GetLeftMessageSent( ) )
            {
                sid = GetSIDFromPID( ( *i )->GetPID( ) );

                if( sid != 255 )
                {
                    team = m_Slots[sid].GetTeam( );

                    if( team == 0 )
                        m_Team1++;

                    if( team == 1 )
                        m_Team2++;

                    if( team == 2 )
                        m_Team3++;

                    if( team == 3 )
                        m_Team4++;
                }
            }
        }
    }

    if( m_GetMapNumTeams == 2 )
    {
        if( m_Team1 > m_Team2 )
            m_TeamDiff = m_Team1 - m_Team2;

        if( m_Team1 < m_Team2 )
            m_TeamDiff = m_Team2 - m_Team1;
    }

}

void CBaseGame::AddToListen( unsigned char user )
{
    m_ListenUser.push_back( user );
    m_Listen = true;
}

void CBaseGame::DelFromListen( unsigned char user )
{
    for( uint32_t i = 0; i != m_ListenUser.size( ); i++ )
    {
        if( m_ListenUser[i] == user )
        {
            m_ListenUser.erase( m_ListenUser.begin( ) + static_cast<int32_t>(i) );
            return;
        }
    }

    if( m_ListenUser.empty( ) )
        m_Listen = false;
}

void CBaseGame::AddToMuted( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    // check that the user is not already muted

    for( vector<string> ::iterator i = m_Muted.begin( ); i != m_Muted.end( ); i++ )
    {
        if( *i == name )
        {
            return;
        }
    }

    m_Muted.push_back( name );
}

void CBaseGame::AddToCensorMuted( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    // check that the user is not already muted

    for( vector<string> ::iterator i = m_CensorMuted.begin( ); i != m_CensorMuted.end( ); i++ )
    {
        if( *i == name )
        {
            return;
        }
    }

    m_CensorMuted.push_back( name );
    unsigned long secs = 0;
    CGamePlayer *Player = GetPlayerFromName( name, false );

    if( Player )
    {
        Player->SetCensors( Player->GetCensors( ) + 1 );

        if( Player->GetCensors( ) == 1 )
            secs = m_UNM->m_CensorMuteFirstSeconds;
        else if( Player->GetCensors( ) == 2 )
            secs = m_UNM->m_CensorMuteSecondSeconds;
        else if( Player->GetCensors( ) > 2 )
            secs = m_UNM->m_CensorMuteExcessiveSeconds;

        m_CensorMutedSeconds.push_back( secs );
        m_CensorMutedTime.push_back( GetTime( ) );

        if( !IsMuted( name ) && !Player->GetMuted( ) )
            SendAllChat( gLanguage->PlayerMutedForBeingFoulMouthed( name, UTIL_ToString( secs ) ) );
    }
}

void CBaseGame::ProcessCensorMuted( )
{
    if( GetTime( ) - m_CensorMutedLastTime >= 5 )
    {
        m_CensorMutedLastTime = GetTime( );

        for( uint32_t i = 0; i < m_CensorMuted.size( ); )
        {
            // mute time has passed?

            if( m_CensorMutedLastTime > m_CensorMutedTime[i] + m_CensorMutedSeconds[i] )
            {
                CGamePlayer *Player = nullptr;
                Player = GetPlayerFromName( m_CensorMuted[i], false );

                if( !IsMuted( m_CensorMuted[i] ) && Player != nullptr && Player && !Player->GetMuted( ) )
                    SendAllChat( gLanguage->RemovedPlayerFromTheMuteList( m_CensorMuted[i] ) );

                m_CensorMuted.erase( m_CensorMuted.begin( ) + i );
                m_CensorMutedTime.erase( m_CensorMutedTime.begin( ) + i );
                m_CensorMutedSeconds.erase( m_CensorMutedSeconds.begin( ) + i );
            }
            else
                i++;
        }
    }
}

void CBaseGame::DelFromCensorMuted( string name )
{
    string nam = name;
    transform( nam.begin( ), nam.end( ), nam.begin( ), static_cast<int(*)(int)>(tolower) );

    for( uint32_t i = 0; i < m_CensorMuted.size( ); i++ )
    {
        if( m_CensorMuted[i] == nam )
        {
            m_CensorMuted.erase( m_CensorMuted.begin( ) + i );
            m_CensorMutedTime.erase( m_CensorMutedTime.begin( ) + i );
            m_CensorMutedSeconds.erase( m_CensorMutedSeconds.begin( ) + i );
            return;
        }
    }
}

bool CBaseGame::IsGameName( string name )
{
    for( vector<string> ::iterator i = m_GameNames.begin( ); i != m_GameNames.end( ); i++ )
    {
        if( name.find( *i ) != string::npos )
            return true;
    }

    return false;
}

void CBaseGame::AddGameName( string name )
{
    // add game name to names list

    for( vector<string> ::iterator i = m_GameNames.begin( ); i != m_GameNames.end( ); i++ )
    {
        if( *i == name )
            return;
    }

    m_GameNames.push_back( name );
}

void CBaseGame::AutoSetHCL( )
{
    // auto set HCL if map_defaulthcl is not empty

    string gameName = m_GameName;
    transform( gameName.begin( ), gameName.end( ), gameName.begin( ), static_cast<int(*)(int)>(tolower) );
    string m_Mode = string( );
    string m_Modes = string( );
    string m_Modes2 = string( );
    bool forceindota = ( m_UNM->m_forceautohclindota && m_DotA );

    if( m_UNM->m_autohclfromgamename || forceindota )
    {
        if( !m_Map->GetMapDefaultHCL( ).empty( ) || forceindota )
        {
            if( gameName.find( "-" ) != string::npos )
            {
                uint32_t j = 0;
                uint32_t k = 0;
                uint32_t i = 0;
                string mode = string( );

                while( j < gameName.length( ) - 1 && ( gameName.find( "-", j ) != string::npos ) )
                {
                    k = gameName.find( "-", j );
                    i = gameName.find( " ", k );

                    if( i == 0 )
                        i = gameName.length( ) - k + 1;
                    else
                        i = i - k;

                    mode = gameName.substr( k, i );

                    // keep the first mode separately to be used in HCL

                    if( m_Mode.empty( ) )
                        m_Mode = gameName.substr( k + 1, i - 1 );

                    m_Modes += " " + mode;

                    if( !m_Modes2.empty( ) )
                        m_Modes2 += " ";

                    m_Modes2 += mode;
                    j = k + i;
                }

                if( !m_Mode.empty( ) )
                {
                    CONSOLE_Print( "[UNM] autosetting HCL to [" + m_Mode + "]" );

                    if( m_Mode.size( ) <= m_Slots.size( ) )
                    {
                        string HCLChars = "abcdefghijklmnopqrstuvwxyz0123456789 -=,.";

                        if( m_Mode.find_first_not_of( HCLChars ) == string::npos )
                        {
                            SetHCL( m_Mode );

                            if( m_Mode != m_HCLCommandString )
                                SendAllChat( gLanguage->SettingHCL( GetHCL( ) ) );
                        }
                        else
                            SendAllChat( gLanguage->UnableToSetHCLInvalid( ) );
                    }
                    else
                        SendAllChat( gLanguage->UnableToSetHCLTooLong( ) );
                }
            }
            else
            {
                // no gamemode detected from gamename, disable map_defaulthcl

                SetHCL( string( ) );
            }
        }
    }
}

void CBaseGame::AddToReserved( string name, string server, unsigned char nr )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    // check that the user is not already reserved

    vector<unsigned char> ::iterator j = m_ReservedS.begin( );

    for( vector<string> ::iterator i = m_Reserved.begin( ); i != m_Reserved.end( ); i++ )
    {
        if( *i == name )
        {
            *j = nr;
            return;
        }

        j++;
    }

    m_Reserved.push_back( name );
    m_ReservedRealms.push_back( server );
    m_ReservedS.push_back( nr );

    // upgrade the user if they're already in the game

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        string NameLower = ( *i )->GetName( );
        transform( NameLower.begin( ), NameLower.end( ), NameLower.begin( ), static_cast<int(*)(int)>(tolower) );

        if( NameLower == name )
            ( *i )->SetReserved( true );
    }
}

void CBaseGame::DelFromReserved( string name )
{
    string nam = name;
    transform( nam.begin( ), nam.end( ), nam.begin( ), static_cast<int(*)(int)>(tolower) );
    uint32_t j = 0;

    for( vector<string> ::iterator i = m_Reserved.begin( ); i != m_Reserved.end( ); i++ )
    {
        if( *i == nam )
        {
            m_Reserved.erase( i );
            m_ReservedRealms.erase( m_ReservedRealms.begin( ) + j );
            m_ReservedS.erase( m_ReservedS.begin( ) + j );
            return;
        }

        j++;
    }

    // downgrade the user if they're already in the game

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        string NameLower = ( *i )->GetName( );
        transform( NameLower.begin( ), NameLower.end( ), NameLower.begin( ), static_cast<int(*)(int)>(tolower) );

        if( NameLower == nam )
            ( *i )->SetReserved( false );
    }
}

bool CBaseGame::IsBlacklisted( string blacklist, string name )
{
    transform( blacklist.begin( ), blacklist.end( ), blacklist.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    stringstream SS;
    string s;
    SS << blacklist;

    while( !SS.eof( ) )
    {
        SS >> s;

        if( name == s )
            return true;
    }

    return false;
}

void CBaseGame::DelBlacklistedName( string name )
{
    string blacklist = m_BannedNames;
    transform( blacklist.begin( ), blacklist.end( ), blacklist.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    stringstream SS;
    string tusers = string( );
    string luser;
    SS << blacklist;

    while( !SS.eof( ) )
    {
        SS >> luser;

        if( luser != name )
        {
            if( tusers.empty( ) )
                tusers = luser;
            else
                tusers += " " + luser;
        }
    }

    m_BannedNames = tusers;
}

bool CBaseGame::IsOwner( string name )
{
    string OwnerLower = m_OwnerName;
    transform( OwnerLower.begin( ), OwnerLower.end( ), OwnerLower.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    stringstream SS;
    string s;
    SS << OwnerLower;

    while( !SS.eof( ) )
    {
        SS >> s;

        if( name == s )
            return true;
    }

    return false;
}

bool CBaseGame::IsDefaultOwner( string name )
{
    string OwnerLower = m_DefaultOwner;
    transform( OwnerLower.begin( ), OwnerLower.end( ), OwnerLower.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    if( OwnerLower == name )
        return true;
    else
        return false;
}

string CBaseGame::CurrentOwners( )
{
    if( m_OwnerName.find( " " ) != string::npos )
    {
        string OwnerNamesFix = m_OwnerName;

        while( OwnerNamesFix.find( " " ) != string::npos )
            UTIL_Replace( OwnerNamesFix, " ", "\\|/@" );

        while( OwnerNamesFix.find( "\\|/@" ) != string::npos )
            UTIL_Replace( OwnerNamesFix, "\\|/@", ", " );

        return "Текущие владельцы игры: " + OwnerNamesFix;
    }
    else if( !m_OwnerName.empty( ) )
        return "Текущий владелец игры: " + m_OwnerName;
    else
        return "Данная игра не имеет владельца";
}

void CBaseGame::DelTempOwner( string name )
{
    string OwnerLower = m_OwnerName;
    transform( OwnerLower.begin( ), OwnerLower.end( ), OwnerLower.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    stringstream SS;
    string tusers = string( );
    string luser;
    SS << OwnerLower;

    while( !SS.eof( ) )
    {
        SS >> luser;
        if( luser != name )
        {
            if( tusers.empty( ) )
                tusers = luser;
            else
                tusers += " " + luser;
        }
    }

    m_OwnerName = tusers;
}

bool CBaseGame::IsSpoofOfline( string name )
{
    string NameSpoofOflineLower = m_NameSpoofOfline;
    transform( NameSpoofOflineLower.begin( ), NameSpoofOflineLower.end( ), NameSpoofOflineLower.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    stringstream SS;
    string s;
    SS << NameSpoofOflineLower;

    while( !SS.eof( ) )
    {
        SS >> s;

        if( name == s )
            return true;
    }

    return false;
}

void CBaseGame::DelSpoofOfline( string name )
{
    string NameSpoofOflineLower = m_NameSpoofOfline;
    transform( NameSpoofOflineLower.begin( ), NameSpoofOflineLower.end( ), NameSpoofOflineLower.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    stringstream SS;
    string tusers = string( );
    string luser;
    SS << NameSpoofOflineLower;

    while( !SS.eof( ) )
    {
        SS >> luser;

        if( luser != name )
        {
            if( tusers.empty( ) )
                tusers = luser;
            else
                tusers += " " + luser;
        }
    }

    m_NameSpoofOfline = tusers;
}

bool CBaseGame::IsReserved( string name, string server )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );

    for( uint32_t i = 0; i < m_Reserved.size( ); i++ )
    {
        if( m_Reserved[i] == name && ( m_ReservedRealms[i] == server || !m_UNM->IsBnetServer( m_ReservedRealms[i] ) || !m_UNM->IsBnetServer( server ) ) )
            return true;
    }

    return false;
}

unsigned char CBaseGame::ReservedSlot( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    vector<unsigned char>::iterator j = m_ReservedS.begin( );

    for( vector<string> ::iterator i = m_Reserved.begin( ); i != m_Reserved.end( ); i++ )
    {
        if( *i == name )
            return *j;

        j++;
    }

    return 255;
}

void CBaseGame::ResetReservedSlot( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    vector<unsigned char> ::iterator j = m_ReservedS.begin( );

    for( vector<string> ::iterator i = m_Reserved.begin( ); i != m_Reserved.end( ); i++ )
    {
        if( *i == name )
        {
            *j = 255;
            return;
        }

        j++;
    }

}

bool CBaseGame::IsDownloading( )
{
    // returns true if at least one player is downloading the map

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( ( *i )->GetDownloadStarted( ) && !( *i )->GetDownloadFinished( ) )
            return true;
    }

    return false;
}


uint32_t CBaseGame::DownloaderNr( string name )
{
    uint32_t result = 1;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( ( *i )->GetDownloadStarted( ) && !( *i )->GetDownloadFinished( ) )
        {
            if( ( *i )->GetName( ) != name )
                result++;
            else return result;
        }
    }

    return result;

}

uint32_t CBaseGame::CountDownloading( )
{
    uint32_t result = 0;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( ( *i )->GetDownloadStarted( ) && !( *i )->GetDownloadFinished( ) )
            result++;
    }

    return result;
}

uint32_t CBaseGame::ShowDownloading( )
{
    if( GetTime( ) - m_DownloadInfoTime < m_UNM->m_ShowDownloadsInfoTime || !m_UNM->m_ShowDownloadsInfo )
        return 0;

    m_DownloadInfoTime = GetTime( );

    uint32_t result = 0;
    string Down;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( ( *i )->GetDownloadStarted( ) && !( *i )->GetDownloadFinished( ) )
        {
            if( Down.length( ) > 0 )
                Down += ", ";

            Down += ( *i )->GetName( ) + ": " + ( *i )->GetDownloadInfo( );
            result++;
        }
    }

    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] " + Down, 6, m_CurrentGameCounter );
    SendAllChat( Down );
    return result;
}

void CBaseGame::StartCountDown( bool force )
{
    if( !m_CountDownStarted )
    {
        if( force )
        {
            m_CountDownStarted = true;

            if( m_NormalCountdown )
            {
                m_CountDownCounter = 5;
                SendAll( m_Protocol->SEND_W3GS_COUNTDOWN_START( ) );
            }
            else if( m_UNM->m_CountDownMethod == 1 )
                m_CountDownCounter = 200;
            else
                m_CountDownCounter = m_UNM->m_CountDownStartForceCounter;
        }
        else
        {
            // check if the HCL command string is short enough

            if( m_HCLCommandString.size( ) > GetSlotsOccupied( ) )
            {
                SendAllChat( gLanguage->TheHCLIsTooLongUseForceToStart( string( 1, m_UNM->m_CommandTrigger ) ) );
                return;
            }

            // check if everyone has the map

            string StillDownloading;

            for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
            {
                if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && ( *i ).GetComputer( ) == 0 && ( *i ).GetDownloadStatus( ) != 100 )
                {
                    CGamePlayer *Player = GetPlayerFromPID( ( *i ).GetPID( ) );

                    if( Player )
                    {
                        if( StillDownloading.empty( ) )
                            StillDownloading = Player->GetName( );
                        else
                            StillDownloading += ", " + Player->GetName( );
                    }
                }
            }

            if( !StillDownloading.empty( ) )
                if( m_UNM->m_StillDownloadingPrint )
                    SendAllChat( gLanguage->PlayersStillDownloading( StillDownloading ) );

            // check if everyone is spoof checked

            string NotSpoofChecked;

            if( m_UNM->m_SpoofChecks > 0 )
            {
                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    if( !( *i )->GetSpoofed( ) )
                    {
                        if( NotSpoofChecked.empty( ) )
                            NotSpoofChecked = ( *i )->GetName( );
                        else
                            NotSpoofChecked += ", " + ( *i )->GetName( );
                    }
                }

                if( !NotSpoofChecked.empty( ) )
                    if( m_UNM->m_NotSpoofCheckedPrint )
                        SendAllChat( gLanguage->PlayersNotYetSpoofChecked( NotSpoofChecked ) );
            }

            // check if everyone has been pinged enough (3 times by default) that the autokicker would have kicked them by now
            // see function EventPlayerPongToHost for the autokicker code

            string NotPinged;
            uint32_t NumReqPings = m_UNM->m_NumRequiredPingsPlayers;
            string NumRequiredPings = UTIL_ToString( NumReqPings );

            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            {
                if( ( *i )->GetNumPings( ) < m_UNM->m_NumRequiredPingsPlayers )
                {
                    if( NotPinged.empty( ) )
                        NotPinged = ( *i )->GetName( );
                    else
                        NotPinged += ", " + ( *i )->GetName( );
                }
            }

            if( !NotPinged.empty( ) )
            {
                if( m_UNM->m_NotPingedPrint )
                {
                    if( NumReqPings >= 10 && NumReqPings <= 20 )
                        SendAllChat( gLanguage->PlayersNotYetPingedRus( UTIL_ToString( m_UNM->m_NumRequiredPingsPlayers ), NotPinged ) );
                    else if( NumRequiredPings.substr( NumRequiredPings.length( ) - 1 ) == "2" || NumRequiredPings.substr( NumRequiredPings.length( ) - 1 ) == "3" || NumRequiredPings.substr( NumRequiredPings.length( ) - 1 ) == "4" )
                        SendAllChat( gLanguage->PlayersNotYetPinged( UTIL_ToString( m_UNM->m_NumRequiredPingsPlayers ), NotPinged ) );
                    else
                        SendAllChat( gLanguage->PlayersNotYetPingedRus( UTIL_ToString( m_UNM->m_NumRequiredPingsPlayers ), NotPinged ) );
                }
            }

            // if no problems found start the game

            if( StillDownloading.empty( ) && NotSpoofChecked.empty( ) && NotPinged.empty( ) )
            {
                m_CountDownStarted = true;

                if( m_NormalCountdown )
                {
                    m_CountDownCounter = 5;
                    SendAll( m_Protocol->SEND_W3GS_COUNTDOWN_START( ) );
                }
                else if( m_UNM->m_CountDownMethod == 1 )
                    m_CountDownCounter = 200;
                else
                    m_CountDownCounter = m_UNM->m_CountDownStartCounter;
            }
        }
    }
}

void CBaseGame::StartCountDownAuto( bool requireSpoofChecks )
{
    if( !m_CountDownStarted )
    {
        // check if enough players are present

        ReCalculateTeams( );
        uint32_t PNr = GetNumHumanPlayers( );

        if( PNr < m_AutoStartPlayers )
        {
            if( m_UNM->m_PlayerBeforeStartPrint && m_AutoStartPlayers > 0 )
            {
                if( m_UNM->m_PlayerBeforeStartPrintDelay > m_ActuallyPlayerBeforeStartPrintDelay )
                    m_ActuallyPlayerBeforeStartPrintDelay++;
                else
                {
                    string s = gLanguage->WaitingForPlayersBeforeAutoStartVS( UTIL_ToString( m_AutoStartPlayers ), UTIL_ToString( m_AutoStartPlayers - PNr ), string( 1, m_UNM->m_CommandTrigger ) );
                    m_ActuallyPlayerBeforeStartPrintDelay = 1;
                    SendAllChat( s );
                }
            }

            return;
        }

        // check if everyone has the map

        string StillDownloading;

        for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
        {
            if( ( *i ).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && ( *i ).GetComputer( ) == 0 && ( *i ).GetDownloadStatus( ) != 100 )
            {
                CGamePlayer *Player = GetPlayerFromPID( ( *i ).GetPID( ) );

                if( Player )
                {
                    if( StillDownloading.empty( ) )
                        StillDownloading = Player->GetName( );
                    else
                        StillDownloading += ", " + Player->GetName( );
                }
            }
        }

        if( !StillDownloading.empty( ) )
        {
            if( m_UNM->m_StillDownloadingASPrint )
            {
                if( m_UNM->m_StillDownloadingASPrintDelay > m_ActuallyStillDownloadingASPrintDelay )
                    m_ActuallyStillDownloadingASPrintDelay++;
                else
                {
                    m_ActuallyStillDownloadingASPrintDelay = 1;
                    SendAllChat( gLanguage->PlayersStillDownloading( StillDownloading ) );
                }
            }

            return;
        }

        // check if everyone is spoof checked

        string NotSpoofChecked;

        if( requireSpoofChecks )
        {
            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            {
                if( !( *i )->GetSpoofed( ) )
                {
                    if( NotSpoofChecked.empty( ) )
                        NotSpoofChecked = ( *i )->GetName( );
                    else
                        NotSpoofChecked += ", " + ( *i )->GetName( );
                }
            }

            if( !NotSpoofChecked.empty( ) )
            {
                if( m_UNM->m_NotSpoofCheckedASPrint )
                {
                    if( m_UNM->m_NotSpoofCheckedASPrintDelay > m_ActuallyNotSpoofCheckedASPrintDelay )
                        m_ActuallyNotSpoofCheckedASPrintDelay++;
                    else
                    {
                        m_ActuallyNotSpoofCheckedASPrintDelay = 1;
                        SendAllChat( gLanguage->PlayersNotYetSpoofChecked( NotSpoofChecked ) );
                    }
                }

                return;
            }
        }

        // check if everyone has been pinged enough (3 times by default) that the autokicker would have kicked them by now
        // see function EventPlayerPongToHost for the autokicker code

        string NotPinged;

        for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
        {
            if( ( *i )->GetNumPings( ) < m_UNM->m_NumRequiredPingsPlayersAS )
            {
                if( NotPinged.empty( ) )
                    NotPinged = ( *i )->GetName( );
                else
                    NotPinged += ", " + ( *i )->GetName( );
            }
        }

        if( !NotPinged.empty( ) )
        {
            if( m_UNM->m_NotPingedASPrint )
            {
                if( m_UNM->m_NotPingedASPrintDelay > m_ActuallyNotPingedASPrintDelay )
                    m_ActuallyNotPingedASPrintDelay++;
                else
                {
                    m_ActuallyNotPingedASPrintDelay = 1;
                    SendAllChat( gLanguage->PlayersNotYetPingedAutoStart( NotPinged ) );
                }
            }

            return;
        }

        // if no problems found start the game

        if( StillDownloading.empty( ) && NotSpoofChecked.empty( ) && NotPinged.empty( ) )
        {
            if( m_FPEnable && !m_FakePlayers.empty( ) )
                DeleteFakePlayer( );

            m_CountDownStarted = true;

            if( m_NormalCountdown )
            {
                m_CountDownCounter = 5;
                SendAll( m_Protocol->SEND_W3GS_COUNTDOWN_START( ) );
            }
            else if( m_UNM->m_CountDownMethod == 1 )
                m_CountDownCounter = 200;
            else
                m_CountDownCounter = m_UNM->m_CountDownStartCounter;
        }
    }
}

void CBaseGame::StopPlayers( string reason )
{
    // disconnect every player and set their left reason to the passed string
    // we use this function when we want the code in the Update function to run before the destructor (e.g. saving players to the database)
    // therefore calling this function when m_GameLoading || m_GameLoaded is roughly equivalent to setting m_Exiting = true
    // the only difference is whether the code in the Update function is executed or not

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        ( *i )->SetDeleteMe( true );
        ( *i )->SetLeftReason( reason );
        ( *i )->SetLeftCode( PLAYERLEAVE_LOST );
    }
}

void CBaseGame::StopLaggers( string reason )
{
    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( ( *i )->GetLagging( ) )
        {
            ( *i )->SetDeleteMe( true );
            ( *i )->SetLeftReason( reason );
            ( *i )->SetLeftCode( PLAYERLEAVE_DISCONNECT );
        }
    }
}

void CBaseGame::StopLagger( string reason )
{
    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( ( *i )->GetLagging( ) && !IsOwner( ( *i )->GetName( ) ) )
        {
            ( *i )->SetDeleteMe( true );
            ( *i )->SetLeftReason( reason );
            ( *i )->SetLeftCode( PLAYERLEAVE_DISCONNECT );
        }
        else if( ( *i )->GetLagging( ) )
            SendAllChat( *i, "Owners can't be dropped!" );
    }
}

void CBaseGame::CreateVirtualHost( )
{
    if( m_VirtualHostPID != 255 )
        return;

    m_VirtualHostPID = GetNewPID( );
    BYTEARRAY IP;
    IP.push_back( 0 );
    IP.push_back( 0 );
    IP.push_back( 0 );
    IP.push_back( 0 );
    SendAll( m_Protocol->SEND_W3GS_PLAYERINFO( m_VirtualHostPID, m_VirtualHostName, IP, IP, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
}

void CBaseGame::DeleteVirtualHost( )
{
    if( m_VirtualHostPID == 255 )
        return;

    SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( m_VirtualHostPID, PLAYERLEAVE_LOBBY, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
    m_VirtualHostPID = 255;
}

void CBaseGame::AddBannedNames( string name )
{
    m_BannedNames += " " + name;
}

void CBaseGame::CreateObsPlayer( string name )
{
    if( m_ObsPlayerPID != 255 )
        return;
    else if( m_GetMapType.find( "noobs" ) != string::npos )
    {
        SendAllChat( "Для этой карты добавление obs-player'ов запрещено!" );
        return;
    }

    unsigned char SID = GetEmptySlotForObsPlayer( );
    string s;

    if( !name.empty( ) )
        s = UTIL_SubStrFix( name, 0, 15 );

    if( s.empty( ) )
        s = UTIL_SubStrFix( m_UNM->m_ObsPlayerName, 0, 15 );

    if( s.empty( ) )
        s = "Observer";

    m_ObsPlayerName = s;

    if( SID < m_Slots.size( ) )
    {
        if( GetSlotsOccupied( ) >= m_Slots.size( ) - 1 )
            DeleteVirtualHost( );

        m_ObsPlayerPID = GetNewPID( );
        BYTEARRAY IP;
        IP.push_back( 0 );
        IP.push_back( 0 );
        IP.push_back( 0 );
        IP.push_back( 0 );
        SendAll( m_Protocol->SEND_W3GS_PLAYERINFO( m_ObsPlayerPID, m_ObsPlayerName, IP, IP, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
        m_Slots[SID] = CGameSlot( m_ObsPlayerPID, 100, SLOTSTATUS_OCCUPIED, 0, m_Slots[SID].GetTeam( ), m_Slots[SID].GetColour( ), m_Slots[SID].GetRace( ) );
        SendAllSlotInfo( );
    }
}

void CBaseGame::DeleteObsPlayer( )
{
    if( m_ObsPlayerPID == 255 )
        return;

    for( unsigned char i = 0; i < m_Slots.size( ); i++ )
    {
        if( m_Slots[i].GetPID( ) == m_ObsPlayerPID )
            m_Slots[i] = CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, m_Slots[i].GetTeam( ), m_Slots[i].GetColour( ), m_Slots[i].GetRace( ) );
    }

    SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( m_ObsPlayerPID, PLAYERLEAVE_LOBBY, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
    SendAllSlotInfo( );
    m_ObsPlayerPID = 255;
    m_ObsPlayerName = m_UNM->m_ObsPlayerName;
}

void CBaseGame::CreateFakePlayer( string name )
{
    unsigned char SID = GetEmptySlotForFakePlayers( );

    if( m_FakePlayers.size( ) < 12 && SID < m_Slots.size( ) && SID != 255 )
    {
        if( GetSlotsOccupied( ) >= m_Slots.size( ) - 1 )
            DeleteVirtualHost( );

        unsigned char FakePlayerPID = GetNewPID( );
        BYTEARRAY IP;
        IP.push_back( 0 );
        IP.push_back( 0 );
        IP.push_back( 0 );
        IP.push_back( 0 );
        string s;

        if( !name.empty( ) )
            s = UTIL_SubStrFix( name, 0, 15 );

        if( s.empty( ) )
            s = UTIL_SubStrFix( m_UNM->GetFPName( ), 0, 15 );

        if( s.empty( ) )
            s = "Wizard[" + UTIL_ToString( FakePlayerPID ) + "]";

        m_UNM->m_FPNamesLast.push_back( s );
        SendAll( m_Protocol->SEND_W3GS_PLAYERINFO( FakePlayerPID, s, IP, IP, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
        m_Slots[SID] = CGameSlot( FakePlayerPID, 100, SLOTSTATUS_OCCUPIED, 0, m_Slots[SID].GetTeam( ), m_Slots[SID].GetColour( ), m_Slots[SID].GetRace( ) );
        BYTEARRAY fpaction;
        m_FakePlayers.push_back( FakePlayerPID );
        m_FakePlayersNames.push_back( s );
        m_FakePlayersActSelection.push_back( fpaction );
        m_FakePlayersOwner.push_back( string( ) );
        m_FakePlayersStatus.push_back( 0 );
        SendAllSlotInfo( );
    }
}

void CBaseGame::CreateInitialFakePlayers( )
{
    uint32_t b = 0;

    if( GetNumHumanPlayers( ) < 3 )
    {
        if( GetSlotsOpen( ) + GetNumPlayers( ) > 7 )
        {
            if( m_GetMapNumTeams < 2 )
                b = 4; // we allows only a maximum of 4 fakeplayers when it's a (>=8) game
            else
                b = 3;
        }
        else
            b = 3; // we allows only a maximum of 3 fakeplayers as to prevent a trash lobby
    }
    else if( GetNumHumanPlayers( ) < 4 )
        b = 2; // we allows only a maximum of 2 fakeplayers as to prevent a trash lobby

    if( ( GetSlotsOpen( ) + GetNumPlayers( ) > 9 ) && m_GetMapNumTeams < 2 && GetNumHumanPlayers( ) < 4 )
        b = 4; // we allows only a maximum of 4 fakeplayers when it's a (>=10) game

    while( GetSlotsOpen( ) > 2 && m_FakePlayers.size( ) < b && GetNumHumanPlayers( ) < 4 )
        CreateFakePlayer( string( ) );
}

void CBaseGame::DeleteAFakePlayer( )
{
    if( m_FakePlayers.empty( ) )
        return;

    if( !m_GameLoaded )
    {
        for( unsigned char i = 0; i < m_Slots.size( ); ++i )
        {
            uint32_t fpnubmer = 0;

            for( vector<unsigned char> ::iterator j = m_FakePlayers.begin( ); j != m_FakePlayers.end( ); ++j )
            {
                if( m_Slots[i].GetPID( ) == *j )
                {
                    m_Slots[i] = CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, m_Slots[i].GetTeam( ), m_Slots[i].GetColour( ), m_Slots[i].GetRace( ) );
                    SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( *j, PLAYERLEAVE_LOBBY, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
                    m_FakePlayers.erase( j );
                    m_FakePlayersNames.erase( m_FakePlayersNames.begin( ) + fpnubmer );
                    m_FakePlayersActSelection.erase( m_FakePlayersActSelection.begin( ) + fpnubmer );
                    m_FakePlayersOwner.erase( m_FakePlayersOwner.begin( ) + fpnubmer );
                    m_FakePlayersStatus.erase( m_FakePlayersStatus.begin( ) + fpnubmer );
                    SendAllSlotInfo( );

                    if( m_UNM->m_FPNamesLast.size( ) > 0 )
                        m_UNM->m_FPNamesLast.resize( m_UNM->m_FPNamesLast.size( ) - 1 );

                    return;
                }

                fpnubmer++;
            }
        }
    }
    else
    {
        SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( m_FakePlayers.back( ), PLAYERLEAVE_LOST, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );
        m_FakePlayers.pop_back( );
        m_FakePlayersNames.pop_back( );
        m_FakePlayersActSelection.pop_back( );
        m_FakePlayersOwner.pop_back( );
        m_FakePlayersStatus.pop_back( );
    }
}

void CBaseGame::DeleteFakePlayer( )
{
    if( m_FakePlayers.empty( ) )
        return;

    if( !m_GameLoaded )
    {
        for( unsigned char i = 0; i < m_Slots.size( ); ++i )
        {
            for( vector<unsigned char> ::iterator j = m_FakePlayers.begin( ); j != m_FakePlayers.end( ); ++j )
            {
                if( m_Slots[i].GetPID( ) == *j )
                {
                    m_Slots[i] = CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, m_Slots[i].GetTeam( ), m_Slots[i].GetColour( ), m_Slots[i].GetRace( ) );
                    SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( *j, PLAYERLEAVE_LOBBY, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

                    if( m_UNM->m_FPNamesLast.size( ) > 0 )
                        m_UNM->m_FPNamesLast.resize( m_UNM->m_FPNamesLast.size( ) - 1 );
                }
            }
        }

        m_FakePlayers.clear( );
        m_FakePlayersNames.clear( );
        m_FakePlayersActSelection.clear( );
        m_FakePlayersOwner.clear( );
        m_FakePlayersStatus.clear( );
        SendAllSlotInfo( );
    }
    else
    {
        for( vector<unsigned char> ::iterator j = m_FakePlayers.begin( ); j != m_FakePlayers.end( ); ++j )
            SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( *j, PLAYERLEAVE_LOST, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ) );

        m_FakePlayers.clear( );
        m_FakePlayersNames.clear( );
        m_FakePlayersActSelection.clear( );
        m_FakePlayersOwner.clear( );
        m_FakePlayersStatus.clear( );
    }
}

uint32_t CBaseGame::GetGameNr( )
{
    uint32_t GameNr = 0;

    for( vector<CBaseGame *> ::iterator i = m_UNM->m_Games.begin( ); i != m_UNM->m_Games.end( ); i++ )
    {
        if( ( *i )->GetCreationTime( ) == GetCreationTime( ) && ( *i )->GetGameName( ) == GetGameName( ) )
            break;

        GameNr++;
    }

    return GameNr;
}

string CBaseGame::GetOwnerName( )
{
    string ownername = m_OwnerName;

    while( !ownername.empty( ) && ownername.front( ) == ' ' )
        ownername.erase( ownername.begin( ) );

    string::size_type Start = ownername.find( " " );

    if( Start != string::npos )
        ownername = ownername.substr( 0, Start );

    if( !ownername.empty( ) )
        return ownername;
    else
        return m_DefaultOwner;
}

string CBaseGame::GetLastOwnerName( )
{
    string ownername = m_OwnerName;

    while( !ownername.empty( ) && ownername.back( ) == ' ' )
        ownername.erase( ownername.end( ) - 1 );

    if( ownername.find( " " ) != string::npos )
    {
        string::size_type Start = ownername.rfind( " " );
        ownername = ownername.substr( Start + 1 );
    }

    return m_OwnerName;
}

bool CBaseGame::IsClanFullMember( string name, string server )
{
    for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
    {
        if( ( server.empty( ) || ( *i )->GetServer( ) == server ) && ( *i )->IsClanFullMember( name ) )
            return true;
    }

    return false;
}

string CBaseGame::CustomReason( string reason )
{
    if( reason.find_first_not_of( " " ) == string::npos || reason == "l" || reason == "noob" || reason == "leaver" || reason == "i" || reason == "r" || reason == "lag" || reason == "feeder" || reason == "lagger" || reason == "idiot" || reason == "f" || reason == "n" || reason == "mh" || reason == "m" )
    {
        if( reason.find_first_not_of( " " ) == string::npos )
            reason = "Причина не указана";
        else if( reason == "leaver" )
            reason = "Leaver";
        else if( reason == "idiot" )
            reason = "Idiot";
        else if( reason == "rager" )
            reason = "Rager";
        else if( reason == "lag" || reason == "lagger" )
            reason = "Lagger";
        else if( reason == "feed" || reason == "feeder" )
            reason = "Feeder";
        else if( reason == "noob" )
            reason = "Noob";
        else if( reason == "mh" )
            reason = "Map Hacker";
    }

    return reason;
}

string CBaseGame::GetCurrentDate( )
{    
    struct tm * timeinfo;
    char buffer[80];
    time_t Now = time( nullptr );
    timeinfo = localtime( &Now );
    strftime( buffer, 80, "%d-%m-%Y", timeinfo );
    string sDate = buffer;
    return sDate;
}

string CBaseGame::GetGameInfo( )
{
    string sMsg = string( );
    uint32_t GameNr = GetGameNr( ) + 1;

    if( !m_GameLoading && !m_GameLoaded )
        sMsg = "Lobby: " + m_UNM->IncGameNrDecryption( m_GameName ) + " (" + UTIL_ToString( GetSlotsOccupied( ) ) + "/" + UTIL_ToString( m_Slots.size( ) ) + ") - " + m_DefaultOwner;
    else
    {
        struct tm * timeinfo;
        char buffer[80];
        string sDate;
        time_t Now = time( nullptr );
        timeinfo = localtime( &Now );
        strftime( buffer, 80, "%B %Y", timeinfo );
        sDate = buffer;

        // time in minutes since game loaded
        float iTime = (float)( GetTime( ) - m_GameLoadedTime ) / 60;

        if( !m_GameLoaded )
            iTime = 0;

        if( m_DotA )
        {
            if( iTime > 2 )
                iTime -= 2;
            else
                iTime = 1;
        }

        string sScore = string( );
        string sTime = UTIL_ToString( iTime );

        if( sTime.length( ) < 2 )
            sTime = "0" + sTime;

        sTime = sTime + "m : ";
        string sTeam = string( );
        string st1 = string( );
        string st2 = string( );

        if( m_GetMapNumTeams == 2 )
        {
            st1 = UTIL_ToString( m_Team1 );
            st2 = UTIL_ToString( m_Team2 );
            sTeam = st1 + "v" + st2 + " : ";
        }

        string s = m_UNM->GetRehostChar( );
        string sAdmin = m_DefaultOwner + " : ";
        string sGameNr = s[0] + UTIL_ToString( GameNr ) + ": [ ";
        string sGameN = GetGameName( );
        sMsg = sGameNr + sTime + sTeam + sScore + sAdmin + sGameN + " ]";
    }

    return sMsg;
}

void CBaseGame::SetDynamicLatency( )
{
    // set dynamic latency with refresh rate m_UNM->m_DynamicLatencyRefreshRate (every 5 seconds by default)

    if( !m_GameLoaded || GetTime( ) - m_DynamicLatencyLastTicks < m_UNM->m_DynamicLatencyRefreshRate )
        return;

    m_DynamicLatencyLastTicks = GetTime( );

    // get the highest ping in the game

    uint32_t highestping = 0;
    uint32_t ping = 0;

    for( uint32_t i = 0; i < m_Players.size( ); i++ )
    {
        if( m_Players[i]->GetNumPings( ) > 0 )
        {
            ping = m_Players[i]->GetPing( true );

            if( ping > highestping )
                highestping = ping;
        }
    }

    m_Latency = highestping;

    if( m_UNM->m_DynamicLatencyPercentPingMax != 0 )
    {
        double dynamiclatencypercentpingmax = 0.01 * m_UNM->m_DynamicLatencyPercentPingMax;
        m_Latency = static_cast<uint32_t>( highestping * dynamiclatencypercentpingmax );
    }

    if( m_UNM->m_DynamicLatencyDifferencePingMax != 0 )
    {
        if( m_UNM->m_DynamicLatencyDifferencePingMax >= m_Latency )
            m_Latency = 0;
        else
            m_Latency = m_Latency - m_UNM->m_DynamicLatencyDifferencePingMax;
    }

    if( m_DynamicLatencyLagging )
    {
        m_Latency = m_Latency + m_UNM->m_DynamicLatencyAddIfLag;
        m_DynamicLatencyLagging = false;
    }

    if( m_DynamicLatencyBotLagging )
    {
        m_Latency = m_Latency + m_UNM->m_DynamicLatencyAddIfBotLag;
        m_DynamicLatencyBotLagging = false;
    }

    if( m_UNM->m_DynamicLatencyMaxSync > 0 && m_DynamicLatencyMaxSync >= m_UNM->m_DynamicLatencyMaxSync )
        m_Latency = m_Latency + m_UNM->m_DynamicLatencyAddIfMaxSync;

    if( m_Latency < m_UNM->m_DynamicLatencyLowestAllowed )
        m_Latency = m_UNM->m_DynamicLatencyLowestAllowed;
    else if( m_UNM->m_DynamicLatencyHighestAllowed != 0 && m_Latency > m_UNM->m_DynamicLatencyHighestAllowed )
        m_Latency = m_UNM->m_DynamicLatencyHighestAllowed;

    else if( m_Latency == 0 )
        m_DynamicLatency = 1;
    else
        m_DynamicLatency = m_Latency;

    if( m_UNM->m_DynamicLatencyConsolePrint != 0 && GetTime( ) - m_DynamicLatencyConsolePrint >= m_UNM->m_DynamicLatencyConsolePrint )
    {
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] " + gLanguage->SettingLatencyTo( UTIL_ToString( m_Latency ) ), 6, m_CurrentGameCounter );
        m_DynamicLatencyConsolePrint = GetTime( );
    }

    return;
}

bool CBaseGame::ValidPID( unsigned char PID )
{
    if( m_VirtualHostPID == PID )
        return true;
    else if( m_ObsPlayerPID == PID )
        return true;

    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( !( *i )->GetLeftMessageSent( ) && ( *i )->GetPID( ) == PID )
            return true;
    }

    for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
    {
        if( m_FakePlayers[i] == PID )
            return true;
    }

    return false;
}

CGamePlayer *CBaseGame::GetPlayerFromPID2( unsigned char PID )
{
    for( vector<CGamePlayer *>::iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    {
        if( ( *i )->GetPID( ) == PID )
            return *i;
    }

    for( vector<CGamePlayer *>::iterator i = m_DeletedPlayers.begin( ); i != m_DeletedPlayers.end( ); ++i )
    {
        if( ( *i )->GetPID( ) == PID )
            return *i;
    }

    return nullptr;
}

CGamePlayer *CBaseGame::GetPlayerFromSID2( unsigned char SID )
{
    if( SID < m_Slots.size( ) )
        return GetPlayerFromPID2( m_Slots[SID].GetPID( ) );

    return nullptr;
}

void CBaseGame::CommandLobbyBan( CGamePlayer *player, string Payload )
{
    if( Payload.empty( ) )
    {
        SendChat( player->GetPID( ), "Укажите ник юзера, которому нужно выдать временный бан." );
        return;
    }

    uint32_t Matches = 0;
    CGamePlayer *LastMatch = nullptr;

    if( Payload.size( ) < 3 && UTIL_ToUInt32( Payload ) > 0 && UTIL_ToUInt32( Payload ) < 13 )
    {
        LastMatch = GetPlayerFromSID( static_cast<unsigned char>( UTIL_ToUInt32( Payload ) - 1 ) );

        if( LastMatch )
            Matches = GetPlayerFromNamePartial( LastMatch->GetName( ), &LastMatch );
        else
        {
            SendChat( player->GetPID( ), "Слот [" + Payload + "] пуст" );
            return;
        }
    }
    else
        Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

    if( Matches == 0 )
        SendChat( player->GetPID( ), "Невозможно дать временный бан игроку - [" + Payload + "]. Значение не найдено." ); // Unable to kick no matches found
    else if( Matches == 1 )
    {
        if( LastMatch->GetName( ) == player->GetName( ) )
        {
            SendChat( player->GetPID( ), "Нельзя дать временный бан самому себу!" ); // admin can't kick yourself
            return;
        }
        else if( IsOwner( LastMatch->GetName( ) ) )
        {
            SendChat( player->GetPID( ), "Нельзя дать временный бан владельцу игры!" ); // admin can't kick owner game
            return;
        }

        LastMatch->SetDeleteMe( true );
        LastMatch->SetLeftReason( "Кикнут и временно забанен игроком [" + player->GetName( ) + "]" );

        if( !m_GameLoading && !m_GameLoaded )
            LastMatch->SetLeftCode( PLAYERLEAVE_LOBBY );
        else
            LastMatch->SetLeftCode( PLAYERLEAVE_LOST );

        if( !m_GameLoading && !m_GameLoaded )
        {
            OpenSlot( GetSIDFromPID( LastMatch->GetPID( ) ), false );
            AddBannedNames( LastMatch->GetName( ) );
        }
    }
    else
        SendChat( player->GetPID( ), "Невозможно дать временный бан игроку - [" + Payload + "]. Найдено более одного значения." ); // UnableToKickFoundMoreThanOneMatch
}

void CBaseGame::SetGameName( string nGameName )
{
    if( m_GameName != nGameName )
    {
        m_GameName = nGameName;
        m_UNM->m_CurrentGameName.clear( );
    }
}

void CBaseGame::GameRefreshOnUNMServer( CBNET *bnet )
{
    if( ( bnet->m_RefreshDelay == 0 || GetTicks( ) - m_LastRefreshTime >= bnet->m_RefreshDelay ) && m_GameCounter > 0 && m_RehostTime > 0 )
    {
        bnet->QueueGameRefreshUNM( m_GameCounter, m_GameLoading, m_GameLoaded, m_HostPort, m_MapWidth, m_MapHeight, m_MapGameType, m_MapGameFlags, m_MapCRC, m_HostCounter, m_EntryKey, GetTime( ) - m_CreationTime, m_UNM->IncGameNrDecryption( m_GameName ), GetOwnerName( ), m_GetMapPath, "UNM Bot", bnet->m_ExternalIP, string( ), "Для " + UTIL_ToString( GetSlotsOpen( ) + GetNumHumanPlayers( ) ) + " игроков", string( ), m_GetMapName, "Без статистики", GetSlotsOpen( ), m_Slots, m_Players );
        m_LastRefreshTime = GetTicks( );
    }
}
