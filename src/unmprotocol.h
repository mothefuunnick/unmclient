#ifndef UNMPROTOCOL_H
#define UNMPROTOCOL_H

//
// CUNMProtocol
//

//#define UNM_SERVER
#define UNM_CLIENT
#define UNM_BOT
#define UNM_HEADER_CONSTANT 255
#define UNM_HEADER_CONSTANT_BIG 254
#define CURRENT_VERSION 1
#define MINIMUM_CLIENT_VERSION 1
#define MINIMUM_BOT_VERSION 1
#define LOGON_RESPONSE_DB_ERROR 0
#define LOGON_RESPONSE_NOT_EXIST 1
#define LOGON_RESPONSE_WRONG_PASSWORD 2
#define LOGON_RESPONSE_BETA_TESTERS_ONLY 3
#define LOGON_RESPONSE_SUCCESS 4
#define LOGON_RESPONSE_UNNKOWN_ERROR 5
#define REGISTRY_RESPONSE_DB_ERROR 0
#define REGISTRY_RESPONSE_ALREADY_TAKEN 1
#define REGISTRY_RESPONSE_INVALID_LOGIN 2
#define REGISTRY_RESPONSE_INVALID_PASSWORD 3
#define REGISTRY_RESPONSE_SUCCESS 4
#define REGISTRY_RESPONSE_UNNKOWN_ERROR 5
#define SMART_LOGON_RESPONSE_DB_ERROR 0
#define SMART_LOGON_RESPONSE_WRONG_PASSWORD 1
#define SMART_LOGON_RESPONSE_BOTNAME_ALREADY_TAKEN 2
#define SMART_LOGON_RESPONSE_INVALID_BOTNAME 3
#define SMART_LOGON_RESPONSE_INVALID_LOGIN 4
#define SMART_LOGON_RESPONSE_INVALID_PASSWORD 5
#define SMART_LOGON_RESPONSE_INVALID_IP 6
#define SMART_LOGON_RESPONSE_INVALID_PORT 7
#define SMART_LOGON_RESPONSE_INVALID_MAPLISTNAME 8
#define SMART_LOGON_RESPONSE_DENIED_ACCESS_TO_USE_PUBLIC_BOT 9
#define SMART_LOGON_RESPONSE_DENIED_ACCESS_TO_CREATE_GAME 10
#define SMART_LOGON_RESPONSE_DENIED_ACCESS_TO_USE_THIS_IP 11
#define SMART_LOGON_RESPONSE_DENIED_ACCESS_TO_USE_THIS_BOTNAME 12
#define SMART_LOGON_RESPONSE_DENIED_ACCESS_TO_USE_THIS_MAPLISTNAME 13
#define SMART_LOGON_RESPONSE_SUCCESS 14
#define SMART_LOGON_RESPONSE_UNNKOWN_ERROR 15

#ifdef UNM_CLIENT
#include "gameslot.h"
#endif
#include "bnetprotocol.h"

#ifdef UNM_SERVER
class CUnmUser;
#else
class CGamePlayer;
#endif

class CGamePlayer;
class CBNETProtocol;
class CIncomingChatEvent;
class CSmartLogonBot;
class CGameSlotData;
class CGameData;
class CPlayerInfo;
class CUserInfo;
class CGameHeader;
class CBotInfo;
class CMapList;

class CUNMProtocol
{
public:
	enum Protocol
	{
        CLIENT_INFO = 1,                    // клиент отпрвляет свою версию и тип (юзер или бот)
        CLIENT_INFO_RESPONSE = 2,           // сервер отвечает на CLIENT_INFO (может подключиться клиент или нет)
        SMART_LOGON_BOT = 3,                // бот отправляет при логине/регистрации
        SMART_LOGON_BOT_RESPONSE = 4,       // сервер отвечает на SMART_LOGON_BOT (валидные данные или нет)
        CHAT_MESSAGE = 5,                   // клиент отправляет сообщение (которое может являться командой. личные сообщения и некоторые запросы реализованы через команды)
        CHAT_EVENT_RESPONSE = 6,            // сервер отправляет событие чата (в частности сообщения юзеру)
        GAME_REFRESH = 7,                   // бот отправляет при обновлении своей игры
        GAME_REFRESH_RESPONSE = 8,          // сервер отвечает на GAME_REFRESH (успех или нет)
        CHECK_JOINED_PLAYER = 9,            // бот запрашивает инфу о юзере (который зашел к боту в игру) для спуфчека
        CHECK_JOINED_PLAYER_RESPONSE = 10,  // сервер отвечает боту на CHECK_JOINED_PLAYER
        REQUEST_JOIN_PLAYER_RESPONSE = 11,  // сервер отправляет боту инфу о юзере, который хочет зайти в игру бота
        GAME_UNHOST = 12,                   // бот отправляет при удалении своей игры
        GAME_UNHOST_RESPONSE = 13,          // сервер отвечает на GAME_UNHOST (успех или нет)
        LOGON_CLIENT = 14,                  // юзер логиниться на сервер
        LOGON_CLIENT_RESPONSE = 15,         // сервер отвечает юзеру на пакет LOGON_CLIENT (удалось залогиниться или нет)
        REGISTRY_CLIENT = 16,               // юзер регистрирует новый ник на сервере
        REGISTRY_CLIENT_RESPONSE = 17,      // сервер отвечает юзеру на пакет REGISTRY_CLIENT (зарегистрирован новый ник или нет)
        GET_GAMELIST = 18,                  // юзер запрашивает список игр
        GET_GAMELIST_RESPONSE = 19,         // сервер отправляет список игр юзеру
        GET_GAME = 20,                      // юзер запрашивает инфу о конкретной игре по ее номеру (юзер получает номер из списка)
        GET_GAME_RESPONSE = 21,             // сервер отправляет инфу о конкретной игре юзеру
        GET_BOTLIST = 22,                   // юзер запрашивает список публичных ботов
        GET_BOTLIST_RESPONSE = 23,          // сервер отправляет список публичных ботов юзеру
        GET_MAPLIST = 24,                   // юзер запрашивает список карт по его названию (для списка карт используется отдельный класс и список карт запрашивается/отправляется отдельным пакетом, потому что некоторые публичные боты могут использоваться общий список карт. класс публичного бота не содержит самого списка карт, а только его название)
        GET_MAPLIST_RESPONSE = 25,          // сервер отправляет список карт
        CHANGE_PASSWORD = 26,               // клиент отправляет запрос на изменение своего пароля
        CHANGE_PASSWORD_RESPONSE = 27,      // сервер отвечает клиенту на пакет CHANGE_PASSWORD (смена пароля прошла успешно или нет)
        SET_MAPLIST = 28,                   // бот отправляет серверу свой список список карт (название:список)
        SET_MAPLIST_RESPONSE = 29,          // сервер отвечает боту на пакет SET_MAPLIST (успешно ли задан/изменен список карт)
        ADD_MAPLIST = 30,                   // бот отправляет серверу запрос на добавление карт в существующий список карт
        ADD_MAPLIST_RESPONSE = 31,          // сервер отвечает боту на пакет ADD_MAPLIST (имеет право бот редактировать данный список карт или нет)
        REMOVE_MAPLIST = 32,                // бот отправляет серверу запрос на удаление указанных карт из существующего списка карт
        REMOVE_MAPLIST_RESPONSE = 33,       // сервер отвечает боту на пакет REMOVE_MAPLIST (имеет право бот редактировать данный список карт или нет)
        CLEAR_MAPLIST = 34,                 // бот отправляет серверу запрос на очистку существующего списка карт
        CLEAR_MAPLIST_RESPONSE = 35,        // сервер отвечает боту на пакет CLEAR_MAPLIST (имеет право бот редактировать данный список карт или нет)
        GET_MAPLIST_COUNT = 34,             // юзер запрашивает инфу о количестве карт в списке (по названию списка)
        GET_MAPLIST_COUNT_RESPONSE = 35,    // сервер отвечает юзеру на пакет GET_MAPLIST_COUNT (количество карт)
        PING_FROM_HOST = 36,                // сервер рассылает всем клиентам пинг пакеты
        PONG_TO_HOST = 37,                  // клиент отвечают серверу на каждый пинг пакет
        RATE_MAP = 38,                      // юзер ставит оценку карте
        RATE_MAP_RESPONSE = 39,             // сервер отвечает юзеру была ли оценка поставлена
        GAME_DELETE = 40,                   // юзер "закрыл" у себя вкладку с игрой, о которой ранее запросил инфу
        GAME_DELETE_RESPONSE = 41           // после анхоста игры на сервере, сервер оповещает всех юзеров, которым была отправлена инфа о данной игре, о том, что данная игра была удалена
	};

	CUNMProtocol( );
	~CUNMProtocol( );

	// receive functions (client)
#ifdef UNM_CLIENT
	unsigned char RECEIVE_CLIENT_INFO_RESPONSE( BYTEARRAY data );
	unsigned char RECEIVE_SMART_LOGON_BOT_RESPONSE( BYTEARRAY data );
    unsigned char RECEIVE_GAME_REFRESH_RESPONSE( BYTEARRAY data, uint32_t &GameNumber, uint32_t &GameCounter );
    CIncomingChatEvent *RECEIVE_CHAT_EVENT_RESPONSE( BYTEARRAY data );
	unsigned char RECEIVE_CHECK_JOINED_PLAYER_RESPONSE( BYTEARRAY data );
	CPlayerInfo *RECEIVE_REQUEST_JOIN_PLAYER_RESPONSE( BYTEARRAY data );
    uint32_t RECEIVE_GAME_UNHOST_RESPONSE( BYTEARRAY data, uint32_t &GameCounter );
    uint32_t RECEIVE_GAME_UNHOST_RESPONSE( BYTEARRAY data );
    unsigned char RECEIVE_LOGON_CLIENT_RESPONSE( BYTEARRAY data );
    unsigned char RECEIVE_REGISTRY_CLIENT_RESPONSE( BYTEARRAY data );
    std::vector<CGameHeader *> RECEIVE_GET_GAMELIST_RESPONSE( BYTEARRAY data );
    CGameData *RECEIVE_GET_GAME_RESPONSE( BYTEARRAY data );
    std::vector<CBotInfo *> RECEIVE_GET_BOTLIST_RESPONSE( BYTEARRAY data );
    CMapList *RECEIVE_GET_MAPLIST_RESPONSE( BYTEARRAY data );
    unsigned char RECEIVE_CHANGE_PASSWORD_RESPONSE( BYTEARRAY data );
    unsigned char RECEIVE_SET_MAPLIST_RESPONSE( BYTEARRAY data );
    unsigned char RECEIVE_ADD_MAPLIST_RESPONSE( BYTEARRAY data );
    unsigned char RECEIVE_REMOVE_MAPLIST_RESPONSE( BYTEARRAY data );
    unsigned char RECEIVE_CLEAR_MAPLIST_RESPONSE( BYTEARRAY data );
    uint32_t RECEIVE_GET_MAPLIST_COUNT_RESPONSE( BYTEARRAY data, std::string &MapListName );
    uint32_t RECEIVE_PING_FROM_HOST_RESPONSE( BYTEARRAY data );
    bool RECEIVE_RATE_MAP_RESPONSE( BYTEARRAY data );
    uint32_t RECEIVE_GAME_DELETE_RESPONSE( BYTEARRAY data );
#endif

	// receive functions (server)
#ifdef UNM_SERVER
	unsigned char RECEIVE_CLIENT_INFO( BYTEARRAY data, unsigned char &ClientType, uint32_t &ClientVersion );
	CSmartLogonBot *RECEIVE_SMART_LOGON_BOT( BYTEARRAY data );
    uint32_t RECEIVE_GAME_REFRESH( BYTEARRAY data );
    CGameData *RECEIVE_GAME_REFRESH( BYTEARRAY data, std::string BotName, std::string BotLogin, uint32_t &GameCounter );
    std::string RECEIVE_CHAT_MESSAGE( BYTEARRAY data );
	CPlayerInfo *RECEIVE_CHECK_JOINED_PLAYER( BYTEARRAY data );
	uint32_t RECEIVE_GAME_UNHOST( BYTEARRAY data );
    CUserInfo *RECEIVE_LOGON_CLIENT( BYTEARRAY data );
    CUserInfo *RECEIVE_REGISTRY_CLIENT( BYTEARRAY data );
    bool RECEIVE_GET_GAMELIST( BYTEARRAY data );
    uint32_t RECEIVE_GET_GAME( BYTEARRAY data );
    bool RECEIVE_GET_BOTLIST( BYTEARRAY data );
    std::string RECEIVE_GET_MAPLIST( BYTEARRAY data );
    std::string RECEIVE_CHANGE_PASSWORD( BYTEARRAY data, std::string &NewPassword );
    CMapList *RECEIVE_SET_MAPLIST( BYTEARRAY data );
    CMapList *RECEIVE_ADD_MAPLIST( BYTEARRAY data );
    CMapList *RECEIVE_REMOVE_MAPLIST( BYTEARRAY data );
    std::string RECEIVE_CLEAR_MAPLIST( BYTEARRAY data );
    std::string RECEIVE_GET_MAPLIST_COUNT( BYTEARRAY data );
    uint32_t RECEIVE_PONG_TO_HOST( BYTEARRAY data );
    bool RECEIVE_RATE_MAP( BYTEARRAY data, std::string &MapCode, uint32_t &PlayerVote );
    uint32_t RECEIVE_GAME_DELETE( BYTEARRAY data );
#endif

	// send functions (client)
#ifdef UNM_CLIENT
	BYTEARRAY SEND_CLIENT_INFO( );
    BYTEARRAY SEND_SMART_LOGON_BOT( unsigned char nBotType, unsigned char nCanCreateGame, uint16_t nHostPort, std::string nUserName, std::string nUserPassword, std::string nIP, std::string nBotName, std::string nMapListName );
    BYTEARRAY SEND_GAME_REFRESH( uint32_t GameNumber, bool LCPing, bool TFT, unsigned char War3Version, bool GameLoading, bool GameLoaded, uint16_t HostPort, BYTEARRAY MapWidth, BYTEARRAY MapHeight, BYTEARRAY MapGameType, BYTEARRAY MapGameFlags, BYTEARRAY MapCRC, uint32_t HostCounter, uint32_t EntryKey, uint32_t UpTime, std::string GameName, std::string GameOwner, std::string MapPath, std::string HostName, std::string HostIP, std::string TGAImage, std::string MapPlayers, std::string MapDescription, std::string MapName, std::string Stats, unsigned char SlotsOpen, std::vector<CGameSlot> &GameSlots, std::vector<CGamePlayer *> players );
    BYTEARRAY SEND_CHAT_MESSAGE( std::string command, unsigned char manual, unsigned char botid );
    BYTEARRAY SEND_CHAT_MESSAGE( std::string command );
    BYTEARRAY SEND_CHECK_JOINED_PLAYER( uint32_t GameNumber, std::string UserName, std::string UserIP );
	BYTEARRAY SEND_GAME_UNHOST( uint32_t GameNumber );
    BYTEARRAY SEND_LOGON_CLIENT( std::string nUserName, std::string nUserPassword );
    BYTEARRAY SEND_REGISTRY_CLIENT( std::string nUserName, std::string nUserPassword );
    BYTEARRAY SEND_GET_GAMELIST( );
    BYTEARRAY SEND_GET_GAME( uint32_t GameCounter );
    BYTEARRAY SEND_GET_BOTLIST( );
    BYTEARRAY SEND_GET_MAPLIST( std::string MapListName );
    BYTEARRAY SEND_CHANGE_PASSWORD( std::string OldPassword, std::string NewPassword );
    BYTEARRAY SEND_SET_MAPLIST( CMapList *MapList );
    BYTEARRAY SEND_ADD_MAPLIST( CMapList *MapList );
    BYTEARRAY SEND_REMOVE_MAPLIST( CMapList *MapList );
    BYTEARRAY SEND_CLEAR_MAPLIST( std::string MapListName );
    BYTEARRAY SEND_GET_MAPLIST_COUNT( std::string MapListName );
    BYTEARRAY SEND_PONG_TO_HOST( uint32_t pong );
    BYTEARRAY SEND_RATE_MAP( std::string MapCode, uint32_t PlayerVote );
    BYTEARRAY SEND_GAME_DELETE( uint32_t GameCounter );
#endif

	// send functions (server)
#ifdef UNM_SERVER
	BYTEARRAY SEND_CLIENT_INFO_RESPONSE( unsigned char result );
	BYTEARRAY SEND_SMART_LOGON_BOT_RESPONSE( unsigned char result );
    BYTEARRAY SEND_GAME_REFRESH_RESPONSE( unsigned char result, uint32_t GameNumber, uint32_t GameCounter );
    BYTEARRAY SEND_CHAT_EVENT_RESPONSE( CBNETProtocol::IncomingChatEvent nChatEvent, uint32_t nUserFlags, uint32_t nPing, std::string nUser, std::string nMessage );
    BYTEARRAY SEND_CHECK_JOINED_PLAYER_RESPONSE( unsigned char result );
    BYTEARRAY SEND_REQUEST_JOIN_PLAYER_RESPONSE( uint32_t GameNumber, std::string UserName, std::string UserIP );
    BYTEARRAY SEND_GAME_UNHOST_RESPONSE( uint32_t GameNumber, uint32_t GameCounter );
    BYTEARRAY SEND_LOGON_CLIENT_RESPONSE( unsigned char result );
    BYTEARRAY SEND_REGISTRY_CLIENT_RESPONSE( unsigned char result );
    BYTEARRAY SEND_GET_GAMELIST_RESPONSE( BYTEARRAY games );
    BYTEARRAY SEND_GET_GAME_RESPONSE( CGameData *game );
    BYTEARRAY SEND_GET_BOTLIST_RESPONSE( vector<CBotInfo *> BotList );
    BYTEARRAY SEND_GET_MAPLIST_RESPONSE( CMapList *MapList );
    BYTEARRAY SEND_CHANGE_PASSWORD_RESPONSE( unsigned char result );
    BYTEARRAY SEND_SET_MAPLIST_RESPONSE( unsigned char result );
    BYTEARRAY SEND_ADD_MAPLIST_RESPONSE( unsigned char result );
    BYTEARRAY SEND_REMOVE_MAPLIST_RESPONSE( unsigned char result );
    BYTEARRAY SEND_CLEAR_MAPLIST_RESPONSE( unsigned char result );
    BYTEARRAY SEND_GET_MAPLIST_COUNT_RESPONSE( uint32_t MapListCount, std::string MapListName );
    BYTEARRAY SEND_PING_FROM_HOST_RESPONSE( );
    BYTEARRAY SEND_RATE_MAP_RESPONSE( bool reuslt );
    BYTEARRAY SEND_GAME_DELETE_RESPONSE( uint32_t GameCounter );
#endif
};

//
// CUserInfo
//

class CUserInfo
{
private:
    std::string m_UserName;
    std::string m_UserPassword;
    std::string m_IP;
    uint32_t m_Access;
    bool m_BetaTester;

public:
    CUserInfo( std::string nUserName, std::string nUserPassword );
    ~CUserInfo( );
    bool UserNameIsValid( );
    bool UserPasswordIsValid( );

    std::string GetUsername( )
    {
        return m_UserName;
    }
    std::string GetUserPassword( )
    {
        return m_UserPassword;
    }
    std::string GetIP( )
    {
        return m_IP;
    }
    uint32_t GetAccess( )
    {
        return m_Access;
    }
    bool GetBetaTester( )
    {
        return m_BetaTester;
    }
};

//
// CSmartLogonBot
//

class CSmartLogonBot
{
private:
    unsigned char m_BotType;
    unsigned char m_CanCreateGame;
    uint16_t m_HostPort;
    std::string m_UserName;
    std::string m_UserPassword;
    std::string m_IP;
    std::string m_BotName;
    std::string m_MapListName;

public:
    CSmartLogonBot( unsigned char nBotType, unsigned char nCanCreateGame, uint16_t nHostPort, std::string nUserName, std::string nUserPassword, std::string nIP, std::string nBotName, std::string nMapListName );
	~CSmartLogonBot( );

    unsigned char GetBotType( )
    {
        return m_BotType;
    }
    unsigned char GetCanCreateGame( )
    {
        return m_CanCreateGame;
    }
    uint16_t GetHostPort( )
    {
        return m_HostPort;
    }
    std::string GetUsername( )
	{
		return m_UserName;
	}
    std::string GetUserPassword( )
	{
		return m_UserPassword;
	}
    std::string GetIP( )
	{
		return m_IP;
	}
    std::string GetBotName( )
	{
		return m_BotName;
	}
    std::string GetMapListName( )
	{
        return m_MapListName;
	}
};

//
// CGameSlotData
//

class CGameSlotData
{
private:
	unsigned char m_SlotDownloadStatus;
	unsigned char m_SlotTeam;
	unsigned char m_SlotColour;
	unsigned char m_SlotRace;
	unsigned char m_SlotHandicap;
	unsigned char m_SlotStatus;
	BYTEARRAY m_PlayerName;
	BYTEARRAY m_PlayerServer;
	BYTEARRAY m_PlayerPing;

public:
	CGameSlotData( unsigned char nSlotDownloadStatus, unsigned char nSlotTeam, unsigned char nSlotColour, unsigned char nSlotRace, unsigned char nSlotHandicap, unsigned char nSlotStatus, BYTEARRAY nPlayerName, BYTEARRAY nPlayerServer, BYTEARRAY nPlayerPing );
	~CGameSlotData( );
    void UpdateData( unsigned char nSlotDownloadStatus, unsigned char nSlotTeam, unsigned char nSlotColour, unsigned char nSlotRace, unsigned char nSlotHandicap, unsigned char nSlotStatus, BYTEARRAY nPlayerName, BYTEARRAY nPlayerServer, BYTEARRAY nPlayerPing );
	
	unsigned char GetSlotDownloadStatus( )
	{
		return m_SlotDownloadStatus;
	}
	unsigned char GetSlotTeam( )
	{
		return m_SlotTeam;
	}
	unsigned char GetSlotColour( )
	{
		return m_SlotColour;
	}
	unsigned char GetSlotRace( )
	{
		return m_SlotRace;
	}
	unsigned char GetSlotHandicap( )
	{
		return m_SlotHandicap;
	}
	unsigned char GetSlotStatus( )
	{
		return m_SlotStatus;
	}
    BYTEARRAY& GetPlayerName( )
	{
		return m_PlayerName;
	}
    BYTEARRAY& GetPlayerServer( )
	{
		return m_PlayerServer;
	}
    BYTEARRAY& GetPlayerPing( )
	{
		return m_PlayerPing;
	}
};

//
// CGameHeader
//

class CGameHeader
{
private:
    uint32_t m_GameCounter;
    uint32_t m_SlotsOpen;
    uint32_t m_SlotsTotal;
    std::string m_GameName;
    std::string m_MapName;
    std::string m_MapFileName;
    std::string m_MapCode;
    std::string m_MapGenre;
    uint32_t m_MapRatings;
    uint32_t m_MapRatingsCount;
    std::string m_BotName;
    std::string m_BotLogin;
    std::string m_GameOwner;
    std::string m_Stats;

public:
    CGameHeader( uint32_t nGameCounter, uint32_t nSlotsOpen, uint32_t nSlotsTotal, std::string nGameName, std::string nMapName, std::string nMapFileName, std::string nMapCode, std::string nMapGenre, uint32_t nMapRatings, uint32_t nMapRatingsCount, std::string nBotName, std::string nBotLogin, std::string nGameOwner, std::string nStats );
    ~CGameHeader( );

    uint32_t GetGameCounter( )
    {
        return m_GameCounter;
    }
    uint32_t GetSlotsOccupied( )
    {
        return m_SlotsTotal - m_SlotsOpen;
    }
    uint32_t GetSlotsOpen( )
    {
        return m_SlotsOpen;
    }
    uint32_t GetSlotsTotal( )
    {
        return m_SlotsTotal;
    }
    std::string GetGameName( )
    {
        return m_GameName;
    }
    std::string GetMapName( )
    {
        return m_MapName;
    }
    std::string GetMapFileName( )
    {
        return m_MapFileName;
    }
    std::string GetMapCode( )
    {
        return m_MapCode;
    }
    std::string GetMapGenre( )
    {
        return m_MapGenre;
    }
    uint32_t GetMapRatings( )
    {
        return m_MapRatings;
    }
    uint32_t GetMapRatingsCount( )
    {
        return m_MapRatingsCount;
    }
    std::string GetBotName( )
    {
        return m_BotName;
    }
    std::string GetBotLogin( )
    {
        return m_BotLogin;
    }
    std::string GetGameOwner( )
    {
        return m_GameOwner;
    }
    std::string GetStats( )
    {
        return m_Stats;
    }
};

//
// CGameData
//

class CGameData
{
private:
    uint32_t m_GameCounter;
    uint32_t m_GameNumber;
	unsigned char m_TFT;
	unsigned char m_War3Version;
	unsigned char m_GameStatus;
	BYTEARRAY m_HostPort;
	BYTEARRAY m_MapWidth;
	BYTEARRAY m_MapHeight;
	BYTEARRAY m_MapGameType;
	BYTEARRAY m_MapGameFlags;
	BYTEARRAY m_MapCRC;
	BYTEARRAY m_HostCounter;
	BYTEARRAY m_EntryKey;
    BYTEARRAY m_UpTime;             // количество секунд прошедшее с момента создания игры, это значение присылает клиент (бот)
    uint32_t m_CreationTime;        // на стороне сервера данное значение = GetTime( ) первого обновления игры, а на стороне клиента = секунды прошедшие с первого обновления игры
	BYTEARRAY m_GameName;
	BYTEARRAY m_GameOwner;
	BYTEARRAY m_MapPath;
    BYTEARRAY m_MapFileName;
	BYTEARRAY m_HostName;
	BYTEARRAY m_HostIP;
	BYTEARRAY m_TGAImage;
	BYTEARRAY m_MapPlayers;
	BYTEARRAY m_MapDescription;
	BYTEARRAY m_MapName;
    BYTEARRAY m_Stats;
    BYTEARRAY m_MapCode;
    BYTEARRAY m_MapGenre;
    BYTEARRAY m_MapRatings;
    BYTEARRAY m_MapRatingsCount;
	unsigned char m_SlotsOpen;
	unsigned char m_SlotsTotal;
    std::string m_BotName;
    std::string m_BotLogin;
    uint32_t m_RefreshTicks;        // на стороне сервера данное значение = GetTicks( ) последнего обновления игры, а на стороне клиента = миллисекунды прошедшие с последнего обновления игры
    uint32_t m_StartTime;           // данное значение = GetTime( ) первого после старта обновления игры, если это первый рефреш этой игры значит значение будет = GetTime( ) - m_UpTime
    uint32_t m_ComparatorTime;      // это значение мы используем для сортировки списка игр на сервере
    BYTEARRAY m_Packet;

public:
    CGameData( uint32_t nGameCounter, uint32_t nGameNumber, unsigned char nTFT, unsigned char nWar3Version, unsigned char nGameStatus, BYTEARRAY nHostPort, BYTEARRAY nMapWidth, BYTEARRAY nMapHeight, BYTEARRAY nMapGameType, BYTEARRAY nMapGameFlags, BYTEARRAY nMapCRC, BYTEARRAY nHostCounter, BYTEARRAY nEntryKey, BYTEARRAY nUpTime, uint32_t nCreationTime, BYTEARRAY nGameName, BYTEARRAY nGameOwner, BYTEARRAY nMapPath, BYTEARRAY nHostName, BYTEARRAY nHostIP, BYTEARRAY nTGAImage, BYTEARRAY nMapPlayers, BYTEARRAY nMapDescription, BYTEARRAY nMapName, BYTEARRAY nStats, BYTEARRAY nMapCode, BYTEARRAY nMapGenre, BYTEARRAY nMapRatings, BYTEARRAY nMapRatingsCount, unsigned char nSlotsOpen, unsigned char nSlotsTotal, std::vector<CGameSlotData> nGameSlots, std::string nBotName, std::string nBotLogin, uint32_t nRefreshTicks );
	~CGameData( );
    std::vector<CGameSlotData> m_GameSlots;
    BYTEARRAY GetPacket( );
    bool UpdateGameData( BYTEARRAY data );

    uint32_t GetGameCounter( )
    {
        return m_GameCounter;
    }
	uint32_t GetGameNumber( )
	{
		return m_GameNumber;
	}
	unsigned char GetTFT( )
	{
		return m_TFT;
	}
	unsigned char GetWar3Version( )
	{
		return m_War3Version;
	}
	unsigned char GetGameStatus( )
	{
		return m_GameStatus;
	}
    BYTEARRAY& GetHostPort( )
	{
		return m_HostPort;
	}
    BYTEARRAY& GetMapWidth( )
	{
		return m_MapWidth;
	}
    BYTEARRAY& GetMapHeight( )
	{
		return m_MapHeight;
	}
    BYTEARRAY& GetMapGameType( )
	{
		return m_MapGameType;
	}
    BYTEARRAY& GetMapGameFlags( )
	{
		return m_MapGameFlags;
	}
    BYTEARRAY& GetMapCRC( )
	{
		return m_MapCRC;
	}
    BYTEARRAY& GetHostCounter( )
	{
		return m_HostCounter;
	}
    BYTEARRAY& GetEntryKey( )
	{
		return m_EntryKey;
	}
    BYTEARRAY& GetUpTime( )
	{
		return m_UpTime;
	}
    uint32_t GetCreationTime( )
	{
		return m_CreationTime;
	}
    BYTEARRAY& GetGameName( )
	{
		return m_GameName;
	}
    BYTEARRAY& GetGameOwner( )
	{
		return m_GameOwner;
	}
    BYTEARRAY& GetMapPath( )
	{
		return m_MapPath;
    }
    BYTEARRAY& GetMapFileName( )
    {
        return m_MapFileName;
    }
    BYTEARRAY& GetHostName( )
	{
		return m_HostName;
	}
    BYTEARRAY& GetHostIP( )
	{
		return m_HostIP;
	}
    BYTEARRAY& GetTGAImage( )
	{
		return m_TGAImage;
	}
    BYTEARRAY& GetMapPlayers( )
	{
		return m_MapPlayers;
	}
    BYTEARRAY& GetMapDescription( )
	{
		return m_MapDescription;
	}
    BYTEARRAY& GetMapName( )
	{
		return m_MapName;
	}
    BYTEARRAY& GetStats( )
    {
        return m_Stats;
    }
    BYTEARRAY& GetMapCode( )
    {
        return m_MapCode;
    }
    BYTEARRAY& GetMapGenre( )
    {
        return m_MapGenre;
    }
    BYTEARRAY& GetMapRatings( )
    {
        return m_MapRatings;
    }
    BYTEARRAY& GetMapRatingsCount( )
    {
        return m_MapRatingsCount;
    }
    unsigned char GetSlotsOccupied( )
    {
        return m_SlotsTotal - m_SlotsOpen;
    }
	unsigned char GetSlotsOpen( )
	{
		return m_SlotsOpen;
	}
	unsigned char GetSlotsTotal( )
	{
		return m_SlotsTotal;
    }
    std::string& GetBotName( )
    {
        return m_BotName;
    }
    std::string& GetBotLogin( )
    {
        return m_BotLogin;
    }
    uint32_t GetRefreshTicks( )
    {
        return m_RefreshTicks;
    }
    uint32_t GetComparatorTime( );
    void SetRefreshTicks( uint32_t nRefreshTicks )
    {
        m_RefreshTicks = nRefreshTicks;
    }
    void SetMapRating( std::string nMapCode, std::string nMapGenre, uint32_t nMapRatings, uint32_t nMapRatingsCount );
};

//
// CPlayerInfo
//

class CPlayerInfo
{
private:
	uint32_t m_GameNumber;
    std::string m_UserName;
    std::string m_UserIP;

public:
    CPlayerInfo( uint32_t nGameNumber, std::string nUserName, std::string nUserIP );
	~CPlayerInfo( );

	uint32_t GetGameNumber( )
	{
		return m_GameNumber;
	}
    std::string GetUsername( )
	{
		return m_UserName;
	}
    std::string GetUserIP( )
	{
		return m_UserIP;
	}
};

//
// CBotInfo
//

class CBotInfo
{
public:
    CBotInfo( std::string nBotName, std::string nMapListName );
    ~CBotInfo( );

    std::string m_BotName;
    std::string m_MapListName;
};

//
// CMapList
//

class CMapList
{
public:
    CMapList( BYTEARRAY nMapListNameArray, std::vector<BYTEARRAY> nMapFileNamesArray );
    ~CMapList( );

    std::string m_MapListName;
    BYTEARRAY m_MapListNameArray;
    std::vector<std::string> m_MapFileNames;
    std::vector<BYTEARRAY> m_MapFileNamesArray;
};

#endif
