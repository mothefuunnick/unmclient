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
#include "map.h"
#include "bnetprotocol.h"
#include "socket.h"
#include "gameprotocol.h"

using namespace std;

CBNETProtocol::CBNETProtocol( CUNM *nUNM, string nServer )
{
    m_UNM = nUNM;
    m_Server = nServer;
	unsigned char ClientToken[] = { 220, 1, 203, 7 };
	m_ClientToken = UTIL_CreateByteArray( ClientToken, 4 );
}

CBNETProtocol::~CBNETProtocol( )
{

}

///////////////////////
// RECEIVE FUNCTIONS //
///////////////////////

bool CBNETProtocol::RECEIVE_SID_NULL( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length

	return ValidateLength( data );
}

vector<CIncomingGameHost *> CBNETProtocol::RECEIVE_SID_GETADVLISTEX( BYTEARRAY data )
{
	// DEBUG_Print( "RECEIVED SID_GETADVLISTEX" );
	// DEBUG_Print( data );

	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> GamesFound
	// for( 1 .. GamesFound )
	//		2 bytes				-> GameType
	//		2 bytes				-> Parameter
	//		4 bytes				-> Language ID
	//		2 bytes				-> AF_INET
	//		2 bytes				-> Port
	//		4 bytes				-> IP
	//		4 bytes				-> zeros
	//		4 bytes				-> zeros
	//		4 bytes				-> Status
	//		4 bytes				-> ElapsedTime
	//		null term string	-> GameName
	//		1 byte				-> GamePassword
	//		1 byte				-> SlotsTotal
	//		8 bytes				-> HostCounter (ascii hex format)
	//		null term string	-> StatString

	vector<CIncomingGameHost *> Games;

	if( ValidateLength( data ) && data.size( ) >= 8 )
	{
		unsigned int i = 8;
		uint32_t GamesFound = UTIL_ByteArrayToUInt32( data, false, 4 );

		while( GamesFound > 0 )
		{
			GamesFound--;

			if( data.size( ) < i + 33 )
				break;

			uint16_t GameType = UTIL_ByteArrayToUInt16( data, false, i );
			i += 2;
			uint16_t Parameter = UTIL_ByteArrayToUInt16( data, false, i );
			i += 2;
			uint32_t LanguageID = UTIL_ByteArrayToUInt32( data, false, i );
			i += 4;
			// AF_INET
			i += 2;
			uint16_t Port = UTIL_ByteArrayToUInt16( data, true, i );
			i += 2;
			BYTEARRAY IP = BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 );
			i += 4;
			// zeros
			i += 4;
			// zeros
			i += 4;
			uint32_t Status = UTIL_ByteArrayToUInt32( data, false, i );
			i += 4;
			uint32_t ElapsedTime = UTIL_ByteArrayToUInt32( data, false, i );
			i += 4;
			BYTEARRAY GameName = UTIL_ExtractCString( data, i );
			i += GameName.size( ) + 1;

			if( data.size( ) < i + 1 )
				break;

			BYTEARRAY GamePassword = UTIL_ExtractCString( data, i );
			i += GamePassword.size( ) + 1;

			if( data.size( ) < i + 10 )
				break;

			// SlotsTotal is in ascii hex format

			unsigned char SlotsTotal = data[i];
			unsigned int c;
			stringstream SS;
			SS << string( 1, SlotsTotal );
			SS >> hex >> c;
			SlotsTotal = c;
			i++;

			// HostCounter is in reverse ascii hex format
			// e.g. 1  is "10000000"
			// e.g. 10 is "a0000000"
			// extract it, reverse it, parse it, construct a single uint32_t

			BYTEARRAY HostCounterRaw = BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 8 );
			string HostCounterString = string( HostCounterRaw.rbegin( ), HostCounterRaw.rend( ) );
			uint32_t HostCounter = 0;

			for( int j = 0; j < 4; j++ )
			{
				unsigned int c;
				stringstream SS;
				SS << HostCounterString.substr( j * 2, 2 );
				SS >> hex >> c;
				HostCounter |= c << ( 24 - j * 8 );
			}

			i += 8;
			BYTEARRAY StatString = UTIL_ExtractCString( data, i );
			i += StatString.size( ) + 1;
            string IPString = string( );

            if( IP.size( ) >= 4 )
            {
                for( unsigned int i = 0; i < 4; i++ )
                {
                    IPString += UTIL_ToString( (unsigned int)IP[i] );

                    if( i < 3 )
                        IPString += ".";
                }
            }

            Games.push_back( new CIncomingGameHost( m_UNM, m_Server, GameType, Parameter, LanguageID, Port, IPString, Status, ElapsedTime, string( GameName.begin( ), GameName.end( ) ), SlotsTotal, HostCounter, StatString ) );
		}
	}

	return Games;
}

bool CBNETProtocol::RECEIVE_SID_ENTERCHAT( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// null terminated string	-> UniqueName

	if( ValidateLength( data ) && data.size( ) >= 5 )
	{
		m_UniqueName = UTIL_ExtractCString( data, 4 );
		return true;
	}

	return false;
}

CIncomingChatEvent *CBNETProtocol::RECEIVE_SID_CHATEVENT( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> EventID
	// 4 bytes					-> UserFlags
	// 4 bytes					-> Ping
	// 12 bytes					-> ???
	// null terminated string	-> User
	// null terminated string	-> Message

	if( ValidateLength( data ) && data.size( ) >= 29 )
	{
		BYTEARRAY EventID = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );
		BYTEARRAY UserFlags = BYTEARRAY( data.begin( ) + 8, data.begin( ) + 12 );
		BYTEARRAY Ping = BYTEARRAY( data.begin( ) + 12, data.begin( ) + 16 );
		BYTEARRAY User = UTIL_ExtractCString( data, 28 );
		BYTEARRAY Message = UTIL_ExtractCString( data, User.size( ) + 29 );

		switch( UTIL_ByteArrayToUInt32( EventID, false ) )
		{
			case CBNETProtocol::EID_SHOWUSER:
			case CBNETProtocol::EID_JOIN:
			case CBNETProtocol::EID_LEAVE:
			case CBNETProtocol::EID_WHISPER:
			case CBNETProtocol::EID_TALK:
			case CBNETProtocol::EID_BROADCAST:
			case CBNETProtocol::EID_CHANNEL:
			case CBNETProtocol::EID_USERFLAGS:
			case CBNETProtocol::EID_WHISPERSENT:
			case CBNETProtocol::EID_CHANNELFULL:
			case CBNETProtocol::EID_CHANNELDOESNOTEXIST:
			case CBNETProtocol::EID_CHANNELRESTRICTED:
			case CBNETProtocol::EID_INFO:
			case CBNETProtocol::EID_ERROR:
			case CBNETProtocol::EID_EMOTE:
            return new CIncomingChatEvent( static_cast<CBNETProtocol::IncomingChatEvent>(UTIL_ByteArrayToUInt32( EventID, false )), UTIL_ByteArrayToUInt32( UserFlags, false ), UTIL_ByteArrayToUInt32( Ping, false ), string( User.begin( ), User.end( ) ), string( Message.begin( ), Message.end( ) ) );
		}

	}

	return nullptr;
}

bool CBNETProtocol::RECEIVE_SID_CHECKAD( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length

	return ValidateLength( data );
}

bool CBNETProtocol::RECEIVE_SID_STARTADVEX3( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Status

	if( ValidateLength( data ) && data.size( ) >= 8 )
	{
		BYTEARRAY Status = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );

		if( UTIL_ByteArrayToUInt32( Status, false ) == 0 )
			return true;
	}

	return false;
}

BYTEARRAY CBNETProtocol::RECEIVE_SID_PING( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Ping

	if( ValidateLength( data ) && data.size( ) >= 8 )
		return BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );

	return BYTEARRAY( );
}

bool CBNETProtocol::RECEIVE_SID_LOGONRESPONSE( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Status

	if( ValidateLength( data ) && data.size( ) >= 8 )
	{
		BYTEARRAY Status = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );

		if( UTIL_ByteArrayToUInt32( Status, false ) == 1 )
			return true;
	}

	return false;
}

bool CBNETProtocol::RECEIVE_SID_AUTH_INFO( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> LogonType
	// 4 bytes					-> ServerToken
	// 4 bytes					-> ???
	// 8 bytes					-> MPQFileTime
	// null terminated string	-> IX86VerFileName
	// null terminated string	-> ValueStringFormula

	if( ValidateLength( data ) && data.size( ) >= 25 )
	{
		m_LogonType = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );
		m_ServerToken = BYTEARRAY( data.begin( ) + 8, data.begin( ) + 12 );
		m_MPQFileTime = BYTEARRAY( data.begin( ) + 16, data.begin( ) + 24 );
		m_IX86VerFileName = UTIL_ExtractCString( data, 24 );
		m_ValueStringFormula = UTIL_ExtractCString( data, m_IX86VerFileName.size( ) + 25 );
		return true;
	}

	return false;
}

bool CBNETProtocol::RECEIVE_SID_AUTH_CHECK( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> KeyState
	// null terminated string	-> KeyStateDescription

	if( ValidateLength( data ) && data.size( ) >= 9 )
	{
		m_KeyState = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );
		m_KeyStateDescription = UTIL_ExtractCString( data, 8 );

		if( UTIL_ByteArrayToUInt32( m_KeyState, false ) == KR_GOOD )
			return true;
	}

	return false;
}

bool CBNETProtocol::RECEIVE_SID_AUTH_ACCOUNTLOGON( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Status
	// if( Status == 0 )
	//		32 bytes			-> Salt
	//		32 bytes			-> ServerPublicKey

	if( ValidateLength( data ) && data.size( ) >= 8 )
	{
		BYTEARRAY status = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );

		if( UTIL_ByteArrayToUInt32( status, false ) == 0 && data.size( ) >= 72 )
		{
			m_Salt = BYTEARRAY( data.begin( ) + 8, data.begin( ) + 40 );
			m_ServerPublicKey = BYTEARRAY( data.begin( ) + 40, data.begin( ) + 72 );
			return true;
		}
	}

	return false;
}

bool CBNETProtocol::RECEIVE_SID_AUTH_ACCOUNTLOGONPROOF( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Status

	if( ValidateLength( data ) && data.size( ) >= 8 )
	{
        uint32_t Status = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );

        if( Status == 0 || Status == 0xE )
            return true;
	}

	return false;
}

BYTEARRAY CBNETProtocol::RECEIVE_SID_WARDEN( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// n bytes					-> Data

	if( ValidateLength( data ) && data.size( ) >= 4 )
		return BYTEARRAY( data.begin( ) + 4, data.end( ) );

	return BYTEARRAY( );
}

vector<CIncomingFriendList *> CBNETProtocol::RECEIVE_SID_FRIENDSLIST( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> Total
	// for( 1 .. Total )
	//		null term string	-> Account
	//		1 byte				-> Status
	//		1 byte				-> Area
	//		4 bytes				-> ???
	//		null term string	-> Location

	vector<CIncomingFriendList *> Friends;

	if( ValidateLength( data ) && data.size( ) >= 5 )
	{
		unsigned int i = 5;
		unsigned char Total = data[4];

		while( Total > 0 )
		{
			Total--;

			if( data.size( ) < i + 1 )
				break;

			BYTEARRAY Account = UTIL_ExtractCString( data, i );
			i += Account.size( ) + 1;

			if( data.size( ) < i + 7 )
				break;

			unsigned char Status = data[i];
			unsigned char Area = data[i + 1];
			i += 6;
			BYTEARRAY Location = UTIL_ExtractCString( data, i );
			i += Location.size( ) + 1;
			Friends.push_back( new CIncomingFriendList( string( Account.begin( ), Account.end( ) ), Status, Area, string( Location.begin( ), Location.end( ) ) ) );
		}
	}

	return Friends;
}

vector<CIncomingClanList *> CBNETProtocol::RECEIVE_SID_CLANMEMBERLIST( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> ???
	// 1 byte					-> Total
	// for( 1 .. Total )
	//		null term string	-> Name
	//		1 byte				-> Rank
	//		1 byte				-> Status
	//		null term string	-> Location

	vector<CIncomingClanList *> ClanList;

	if( ValidateLength( data ) && data.size( ) >= 9 )
	{
		unsigned int i = 9;
		unsigned char Total = data[8];

		while( Total > 0 )
		{
			Total--;

			if( data.size( ) < i + 1 )
				break;

			BYTEARRAY Name = UTIL_ExtractCString( data, i );
			i += Name.size( ) + 1;

			if( data.size( ) < i + 3 )
				break;

			unsigned char Rank = data[i];
			unsigned char Status = data[i + 1];
			i += 2;

			// in the original VB source the location string is read but discarded, so that's what I do here

			BYTEARRAY Location = UTIL_ExtractCString( data, i );
			i += Location.size( ) + 1;
			ClanList.push_back( new CIncomingClanList( string( Name.begin( ), Name.end( ) ), Rank, Status ) );
		}
	}

	return ClanList;
}

CIncomingClanList *CBNETProtocol::RECEIVE_SID_CLANMEMBERSTATUSCHANGE( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// null terminated string	-> Name
	// 1 byte					-> Rank
	// 1 byte					-> Status
	// null terminated string	-> Location

	if( ValidateLength( data ) && data.size( ) >= 5 )
	{
		BYTEARRAY Name = UTIL_ExtractCString( data, 4 );

		if( data.size( ) >= Name.size( ) + 7 )
		{
			unsigned char Rank = data[Name.size( ) + 5];
			unsigned char Status = data[Name.size( ) + 6];

			// in the original VB source the location string is read but discarded, so that's what I do here

			BYTEARRAY Location = UTIL_ExtractCString( data, Name.size( ) + 7 );
			return new CIncomingClanList( string( Name.begin( ), Name.end( ) ), Rank, Status );
		}
	}

	return nullptr;
}

////////////////////
// SEND FUNCTIONS //
////////////////////

BYTEARRAY CBNETProtocol::SEND_ICCUP_ANTIHACK_PRELOGIN_DATA( )
{
    BYTEARRAY Data;
    Data.push_back( 0xff ); Data.push_back( 0xf6 ); Data.push_back( 0x28 ); Data.push_back( 0x00 ); Data.push_back( 0x0c ); Data.push_back( 0xe1 ); Data.push_back( 0x7a ); Data.push_back( 0x97 ); Data.push_back( 0x0c ); Data.push_back( 0xe1 ); Data.push_back( 0x7b ); Data.push_back( 0x97 ); Data.push_back( 0x0c ); Data.push_back( 0xe1 ); Data.push_back( 0x7b ); Data.push_back( 0x97 ); Data.push_back( 0x0c ); Data.push_back( 0xe1 ); Data.push_back( 0x7b ); Data.push_back( 0x97 ); Data.push_back( 0x0c ); Data.push_back( 0xe1 ); Data.push_back( 0x7b ); Data.push_back( 0x97 ); Data.push_back( 0x0c ); Data.push_back( 0xe1 ); Data.push_back( 0x75 ); Data.push_back( 0xa9 ); Data.push_back( 0x03 ); Data.push_back( 0xe8 ); Data.push_back( 0x79 ); Data.push_back( 0x97 ); Data.push_back( 0x0c ); Data.push_back( 0xe1 ); Data.push_back( 0x7b ); Data.push_back( 0x97 ); Data.push_back( 0x0c ); Data.push_back( 0xe1 ); Data.push_back( 0x7b ); Data.push_back( 0x97 ); Data.push_back( 0xff ); Data.push_back( 0xf6 ); Data.push_back( 0x20 ); Data.push_back( 0x00 ); Data.push_back( 0x6d ); Data.push_back( 0xf2 ); Data.push_back( 0x71 ); Data.push_back( 0xe9 ); Data.push_back( 0x6d ); Data.push_back( 0xf2 ); Data.push_back( 0x76 ); Data.push_back( 0xe9 ); Data.push_back( 0x6d ); Data.push_back( 0xf2 ); Data.push_back( 0x76 ); Data.push_back( 0xe9 ); Data.push_back( 0x6d ); Data.push_back( 0xf2 ); Data.push_back( 0x76 ); Data.push_back( 0xe9 ); Data.push_back( 0x6d ); Data.push_back( 0xf2 ); Data.push_back( 0x76 ); Data.push_back( 0xe9 ); Data.push_back( 0x6d ); Data.push_back( 0xf2 ); Data.push_back( 0x78 ); Data.push_back( 0xd7 ); Data.push_back( 0x62 ); Data.push_back( 0xfb ); Data.push_back( 0x76 ); Data.push_back( 0xe9 );
    return Data;
}

/*BYTEARRAY CBNETProtocol::SEND_ICCUP_ANTIHACK_PRELOGIN_DATA2( )
{
    BYTEARRAY Data;
    Data.push_back( 0xff ); Data.push_back( 0xf6 ); Data.push_back( 0x28 ); Data.push_back( 0x00 ); Data.push_back( 0x40 ); Data.push_back( 0x71 ); Data.push_back( 0xa3 ); Data.push_back( 0x40 ); Data.push_back( 0x40 ); Data.push_back( 0x71 ); Data.push_back( 0xa2 ); Data.push_back( 0x40 ); Data.push_back( 0x40 ); Data.push_back( 0x71 ); Data.push_back( 0xa2 ); Data.push_back( 0x40 ); Data.push_back( 0x40 ); Data.push_back( 0x71 ); Data.push_back( 0xa2 ); Data.push_back( 0x40 ); Data.push_back( 0x40 ); Data.push_back( 0x71 ); Data.push_back( 0xa2 ); Data.push_back( 0x40 ); Data.push_back( 0x40 ); Data.push_back( 0x71 ); Data.push_back( 0xae ); Data.push_back( 0x7e ); Data.push_back( 0x4f ); Data.push_back( 0x78 ); Data.push_back( 0xa0 ); Data.push_back( 0x40 ); Data.push_back( 0x40 ); Data.push_back( 0x71 ); Data.push_back( 0xa2 ); Data.push_back( 0x40 ); Data.push_back( 0x40 ); Data.push_back( 0x71 ); Data.push_back( 0xa2 ); Data.push_back( 0x40 ); Data.push_back( 0xff ); Data.push_back( 0xf6 ); Data.push_back( 0x20 ); Data.push_back( 0x00 ); Data.push_back( 0x73 ); Data.push_back( 0xa7 ); Data.push_back( 0x8d ); Data.push_back( 0xf9 ); Data.push_back( 0x73 ); Data.push_back( 0xa7 ); Data.push_back( 0x8a ); Data.push_back( 0xf9 ); Data.push_back( 0x73 ); Data.push_back( 0xa7 ); Data.push_back( 0x8a ); Data.push_back( 0xf9 ); Data.push_back( 0x73 ); Data.push_back( 0xa7 ); Data.push_back( 0x8a ); Data.push_back( 0xf9 ); Data.push_back( 0x73 ); Data.push_back( 0xa7 ); Data.push_back( 0x8a ); Data.push_back( 0xf9 ); Data.push_back( 0x73 ); Data.push_back( 0xa7 ); Data.push_back( 0x86 ); Data.push_back( 0xc7 ); Data.push_back( 0x7c ); Data.push_back( 0xae ); Data.push_back( 0x8a ); Data.push_back( 0xf9 );
    return Data;
}

BYTEARRAY CBNETProtocol::SEND_ICCUP_SET_GAMELIST_FILTER( )
{
    BYTEARRAY Data;
    Data.push_back( 0xff ); Data.push_back( 0xf6 ); Data.push_back( 0x20 ); Data.push_back( 0x01 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x89 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0xe4 ); Data.push_back( 0x97 ); Data.push_back( 0x5b ); Data.push_back( 0x26 ); Data.push_back( 0xf8 ); Data.push_back( 0x82 ); Data.push_back( 0x6b ); Data.push_back( 0x0a ); Data.push_back( 0xef ); Data.push_back( 0x8a ); Data.push_back( 0x4e ); Data.push_back( 0x00 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x8d ); Data.push_back( 0xdd ); Data.push_back( 0x3b ); Data.push_back( 0x70 ); Data.push_back( 0xf7 ); Data.push_back( 0x35 ); Data.push_back( 0xee ); Data.push_back( 0xcb ); Data.push_back( 0x9b ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x7b ); Data.push_back( 0x53 ); Data.push_back( 0x5f ); Data.push_back( 0xc9 ); Data.push_back( 0x8b ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x37 ); Data.push_back( 0xc3 ); Data.push_back( 0xef ); Data.push_back( 0xcb ); Data.push_back( 0xa5 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x36 ); Data.push_back( 0xc3 ); Data.push_back( 0xef ); Data.push_back( 0xcb ); Data.push_back( 0x9b ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x6d ); Data.push_back( 0xc9 ); Data.push_back( 0xb1 ); Data.push_back( 0xc0 ); Data.push_back( 0xe2 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x55 ); Data.push_back( 0x07 ); Data.push_back( 0x67 ); Data.push_back( 0x4a ); Data.push_back( 0xa8 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x3c ); Data.push_back( 0xc3 ); Data.push_back( 0xef ); Data.push_back( 0xcb ); Data.push_back( 0x9e ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x52 ); Data.push_back( 0x07 ); Data.push_back( 0x67 ); Data.push_back( 0x4a ); Data.push_back( 0xae ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x34 ); Data.push_back( 0xc3 ); Data.push_back( 0xef ); Data.push_back( 0xcb ); Data.push_back( 0x9e ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x17 ); Data.push_back( 0x53 ); Data.push_back( 0x5f ); Data.push_back( 0xc9 ); Data.push_back( 0x8b ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0xf5 ); Data.push_back( 0x35 ); Data.push_back( 0xee ); Data.push_back( 0xcb ); Data.push_back( 0x9b ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0xf4 ); Data.push_back( 0x35 ); Data.push_back( 0xee ); Data.push_back( 0xcb ); Data.push_back( 0x9e ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x51 ); Data.push_back( 0xc9 ); Data.push_back( 0xb1 ); Data.push_back( 0xc0 ); Data.push_back( 0xdf ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 ); Data.push_back( 0x34 ); Data.push_back( 0x79 ); Data.push_back( 0x81 ); Data.push_back( 0xe3 );
    return Data;
}*/

BYTEARRAY CBNETProtocol::SEND_PROTOCOL_INITIALIZE_SELECTOR( )
{
	BYTEARRAY packet;
	packet.push_back( 1 );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_NULL( )
{
	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );	// BNET header constant
	packet.push_back( SID_NULL );				// SID_NULL
	packet.push_back( 0 );						// packet length will be assigned later
	packet.push_back( 0 );						// packet length will be assigned later
	AssignLength( packet );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_STOPADV( )
{
	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );	// BNET header constant
	packet.push_back( SID_STOPADV );			// SID_STOPADV
	packet.push_back( 0 );						// packet length will be assigned later
	packet.push_back( 0 );						// packet length will be assigned later
	AssignLength( packet );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_GETADVLISTEX( string gameName, uint32_t numGames, bool iccup )
{
    unsigned char Condition1[]	= {   0,   0 };
    unsigned char Condition2[]	= {   0,   0 };
    unsigned char Condition3[]	= {   0,   0,   0,   0 };
    unsigned char Condition4[]	= {   0,   0,   0,   0 };

    if( gameName.empty( ) )
    {
        if( !iccup )
        {
            Condition1[0] = 0;		Condition1[1] = 224;
            Condition2[0] = 127;	Condition2[1] = 0;
        }
        else
        {
            Condition1[0] = 0;		Condition1[1] = 224;
            Condition2[0] = 03;     Condition2[1] = 0;
            Condition3[0] = 255;	Condition3[1] = 255;	Condition3[2] = 255;	Condition3[3] = 255;
            Condition4[0] = 0;      Condition4[1] = 0;		Condition4[2] = 51;		Condition4[3] = 255;
        }
    }
    else
    {
        Condition1[0] = 255;	Condition1[1] = 3;
        Condition2[0] = 0;		Condition2[1] = 0;
        Condition3[0] = 255;	Condition3[1] = 3;		Condition3[2] = 0;		Condition3[3] = 0;
        numGames = 1;
    }

	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );			// BNET header constant
	packet.push_back( SID_GETADVLISTEX );				// SID_GETADVLISTEX
	packet.push_back( 0 );								// packet length will be assigned later
	packet.push_back( 0 );								// packet length will be assigned later
	UTIL_AppendByteArray( packet, Condition1, 2 );
	UTIL_AppendByteArray( packet, Condition2, 2 );
	UTIL_AppendByteArray( packet, Condition3, 4 );
	UTIL_AppendByteArray( packet, Condition4, 4 );
	UTIL_AppendByteArray( packet, numGames, false );	// maximum number of games to list
	UTIL_AppendByteArrayFast( packet, gameName );		// Game Name
	packet.push_back( 0 );								// Game Password is NULL
	packet.push_back( 0 );								// Game Stats is NULL
	AssignLength( packet );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_ENTERCHAT( )
{
    BYTEARRAY packet;
    packet.push_back( BNET_HEADER_CONSTANT );	// BNET header constant
    packet.push_back( SID_ENTERCHAT );			// SID_ENTERCHAT
    packet.push_back( 0 );						// packet length will be assigned later
    packet.push_back( 0 );						// packet length will be assigned later
    packet.push_back( 0 );						// Account Name is NULL on Warcraft III/The Frozen Throne
    packet.push_back( 0 );						// Stat String is NULL on CDKEY'd products
    AssignLength( packet );
    return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_JOINCHANNEL( string channel )
{
	unsigned char NoCreateJoin[] = { 2, 0, 0, 0 };
	unsigned char FirstJoin[] = { 1, 0, 0, 0 };
	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );				// BNET header constant
	packet.push_back( SID_JOINCHANNEL );					// SID_JOINCHANNEL
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later

	if( channel.size( ) > 0 )
		UTIL_AppendByteArray( packet, NoCreateJoin, 4 );	// flags for no create join
	else
		UTIL_AppendByteArray( packet, FirstJoin, 4 );		// flags for first join

	UTIL_AppendByteArrayFast( packet, channel );
	AssignLength( packet );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_CHATCOMMAND( string command, unsigned char manual, unsigned char botid )
{
    BYTEARRAY packet;
    packet.push_back( manual );						// костыльный способ как-то обнозначить данный пакет, перед отправкой это значение будет измененно на BNET_HEADER_CONSTANT
    packet.push_back( SID_CHATCOMMAND );			// SID_CHATCOMMAND
    packet.push_back( 0 );                          // packet length will be assigned later
	packet.push_back( 0 );							// packet length will be assigned later
	UTIL_AppendByteArrayFast( packet, command );	// Message
	AssignLength( packet );
    packet.back( ) = botid;				// костыльный способ как-то обнозначить данный пакет, перед отправкой это значение будет измененно на null
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_CHECKAD( )
{
	unsigned char Zeros[] = { 0, 0, 0, 0 };

	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );	// BNET header constant
	packet.push_back( SID_CHECKAD );			// SID_CHECKAD
	packet.push_back( 0 );						// packet length will be assigned later
	packet.push_back( 0 );						// packet length will be assigned later
	UTIL_AppendByteArray( packet, Zeros, 4 );	// ???
	UTIL_AppendByteArray( packet, Zeros, 4 );	// ???
	UTIL_AppendByteArray( packet, Zeros, 4 );	// ???
	UTIL_AppendByteArray( packet, Zeros, 4 );	// ???
	AssignLength( packet );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_STARTADVEX3( string serverName, unsigned char state, BYTEARRAY mapGameType, BYTEARRAY mapFlags, BYTEARRAY mapWidth, BYTEARRAY mapHeight, string gameName, string hostName, uint32_t upTime, string mapPath, BYTEARRAY mapCRC, BYTEARRAY mapSHA1, uint32_t hostCounter )
{
	// todotodo: sort out how GameType works, the documentation is horrendous

/*

Game type tag: (read W3GS_GAMEINFO for this field)
 0x00000001 - Custom
 0x00000009 - Blizzard/Ladder
Map author: (mask 0x00006000) can be combined
*0x00002000 - Blizzard
 0x00004000 - Custom
Battle type: (mask 0x00018000) cant be combined
 0x00000000 - Battle
*0x00010000 - Scenario
Map size: (mask 0x000E0000) can be combined with 2 nearest values
 0x00020000 - Small
 0x00040000 - Medium
*0x00080000 - Huge
Observers: (mask 0x00700000) cant be combined
 0x00100000 - Allowed observers
 0x00200000 - Observers on defeat
*0x00400000 - No observers
Flags:
 0x00000800 - Private game flag (not used in game list)

*/

	unsigned char Unknown[] = { 255,  3,  0,  0 };
	unsigned char CustomGame[] = { 0,  0,  0,  0 };

	string HostCounterString = UTIL_ToHexString( hostCounter );

	if( HostCounterString.size( ) < 8 )
		HostCounterString.insert( 0, 8 - HostCounterString.size( ), '0' );

	HostCounterString = string( HostCounterString.rbegin( ), HostCounterString.rend( ) );
	BYTEARRAY packet;

	// make the stat string

	BYTEARRAY StatString;
	UTIL_AppendByteArrayFast( StatString, mapFlags );
	StatString.push_back( 0 );
	UTIL_AppendByteArrayFast( StatString, mapWidth );
	UTIL_AppendByteArrayFast( StatString, mapHeight );
	UTIL_AppendByteArrayFast( StatString, mapCRC );

	if( mapPath.size( ) > 53 )
	{
		CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - mapPath size " + UTIL_ToString( mapPath.size( ) ) + " > 53 - AUTOFIXED" );
		string mapPathTest;
		mapPathTest = mapPath.substr( 0, 49 ) + mapPath.substr( mapPath.size( ) - 4 );
		mapPath = mapPathTest;
	}

	UTIL_AppendByteArrayFast( StatString, mapPath );
	UTIL_AppendByteArrayFast( StatString, hostName );
	StatString.push_back( 0 );

	if( !Patch21( ) )
		UTIL_AppendByteArrayFast( StatString, mapSHA1 );

	StatString = UTIL_EncodeStatString( StatString );

	if( mapGameType.size( ) == 4 )
	{
		if( mapFlags.size( ) == 4 )
		{
			if( mapWidth.size( ) == 2 )
			{
				if( mapHeight.size( ) == 2 )
				{
					if( !gameName.empty( ) )
					{
						if( !hostName.empty( ) )
						{
							if( !mapPath.empty( ) )
							{
								if( mapCRC.size( ) == 4 )
								{
									if( mapSHA1.size( ) == 20 )
									{
										if( StatString.size( ) < 128 )
										{
											if( HostCounterString.size( ) == 8 )
											{
												// make the rest of the packet

												packet.push_back( BNET_HEADER_CONSTANT );						// BNET header constant
												packet.push_back( SID_STARTADVEX3 );							// SID_STARTADVEX3
												packet.push_back( 0 );											// packet length will be assigned later
												packet.push_back( 0 );											// packet length will be assigned later
												packet.push_back( state );										// State (16 = public, 17 = private, 18 = close)
												packet.push_back( 0 );											// State continued...
												packet.push_back( 0 );											// State continued...
												packet.push_back( 0 );											// State continued...
												UTIL_AppendByteArray( packet, upTime, false );					// time since creation
												UTIL_AppendByteArrayFast( packet, mapGameType );				// Game Type, Parameter
												UTIL_AppendByteArray( packet, Unknown, 4 );						// ???
												UTIL_AppendByteArray( packet, CustomGame, 4 );					// Custom Game
												UTIL_AppendByteArrayFast( packet, gameName );					// Game Name
												packet.push_back( 0 );											// Game Password is NULL
												packet.push_back( 98 );											// Slots Free (ascii 98 = char 'b' = 11 slots free) - note: do not reduce this as this is the # of PID's Warcraft III will allocate
												UTIL_AppendByteArrayFast( packet, HostCounterString, false );	// Host Counter
												UTIL_AppendByteArrayFast( packet, StatString );					// Stat String
												packet.push_back( 0 );											// Stat String null terminator (the stat string is encoded to remove all even numbers i.e. zeros)
                                                AssignLength( packet );
											}
											else
                                                CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - HostCounterString" );
										}
										else
                                            CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - StatString size " + UTIL_ToString( StatString.size( ) ) + " > 127" );
									}
									else
                                        CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - mapSHA1" );
								}
								else
                                    CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - mapCRC" );
							}
							else
                                CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - mapPath empty" );
						}
						else
                            CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - hostName" );
					}
					else
                        CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - gameName" );
				}
				else
                    CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - mapHeight" );
			}
			else
                CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - mapWidth" );
		}
		else
            CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - mapFlags" );
	}
	else
        CONSOLE_Print( "[BNET: " + serverName + "] SEND_SID_STARTADVEX3 - mapGameType" );

    return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_NOTIFYJOIN( string gameName )
{
	unsigned char ProductID[] = { 0, 0, 0, 0 };
	unsigned char ProductVersion[] = { 14, 0, 0, 0 };	// Warcraft III is 14
	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );			// BNET header constant
	packet.push_back( SID_NOTIFYJOIN );					// SID_NOTIFYJOIN
	packet.push_back( 0 );								// packet length will be assigned later
	packet.push_back( 0 );								// packet length will be assigned later
	UTIL_AppendByteArray( packet, ProductID, 4 );		// Product ID
	UTIL_AppendByteArray( packet, ProductVersion, 4 );	// Product Version
	UTIL_AppendByteArrayFast( packet, gameName );		// Game Name
	packet.push_back( 0 );								// Game Password is NULL
	AssignLength( packet );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_PING( BYTEARRAY pingValue )
{
	BYTEARRAY packet;

	if( pingValue.size( ) == 4 )
	{
		packet.push_back( BNET_HEADER_CONSTANT );		// BNET header constant
		packet.push_back( SID_PING );					// SID_PING
		packet.push_back( 0 );							// packet length will be assigned later
		packet.push_back( 0 );							// packet length will be assigned later
		UTIL_AppendByteArrayFast( packet, pingValue );	// Ping Value
		AssignLength( packet );
	}
	else
		CONSOLE_Print( "[BNETPROTO] invalid parameters passed to SEND_SID_PING" );

	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_LOGONRESPONSE( BYTEARRAY clientToken, BYTEARRAY serverToken, BYTEARRAY passwordHash, string accountName )
{
	// todotodo: check that the passed BYTEARRAY sizes are correct (don't know what they should be right now so I can't do this today)

	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );			// BNET header constant
	packet.push_back( SID_LOGONRESPONSE );				// SID_LOGONRESPONSE
	packet.push_back( 0 );								// packet length will be assigned later
	packet.push_back( 0 );								// packet length will be assigned later
	UTIL_AppendByteArrayFast( packet, clientToken );	// Client Token
	UTIL_AppendByteArrayFast( packet, serverToken );	// Server Token
	UTIL_AppendByteArrayFast( packet, passwordHash );	// Password Hash
	UTIL_AppendByteArrayFast( packet, accountName );	// Account Name
	AssignLength( packet );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_NETGAMEPORT( uint16_t serverPort )
{
	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );			// BNET header constant
	packet.push_back( SID_NETGAMEPORT );				// SID_NETGAMEPORT
	packet.push_back( 0 );								// packet length will be assigned later
	packet.push_back( 0 );								// packet length will be assigned later
	UTIL_AppendByteArray( packet, serverPort, false );	// local game server port
	AssignLength( packet );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_INFO( unsigned char ver, bool TFT, uint32_t localeID, string countryAbbrev, string country )
{
	unsigned char ProtocolID[] = { 0,   0,   0,   0 };
	unsigned char PlatformID[] = { 54,  56,  88,  73 };	// "IX86"
	unsigned char ProductID_ROC[] = { 51,  82,  65,  87 };	// "WAR3"
	unsigned char ProductID_TFT[] = { 80,  88,  51,  87 };	// "W3XP"
	unsigned char Version[] = { ver,   0,   0,   0 };
	unsigned char Language[] = { 83,  85, 110, 101 };	// "enUS"
	unsigned char LocalIP[] = { 127,   0,   0,   1 };
	unsigned char TimeZoneBias[] = { 44,   1,   0,   0 };	// 300 minutes (GMT -0500)
	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );				// BNET header constant
	packet.push_back( SID_AUTH_INFO );						// SID_AUTH_INFO
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	UTIL_AppendByteArray( packet, ProtocolID, 4 );			// Protocol ID
	UTIL_AppendByteArray( packet, PlatformID, 4 );			// Platform ID

	if( TFT )
		UTIL_AppendByteArray( packet, ProductID_TFT, 4 );	// Product ID (TFT)
	else
		UTIL_AppendByteArray( packet, ProductID_ROC, 4 );	// Product ID (ROC)

	UTIL_AppendByteArray( packet, Version, 4 );				// Version
	UTIL_AppendByteArray( packet, Language, 4 );			// Language (hardcoded as enUS to ensure battle.net sends the bot messages in English)
	UTIL_AppendByteArray( packet, LocalIP, 4 );				// Local IP for NAT compatibility
	UTIL_AppendByteArray( packet, TimeZoneBias, 4 );		// Time Zone Bias
	UTIL_AppendByteArray( packet, localeID, false );		// Locale ID
	UTIL_AppendByteArray( packet, localeID, false );		// Language ID (copying the locale ID should be sufficient since we don't care about sublanguages)
	UTIL_AppendByteArrayFast( packet, countryAbbrev );		// Country Abbreviation
	UTIL_AppendByteArrayFast( packet, country );			// Country
	AssignLength( packet );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_CHECK( string serverName, bool TFT, BYTEARRAY clientToken, BYTEARRAY exeVersion, BYTEARRAY exeVersionHash, BYTEARRAY keyInfoROC, BYTEARRAY keyInfoTFT, string exeInfo, string keyOwnerName )
{
	uint32_t NumKeys = 0;

	if( TFT )
		NumKeys = 2;
	else
		NumKeys = 1;

	BYTEARRAY packet;

	if( clientToken.size( ) == 4 && exeVersion.size( ) == 4 && exeVersionHash.size( ) == 4 && keyInfoROC.size( ) == 36 && ( !TFT || keyInfoTFT.size( ) == 36 ) )
	{
		packet.push_back( BNET_HEADER_CONSTANT );			// BNET header constant
		packet.push_back( SID_AUTH_CHECK );					// SID_AUTH_CHECK
		packet.push_back( 0 );								// packet length will be assigned later
		packet.push_back( 0 );								// packet length will be assigned later
		UTIL_AppendByteArrayFast( packet, clientToken );	// Client Token
		UTIL_AppendByteArrayFast( packet, exeVersion );		// EXE Version
		UTIL_AppendByteArrayFast( packet, exeVersionHash );	// EXE Version Hash
		UTIL_AppendByteArray( packet, NumKeys, false );		// number of keys in this packet
        UTIL_AppendByteArray( packet, static_cast<uint32_t>(0), false );	// boolean Using Spawn (32 bit)
		UTIL_AppendByteArrayFast( packet, keyInfoROC );		// ROC Key Info

		if( TFT )
			UTIL_AppendByteArrayFast( packet, keyInfoTFT );	// TFT Key Info

		UTIL_AppendByteArrayFast( packet, exeInfo );		// EXE Info
		UTIL_AppendByteArrayFast( packet, keyOwnerName );	// CD Key Owner Name
		AssignLength( packet );
	}
	else
        CONSOLE_Print( "[BNETPROTO: " + serverName + "] invalid parameters passed to SEND_SID_AUTH_CHECK" );

	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_ACCOUNTLOGON( string serverName, BYTEARRAY clientPublicKey, string accountName )
{
	BYTEARRAY packet;

	if( clientPublicKey.size( ) == 32 )
	{
		packet.push_back( BNET_HEADER_CONSTANT );				// BNET header constant
		packet.push_back( SID_AUTH_ACCOUNTLOGON );				// SID_AUTH_ACCOUNTLOGON
		packet.push_back( 0 );									// packet length will be assigned later
		packet.push_back( 0 );									// packet length will be assigned later
		UTIL_AppendByteArrayFast( packet, clientPublicKey );	// Client Key
		UTIL_AppendByteArrayFast( packet, accountName );		// Account Name
		AssignLength( packet );
	}
	else
        CONSOLE_Print( "[BNETPROTO: " + serverName + "] invalid parameters passed to SEND_SID_AUTH_ACCOUNTLOGON" );

	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_ACCOUNTLOGONPROOF( string serverName, BYTEARRAY clientPasswordProof )
{
	BYTEARRAY packet;

	if( clientPasswordProof.size( ) == 20 )
	{
		packet.push_back( BNET_HEADER_CONSTANT );					// BNET header constant
		packet.push_back( SID_AUTH_ACCOUNTLOGONPROOF );				// SID_AUTH_ACCOUNTLOGONPROOF
		packet.push_back( 0 );										// packet length will be assigned later
		packet.push_back( 0 );										// packet length will be assigned later
		UTIL_AppendByteArrayFast( packet, clientPasswordProof );	// Client Password Proof
		AssignLength( packet );
	}
	else
        CONSOLE_Print( "[BNETPROTO: " + serverName + "] invalid parameters passed to SEND_SID_AUTH_ACCOUNTLOGON" );

	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_ACCOUNTLOGONPROOF_ENTGAMING( string password )
{
	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );					// BNET header constant
	packet.push_back( SID_AUTH_ACCOUNTLOGONPROOF );				// SID_AUTH_ACCOUNTLOGONPROOF
	packet.push_back( 0 );										// packet length will be assigned later
	packet.push_back( 0 );										// packet length will be assigned later

	// empty proof indicates entgaming hash type
	for( int i = 0; i < 20; i++ )
		packet.push_back( 0 );

	UTIL_AppendByteArrayFast( packet, password );
	AssignLength( packet );

	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_WARDEN( BYTEARRAY wardenResponse )
{
	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );			// BNET header constant
	packet.push_back( SID_WARDEN );						// SID_WARDEN
	packet.push_back( 0 );								// packet length will be assigned later
	packet.push_back( 0 );								// packet length will be assigned later
	UTIL_AppendByteArrayFast( packet, wardenResponse );	// warden response
	AssignLength( packet );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_FRIENDSLIST( )
{
	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );	// BNET header constant
	packet.push_back( SID_FRIENDSLIST );		// SID_FRIENDSLIST
	packet.push_back( 0 );						// packet length will be assigned later
	packet.push_back( 0 );						// packet length will be assigned later
	AssignLength( packet );
	return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_CLANMEMBERLIST( )
{
	unsigned char Cookie[] = { 0, 0, 0, 0 };

	BYTEARRAY packet;
	packet.push_back( BNET_HEADER_CONSTANT );	// BNET header constant
	packet.push_back( SID_CLANMEMBERLIST );		// SID_CLANMEMBERLIST
	packet.push_back( 0 );						// packet length will be assigned later
	packet.push_back( 0 );						// packet length will be assigned later
	UTIL_AppendByteArray( packet, Cookie, 4 );	// cookie
	AssignLength( packet );
	return packet;
}

/////////////////////
// OTHER FUNCTIONS //
/////////////////////

bool CBNETProtocol::AssignLength( BYTEARRAY &content )
{
	// insert the actual length of the content array into bytes 3 and 4 (indices 2 and 3)

	BYTEARRAY LengthBytes;

	if( content.size( ) >= 4 && content.size( ) <= 65535 )
	{
        LengthBytes = UTIL_CreateByteArray( static_cast<uint16_t>(content.size( )), false );
		content[2] = LengthBytes[0];
		content[3] = LengthBytes[1];
		return true;
	}

	return false;
}

bool CBNETProtocol::ValidateLength( BYTEARRAY &content )
{
	// verify that bytes 3 and 4 (indices 2 and 3) of the content array describe the length

	uint16_t Length;
	BYTEARRAY LengthBytes;

	if( content.size( ) >= 4 && content.size( ) <= 65535 )
	{
		LengthBytes.push_back( content[2] );
		LengthBytes.push_back( content[3] );
		Length = UTIL_ByteArrayToUInt16( LengthBytes, false );

		if( Length == content.size( ) )
			return true;
	}

	return false;
}

//
// CIncomingGameHost
//

CIncomingGameHost::CIncomingGameHost( CUNM *nUNM, string nServer, uint16_t nGameType, uint16_t nParameter, uint32_t nLanguageID, uint16_t nPort, string nIPString, uint32_t nStatus, uint32_t nElapsedTime, string nGameName, unsigned char nSlotsTotal, uint32_t nHostCounter, BYTEARRAY &nStatString )
{
    m_UNM = nUNM;
    m_Server = nServer;
    m_MapDataRead = false;
    m_MapName = string( );
    m_MapAuthor = string( );
    m_MapDescription = string( );
    m_MapPlayers = string( );
	m_GameType = nGameType;
	m_Parameter = nParameter;
	m_LanguageID = nLanguageID;
	m_Port = nPort;
    m_IPString = nIPString;
    m_Status = nStatus;
	m_ElapsedTime = nElapsedTime;
	m_GameName = nGameName;
	m_SlotsTotal = nSlotsTotal;
	m_HostCounter = nHostCounter;
	m_StatString = nStatString;
    m_UniqueGameID = 0;
	m_ReceivedTime = GetTime( );
    m_Broadcasted = false;
    m_GProxy = false;

    if( m_IPString == "185.60.44.131" )
        m_GameIcon = 1;
    else if( m_IPString == "195.88.208.203" || m_IPString == "185.158.112.152" )
        m_GameIcon = 2;
    else
        m_GameIcon = 0;

    // decode stat string

    BYTEARRAY StatString = UTIL_DecodeStatString( m_StatString );
	BYTEARRAY MapFlags;
	BYTEARRAY MapWidth;
	BYTEARRAY MapHeight;
	BYTEARRAY MapCRC;
	BYTEARRAY MapPath;
	BYTEARRAY HostName;

	if( StatString.size( ) >= 14 )
	{
		unsigned int i = 13;
		MapFlags = BYTEARRAY( StatString.begin( ), StatString.begin( ) + 4 );
		MapWidth = BYTEARRAY( StatString.begin( ) + 5, StatString.begin( ) + 7 );
		MapHeight = BYTEARRAY( StatString.begin( ) + 7, StatString.begin( ) + 9 );
		MapCRC = BYTEARRAY( StatString.begin( ) + 9, StatString.begin( ) + 13 );
		MapPath = UTIL_ExtractCString( StatString, 13 );
        i += MapPath.size( ) + 1;
		m_MapFlags = UTIL_ByteArrayToUInt32( MapFlags, false );
		m_MapWidth = UTIL_ByteArrayToUInt16( MapWidth, false );
		m_MapHeight = UTIL_ByteArrayToUInt16( MapHeight, false );
		m_MapCRC = MapCRC;
		m_MapPath = string( MapPath.begin( ), MapPath.end( ) );
        m_MapPathName = UTIL_GetFileNameFromPath( m_MapPath );

		if( StatString.size( ) >= i + 1 )
		{
			HostName = UTIL_ExtractCString( StatString, i );
			m_HostName = string( HostName.begin( ), HostName.end( ) );
        }

        if( m_MapWidth == 1984 && m_MapHeight == 1984 )
            m_GProxy = true;
	}
}

CIncomingGameHost::~CIncomingGameHost( )
{

}

bool CIncomingGameHost::UpdateGame( CIncomingGameHost *game )
{
    m_GameType = game->GetGameType( );
    m_Parameter = game->GetParameter( );
    m_LanguageID = game->GetLanguageID( );
    m_Status = game->GetStatus( );
    m_ElapsedTime = game->GetElapsedTime( );
    m_SlotsTotal = game->GetSlotsTotal( );
    m_HostCounter = game->GetHostCounter( );
    m_ReceivedTime = GetTime( );
    bool UpdateStatString = false;
    bool UpdateMapSize = false;
    bool UpdateMap = false;

    if( m_MapFlags != game->GetMapFlags( ) )
    {
        m_MapFlags = game->GetMapFlags( );
        UpdateStatString = true;
    }

    if( m_MapWidth != game->GetMapWidth( ) )
    {
        m_MapWidth = game->GetMapWidth( );
        UpdateStatString = true;
        UpdateMapSize = true;
    }

    if( m_MapHeight != game->GetMapHeight( ) )
    {
        m_MapHeight = game->GetMapHeight( );
        UpdateStatString = true;
        UpdateMapSize = true;
    }

    if( m_MapCRC != game->GetMapCRC( ) )
    {
        m_MapCRC = game->GetMapCRC( );
        UpdateStatString = true;
    }

    if( m_MapPath != game->GetMapPath( ) )
    {
        m_MapPath = game->GetMapPath( );
        m_MapPathName = game->GetMapPathName( );
        UpdateStatString = true;
        UpdateMap = true;
    }

    if( m_HostName != game->GetHostName( ) )
    {
        m_HostName = game->GetHostName( );
        UpdateStatString = true;
    }

    if( UpdateStatString )
    {
        m_UNM->m_UDPSocket->Broadcast( 6112, m_UNM->m_GameProtocol->SEND_W3GS_DECREATEGAME( m_UniqueGameID ), "[GPROXY: " + m_GameName + "]" );
        m_GameName = game->GetGameName( );
        m_StatString = game->GetStatString( );

        if( UpdateMapSize )
        {
            if( m_MapWidth == 1984 && m_MapHeight == 1984 )
                m_GProxy = true;
            else
                m_GProxy = false;
        }

        if( UpdateMap )
        {
            m_MapDataRead = false;
            m_MapName = string( );
            m_MapAuthor = string( );
            m_MapDescription = string( );
            m_MapPlayers = string( );

            if( m_Broadcasted )
            {
                m_Broadcasted = false;
                return true;
            }
        }
    }
    else if( !UTIL_CompareGameNames( m_GameName, game->GetGameName( ) ) )
    {
        m_UNM->m_UDPSocket->Broadcast( 6112, m_UNM->m_GameProtocol->SEND_W3GS_DECREATEGAME( m_UniqueGameID ), "[GPROXY: " + m_GameName + "]" );
        m_GameName = game->GetGameName( );
    }

    return false;
}

void CIncomingGameHost::ReadMapData( )
{
    m_MapDataRead = true;

    try
    {
        std::filesystem::path MapPath( m_UNM->m_MapPath );
        string Pattern = m_MapPathName;
        Pattern = UTIL_StringToLower( Pattern );

        if( !std::filesystem::exists( MapPath ) )
            MapDataReadError( "error listing maps - map path doesn't exist" );
        else
        {
            bool mapfound = false;
            string FilePath;
            string FileName ;
            using convert_typeX = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_typeX, wchar_t> converterX;
            std::filesystem::recursive_directory_iterator EndIterator;

            for( std::filesystem::recursive_directory_iterator i( MapPath ); i != EndIterator; i++ )
            {
                if( !is_directory( i->status( ) ) && ( i->path( ).extension( ) == ".w3x" || i->path( ).extension( ) == ".w3m" ) )
                {
                    FileName = converterX.to_bytes( i->path( ).filename( ) );
                    FileName = UTIL_StringToLower( FileName );

                    if( FileName == Pattern )
                    {
                        mapfound = true;
                        FilePath = converterX.to_bytes( i->path( ) );

                        if( m_UNM->m_AllowAutoRenameMaps )
                            m_UNM->MapAutoRename( FilePath, FileName, m_Server + " " );

                        // hackhack: create a config file in memory with the required information to load the map

                        CConfig MapCFG;
                        MapCFG.Set( "map_path", "Maps\\Download\\" + FileName );
                        MapCFG.Set( "map_localpath", FileName );
                        m_UNM->m_Map->Load( &MapCFG, FileName, FilePath );

                        if( !m_UNM->m_Map->GetValid( ) )
                            MapDataReadError( "ошибка при загрузке карты [" + m_MapPathName + "]" );
                        else
                        {
                            m_MapName = UTIL_Wc3ColorStringFix( m_UNM->m_Map->GetMapName( ) );
                            m_MapAuthor = UTIL_Wc3ColorStringFix( m_UNM->m_Map->GetMapAuthor( ) );
                            m_MapDescription = UTIL_Wc3ColorStringFix( m_UNM->m_Map->GetMapDescription( ) );
                            m_MapPlayers = UTIL_Wc3ColorStringFix( m_UNM->m_Map->GetMapPlayers( ) );
                            m_MapTGA = m_UNM->m_Map->GetMapTGA( );
                        }

                        break;
                    }
                }
            }

            if( !mapfound )
                MapDataReadError( "карта [" + m_MapPathName + "] не найдена!" );
        }
    }
    catch( const exception &ex )
    {
        MapDataReadError( "error listing maps - caught exception [" + string( ex.what( ) ) + "]" );
    }
}

void CIncomingGameHost::MapDataReadError( string logmessage )
{
    CONSOLE_Print( m_Server + " " + logmessage );
    m_MapName = m_MapPathName;
    m_MapAuthor = "N/A";
    m_MapDescription = "N/A";
    m_MapPlayers = "N/A";
}

string CIncomingGameHost::GetMapName( )
{
    if( m_MapDataRead )
        return m_MapName;

    ReadMapData( );
    return m_MapName;
}

BYTEARRAY CIncomingGameHost::GetMapTGA( )
{
    if( m_MapDataRead )
        return m_MapTGA;

    ReadMapData( );
    return m_MapTGA;
}

string CIncomingGameHost::GetMapAuthor( )
{
    if( m_MapDataRead )
        return m_MapAuthor;

    ReadMapData( );
    return m_MapAuthor;
}

string CIncomingGameHost::GetMapDescription( )
{
    if( m_MapDataRead )
        return m_MapDescription;

    ReadMapData( );
    return m_MapDescription;
}

string CIncomingGameHost::GetMapPlayers( )
{
    if( m_MapDataRead )
        return m_MapPlayers;

    ReadMapData( );
    return m_MapPlayers;
}

//
// CIncomingChatEvent
//

CIncomingChatEvent::CIncomingChatEvent( CBNETProtocol::IncomingChatEvent nChatEvent, uint32_t nUserFlags, uint32_t nPing, string nUser, string nMessage )
{
	m_ChatEvent = nChatEvent;
	m_UserFlags = nUserFlags;
	m_Ping = nPing;
	m_User = nUser;
	m_Message = nMessage;
}

CIncomingChatEvent::~CIncomingChatEvent( )
{

}

//
// CIncomingFriendList
//

CIncomingFriendList::CIncomingFriendList( string nAccount, unsigned char nStatus, unsigned char nArea, string nLocation )
{
	m_Account = nAccount;
	m_Status = nStatus;
	m_Area = nArea;
	m_Location = nLocation;
}

CIncomingFriendList::~CIncomingFriendList( )
{

}

string CIncomingFriendList::GetDescription( )
{
	string Description;
	Description += GetAccount( ) + "\n";
	Description += ExtractStatus( GetStatus( ) ) + "\n";
	Description += ExtractArea( GetArea( ) ) + "\n";
	Description += ExtractLocation( GetLocation( ) ) + "\n\n";
	return Description;
}

string CIncomingFriendList::ExtractStatus( unsigned char status )
{
	string Result;

	if( status & 1 )
		Result += "<Mutual>";

	if( status & 2 )
		Result += "<DND>";

	if( status & 4 )
		Result += "<Away>";

	if( Result.empty( ) )
		Result = "<None>";

	return Result;
}

string CIncomingFriendList::ExtractArea( unsigned char area )
{
	switch( area )
	{
		case 0: return "<Offline>";
		case 1: return "<No Channel>";
		case 2: return "<In Channel>";
		case 3: return "<Public Game>";
		case 4: return "<Private Game>";
		case 5: return "<Private Game>";
	}

	return "<Unknown>";
}

string CIncomingFriendList::ExtractLocation( string location )
{
	string Result;

	if( location.substr( 0, 4 ) == "PX3W" )
		Result = location.substr( 4 );

	if( Result.empty( ) )
		Result = ".";

	return Result;
}

//
// CIncomingClanList
//

CIncomingClanList::CIncomingClanList( string nName, unsigned char nRank, unsigned char nStatus )
{
	m_Name = nName;
	m_Rank = nRank;
	m_Status = nStatus;
}

CIncomingClanList::~CIncomingClanList( )
{

}

string CIncomingClanList::GetRank( )
{
	switch( m_Rank )
	{
		case 0: return "Recruit";
		case 1: return "Peon";
		case 2: return "Grunt";
		case 3: return "Shaman";
		case 4: return "Chieftain";
	}

	return "Rank Unknown";
}

string CIncomingClanList::GetStatus( )
{
	if( m_Status == 0 )
		return "Offline";
	else
		return "Online";
}

string CIncomingClanList::GetDescription( )
{
	string Description;
	Description += GetName( ) + "\n";
	Description += GetStatus( ) + "\n";
	Description += GetRank( ) + "\n\n";
	return Description;
}
