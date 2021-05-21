#include "unm.h"
#include "util.h"
#include "irinaws.h"
#include "socket.h"
#include "gameprotocol.h"
#include "base64.h"

using namespace std;

extern ExampleObject *gToLog;
extern bool AuthorizationPassed;
extern QString gOAuthDiscordToken;
extern QString gOAuthDiscordName;

CIrinaWS::CIrinaWS( CUNM *nUNM )
{
    m_UNM = nUNM;
    m_User = nullptr;
    m_Connecting = false;
    m_ConnectingTime = 0;
    m_ConnectedTime = 0;
    m_WaitForInit = false;
    m_LogonSended = false;
    m_LastPingSendTime = 0;
    m_LastGamelistSendTime = 0;
    m_Inited = false;
    m_Connected = false;
    m_FirstConnected = true;
    m_Exiting = false;
    m_LastPingGetTime = 0;
    m_Logon = false;
    m_LogonSendTime = 0;
    m_TokenTime = 0;
}

CIrinaWS::~CIrinaWS( )
{
    if( m_User != nullptr )
        delete m_User;

    for( vector<CIrinaGame *>::iterator i = m_GPGames.begin( ); i != m_GPGames.end( ); i++ )
    {
        if( ( *i )->GetBroadcasted( ) )
            m_UNM->m_UDPSocket->Broadcast( 6112, m_UNM->m_GameProtocol->SEND_W3GS_DECREATEGAME( ( *i )->GetUniqueGameID( ) ), "[GPROXY: " + ( *i )->GetGameName( ) + "]" );
    }

    for( vector<CIrinaGame *>::iterator i = m_GPGames.begin( ); i != m_GPGames.end( ); i++ )
        delete *i;
}

bool CIrinaWS::Update( )
{
    //
    // update comand from GUI
    //

    while( !m_CommandQueue.empty( ) )
    {
        // 0 = send request for token (set m_TokenTime)
        // 1 = get token (reset m_TokenTime)
        // 2 = error geting (reset m_TokenTime)

        if( m_CommandQueue[0] == 0 )
            m_TokenTime = GetTime( );
        else if( m_CommandQueue[0] == 1 )
            m_TokenTime = 0;
        else if( m_CommandQueue[0] == 2 )
            m_TokenTime = 0;

        m_CommandQueue.erase( m_CommandQueue.begin( ) );
    }

    if( m_TokenTime > 0 && GetTime( ) - m_TokenTime > 5 )
    {
        emit gToLog->authButtonReset( );
        m_TokenTime = 0;
    }

    if( !m_Connecting && !m_Connected )
    {
        if( AuthorizationPassed )
        {
            m_WaitForInit = false;
            m_Inited = false;
            m_Connecting = true;
            m_ConnectingTime = GetTime( );
            m_RequestQueue.clear( );
            emit gToLog->openConnect( );
        }
    }
    else if( m_Connecting && !m_Connected )
    {
        if( m_ConnectingTime < GetTime( ) && GetTime( ) - m_ConnectingTime > 10 )
        {
            CONSOLE_Print( "[IRINA] Disconected from irina server (connect timeout)" );
            m_Connecting = false;
            m_ConnectingTime = 0;
            m_ConnectedTime = 0;
            m_WaitForInit = false;
            m_LogonSended = false;
            m_Logon = false;
            m_LogonSendTime = 0;
            m_LastPingSendTime = 0;
            m_LastGamelistSendTime = 0;
            m_Inited = false;
            m_Connected = false;
            m_LastPingGetTime = 0;
            m_RequestQueue.clear( );
            emit gToLog->closeConnect( );
        }
        else
        {
            for( uint32_t i = 0; i < m_RequestQueue.size( ); i++ )
            {
                if( m_RequestQueue[i].size( ) >= 2 && m_RequestQueue[i][0] == IRINALocal_HEADER_CONSTANT )
                {
                    if( m_RequestQueue[i][1] == CONNECTED )
                    {
                        CONSOLE_Print( "[IRINA] Connected to irina server" );
                        m_RequestQueue.erase( m_RequestQueue.begin( ) + i );
                        m_Connecting = false;
                        m_Connected = true;
                        m_WaitForInit = true;
                        m_Inited = false;
                        m_ConnectedTime = GetTime( );
                        m_FirstConnected = false;
                        break;
                    }
                    else if( m_RequestQueue[i][1] == DISCONECTED )
                    {
                        CONSOLE_Print( "[IRINA] Disconected from irina server (disconect on initing by server)" );
                        m_Connecting = false;
                        m_ConnectingTime = 0;
                        m_ConnectedTime = 0;
                        m_WaitForInit = false;
                        m_LogonSended = false;
                        m_Logon = false;
                        m_LogonSendTime = 0;
                        m_LastPingSendTime = 0;
                        m_LastGamelistSendTime = 0;
                        m_Inited = false;
                        m_Connected = false;
                        m_LastPingGetTime = 0;
                        m_RequestQueue.clear( );
                        emit gToLog->closeConnect( );
                        break;
                    }
                }
            }
        }
    }
    else if( m_Connected )
    {
        if( m_WaitForInit )
        {
            if( m_ConnectedTime < GetTime( ) && GetTime( ) - m_ConnectedTime > 10 )
            {
                CONSOLE_Print( "[IRINA] Disconected from irina server (init timeout)" );
                m_Connecting = false;
                m_ConnectingTime = 0;
                m_ConnectedTime = 0;
                m_WaitForInit = false;
                m_LogonSended = false;
                m_Logon = false;
                m_LogonSendTime = 0;
                m_LastPingSendTime = 0;
                m_LastGamelistSendTime = 0;
                m_Inited = false;
                m_Connected = false;
                m_LastPingGetTime = 0;
                m_RequestQueue.clear( );
                emit gToLog->closeConnect( );
            }
        }
        else
        {
            if( GetTime( ) - m_LastPingGetTime > 17 && GetTime( ) > m_LastPingGetTime )
            {
                CONSOLE_Print( "[IRINA] Disconected from irina server (ping timeout)" );
                m_Connecting = false;
                m_ConnectingTime = 0;
                m_ConnectedTime = 0;
                m_WaitForInit = false;
                m_LogonSended = false;
                m_Logon = false;
                m_LogonSendTime = 0;
                m_LastPingSendTime = 0;
                m_LastGamelistSendTime = 0;
                m_Inited = false;
                m_Connected = false;
                m_LastPingGetTime = 0;
                m_RequestQueue.clear( );
                emit gToLog->closeConnect( );
            }

            if( !m_Logon && !gOAuthDiscordToken.isEmpty( ) )
            {
                if( !m_LogonSended )
                {
                    if( ( GetTime( ) > m_LogonSendTime && GetTime( ) - m_LogonSendTime >= 60 ) || m_LogonSendTime == 0 )
                    {
                        m_LogonSended = true;
                        m_LogonSendTime = GetTime( );
                        emit gToLog->sendLogin( );
                    }
                }
                else if( GetTime( ) > m_LogonSendTime && GetTime( ) - m_LogonSendTime > 5 )
                {
                    m_LogonSended = false;
                    gOAuthDiscordName = QString( );
                    emit gToLog->badLogon( );
                }
            }

            if( GetTime( ) > m_LastPingSendTime && ( GetTime( ) - m_LastPingSendTime >= 5 || m_LastPingSendTime == 0 ) )
            {
                m_LastPingSendTime = GetTime( );
                emit gToLog->sendPing( );
            }

            if( GetTime( ) > m_LastGamelistSendTime && ( GetTime( ) - m_LastGamelistSendTime >= 15 || m_LastGamelistSendTime == 0 ) )
            {
                m_LastGamelistSendTime = GetTime( );
                emit gToLog->sendGamelist( );
            }
        }

        //
        // update request from GUI
        //

        while( !m_RequestQueue.empty( ) )
        {
            if( m_RequestQueue[0].size( ) >= 2 )
            {
                if( m_RequestQueue[0][0] == IRINAWS_HEADER_CONSTANT )
                {
                    if( m_RequestQueue[0][1] == GET_GAMELIST )
                    {
                        // мы получили пакет со списком всех игр, формируем его в вектор CIrinaGame'ов

                        vector<CIrinaGame *> Games = RECEIVE_GET_GAMELIST( m_RequestQueue[0] );
                        uint32_t GamesReceived = Games.size( );
                        uint32_t OldReliableGamesReceived = 0;
                        uint32_t NewReliableGamesReceived = 0;
                        uint32_t DeletedGames = 0;

                        // нам нужно добавить каждую полученную игру в текущий список, при этом нужно определить это новая игра или она уже есть в списке:
                        // если есть - обновляем данные, если нет - добавляем игру в список
                        // это все, а также отправка сигнала на добалвение/изменение игр в GUI производится в функции AddGame
                        // стоит понимать, что мы сейчас получаем полный список игр на севере, если в нашем списке игр есть игра, обновление для которой не пришло в этом пакете - значит игра была удалена на севере (отменена/завершена) и соотвественно эту игру нужно удалить со списка
                        // поэтому сейчас мы помечаем каждую игру в нашем списке как необновленную, анализирумем полученный пакет со списком игр, обновляем/добавляем игры (новые игры добавляются со статусом "обновленная") и после завершения этой процедуры, мы удаляем все необновленные игры

                        for( vector<CIrinaGame *>::iterator i = m_GPGames.begin( ); i != m_GPGames.end( ); i++ )
                            ( *i )->SetUpdated( false );

                        for( vector<CIrinaGame *>::iterator i = Games.begin( ); i != Games.end( ); )
                        {
                            if( AddGame( *i ) )
                                NewReliableGamesReceived++;
                            else
                                OldReliableGamesReceived++;

                            i++;
                        }

                        for( vector<CIrinaGame *>::iterator i = m_GPGames.begin( ); i != m_GPGames.end( ); )
                        {
                            if( !( *i )->GetUpdated( ) )
                            {
                                DeletedGames++;
                                emit gToLog->deleteGameFromListIrina( ( *i )->GetUniqueGameID( ), ( ( *i )->GetBroadcasted( ) ? 0 : 1 ) );
                                delete *i;
                                i = m_GPGames.erase( i );
                            }
                            else
                                i++;
                        }

                        emit gToLog->gameListTablePosFixIrina( );

                        if( !m_UNM->m_LogReduction && GamesReceived > 0 )
                            CONSOLE_Print( "[IRINA] sifted game list, found " + UTIL_ToString( OldReliableGamesReceived + NewReliableGamesReceived ) + "/" + UTIL_ToString( GamesReceived ) + " reliable games (" + UTIL_ToString( OldReliableGamesReceived ) + " duplicates). " + UTIL_ToString( DeletedGames ) + " games was deleted." );
                    }
                    else if( m_RequestQueue[0][1] == INIT )
                    {
                        if( m_WaitForInit )
                        {
                            m_WaitForInit = false;
                            m_LogonSended = false;
                            m_Logon = false;
                            m_LogonSendTime = 0;
                            m_LastPingSendTime = 0;
                            m_LastGamelistSendTime = 0;
                            m_LastPingGetTime = GetTime( );
                            emit gToLog->successInit( );
                        }
                    }
                    else if( m_RequestQueue[0][1] == GET_GAMEDATA )
                    {
                        CIrinaGameData *IrinaGameData = RECEIVE_GET_GAMEDATA( m_RequestQueue[0] );

                        if( IrinaGameData == nullptr )
                        {
                            // игры которые передаются в LAN обрабатываются в функции update
                            // и в случае, если пакет GET_GAMEDATA для игры не пришел (или не был разобран) игра перестанет хоститься в LAN и будет удалена из списка 'мои игы' (также возможно следует пометить такую игру как недействительную...)
                            // поэтому нет смысла удалять игру из списка сейчас, потому что в некоторых случаях (например, когда мы ожидаем ответа для двух игр сразу) мы не можем определить наверняка к какой именно игре относится этот пакет
                        }
                        else
                        {
                            for( vector<CIrinaGame *>::iterator i = m_GPGames.begin( ); i != m_GPGames.end( ); i++ )
                            {
                                if( IrinaGameData->m_GameCounter == ( *i )->GetIrinaID( ) )
                                {
                                    if( ( *i )->m_GameData != nullptr )
                                    {
                                        delete ( *i )->m_GameData;
                                        ( *i )->m_GameData = nullptr;
                                    }

                                    ( *i )->SetDataReceived( true );
                                    ( *i )->SetBadGameData( false );
                                    ( *i )->m_GameData = IrinaGameData;

                                    // тут мы отправляем сигнал обновления игры в gui!

                                    string MapName = ( *i )->m_GameData->GetMapPath( );

                                    if( ( *i )->GetDataReceivedEX( ) && !( *i )->m_GameDataEX->m_MapName.empty( ) )
                                        MapName += " (" + ( *i )->m_GameDataEX->m_MapName + ")";

                                    emit gToLog->updateGameMapInfo( ( *i )->GetUniqueGameID( ), QString::fromUtf8( MapName.c_str( ) ) );
                                    break;
                                }
                            }
                        }
                    }
                    else if( m_RequestQueue[0][1] == GET_GAMEDATAEX )
                    {
                        // тут мы отправляем сигнал обновления инфы о игре в списке игр в gui!

                        CIrinaGameDataEX *IrinaGameDataEX = RECEIVE_GET_GAMEDATAEX( m_RequestQueue[0] );

                        if( IrinaGameDataEX == nullptr )
                        {
                            // а тут отправляем сигнал в gui, мол ошибка при чтении пакета...

                            emit gToLog->setSelectGameDataBadTga( 0, QString( "unknown" ), QString( "N/A" ), QString( "N/A" ), QString( "N/A" ) );
                        }
                        else
                        {
                            bool found = false;

                            for( vector<CIrinaGame *>::iterator i = m_GPGames.begin( ); i != m_GPGames.end( ); i++ )
                            {
                                if( IrinaGameDataEX->m_GameCounter == ( *i )->GetIrinaID( ) )
                                {
                                    if( ( *i )->m_GameDataEX != nullptr )
                                    {
                                        delete ( *i )->m_GameDataEX;
                                        ( *i )->m_GameDataEX = nullptr;
                                    }

                                    ( *i )->SetDataReceivedEX( true );
                                    ( *i )->SetBadGameDataEX( false );
                                    ( *i )->m_GameDataEX = IrinaGameDataEX;
                                    found = true;

                                    if( ( *i )->m_GameDataEX->m_TGA.size( ) > 20 ) // :p
                                    {
                                        bool success = false;
                                        QImage image = UTIL_LoadTga( ( *i )->m_GameDataEX->m_TGA, success );

                                        if( success )
                                            emit gToLog->setSelectGameData( ( *i )->GetUniqueGameID( ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapName.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapPlayers.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapAuthor.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapDescription.c_str( ) ), QPixmap::fromImage( image ) );
                                        else
                                            emit gToLog->setSelectGameDataBadTga( ( *i )->GetUniqueGameID( ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapName.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapPlayers.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapAuthor.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapDescription.c_str( ) ) );
                                    }
                                    else
                                        emit gToLog->setSelectGameDataBadTga( ( *i )->GetUniqueGameID( ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapName.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapPlayers.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapAuthor.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapDescription.c_str( ) ) );

                                    break;
                                }
                            }

                            if( !found )
                            {
                                // а тут отправляем сигнал в gui, мол ошибка при чтении пакета...

                                emit gToLog->setSelectGameDataBadTga( 0, QString( "unknown" ), QString( "N/A" ), QString( "N/A" ), QString( "N/A" ) );
                            }
                        }
                    }
                    else if( m_RequestQueue[0][1] == PING )
                        m_LastPingGetTime = GetTime( );
                    else
                    {
                        string unknown;

                        for( uint32_t j = 0; j < m_RequestQueue[0].size( ); j++ )
                        {
                            if( j == 0 )
                                unknown += UTIL_ToString( m_RequestQueue[0][j] );
                            else
                                unknown += " " + UTIL_ToString( m_RequestQueue[0][j] );
                        }

                        CONSOLE_Print( "[IRINA] Получен неизвестный пакет (он не будет обработан): [" + unknown + "]" );
                        CONSOLE_Print( "[IRINA] Hex to string: [" + string( m_RequestQueue[0].begin( ), m_RequestQueue[0].end( ) ) + "]" );
                    }
                }
                else if( m_RequestQueue[0][0] == IRINALocal_HEADER_CONSTANT )
                {
                    if( m_RequestQueue[0][1] == SET_GAMEDATA )
                    {
                        uint32_t GameID = RECEIVE_SET_GAMEDATA( m_RequestQueue[0] );
                        bool gamefound = false;

                        for( vector<CIrinaGame *>::iterator j = m_GPGames.begin( ); j != m_GPGames.end( ); j++ )
                        {
                            if( ( *j )->GetUniqueGameID( ) == GameID )
                            {
                                if( ( *j )->GetDataReceived( ) )
                                {
                                    // тут мы отправляем сигнал обновления игры в gui, если инфа о игре уже получена для этой игры!

                                    string MapName = ( *j )->m_GameData->GetMapPath( );

                                    if( ( *j )->GetDataReceivedEX( ) && !( *j )->m_GameDataEX->m_MapName.empty( ) )
                                        MapName += " (" + ( *j )->m_GameDataEX->m_MapName + ")";

                                    emit gToLog->updateGameMapInfo( GameID, QString::fromUtf8( MapName.c_str( ) ) );
                                }

                                bool gamedata = false;
                                bool gamedataex = false;

                                if( ( *j )->GetDataReceivedRequestTime( ) == 0 || ( !( *j )->GetDataReceived( ) && GetTime( ) - ( *j )->GetDataReceivedRequestTime( ) >= 2 && GetTime( ) > ( *j )->GetDataReceivedRequestTime( ) ) )
                                    gamedata = true;

                                if( ( *j )->GetDataReceivedEXRequestTime( ) == 0 || ( !( *j )->GetDataReceivedEX( ) && GetTime( ) - ( *j )->GetDataReceivedEXRequestTime( ) >= 2 && GetTime( ) > ( *j )->GetDataReceivedEXRequestTime( ) ) )
                                    gamedataex = true;

                                if( gamedata && gamedataex )
                                {
                                    ( *j )->SetDataReceivedRequestTime( GetTime( ) );
                                    ( *j )->SetDataReceivedEXRequestTime( GetTime( ) );
                                    emit gToLog->sendGetgame( ( *j )->GetIrinaID( ), 2 );
                                }
                                else if( gamedata )
                                {
                                    ( *j )->SetDataReceivedRequestTime( GetTime( ) );
                                    emit gToLog->sendGetgame( ( *j )->GetIrinaID( ), 1 );
                                }
                                else if( gamedataex )
                                {
                                    ( *j )->SetDataReceivedEXRequestTime( GetTime( ) );
                                    emit gToLog->sendGetgame( ( *j )->GetIrinaID( ), 0 );
                                }

                                gamefound = true;
                                ( *j )->SetBroadcasted( true );
                                break;
                            }
                        }

                        if( !gamefound )
                        {
                            // удаляем игру из списка игр (хотя она будет удалена при следующем обновлении списка игр)
                            // также удаляем игру из списка текущих игр (вкладка "мои игры")

                            emit gToLog->deleteGameFromListIrina( GameID, 0 );
                        }
                    }
                    else if( m_RequestQueue[0][1] == SET_GAMEDATAEX )
                    {
                        uint32_t GameID = RECEIVE_SET_GAMEDATAEX( m_RequestQueue[0] );
                        bool gamefound = false;

                        for( vector<CIrinaGame *>::iterator i = m_GPGames.begin( ); i != m_GPGames.end( ); i++ )
                        {
                            if( ( *i )->GetUniqueGameID( ) == GameID )
                            {
                                if( ( *i )->GetDataReceivedEX( ) )
                                {
                                    // тут мы отправляем сигнал обновления инфы о игре в списке игр в gui, если инфа о игре уже получена для этой игры!

                                    if( ( *i )->m_GameDataEX->m_TGA.size( ) > 20 ) // :p
                                    {
                                        bool success = false;
                                        QImage image = UTIL_LoadTga( ( *i )->m_GameDataEX->m_TGA, success );

                                        if( success )
                                            emit gToLog->setSelectGameData( ( *i )->GetUniqueGameID( ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapName.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapPlayers.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapAuthor.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapDescription.c_str( ) ), QPixmap::fromImage( image ) );
                                        else
                                            emit gToLog->setSelectGameDataBadTga( ( *i )->GetUniqueGameID( ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapName.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapPlayers.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapAuthor.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapDescription.c_str( ) ) );

                                    }
                                    else
                                        emit gToLog->setSelectGameDataBadTga( ( *i )->GetUniqueGameID( ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapName.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapPlayers.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapAuthor.c_str( ) ), QString::fromUtf8( ( *i )->m_GameDataEX->m_MapDescription.c_str( ) ) );
                                }
                                else if( ( *i )->GetDataReceivedEXRequestTime( ) == 0 || ( !( *i )->GetDataReceivedEX( ) && GetTime( ) - ( *i )->GetDataReceivedEXRequestTime( ) >= 2 && GetTime( ) > ( *i )->GetDataReceivedEXRequestTime( ) ) )
                                {
                                    // или посылаем запрос на сервер, чтобы получить нужную информцию об игре, если данной инфы у нас нет

                                    ( *i )->SetDataReceivedEXRequestTime( GetTime( ) );
                                    emit gToLog->sendGetgame( ( *i )->GetIrinaID( ), false );
                                }

                                gamefound = true;
                                break;
                            }
                        }

                        if( !gamefound )
                        {
                            // удаляем игру из списка игр (хотя она будет удалена при следующем обновлении списка игр)

                            emit gToLog->deleteGameFromListIrina( GameID, 1 );
                        }
                    }
                    else if( m_RequestQueue[0][1] == CONNECTED )
                        CONSOLE_Print( "[IRINA] получен неожиданный connected сигнал" );
                    else if( m_RequestQueue[0][1] == DISCONECTED )
                    {
                        CONSOLE_Print( "[IRINA] Disconected from irina server (disconect by server)" );
                        m_Connecting = false;
                        m_ConnectingTime = 0;
                        m_ConnectedTime = 0;
                        m_WaitForInit = false;
                        m_LogonSended = false;
                        m_Logon = false;
                        m_LogonSendTime = 0;
                        m_LastPingSendTime = 0;
                        m_LastGamelistSendTime = 0;
                        m_Inited = false;
                        m_Connected = false;
                        m_LastPingGetTime = 0;
                        m_RequestQueue.clear( );
                        emit gToLog->closeConnect( );
                        break;
                    }
                    else
                    {
                        string unknown;

                        for( uint32_t j = 0; j < m_RequestQueue[0].size( ); j++ )
                        {
                            if( j == 0 )
                                unknown += UTIL_ToString( m_RequestQueue[0][j] );
                            else
                                unknown += " " + UTIL_ToString( m_RequestQueue[0][j] );
                        }

                        CONSOLE_Print( "[IRINA] Получен неизвестный пакет (он не будет обработан): [" + unknown + "]" );
                        CONSOLE_Print( "[IRINA] Hex to string: [" + string( m_RequestQueue[0].begin( ), m_RequestQueue[0].end( ) ) + "]" );
                    }
                }
                else if( m_RequestQueue[0][0] == IRINAGlobal_HEADER_CONSTANT )
                {
                    if( m_RequestQueue[0][1] == LOGIN )
                    {
                        if( m_LogonSended )
                        {
                            CIrinaUser *User = RECEIVE_LOGIN( m_RequestQueue[0] );

                            if( User == nullptr )
                            {
                                string unknown;

                                for( uint32_t j = 0; j < m_RequestQueue[0].size( ); j++ )
                                {
                                    if( j == 0 )
                                        unknown += UTIL_ToString( m_RequestQueue[0][j] );
                                    else
                                        unknown += " " + UTIL_ToString( m_RequestQueue[0][j] );
                                }

                                CONSOLE_Print( "[IRINA] Не удалось разобрать пакет логина (он не будет обработан): [" + unknown + "]" );
                                CONSOLE_Print( "[IRINA] Hex to string: [" + string( m_RequestQueue[0].begin( ), m_RequestQueue[0].end( ) ) + "]" );
                            }
                            else
                            {
                                if( m_User != nullptr )
                                    delete m_User;

                                m_User = User;

                                if( !m_User->m_ConnectorName.empty( ) )
                                    CONSOLE_Print( "[IRINA] Logined on irina server as [" + m_User->m_ConnectorName + "]" );
                                else if( !m_User->m_DiscordName.empty( ) )
                                    CONSOLE_Print( "[IRINA] Logined on irina server as [" + m_User->m_DiscordName + "]" );
                                else
                                    CONSOLE_Print( "[IRINA] Logined on irina server" );

                                m_Logon = true;
                                m_LogonSended = false;
                                gOAuthDiscordName = QString::fromUtf8( m_User->m_DiscordName.c_str( ) );
                                emit gToLog->successLogon( );
                            }
                        }
                        else
                            CONSOLE_Print( "[IRINA] получен неожиданный logon сигнал." );
                    }
                    else
                    {
                        string unknown;

                        for( uint32_t j = 0; j < m_RequestQueue[0].size( ); j++ )
                        {
                            if( j == 0 )
                                unknown += UTIL_ToString( m_RequestQueue[0][j] );
                            else
                                unknown += " " + UTIL_ToString( m_RequestQueue[0][j] );
                        }

                        CONSOLE_Print( "[IRINA] Получен неизвестный пакет (он не будет обработан): [" + unknown + "]" );
                        CONSOLE_Print( "[IRINA] Hex to string: [" + string( m_RequestQueue[0].begin( ), m_RequestQueue[0].end( ) ) + "]" );
                    }
                }
                else
                {
                    string unknown;

                    for( uint32_t j = 0; j < m_RequestQueue[0].size( ); j++ )
                    {
                        if( j == 0 )
                            unknown += UTIL_ToString( m_RequestQueue[0][j] );
                        else
                            unknown += " " + UTIL_ToString( m_RequestQueue[0][j] );
                    }

                    CONSOLE_Print( "[IRINA] Получен неизвестный пакет (он не будет обработан): [" + unknown + "]" );
                    CONSOLE_Print( "[IRINA] Hex to string: [" + string( m_RequestQueue[0].begin( ), m_RequestQueue[0].end( ) ) + "]" );
                }
            }

            m_RequestQueue.erase( m_RequestQueue.begin( ) );
        }
    }

    return m_Exiting;
}

bool CIrinaWS::AddGame( CIrinaGame *game )
{
    bool DuplicateFound = false;

    // check for duplicates and rehosted games

    for( vector<CIrinaGame *>::iterator i = m_GPGames.begin( ); i != m_GPGames.end( ); i++ )
    {
        if( game->GetIrinaID( ) == ( *i )->GetIrinaID( ) )
        {
            // duplicate or rehosted game, delete the old one and add the new one
            // don't forget to remove the old one from the LAN list first

            bool deleteFromCurrentGames = false;

            if( ( *i )->GetBroadcasted( ) && !( *i )->GetGameStarted( ) && game->GetGameStarted( ) )
            {
                // удаление игры из списка "мои игры" происходит ниже в updateGameToListIrina
                // ( *i )->UpdateGame( game ) будет true если ( *i )->GetGameStarted( ) != game->GetGameStarted( )

                ( *i )->SetBroadcasted( false );
                deleteFromCurrentGames = true;
                m_UNM->m_UDPSocket->Broadcast( 6112, m_UNM->m_GameProtocol->SEND_W3GS_DECREATEGAME( ( *i )->GetUniqueGameID( ) ), "[GPROXY: " + ( *i )->GetGameName( ) + "]" );
            }
            else if( ( *i )->GetBroadcasted( ) && !UTIL_CompareGameNames( game->GetGameName( ), ( *i )->GetGameName( ) ) )
                m_UNM->m_UDPSocket->Broadcast( 6112, m_UNM->m_GameProtocol->SEND_W3GS_DECREATEGAME( ( *i )->GetUniqueGameID( ) ), "[GPROXY: " + ( *i )->GetGameName( ) + "]" );

            if( ( *i )->UpdateGame( game ) )
            {
                QStringList GameInfoInit;
                GameInfoInit.append( QString::fromUtf8( UTIL_SubStrFix( game->GetGameName( ), 0, 0 ).c_str( ) ) );
                GameInfoInit.append( QString::number( game->GetPlayersCount( ) ) + "/" + QString::number( game->GetSlotsCount( ) ) );
                QString players;
                QString playersWithRealms;
                QString playersShadow;

                for( vector<CIrinaSlots>::iterator j = game->m_Slots.begin( ); j != game->m_Slots.end( ); j++ )
                {
                    if( !j->m_PlayerName.empty( ) )
                    {
                        players.append( j->m_PlayerColorName.c_str( ) + QString( " &nbsp;" ) );
                        playersWithRealms.append( j->m_PlayerName.c_str( ) + QString( "(" ) + j->m_PlayerRealm.c_str( ) + "), " );
                        playersShadow.append( j->m_PlayerShadowName.c_str( ) + QString( " &nbsp;" ) );
                    }
                }

                if( !players.isEmpty( ) )
                {
                    players.chop( 7 );
                    playersWithRealms.chop( 2 );
                    playersShadow.chop( 7 );
                }
                else
                {
                    players = "<font color = #000000>нет игроков</font>";
                    playersWithRealms = "в этой игре отсутствуют игроки";
                    playersShadow = "<font color = #50808080>нет игроков</font>";
                }

                GameInfoInit.append( players );
                GameInfoInit.append( playersWithRealms );
                GameInfoInit.append( playersShadow );
                emit gToLog->updateGameToListIrina( ( *i )->GetUniqueGameID( ), ( *i )->GetGameStarted( ), GameInfoInit, deleteFromCurrentGames );
            }

            delete game;
            DuplicateFound = true;
            break;
        }
    }

    if( !DuplicateFound )
    {
        QStringList GameInfoInit;
        GameInfoInit.append( QString::fromUtf8( UTIL_SubStrFix( game->GetGameName( ), 0, 0 ).c_str( ) ) );
        GameInfoInit.append( QString::number( game->GetPlayersCount( ) ) + "/" + QString::number( game->GetSlotsCount( ) ) );
        QString players;
        QString playersWithRealms;
        QString playersShadow;

        for( vector<CIrinaSlots>::iterator j = game->m_Slots.begin( ); j != game->m_Slots.end( ); j++ )
        {
            if( !j->m_PlayerName.empty( ) )
            {
                players.append( j->m_PlayerColorName.c_str( ) + QString( " &nbsp;" ) );
                playersWithRealms.append( j->m_PlayerName.c_str( ) + QString( "(" ) + j->m_PlayerRealm.c_str( ) + "), " );
                playersShadow.append( j->m_PlayerShadowName.c_str( ) + QString( " &nbsp;" ) );
            }
        }

        if( !players.isEmpty( ) )
        {
            players.chop( 7 );
            playersWithRealms.chop( 2 );
            playersShadow.chop( 7 );
        }
        else
        {
            players = "<font color = #000000>нет игроков</font>";
            playersWithRealms = "в этой игре отсутствуют игроки";
            playersShadow = "<font color = #50808080>нет игроков</font>";
        }

        GameInfoInit.append( players );
        GameInfoInit.append( playersWithRealms );
        GameInfoInit.append( playersShadow );
        m_UNM->m_UniqueGameID++;
        game->SetUniqueGameID( m_UNM->m_UniqueGameID );
        emit gToLog->addGameToListIrina( game->GetUniqueGameID( ), game->GetGameStarted( ), game->GetGameIcon( ), GameInfoInit );
        m_GPGames.push_back( game );
    }

    return !DuplicateFound;
}

bool CIrinaWS::AssignLength( BYTEARRAY &content )
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

BYTEARRAY CIrinaWS::SEND_GPSC_SPOOFCHECK_IRINA( )
{
    BYTEARRAY packet;

    for( vector<CIrinaGame *>::iterator i = m_GPGames.begin( ); i != m_GPGames.end( ); i++ )
    {
        if( ( *i )->GetUniqueGameID( ) == m_UNM->m_CurrentGameID && ( *i )->GetDataReceived( ) )
        {
            packet.push_back( 248 );                                                      // GPSC header constant
            packet.push_back( 10 );                                                       // SPOOFCHECK IRINA
            packet.push_back( 0 );                                                        // packet length will be assigned later
            packet.push_back( 0 );                                                        // packet length will be assigned later
            UTIL_AppendByteArrayFast( packet, ( *i )->m_GameData->m_GameKey );            // key
            packet.push_back( 0x00 );                                                     // key-end
            packet.push_back( 0x43 ); packet.push_back( 0x2b ); packet.push_back( 0x2b ); // н
            packet.push_back( 0x20 ); packet.push_back( 0x34 ); packet.push_back( 0x00 ); // е
            packet.push_back( 0x54 ); packet.push_back( 0x6f ); packet.push_back( 0x20 ); // и
            packet.push_back( 0x62 ); packet.push_back( 0x65 ); packet.push_back( 0x20 ); // з
            packet.push_back( 0x66 ); packet.push_back( 0x69 ); packet.push_back( 0x6c ); // в
            packet.push_back( 0x6c ); packet.push_back( 0x65 ); packet.push_back( 0x64 ); // е
            packet.push_back( 0x20 ); packet.push_back( 0x62 ); packet.push_back( 0x79 ); // с
            packet.push_back( 0x20 ); packet.push_back( 0x4f ); packet.push_back( 0x2e ); // т
            packet.push_back( 0x45 ); packet.push_back( 0x2e ); packet.push_back( 0x4d ); // н
            packet.push_back( 0x2e ); packet.push_back( 0x20 ); packet.push_back( 0x20 ); // ы
            packet.push_back( 0x00 ); packet.push_back( 0x31 ); packet.push_back( 0x61 ); // е
            packet.push_back( 0x38 ); packet.push_back( 0x35 ); packet.push_back( 0x62 ); //
            packet.push_back( 0x31 ); packet.push_back( 0x66 ); packet.push_back( 0x33 ); // ч
            packet.push_back( 0x00 ); packet.push_back( 0x30 ); packet.push_back( 0x30 ); // у
            packet.push_back( 0x30 ); packet.push_back( 0x30 ); packet.push_back( 0x30 ); // д
            packet.push_back( 0x30 ); packet.push_back( 0x30 ); packet.push_back( 0x30 ); // е
            packet.push_back( 0x20 ); packet.push_back( 0x20 ); packet.push_back( 0x20 ); // с
            packet.push_back( 0x20 ); packet.push_back( 0x20 ); packet.push_back( 0x20 ); // а
            packet.push_back( 0x00 ); packet.push_back( 0x01 );                           // !
            AssignLength( packet );
            break;
        }
    }

    return packet;
}

BYTEARRAY CIrinaWS::SEND_W3GS_GAMEINFO_IRINA( CIrinaGameData *GameData, uint32_t gameID, uint32_t upTime )
{
    BYTEARRAY packet;

    if( !GameData->m_GameName.empty( ) )
    {
        unsigned char ProductID_ROC[] = { 51, 82, 65, 87 };	// "WAR3"
        unsigned char ProductID_TFT[] = { 80, 88, 51, 87 };	// "W3XP"
        unsigned char Version[] = { m_UNM->m_LANWar3Version,  0,  0,  0 };
        unsigned char Unknown[] = { 1,  0,  0,  0 };
        unsigned char mapGameType[] = { 94,  0,  0,  0 };

        // у нас уже есть закодированая statstring в CIrinaGameData*
        // составляем пакет

        packet.push_back( W3GS_HEADER_CONSTANT );                           // W3GS header constant
        packet.push_back( 48 );                                             // W3GS_GAMEINFO
        packet.push_back( 0 );                                              // packet length will be assigned later
        packet.push_back( 0 );                                              // packet length will be assigned later

        if( m_UNM->m_TFT )
            UTIL_AppendByteArray( packet, ProductID_TFT, 4 );               // Product ID (TFT)
        else
            UTIL_AppendByteArray( packet, ProductID_ROC, 4 );               // Product ID (ROC)

        UTIL_AppendByteArray( packet, Version, 4 );                         // Version
        UTIL_AppendByteArray( packet, gameID, false );                      // Host Counter
        UTIL_AppendByteArray( packet, gameID, false );                      // Entry Key
        UTIL_AppendByteArrayFast( packet, GameData->m_GameName );           // Game Name
        packet.push_back( 0 );                                              // ??? (maybe game password)
        UTIL_AppendByteArrayFast( packet, GameData->m_EncodeStatString );	// Stat String
        packet.push_back( 0 );                                              // Stat String null terminator (the stat string is encoded to remove all even numbers i.e. zeros)
        UTIL_AppendByteArray( packet, (uint32_t)12, false );                // Slots Total
        UTIL_AppendByteArray( packet, mapGameType, 4 );                     // Game Type
        UTIL_AppendByteArray( packet, Unknown, 4 );                         // ???
        UTIL_AppendByteArray( packet, (uint32_t)12, false );                // Slots Open
        UTIL_AppendByteArray( packet, upTime, false );                      // time since creation
        UTIL_AppendByteArray( packet, m_UNM->m_Port, false );               // port
        AssignLength( packet );
    }
    else
        CONSOLE_Print( "[GPROXY: " + GameData->m_GameName + "] GAMEPROTO: invalid parameters passed to SEND_W3GS_GAMEINFO", 7, 0 );

    return packet;
}

////////////////////////////////
// RECEIVE FUNCTIONS (CLIENT) //
////////////////////////////////

vector<CIrinaGame *> CIrinaWS::RECEIVE_GET_GAMELIST( BYTEARRAY data )
{
    // 1 byte							-> IRINAWS_HEADER_CONSTANT
    // 1 byte                  			-> GET_GAMELIST
	// 2 bytes							-> кол-во игр
    // for( i = 0; i < кол-во игр; i++ )
	// {
    //		1 byte						-> начатая игра (1) или нет (0)
    //		null terminated string		-> gamename
    //		null terminated string		-> mapfilename
    //		1 byte						-> игра с паролем (игры с паролем имеют название типа "Игра #12345")
    //      1 byte                      -> сторонний бот
    //		1 byte						-> GamePowerUp: имеет игра подсветку на сайте (1) или нет (0)
    //		1 byte						-> GamePosition: 1 = сверху списка (power up/некоторый автохост), 3 = в середине списка (автохост), 0 = внизу списка (создана юзером/сторонний бот)
    //		4 bytes						-> game counter
    //		4 bytes						-> ms со старта игры
    //      4 bytes                     -> gamecreatorID
    //		null terminated string		-> bot name (логин, с которого хостится игра на айкапе)
    //		1 bytes						-> maxplayers (странно, всегда либо 1, либо 3)
    //		4 bytes						-> orderID
	//		1 bytes						-> кол-во слотов (для игроков) в игре
	// 		for( j = 0; j < кол-во слотов; j++ )
	//		{
	//			1 bytes					-> цвет (0 = красный, 1 = синий...)
	//			null terminated string	-> ник
	//			null terminated string	-> реалм игрока
    //			null terminated string	-> comment
	//		}
	//	}

    vector<CIrinaGame *> Games;

    if( data.size( ) < 33 )
		return Games;

    bool GameStarted;
    bool PowerUP;
    bool OtherBot;
    uint16_t NumberGames = UTIL_ByteArrayToUInt16( BYTEARRAY( data.begin( ) + 2, data.begin( ) + 4 ), false );
	uint32_t i = 4;
	uint32_t GameCounter;
    uint32_t GameTicks;
	unsigned char NumberSlots;
    unsigned char Position;
	BYTEARRAY GameName;
    BYTEARRAY MapName;
    BYTEARRAY MapFileName;
	BYTEARRAY BotName;
	unsigned char SlotColor;
	BYTEARRAY PlayerName;
	BYTEARRAY PlayerRealm;
    BYTEARRAY Comment;
    uint32_t SlotsCount;
    uint32_t PlayersCount;

	while( NumberGames > 0 )
    {
        if( data.size( ) < i + 29 )
			return Games;

        if( data[i] == 0 )
            GameStarted = false;
        else
            GameStarted = true;

		GameName = UTIL_ExtractCString( data, i + 1 );
        i += GameName.size( ) + 2;

        if( data.size( ) < i + 29 )
            return Games;

        MapName = UTIL_ExtractCString( data, i );
        i += MapName.size( ) + 1;

        if( data.size( ) < i + 28 )
            return Games;

        MapFileName = UTIL_ExtractCString( data, i );
        i += MapFileName.size( ) + 2;

        if( data.size( ) < i + 26 )
			return Games;

        if( data[i] == 0 )
            OtherBot = false;
        else
            OtherBot = true;

        i++;

        if( data[i] == 0 )
            PowerUP = false;
        else
            PowerUP = true;

        Position = data[i+1];
        i += 2;
		GameCounter = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
        i += 4;
        GameTicks = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
        i += 8;
		BotName = UTIL_ExtractCString( data, i );
        i += BotName.size( ) + 6;

        if( data.size( ) < i + 5 )
			return Games;

		NumberSlots = data[i];
        i++;

        if( data.size( ) < i + ( 4 * NumberSlots ) )
			return Games;

		vector<CIrinaSlots> Slots;
        SlotsCount = 0;
        PlayersCount = 0;

		while( NumberSlots > 0 )
		{
            SlotsCount++;
            NumberSlots--;

            if( data.size( ) < i + ( 4 * NumberSlots ) + 4 )
				return Games;

			SlotColor = data[i];
			PlayerName = UTIL_ExtractCString( data, i + 1 );
            i += PlayerName.size( ) + 2;

            if( data.size( ) < i + ( 4 * NumberSlots ) + 2 )
				return Games;

			PlayerRealm = UTIL_ExtractCString( data, i );

            if( !PlayerName.empty( ) )
            {
                if( PlayerRealm.empty( ) )
                {
                    PlayerRealm.push_back( 0x4c ); // L
                    PlayerRealm.push_back( 0x41 ); // A
                    PlayerRealm.push_back( 0x4e ); // N
                }

                PlayersCount++;
            }

            i += PlayerRealm.size( ) + 1;

            if( data.size( ) < i + ( 4 * NumberSlots ) + 1 )
                return Games;

            Comment = UTIL_ExtractCString( data, i );
            i += Comment.size( ) + 1;

            if( OtherBot && SlotsCount == 1 && NumberSlots == 0 && string( PlayerName.begin( ), PlayerName.end( ) ) == "Не известно" )
                Slots.push_back( CIrinaSlots( 25, string( ), string( ) ) );
            else
                Slots.push_back( CIrinaSlots( SlotColor, string( PlayerName.begin( ), PlayerName.end( ) ), string( PlayerRealm.begin( ), PlayerRealm.end( ) ) ) );
		}

		NumberGames--;
        Games.push_back( new CIrinaGame( GameCounter, string( GameName.begin( ), GameName.end( ) ), string( MapName.begin( ), MapName.end( ) ), string( MapFileName.begin( ), MapFileName.end( ) ), string( BotName.begin( ), BotName.end( ) ), GameStarted, PowerUP, OtherBot, Position, GameTicks, SlotsCount, PlayersCount, Slots ) );
	}

    return Games;
}

CIrinaUser *CIrinaWS::RECEIVE_LOGIN( BYTEARRAY data )
{
    // 1 byte							-> IRINAWS_HEADER_CONSTANT
    // 1 byte                  			-> LOGIN
    // null terminated string           -> id
    // null terminated string           -> discord name
    // null terminated string           -> discord avatar URL
    // 4 bytes                          -> local id
    // null terminated string           -> discord id
    // 4 bytes                          -> vk id
    // null terminated string           -> realm
    // null terminated string           -> bnet name
    // null terminated string           -> connector name
    // 1 byte                           -> type

    if( data.size( ) < 16 )
        return nullptr;

    uint32_t i = 2;
    BYTEARRAY ID = UTIL_ExtractCString( data, i );
    i += ID.size( ) + 1;

    if( data.size( ) < i + 15 )
        return nullptr;

    BYTEARRAY DiscordName = UTIL_ExtractCString( data, i );
    i += DiscordName.size( ) + 1;

    if( data.size( ) < i + 14 )
        return nullptr;

    BYTEARRAY DiscordAvatarURL = UTIL_ExtractCString( data, i );
    i += DiscordAvatarURL.size( ) + 1;

    if( data.size( ) < i + 13 )
        return nullptr;

    uint32_t LocalID = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
    BYTEARRAY DiscordID = UTIL_ExtractCString( data, i + 4 );
    i += DiscordID.size( ) + 5;

    if( data.size( ) < i + 8 )
        return nullptr;

    uint32_t VkID = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
    BYTEARRAY Realm = UTIL_ExtractCString( data, i + 4 );
    i += Realm.size( ) + 5;

    if( data.size( ) < i + 3 )
        return nullptr;

    BYTEARRAY BnetName = UTIL_ExtractCString( data, i );
    i += BnetName.size( ) + 1;

    if( data.size( ) < i + 2 )
        return nullptr;

    BYTEARRAY ConnectorName = UTIL_ExtractCString( data, i );
    i += ConnectorName.size( ) + 1;

    if( data.size( ) != i + 1 )
        return nullptr;

    unsigned char Type = data[i];
    CIrinaUser *IrinaUser = new CIrinaUser( string( ID.begin( ), ID.end( ) ), string( DiscordName.begin( ), DiscordName.end( ) ), string( DiscordAvatarURL.begin( ), DiscordAvatarURL.end( ) ), LocalID, string( DiscordID.begin( ), DiscordID.end( ) ), VkID, string( Realm.begin( ), Realm.end( ) ), string( BnetName.begin( ), BnetName.end( ) ), string( ConnectorName.begin( ), ConnectorName.end( ) ), Type );
    return IrinaUser;
}

CIrinaGameData *CIrinaWS::RECEIVE_GET_GAMEDATA( BYTEARRAY data )
{
    // 1 byte							-> IRINAWS_HEADER_CONSTANT
    // 1 byte                  			-> GET_GAMEDATA
    // null terminated string           -> key?
    // 4 bytes                          -> game counter
    // 4 bytes                          -> unknown
    // 2 bytes                          -> port
    // null terminated string           -> ip
    // null terminated string           -> gamename
    // null terminated bytearray        -> statstring
    // 21 bytes                         -> unknown

    if( data.size( ) < 55 )
        return nullptr;

    uint32_t i = 2;
    BYTEARRAY GameKey = UTIL_ExtractCString( data, i );
    i += GameKey.size( ) + 1;

    if( data.size( ) < 52 + i )
        return nullptr;

    uint32_t GameCounter = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
    uint16_t GamePort = UTIL_ByteArrayToUInt16( BYTEARRAY( data.begin( ) + i + 8, data.begin( ) + i + 10 ), false );
    BYTEARRAY GameIP = UTIL_ExtractCString( data, i + 10 );
    i += GameIP.size( ) + 11;

    if( data.size( ) < 41 + i )
        return nullptr;

    BYTEARRAY GameName = UTIL_ExtractCString( data, i );
    i += GameName.size( ) + 1;

    if( data.size( ) < 40 + i )
        return nullptr;

    BYTEARRAY EncodeStatString = UTIL_ExtractCString( data, i );
    i += EncodeStatString.size( ) + 1;

    if( data.size( ) < 21 + i )
        return nullptr;

    CIrinaGameData *IrinaGameData = new CIrinaGameData( GameCounter, GamePort, string( GameIP.begin( ), GameIP.end( ) ), string( GameName.begin( ), GameName.end( ) ), EncodeStatString, GameKey );
    return IrinaGameData;
}

CIrinaGameDataEX *CIrinaWS::RECEIVE_GET_GAMEDATAEX( BYTEARRAY data )
{
    // 1 byte							-> IRINAWS_HEADER_CONSTANT
    // 1 byte                  			-> GET_GAMEDATAEX
    // null terminated string           -> mapname
    // 4 bytes                          -> tgasize
    // tga size bytes                   -> tga
    // null terminated string           -> map author
    // null terminated string           -> map description
    // null terminated string           -> map players
    // 4 bytes                          -> game counter

    if( data.size( ) < 14 )
        return nullptr;

    uint32_t i = 2;
    BYTEARRAY MapName = UTIL_ExtractCString( data, i );
    i += MapName.size( ) + 1;

    if( data.size( ) < 11 + i )
        return nullptr;

    uint32_t TGASize = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
    i += 4;

    if( data.size( ) < 7 + i + TGASize )
        return nullptr;

    BYTEARRAY TGA = BYTEARRAY( data.begin( ) + i, data.begin( ) + i + TGASize );
    BYTEARRAY MapAuthor = UTIL_ExtractCString( data, i + TGASize );
    i += MapAuthor.size( ) + 1 + TGASize;

    if( data.size( ) < 6 + i )
        return nullptr;

    BYTEARRAY MapDescription = UTIL_ExtractCString( data, i );
    i += MapDescription.size( ) + 1;

    if( data.size( ) < 5 + i )
        return nullptr;

    BYTEARRAY MapPlayers = UTIL_ExtractCString( data, i );
    i += MapPlayers.size( ) + 1;

    if( data.size( ) < 4 + i )
        return nullptr;

    uint32_t GameCounter = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 ), false );
    CIrinaGameDataEX *IrinaGameDataEX = new CIrinaGameDataEX( string( MapName.begin( ), MapName.end( ) ), TGA, string( MapAuthor.begin( ), MapAuthor.end( ) ), string( MapDescription.begin( ), MapDescription.end( ) ), string( MapPlayers.begin( ), MapPlayers.end( ) ), GameCounter );
    return IrinaGameDataEX;
}

uint32_t CIrinaWS::RECEIVE_SET_GAMEDATA( BYTEARRAY data )
{
    // 1 byte							-> IRINALocal_HEADER_CONSTANT
    // 1 byte                  			-> SET_GAMEDATA
    // 4 byte                           -> Unique Game ID

    if( data.size( ) == 6 )
        return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 2, data.begin( ) + 6 ), false );

    return 0;
}

uint32_t CIrinaWS::RECEIVE_SET_GAMEDATAEX( BYTEARRAY data )
{
    // 1 byte							-> IRINALocal_HEADER_CONSTANT
    // 1 byte                  			-> SET_GAMEDATAEX
    // 4 byte                           -> Unique Game ID

    if( data.size( ) == 6 )
        return UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 2, data.begin( ) + 6 ), false );

    return 0;
}

//
// CIrinaGameDataEX
//

CIrinaGameDataEX::CIrinaGameDataEX( string nMapName, BYTEARRAY nTGA, string nMapAuthor, string nMapDescription, string nMapPlayers, uint32_t nGameCounter )
{
    m_MapName = UTIL_Wc3ColorStringFix( nMapName );
    m_TGA = nTGA;
    m_MapAuthor = UTIL_Wc3ColorStringFix( nMapAuthor );
    m_MapDescription = UTIL_Wc3ColorStringFix( nMapDescription );
    m_MapPlayers = UTIL_Wc3ColorStringFix( nMapPlayers );
    m_GameCounter = nGameCounter;
}

CIrinaGameDataEX::~CIrinaGameDataEX( )
{

}

//
// CIrinaGameData
//

CIrinaGameData::CIrinaGameData( uint32_t nGameCounter, uint16_t nGamePort, string nGameIP, string nGameName, BYTEARRAY nEncodeStatString, BYTEARRAY nGameKey )
{
    m_GameCounter = nGameCounter;
    m_GamePort = nGamePort;
    m_GameIP = nGameIP;
    m_GameName = nGameName;
    m_EncodeStatString = nEncodeStatString;
    m_GameKey = nGameKey;
    m_MapPath = string( );
}

CIrinaGameData::~CIrinaGameData( )
{

}

string CIrinaGameData::GetMapPath( )
{
    if( !m_MapPath.empty( ) )
        return m_MapPath;

    BYTEARRAY DecodeStatString = UTIL_DecodeStatString( m_EncodeStatString );

    if( DecodeStatString.size( ) >= 18 )
    {
        BYTEARRAY MapPath = UTIL_ExtractCString( DecodeStatString, 13 );

        if( !MapPath.empty( ) )
        {
            m_MapPath = string( MapPath.begin( ), MapPath.end( ) );

            if( !m_MapPath.empty( ) )
                m_MapPath = UTIL_GetFileNameFromPath( m_MapPath );
            else
                m_MapPath = "unknown map name";
        }
        else
            m_MapPath = "unknown map name";
    }
    else
        m_MapPath = "unknown map name";

    return m_MapPath;
}

//
// CIrinaUser
//

CIrinaUser::CIrinaUser( string nID, string nDiscordName, string nDiscordAvatarURL, uint32_t nLocalID, string nDiscordID, uint32_t nVkID, string nRealm, string nBnetName, string nConnectorName, unsigned char nType )
{
    m_ID = nID;
    m_DiscordName = nDiscordName;
    m_DiscordAvatarURL = nDiscordAvatarURL;
    m_LocalID = nLocalID;
    m_DiscordID = nDiscordID;
    m_VkID = nVkID;
    m_Realm = nRealm;
    m_BnetName = nBnetName;
    m_ConnectorName = nConnectorName;
    m_Type = nType;
}

CIrinaUser::~CIrinaUser( )
{

}

//
// CIrinaSlots
//

CIrinaSlots::CIrinaSlots( unsigned char nSlotColor, string nPlayerName, string nPlayerRealm )
{
    m_SlotColor = nSlotColor;

    if( nSlotColor == 25 )
    {
        m_PlayerName = "Нет информации о игроках";
        m_PlayerRealm = "Бот не отправил инфу о игроках для этой игры";
        m_PlayerColorName = "<font color = #000000>" + m_PlayerName + "</font>";
        m_PlayerShadowName = "<font color = #50808080>" + m_PlayerName + "</font>";
    }
    else
    {
        m_PlayerName = nPlayerName;

        if( nPlayerRealm == "178.218.214.114" )
            m_PlayerRealm = "iCCup";
        else
            m_PlayerRealm = nPlayerRealm;

        if( !m_PlayerName.empty( ) )
        {
            string::size_type MessageFind = m_PlayerName.find( "|c" );

            while( MessageFind != string::npos && MessageFind + 10 < m_PlayerName.size( ) )
            {
                m_PlayerName = m_PlayerName.substr( 0, MessageFind ) + m_PlayerName.substr( MessageFind + 10 );
                MessageFind = m_PlayerName.find( "|c" );
            }

            MessageFind = m_PlayerName.find( "|C" );

            while( MessageFind != string::npos && MessageFind + 10 < m_PlayerName.size( ) )
            {
                m_PlayerName = m_PlayerName.substr( 0, MessageFind ) + m_PlayerName.substr( MessageFind + 10 );
                MessageFind = m_PlayerName.find( "|C" );
            }

            MessageFind = m_PlayerName.find( "|r" );

            while( MessageFind != string::npos )
            {
                m_PlayerName = m_PlayerName.substr( 0, MessageFind ) + m_PlayerName.substr( MessageFind + 2 );
                MessageFind = m_PlayerName.find( "|r" );
            }

            MessageFind = m_PlayerName.find( "|R" );

            while( MessageFind != string::npos )
            {
                m_PlayerName = m_PlayerName.substr( 0, MessageFind ) + m_PlayerName.substr( MessageFind + 2 );
                MessageFind = m_PlayerName.find( "|R" );
            }

            switch( m_SlotColor )
            {
                case 0:
                m_PlayerColorName = "<font color = #ff0000>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                case 1:
                m_PlayerColorName = "<font color = #0000ff>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                case 2:
                m_PlayerColorName = "<font color = #00ffff>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                case 3:
                m_PlayerColorName = "<font color = #800080>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                case 4:
                m_PlayerColorName = "<font color = #ffff00>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                case 5:
                m_PlayerColorName = "<font color = #ffa500>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                case 6:
                m_PlayerColorName = "<font color = #00ff00>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                case 7:
                m_PlayerColorName = "<font color = #ff00ff>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                case 8:
                m_PlayerColorName = "<font color = #858585>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                case 9:
                m_PlayerColorName = "<font color = #add8e6>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                case 10:
                m_PlayerColorName = "<font color = #008000>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                case 11:
                m_PlayerColorName = "<font color = #964B00>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #70000000>" + m_PlayerName + "</font>";
                break;
                default:
                m_PlayerColorName = "<font color = #2D3236>" + m_PlayerName + "</font>";
                m_PlayerShadowName = "<font color = #50808080>" + m_PlayerName + "</font>";
            }
        }
        else
            m_PlayerColorName = string( );
    }
}

CIrinaSlots::~CIrinaSlots( )
{

}

//
// CIrinaGame
//

CIrinaGame::CIrinaGame( uint32_t nIrinaID, string nGameName, string nMapName, string nMapFileName, string nIccupBotName, bool nGameStarted, bool nPowerUP, bool nOtherBot, unsigned char nPosition, uint32_t nGameTicks, uint32_t nSlotsCount, uint32_t nPlayersCount, vector <CIrinaSlots> nSlots )
{
    m_Broadcasted = false;
    m_Updated = true;
    m_DataReceived = false;
    m_DataReceivedEX = false;
    m_DataReceivedRequestTime = 0;
    m_DataReceivedEXRequestTime = 0;
    m_GameStarted = nGameStarted;
    m_PowerUP = nPowerUP;
    m_OtherBot = nOtherBot;
    m_Position = nPosition;
    m_UniqueGameID = 0;
    m_IrinaID = nIrinaID;
    m_GameTicks = nGameTicks;
    m_GameName = nGameName;
    m_MapName = nMapName;
    m_MapFileName = nMapFileName;
    m_IccupBotName = nIccupBotName;
    m_SlotsCount = nSlotsCount;
    m_PlayersCount = nPlayersCount;
    m_Slots = nSlots;
    m_BadGameData = false;
    m_BadGameDataEX = false;
    m_GameData = nullptr;
    m_GameDataEX = nullptr;
}

bool CIrinaGame::UpdateGame( CIrinaGame *game )
{
    m_Updated = true;
    m_GameTicks = game->m_GameTicks;

    if( m_GameStarted != game->m_GameStarted || m_GameName != game->m_GameName || m_PlayersCount != game->m_PlayersCount || m_Slots.size( ) != game->m_Slots.size( ) )
    {
        m_GameStarted = game->m_GameStarted;
        m_GameName = game->m_GameName;
        m_SlotsCount = game->m_SlotsCount;
        m_PlayersCount = game->m_PlayersCount;
        m_Slots = game->m_Slots;
        return true;
    }
    else
    {
        for( uint32_t i = 0; i < m_Slots.size( ); i++ )
        {
            if( m_Slots[i].m_PlayerName != game->m_Slots[i].m_PlayerName || m_Slots[i].m_PlayerRealm != game->m_Slots[i].m_PlayerRealm )
            {
                m_Slots = game->m_Slots;
                return true;
            }
        }
    }

    return false;
}

unsigned int CIrinaGame::GetGameIcon( )
{
    /*
    m_Icons:

    0 = IirinaRed
    1 = IirinaBlue
    2 = IirinaAHRed
    3 = IirinaAHBlue
    4 = IirinaRedICCup
    5 = IirinaBlueICCup
    6 = IirinaAHRedICCup
    7 = IirinaAHBlueICCup
    8 = OtherBot
    */

    if( GetOtherBot( ) )
        return 8;
    else if( m_IccupBotName.empty( ) )
    {
        if( !GetAutoHost( ) )
            return m_PowerUP ? 0 : 1;
        else
            return m_PowerUP ? 2 : 3;
    }
    else if( !GetAutoHost( ) )
        return m_PowerUP ? 4 : 5;
    else
        return m_PowerUP ? 6 : 7;
}

CIrinaGame::~CIrinaGame( )
{
    if( m_GameData != nullptr )
        delete m_GameData;

    if( m_GameDataEX != nullptr )
        delete m_GameDataEX;
}
