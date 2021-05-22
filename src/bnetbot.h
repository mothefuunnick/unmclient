#ifndef BNETBOT_H
#define BNETBOT_H

//
// CBNETBot
//

class CBNETBot
{
private:
    CBNET *m_BNET;
	std::string m_Login;
    std::string m_LoginLower;
    std::string m_CommandTrigger;
	std::string m_Name;
	std::string m_Description;
	std::string m_Site;
	std::string m_VK;
	std::string m_AccessName;
	uint32_t m_Access;
	uint32_t m_MaxAccess;
    bool m_CanMap;
	bool m_CanPub;
	bool m_CanPrivate;
	bool m_CanInhouse;
	bool m_CanPubBy;
	bool m_CanDM;
    uint32_t m_AccessMap;
	uint32_t m_AccessPub;
	uint32_t m_AccessPrivate;
	uint32_t m_AccessInhouse;
	uint32_t m_AccessPubBy;
	uint32_t m_AccessDM;
    uint32_t m_Init;
    uint32_t m_InitLastTime;
    uint32_t m_Logon;
    uint32_t m_LogonLastTime;
    uint32_t m_WaitForResponse;
    uint32_t m_ResponseTime;
    uint32_t m_ResponseError;
    uint32_t m_ReceiveTime;
    uint32_t m_RespondingBotLoginTime;
    std::string m_RespondingBotLogin;
    std::string m_CurrentMap;
    std::string m_TempMapsString;
    std::vector<std::string> m_MapList;
    std::vector<std::string> m_CommandsQueue;
    std::vector<std::uint32_t> m_CommandsTypeQueue;

public:
    CBNETBot( CBNET *nBNET, std::string nLogin, std::string nName = std::string( ), std::string nCommandTrigger = "!", std::string nDescription = std::string( ), std::string nSite = std::string( ), std::string nVK = std::string( ), std::string nAccessName = std::string( ), uint32_t nAccess = 0, uint32_t nMaxAccess = 1, bool nCanMap = true, bool nCanPub = true, bool nCanPrivate = false, bool nCanInhouse = false, bool nCanPubBy = true, bool nCanDM = true, uint32_t nAccessMap = 0, uint32_t nAccessPub = 0, uint32_t nAccessPrivate = 0, uint32_t nAccessInhouse = 0, uint32_t nAccessPubBy = 1, uint32_t nAccessDM = 1 );
	~CBNETBot( );
    void Update( );
    void Logon( bool init, uint32_t access = 0 );
    void AddCommand( std::string Command, uint32_t type );
    void MapNotFound( );
    void AddMaps( std::string message, bool first );
    void SetCurrentMap( std::string message );
    void CreateGame( std::string message );
    void CreateGameBad( std::string message );
    void MapUpload( std::string message );

    std::vector<std::string> GetMapList( )
    {
        return m_MapList;
    }
	std::string GetLogin( )
	{
		return m_Login;
	}
    std::string GetLoginLower( )
    {
        return m_LoginLower;
    }
	std::string GetName( )
	{
		return m_Name;
	}
	std::string GetDescription( )
	{
		return m_Description;
	}
	std::string GetSite( )
	{
		return m_Site;
	}
	std::string GetVK( )
	{
		return m_VK;
	}
    bool GetInit( )
    {
        return m_Init == 2;
    }
    bool GetLogon( )
    {
        return m_Logon == 2;
    }
    bool GetWaitForResponse( )
    {
        return m_WaitForResponse > 0;
    }
    uint32_t GetResponseType( )
    {
        return m_WaitForResponse;
    }
    uint32_t GetResponseTime( )
    {
        return m_ResponseTime;
    }
    std::string GetCurrentMap( )
    {
        return m_CurrentMap;
    }
    std::string GetRespondingBotLogin( )
    {
        return m_RespondingBotLogin;
    }
    bool GetSupMap( )
    {
        return m_CanMap;
    }
    bool GetSupDM( )
    {
        return m_CanDM;
    }

    std::string GetStatusString( );
	std::string GetAccessUpString( );
    bool GetCanMap( );
	bool GetCanPub( );
	bool GetCanPrivate( );
	bool GetCanInhouse( );
    bool GetCanPubBy( );
    bool GetCanDM( );
};

#endif // BNETBOT_H
