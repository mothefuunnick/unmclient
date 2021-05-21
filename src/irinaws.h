#ifndef IRINAWS_H
#define IRINAWS_H

//
// CIrinaWS
//

#define IRINAWS_HEADER_CONSTANT 1
#define IRINALocal_HEADER_CONSTANT 2
#define IRINAGlobal_HEADER_CONSTANT 0
#define IRINAWSString_HEADER_CONSTANT 3

class CIrinaGameDataEX;
class CIrinaGameData;
class CIrinaUser;
class CIrinaSlots;
class CIrinaGame;

class CIrinaWS
{
public:
    CUNM *m_UNM;

    enum IRINAWS
	{
        INIT = 0,
        GET_GAMELIST = 1,
        GET_GAMEDATA = 3,
        PING = 5,
        GET_GAMEDATAEX = 13
	};

    enum IRINALocal
    {
        SET_GAMEDATA = 1,
        CONNECTED = 2,
        DISCONECTED = 3,
        SET_GAMEDATAEX = 4
    };

    enum IRINAGlobal
    {
        LOGIN = 3
    };

    CIrinaWS( CUNM *nUNM );
    ~CIrinaWS( );
    CIrinaUser *m_User;
    bool Update( );
    bool AddGame( CIrinaGame *game  );
    BYTEARRAY SEND_GPSC_SPOOFCHECK_IRINA( );
    BYTEARRAY SEND_W3GS_GAMEINFO_IRINA( CIrinaGameData *GameData, uint32_t gameID, uint32_t upTime );
    bool AssignLength( BYTEARRAY &content );
    std::vector<CIrinaGame *> m_GPGames;
    std::vector<BYTEARRAY> m_RequestQueue;
    std::vector<unsigned char> m_CommandQueue;

private:
    bool m_Connecting;
    uint32_t m_ConnectingTime;
    bool m_Connected;
    bool m_FirstConnected;
    bool m_Exiting;
    bool m_WaitForInit;
    bool m_Inited;
    uint32_t m_ConnectedTime;
    bool m_LogonSended;
    uint32_t m_LastPingSendTime;
    uint32_t m_LastGamelistSendTime;
    uint32_t m_LastPingGetTime;
    bool m_Logon;
    uint32_t m_LogonSendTime;
    uint32_t m_TokenTime;

	// receive functions (client)

    std::vector<CIrinaGame *> RECEIVE_GET_GAMELIST( BYTEARRAY data );
    CIrinaUser *RECEIVE_LOGIN( BYTEARRAY data );
    CIrinaGameData *RECEIVE_GET_GAMEDATA( BYTEARRAY data );
    CIrinaGameDataEX *RECEIVE_GET_GAMEDATAEX( BYTEARRAY data );
    uint32_t RECEIVE_SET_GAMEDATA( BYTEARRAY data );
    uint32_t RECEIVE_SET_GAMEDATAEX( BYTEARRAY data );
};

//
// CIrinaGameDataEX
//

class CIrinaGameDataEX
{
public:
    CIrinaGameDataEX( std::string nMapName, BYTEARRAY nTGA, std::string nMapAuthor, std::string nMapDescription, std::string nMapPlayers, uint32_t nGameCounter );
    ~CIrinaGameDataEX( );
    std::string m_MapName;
    BYTEARRAY m_TGA;
    std::string m_MapAuthor;
    std::string m_MapDescription;
    std::string m_MapPlayers;
    uint32_t m_GameCounter;
};

//
// CIrinaGameData
//

class CIrinaGameData
{
private:
    std::string m_MapPath;

public:
    CIrinaGameData( uint32_t nGameCounter, uint16_t nGamePort, std::string nGameIP, std::string nGameName, BYTEARRAY nEncodeStatString, BYTEARRAY nGameKey );
    ~CIrinaGameData( );
    std::string GetMapPath( );
    uint32_t m_GameCounter;
    uint16_t m_GamePort;
    std::string m_GameIP;
    std::string m_GameName;
    BYTEARRAY m_EncodeStatString;
    BYTEARRAY m_GameKey;
};

//
// CIrinaUser
//

class CIrinaUser
{
public:
    CIrinaUser( std::string nID, std::string nDiscordName, std::string nDiscordAvatarURL, uint32_t nLocalID, std::string nDiscordID, uint32_t nVkID, std::string nRealm, std::string nBnetName, std::string nConnectorName, unsigned char nType );
    ~CIrinaUser( );

    std::string m_ID;
    std::string m_DiscordName;
    std::string m_DiscordAvatarURL;
    uint32_t m_LocalID;
    std::string m_DiscordID;
    uint32_t m_VkID;
    std::string m_Realm;
    std::string m_BnetName;
    std::string m_ConnectorName;
    unsigned char m_Type;
};

//
// CIrinaSlots
//

class CIrinaSlots
{
public:
    CIrinaSlots( unsigned char nSlotColor, std::string nPlayerName, std::string nPlayerRealm );
    ~CIrinaSlots( );
	
	unsigned char m_SlotColor;
    std::string m_PlayerName;
    std::string m_PlayerRealm;
    std::string m_PlayerColorName;
    std::string m_PlayerShadowName;
};

//
// CIrinaGame
//

class CIrinaGame
{
private:
    bool m_Broadcasted;
    bool m_Updated;
    bool m_GameStarted;
    bool m_DataReceived;
    bool m_DataReceivedEX;
    bool m_PowerUP;
    bool m_OtherBot;
    unsigned char m_Position;
    uint32_t m_DataReceivedRequestTime;
    uint32_t m_DataReceivedEXRequestTime;
    uint32_t m_UniqueGameID;
    uint32_t m_IrinaID;
    uint32_t m_GameTicks;
    std::string m_GameName;
    std::string m_MapName;
    std::string m_MapFileName;
    std::string m_IccupBotName;
    uint32_t m_SlotsCount;
    uint32_t m_PlayersCount;
    bool m_BadGameData;
    bool m_BadGameDataEX;

public:
    CIrinaGame( uint32_t nIrinaID, std::string nGameName, std::string nMapName, std::string nMapFileName, std::string nIccupBotName, bool nGameStarted, bool nPowerUP, bool nOtherBot, unsigned char nPosition, uint32_t nGameTicks, uint32_t nSlotsCount, uint32_t nPlayersCount, std::vector <CIrinaSlots> nSlots );
    ~CIrinaGame( );
    std::vector <CIrinaSlots> m_Slots;
    CIrinaGameData *m_GameData;
    CIrinaGameDataEX *m_GameDataEX;
    bool UpdateGame( CIrinaGame *game );
    unsigned int GetGameIcon( );
    bool GetPowerUP( )
    {
        return m_PowerUP;
    }
    bool GetOtherBot( )
    {
        return m_OtherBot;
    }
    bool GetAutoHost( )
    {
        return (m_Position != 0);
    }
    unsigned char GetPosition( )
    {
        return m_Position;
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
    std::string GetIccupBotName( )
    {
        return m_IccupBotName;
    }
    uint32_t GetSlotsCount( )
    {
        return m_SlotsCount;
    }
    uint32_t GetPlayersCount( )
    {
        return m_PlayersCount;
    }
    uint32_t GetUniqueGameID( )
    {
        return m_UniqueGameID;
    }
    uint32_t GetIrinaID( )
    {
        return m_IrinaID;
    }
    uint32_t GetGameTicks( )
    {
        return m_GameTicks;
    }
    bool GetGameStarted( )
    {
        return m_GameStarted;
    }
    bool GetBroadcasted( )
    {
        return m_Broadcasted;
    }
    bool GetUpdated( )
    {
        return m_Updated;
    }
    bool GetDataReceived( )
    {
        return m_DataReceived;
    }
    uint32_t GetDataReceivedRequestTime( )
    {
        return m_DataReceivedRequestTime;
    }
    uint32_t GetDataReceivedEXRequestTime( )
    {
        return m_DataReceivedEXRequestTime;
    }
    bool GetDataReceivedEX( )
    {
        return m_DataReceivedEX;
    }
    bool GetBadGameData( )
    {
        return m_BadGameData;
    }
    void SetBadGameData( bool nBadGameData )
    {
        m_BadGameData = nBadGameData;
    }
    bool GetBadGameDataEX( )
    {
        return m_BadGameDataEX;
    }
    void SetBadGameDataEX( bool nBadGameDataEX )
    {
        m_BadGameDataEX = nBadGameDataEX;
    }
    void SetDataReceivedEX( bool nDataReceivedEX )
    {
        m_DataReceivedEX = nDataReceivedEX;
    }
    void SetDataReceivedRequestTime( uint32_t nDataReceivedRequestTime )
    {
        m_DataReceivedRequestTime = nDataReceivedRequestTime;
    }
    void SetDataReceivedEXRequestTime( uint32_t nDataReceivedEXRequestTime )
    {
        m_DataReceivedEXRequestTime = nDataReceivedEXRequestTime;
    }
    void SetDataReceived( bool nDataReceived )
    {
        m_DataReceived = nDataReceived;
    }
    void SetUpdated( bool nUpdated )
    {
        m_Updated = nUpdated;
    }
    void SetBroadcasted( bool nBroadcasted )
    {
        m_Broadcasted = nBroadcasted;
    }
    void SetUniqueGameID( uint32_t nUniqueGameID )
    {
        m_UniqueGameID = nUniqueGameID;
    }

};

#endif
