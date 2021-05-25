#include "unm.h"
#include "util.h"
#include "bnet.h"
#include "bnetbot.h"

using namespace std;

extern ExampleObject *gToLog;

CBNETBot::CBNETBot( CBNET *nBNET, string nLogin, string nName, string nCommandTrigger, string nDescription, string nSite, string nVK, string nAccessName, uint32_t nAccess, uint32_t nMaxAccess, bool nCanMap, bool nCanPub, bool nCanPrivate, bool nCanInhouse, bool nCanPubBy, bool nCanDM, uint32_t nAccessMap, uint32_t nAccessPub, uint32_t nAccessPrivate, uint32_t nAccessInhouse, uint32_t nAccessPubBy, uint32_t nAccessDM )
{
    m_BNET = nBNET;
	m_Login = nLogin;
    m_LoginLower = m_Login;
    transform( m_LoginLower.begin( ), m_LoginLower.end( ), m_LoginLower.begin( ), static_cast<int(*)(int)>(tolower) );
    m_Name = nName;
	
	if( m_Name.empty( ) )
		m_Name = m_Login;

    m_CommandTrigger = nCommandTrigger;

    if( m_CommandTrigger.size( ) > 1 )
        m_CommandTrigger = m_CommandTrigger.substr( 0, 1 );

    if( m_CommandTrigger.empty( ) || m_CommandTrigger == " " )
        m_CommandTrigger = "!";

    m_Description = nDescription;
    m_Site = UTIL_UrlFix( nSite );
    m_VK = UTIL_UrlFix( nVK );

    if( m_Site.find( "." ) == string::npos || m_Site.find( ".." ) != string::npos || m_Site.size( ) <= 3 || m_Site.back( ) == '.' || m_Site.front( ) == '.' )
        m_Site = string( );
    else
    {
        m_Site = nSite;
        transform( m_Site.begin( ), m_Site.end( ), m_Site.begin( ), static_cast<int(*)(int)>(tolower) );
    }

    if( m_VK.find( "." ) == string::npos || m_VK.find( ".." ) != string::npos || m_VK.size( ) <= 3 || m_VK.back( ) == '.' || m_VK.front( ) == '.' )
        m_VK = string( );
    else
    {
        m_VK = nVK;
        transform( m_VK.begin( ), m_VK.end( ), m_VK.begin( ), static_cast<int(*)(int)>(tolower) );
    }

    m_AccessName = nAccessName;

    if( m_AccessName.empty( ) )
        m_AccessName = "Не определен";

    m_Access = nAccess;
	m_MaxAccess = nMaxAccess;
    m_CanMap = nCanMap;
	m_CanPub = nCanPub;
	m_CanPrivate = nCanPrivate;
	m_CanInhouse = nCanInhouse;
	m_CanPubBy = nCanPubBy;
	m_CanDM = nCanDM;
    m_AccessMap = nAccessMap;
	m_AccessPub = nAccessPub;
	m_AccessPrivate = nAccessPrivate;
	m_AccessInhouse = nAccessInhouse;
	m_AccessPubBy = nAccessPubBy;
	m_AccessDM = nAccessDM;
    m_Init = 0;
    m_InitLastTime = 0;
    m_Logon = 0;
    m_LogonLastTime = 0;
    m_WaitForResponse = 0;
    m_ResponseTime = 0;
    m_ResponseError = 0;
    m_ReceiveTime = 0;
    m_CurrentMap = string( );
    m_LastMapRequest = string( );
    m_RespondingBotLogin = string( );
    m_RespondingBotLoginTime = 0;
}

CBNETBot::~CBNETBot( )
{
	
}

void CBNETBot::Update( )
{
    if( !GetInit( ) )
    {
        if( m_InitLastTime == 0 || GetTime( ) - m_InitLastTime >= ( m_Init == 0 ? 10 : 300 ) )
        {
            if( m_Init == 0 )
                m_Init++;

            if( m_Logon == 0 )
                m_Logon++;
            else
                m_Logon--;

            m_InitLastTime = GetTime( );
            m_LogonLastTime = GetTime( );
            m_BNET->QueueChatCommand( "/w " + m_Login + " " + m_CommandTrigger + "cw etm.chat" );
        }
    }
    else if( !GetLogon( ) )
    {
        if( m_LogonLastTime == 0 || GetTime( ) - m_LogonLastTime >= ( m_Logon == 0 ? 10 : 300 ) )
        {
            if( m_Init == 0 )
                m_Init++;

            if( m_Logon == 0 )
                m_Logon++;
            else
                m_Logon--;

            m_InitLastTime = GetTime( );
            m_LogonLastTime = GetTime( );
            m_BNET->QueueChatCommand( "/w " + m_Login + " " + m_CommandTrigger + "cw etm.chat" );
        }
    }
    else
    {
        if( !m_RespondingBotLogin.empty( ) && GetTime( ) - m_RespondingBotLoginTime > 5 )
            m_RespondingBotLogin.clear( );

        if( GetResponseType( ) > 1 )
        {
            if( GetTime( ) - GetResponseTime( ) > 5 )
            {
                if( m_WaitForResponse == 1 )
                {
                    m_ResponseError = 0;
                    m_WaitForResponse = 0;
                }
                else
                {
                    m_ResponseError++;
                    m_WaitForResponse = 0;

                    if( m_ResponseError == 2 )
                    {
                        m_InitLastTime = 0;
                        m_Logon = 0;
                        m_LogonLastTime = 0;
                        m_ResponseTime = 0;
                        m_ResponseError = 0;
                        m_ReceiveTime = 0;
                        m_CurrentMap = string( );
                        emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 0, QString( ) );
                    }
                    else // ответ от бота не пришел, на первый раз предупреждаем, на второй (m_ResponseError == 2) - определяем что бот офлайн
                        emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 10, QString( ) );
                }
            }
        }
        else
        {
            if( GetResponseType( ) == 1 && m_ReceiveTime >= 4 )
                m_WaitForResponse = 0;

            for( uint32_t i = 0; i < m_CommandsTypeQueue.size( ); )
            {
                m_BNET->QueueChatCommand( m_CommandsQueue[i] );
                m_WaitForResponse = m_CommandsTypeQueue[i];
                m_ResponseTime = GetTime( );
                m_CommandsTypeQueue.erase( m_CommandsTypeQueue.begin( ) );
                m_CommandsQueue.erase( m_CommandsQueue.begin( ) );
                break;
            }
        }
    }
}

void CBNETBot::Logon( bool init, uint32_t access )
{
    if( init )
    {
        m_Init = 2;
        m_Access = access;

        if( m_Access == 0 )
            m_AccessName = "Юзер";
        else if( m_Access == 1 )
            m_AccessName = "Админ";
        else
            m_AccessName = "Рут-админ";
    }

    m_Logon = 2;
    emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 0, QString( ) );
}

void CBNETBot::AddCommand( string Command, uint32_t type )
{
    // type:
    // 1 = map from list
    // 2 = map
    // 3 = pub
    // 4 = dm

    if( type < 1 )
        type = 1;
    else if( type > 4 )
        type = 4;

    if( type == 3 )
        m_RespondingBotLogin.clear( );
    else if( type == 1 || type == 2 )
    {
        if( Command.size( ) > 4 )
            m_LastMapRequest = Command.substr( 4 );

        if( type == 1 )
        {
            m_CurrentMap = Command.substr( 4 );

            for( uint32_t i = 0; i < m_CommandsTypeQueue.size( ); )
            {
                if( m_CommandsTypeQueue[i] == 1 )
                {
                    m_CommandsTypeQueue.erase( m_CommandsTypeQueue.begin( ) + i );
                    m_CommandsQueue.erase( m_CommandsQueue.begin( ) + i );
                }
                else
                    i++;
            }
        }
    }

    m_CommandsQueue.push_back( "/w " + m_Login + " " + m_CommandTrigger + Command );
    m_CommandsTypeQueue.push_back( type );
}

void CBNETBot::MapNotFound( )
{
    m_ReceiveTime = GetTime( );
    m_ResponseError = 0;
    m_WaitForResponse = 0;
    emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 4, QString::fromUtf8( m_LastMapRequest.c_str( ) ) );
}

void CBNETBot::CreateGameBad( string message )
{
    m_RespondingBotLogin.clear( );
    emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 9, QString::fromUtf8( message.c_str( ) ) );
}

void CBNETBot::CreateGame( string message )
{
    m_ReceiveTime = GetTime( );
    m_ResponseError = 0;
    m_WaitForResponse = 0;
    string GameName = string( );
    string BotName = string( );

    if( message.substr( 0, 27 ) == "Создание игры [" || message.substr( 0, 46 ) == "Создание публичной игры [" || message.substr( 0, 46 ) == "Создание приватной игры [" )
    {
        string::size_type BotStartPos = message.find( "] на боте [" );

        if( BotStartPos != string::npos )
        {
            string::size_type OwnerStartPos = message.find( "]. Владелец" );

            if( OwnerStartPos != string::npos )
            {
                if( message.substr( 0, 27 ) == "Создание игры [" )
                    GameName = message.substr( 27, BotStartPos - 27 );
                else
                    GameName = message.substr( 46, BotStartPos - 46 );

                BotName = message.substr( BotStartPos + 17, OwnerStartPos - ( BotStartPos + 17 ) );
            }
            else
            {
                if( message.substr( 0, 27 ) == "Создание игры [" )
                    GameName = message.substr( 27, BotStartPos - 27 );
                else
                    GameName = message.substr( 46, BotStartPos - 46 );

                BotName = message.substr( BotStartPos + 17 );
                string::size_type BotEndPos = BotName.rfind( "]" );

                if( BotEndPos != string::npos )
                {
                    if( BotEndPos > ( m_BNET->m_UNM->IsIccupServer( m_BNET->GetLowerServer( ) ) ? 14 : 15 ) ) // max botname length = 15
                       BotEndPos = BotName.find( "]" );

                    if( BotEndPos > ( m_BNET->m_UNM->IsIccupServer( m_BNET->GetLowerServer( ) ) ? 14 : 15 ) )
                        BotName = string( );
                    else
                        BotName = BotName.substr( 0, BotEndPos );
                }
                else
                    BotName = string( );
            }
        }
        else
        {
            string::size_type OwnerStartPos = message.find( "]. Владелец" );

            if( OwnerStartPos != string::npos )
            {
                if( message.substr( 0, 27 ) == "Создание игры [" )
                    GameName = message.substr( 27, OwnerStartPos - 27 );
                else
                    GameName = message.substr( 46, OwnerStartPos - 46 );
            }
            else
            {
                if( message.substr( 0, 27 ) == "Создание игры [" )
                    GameName = message.substr( 27 );
                else
                    GameName = message.substr( 46 );

                string::size_type GameNameEndPos = GameName.rfind( "]" );

                if( GameNameEndPos != string::npos )
                {
                    if( GameNameEndPos > 31 ) // max gamename length = 31
                       GameNameEndPos = GameName.find( "]" );

                    if( GameNameEndPos > 31 )
                        GameName = string( );
                    else
                        GameName = message.substr( 0, GameNameEndPos );
                }
                else
                    GameName = string( );
            }
        }
    }
    else if( message.substr( 0, 12 + m_BNET->GetName( ).size( ) ) == m_BNET->GetName( ) + ", игра [" || message.substr( 0, 31 + m_BNET->GetName( ).size( ) ) == m_BNET->GetName( ) + ", публичная игра [" || message.substr( 0, 31 + m_BNET->GetName( ).size( ) ) == m_BNET->GetName( ) + ", приватная игра [" )
    {
        string::size_type GameNameEndPos = message.find( "] создана на боте [" );

        if( GameNameEndPos != string::npos )
        {
            if( message.substr( 0, 12 + m_BNET->GetName( ).size( ) ) == m_BNET->GetName( ) + ", игра [" )
                GameName = message.substr( 12 + m_BNET->GetName( ).size( ), GameNameEndPos - ( 12 + m_BNET->GetName( ).size( ) ) );
            else
                GameName = message.substr( 31 + m_BNET->GetName( ).size( ), GameNameEndPos - ( 31 + m_BNET->GetName( ).size( ) ) );

            BotName = message.substr( GameNameEndPos + 32 );
            string::size_type BotEndPos = BotName.rfind( "]" );

            if( BotEndPos != string::npos )
            {
                if( BotEndPos > ( m_BNET->m_UNM->IsIccupServer( m_BNET->GetLowerServer( ) ) ? 14 : 15 ) ) // max botname length = 15
                   BotEndPos = BotName.find( "]" );

                if( BotEndPos > ( m_BNET->m_UNM->IsIccupServer( m_BNET->GetLowerServer( ) ) ? 14 : 15 ) )
                    BotName = string( );
                else
                    BotName = BotName.substr( 0, BotEndPos );
            }
            else
                BotName = string( );
        }
        else
        {
            GameNameEndPos = message.find( "] создана" );

            if( GameNameEndPos != string::npos )
            {
                if( message.substr( 0, 12 + m_BNET->GetName( ).size( ) ) == m_BNET->GetName( ) + ", игра [" )
                    GameName = message.substr( 12 + m_BNET->GetName( ).size( ), GameNameEndPos - ( 12 + m_BNET->GetName( ).size( ) ) );
                else
                    GameName = message.substr( 31 + m_BNET->GetName( ).size( ), GameNameEndPos - ( 31 + m_BNET->GetName( ).size( ) ) );
            }
        }
    }
    else if( message.substr( 0, 10 ) == "Игра [" || message.substr( 0, 29 ) == "Публичная игра [" || message.substr( 0, 29 ) == "Приватная игра [" )
    {
        string::size_type GameNameEndPos = message.find( "] создана на боте [" );

        if( GameNameEndPos != string::npos )
        {
            if( message.substr( 0, 10 ) == m_BNET->GetName( ) + ", игра [" )
                GameName = message.substr( 10, GameNameEndPos - ( 10 ) );
            else
                GameName = message.substr( 29, GameNameEndPos - ( 29 ) );

            BotName = message.substr( GameNameEndPos + 32 );
            string::size_type BotEndPos = BotName.rfind( "]" );

            if( BotEndPos != string::npos )
            {
                if( BotEndPos > ( m_BNET->m_UNM->IsIccupServer( m_BNET->GetLowerServer( ) ) ? 14 : 15 ) ) // max botname length = 15
                   BotEndPos = BotName.find( "]" );

                if( BotEndPos > ( m_BNET->m_UNM->IsIccupServer( m_BNET->GetLowerServer( ) ) ? 14 : 15 ) )
                    BotName = string( );
                else
                    BotName = BotName.substr( 0, BotEndPos );
            }
            else
                BotName = string( );
        }
        else
        {
            GameNameEndPos = message.find( "] создана" );

            if( GameNameEndPos != string::npos )
            {
                if( message.substr( 0, 10 ) == m_BNET->GetName( ) + ", игра [" )
                    GameName = message.substr( 10, GameNameEndPos - ( 10 ) );
                else
                    GameName = message.substr( 29, GameNameEndPos - ( 29 ) );
            }
        }
    }

    if( !GameName.empty( ) )
    {
        if( GameName.size( ) > 31 )
            GameName = GameName.substr( 0, 31 );

        if( BotName.size( ) > ( m_BNET->m_UNM->IsIccupServer( m_BNET->GetLowerServer( ) ) ? 14 : 15 ) )
            BotName = string( );

        if( !BotName.empty( ) )
        {
            m_RespondingBotLogin = BotName;
            transform( m_RespondingBotLogin.begin( ), m_RespondingBotLogin.end( ), m_RespondingBotLogin.begin( ), static_cast<int(*)(int)>(tolower) );
            m_RespondingBotLoginTime = GetTime( );
        }

        emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 7, QString::fromUtf8( message.c_str( ) ) );
    }
    else
        emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 8, QString::fromUtf8( message.c_str( ) ) );
}

void CBNETBot::MapUpload( string message )
{
    m_ReceiveTime = GetTime( );
    m_ResponseError = 0;
    m_WaitForResponse = 0;
    bool success = true;

    if( message == "Пожалуйста, подождите несколько секунд..." )
        message = "Пожалуйста, подождите несколько секунд и попробуйте снова.";

    if( message.substr( 0, 40 ) == "Пожалуйста, подождите" )
        success = false;
    else if( message.substr( 0, 40 ) == "Укажите прямую ссылку" )
        success = false;
    else if( message.substr( 0, 44 ) == "Сейчас уже производится" )
        success = false;

    if( success )
        emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 5, QString::fromUtf8( message.c_str( ) ) );
    else
        emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 6, QString::fromUtf8( message.c_str( ) ) );
}

void CBNETBot::AddMaps( string message, bool first )
{
    m_ReceiveTime = GetTime( );

    if( first )
    {
        m_TempMapsString = string( );
        m_MapList.clear( );
        m_WaitForResponse = 1;
    }
    else
    {
        if( message.size( ) + m_BNET->GetName( ).size( ) < 195 )
        {
            m_ResponseError = 0;
            m_WaitForResponse = 0;
        }

        message = m_TempMapsString + message;
    }

    string mapname = string( );
    string::size_type nextMapPos = message.find( ", " );

    while( nextMapPos != string::npos )
    {
        mapname = message.substr( 0, nextMapPos );
        message = message.substr( nextMapPos + 2 );
        nextMapPos = message.find( ", " );

        if( mapname.size( ) > 4 && ( mapname.substr( mapname.size( ) - 4 ) == ".w3x" || mapname.substr( mapname.size( ) - 4 ) == ".w3m" ) )
        {
            for( uint32_t i = 0; i < m_MapList.size( ); i++ )
            {
                if( m_MapList[i] == mapname )
                {
                    m_MapList.erase( m_MapList.begin( ) + static_cast<int32_t>(i) );
                    break;
                }
            }

            m_MapList.push_back( mapname );
        }
    }

    if( message.size( ) > 4 && ( message.substr( message.size( ) - 4 ) == ".w3x" || message.substr( message.size( ) - 4 ) == ".w3m" ) )
    {
        for( uint32_t i = 0; i < m_MapList.size( ); i++ )
        {
            if( m_MapList[i] == message )
            {
                m_MapList.erase( m_MapList.begin( ) + static_cast<int32_t>(i) );
                break;
            }
        }

        m_MapList.push_back( message );
    }
    else
        m_TempMapsString = message;

    if( first )
        emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 2, QString( ) );
    else
        emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 3, QString( ) );
}

void CBNETBot::SetCurrentMap( string message )
{
    m_ReceiveTime = GetTime( );
    m_ResponseError = 0;
    m_WaitForResponse = 0;

    if( message.substr( 0, 2 ) == ": " )
        message = message.substr( 2 );
    else if( message.substr( 0, 1 ) == " " )
        message = message.substr( 1 );

    if( message.size( ) > 1 && message.back( ) == '.' )
        message.erase( message.end( ) - 1 );

    if( message.size( ) > 2 && message.front( ) == '[' && message.back( ) == ']' )
    {
        message.erase( message.begin( ) );
        message.erase( message.end( ) - 1 );
    }

    for( uint32_t i = 0; i < m_MapList.size( ); i++ )
    {
        if( m_MapList[i] == message )
        {
            m_MapList.erase( m_MapList.begin( ) + static_cast<int32_t>(i) );
            break;
        }
    }

    m_MapList.push_back( message );
    m_CurrentMap = message;
    emit gToLog->updateBnetBot( m_BNET->GetHostCounterID( ), QString::fromUtf8( m_Name.c_str( ) ), 1, QString::fromUtf8( m_CurrentMap.c_str( ) ) );
}

string CBNETBot::GetStatusString( )
{
    if( m_Logon == 2 )
        return "Логин бота: " + m_Name + " (онлайн). Ваш статус: " + m_AccessName;
    else
        return "Логин бота: " + m_Name + " (офлайн). Ваш статус: " + m_AccessName;
}

string CBNETBot::GetAccessUpString( )
{
	if( m_Login == "etm.chat" )
	{
		if( m_Access == 0 )
			return "Вы можете приобрести привилегии в ВК группе бота.";
		else
			return "Вы можете получить больше информации в ВК группе бота.";
	}
    else if( m_MaxAccess > m_Access || m_AccessMap > m_Access || m_AccessPub > m_Access || m_AccessPrivate > m_Access || m_AccessInhouse > m_Access || m_AccessPubBy > m_Access || m_AccessDM > m_Access )
	{
		if( m_Access == 0 )
		{
			if( !m_VK.empty( ) && !m_Site.empty( ) )
				return "Вы можете приобрести привилегии на сайте или в ВК группе бота.";
			else if( !m_VK.empty( ) )
				return "Вы можете приобрести привилегии в ВК группе бота.";
			else if( !m_Site.empty( ) )
				return "Вы можете приобрести привилегии на сайте бота.";
		}
		else
		{
			if( !m_VK.empty( ) && !m_Site.empty( ) )
				return "Вы можете повысить свои привилегии на сайте или в ВК группе бота.";
			else if( !m_VK.empty( ) )
				return "Вы можете повысить свои привилегии в ВК группе бота.";
			else if( !m_Site.empty( ) )
				return "Вы можете повысить свои привилегии на сайте бота.";
		}
	}
	else
	{
		if( !m_VK.empty( ) && !m_Site.empty( ) )
			return "Вы можете получить больше информации на сайте или в ВК группе бота.";
		else if( !m_VK.empty( ) )
			return "Вы можете получить больше информации в ВК группе бота.";
		else if( !m_Site.empty( ) )
			return "Вы можете получить больше информации на сайте бота.";
	}

    return string( );
}

bool CBNETBot::GetCanMap( )
{
    if( m_CanMap && m_Access >= m_AccessMap )
        return true;
    else
        return false;
}

bool CBNETBot::GetCanPub( )
{
	if( m_CanPub && m_Access >= m_AccessPub )
		return true;
	else
		return false;
}

bool CBNETBot::GetCanPrivate( )
{
	if( m_CanPrivate && m_Access >= m_AccessPrivate )
		return true;
	else
		return false;
}

bool CBNETBot::GetCanInhouse( )
{
	if( m_CanInhouse && m_Access >= m_AccessInhouse )
		return true;
	else
		return false;
}

bool CBNETBot::GetCanPubBy( )
{
	if( m_CanPubBy && m_Access >= m_AccessPubBy )
		return true;
	else
		return false;
}

bool CBNETBot::GetCanDM( )
{
	if( m_CanDM && m_Access >= m_AccessDM )
		return true;
	else
		return false;
}
