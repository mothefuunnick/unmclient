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

#ifndef GAMEPLAYER_H
#define GAMEPLAYER_H

#include <list>

class CTCPSocket;
class CCommandPacket;
class CGameProtocol;
class CGame;
class CIncomingJoinPlayer;

//
// CPotentialPlayer
//

class CPotentialPlayer
{
public:
	CGameProtocol *m_Protocol;
	CBaseGame *m_Game;

protected:
	// note: we permit m_Socket to be nullptr in this class to allow for the virtual host player which doesn't really exist
	// it also allows us to convert CPotentialPlayers to CGamePlayers without the CPotentialPlayer's destructor closing the socket

	CTCPSocket *m_Socket;
    std::queue<CCommandPacket *> m_Packets;
    BYTEARRAY m_ExternalIP;
    std::string m_ExternalIPString;
	bool m_DeleteMe;
	bool m_Error;
	bool m_LANSet;
	bool m_LAN;
    std::string m_Name;
    std::string m_ErrorString;
    CIncomingJoinPlayer *m_IncomingJoinPlayer;
    bool m_Banned;
	uint32_t m_ConnectionState;		// zero if no packets received (wait REQJOIN), one if only REQJOIN received (wait MAPSIZE), two otherwise
	uint32_t m_ConnectionTime;		// last time the player did something relating to connection state
    std::string m_CachedIP;				// cached external IP std::string

public:
	CPotentialPlayer( CGameProtocol *nProtocol, CBaseGame *nGame, CTCPSocket *nSocket );
	virtual ~CPotentialPlayer( );
	virtual BYTEARRAY GetExternalIP( );
    virtual std::string GetExternalIPString( );
    virtual CTCPSocket *GetSocket( )
	{
		return m_Socket;
	}
    virtual std::queue<CCommandPacket *> GetPackets( )
	{
		return m_Packets;
	}
    virtual std::string GetName( )
	{
		return m_Name;
	}
	virtual bool IsLAN( )
	{
		if( m_LANSet )
			return m_LAN;
		else
		{
			GetExternalIPString( );
			return m_LAN;
		}
	}
	virtual bool GetDeleteMe( )
	{
		return m_DeleteMe;
	}
	virtual bool GetError( )
	{
		return m_Error;
	}
    virtual std::string GetErrorString( )
	{
		return m_ErrorString;
    }
	virtual CIncomingJoinPlayer *GetJoinPlayer( )
	{
		return m_IncomingJoinPlayer;
    }
	virtual void SetSocket( CTCPSocket *nSocket )
	{
		m_Socket = nSocket;
	}
	virtual void SetDeleteMe( bool nDeleteMe )
	{
		m_DeleteMe = nDeleteMe;
    }
	virtual void SetBanned( )
	{
		m_Banned = true;
	}

	// processing functions

	virtual bool Update( void *fd );
	virtual void ExtractPackets( );
	virtual void ProcessPackets( );

	// other functions

	virtual void Send( BYTEARRAY data );
};

//
// CGamePlayer
//

class CGamePlayer : public CPotentialPlayer
{
private:
	unsigned char m_PID;
	unsigned char m_SID;
    std::string m_Name;								// the player's name
    std::string m_NameLower;                         // the player's name lower
	BYTEARRAY m_InternalIP;						// the player's internal IP address as reported by the player when connecting
    std::vector<uint32_t> m_Pings;					// store the last few (20) pings received so we can take an average
    std::queue<uint32_t> m_CheckSums;				// the last few checksums the player has sent (for detecting desyncs)
    std::string m_LeftReason;						// the reason the player left the game
    std::string m_SpoofedRealm;						// the realm the player last spoof checked on
	uint32_t m_JoinedRealmID;					// the realm ID the player joined on
    std::string m_JoinedRealm;						// the realm the player joined on (probable, can be spoofed)
	uint32_t m_TotalPacketsSent;
	uint32_t m_TotalPacketsReceived;
    uint32_t m_Left;
	uint32_t m_LeftCode;						// the code to be sent in W3GS_PLAYERLEAVE_OTHERS for why this player left the game
    uint32_t m_Cookies;							// the number of cookies this user has, for the !cookie and !eat command
	uint32_t m_SyncCounter;						// the number of keepalive packets received from this player
    uint32_t m_JoinTime;						// GetTime when the player joined the game (used to delay sending the /whereis a few seconds to allow for some lag)
	uint32_t m_LastMapPartSent;					// the last mappart sent to the player (for sending more than one part at a time)
	uint32_t m_LastMapPartAcked;				// the last mappart acknowledged by the player
	uint32_t m_StartedDownloadingTicks;			// GetTicks when the player started downloading the map
	uint32_t m_FinishedDownloadingTime;			// GetTime when the player finished downloading the map
	uint32_t m_FinishedLoadingTicks;			// GetTicks when the player finished loading the game
	uint32_t m_StartedLaggingTicks;				// GetTicks when the player started lagging
	uint32_t m_TotalLaggingTicks;				// total ticks that the player has been lagging in the game
    uint32_t m_LastMars;						// time when last mars was issued
	uint32_t m_LastBut;							// time when last but was issued
	uint32_t m_LastSlap;						// time when last slap was issued
	uint32_t m_LastRoll;						// time when last roll was issued
	uint32_t m_LastPicture;						// time when last picture was issued
	uint32_t m_LastEat;							// time when last eat was issued
	uint32_t m_LastGProxyWaitNoticeSentTime;
    std::queue<BYTEARRAY> m_LoadInGameData;			// queued data to be sent when the player finishes loading when using "load in game"
    uint32_t m_Censors;
	bool m_Silence;
    std::string m_DownloadInfo;						// Download info
    bool m_Spoofed;								// if the player has spoof checked or not
	bool m_Reserved;							// if the player is reserved (VIP) or not
    bool m_WhoisShouldBeSent;					// if a battle.net /whereis should be sent for this player or not
    bool m_WhoisSent;							// if we've sent a battle.net /whereis for this player yet (for spoof checking)
	bool m_DownloadAllowed;						// if we're allowed to download the map or not (used with permission based map downloads)
	bool m_DownloadStarted;						// if we've started downloading the map or not
	bool m_DownloadFinished;					// if we've finished downloading the map or not
	bool m_FinishedLoading;						// if the player has finished loading or not
	bool m_Lagging;								// if the player is lagging or not (on the lag screen)
	bool m_DropVote;							// if the player voted to drop the laggers or not (on the lag screen)
	bool m_KickVote;							// if the player voted to kick a player or not
	bool m_StartVote;							// if the player voted to start or not
    bool m_Muted;								// if the player is muted or not
	uint32_t m_MutedTicks;
    std::vector<uint32_t> m_FlameMessages;			// times player sent messages to determine flame level
	bool m_MutedAuto;							// whether or not mute was automatic mute
	bool m_LeftMessageSent;						// if the playerleave message has been sent or not
	bool m_GProxy;								// if the player is using GProxy++
	bool m_GProxyDisconnectNoticeSent;			// if a disconnection notice has been sent or not when using GProxy++
    std::queue<BYTEARRAY> m_GProxyBuffer;
	uint32_t m_GProxyReconnectKey;           	// the GProxy++ reconnect key
    uint32_t m_LastGProxyAckTime;
    std::vector<std::string> m_IgnoreList;				// list of usernames this player is ignoring
    uint32_t m_TimeActive;						// AFK detection
    bool m_PubCommandWarned;

public:
    CGamePlayer( CPotentialPlayer *potential, unsigned char nPID, uint32_t nJoinedRealmID, std::string nJoinedRealm, std::string nName, BYTEARRAY nInternalIP, bool nReserved );
	virtual ~CGamePlayer( );

    std::deque<std::string> m_MessagesBeforeLoading;
    std::deque<unsigned char> m_MessagesAuthorsBeforeLoading;
    std::deque<unsigned char> m_MessagesFlagsBeforeLoading;
    std::deque<BYTEARRAY> m_MessagesExtraFlagsBeforeLoading;
	bool m_Switched;
	bool m_Switching;
	bool m_Switchok;
    bool m_MsgAutoCorrect;
	uint32_t m_ActionsSum;
    std::deque<BYTEARRAY> m_BufferMessages;
	unsigned char GetSID( )
	{
		return m_SID;
	}
	unsigned char GetPID( )
	{
		return m_PID;
	}
    std::string GetName( )
	{
		return m_Name;
    }
    std::string GetNameLower( )
    {
        return m_NameLower;
    }
	BYTEARRAY GetInternalIP( )
	{
		return m_InternalIP;
	}
	unsigned int GetNumPings( )
	{
		return m_Pings.size( );
	}
	unsigned int GetNumCheckSums( )
	{
		return m_CheckSums.size( );
	}
    std::queue<uint32_t> *GetCheckSums( )
	{
		return &m_CheckSums;
	}
    std::string GetLeftReason( )
	{
		return m_LeftReason;
	}
    std::string GetSpoofedRealm( )
	{
		return m_SpoofedRealm;
	}
	uint32_t GetJoinedRealmID( )
	{
		return m_JoinedRealmID;
	}
    std::string GetJoinedRealm( )
	{
		return m_JoinedRealm;
	}
	uint32_t GetCookies( )
	{
		return m_Cookies;
	}
	uint32_t GetLeftCode( )
	{
		return m_LeftCode;
    }
	uint32_t GetSyncCounter( )
	{
		return m_SyncCounter;
	}
	uint32_t GetJoinTime( )
	{
		return m_JoinTime;
	}
	uint32_t GetLastMapPartSent( )
	{
		return m_LastMapPartSent;
	}
	uint32_t GetLastMapPartAcked( )
	{
		return m_LastMapPartAcked;
	}
	uint32_t GetStartedDownloadingTicks( )
	{
		return m_StartedDownloadingTicks;
	}
	uint32_t GetFinishedDownloadingTime( )
	{
		return m_FinishedDownloadingTime;
	}
	uint32_t GetFinishedLoadingTicks( )
	{
		return m_FinishedLoadingTicks;
	}
	uint32_t GetStartedLaggingTicks( )
	{
		return m_StartedLaggingTicks;
	}
	uint32_t GetTotalLaggingTicks( )
	{
		return m_TotalLaggingTicks;
    }
	uint32_t GetLastMars( )
	{
		return m_LastMars;
	}
	uint32_t GetLastBut( )
	{
		return m_LastBut;
	}
	uint32_t GetLastSlap( )
	{
		return m_LastSlap;
	}
	uint32_t GetLastRoll( )
	{
		return m_LastRoll;
	}
	uint32_t GetLastPicture( )
	{
		return m_LastPicture;
	}
	uint32_t GetLastEat( )
	{
		return m_LastEat;
	}
	uint32_t GetLastGProxyWaitNoticeSentTime( )
	{
		return m_LastGProxyWaitNoticeSentTime;
	}
    std::queue<BYTEARRAY> *GetLoadInGameData( )
	{
		return &m_LoadInGameData;
    }
    std::string GetDownloadInfo( )
	{
		return m_DownloadInfo;
    }
	bool GetSpoofed( )
	{
		return m_Spoofed;
	}
	bool GetReserved( )
	{
		return m_Reserved;
	}
	bool GetWhoisShouldBeSent( )
	{
		return m_WhoisShouldBeSent;
	}
	bool GetWhoisSent( )
	{
		return m_WhoisSent;
	}
	bool GetDownloadAllowed( )
	{
		return m_DownloadAllowed;
	}
	bool GetDownloadStarted( )
	{
		return m_DownloadStarted;
	}
	bool GetDownloadFinished( )
	{
		return m_DownloadFinished;
	}
	bool GetFinishedLoading( )
	{
		return m_FinishedLoading;
	}
	bool GetLagging( )
	{
		return m_Lagging;
	}
	bool GetDropVote( )
	{
		return m_DropVote;
	}
	bool GetKickVote( )
	{
		return m_KickVote;
	}
	bool GetStartVote( )
	{
		return m_StartVote;
    }
	bool GetMuted( )
	{
		return m_Muted;
	}
	bool GetLeftMessageSent( )
	{
		return m_LeftMessageSent;
    }
	uint32_t GetCensors( )
	{
		return m_Censors;
	}
	bool GetSilence( )
	{
		return m_Silence;
    }
	bool GetGProxy( )
	{
		return m_GProxy;
	}
	bool GetGProxyDisconnectNoticeSent( )
	{
		return m_GProxyDisconnectNoticeSent;
	}
	uint32_t GetGProxyReconnectKey( )
	{
		return m_GProxyReconnectKey;
    }
    bool GetPubCommandWarned( )
    {
        return m_PubCommandWarned;
    }
    void SetPubCommandWarned( bool nPubCommandWarned )
    {
        m_PubCommandWarned = nPubCommandWarned;
    }
    void SetLeftReason( std::string nLeftReason )
	{
		m_LeftReason = nLeftReason;
	}
    void SetSpoofedRealm( std::string nSpoofedRealm )
	{
		m_SpoofedRealm = nSpoofedRealm;
	}
	void SetFinishedLoadingTicks( uint32_t nFinishedLoadingTicks )
	{
		m_FinishedLoadingTicks = nFinishedLoadingTicks;
	}
	void SetLeftCode( uint32_t nLeftCode )
	{
		m_LeftCode = nLeftCode;
	}
	void SetCookies( uint32_t nCookies )
	{
		m_Cookies = nCookies;
    }
	void SetSyncCounter( uint32_t nSyncCounter )
	{
		m_SyncCounter = nSyncCounter;
	}
	void SetLastMapPartSent( uint32_t nLastMapPartSent )
	{
		m_LastMapPartSent = nLastMapPartSent;
	}
	void SetLastMapPartAcked( uint32_t nLastMapPartAcked )
	{
		m_LastMapPartAcked = nLastMapPartAcked;
	}
	void SetStartedDownloadingTicks( uint32_t nStartedDownloadingTicks )
	{
		m_StartedDownloadingTicks = nStartedDownloadingTicks;
	}
	void SetFinishedDownloadingTime( uint32_t nFinishedDownloadingTime )
	{
		m_FinishedDownloadingTime = nFinishedDownloadingTime;
	}
	void SetStartedLaggingTicks( uint32_t nStartedLaggingTicks )
	{
		m_StartedLaggingTicks = nStartedLaggingTicks;
	}
	void SetTotalLaggingTicks( uint32_t nTotalLaggingTicks )
	{
		m_TotalLaggingTicks = nTotalLaggingTicks;
    }
	void SetLastMars( uint32_t nLastMars )
	{
		m_LastMars = nLastMars;
	}
	void SetLastBut( uint32_t nLastBut )
	{
		m_LastBut = nLastBut;
	}
	void SetLastSlap( uint32_t nLastSlap )
	{
		m_LastSlap = nLastSlap;
	}
	void SetLastRoll( uint32_t nLastRoll )
	{
		m_LastRoll = nLastRoll;
	}
	void SetLastPicture( uint32_t nLastPicture )
	{
		m_LastPicture = nLastPicture;
	}
	void SetLastEat( uint32_t nLastEat )
	{
		m_LastEat = nLastEat;
	}
	void SetLastGProxyWaitNoticeSentTime( uint32_t nLastGProxyWaitNoticeSentTime )
	{
		m_LastGProxyWaitNoticeSentTime = nLastGProxyWaitNoticeSentTime;
    }
	void SetSpoofed( bool nSpoofed )
	{
		m_Spoofed = nSpoofed;
	}
	void SetReserved( bool nReserved )
	{
		m_Reserved = nReserved;
	}
	void SetWhoisShouldBeSent( bool nWhoisShouldBeSent )
	{
		m_WhoisShouldBeSent = nWhoisShouldBeSent;
	}
	void SetDownloadAllowed( bool nDownloadAllowed )
	{
		m_DownloadAllowed = nDownloadAllowed;
	}
	void SetDownloadStarted( bool nDownloadStarted )
	{
		m_DownloadStarted = nDownloadStarted;
	}
	void SetDownloadFinished( bool nDownloadFinished )
	{
		m_DownloadFinished = nDownloadFinished;
	}
    void SetDownloadInfo( std::string nDownloadInfo )
	{
		m_DownloadInfo = nDownloadInfo;
	}
	void SetLagging( bool nLagging )
	{
		m_Lagging = nLagging;
	}
	void SetDropVote( bool nDropVote )
	{
		m_DropVote = nDropVote;
	}
	void SetKickVote( bool nKickVote )
	{
		m_KickVote = nKickVote;
	}
	void SetStartVote( bool nStartVote )
	{
		m_StartVote = nStartVote;
    }
	void SetMuted( bool nMuted )
	{
        m_Muted = nMuted;
        m_MutedTicks = GetTicks( );
        m_MutedAuto = false;
	}
	void SetLeftMessageSent( bool nLeftMessageSent )
	{
		m_LeftMessageSent = nLeftMessageSent;
	}
	void SetGProxyDisconnectNoticeSent( bool nGProxyDisconnectNoticeSent )
	{
		m_GProxyDisconnectNoticeSent = nGProxyDisconnectNoticeSent;
    }
    void SetName( std::string nName )
	{
		m_Name = nName;
	}
	void SetSID( unsigned char nSID )
	{
		m_SID = nSID;
    }
	void SetCensors( uint32_t nCensors )
	{
		m_Censors = nCensors;
	}
	void SetSilence( bool nSilence )
	{
		m_Silence = nSilence;
    }
    uint32_t GetLeftTime( )
    {
        return m_Left;
    }
    void SetLeftTime( uint32_t nLeft )
    {
        if( nLeft == 0 )
            m_Left = 1;
        else
            m_Left = nLeft;
    }

    std::string GetNameTerminated( );
	uint32_t GetPing( bool LCPing );
    std::string GetPingDetail( bool LCPing );
    std::vector<uint32_t> m_MuteMessages;			// times player sent messages to determine if we should automute
    bool GetIsIgnoring( std::string username );
    void Ignore( std::string username );
    void UnIgnore( std::string username );


	void AddLoadInGameData( BYTEARRAY nLoadInGameData )
	{
		m_LoadInGameData.push( nLoadInGameData );
	}

	// AFK detection
	uint32_t GetTimeActive( )
	{
		return m_TimeActive;
	}
	void SetTimeActive( uint32_t nTimeActive )
	{
		m_TimeActive = nTimeActive;
	}

	// processing functions

	virtual bool Update( void *fd );
	virtual void ExtractPackets( );
	virtual void ProcessPackets( );

	// other functions

	virtual void Send( BYTEARRAY data );
	virtual void EventGProxyReconnect( CTCPSocket *NewSocket, uint32_t LastPacket );
};

#endif
