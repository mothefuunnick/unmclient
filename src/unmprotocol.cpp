#include "unm.h"
#include "util.h"
#include "unmprotocol.h"

#ifdef UNM_SERVER
#include "unmuser.h"
#else
#include "gameplayer.h"
#endif

using namespace std;

bool AssignLength( BYTEARRAY &content )
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

bool ValidateLength( BYTEARRAY &content )
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

bool AssignLength32Bit( BYTEARRAY &content )
{
    // insert the actual length of the content array into bytes 3, 4, 5 and 6 (indices 2, 3, 4 and 5)

    BYTEARRAY LengthBytes;

    if( content.size( ) >= 6 && content.size( ) <= 4294967295 )
    {
        LengthBytes = UTIL_CreateByteArray( static_cast<uint32_t>( content.size( ) ), false );
        content[2] = LengthBytes[0];
        content[3] = LengthBytes[1];
        content[4] = LengthBytes[2];
        content[5] = LengthBytes[3];
        return true;
    }

    return false;
}

bool ValidateLength32Bit( BYTEARRAY &content )
{
    // verify that bytes 3, 4, 5 and 6 (indices 2, 3, 4 and 5) of the content array describe the length

    uint32_t Length;
    BYTEARRAY LengthBytes;

    if( content.size( ) >= 6 && content.size( ) <= 4294967295 )
    {
        LengthBytes.push_back( content[2] );
        LengthBytes.push_back( content[3] );
        LengthBytes.push_back( content[4] );
        LengthBytes.push_back( content[5] );
        Length = UTIL_ByteArrayToUInt32( LengthBytes, false );

        if( Length == content.size( ) )
            return true;
    }

    return false;
}

CUNMProtocol::CUNMProtocol( )
{
}

CUNMProtocol::~CUNMProtocol( )
{
}

////////////////////////////////
// RECEIVE FUNCTIONS (CLIENT) //
////////////////////////////////
#ifdef UNM_CLIENT
unsigned char CUNMProtocol::RECEIVE_CLIENT_INFO_RESPONSE( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> Result

	if( ValidateLength( data ) && data.size( ) >= 5 )
		return data[4];

	return 2;
}

unsigned char CUNMProtocol::RECEIVE_SMART_LOGON_BOT_RESPONSE( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> Result

	if( ValidateLength( data ) && data.size( ) >= 5 )
		return data[4];

    return SMART_LOGON_RESPONSE_UNNKOWN_ERROR;
}

unsigned char CUNMProtocol::RECEIVE_GAME_REFRESH_RESPONSE( BYTEARRAY data, uint32_t &GameNumber, uint32_t &GameCounter )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 bytes                  -> Result: 0 = успех (игра создана); 1 = успех (игра обновлена); 2 = успех, но предупреждение (частый рефреш); 3 = успех - игра обновлена, но предупреждение (bad packet); 4 = 1 & 2; 5 = ошибка - игра не создана (bad packet); 6 = ошибка, некорректно указан номер игры; 7 = неизвестная ошибка
    // 4 bytes					-> Game Number
    // 4 bytes					-> Game Counter

    GameNumber = 0;
    GameCounter = 0;

    if( ValidateLength( data ) && data.size( ) >= 13 )
    {
        GameNumber = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 5, data.begin( ) + 9 ), false );
        GameCounter = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 9, data.begin( ) + 13 ), false );
        return data[4];
    }

    return 7;
}

CIncomingChatEvent *CUNMProtocol::RECEIVE_CHAT_EVENT_RESPONSE( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> EventID
	// 4 bytes					-> UserFlags
	// 4 bytes					-> Ping
	// null terminated string	-> User
	// null terminated string	-> Message

	if( ValidateLength( data ) && data.size( ) >= 18 )
	{
		BYTEARRAY EventID = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );
		BYTEARRAY UserFlags = BYTEARRAY( data.begin( ) + 8, data.begin( ) + 12 );
		BYTEARRAY Ping = BYTEARRAY( data.begin( ) + 12, data.begin( ) + 16 );
		BYTEARRAY User = UTIL_ExtractCString( data, 16 );
		BYTEARRAY Message = UTIL_ExtractCString( data, User.size( ) + 17 );

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

unsigned char CUNMProtocol::RECEIVE_CHECK_JOINED_PLAYER_RESPONSE( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> Result

	if( ValidateLength( data ) && data.size( ) >= 5 )
		return data[4];

	return 0;
}

CPlayerInfo *CUNMProtocol::RECEIVE_REQUEST_JOIN_PLAYER_RESPONSE( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Game Number
	// null terminated string	-> User Name
	// null terminated string	-> User IP
	
	if( ValidateLength( data ) && data.size( ) >= 10 )
	{
		uint32_t GameNumber = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );
		BYTEARRAY UserName = UTIL_ExtractCString( data, 8 );
		BYTEARRAY UserIP = UTIL_ExtractCString( data, 9 + UserName.size( ) );
		return new CPlayerInfo( GameNumber, string( UserName.begin( ), UserName.end( ) ), string( UserIP.begin( ), UserIP.end( ) ) );
	}
	
	return nullptr;
}

uint32_t CUNMProtocol::RECEIVE_GAME_UNHOST_RESPONSE( BYTEARRAY data, uint32_t &GameCounter )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Game Number
    // 4 bytes					-> Game Counter

    GameCounter = 0;

    if( ValidateLength( data ) && data.size( ) >= 8 )
    {
        if( data.size( ) >= 12 )
            GameCounter = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 8, data.begin( ) + 12 ), false );

        return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );
    }

    return 0;
}

uint32_t CUNMProtocol::RECEIVE_GAME_UNHOST_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Game Number
    // 4 bytes					-> Game Counter

    if( ValidateLength( data ) && data.size( ) >= 8 )
        return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );

    return 0;
}

unsigned char CUNMProtocol::RECEIVE_LOGON_CLIENT_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = ошибка базы, 1 = логин не существует, 2 = неверный пароль, 3 = авторизация под данной версией клиента разрешена только бета-тестерам, 4 = успех, 5 = неизвестная ошибка

    if( ValidateLength( data ) && data.size( ) >= 5 )
        return data[4];

    return LOGON_RESPONSE_UNNKOWN_ERROR;
}

unsigned char CUNMProtocol::RECEIVE_REGISTRY_CLIENT_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = ошибка базы, 1 = логин занят, 2 = недопустимый логин, 3 = недопустимый пароль, 4 = успех, 5 = неизвестная ошибка

    if( ValidateLength( data ) && data.size( ) >= 5 )
        return data[4];

    return REGISTRY_RESPONSE_UNNKOWN_ERROR;
}

vector<CGameHeader *> CUNMProtocol::RECEIVE_GET_GAMELIST_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes                  -> number of games
    // for( i = 0; i < number of games; i++ )
    // 		4 byets					-> Game Counter
    //      1 byte					-> Slots Open
    //      1 byte					-> Slots Total
    //		null terminated string	-> Game Name
    //		null terminated string	-> Map Name
    //		null terminated string	-> Map FileName
    //		null terminated string	-> Map Code
    //		null terminated string	-> Map Genre
    //      4 bytes					-> Map Ratings
    //      4 bytes					-> Map Ratings Count
    //		null terminated string	-> Bot Name
    //		null terminated string	-> Bot Login
    //		null terminated string	-> Game Owner
    //		null terminated string	-> Stats

    vector<CGameHeader *> Games;
    Games.clear( );

    if( ValidateLength( data ) && data.size( ) >= 8 )
    {
        uint32_t NumberGames = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );
        uint32_t i = 8;

        while( NumberGames > 0 )
        {
            if( data.size( ) < i + 23 )
                return Games;

            NumberGames--;
            uint32_t GameCounter = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
            unsigned char SlotsOpen = data[i+4];
            unsigned char SlotsTotal = data[i+5];
            i += 6;
            BYTEARRAY GameName = UTIL_ExtractCString( data, i );
			i += GameName.size( ) + 1;

            if( data.size( ) < i + 16 )
                return Games;

            BYTEARRAY MapName = UTIL_ExtractCString( data, i );
			i += MapName.size( ) + 1;

            if( data.size( ) < i + 15 )
                return Games;

            BYTEARRAY MapFileName = UTIL_ExtractCString( data, i );
			i += MapFileName.size( ) + 1;

            if( data.size( ) < i + 14 )
                return Games;

            BYTEARRAY MapCode = UTIL_ExtractCString( data, i );
			i += MapCode.size( ) + 1;

            if( data.size( ) < i + 13 )
                return Games;

            BYTEARRAY MapGenre = UTIL_ExtractCString( data, i );
			i += MapGenre.size( ) + 1;

            if( data.size( ) < i + 12 )
                return Games;

            uint32_t MapRatings = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
            uint32_t MapRatingsCount = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i + 4, data.begin( ) + i + 8 ), false );
            i += 8;

            BYTEARRAY BotName = UTIL_ExtractCString( data, i );
			i += BotName.size( ) + 1;

            if( data.size( ) < i + 3 )
                return Games;

            BYTEARRAY BotLogin = UTIL_ExtractCString( data, i );
			i += BotLogin.size( ) + 1;

            if( data.size( ) < i + 2 )
                return Games;

            BYTEARRAY GameOwner = UTIL_ExtractCString( data, i );
			i += GameOwner.size( ) + 1;

            if( data.size( ) < i + 1 )
                return Games;

            BYTEARRAY Stats = UTIL_ExtractCString( data, i );
			i += Stats.size( ) + 1;
            Games.push_back( new CGameHeader( GameCounter, static_cast<uint32_t>(SlotsOpen), static_cast<uint32_t>(SlotsTotal), string( GameName.begin( ), GameName.end( ) ), string( MapName.begin( ), MapName.end( ) ), string( MapFileName.begin( ), MapFileName.end( ) ), string( MapCode.begin( ), MapCode.end( ) ), string( MapGenre.begin( ), MapGenre.end( ) ), MapRatings, MapRatingsCount, string( BotName.begin( ), BotName.end( ) ), string( BotLogin.begin( ), BotLogin.end( ) ), string( GameOwner.begin( ), GameOwner.end( ) ), string( Stats.begin( ), Stats.end( ) ) ) );
        }

        return Games;
    }

    return Games;
}

CGameData *CUNMProtocol::RECEIVE_GET_GAME_RESPONSE( BYTEARRAY data )
{
    // 2 bytes						-> Header
    // 2 bytes						-> Length
    // 4 bytes						-> Game Counter
    // 4 bytes						-> Game Number
    // 1 byte						-> TFT/RoC
    // 1 byte						-> War3 version
    // 1 byte						-> Game Status
    // 2 bytes						-> Host Port
    // 2 bytes						-> Map Width
    // 2 bytes						-> Map Height
    // 4 bytes						-> Map Game Type
    // 4 bytes						-> Map Game Flags
    // 4 bytes						-> Map CRC
    // 4 bytes						-> Host Counter
    // 4 bytes						-> Entry Key
    // 4 bytes						-> Up Time
    // 4 bytes						-> Creation Time
    // null terminated string		-> Game Name
    // null terminated string		-> Game Owner
    // null terminated string		-> Map Path
    // null terminated string		-> Host Name
    // null terminated string		-> Host IP
    // null terminated string		-> TGA Image
    // null terminated string		-> Map Players
    // null terminated string		-> Map Description
    // null terminated string		-> Map Name
    // null terminated string		-> Stats
    // null terminated string		-> Bot Name
    // null terminated string		-> Bot Login
    // null terminated string		-> Map Code
    // null terminated string		-> Map Genre
    // 4 bytes						-> Map Ratings
    // 4 bytes						-> Map Ratings Count
    // 1 byte						-> Slots Open
    // 1 byte						-> Slots Total
    // for( i = 0; i < Slots Total; i++ )
    // {
    // 		1 byte					-> Slot Download Status
    // 		1 byte					-> Slot Team
    // 		1 byte					-> Slot Colour
    // 		1 byte					-> Slot Race
    // 		1 byte					-> Slot Handicap
    // 		1 byte					-> Slot Status
    // 		null terminated string	-> Player Name
    // 		null terminated string	-> Player Server
    // 		4 bytes					-> Player Ping
    // }
    // 4 bytes                      -> Refresh Ticks < данное значение помещаем в самый конец, для того чтобы была возможность быстро его изменять

    if( ValidateLength( data ) && data.size( ) >= 63 )
    {
        uint32_t GameCounter = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );
        uint32_t GameNumber = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 8, data.begin( ) + 12 ), false );
        unsigned char TFT = data[12];
        unsigned char War3Version = data[13];
        unsigned char GameStatus = data[14];
        BYTEARRAY HostPort = BYTEARRAY( data.begin( ) + 15, data.begin( ) + 17 );
        BYTEARRAY MapWidth = BYTEARRAY( data.begin( ) + 17, data.begin( ) + 19 );
        BYTEARRAY MapHeight = BYTEARRAY( data.begin( ) + 19, data.begin( ) + 21 );
        BYTEARRAY MapGameType = BYTEARRAY( data.begin( ) + 21, data.begin( ) + 25 );
        BYTEARRAY MapGameFlags = BYTEARRAY( data.begin( ) + 25, data.begin( ) + 29 );
        BYTEARRAY MapCRC = BYTEARRAY( data.begin( ) + 29, data.begin( ) + 33 );
        BYTEARRAY HostCounter = BYTEARRAY( data.begin( ) + 33, data.begin( ) + 37 );
        BYTEARRAY EntryKey = BYTEARRAY( data.begin( ) + 37, data.begin( ) + 41 );
        BYTEARRAY UpTime = BYTEARRAY( data.begin( ) + 41, data.begin( ) + 45 );
        uint32_t CreationTime = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 45, data.begin( ) + 49 ), false );
        uint32_t i = 49;
        BYTEARRAY GameName = UTIL_ExtractCString( data, i );
        i += GameName.size( ) + 1;
        BYTEARRAY GameOwner = UTIL_ExtractCString( data, i );
        i += GameOwner.size( ) + 1;
        BYTEARRAY MapPath = UTIL_ExtractCString( data, i );
        i += MapPath.size( ) + 1;
        BYTEARRAY HostName = UTIL_ExtractCString( data, i );
        i += HostName.size( ) + 1;
        BYTEARRAY HostIP = UTIL_ExtractCString( data, i );
        i += HostIP.size( ) + 1;
        BYTEARRAY TGAImage = UTIL_ExtractCString( data, i );
        i += TGAImage.size( ) + 1;
        BYTEARRAY MapPlayers = UTIL_ExtractCString( data, i );
        i += MapPlayers.size( ) + 1;
        BYTEARRAY MapDescription = UTIL_ExtractCString( data, i );
        i += MapDescription.size( ) + 1;
        BYTEARRAY MapName = UTIL_ExtractCString( data, i );
        i += MapName.size( ) + 1;
        BYTEARRAY Stats = UTIL_ExtractCString( data, i );
        i += Stats.size( ) + 1;
        BYTEARRAY BotName = UTIL_ExtractCString( data, i );
        i += BotName.size( ) + 1;
        BYTEARRAY BotLogin = UTIL_ExtractCString( data, i );
        i += BotLogin.size( ) + 1;
        BYTEARRAY MapCode = UTIL_ExtractCString( data, i );
        i += MapCode.size( ) + 1;
        BYTEARRAY MapGenre = UTIL_ExtractCString( data, i );
        i += MapGenre.size( ) + 1;
        BYTEARRAY MapRatings = BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 );
        BYTEARRAY MapRatingsCount = BYTEARRAY( data.begin( ) + i + 4, data.begin( ) + i + 8 );
        unsigned char SlotsOpen = data[i+8];
        unsigned char SlotsTotal = data[i+9];
        i += 10;

        if( SlotsTotal == 0 || SlotsTotal > 24 )
            return nullptr;

        CGameData *GameData = new CGameData( GameCounter, GameNumber, TFT, War3Version, GameStatus, HostPort, MapWidth, MapHeight, MapGameType, MapGameFlags, MapCRC, HostCounter, EntryKey, UpTime, CreationTime, GameName, GameOwner, MapPath, HostName, HostIP, TGAImage, MapPlayers, MapDescription, MapName, Stats, MapCode, MapGenre, MapRatings, MapRatingsCount, SlotsOpen, SlotsTotal, { }, string( BotName.begin( ), BotName.end( ) ), string( BotLogin.begin( ), BotLogin.end( ) ), 0 );

        while( SlotsTotal > 0 )
        {
            SlotsTotal--;

            if( data.size( ) < i + 12 )
            {
                delete GameData;
                return nullptr;
            }

            unsigned char SlotDownloadStatus = data[i];
            unsigned char SlotTeam = data[i+1];
            unsigned char SlotColour = data[i+2];
            unsigned char SlotRace = data[i+3];
            unsigned char SlotHandicap = data[i+4];
            unsigned char SlotStatus = data[i+5];
            i += 6;
            BYTEARRAY PlayerName = UTIL_ExtractCString( data, i );
            i += PlayerName.size( ) + 1;

            if( data.size( ) < i + 5 )
            {
                delete GameData;
                return nullptr;
            }

            BYTEARRAY PlayerServer = UTIL_ExtractCString( data, i );
            i += PlayerServer.size( ) + 1;

            if( data.size( ) < i + 4 )
            {
                delete GameData;
                return nullptr;
            }

            BYTEARRAY PlayerPing = BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 );
            i += 4;
            GameData->m_GameSlots.push_back( CGameSlotData( SlotDownloadStatus, SlotTeam, SlotColour, SlotRace, SlotHandicap, SlotStatus, PlayerName, PlayerServer, PlayerPing ) );
        }

        if( data.size( ) < i + 4 )
        {
            delete GameData;
            return nullptr;
        }

        uint32_t RefreshTicks = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
        GameData->SetRefreshTicks( RefreshTicks );
        return GameData;
    }

    return nullptr;
}

vector<CBotInfo *> CUNMProtocol::RECEIVE_GET_BOTLIST_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes                  -> number of bots
    // for( i = 0; i < number of bots; i++ )
    //		null terminated string	-> bot name
    //		null terminated string	-> map list

    vector<CBotInfo *> Bots;
    Bots.clear( );

    if( ValidateLength( data ) && data.size( ) >= 8 )
    {
        uint32_t NumberBots = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );
        uint32_t i = 8;

        while( NumberBots > 0 )
        {
            if( data.size( ) < i + 2 )
                return Bots;

            NumberBots--;
            BYTEARRAY BotName = UTIL_ExtractCString( data, i );
            i += BotName.size( ) + 1;

            if( data.size( ) < i + 1 )
                return Bots;

            BYTEARRAY MapList = UTIL_ExtractCString( data, i );
            i += MapList.size( ) + 1;
            Bots.push_back( new CBotInfo( string( BotName.begin( ), BotName.end( ) ), string( MapList.begin( ), MapList.end( ) ) ) );
        }

        return Bots;
    }

    return Bots;
}

CMapList *CUNMProtocol::RECEIVE_GET_MAPLIST_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 4 bytes					-> Length
    // null terminated string	-> map list name
    // 4 bytes                  -> number of maps
    // for( i = 0; i < number of bots; i++ )
    //		null terminated string	-> map file name

    if( ValidateLength32Bit( data ) && data.size( ) >= 11 )
    {
        BYTEARRAY MapListName = UTIL_ExtractCString( data, 6 );
        uint32_t i = MapListName.size( ) + 7;
        vector<BYTEARRAY> Maps;
        uint32_t NumberMaps = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
        i += 4;

        while( NumberMaps > 0 )
        {
            if( data.size( ) < i + 1 )
                return new CMapList( MapListName, Maps );

            NumberMaps--;
            BYTEARRAY MapFileName = UTIL_ExtractCString( data, i );
            i += MapFileName.size( ) + 1;
            Maps.push_back( MapFileName );
        }

        return new CMapList( MapListName, Maps );
    }

    return nullptr;
}

unsigned char CUNMProtocol::RECEIVE_CHANGE_PASSWORD_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = успех, 1 = неверно указан текущий пароль, 2 = недопустимый пароль, 3 = ошибка базы, 4 = неизвестная ошибка

    if( ValidateLength( data ) && data.size( ) >= 5 )
        return data[4];

    return 4;
}

unsigned char CUNMProtocol::RECEIVE_SET_MAPLIST_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = успех, 1 = нет прав на изменение данного списка карт, 2 = ошибка базы, 3 = неизвестная ошибка

    if( ValidateLength( data ) && data.size( ) >= 5 )
        return data[4];

    return 3;
}

unsigned char CUNMProtocol::RECEIVE_ADD_MAPLIST_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = успех, 1 = нет прав на изменение данного списка карт, 2 = ошибка базы, 3 = неизвестная ошибка

    if( ValidateLength( data ) && data.size( ) >= 5 )
        return data[4];

    return 3;
}

unsigned char CUNMProtocol::RECEIVE_REMOVE_MAPLIST_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = успех, 1 = нет прав на изменение данного списка карт, 2 = ошибка базы, 3 = неизвестная ошибка

    if( ValidateLength( data ) && data.size( ) >= 5 )
        return data[4];

    return 3;
}

unsigned char CUNMProtocol::RECEIVE_CLEAR_MAPLIST_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = успех, 1 = нет прав на изменение данного списка карт, 2 = ошибка базы, 3 = неизвестная ошибка

    if( ValidateLength( data ) && data.size( ) >= 5 )
        return data[4];

    return 3;
}

uint32_t CUNMProtocol::RECEIVE_GET_MAPLIST_COUNT_RESPONSE( BYTEARRAY data, string &MapListName )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> map list count
    // null terminated string	-> map list name

    MapListName = string( );

    if( ValidateLength( data ) && data.size( ) >= 8 )
    {
        if( data.size( ) >= 9 )
        {
            BYTEARRAY UserName = UTIL_ExtractCString( data, 8 );
            MapListName = string( UserName.begin( ), UserName.end( ) );
        }

        return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );
    }

    return 0;
}

uint32_t CUNMProtocol::RECEIVE_PING_FROM_HOST_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Ping

    if( ValidateLength( data ) && data.size( ) >= 8 )
        return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );

    return 0;
}

bool CUNMProtocol::RECEIVE_RATE_MAP_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte                   -> Result

    if( ValidateLength( data ) && data.size( ) == 5 && data[4] == 1 )
        return true;

    return false;
}

uint32_t CUNMProtocol::RECEIVE_GAME_DELETE_RESPONSE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Game Counter

    if( ValidateLength( data ) && data.size( ) >= 8 )
        return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );

    return 0;
}

#endif
////////////////////////////////
// RECEIVE FUNCTIONS (SERVER) //
////////////////////////////////
#ifdef UNM_SERVER
unsigned char CUNMProtocol::RECEIVE_CLIENT_INFO( BYTEARRAY data, unsigned char &ClientType, uint32_t &ClientVersion )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> client type: 0 = user, 1 = bot 
	// 4 bytes					-> client version

	if( ValidateLength( data ) && data.size( ) >= 9 )
	{
		ClientType = data[4];
		ClientVersion = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 5, data.begin( ) + 9 ), false );
		
		if( ClientType > 1 )
			return 3;
		else if( ClientVersion == CURRENT_VERSION )
			return 0;
		else if( ( ClientType == 0 && ClientVersion >= MINIMUM_CLIENT_VERSION ) || ( ClientType == 1 && ClientVersion >= MINIMUM_BOT_VERSION ) )
			return 1;
		else
			return 2;
	}

	return 3;
}

CSmartLogonBot *CUNMProtocol::RECEIVE_SMART_LOGON_BOT( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Bot Type
    // 1 byte					-> Bot Can Create Game (supports special packets to create game from gui or not)
    // 2 bytes					-> Host Port
    // null terminated string	-> User Name
    // null terminated string	-> User Password
    // null terminated string	-> IP
    // null terminated string	-> Bot Name
    // null terminated string	-> Map List Name

	if( ValidateLength( data ) && data.size( ) >= 13 )
    {
        unsigned char BotType = data[4];
        unsigned char CanCreateGame = data[5];
        BYTEARRAY HostPort = BYTEARRAY( data.begin( ) + 6, data.begin( ) + 8 );
        uint32_t i = 8;
		BYTEARRAY UserName = UTIL_ExtractCString( data, i );
		i += UserName.size( ) + 1;
		BYTEARRAY UserPassword = UTIL_ExtractCString( data, i );
		i += UserPassword.size( ) + 1;
		BYTEARRAY IP = UTIL_ExtractCString( data, i );
		i += IP.size( ) + 1;
        BYTEARRAY BotName = UTIL_ExtractCString( data, i );
        i += BotName.size( ) + 1;
        BYTEARRAY MapListName = UTIL_ExtractCString( data, i );
        return new CSmartLogonBot( BotType, CanCreateGame, UTIL_ByteArrayToUInt16( HostPort, false ), string( UserName.begin( ), UserName.end( ) ), string( UserPassword.begin( ), UserPassword.end( ) ), string( IP.begin( ), IP.end( ) ), string( BotName.begin( ), BotName.end( ) ), string( MapListName.begin( ), MapListName.end( ) ) );
	}

	return nullptr;
}

uint32_t CUNMProtocol::RECEIVE_GAME_REFRESH( BYTEARRAY data )
{
    // 2 bytes						-> Header
    // 2 bytes						-> Length
    // 4 bytes						-> Game Number
    // other bytes                  -> (см. ниже)

    if( ValidateLength( data ) && data.size( ) >= 63 )
        return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );

    return 0;
}

CGameData *CUNMProtocol::RECEIVE_GAME_REFRESH( BYTEARRAY data, string BotName, string BotLogin, uint32_t &GameCounter )
{
	// 2 bytes						-> Header
	// 2 bytes						-> Length
    // 4 bytes						-> Game Number
	// 1 byte						-> TFT/RoC
	// 1 byte						-> War3 version
	// 1 byte						-> Game Status
	// 2 bytes						-> Host Port
	// 2 bytes						-> Map Width
	// 2 bytes						-> Map Height
	// 4 bytes						-> Map Game Type
	// 4 bytes						-> Map Game Flags
	// 4 bytes						-> Map CRC
	// 4 bytes						-> Host Counter
	// 4 bytes						-> Entry Key
    // 4 bytes						-> Up Time
	// null terminated string		-> Game Name
	// null terminated string		-> Game Owner
	// null terminated string		-> Map Path
	// null terminated string		-> Host Name
	// null terminated string		-> Host IP
	// null terminated string		-> TGA Image
	// null terminated string		-> Map Players
	// null terminated string		-> Map Description
	// null terminated string		-> Map Name
    // null terminated string		-> Stats
	// 1 byte						-> Slots Open
	// 1 byte						-> Slots Total
    // for( i = 0; i < Slots Total; i++ )
	// 		1 byte					-> Slot Download Status
	// 		1 byte					-> Slot Team
    // 		1 byte					-> Slot Colour
    // 		1 byte					-> Slot Race
    // 		1 byte					-> Slot Handicap
	// 		1 byte					-> Slot Status
    // 		null terminated string	-> Player Name
    // 		null terminated string	-> Player Server
    // 		4 bytes					-> Player Ping
	
    if( ValidateLength( data ) && data.size( ) >= 63 )
	{
		uint32_t GameNumber = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );
		unsigned char TFT = data[8];
		unsigned char War3Version = data[9];
		unsigned char GameStatus = data[10];
		BYTEARRAY HostPort = BYTEARRAY( data.begin( ) + 11, data.begin( ) + 13 );
		BYTEARRAY MapWidth = BYTEARRAY( data.begin( ) + 13, data.begin( ) + 15 );
		BYTEARRAY MapHeight = BYTEARRAY( data.begin( ) + 15, data.begin( ) + 17 );
		BYTEARRAY MapGameType = BYTEARRAY( data.begin( ) + 17, data.begin( ) + 21 );
		BYTEARRAY MapGameFlags = BYTEARRAY( data.begin( ) + 21, data.begin( ) + 25 );
		BYTEARRAY MapCRC = BYTEARRAY( data.begin( ) + 25, data.begin( ) + 29 );
		BYTEARRAY HostCounter = BYTEARRAY( data.begin( ) + 29, data.begin( ) + 33 );
		BYTEARRAY EntryKey = BYTEARRAY( data.begin( ) + 33, data.begin( ) + 37 );
		BYTEARRAY UpTime = BYTEARRAY( data.begin( ) + 37, data.begin( ) + 41 );
        uint32_t i = 41;
		BYTEARRAY GameName = UTIL_ExtractCString( data, i );
		i += GameName.size( ) + 1;
		BYTEARRAY GameOwner = UTIL_ExtractCString( data, i );
		i += GameOwner.size( ) + 1;
		BYTEARRAY MapPath = UTIL_ExtractCString( data, i );
		i += MapPath.size( ) + 1;
		BYTEARRAY HostName = UTIL_ExtractCString( data, i );
		i += HostName.size( ) + 1;
		BYTEARRAY HostIP = UTIL_ExtractCString( data, i );
		i += HostIP.size( ) + 1;
		BYTEARRAY TGAImage = UTIL_ExtractCString( data, i );
		i += TGAImage.size( ) + 1;
		BYTEARRAY MapPlayers = UTIL_ExtractCString( data, i );
		i += MapPlayers.size( ) + 1;
		BYTEARRAY MapDescription = UTIL_ExtractCString( data, i );
		i += MapDescription.size( ) + 1;
		BYTEARRAY MapName = UTIL_ExtractCString( data, i );
		i += MapName.size( ) + 1;
        BYTEARRAY Stats = UTIL_ExtractCString( data, i );
        i += Stats.size( ) + 1;
		unsigned char SlotsOpen = data[i];
		unsigned char SlotsTotal = data[i+1];
		i += 2;

		if( SlotsTotal == 0 || SlotsTotal > 24 )
			return nullptr;

        CGameData *GameData = new CGameData( GameCounter + 1, GameNumber, TFT, War3Version, GameStatus, HostPort, MapWidth, MapHeight, MapGameType, MapGameFlags, MapCRC, HostCounter, EntryKey, UpTime, GetTime( ), GameName, GameOwner, MapPath, HostName, HostIP, TGAImage, MapPlayers, MapDescription, MapName, Stats, { }, { }, { }, { }, SlotsOpen, SlotsTotal, { }, BotName, BotLogin, GetTicks( ) );

		while( SlotsTotal > 0 )
		{
			SlotsTotal--;

			if( data.size( ) < i + 12 )
			{
				delete GameData;
				return nullptr;
			}

			unsigned char SlotDownloadStatus = data[i];
			unsigned char SlotTeam = data[i+1];
			unsigned char SlotColour = data[i+2];
			unsigned char SlotRace = data[i+3];
			unsigned char SlotHandicap = data[i+4];
			unsigned char SlotStatus = data[i+5];
			i += 6;
			BYTEARRAY PlayerName = UTIL_ExtractCString( data, i );
			i += PlayerName.size( ) + 1;

			if( data.size( ) < i + 5 )
			{
				delete GameData;
				return nullptr;
			}

			BYTEARRAY PlayerServer = UTIL_ExtractCString( data, i );
			i += PlayerServer.size( ) + 1;
			
			if( data.size( ) < i + 4 )
			{
				delete GameData;
				return nullptr;
			}

			BYTEARRAY PlayerPing = BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 );
            i += 4;
			GameData->m_GameSlots.push_back( CGameSlotData( SlotDownloadStatus, SlotTeam, SlotColour, SlotRace, SlotHandicap, SlotStatus, PlayerName, PlayerServer, PlayerPing ) );
		}

        GameCounter = GameCounter + 1;
		return GameData;
	}

	return nullptr;
}

string CUNMProtocol::RECEIVE_CHAT_MESSAGE( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// null terminated string	-> Message
	
	if( ValidateLength( data ) && data.size( ) >= 5 )
	{
		BYTEARRAY Message = UTIL_ExtractCString( data, 4 );
		return string( Message.begin( ), Message.end( ) );
	}

	return string( );
}

CPlayerInfo *CUNMProtocol::RECEIVE_CHECK_JOINED_PLAYER( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Game Number
	// null terminated string	-> User Name
	// null terminated string	-> User IP
	
	if( ValidateLength( data ) && data.size( ) >= 10 )
	{
		uint32_t GameNumber = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );
		BYTEARRAY UserName = UTIL_ExtractCString( data, 8 );
		BYTEARRAY UserIP = UTIL_ExtractCString( data, 9 + UserName.size( ) );
		return new CPlayerInfo( GameNumber, string( UserName.begin( ), UserName.end( ) ), string( UserIP.begin( ), UserIP.end( ) ) );
	}
	
	return nullptr;
}

uint32_t CUNMProtocol::RECEIVE_GAME_UNHOST( BYTEARRAY data )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Game Number

	if( ValidateLength( data ) && data.size( ) >= 8 )
		return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );

	return 0;
}

CUserInfo *CUNMProtocol::RECEIVE_LOGON_CLIENT( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> User Name
    // null terminated string	-> User Password

    if( ValidateLength( data ) && data.size( ) >= 6 )
    {
        BYTEARRAY UserName = UTIL_ExtractCString( data, 4 );
        BYTEARRAY UserPassword = UTIL_ExtractCString( data, 5 + UserName.size( ) );
        return new CUserInfo( string( UserName.begin( ), UserName.end( ) ), string( UserPassword.begin( ), UserPassword.end( ) ) );
    }

    return nullptr;
}

CUserInfo *CUNMProtocol::RECEIVE_REGISTRY_CLIENT( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> User Name
    // null terminated string	-> User Password

    if( ValidateLength( data ) && data.size( ) >= 6 )
    {
        BYTEARRAY UserName = UTIL_ExtractCString( data, 4 );
        BYTEARRAY UserPassword = UTIL_ExtractCString( data, 5 + UserName.size( ) );
        return new CUserInfo( string( UserName.begin( ), UserName.end( ) ), string( UserPassword.begin( ), UserPassword.end( ) ) );
    }

    return nullptr;
}

bool CUNMProtocol::RECEIVE_GET_GAMELIST( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length

    if( ValidateLength( data ) && data.size( ) >= 4 )
        return true;

    return false;
}

uint32_t CUNMProtocol::RECEIVE_GET_GAME( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Game Counter

    if( ValidateLength( data ) && data.size( ) >= 8 )
        return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );

    return 0;
}

bool CUNMProtocol::RECEIVE_GET_BOTLIST( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length

    if( ValidateLength( data ) && data.size( ) >= 4 )
        return true;

    return false;
}

string CUNMProtocol::RECEIVE_GET_MAPLIST( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> map list name

    if( ValidateLength( data ) && data.size( ) >= 5 )
    {
        BYTEARRAY MapListName = UTIL_ExtractCString( data, 4 );
        return string( MapListName.begin( ), MapListName.end( ) );
    }

    return string( );
}

string CUNMProtocol::RECEIVE_CHANGE_PASSWORD( BYTEARRAY data, string &NewPassword )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> old password
    // null terminated string	-> new password

    NewPassword = string( );

    if( ValidateLength( data ) && data.size( ) >= 6 )
    {
        BYTEARRAY OldPasswordArray = UTIL_ExtractCString( data, 4 );

        if( data.size( ) < 6 + OldPasswordArray.size( ) )
            return string( );

        BYTEARRAY NewPasswordArray = UTIL_ExtractCString( data, 5 + OldPasswordArray.size( ) );
        NewPassword = string( NewPasswordArray.begin( ), NewPasswordArray.end( ) );
        return string( OldPasswordArray.begin( ), OldPasswordArray.end( ) );
    }

    return string( );
}

CMapList *CUNMProtocol::RECEIVE_SET_MAPLIST( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 4 bytes					-> Length
    // null terminated string	-> map list name
    // 4 bytes                  -> number of maps
    // for( i = 0; i < number of bots; i++ )
    //		null terminated string	-> map file name

    if( ValidateLength32Bit( data ) && data.size( ) >= 11 )
    {
        BYTEARRAY MapListName = UTIL_ExtractCString( data, 6 );
        uint32_t i = MapListName.size( ) + 7;
        vector<BYTEARRAY> Maps;
        uint32_t NumberMaps = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
        i += 4;

        while( NumberMaps > 0 )
        {
            if( data.size( ) < i + 1 )
                return new CMapList( MapListName, Maps );

            NumberMaps--;
            BYTEARRAY MapFileName = UTIL_ExtractCString( data, i );
            i += MapFileName.size( ) + 1;
            Maps.push_back( MapFileName );
        }

        return new CMapList( MapListName, Maps );
    }

    return nullptr;
}

CMapList *CUNMProtocol::RECEIVE_ADD_MAPLIST( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 4 bytes					-> Length
    // null terminated string	-> map list name
    // 4 bytes                  -> number of maps
    // for( i = 0; i < number of bots; i++ )
    //		null terminated string	-> map file name

    if( ValidateLength32Bit( data ) && data.size( ) >= 11 )
    {
        BYTEARRAY MapListName = UTIL_ExtractCString( data, 6 );
        uint32_t i = MapListName.size( ) + 7;
        vector<BYTEARRAY> Maps;
        uint32_t NumberMaps = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
        i += 4;

        while( NumberMaps > 0 )
        {
            if( data.size( ) < i + 1 )
                return new CMapList( MapListName, Maps );

            NumberMaps--;
            BYTEARRAY MapFileName = UTIL_ExtractCString( data, i );
            i += MapFileName.size( ) + 1;
            Maps.push_back( MapFileName );
        }

        return new CMapList( MapListName, Maps );
    }

    return nullptr;
}

CMapList *CUNMProtocol::RECEIVE_REMOVE_MAPLIST( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 4 bytes					-> Length
    // null terminated string	-> map list name
    // 4 bytes                  -> number of maps
    // for( i = 0; i < number of bots; i++ )
    //		null terminated string	-> map file name

    if( ValidateLength32Bit( data ) && data.size( ) >= 11 )
    {
        BYTEARRAY MapListName = UTIL_ExtractCString( data, 6 );
        uint32_t i = MapListName.size( ) + 7;
        vector<BYTEARRAY> Maps;
        uint32_t NumberMaps = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
        i += 4;

        while( NumberMaps > 0 )
        {
            if( data.size( ) < i + 1 )
                return new CMapList( MapListName, Maps );

            NumberMaps--;
            BYTEARRAY MapFileName = UTIL_ExtractCString( data, i );
            i += MapFileName.size( ) + 1;
            Maps.push_back( MapFileName );
        }

        return new CMapList( MapListName, Maps );
    }

    return nullptr;
}

string CUNMProtocol::RECEIVE_CLEAR_MAPLIST( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> map list name

    if( ValidateLength( data ) && data.size( ) >= 5 )
    {
        BYTEARRAY MapListName = UTIL_ExtractCString( data, 4 );
        return string( MapListName.begin( ), MapListName.end( ) );
    }

    return string( );
}

string CUNMProtocol::RECEIVE_GET_MAPLIST_COUNT( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> map list name

    if( ValidateLength( data ) && data.size( ) >= 5 )
    {
        BYTEARRAY MapListName = UTIL_ExtractCString( data, 4 );
        return string( MapListName.begin( ), MapListName.end( ) );
    }

    return string( );
}

uint32_t CUNMProtocol::RECEIVE_PONG_TO_HOST( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Pong

    if( ValidateLength( data ) && data.size( ) >= 8 )
        return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );

    return 0;
}

bool CUNMProtocol::RECEIVE_RATE_MAP( BYTEARRAY data, string &MapCode, uint32_t &PlayerVote )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes                  -> Player Vote
    // null terminated string	-> Map Code

    if( ValidateLength( data ) && data.size( ) >= 9 )
    {
        PlayerVote = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );
        BYTEARRAY MapCodeByteArray = UTIL_ExtractCString( data, 8 );
        MapCode = string( MapCodeByteArray.begin( ), MapCodeByteArray.end( ) );

        if( !MapCode.empty( ) )
            return true;
    }

    return false;
}

uint32_t CUNMProtocol::RECEIVE_GAME_DELETE( BYTEARRAY data )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Game Counter

    if( ValidateLength( data ) && data.size( ) >= 8 )
        return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );

    return 0;
}

#endif
/////////////////////////////
// SEND FUNCTIONS (CLIENT) //
/////////////////////////////
#ifdef UNM_CLIENT
BYTEARRAY CUNMProtocol::SEND_CLIENT_INFO( )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> client type: 0 = user, 1 = bot 
	// 4 bytes					-> client version

	BYTEARRAY packet;
	packet.push_back( UNM_HEADER_CONSTANT );										// UNM header constant
	packet.push_back( CLIENT_INFO );												// CLIENT_INFO
	packet.push_back( 0 );															// packet length will be assigned later
	packet.push_back( 0 );															// packet length will be assigned later
	packet.push_back( 1 );															// client type: 0 = user, 1 = bot 
	UTIL_AppendByteArray( packet, static_cast<uint32_t>(CURRENT_VERSION), false );	// client version
	AssignLength( packet );
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_SMART_LOGON_BOT( unsigned char nBotType, unsigned char nCanCreateGame, uint16_t nHostPort, string nUserName, string nUserPassword, string nIP, string nBotName, string nMapListName )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
    // 1 byte					-> Bot Type
    // 1 byte					-> Bot Can Create Game (supports special packets to create game from gui or not)
    // 2 bytes					-> Host Port
	// null terminated string	-> User Name
	// null terminated string	-> User Password
	// null terminated string	-> IP
	// null terminated string	-> Bot Name
    // null terminated string	-> Map List Name

	BYTEARRAY packet;
	packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
	packet.push_back( SMART_LOGON_BOT );					// SMART_LOGON_BOT
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( nBotType );							// Bot Type
    packet.push_back( nCanCreateGame );						// Bot Can Create Game (supports special packets to create game from gui or not)
    UTIL_AppendByteArray( packet, nHostPort, false );		// Host Port
	UTIL_AppendByteArrayFast( packet, nUserName );			// User Name
	UTIL_AppendByteArrayFast( packet, nUserPassword );		// User Password
    UTIL_AppendByteArrayFast( packet, nIP );				// IP
    UTIL_AppendByteArrayFast( packet, nBotName );			// Bot Name
    UTIL_AppendByteArrayFast( packet, nMapListName );		// Map List Name
	AssignLength( packet );
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_GAME_REFRESH( uint32_t GameNumber, bool LCPing, bool TFT, unsigned char War3Version, bool GameLoading, bool GameLoaded, uint16_t HostPort, BYTEARRAY MapWidth, BYTEARRAY MapHeight, BYTEARRAY MapGameType, BYTEARRAY MapGameFlags, BYTEARRAY MapCRC, uint32_t HostCounter, uint32_t EntryKey, uint32_t UpTime, string GameName, string GameOwner, string MapPath, string HostName, string HostIP, string TGAImage, string MapPlayers, string MapDescription, string MapName, string Stats, unsigned char SlotsOpen, vector<CGameSlot> &GameSlots, vector<CGamePlayer *> players )
{
	// 2 bytes						-> Header
	// 2 bytes						-> Length
    // 4 bytes						-> Game Number
	// 1 byte						-> TFT/RoC
	// 1 byte						-> War3 version
	// 1 byte						-> Game Status
	// 2 bytes						-> Host Port
	// 2 bytes						-> Map Width
	// 2 bytes						-> Map Height
	// 4 bytes						-> Map Game Type
	// 4 bytes						-> Map Game Flags
	// 4 bytes						-> Map CRC
	// 4 bytes						-> Host Counter
	// 4 bytes						-> Entry Key
    // 4 bytes						-> Up Time
	// null terminated string		-> Game Name
	// null terminated string		-> Game Owner
	// null terminated string		-> Map Path
	// null terminated string		-> Host Name
	// null terminated string		-> Host IP
	// null terminated string		-> TGA Image
	// null terminated string		-> Map Players
	// null terminated string		-> Map Description
	// null terminated string		-> Map Name
    // null terminated string		-> Stats
	// 1 byte						-> Slots Open
	// 1 byte						-> Slots Total
    // for( i = 0; i < Slots Total; i++ )
	// 		1 byte					-> Slot Download Status
	// 		1 byte					-> Slot Team
    // 		1 byte					-> Slot Colour
    // 		1 byte					-> Slot Race
    // 		1 byte					-> Slot Handicap
	// 		1 byte					-> Slot Status
    // 		null terminated string	-> Player Name
    // 		null terminated string	-> Player Server
    // 		4 bytes					-> Player Ping

	BYTEARRAY packet;
	packet.push_back( UNM_HEADER_CONSTANT );										// UNM header constant
	packet.push_back( GAME_REFRESH );												// GAME_REFRESH
	packet.push_back( 0 );															// packet length will be assigned later
	packet.push_back( 0 );															// packet length will be assigned later
    UTIL_AppendByteArray( packet, GameNumber, false );								// Game Number

	if( TFT )
		packet.push_back( 1 );														// TFT
	else
		packet.push_back( 0 );														// RoC

	packet.push_back( War3Version );												// War3 Version

	if( GameLoading )
		packet.push_back( 1 );														// Game Status = Loading
	else if( GameLoaded )
		packet.push_back( 2 );														// Game Status = Started
	else
        packet.push_back( 0 );														// Game Status = Lobby

    UTIL_AppendByteArray( packet, HostPort, false );								// Host Port
	UTIL_AppendByteArrayFast( packet, MapWidth );									// Map Width
	UTIL_AppendByteArrayFast( packet, MapHeight );									// Map Height
	UTIL_AppendByteArrayFast( packet, MapGameType );								// Map Game Type
	UTIL_AppendByteArrayFast( packet, MapGameFlags );								// Map Game Flags
	UTIL_AppendByteArrayFast( packet, MapCRC );										// Map CRC
	UTIL_AppendByteArray( packet, HostCounter, false );								// Host Counter
	UTIL_AppendByteArray( packet, EntryKey, false );								// Entry Key
	UTIL_AppendByteArray( packet, UpTime, false );									// Up Time
	UTIL_AppendByteArrayFast( packet, GameName );									// Game Name
	UTIL_AppendByteArrayFast( packet, GameOwner );									// Game Owner
	UTIL_AppendByteArrayFast( packet, MapPath );									// Map Path
	UTIL_AppendByteArrayFast( packet, HostName );									// Host Name
	UTIL_AppendByteArrayFast( packet, HostIP );										// Host IP
	UTIL_AppendByteArrayFast( packet, TGAImage );									// TGA Image
	UTIL_AppendByteArrayFast( packet, MapPlayers );									// Map Players
	UTIL_AppendByteArrayFast( packet, MapDescription );								// Map Description
	UTIL_AppendByteArrayFast( packet, MapName );									// Map Name
    UTIL_AppendByteArrayFast( packet, Stats );                                      // Stats
	packet.push_back( SlotsOpen );													// Slots Open
	packet.push_back( static_cast<unsigned char>( GameSlots.size( ) ) );			// Slots Total

    for( uint32_t i = 0; i < GameSlots.size( ); i++ )
	{
		packet.push_back( GameSlots[i].GetDownloadStatus( ) );						// Slot Download Status
		packet.push_back( GameSlots[i].GetTeam( ) );								// Slot Team
		packet.push_back( GameSlots[i].GetColour( ) );								// Slot Colour
		packet.push_back( GameSlots[i].GetRace( ) );								// Slot Race
        packet.push_back( GameSlots[i].GetHandicap( ) );							// Slot Handicap

        if( GameSlots[i].GetSlotStatus( ) != SLOTSTATUS_OCCUPIED || players.empty( ) )
        {
            string SlotStatus;

            if( GameSlots[i].GetComputer( ) == 1 )
            {
                if( GameSlots[i].GetComputerType( ) == 0 )
                {
                    packet.push_back( 4 );										// Slot Status = Copmuter Easy
                    SlotStatus = "Компьютер (слабый)";
                }
                else if( GameSlots[i].GetComputerType( ) == 2 )
                {
                    packet.push_back( 5 );										// Slot Status = Copmuter Normal
                    SlotStatus = "Компьютер (средний)";
                }
                else
                {
                    packet.push_back( 6 );										// Slot Status = Copmuter Hard
                    SlotStatus = "Компьютер (сильный)";
                }
            }
            else if( GameSlots[i].GetSlotStatus( ) == 2 )
            {
                packet.push_back( 3 );											// Slot Status = FakePlayer
                SlotStatus = "FakePlayer";
            }
            else if( GameSlots[i].GetSlotStatus( ) == 1 )
            {
                packet.push_back( 2 );											// Slot Status = Close
                SlotStatus = "Закрыто";
            }
            else
            {
                packet.push_back( 0 );											// Slot Status = Open
                SlotStatus = "Открыто";
            }

            UTIL_AppendByteArrayFast( packet, SlotStatus );						// Slot Name
            packet.push_back( 0 );												// Server is NULL
            packet.push_back( 0 );												// Ping is NULL (4 bytes)
            packet.push_back( 0 );
            packet.push_back( 0 );
            packet.push_back( 0 );
        }
        else
        {
            for( vector<CGamePlayer *> ::iterator n = players.begin( ); n != players.end( ); n++ )
            {
                if( !( *n )->GetLeftMessageSent( ) && ( *n )->GetSID( ) == i )
                {
                    packet.push_back( 1 );												// Slot Status = Player
                    UTIL_AppendByteArray( packet, ( *n )->GetName( ) );                 // Player Name
                    UTIL_AppendByteArray( packet, ( *n )->GetJoinedRealm( ) );          // Player Server
                    UTIL_AppendByteArray( packet, ( *n )->GetPing( LCPing ), false );	// Player Ping
                    break;
                }
                else if( n + 1 == players.end( ) )
                {
                    string SlotStatus;

                    if( GameSlots[i].GetComputer( ) == 1 )
                    {
                        if( GameSlots[i].GetComputerType( ) == 0 )
                        {
                            packet.push_back( 4 );										// Slot Status = Copmuter Easy
                            SlotStatus = "Компьютер (слабый)";
                        }
                        else if( GameSlots[i].GetComputerType( ) == 2 )
                        {
                            packet.push_back( 5 );										// Slot Status = Copmuter Normal
                            SlotStatus = "Компьютер (средний)";
                        }
                        else
                        {
                            packet.push_back( 6 );										// Slot Status = Copmuter Hard
                            SlotStatus = "Компьютер (сильный)";
                        }
                    }
                    else if( GameSlots[i].GetSlotStatus( ) == 2 )
                    {
                        packet.push_back( 3 );											// Slot Status = FakePlayer
                        SlotStatus = "FakePlayer";
                    }
                    else if( GameSlots[i].GetSlotStatus( ) == 1 )
                    {
                        packet.push_back( 2 );											// Slot Status = Close
                        SlotStatus = "Закрыто";
                    }
                    else
                    {
                        packet.push_back( 0 );											// Slot Status = Open
                        SlotStatus = "Открыто";
                    }

                    UTIL_AppendByteArrayFast( packet, SlotStatus );						// Slot Name
                    packet.push_back( 0 );												// Server is NULL
                    packet.push_back( 0 );												// Ping is NULL (4 bytes)
                    packet.push_back( 0 );
                    packet.push_back( 0 );
                    packet.push_back( 0 );
                    break;
                }
            }
        }
	}

    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_CHAT_MESSAGE( string command, unsigned char manual, unsigned char botid )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// null terminated string	-> Message

	BYTEARRAY packet;
    packet.push_back( manual );						// костыльный способ как-то обнозначить данный пакет, перед отправкой это значение будет измененно на BNET_HEADER_CONSTANT
    packet.push_back( CHAT_MESSAGE );				// CHAT_MESSAGE
    packet.push_back( 0 );                          // packet length will be assigned later
	packet.push_back( 0 );							// packet length will be assigned later
	UTIL_AppendByteArrayFast( packet, command );	// Message
	AssignLength( packet );
    packet.back( ) = botid;				// костыльный способ как-то обнозначить данный пакет, перед отправкой это значение будет измененно на null
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_CHAT_MESSAGE( string command )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// null terminated string	-> Message

	BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );		// UNM header constant
    packet.push_back( CHAT_MESSAGE );				// CHAT_MESSAGE
    packet.push_back( 0 );                          // packet length will be assigned later
	packet.push_back( 0 );							// packet length will be assigned later
	UTIL_AppendByteArrayFast( packet, command );	// Message
	AssignLength( packet );
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_CHECK_JOINED_PLAYER( uint32_t GameNumber, string UserName, string UserIP )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Game Number
	// null terminated string	-> User Name
	// null terminated string	-> User IP
	
	BYTEARRAY packet;
	packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
	packet.push_back( CHECK_JOINED_PLAYER );				// CHECK_JOINED_PLAYER
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	UTIL_AppendByteArray( packet, GameNumber, false );		// Game Number
	UTIL_AppendByteArrayFast( packet, UserName );			// User Name
	UTIL_AppendByteArrayFast( packet, UserIP );				// User IP
	AssignLength( packet );
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_GAME_UNHOST( uint32_t GameNumber )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Game Number
	
	BYTEARRAY packet;
	packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
	packet.push_back( GAME_UNHOST );						// GAME_UNHOST
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	UTIL_AppendByteArray( packet, GameNumber, false );		// Game Number
	AssignLength( packet );
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_LOGON_CLIENT( string nUserName, string nUserPassword )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> User Name
    // null terminated string	-> User Password

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( LOGON_CLIENT );                       // LOGON_CLIENT
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, nUserName );			// User Name
    UTIL_AppendByteArrayFast( packet, nUserPassword );		// User Password
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_REGISTRY_CLIENT( string nUserName, string nUserPassword )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> User Name
    // null terminated string	-> User Password

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( REGISTRY_CLIENT );                    // REGISTRY_CLIENT
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, nUserName );			// User Name
    UTIL_AppendByteArrayFast( packet, nUserPassword );		// User Password
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_GET_GAMELIST( )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( GET_GAMELIST );                       // GET_GAMELIST
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_GET_GAME( uint32_t GameCounter )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Game Counter

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( GET_GAME );                           // GET_GAME
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    UTIL_AppendByteArray( packet, GameCounter, false );		// Game Counter
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_GET_BOTLIST( )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( GET_BOTLIST );                        // GET_BOTLIST
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_GET_MAPLIST( string MapListName )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> map list name

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );                // UNM header constant
    packet.push_back( GET_MAPLIST );                        // GET_MAPLIST
    packet.push_back( 0 );                                  // packet length will be assigned later
    packet.push_back( 0 );                                  // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, MapListName );        // map list name
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_CHANGE_PASSWORD( string OldPassword, string NewPassword )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> old password
    // null terminated string	-> new password

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );                // UNM header constant
    packet.push_back( CHANGE_PASSWORD );                    // CHANGE_PASSWORD
    packet.push_back( 0 );                                  // packet length will be assigned later
    packet.push_back( 0 );                                  // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, OldPassword );        // old password
    UTIL_AppendByteArrayFast( packet, NewPassword );        // new password
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_SET_MAPLIST( CMapList *MapList )
{
    // 2 bytes					-> Header
    // 4 bytes					-> Length
    // null terminated string	-> map list name
    // 4 bytes                  -> number of maps
    // for( i = 0; i < number of bots; i++ )
    //		null terminated string	-> map file name

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );                                                                // UNM header constant
    packet.push_back( SET_MAPLIST );                                                                        // SET_MAPLIST
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, MapList->m_MapListName );                                             // map list name
    UTIL_AppendByteArray( packet, static_cast<uint32_t>( MapList->m_MapFileNamesArray.size( ) ), false );   // number of maps

    for( uint32_t i = 0; i < MapList->m_MapFileNamesArray.size( ); i++ )
        UTIL_AppendByteArrayFast( packet, MapList->m_MapFileNamesArray[i] );                                // map file name

    AssignLength32Bit( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_ADD_MAPLIST( CMapList *MapList )
{
    // 2 bytes					-> Header
    // 4 bytes					-> Length
    // null terminated string	-> map list name
    // 4 bytes                  -> number of maps
    // for( i = 0; i < number of bots; i++ )
    //		null terminated string	-> map file name

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );                                                                // UNM header constant
    packet.push_back( ADD_MAPLIST );                                                                        // ADD_MAPLIST
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, MapList->m_MapListName );                                             // map list name
    UTIL_AppendByteArray( packet, static_cast<uint32_t>( MapList->m_MapFileNamesArray.size( ) ), false );   // number of maps

    for( uint32_t i = 0; i < MapList->m_MapFileNamesArray.size( ); i++ )
        UTIL_AppendByteArrayFast( packet, MapList->m_MapFileNamesArray[i] );                                // map file name

    AssignLength32Bit( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_REMOVE_MAPLIST( CMapList *MapList )
{
    // 2 bytes					-> Header
    // 4 bytes					-> Length
    // null terminated string	-> map list name
    // 4 bytes                  -> number of maps
    // for( i = 0; i < number of bots; i++ )
    //		null terminated string	-> map file name

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );                                                                // UNM header constant
    packet.push_back( REMOVE_MAPLIST );                                                                     // REMOVE_MAPLIST
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, MapList->m_MapListName );                                             // map list name
    UTIL_AppendByteArray( packet, static_cast<uint32_t>( MapList->m_MapFileNamesArray.size( ) ), false );   // number of maps

    for( uint32_t i = 0; i < MapList->m_MapFileNamesArray.size( ); i++ )
        UTIL_AppendByteArrayFast( packet, MapList->m_MapFileNamesArray[i] );                                // map file name

    AssignLength32Bit( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_CLEAR_MAPLIST( string MapListName )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> map list name

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );                // UNM header constant
    packet.push_back( CLEAR_MAPLIST );                      // CLEAR_MAPLIST
    packet.push_back( 0 );                                  // packet length will be assigned later
    packet.push_back( 0 );                                  // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, MapListName );        // map list name
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_GET_MAPLIST_COUNT( string MapListName )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // null terminated string	-> map list name

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );                // UNM header constant
    packet.push_back( GET_MAPLIST_COUNT );                  // GET_MAPLIST_COUNT
    packet.push_back( 0 );                                  // packet length will be assigned later
    packet.push_back( 0 );                                  // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, MapListName );        // map list name
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_PONG_TO_HOST( uint32_t pong )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Pong

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( PONG_TO_HOST );                       // PONG_TO_HOST
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    UTIL_AppendByteArray( packet, pong, false );            // Pong
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_RATE_MAP( string MapCode, uint32_t PlayerVote )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes                  -> Player Vote
    // null terminated string	-> Map Code

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( RATE_MAP );                           // RATE_MAP
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    UTIL_AppendByteArray( packet, PlayerVote, false );      // Player Vote
    UTIL_AppendByteArrayFast( packet, MapCode );            // Map Code
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_GAME_DELETE( uint32_t GameCounter )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Game Counter

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( GAME_DELETE );                        // GAME_DELETE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    UTIL_AppendByteArray( packet, GameCounter, false );     // Game Counter
    AssignLength( packet );
    return packet;
}

#endif
/////////////////////////////
// SEND FUNCTIONS (SERVER) //
/////////////////////////////
#ifdef UNM_SERVER
BYTEARRAY CUNMProtocol::SEND_CLIENT_INFO_RESPONSE( unsigned char result )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> result

	BYTEARRAY packet;
	packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
	packet.push_back( CLIENT_INFO_RESPONSE );				// CLIENT_INFO_RESPONSE
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( result );								// result
	AssignLength( packet );
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_SMART_LOGON_BOT_RESPONSE( unsigned char result )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> result
    // 1 byte					-> Result 0 = ошибка базы, 1 = неверный пароль, 2 = недопустимый логин, 3 = недопустимый пароль, 4 = успех, 5 = неизвестная ошибка

	BYTEARRAY packet;
	packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
	packet.push_back( SMART_LOGON_BOT_RESPONSE );			// SMART_LOGON_BOT_RESPONSE
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( result );								// result
	AssignLength( packet );
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_GAME_REFRESH_RESPONSE( unsigned char result, uint32_t GameNumber, uint32_t GameCounter )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 bytes                  -> Result: 0 = успех (игра создана); 1 = успех (игра обновлена); 2 = успех, но предупреждение (частый рефреш); 3 = успех - игра обновлена, но предупреждение (bad packet); 4 = 1 & 2; 5 = ошибка - игра не создана (bad packet); 6 = ошибка, некорректно указан номер игры; 7 = неизвестная ошибка
    // 4 bytes					-> Game Number
    // 4 bytes					-> Game Counter

	BYTEARRAY packet;
	packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
	packet.push_back( GAME_REFRESH_RESPONSE );				// GAME_REFRESH_RESPONSE
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later

    if( result > 7 )
        result = 7;

    packet.push_back( result );								// result
    UTIL_AppendByteArray( packet, GameNumber, false );		// Game Number
    UTIL_AppendByteArray( packet, GameCounter, false );		// Game Counter
	AssignLength( packet );
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_CHAT_EVENT_RESPONSE( CBNETProtocol::IncomingChatEvent nChatEvent, uint32_t nUserFlags, uint32_t nPing, string nUser, string nMessage )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> EventID
    // 4 bytes					-> UserFlags
    // 4 bytes					-> Ping
    // null terminated string	-> User
    // null terminated string	-> Message

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );										// UNM header constant
    packet.push_back( CHAT_EVENT_RESPONSE );                                        // CHAT_EVENT_RESPONSE
    packet.push_back( 0 );															// packet length will be assigned later
    packet.push_back( 0 );															// packet length will be assigned later
    UTIL_AppendByteArray( packet, static_cast<uint32_t>(nChatEvent), false );		// Event ID
    UTIL_AppendByteArray( packet, nUserFlags, false );								// User Flags
    UTIL_AppendByteArray( packet, nPing, false );									// Ping
    UTIL_AppendByteArrayFast( packet, nUser );										// User
    UTIL_AppendByteArrayFast( packet, nMessage );									// Message
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_CHECK_JOINED_PLAYER_RESPONSE( unsigned char result )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> result

	BYTEARRAY packet;
	packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
	packet.push_back( CHECK_JOINED_PLAYER_RESPONSE );		// CHECK_JOINED_PLAYER_RESPONSE
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( result );								// result
	AssignLength( packet );
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_REQUEST_JOIN_PLAYER_RESPONSE( uint32_t GameNumber, string UserName, string UserIP )
{
	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Game Number
	// null terminated string	-> User Name
	// null terminated string	-> User IP
	
	BYTEARRAY packet;
	packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
	packet.push_back( REQUEST_JOIN_PLAYER_RESPONSE );		// REQUEST_JOIN_PLAYER_RESPONSE
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	UTIL_AppendByteArray( packet, GameNumber, false );		// Game Number
	UTIL_AppendByteArrayFast( packet, UserName );			// User Name
	UTIL_AppendByteArrayFast( packet, UserIP );				// User IP
	AssignLength( packet );
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_GAME_UNHOST_RESPONSE( uint32_t GameNumber, uint32_t GameCounter )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Game Number
    // 4 bytes					-> Game Counter

	BYTEARRAY packet;
	packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
	packet.push_back( GAME_UNHOST_RESPONSE );				// GAME_UNHOST_RESPONSE
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
    UTIL_AppendByteArray( packet, GameNumber, false );		// Game Number
    UTIL_AppendByteArray( packet, GameCounter, false );		// Game Counter
	AssignLength( packet );
	return packet;
}

BYTEARRAY CUNMProtocol::SEND_LOGON_CLIENT_RESPONSE( unsigned char result )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = ошибка базы, 1 = логин не существует, 2 = неверный пароль, 3 = авторизация под данной версией клиента разрешена только бета-тестерам, 4 = успех, 5 = неизвестная ошибка

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( LOGON_CLIENT_RESPONSE );				// LOGON_CLIENT_RESPONSE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( result );								// result
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_REGISTRY_CLIENT_RESPONSE( unsigned char result )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = ошибка базы, 1 = логин занят, 2 = недопустимый логин, 3 = недопустимый пароль, 4 = успех, 5 = неизвестная ошибка

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( REGISTRY_CLIENT_RESPONSE );			// REGISTRY_CLIENT_RESPONSE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( result );								// result
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_GET_GAMELIST_RESPONSE( BYTEARRAY games )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes                  -> number of games
    // for( i = 0; i < number of games; i++ )
    // 		4 byets					-> Game Counter
    //      1 byte					-> Slots Open
    //      1 byte					-> Slots Total
    //		null terminated string	-> Game Name
    //		null terminated string	-> Map Name
    //		null terminated string	-> Map FileName
    //		null terminated string	-> Map Code
    //		null terminated string	-> Map Genre
    //      4 bytes					-> Map Ratings
    //      4 bytes					-> Map Ratings Count
    //		null terminated string	-> Bot Name
    //		null terminated string	-> Bot Login
    //		null terminated string	-> Game Owner
    //		null terminated string	-> Stats

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( GET_GAMELIST_RESPONSE );              // GET_GAMELIST_RESPONSE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, games );				// games
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_GET_GAME_RESPONSE( CGameData *game )
{
    // описание и построение пакета в CGameData::GetPacket( )

    if( game != nullptr )
        return { };

    return game->GetPacket( );
}

BYTEARRAY CUNMProtocol::SEND_GET_BOTLIST_RESPONSE( vector<CBotInfo *> BotList )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes                  -> number of bots
    // for( i = 0; i < number of bots; i++ )
    //		null terminated string	-> bot name
    //		null terminated string	-> map list

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );                                        // UNM header constant
    packet.push_back( GET_BOTLIST_RESPONSE );                                       // GET_BOTLIST_RESPONSE
    packet.push_back( 0 );                                                          // packet length will be assigned later
    packet.push_back( 0 );                                                          // packet length will be assigned later
    UTIL_AppendByteArray( packet, static_cast<uint32_t>( BotList.size( ) ), false );// number of bots

    for( uint32_t i = 0; i < BotList.size( ); i++ )
    {
        UTIL_AppendByteArrayFast( packet, BotList[i]->m_BotName );                  // bot name
        UTIL_AppendByteArrayFast( packet, BotList[i]->m_MapListName );              // map list
    }

    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_GET_MAPLIST_RESPONSE( CMapList *MapList )
{
    // 2 bytes					-> Header
    // 4 bytes					-> Length
    // null terminated string	-> map list name
    // 4 bytes                  -> number of maps
    // for( i = 0; i < number of bots; i++ )
    //		null terminated string	-> map file name

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );                                                                // UNM header constant
    packet.push_back( GET_MAPLIST_RESPONSE );                                                               // GET_MAPLIST_RESPONSE
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    packet.push_back( 0 );                                                                                  // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, MapList->m_MapListName );                                             // map list name
    UTIL_AppendByteArray( packet, static_cast<uint32_t>( MapList->m_MapFileNamesArray.size( ) ), false );   // number of maps

    for( uint32_t i = 0; i < MapList->m_MapFileNamesArray.size( ); i++ )
        UTIL_AppendByteArrayFast( packet, MapList->m_MapFileNamesArray[i] );                                // map file name

    AssignLength32Bit( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_CHANGE_PASSWORD_RESPONSE( unsigned char result )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = успех, 1 = неверно указан текущий пароль, 2 = недопустимый пароль, 3 = ошибка базы, 4 = неизвестная ошибка

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( CHANGE_PASSWORD_RESPONSE );			// CHANGE_PASSWORD_RESPONSE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( result );								// result
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_SET_MAPLIST_RESPONSE( unsigned char result )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = успех, 1 = нет прав на изменение данного списка карт, 2 = ошибка базы, 3 = неизвестная ошибка

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( SET_MAPLIST_RESPONSE );               // SET_MAPLIST_RESPONSE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( result );								// result
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_ADD_MAPLIST_RESPONSE( unsigned char result )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = успех, 1 = нет прав на изменение данного списка карт, 2 = ошибка базы, 3 = неизвестная ошибка

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( ADD_MAPLIST_RESPONSE );               // ADD_MAPLIST_RESPONSE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( result );								// result
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_REMOVE_MAPLIST_RESPONSE( unsigned char result )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = успех, 1 = нет прав на изменение данного списка карт, 2 = ошибка базы, 3 = неизвестная ошибка

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( REMOVE_MAPLIST_RESPONSE );			// REMOVE_MAPLIST_RESPONSE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( result );								// result
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_CLEAR_MAPLIST_RESPONSE( unsigned char result )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte					-> Result 0 = успех, 1 = нет прав на изменение данного списка карт, 2 = ошибка базы, 3 = неизвестная ошибка

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( CLEAR_MAPLIST_RESPONSE );             // CLEAR_MAPLIST_RESPONSE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( result );								// result
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_GET_MAPLIST_COUNT_RESPONSE( uint32_t MapListCount, string MapListName )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> map list count
    // null terminated string	-> map list name

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( GET_MAPLIST_COUNT_RESPONSE );         // GET_MAPLIST_COUNT_RESPONSE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    UTIL_AppendByteArray( packet, MapListCount, false );	// map list count
    UTIL_AppendByteArrayFast( packet, MapListName );		// map list name
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_PING_FROM_HOST_RESPONSE( )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Ping

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( PING_FROM_HOST );                     // PING_FROM_HOST
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    UTIL_AppendByteArray( packet, GetTicks( ), false );     // Ping
    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_RATE_MAP_RESPONSE( bool reuslt )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 1 byte                   -> Result

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( RATE_MAP_RESPONSE );                  // RATE_MAP_RESPONSE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later

    if( reuslt )
        packet.push_back( 1 );								// result = success
    else
        packet.push_back( 0 );								// result = error

    AssignLength( packet );
    return packet;
}

BYTEARRAY CUNMProtocol::SEND_GAME_DELETE_RESPONSE( uint32_t GameCounter )
{
    // 2 bytes					-> Header
    // 2 bytes					-> Length
    // 4 bytes					-> Game Counter

    BYTEARRAY packet;
    packet.push_back( UNM_HEADER_CONSTANT );				// UNM header constant
    packet.push_back( GAME_DELETE_RESPONSE );               // GAME_DELETE_RESPONSE
    packet.push_back( 0 );									// packet length will be assigned later
    packet.push_back( 0 );									// packet length will be assigned later
    UTIL_AppendByteArray( packet, GameCounter, false );     // Game Counter
    AssignLength( packet );
    return packet;
}

#endif

//
// CUserInfo
//

CUserInfo::CUserInfo( string nUserName, string nUserPassword ) :
    m_UserName( nUserName ),
    m_UserPassword( nUserPassword ),
    m_IP( string( ) ),
    m_Access( 0 ),
    m_BetaTester( false )
{

}

CUserInfo::~CUserInfo( )
{

}

bool CUserInfo::UserNameIsValid( )
{
    if( m_UserName.size( ) > 3 && m_UserName.size( ) <= 15 && m_UserName.find_first_not_of( "qwertyuiopasdfghjklzxcvbnm1234567890<>[]()-_.,!?&" ) == string::npos )
        return true;

    return false;
}

bool CUserInfo::UserPasswordIsValid( )
{
    if( m_UserPassword.size( ) > 5 && m_UserPassword.find_first_of( " ●•" ) == string::npos )
        return true;

    return false;
}

//
// CSmartLogonBot
//

CSmartLogonBot::CSmartLogonBot( unsigned char nBotType, unsigned char nCanCreateGame, uint16_t nHostPort, string nUserName, string nUserPassword, string nIP, string nBotName, string nMapListName ) :
    m_BotType( nBotType ),
    m_CanCreateGame( nCanCreateGame ),
    m_HostPort( nHostPort ),
    m_UserName( nUserName ),
    m_UserPassword( nUserPassword ),
    m_IP( nIP ),
    m_BotName( nBotName ),
    m_MapListName( nMapListName )
{

}

CSmartLogonBot::~CSmartLogonBot( )
{

}

//
// CGameSlotData
//

CGameSlotData::CGameSlotData( unsigned char nSlotDownloadStatus, unsigned char nSlotTeam, unsigned char nSlotColour, unsigned char nSlotRace, unsigned char nSlotHandicap, unsigned char nSlotStatus, BYTEARRAY nPlayerName, BYTEARRAY nPlayerServer, BYTEARRAY nPlayerPing ) :
    m_SlotDownloadStatus( nSlotDownloadStatus ),
    m_SlotTeam( nSlotTeam ),
    m_SlotColour( nSlotColour ),
    m_SlotRace( nSlotRace ),
    m_SlotHandicap( nSlotHandicap ),
    m_SlotStatus( nSlotStatus ),
    m_PlayerName( nPlayerName ),
    m_PlayerServer( nPlayerServer ),
    m_PlayerPing( nPlayerPing )
{

}

CGameSlotData::~CGameSlotData( )
{

}

void CGameSlotData::UpdateData( unsigned char nSlotDownloadStatus, unsigned char nSlotTeam, unsigned char nSlotColour, unsigned char nSlotRace, unsigned char nSlotHandicap, unsigned char nSlotStatus, BYTEARRAY nPlayerName, BYTEARRAY nPlayerServer, BYTEARRAY nPlayerPing )
{
	m_SlotDownloadStatus = nSlotDownloadStatus;
	m_SlotTeam = nSlotTeam;
	m_SlotColour = nSlotColour;
	m_SlotRace = nSlotRace;
	m_SlotHandicap = nSlotHandicap;
	m_SlotStatus = nSlotStatus;
	m_PlayerName = nPlayerName;
	m_PlayerServer = nPlayerServer;
	m_PlayerPing = nPlayerPing;
}

//
// CGameHeader
//

CGameHeader::CGameHeader( uint32_t nGameCounter, uint32_t nSlotsOpen, uint32_t nSlotsTotal, string nGameName, string nMapName, string nMapFileName, string nMapCode, string nMapGenre, uint32_t nMapRatings, uint32_t nMapRatingsCount, string nBotName, string nBotLogin, string nGameOwner, string nStats ) :
    m_GameCounter( nGameCounter ),
    m_SlotsOpen( nSlotsOpen ),
    m_SlotsTotal( nSlotsTotal ),
    m_GameName( nGameName ),
    m_MapName( nMapName ),
    m_MapFileName( nMapFileName ),
    m_MapCode( nMapCode ),
    m_MapGenre( nMapGenre ),
    m_MapRatings( nMapRatings ),
    m_MapRatingsCount( nMapRatingsCount ),
    m_BotName( nBotName ),
    m_BotLogin( nBotLogin ),
    m_GameOwner( nGameOwner ),
    m_Stats( nStats )
{

}

CGameHeader::~CGameHeader( )
{

}

//
// CGameData
//

CGameData::CGameData( uint32_t nGameCounter, uint32_t nGameNumber, unsigned char nTFT, unsigned char nWar3Version, unsigned char nGameStatus, BYTEARRAY nHostPort, BYTEARRAY nMapWidth, BYTEARRAY nMapHeight, BYTEARRAY nMapGameType, BYTEARRAY nMapGameFlags, BYTEARRAY nMapCRC, BYTEARRAY nHostCounter, BYTEARRAY nEntryKey, BYTEARRAY nUpTime, uint32_t nCreationTime, BYTEARRAY nGameName, BYTEARRAY nGameOwner, BYTEARRAY nMapPath, BYTEARRAY nHostName, BYTEARRAY nHostIP, BYTEARRAY nTGAImage, BYTEARRAY nMapPlayers, BYTEARRAY nMapDescription, BYTEARRAY nMapName, BYTEARRAY nStats, BYTEARRAY nMapCode, BYTEARRAY nMapGenre, BYTEARRAY nMapRatings, BYTEARRAY nMapRatingsCount, unsigned char nSlotsOpen, unsigned char nSlotsTotal, vector<CGameSlotData> nGameSlots, string nBotName, string nBotLogin, uint32_t nRefreshTicks ) :
    m_GameCounter( nGameCounter ),
    m_GameNumber( nGameNumber ),
    m_TFT( nTFT ),
    m_War3Version( nWar3Version ),
    m_GameStatus( nGameStatus ),
    m_HostPort( nHostPort ),
    m_MapWidth( nMapWidth ),
    m_MapHeight( nMapHeight ),
    m_MapGameType( nMapGameType ),
    m_MapGameFlags( nMapGameFlags ),
    m_MapCRC( nMapCRC ),
    m_HostCounter( nHostCounter ),
    m_EntryKey( nEntryKey ),
    m_UpTime( nUpTime ),
    m_CreationTime( nCreationTime ),
    m_GameName( nGameName ),
    m_GameOwner( nGameOwner ),
    m_MapPath( nMapPath ),
    m_HostName( nHostName ),
    m_HostIP( nHostIP ),
    m_TGAImage( nTGAImage ),
    m_MapPlayers( nMapPlayers ),
    m_MapDescription( nMapDescription ),
    m_MapName( nMapName ),
    m_Stats( nStats ),
    m_MapCode( nMapCode ),
    m_MapGenre( nMapGenre ),
    m_MapRatings( nMapRatings ),
    m_MapRatingsCount( nMapRatingsCount ),
    m_SlotsOpen( nSlotsOpen ),
    m_SlotsTotal( nSlotsTotal ),
    m_BotName( nBotName ),
    m_BotLogin( nBotLogin ),
    m_RefreshTicks( nRefreshTicks ),
    m_ComparatorTime( 0 ),
    m_GameSlots( nGameSlots )
{
    uint32_t UpTime = UTIL_ByteArrayToUInt32( BYTEARRAY( m_UpTime.begin( ) , m_UpTime.begin( ) + 4 ), false );

    if( UpTime >= GetTime( ) )
        m_StartTime = 0;
    else
        m_StartTime = GetTime( ) - UpTime;

    m_MapFileName.clear( );

    for( int j = m_MapPath.size( ) - 1; j >= 0; j-- )
    {
        if( m_MapPath[j] == '/' || m_MapPath[j] == '\\' )
        {
            if( j < (int)m_MapPath.size( ) - 1 )
                m_MapFileName = BYTEARRAY( m_MapPath.begin( ) + j + 1, m_MapPath.end( ) );

            break;
        }
    }
}

CGameData::~CGameData( )
{

}

BYTEARRAY CGameData::GetPacket( )
{
    if( m_Packet.empty( ) )
    {
        // 2 bytes						-> Header
        // 2 bytes						-> Length
        // 4 bytes						-> Game Counter
        // 4 bytes						-> Game Number
        // 1 byte						-> TFT/RoC
        // 1 byte						-> War3 version
        // 1 byte						-> Game Status
        // 2 bytes						-> Host Port
        // 2 bytes						-> Map Width
        // 2 bytes						-> Map Height
        // 4 bytes						-> Map Game Type
        // 4 bytes						-> Map Game Flags
        // 4 bytes						-> Map CRC
        // 4 bytes						-> Host Counter
        // 4 bytes						-> Entry Key
        // 4 bytes						-> Up Time
        // 4 bytes						-> Creation Time
        // null terminated string		-> Game Name
        // null terminated string		-> Game Owner
        // null terminated string		-> Map Path
        // null terminated string		-> Host Name
        // null terminated string		-> Host IP
        // null terminated string		-> TGA Image
        // null terminated string		-> Map Players
        // null terminated string		-> Map Description
        // null terminated string		-> Map Name
        // null terminated string		-> Stats
        // null terminated string		-> Bot Name
        // null terminated string		-> Bot Login
        // null terminated string		-> Map Code
        // null terminated string		-> Map Genre
        // 4 bytes						-> Map Ratings
        // 4 bytes						-> Map Ratings Count
        // 1 byte						-> Slots Open
        // 1 byte						-> Slots Total
        // for( i = 0; i < Slots Total; i++ )
        // {
        // 		1 byte					-> Slot Download Status
        // 		1 byte					-> Slot Team
        // 		1 byte					-> Slot Colour
        // 		1 byte					-> Slot Race
        // 		1 byte					-> Slot Handicap
        // 		1 byte					-> Slot Status
        // 		null terminated string	-> Player Name
        // 		null terminated string	-> Player Server
        // 		4 bytes					-> Player Ping
        // }
        // 4 bytes                      -> Refresh Ticks < данное значение помещаем в самый конец, для того чтобы была возможность быстро его изменять

        m_Packet.push_back( UNM_HEADER_CONSTANT );                                  // UNM header constant
        m_Packet.push_back( CUNMProtocol::GET_GAME_RESPONSE );                      // GET_GAME_RESPONSE
        m_Packet.push_back( 0 );                                                    // packet length will be assigned later
        m_Packet.push_back( 0 );                                                    // packet length will be assigned later
        UTIL_AppendByteArray( m_Packet, GetGameCounter( ), false );					// Game Counter
        UTIL_AppendByteArray( m_Packet, GetGameNumber( ), false );					// Game Number
        m_Packet.push_back( GetTFT( ) );											// TFT/RoC
        m_Packet.push_back( GetWar3Version( ) );									// War3 Version
        m_Packet.push_back( GetGameStatus( ) );										// Game Status
        UTIL_AppendByteArrayFast( m_Packet, m_HostPort );                           // Host Port
        UTIL_AppendByteArrayFast( m_Packet, m_MapWidth );                           // Map Width
        UTIL_AppendByteArrayFast( m_Packet, m_MapHeight );                          // Map Height
        UTIL_AppendByteArrayFast( m_Packet, m_MapGameType );                        // Map Game Type
        UTIL_AppendByteArrayFast( m_Packet, m_MapGameFlags );                       // Map Game Flags
        UTIL_AppendByteArrayFast( m_Packet, m_MapCRC );                             // Map CRC
        UTIL_AppendByteArrayFast( m_Packet, m_HostCounter );                        // Host Counter
        UTIL_AppendByteArrayFast( m_Packet, m_EntryKey );                           // Entry Key
        UTIL_AppendByteArrayFast( m_Packet, m_UpTime );                             // Up Time

        if( GetGameStatus( ) == 0 )
            UTIL_AppendByteArray( m_Packet, GetTime( ) - m_CreationTime, false );   // Creation Time
        else
            UTIL_AppendByteArray( m_Packet, GetTime( ) - m_StartTime, false );      // Creation Time

        UTIL_AppendByteArrayFast( m_Packet, m_GameName );                           // Game Name
        UTIL_AppendByteArrayFast( m_Packet, m_GameOwner );                          // Game Owner
        UTIL_AppendByteArrayFast( m_Packet, m_MapPath );                            // Map Path
        UTIL_AppendByteArrayFast( m_Packet, m_HostName );                           // Host Name
        UTIL_AppendByteArrayFast( m_Packet, m_HostIP );                             // Host IP
        UTIL_AppendByteArrayFast( m_Packet, m_TGAImage );                           // TGA Image
        UTIL_AppendByteArrayFast( m_Packet, m_MapPlayers );                         // Map Players
        UTIL_AppendByteArrayFast( m_Packet, m_MapDescription );                     // Map Description
        UTIL_AppendByteArrayFast( m_Packet, m_MapName );                            // Map Name
        UTIL_AppendByteArrayFast( m_Packet, m_Stats );                              // Stats
        UTIL_AppendByteArrayFast( m_Packet, m_BotName );                            // Bot Name
        UTIL_AppendByteArrayFast( m_Packet, m_BotLogin );                           // Bot Login
        UTIL_AppendByteArrayFast( m_Packet, m_MapCode );                            // Map Code
        UTIL_AppendByteArrayFast( m_Packet, m_MapGenre );                           // Map Genre
        UTIL_AppendByteArrayFast( m_Packet, m_MapRatings );                         // Map Ratings
        UTIL_AppendByteArrayFast( m_Packet, m_MapRatingsCount );                    // Map Ratings Count
        m_Packet.push_back( m_SlotsOpen );                                          // Slots Open
        m_Packet.push_back( m_SlotsTotal );                                         // Slots Total

        for( uint32_t i = 0; i < m_GameSlots.size( ); i++ )
        {
            m_Packet.push_back( m_GameSlots[i].GetSlotDownloadStatus( ) );			// Slot Download Status
            m_Packet.push_back( m_GameSlots[i].GetSlotTeam( ) );					// Slot Team
            m_Packet.push_back( m_GameSlots[i].GetSlotColour( ) );					// Slot Colour
            m_Packet.push_back( m_GameSlots[i].GetSlotRace( ) );					// Slot Race
            m_Packet.push_back( m_GameSlots[i].GetSlotHandicap( ) );				// Slot Handicap
            m_Packet.push_back( m_GameSlots[i].GetSlotStatus( ) );                  // Slot Status
            UTIL_AppendByteArrayFast( m_Packet, m_GameSlots[i].GetPlayerName( ) );  // Player Name
            UTIL_AppendByteArrayFast( m_Packet, m_GameSlots[i].GetPlayerServer( ) );// Player Server
            UTIL_AppendByteArrayFast( m_Packet, m_GameSlots[i].GetPlayerPing( ) );  // Player Ping
        }

        UTIL_AppendByteArray( m_Packet, GetTicks( ) - m_RefreshTicks, false );		// Refresh Ticks
        AssignLength( m_Packet );
    }
    else if( m_Packet.size( ) > 49 )
    {
        BYTEARRAY RefreshTicks = UTIL_CreateByteArray( GetTicks( ) - m_RefreshTicks, false );
        BYTEARRAY CreationTime = UTIL_CreateByteArray( GetTime( ) - m_CreationTime, false );

        if( RefreshTicks.size( ) >= 4 )
        {
            m_Packet[ m_Packet.size( ) - 4 ] = RefreshTicks[0];
            m_Packet[ m_Packet.size( ) - 3 ] = RefreshTicks[1];
            m_Packet[ m_Packet.size( ) - 2 ] = RefreshTicks[2];
            m_Packet[ m_Packet.size( ) - 1 ] = RefreshTicks[3];
        }

        if( CreationTime.size( ) >= 4 )
        {
            m_Packet[45] = RefreshTicks[0];
            m_Packet[46] = RefreshTicks[1];
            m_Packet[47] = RefreshTicks[2];
            m_Packet[48] = RefreshTicks[3];
        }
    }

    return m_Packet;
}

bool CGameData::UpdateGameData( BYTEARRAY data )
{
    // 2 bytes						-> Header
    // 2 bytes						-> Length
    // 4 bytes						-> Game Number
    // 1 byte						-> TFT/RoC
    // 1 byte						-> War3 version
    // 1 byte						-> Game Status
    // 2 bytes						-> Host Port
    // 2 bytes						-> Map Width
    // 2 bytes						-> Map Height
    // 4 bytes						-> Map Game Type
    // 4 bytes						-> Map Game Flags
    // 4 bytes						-> Map CRC
    // 4 bytes						-> Host Counter
    // 4 bytes						-> Entry Key
    // 4 bytes						-> Up Time
    // null terminated string		-> Game Name
    // null terminated string		-> Game Owner
    // null terminated string		-> Map Path
    // null terminated string		-> Host Name
    // null terminated string		-> Host IP
    // null terminated string		-> TGA Image
    // null terminated string		-> Map Players
    // null terminated string		-> Map Description
    // null terminated string		-> Map Name
    // null terminated string		-> Stats
    // 1 byte						-> Slots Open
    // 1 byte						-> Slots Total
    // for( i = 0; i < Slots Total; i++ )
    // 		1 byte					-> Slot Download Status
    // 		1 byte					-> Slot Team
    // 		1 byte					-> Slot Colour
    // 		1 byte					-> Slot Race
    // 		1 byte					-> Slot Handicap
    // 		1 byte					-> Slot Status
    // 		null terminated string	-> Player Name
    // 		null terminated string	-> Player Server
    // 		4 bytes					-> Player Ping

    if( ValidateLength( data ) && data.size( ) >= 63 )
	{
        m_ComparatorTime = 0;
        m_RefreshTicks = GetTicks( );
        m_Packet.clear( );
		m_GameNumber = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );
		m_TFT = data[8];
		m_War3Version = data[9];

        if( m_GameStatus == 0 && data[10] != m_GameStatus )
                m_StartTime = GetTime( );

        m_GameStatus = data[10];
		m_HostPort = BYTEARRAY( data.begin( ) + 11, data.begin( ) + 13 );
		m_MapWidth = BYTEARRAY( data.begin( ) + 13, data.begin( ) + 15 );
		m_MapHeight = BYTEARRAY( data.begin( ) + 15, data.begin( ) + 17 );
		m_MapGameType = BYTEARRAY( data.begin( ) + 17, data.begin( ) + 21 );
		m_MapGameFlags = BYTEARRAY( data.begin( ) + 21, data.begin( ) + 25 );
		m_MapCRC = BYTEARRAY( data.begin( ) + 25, data.begin( ) + 29 );
		m_HostCounter = BYTEARRAY( data.begin( ) + 29, data.begin( ) + 33 );
		m_EntryKey = BYTEARRAY( data.begin( ) + 33, data.begin( ) + 37 );
		m_UpTime = BYTEARRAY( data.begin( ) + 37, data.begin( ) + 41 );
        uint32_t i = 41;
		m_GameName = UTIL_ExtractCString( data, i );
		i += m_GameName.size( ) + 1;
		m_GameOwner = UTIL_ExtractCString( data, i );
		i += m_GameOwner.size( ) + 1;
		m_MapPath = UTIL_ExtractCString( data, i );
        m_MapFileName.clear( );

        for( int j = m_MapPath.size( ) - 1; j >= 0; j-- )
        {
            if( m_MapPath[j] == '/' || m_MapPath[j] == '\\' )
            {
                if( j < (int)m_MapPath.size( ) - 1 )
                    m_MapFileName = BYTEARRAY( m_MapPath.begin( ) + j + 1, m_MapPath.end( ) );

                break;
            }
        }

		i += m_MapPath.size( ) + 1;
		m_HostName = UTIL_ExtractCString( data, i );
		i += m_HostName.size( ) + 1;
		m_HostIP = UTIL_ExtractCString( data, i );
		i += m_HostIP.size( ) + 1;
		m_TGAImage = UTIL_ExtractCString( data, i );
		i += m_TGAImage.size( ) + 1;
		m_MapPlayers = UTIL_ExtractCString( data, i );
		i += m_MapPlayers.size( ) + 1;
		m_MapDescription = UTIL_ExtractCString( data, i );
		i += m_MapDescription.size( ) + 1;
		m_MapName = UTIL_ExtractCString( data, i );
		i += m_MapName.size( ) + 1;
        m_Stats = UTIL_ExtractCString( data, i );
        i += m_Stats.size( ) + 1;
        unsigned char SlotsTotalTest = data[i+1];

		if( SlotsTotalTest == 0 || SlotsTotalTest > 24 )
            return false;

        m_SlotsOpen = data[i];
        m_SlotsTotal = data[i+1];
        i += 2;
		bool NewSlots = false;

		if( SlotsTotalTest != m_GameSlots.size( ) )
		{
			m_GameSlots.clear( );
			NewSlots = true;
		}

		while( SlotsTotalTest > 0 )
		{
			if( data.size( ) < i + 12 )
                return false;

			unsigned char SlotDownloadStatus = data[i];
			unsigned char SlotTeam = data[i+1];
			unsigned char SlotColour = data[i+2];
			unsigned char SlotRace = data[i+3];
			unsigned char SlotHandicap = data[i+4];
			unsigned char SlotStatus = data[i+5];
			i += 6;
			BYTEARRAY PlayerName = UTIL_ExtractCString( data, i );
			i += PlayerName.size( ) + 1;

			if( data.size( ) < i + 5 )
                return false;

			BYTEARRAY PlayerServer = UTIL_ExtractCString( data, i );
			i += PlayerServer.size( ) + 1;

			if( data.size( ) < i + 4 )
                return false;

			BYTEARRAY PlayerPing = BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 );
            i += 4;

			if( NewSlots )
				m_GameSlots.push_back( CGameSlotData( SlotDownloadStatus, SlotTeam, SlotColour, SlotRace, SlotHandicap, SlotStatus, PlayerName, PlayerServer, PlayerPing ) );
			else
				m_GameSlots[m_GameSlots.size( ) - SlotsTotalTest].UpdateData( SlotDownloadStatus, SlotTeam, SlotColour, SlotRace, SlotHandicap, SlotStatus, PlayerName, PlayerServer, PlayerPing );

			SlotsTotalTest--;
		}

        return true;
	}

    return false;
}

uint32_t CGameData::GetComparatorTime( )
{
    // список игр формируется следующим образом:
    // все неначатые игры находятся сверху списка, начатые снизу
    // кроме того неначатые игры сортируются еще и по времени первого обновления игры (чем игра новее - тем она выше)
    // начатые игры в свою очередь сортируются по времени начала игры (чем игра позже началась - тем она выше)
    // если же случилось так, что первый рефреш игры произошел после старта игры (то есть время старта уже невозможно определить на стороне сервера) - в этом случае мы полагаемся на UpTime, который отправляет юзер: CreationStartTime = GetTime( ) - UpTime

    if( m_ComparatorTime == 0 )
    {
        if( m_GameStatus == 0 )
            m_ComparatorTime = m_CreationTime;
        else
            m_ComparatorTime = 100000000 + m_StartTime;
    }

    return m_ComparatorTime;
}

void CGameData::SetMapRating( string nMapCode, string nMapGenre, uint32_t nMapRatings, uint32_t nMapRatingsCount  )
{
    m_MapCode = BYTEARRAY( nMapCode.begin( ), nMapCode.end( ) );
    m_MapGenre = BYTEARRAY( nMapGenre.begin( ), nMapGenre.end( ) );
    m_MapRatings = BYTEARRAY( 4 );
    *(uint32_t*)&m_MapRatings[0] = nMapRatings;
    m_MapRatingsCount = BYTEARRAY( 4 );
    *(uint32_t*)&m_MapRatingsCount[0] = nMapRatingsCount;
}

//
// CPlayerInfo
//

CPlayerInfo::CPlayerInfo( uint32_t nGameNumber, string nUserName, string nUserIP ) :
    m_GameNumber( nGameNumber ),
    m_UserName( nUserName ),
    m_UserIP( nUserIP )
{

}

CPlayerInfo::~CPlayerInfo( )
{

}

//
// CBotInfo
//

CBotInfo::CBotInfo( string nBotName, string nMapListName ) :
    m_BotName( nBotName ),
    m_MapListName( nMapListName )
{

}

CBotInfo::~CBotInfo( )
{

}

//
// CMapList
//

CMapList::CMapList( BYTEARRAY nMapListNameArray, vector<BYTEARRAY> nMapFileNamesArray )
{
    m_MapListNameArray = nMapListNameArray;
    m_MapListName = string( m_MapListNameArray.begin( ), m_MapListNameArray.end( ) );
    m_MapFileNamesArray = nMapFileNamesArray;
    m_MapFileNames.clear( );

    for( uint32_t i = 0; i != m_MapFileNamesArray.size( ); i++ )
        m_MapFileNames.push_back( string( m_MapFileNamesArray[i].begin( ), m_MapFileNamesArray[i].end( ) ) );
}

CMapList::~CMapList( )
{

}
