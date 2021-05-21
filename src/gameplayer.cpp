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
#include "language.h"
#include "socket.h"
#include "commandpacket.h"
#include "bnet.h"
#include "map.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "gpsprotocol.h"
#include "game_base.h"

using namespace std;

extern CLanguage *gLanguage;

//
// CPotentialPlayer
//

CPotentialPlayer::CPotentialPlayer( CGameProtocol *nProtocol, CBaseGame *nGame, CTCPSocket *nSocket ) : m_Protocol( nProtocol ), m_Game( nGame ), m_Socket( nSocket ), m_DeleteMe( false ), m_Error( false ), m_IncomingJoinPlayer( nullptr ), m_Banned( false ), m_ConnectionState( 0 ), m_ConnectionTime( GetTicks( ) )
{
	if( nSocket )
		m_CachedIP = nSocket->GetIPString( );
	else
		m_CachedIP = string( );

    m_ExternalIPString = string( );
	m_Protocol = nProtocol;
	m_Game = nGame;
	m_Socket = nSocket;
	m_DeleteMe = false;
	m_Error = false;
	m_IncomingJoinPlayer = nullptr;
	m_LAN = false;
	m_LANSet = false;
	m_Name = string( );
}

CPotentialPlayer::~CPotentialPlayer( )
{
	if( m_Socket )
		delete m_Socket;

	while( !m_Packets.empty( ) )
	{
		delete m_Packets.front( );
		m_Packets.pop( );
	}

	delete m_IncomingJoinPlayer;
    m_IncomingJoinPlayer = nullptr;
	m_Protocol = nullptr;
	m_Game = nullptr;
	m_Socket = nullptr;
}

BYTEARRAY CPotentialPlayer::GetExternalIP( )
{
    if( !m_ExternalIP.empty( ) )
        return m_ExternalIP;

    unsigned char Zeros[] = { 0, 0, 0, 0 };
    BYTEARRAY IP;

    if( m_Socket )
    {
        IP = m_Socket->GetIP( );

        if( !m_LANSet )
        {
            bool local = false;

            if( IP.size( ) >= 2 )
            {
                if( IP[0] == 10 )
                    local = true;
                else if( IP[0] == 192 && IP[1] == 168 )
                    local = true;
                else if( IP[0] == 169 && IP[1] == 254 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 16 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 17 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 18 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 19 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 20 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 21 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 22 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 23 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 24 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 25 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 26 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 27 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 28 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 29 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 30 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 31 )
                    local = true;
            }

            if( UTIL_IsLocalIP( IP, m_Game->m_UNM->m_LocalAddresses ) || UTIL_IsLanIP( IP ) )
                local = true;

            m_LANSet = true;
            m_LAN = local;
        }

        m_ExternalIP = IP;
        return IP;
    }

    return UTIL_CreateByteArray( Zeros, 4 );
}

string CPotentialPlayer::GetExternalIPString( )
{
    if( !m_ExternalIPString.empty( ) )
        return m_ExternalIPString;
    else if( !m_ExternalIP.empty( ) )
    {
        m_ExternalIPString = UTIL_ToString( m_ExternalIP[0] ) + "." + UTIL_ToString( m_ExternalIP[1] ) + "." + UTIL_ToString( m_ExternalIP[2] ) + "." + UTIL_ToString( m_ExternalIP[3] );
        return m_ExternalIPString;
    }

    BYTEARRAY IP;
    string EIP;

    if( m_Socket )
    {
        if( !m_LANSet )
        {
            bool local = m_LAN;
            IP = m_Socket->GetIP( );

            if( IP.size( ) >= 2 )
            {
                if( IP[0] == 10 )
                    local = true;
                else if( IP[0] == 192 && IP[1] == 168 )
                    local = true;
                else if( IP[0] == 169 && IP[1] == 254 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 16 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 17 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 18 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 19 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 20 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 21 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 22 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 23 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 24 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 25 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 26 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 27 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 28 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 29 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 30 )
                    local = true;
                else if( IP[0] == 172 && IP[1] == 31 )
                    local = true;
            }

            if( UTIL_IsLocalIP( IP, m_Game->m_UNM->m_LocalAddresses ) || UTIL_IsLanIP( IP ) )
                local = true;

            m_LANSet = true;
            m_LAN = local;
        }

        EIP = m_Socket->GetIPString( );

        if( !EIP.empty( ) && EIP != "0.0.0.0" )
        {
            m_ExternalIPString = EIP;
            return EIP;
        }
        else if( !m_CachedIP.empty( ) && m_CachedIP != "0.0.0.0" )
        {
            m_ExternalIPString = m_CachedIP;
            return m_CachedIP;
        }
        else
            return m_CachedIP;
    }

    return m_CachedIP;
}

bool CPotentialPlayer::Update( void *fd )
{
	if( m_DeleteMe )
		return true;

	if( !m_Socket )
        return false;

	if( !GetName( ).empty( ) )
        m_Socket->DoRecv( static_cast<fd_set *>(fd), "[GAME: " + m_Game->m_UNM->IncGameNrDecryption( m_Game->GetGameName( ) ) + "] Player [" + GetName( ) + "] caused an error:" );
	else
        m_Socket->DoRecv( static_cast<fd_set *>(fd), "[GAME: " + m_Game->m_UNM->IncGameNrDecryption( m_Game->GetGameName( ) ) + "] Player caused an error:" );

	ExtractPackets( );
	ProcessPackets( );

    // make sure we don't keep this socket open forever (disconnect after m_DenyMaxReqjoinTime seconds)
    if( m_Game->m_UNM->m_DenyMaxReqjoinTime != 0 && m_ConnectionState == 0 && GetTicks( ) - m_ConnectionTime > m_Game->m_UNM->m_DenyMaxReqjoinTime * 1000 && !m_Banned )
		m_DeleteMe = true;

	// don't call DoSend here because some other players may not have updated yet and may generate a packet for this player
	// also m_Socket may have been set to nullptr during ProcessPackets but we're banking on the fact that m_DeleteMe has been set to true as well so it'll short circuit before dereferencing

	return m_DeleteMe || m_Error || m_Socket->HasError( ) || !m_Socket->GetConnected( );
}

void CPotentialPlayer::ExtractPackets( )
{
	if( !m_Socket )
		return;

	// extract as many packets as possible from the socket's receive buffer and put them in the m_Packets queue

	string *RecvBuffer = m_Socket->GetBytes( );
    BYTEARRAY Bytes = UTIL_CreateByteArray( reinterpret_cast<unsigned char *>(const_cast<char *>(RecvBuffer->c_str( ))), RecvBuffer->size( ) );

	// a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

	while( Bytes.size( ) >= 4 )
	{
        if( Bytes[0] == W3GS_HEADER_CONSTANT || Bytes[0] == GPS_HEADER_CONSTANT )
		{
			// bytes 2 and 3 contain the length of the packet

			uint16_t Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

			if( Length >= 4 )
			{
				if( Bytes.size( ) >= Length )
				{
					m_Packets.push( new CCommandPacket( Bytes[0], Bytes[1], BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) );
					*RecvBuffer = RecvBuffer->substr( Length );
					Bytes = BYTEARRAY( Bytes.begin( ) + Length, Bytes.end( ) );
				}
				else
					return;
			}
			else
			{
				m_Error = true;

                if( !GetDeleteMe( ) )
                    m_ErrorString = "received invalid packet from player (bad length)";

				return;
			}
		}
		else
		{
			m_Error = true;

            if( !GetDeleteMe( ) )
                m_ErrorString = "received invalid packet from player (bad header constant)";

			return;
		}
	}
}

void CPotentialPlayer::ProcessPackets( )
{
	if( !m_Socket )
		return;

	// process all the received packets in the m_Packets queue

	while( !m_Packets.empty( ) )
	{
		CCommandPacket *Packet = m_Packets.front( );
		m_Packets.pop( );

		if( Packet->GetPacketType( ) == W3GS_HEADER_CONSTANT )
		{
			// the only packet we care about as a potential player is W3GS_REQJOIN, ignore everything else

			switch( Packet->GetID( ) )
			{
				case CGameProtocol::W3GS_REQJOIN:
				delete m_IncomingJoinPlayer;
				m_IncomingJoinPlayer = m_Protocol->RECEIVE_W3GS_REQJOIN( Packet->GetData( ) );

				if( m_Name.empty( ) && m_IncomingJoinPlayer )
					m_Name = m_IncomingJoinPlayer->GetName( );

				if( m_IncomingJoinPlayer && !m_Banned )
                    m_Game->EventPlayerJoined( this, m_IncomingJoinPlayer );

				// don't continue looping because there may be more packets waiting and this parent class doesn't handle them
				// EventPlayerJoined creates the new player, NULLs the socket, and sets the delete flag on this object so it'll be deleted shortly
				// any unprocessed packets will be copied to the new CGamePlayer in the constructor or discarded if we get deleted because the game is full

				delete Packet;
				return;
			}
		}

		delete Packet;
	}
}

void CPotentialPlayer::Send( BYTEARRAY data )
{
	if( m_Socket )
		m_Socket->PutBytes( data );
}

//
// CGamePlayer
//

CGamePlayer::CGamePlayer( CPotentialPlayer *potential, unsigned char nPID, uint32_t nJoinedRealmID, string nJoinedRealm, string nName, BYTEARRAY nInternalIP, bool nReserved ) : CPotentialPlayer( potential->m_Protocol, potential->m_Game, potential->GetSocket( ) )
{
	// todotodo: properly copy queued packets to the new player, this just discards them
	// this isn't a big problem because official Warcraft III clients don't send any packets after the join request until they receive a response

	m_PID = nPID;
    m_Name = nName;
    transform( nName.begin( ), nName.end( ), nName.begin( ), static_cast<int(*)(int)>(tolower) );
    m_NameLower = nName;
	m_InternalIP = nInternalIP;
	m_JoinedRealmID = nJoinedRealmID;
	m_JoinedRealm = nJoinedRealm;
    m_TotalPacketsSent = 0;

	// hackhack: we initialize this to 1 because the CPotentialPlayer must have received a W3GS_REQJOIN before this class was created
	// to fix this we could move the packet counters to CPotentialPlayer and copy them here
	// note: we must make sure we never send a packet to a CPotentialPlayer otherwise the send counter will be incorrect too! what a mess this is...
	// that said, the packet counters are only used for managing GProxy++ reconnections

	m_ConnectionState = 1;
	m_TotalPacketsReceived = 1;
	m_LeftCode = PLAYERLEAVE_LOBBY;
	m_SyncCounter = 0;
	m_JoinTime = GetTime( );
	m_LastMapPartSent = 0;
	m_LastMapPartAcked = 0;
	m_StartedDownloadingTicks = 0;
	m_FinishedDownloadingTime = 0;
	m_FinishedLoadingTicks = 0;
	m_StartedLaggingTicks = 0;
	m_TotalLaggingTicks = 0;
	m_Cookies = 0;
	m_LastMars = GetTime( ) - m_Game->m_UNM->m_MarsGameDelay;
	m_LastBut = GetTime( ) - m_Game->m_UNM->m_ButGameDelay;
	m_LastSlap = GetTime( ) - m_Game->m_UNM->m_SlapGameDelay;
	m_LastRoll = GetTime( ) - m_Game->m_UNM->m_RollGameDelay;
	m_LastPicture = GetTime( ) - m_Game->m_UNM->m_PictureGameDelay;
	m_LastEat = GetTime( ) - m_Game->m_UNM->m_EatGameDelay;
	m_LastGProxyWaitNoticeSentTime = 0;
	m_Spoofed = false;
	m_Reserved = nReserved;
	m_WhoisShouldBeSent = false;
	m_WhoisSent = false;
	m_DownloadAllowed = false;
	m_DownloadStarted = false;
	m_DownloadFinished = false;
	m_FinishedLoading = false;
	m_Lagging = false;
	m_DropVote = false;
    m_KickVote = false;
    m_StartVote = false;
    m_Muted = false;

	if( m_Game->m_UNM->m_MsgAutoCorrectMode == 3 )
		m_MsgAutoCorrect = true;
	else
		m_MsgAutoCorrect = false;

	m_ActionsSum = 0;
	m_LeftMessageSent = false;
	m_GProxy = false;
	m_GProxyDisconnectNoticeSent = false;
	m_GProxyReconnectKey = GetTicks( );
	m_LastGProxyAckTime = 0;
	m_TimeActive = 0;
	m_DownloadInfo = string( );
	m_Censors = 0;
	m_Silence = false;
	m_Switched = false;
	m_Switching = false;
	m_Switchok = false;
    m_Left = 0;
    m_MutedTicks = 0;
    m_MutedAuto = false;
    m_FlameMessages.clear( );
    m_PubCommandWarned = false;
}

CGamePlayer::~CGamePlayer( )
{

}

string CGamePlayer::GetNameTerminated( )
{
	// if the player's name contains an unterminated colour code add the colour terminator to the end of their name
	// this is useful because it allows you to print the player's name in a longer message which doesn't colour all the subsequent text

	string LowerName = m_Name;
    transform( LowerName.begin( ), LowerName.end( ), LowerName.begin( ), static_cast<int(*)(int)>(tolower) );
	string::size_type Start = LowerName.find( "|c" );
	string::size_type End = LowerName.find( "|r" );

	if( Start != string::npos && ( End == string::npos || End < Start ) )
		return m_Name + "|r";
	else
		return m_Name;
}

uint32_t CGamePlayer::GetPing( bool LCPing )
{
    if( m_Pings.empty( ) )
        return 0;

    // just average all the pings in the vector, nothing fancy

    uint32_t AvgPing = 0;

	for( uint32_t i = 0; i < m_Pings.size( ); i++ )
		AvgPing += m_Pings[i];

	AvgPing /= m_Pings.size( );

	if( LCPing )
		return AvgPing / 2;
	else
		return AvgPing;
}

string CGamePlayer::GetPingDetail( bool LCPing )
{
    string PingStr = "Last pings: ";

    if( m_Pings.empty( ) )
    {
        PingStr += "N/A";
        return PingStr;
    }

    for( int32_t i = m_Pings.size( ) - 1; i >= 0; i-- )
        PingStr += UTIL_ToString( LCPing ? m_Pings[i] / 2 : m_Pings[i] ) + ", ";

    PingStr.erase( PingStr.end( ) - 1 );
    PingStr.erase( PingStr.end( ) - 1 );
    PingStr += " ms.";
    return PingStr;
}

bool CGamePlayer::GetIsIgnoring( string username )
{
    transform( username.begin( ), username.end( ), username.begin( ), static_cast<int(*)(int)>(tolower) );

	for( vector<string> ::iterator i = m_IgnoreList.begin( ); i != m_IgnoreList.end( ); ++i )
	{
		if( ( *i ) == username )
			return true;
	}

	return false;
}

void CGamePlayer::Ignore( string username )
{
    if( GetIsIgnoring( username ) )
        return;

    transform( username.begin( ), username.end( ), username.begin( ), static_cast<int(*)(int)>(tolower) );
	m_IgnoreList.push_back( username );
}

void CGamePlayer::UnIgnore( string username )
{
    transform( username.begin( ), username.end( ), username.begin( ), static_cast<int(*)(int)>(tolower) );

	for( vector<string> ::iterator i = m_IgnoreList.begin( ); i != m_IgnoreList.end( ); )
	{
		if( ( *i ) == username )
		{
			m_IgnoreList.erase( i );
			continue;
		}

		++i;
	}
}

bool CGamePlayer::Update( void *fd )
{
    // wait 4 seconds after joining before sending the /whereis or /w
    // if we send the /whereis too early battle.net may not have caught up with where the player is and return erroneous results

    if( m_WhoisShouldBeSent && !m_Spoofed && !m_WhoisSent && GetTime( ) - m_JoinTime >= 3 && m_Game->m_UNM->IsBnetServer( m_JoinedRealm ) )
	{
		// todotodo: we could get kicked from battle.net for sending a command with invalid characters, do some basic checking

		// check if we have this ip in our spoofed cached ip list
		bool isspoofed = m_Game->m_UNM->IsSpoofedIP( GetName( ), GetExternalIPString( ) );

		if( isspoofed )
		{
			CONSOLE_Print( "[OPT] Player " + GetName( ) + " is in the cached spoof checked list" );
			m_Game->AddToSpoofed( m_JoinedRealm, GetName( ), false );
		}

		if( !isspoofed && GetExternalIPString( ) != "127.0.0.1" )
			for( vector<CBNET *> ::iterator i = m_Game->m_UNM->m_BNETs.begin( ); i != m_Game->m_UNM->m_BNETs.end( ); i++ )
			{
				if( ( *i )->GetServer( ) == m_JoinedRealm )
				{
					if( m_Game->GetGameState( ) == GAME_PUBLIC )
					{
                        ( *i )->QueueChatCommand( "/whereis " + m_Name );

						if( m_Game->m_NameSpoofOfline.empty( ) )
							m_Game->m_NameSpoofOfline = m_Name;
						else
							m_Game->m_NameSpoofOfline += " " + m_Name;
					}
					else if( m_Game->GetGameState( ) == GAME_PRIVATE )
                        ( *i )->QueueChatCommand( gLanguage->SpoofCheckByReplying( ), m_Name, true );

					break;
				}
			}

		m_WhoisSent = true;
	}

	// check for socket timeouts
	// if we don't receive anything from a player for 30 seconds we can assume they've dropped
	// this works because in the lobby we send pings every 5 seconds and expect a response to each one
	// and in the game the Warcraft 3 client sends keepalives frequently (at least once per second it looks like)

	uint32_t WaitTime;

	if( m_Game->GetGameLoading( ) )
		WaitTime = m_Game->m_UNM->m_DropWaitTimeLoad;
	else if( m_Game->GetGameLoaded( ) )
		WaitTime = m_Game->m_UNM->m_DropWaitTimeGame;
	else
		WaitTime = m_Game->m_UNM->m_DropWaitTimeLobby;

	if( m_Socket && WaitTime != 0 && GetTime( ) - m_Socket->GetLastRecv( ) >= WaitTime )
		m_Game->EventPlayerDisconnectTimedOut( this );

	// make sure we're not waiting too long for the first MAPSIZE packet

    if( !m_Game->IsOwner( GetName( ) ) && !m_Game->IsDefaultOwner( GetName( ) ) && m_Game->m_UNM->m_DenyMaxMapsizeTime != 0 && m_ConnectionState == 1 && GetTicks( ) - m_ConnectionTime > m_Game->m_UNM->m_DenyMaxMapsizeTime * 1000 && !m_Game->GetGameLoaded( ) && !m_Game->GetGameLoading( ) )
    {
        m_Game->DelTempOwner( GetName( ) );
        m_Game->SendAllChat( gLanguage->KickMsgForSlowDL( GetName( ) ) );
        m_DeleteMe = true;
        SetLeftReason( "MAPSIZE not received within a few seconds" );
        SetLeftCode( PLAYERLEAVE_LOBBY );
        m_Game->OpenSlot( m_Game->GetSIDFromPID( GetPID( ) ), false );
    }

	// disconnect if the player is downloading too slowly

    if( m_DownloadStarted && !m_DownloadFinished && !m_Game->GetGameLoaded( ) && !m_Game->GetGameLoading( ) && GetLastMapPartSent( ) != 0 )
	{
		uint32_t downloadingTime = GetTicks( ) - m_StartedDownloadingTicks;
		uint32_t Seconds = downloadingTime / 1000;
		uint32_t Rate = GetLastMapPartSent( ) / 1024;	// Rate in miliseconds

		if( Seconds > 0 )
			Rate /= Seconds;

        if( !m_Game->IsOwner( GetName( ) ) && !m_Game->IsDefaultOwner( GetName( ) ) )
		{
			if( m_Game->m_UNM->m_DenyMinDownloadRate != 0 && GetLastMapPartSent( ) <= 102400 && 0 < Rate && Rate < m_Game->m_UNM->m_DenyMinDownloadRate )
			{
				m_Game->DelTempOwner( GetName( ) );
                m_Game->SendAllChat( gLanguage->KickMsgForLowDLRate( GetName( ), UTIL_ToString( Rate ), UTIL_ToString( m_Game->m_UNM->m_DenyMinDownloadRate ) ) );
				m_DeleteMe = true;
				SetLeftReason( "kicked for downloading too slowly " + UTIL_ToString( Rate ) + " KB/s" );
				SetLeftCode( PLAYERLEAVE_LOBBY );
				m_Game->OpenSlot( m_Game->GetSIDFromPID( GetPID( ) ), false );
			}

			if( m_Game->m_UNM->m_DenyMaxDownloadTime != 0 && downloadingTime > m_Game->m_UNM->m_DenyMaxDownloadTime * 1000 )
			{
				m_Game->DelTempOwner( GetName( ) );
                m_Game->SendAllChat( gLanguage->KickMsgForLongDL( GetName( ), UTIL_ToString( downloadingTime / 1000 ), UTIL_ToString( m_Game->m_UNM->m_DenyMaxDownloadTime ) ) );
				m_DeleteMe = true;
				SetLeftReason( "download time is too long" );
				SetLeftCode( PLAYERLEAVE_LOBBY );
				m_Game->OpenSlot( m_Game->GetSIDFromPID( GetPID( ) ), false );
			}
		}
	}

	// unmute player

	if( GetMuted( ) && m_MutedAuto && ( GetTicks( ) - m_MutedTicks > ( m_Game->m_UNM->m_AntiFloodMuteSeconds * 1000 ) ) )
	{
		m_MuteMessages.clear( );
		SetMuted( false );

		if( !m_Game->IsMuted( m_Name ) || m_Game->IsCensorMuted( m_Name ) == 0 )
		{
			if( m_Game->m_UNM->m_AutoUnMutePrinting == 1 )
                m_Game->SendAllChat( gLanguage->RemovedPlayerFromTheAutoMuteList( m_Name ) );
			else if( m_Game->m_UNM->m_AutoUnMutePrinting == 2 )
				m_Game->SendChat( m_PID, "Вы снова можете писать в чате" );
			else if( m_Game->m_UNM->m_AutoUnMutePrinting == 3 )
			{
				m_Game->SendChat( m_PID, "Вы снова можете писать в чате" );
                m_Game->SendAllChat( gLanguage->RemovedPlayerFromTheAutoMuteList( m_Name ) );
			}
		}
	}

	// GProxy++ acks

	if( m_Socket && m_GProxy && GetTime( ) - m_LastGProxyAckTime >= 10 )
	{
		m_Socket->PutBytes( m_Game->m_UNM->m_GPSProtocol->SEND_GPSS_ACK( m_TotalPacketsReceived ) );
		m_LastGProxyAckTime = GetTime( );
	}

	// base class update

	CPotentialPlayer::Update( fd );
	bool Deleting;

	if( m_GProxy && m_Game->GetGameLoaded( ) )
		Deleting = m_DeleteMe || m_Error;
    else if( m_Socket )
		Deleting = m_DeleteMe || m_Error || m_Socket->HasError( ) || !m_Socket->GetConnected( );
    else
        Deleting = true;

	// try to find out why we're requesting deletion
	// in cases other than the ones covered here m_LeftReason should have been set when m_DeleteMe was set

    if( m_Error )
    {
        m_Game->EventPlayerDisconnectPlayerError( this );
        m_Socket->Reset( );
        return Deleting;
    }

	if( m_Socket )
	{
		if( m_Socket->HasError( ) )
			m_Game->EventPlayerDisconnectSocketError( this );
		else if( !m_Socket->GetConnected( ) )
			m_Game->EventPlayerDisconnectConnectionClosed( this );
	}

	return Deleting;
}

void CGamePlayer::ExtractPackets( )
{
	if( !m_Socket )
		return;

	// extract as many packets as possible from the socket's receive buffer and put them in the m_Packets queue

	string *RecvBuffer = m_Socket->GetBytes( );
    BYTEARRAY Bytes = UTIL_CreateByteArray( reinterpret_cast<unsigned char *>(const_cast<char *>(RecvBuffer->c_str( ))), RecvBuffer->size( ) );

	// a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

	while( Bytes.size( ) >= 4 )
	{
        if( Bytes[0] == W3GS_HEADER_CONSTANT || Bytes[0] == GPS_HEADER_CONSTANT )
		{
			// bytes 2 and 3 contain the length of the packet

			uint16_t Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

			if( Length >= 4 )
			{
				if( Bytes.size( ) >= Length )
				{
                    m_Packets.push( new CCommandPacket( Bytes[0], Bytes[1], BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) );

                    if( Bytes[0] == W3GS_HEADER_CONSTANT )
                        m_TotalPacketsReceived++;

                    *RecvBuffer = RecvBuffer->substr( Length );
                    Bytes = BYTEARRAY( Bytes.begin( ) + Length, Bytes.end( ) );
				}
				else
					return;
			}
			else
			{
				m_Error = true;

                if( !GetDeleteMe( ) )
                    m_ErrorString = "received invalid packet from player (bad length)";

				return;
			}
		}
		else
		{
			m_Error = true;

            if( !GetDeleteMe( ) )
                m_ErrorString = "received invalid packet from player (bad header constant)";

			return;
		}
	}
}

void CGamePlayer::ProcessPackets( )
{
	if( !m_Socket )
		return;

	CIncomingAction *Action = nullptr;
	CIncomingChatPlayer *ChatPlayer = nullptr;
	CIncomingMapSize *MapSize = nullptr;
	uint32_t CheckSum = 0;
	uint32_t Pong = 0;

	// process all the received packets in the m_Packets queue

	while( !m_Packets.empty( ) )
	{
		CCommandPacket *Packet = m_Packets.front( );
		m_Packets.pop( );

		if( Packet->GetPacketType( ) == W3GS_HEADER_CONSTANT )
		{
			switch( Packet->GetID( ) )
			{
				case CGameProtocol::W3GS_LEAVEGAME:
				m_Game->EventPlayerLeft( this, m_Protocol->RECEIVE_W3GS_LEAVEGAME( Packet->GetData( ) ) );
				break;

				case CGameProtocol::W3GS_GAMELOADED_SELF:
				if( m_Protocol->RECEIVE_W3GS_GAMELOADED_SELF( Packet->GetData( ) ) )
				{
					if( !m_FinishedLoading && m_Game->GetGameLoading( ) )
					{
						m_FinishedLoading = true;
						m_FinishedLoadingTicks = GetTicks( );
						m_Game->EventPlayerLoaded( this );
					}
					else
					{
						// we received two W3GS_GAMELOADED_SELF packets from this player!
					}
				}

				break;

				case CGameProtocol::W3GS_OUTGOING_ACTION:
				Action = m_Protocol->RECEIVE_W3GS_OUTGOING_ACTION( Packet->GetData( ), m_PID );

				if( Action )
					m_Game->EventPlayerAction( this, Action );

				// don't delete Action here because the game is going to store it in a queue and delete it later

				break;

				case CGameProtocol::W3GS_OUTGOING_KEEPALIVE:
				CheckSum = m_Protocol->RECEIVE_W3GS_OUTGOING_KEEPALIVE( Packet->GetData( ) );
				m_CheckSums.push( CheckSum );

				if( !m_Game->GetPause( ) )
					m_SyncCounter++;

				m_Game->EventPlayerKeepAlive( this );
				break;

				case CGameProtocol::W3GS_CHAT_TO_HOST:
				ChatPlayer = m_Protocol->RECEIVE_W3GS_CHAT_TO_HOST( Packet->GetData( ) );

				if( ChatPlayer )
				{
					// determine if we should auto-mute this player

                    if( ChatPlayer->GetFromPID( ) == GetPID( ) && ( ChatPlayer->GetType( ) == CIncomingChatPlayer::CTH_MESSAGE || ChatPlayer->GetType( ) == CIncomingChatPlayer::CTH_MESSAGEEXTRA ) )
					{
						m_MuteMessages.push_back( GetTicks( ) );

						if( m_MuteMessages.size( ) > m_Game->m_UNM->m_AntiFloodRigidityNumber )
							m_MuteMessages.erase( m_MuteMessages.begin( ) );

						uint32_t RecentCount = 0;

						for( uint32_t i = 0; i < m_MuteMessages.size( ); ++i )
						{
							if( GetTicks( ) - m_MuteMessages[i] < m_Game->m_UNM->m_AntiFloodRigidityTime * 1000 )
								RecentCount++;
						}

						if( !GetMuted( ) && !m_Game->IsMuted( m_Name ) && m_Game->IsCensorMuted( m_Name ) == 0 && RecentCount >= m_Game->m_UNM->m_AntiFloodRigidityNumber && m_Game->m_UNM->m_AntiFloodMute )
						{
							SetMuted( true );
							m_MutedAuto = true;
                            m_Game->SendAllChat( gLanguage->PlayerMutedForBeingFloodMouthed( m_Name, UTIL_ToString( m_Game->m_UNM->m_AntiFloodMuteSeconds ) ) );
							m_MuteMessages.clear( );
						}

						// now check for flamers

						if( m_Game->m_UNM->m_NewCensorCookies )
                        {

							if( m_Game->m_UNM->CensorCheck( ChatPlayer->GetMessage( ) ) )
							{
								m_FlameMessages.push_back( GetTicks( ) );

								if( m_FlameMessages.size( ) > m_Game->m_UNM->m_NewCensorCookiesNumber )
									m_FlameMessages.erase( m_FlameMessages.begin( ) );

								RecentCount = 0;

								for( uint32_t i = 0; i < m_FlameMessages.size( ); ++i )
								{
									if( GetTicks( ) - m_FlameMessages[i] < 3600000 )
										RecentCount++;
								}

								if( RecentCount >= m_Game->m_UNM->m_NewCensorCookiesNumber )
								{
                                    m_FlameMessages.clear( );
									uint32_t numCookies = GetCookies( );

                                    if( numCookies != m_Game->m_UNM->m_MaximumNumberCookies )
									{

										if( numCookies < m_Game->m_UNM->m_MaximumNumberCookies )
                                            numCookies++;

										if( numCookies > m_Game->m_UNM->m_MaximumNumberCookies )
                                            numCookies = 1;

										SetCookies( numCookies );

										if( m_Game->m_UNM->m_FunCommandsGame && m_Game->m_UNM->m_EatGame )
											m_Game->SendAllChat( "Игрок [" + GetName( ) + "] получил печенье. Юзай " + string( 1, m_Game->m_UNM->m_CommandTrigger ) + "eat чтобы съесть!" );
										else
											m_Game->SendAllChat( "Игрок [" + GetName( ) + "] получил печенье!" );
									}
								}
							}
                        }
					}

					m_Game->EventPlayerChatToHost( this, ChatPlayer );
				}

				delete ChatPlayer;
				ChatPlayer = nullptr;
				break;

				case CGameProtocol::W3GS_DROPREQ:
				// todotodo: no idea what's in this packet

				if( !m_DropVote )
					m_Game->EventPlayerDropRequest( this );

				break;

				case CGameProtocol::W3GS_MAPSIZE:
				m_ConnectionState = 2;
				m_ConnectionTime = GetTicks( );
                MapSize = m_Protocol->RECEIVE_W3GS_MAPSIZE( Packet->GetData( ) );

				if( MapSize )
					m_Game->EventPlayerMapSize( this, MapSize );

				delete MapSize;
				MapSize = nullptr;
				break;

				case CGameProtocol::W3GS_PONG_TO_HOST:
				Pong = m_Protocol->RECEIVE_W3GS_PONG_TO_HOST( Packet->GetData( ) );

				// we discard pong values of 1
				// the client sends one of these when connecting plus we return 1 on error to kill two birds with one stone

				if( Pong != 1 )
				{
					// we also discard pong values when we're downloading because they're almost certainly inaccurate
					// this statement also gives the player a 3 second grace period after downloading the map to allow queued (i.e. delayed) ping packets to be ignored

					if( !m_DownloadStarted || ( m_DownloadFinished && GetTime( ) - m_FinishedDownloadingTime >= 3 ) )
					{
						// we also discard pong values when anyone else is downloading if we're configured to

						if( m_Game->m_UNM->m_PingDuringDownloads || !m_Game->IsDownloading( ) )
						{
							m_Pings.push_back( GetTicks( ) - Pong );

							if( m_Pings.size( ) > 20 )
								m_Pings.erase( m_Pings.begin( ) );
						}
					}
				}

				m_Game->EventPlayerPongToHost( this );
				break;
			}
		}
		else if( Packet->GetPacketType( ) == GPS_HEADER_CONSTANT )
		{
			BYTEARRAY Data = Packet->GetData( );

			if( Packet->GetID( ) == CGPSProtocol::GPS_INIT )
			{
				if( m_Game->m_UNM->m_Reconnect )
				{
					m_GProxy = true;
					m_Socket->PutBytes( m_Game->m_UNM->m_GPSProtocol->SEND_GPSS_INIT( m_Game->m_UNM->m_ReconnectPort, m_PID, m_GProxyReconnectKey, m_Game->GetGProxyEmptyActions( ) ) );
                    CONSOLE_Print( "[GAME: " + m_Game->m_UNM->IncGameNrDecryption( m_Game->GetGameName( ) ) + "] player [" + m_Name + "] is using GProxy++", 6, m_Game->GetCurrentGameCounter( ) );
				}
				else
				{
					// todotodo: send notice that we're not permitting reconnects
					// note: GProxy++ should never send a GPS_INIT message if bot_reconnect = 0 because we don't advertise the game with invalid map dimensions
					// but it would be nice to cover this case anyway
				}
			}
			else if( Packet->GetID( ) == CGPSProtocol::GPS_ACK && Data.size( ) == 8 )
			{
				uint32_t LastPacket = UTIL_ByteArrayToUInt32( Data, false, 4 );
				uint32_t PacketsAlreadyUnqueued = m_TotalPacketsSent - m_GProxyBuffer.size( );

				if( LastPacket > PacketsAlreadyUnqueued )
				{
					uint32_t PacketsToUnqueue = LastPacket - PacketsAlreadyUnqueued;

					if( PacketsToUnqueue > m_GProxyBuffer.size( ) )
						PacketsToUnqueue = m_GProxyBuffer.size( );

					while( PacketsToUnqueue > 0 )
					{
						m_GProxyBuffer.pop( );
						PacketsToUnqueue--;
					}
				}
			}
		}

		delete Packet;
	}
}

void CGamePlayer::Send( BYTEARRAY data )
{
	// must start counting packet total from beginning of connection
	// but we can avoid buffering packets until we know the client is using GProxy++ since that'll be determined before the game starts
	// this prevents us from buffering packets for non-GProxy++ clients

	++m_TotalPacketsSent;

	if( m_GProxy && m_Game->GetGameLoaded( ) )
		m_GProxyBuffer.push( data );

	CPotentialPlayer::Send( data );
}

void CGamePlayer::EventGProxyReconnect( CTCPSocket *NewSocket, uint32_t LastPacket )
{
	delete m_Socket;
	m_Socket = NewSocket;
	m_Socket->PutBytes( m_Game->m_UNM->m_GPSProtocol->SEND_GPSS_RECONNECT( m_TotalPacketsReceived ) );

	uint32_t PacketsAlreadyUnqueued = m_TotalPacketsSent - m_GProxyBuffer.size( );

	if( LastPacket > PacketsAlreadyUnqueued )
	{
		uint32_t PacketsToUnqueue = LastPacket - PacketsAlreadyUnqueued;

		if( PacketsToUnqueue > m_GProxyBuffer.size( ) )
			PacketsToUnqueue = m_GProxyBuffer.size( );

		while( PacketsToUnqueue > 0 )
		{
			m_GProxyBuffer.pop( );
			PacketsToUnqueue--;
		}
	}

	// send remaining packets from buffer, preserve buffer

	queue<BYTEARRAY> TempBuffer;

	while( !m_GProxyBuffer.empty( ) )
	{
		m_Socket->PutBytes( m_GProxyBuffer.front( ) );
		TempBuffer.push( m_GProxyBuffer.front( ) );
		m_GProxyBuffer.pop( );
	}

	m_GProxyBuffer = TempBuffer;
	m_GProxyDisconnectNoticeSent = false;
    m_Game->SendAllChat( gLanguage->PlayerReconnectedWithGProxy( m_Name ) );
	m_CachedIP = m_Socket->GetIPString( );
}
