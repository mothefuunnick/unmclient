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
#include "unmcrc32.h"
#include "config.h"
#include "language.h"
#include "socket.h"
#include "bnet.h"
#include "map.h"
#include "packed.h"
#include "savegame.h"
#include "replay.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "game_base.h"
#include "game.h"

using namespace std;

extern CLanguage *gLanguage;

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
// CGame
//

CGame::CGame( CUNM *nUNM, CMap *nMap, CSaveGame *nSaveGame, uint16_t nHostPort, unsigned char nGameState, string nGameName, string nOwnerName, string nCreatorName, string nCreatorServer ) : CBaseGame( nUNM, nMap, nSaveGame, nHostPort, nGameState, nGameName, nOwnerName, nCreatorName, nCreatorServer )
{
	m_GameOverTime = 0;
	m_GameLoadedTime = 0;
	m_Server = nCreatorServer;
}

CGame::~CGame( )
{

}

bool CGame::Update( void *fd, void *send_fd )
{
	return CBaseGame::Update( fd, send_fd );
}

void CGame::EventPlayerLeft( CGamePlayer *player, uint32_t reason )
{
    // this function is only called when a player leave packet is received, not when there's a socket error, kick, etc...
	// Check if leaver is admin/root admin with a loop then set the bool accordingly.

	CBaseGame::EventPlayerLeft( player, reason );
}

void CGame::EventPlayerDeleted( CGamePlayer *player )
{
	CBaseGame::EventPlayerDeleted( player );
}

void CGame::EventPlayerAction( CGamePlayer *player, CIncomingAction *action )
{
	CBaseGame::EventPlayerAction( player, action );
}

bool CGame::EventPlayerBotCommand( CGamePlayer *player, string command, string payload )
{
    bool HideCommand = CBaseGame::EventPlayerBotCommand( player, command, payload );

	// todotodo: don't be lazy

	string User = player->GetName( );
    string UserLower = UTIL_StringToLower( User );
	string UserServer = player->GetJoinedRealm( );
	string Command = command;
	string Payload = payload;
    bool OwnerCheck = IsOwner( User ) && ( player->GetSpoofed( ) || !player->GetWhoisShouldBeSent( ) );
    bool DefaultOwnerCheck = IsDefaultOwner( User ) && ( player->GetSpoofed( ) || !player->GetWhoisShouldBeSent( ) );
    string PayloadLower = Payload;

    if( !PayloadLower.empty( ) )
        PayloadLower = UTIL_StringToLower( PayloadLower );

    if( OwnerCheck || DefaultOwnerCheck )
	{
        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] admin [" + User + "] sent command [" + Command + "] with payload [" + Payload + "]", 6, m_CurrentGameCounter );

        /*****************
        * ADMIN COMMANDS *
        *****************/

        //
        // !CHAT
        // !GAMECHAT
        // !BNETCHAT
        // !ЧАТ
        //

        if( Command == "chat" || Command == "gamechat" || Command == "bnetchat" || Command == "чат" )
        {
            m_IntergameChat = !m_IntergameChat;

            if( m_IntergameChat )
            {
                for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                {
                    if( !( *i )->m_SpamSubBotChat )
                        ( *i )->QueueChatCommand( "Чат между игрой №" + UTIL_ToString( GetGameNr( ) + 1 ) + " и каналом включен!" );
                }

                SendAllChat( "Чат между этой игрой и каналом включен!" );
            }
            else
            {
                for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
                {
                    if( !( *i )->m_SpamSubBotChat )
                        ( *i )->QueueChatCommand( "Чат между игрой №" + UTIL_ToString( GetGameNr( ) + 1 ) + " и каналом отключен!" );
                }

                SendAllChat( "Чат между этой игрой и каналом отключен!" );
            }
        }

        //
        // !ADMINCHAT
        // !AC
        // !АДМИНЧАТ
        //

        else if( Command == "adminchat" || Command == "ac" || Command == "админчат" )
        {
            if( !Payload.empty( ) )
            {
                for( vector<CBaseGame *> ::iterator i = m_UNM->m_Games.begin( ); i != m_UNM->m_Games.end( ); i++ )
                {
                    for( vector<CGamePlayer *> ::iterator it = ( *i )->m_Players.begin( ); it != ( *i )->m_Players.end( ); it++ )
                    {
                        if( ( *it )->GetName( ) != User && ( *it )->GetSpoofed( ) && ( ( *i )->IsOwner( ( *it )->GetName( ) ) || ( *i )->IsDefaultOwner( ( *it )->GetName( ) ) ) )
                        {
                            if( GetGameNr( ) != ( *i )->GetGameNr( ) )
                                ( *i )->SendChat( ( *it )->GetPID( ), ( *it )->GetPID( ), "[" + User + "](ADMINCHAT): " + Payload );
                            else
                                SendChat( player->GetPID( ), ( *it )->GetPID( ), "(ADMINCHAT): " + Payload );
                        }
                    }
                }
            }

            if( !Payload.empty( ) && m_UNM->m_CurrentGame )
            {
                for( vector<CGamePlayer *> ::iterator i = m_UNM->m_CurrentGame->m_Players.begin( ); i != m_UNM->m_CurrentGame->m_Players.end( ); i++ )
                {
                    if( ( *i )->GetName( ) != User && ( *i )->GetSpoofed( ) && ( m_UNM->m_CurrentGame->IsOwner( ( *i )->GetName( ) ) || m_UNM->m_CurrentGame->IsDefaultOwner( ( *i )->GetName( ) ) ) )
                    {
                        if( m_GameLoaded || m_GameLoading )
                            m_UNM->m_CurrentGame->SendChat( ( *i )->GetPID( ), ( *i )->GetPID( ), "[" + User + "](ADMINCHAT): " + Payload );
                        else
                            SendChat( player->GetPID( ), ( *i )->GetPID( ), "(ADMINCHAT): " + Payload );
                    }
                }
            }

            return true;
        }

        //
        // !ADMINCHATLOCAL
        // !ACL
        //

        else if( Command == "adminchatlocal" || Command == "acl" )
        {
            if( !Payload.empty( ) )
            {
                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    if( ( *i )->GetName( ) != User && ( *i )->GetSpoofed( ) && ( IsOwner( ( *i )->GetName( ) ) || IsDefaultOwner( ( *i )->GetName( ) ) ) )
                        SendChat( player->GetPID( ), ( *i )->GetPID( ), "(ADMINCHAT): " + Payload );
                }
            }

            return true;
        }

        //
        // !ABORT (abort countdown)
        // !A
        // !ОТМЕНА
        //

        // we use "!a" as an alias for abort because you don't have much time to abort the countdown so it's useful for the abort command to be easy to type

        else if( ( Command == "abort" || Command == "a" || Command == "отмена" ) && m_CountDownStarted && !m_GameLoading && !m_GameLoaded && !m_NormalCountdown )
        {
            if( m_AutoStartPlayers == 0 || GetNumHumanPlayers( ) < m_AutoStartPlayers )
            {
                m_CountDownStarted = false;
                SendAllChat( gLanguage->CountDownAborted( ) );
            }
            else
            {
                m_AutoStartPlayers = 0;
                m_CountDownStarted = false;
                SendAllChat( gLanguage->AutostartCanceled( ) );
            }
        }

        //
        // !CD
        // !COUNTDOWN
        //

        else if( Command == "cd" || Command == "countdown" )
        {
            if( !m_NormalCountdown )
            {
                SendChat( player->GetPID( ), "Normal countdown active!" );
                m_NormalCountdown = true;
            }
            else
            {
                SendChat( player->GetPID( ), "Unm countdown active!" );
                m_NormalCountdown = false;
            }
        }

        //
        // !CLEARBLNAME (Clear Blacklisted Name)
        // !CLEARBLACKLISTEDNAME
        // !CLEARBLACKLISTNAME
        // !CBL
        //

        else if( Command == "clearblname" || Command == "clearblacklistedname" || Command == "clearblacklistname" || Command == "cbl" )
        {
            m_BannedNames.clear( );
        }

        //
        // !DELBLNAME (DEL Blacklisted Name)
        // !DELBLACKLISTEDNAME
        // !DELBLACKLISTNAME
        // !DBL
        //

        else if( ( Command == "delblname" || Command == "delblacklistedname" || Command == "delblacklistname" || Command == "dbl" ) && !Payload.empty( ) )
        {
            string PlayerName;
            CGamePlayer *LastMatch = nullptr;
            uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

            if( Matches == 1 )
                PlayerName = LastMatch->GetName( );
            else
                PlayerName = Payload;

            if( !IsBlacklisted( m_BannedNames, PlayerName ) )
            {
                SendChat( player->GetPID( ), "Игрок [" + PlayerName + "] не находится в блеклисте." );
                return HideCommand;
            }

            if( Matches <= 1 )
            {
                DelBlacklistedName( PlayerName );
                SendChat( player->GetPID( ), "Игрок [" + PlayerName + "] был удален из временного блеклиста." );
            }
            else
                SendChat( player->GetPID( ), "Can't del blacklisted name. More than one match found" );
        }

        //
        // !ADDIPTOBLACKLIST
        // !ADDIPBLACKLIST
        // !ADDIPTOBL
        // !ADDIPBL
        // !BLACKLISTEDIP
        // !IPBLACKLISTED
        // !BLACKLISTIP
        // !IPBLACKLIST
        // !IPTOBLACKLIST
        // !IPTOBL
        // !IPBL
        // !BLIP
        //

        else if( Command == "addiptoblacklist" || Command == "addipblacklist" || Command == "addiptobl" || Command == "addipbl" || Command == "blacklistedip" || Command == "ipblacklisted" || Command == "blacklistip" || Command == "ipblacklist" || Command == "iptoblacklist" || Command == "iptobl" || Command == "ipbl" || Command == "blip" )
        {
            if( !DefaultOwnerCheck )
                SendChat( player->GetPID( ), gLanguage->YouDontHaveAccessToThatCommand( ) );
            else if( m_UNM->m_IPBlackListFile.empty( ) )
                SendChat( player, gLanguage->CommandDisabled( string( 1, m_UNM->m_CommandTrigger ) + Command ) );
            else if( Payload.empty( ) )
                SendChat( player, "Укажите IP, который следует внести в блэклист, например: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " 192.168.0.1" );
            else if( !m_UNM->IPIsValid( Payload ) )
                SendChat( player, "Не удалось добавить [" + Payload + "] в блэклист, указан некорректный IP!" );
            else
            {
                ifstream in;

#ifdef WIN32
                in.open( UTIL_UTF8ToCP1251( m_UNM->m_IPBlackListFile.c_str( ) ) );
#else
                in.open( m_UNM->m_IPBlackListFile );
#endif

                if( in.fail( ) )
                    SendChat( player, "Не удалось открыть файл [" + m_UNM->m_IPBlackListFile + "]" );
                else
                {
                    string Line;

                    while( !in.eof( ) )
                    {
                        getline( in, Line );
                        Line = UTIL_FixFileLine( Line, true );

                        // ignore blank lines and comments

                        if( Line.empty( ) || Line[0] == '#' )
                            continue;

                        // ignore lines that don't look like IP addresses

                        if( Line.find_first_not_of( "1234567890." ) != string::npos )
                            continue;

                        if( Line == Payload )
                        {
                            SendChat( player, "IP [" + Payload + "] уже имеется в блэклисте!" );
                            return true;
                        }
                    }

                    SendChat( player->GetPID( ), "IP [" + Payload + "] был успешно добавлен в блэклист!" );
                    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] admin [" + User + "] added IP [" + Payload + "] to blacklist file", 6, m_CurrentGameCounter );
                    m_IPBlackList.insert( Payload );
                    ofstream in2;
#ifdef WIN32
                    in2.open( UTIL_UTF8ToCP1251( m_UNM->m_IPBlackListFile.c_str( ) ), ios::app );
#else
                    in2.open( m_UNM->m_IPBlackListFile, ios::app );
#endif
                    in2 << Payload << endl;
                    in2.close( );

                    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    {
                        if( !( *i )->GetLeftMessageSent( ) && ( *i )->GetExternalIPString( ) == Payload )
                        {
                            SendAllChat( "Игрок [" + ( *i )->GetName( ) + "] забанен по IP админом [" + User + "]" );
                            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] admin [" + User + "] banned player [" + ( *i )->GetName( ) + "] by IP", 6, m_CurrentGameCounter );
                            ( *i )->SetDeleteMe( true );
                            ( *i )->SetLeftReason( "kicked and banned by ip" );

                            if( !m_GameLoading && !m_GameLoaded )
                            {
                                ( *i )->SetLeftCode( PLAYERLEAVE_LOBBY );
                                OpenSlot( GetSIDFromPID( ( *i )->GetPID( ) ), false );
                            }
                            else
                                ( *i )->SetLeftCode( PLAYERLEAVE_LOST );
                        }
                    }
                }

                in.close( );
            }

            return true;
        }

        //
        // !ANNOUNCE
        // !ANN
        // !АНОНС
        //

        else if( ( Command == "announce" || Command == "ann" || Command == "анонс" ) && !m_CountDownStarted )
        {
            if( Payload.empty( ) || PayloadLower == "off" )
            {
                SendAllChat( gLanguage->AnnounceMessageDisabled( ) );
                SetAnnounce( 0, string( ) );
            }
            else
            {
                // extract the interval and the message
                // e.g. "30 hello everyone" -> interval: "30", message: "hello everyone"

                uint32_t Interval;
                string Message;
                stringstream SS;
                SS << Payload;
                SS >> Interval;

                if( SS.fail( ) || Interval == 0 )
                    SendChat( player, "bad input #1 to [" + Command + "] command" );
                else
                {
                    if( SS.eof( ) )
                        SendChat( player, "missing input #2 to [" + Command + "] command" );
                    else
                    {
                        getline( SS, Message );

                        if( Message.size( ) > 0 && Message[0] == ' ' )
                            Message = Message.substr( 1 );

                        if( Message.empty( ) )
                            SendChat( player, "missing input #2 to [" + Command + "] command" );
                        else
                        {
                            SendAllChat( gLanguage->AnnounceMessageEnabled( ) );
                            SetAnnounce( Interval, Message );
                        }
                    }
                }
            }
        }

        //
        // !AUTOSAVE
        // !АВТОСОХРАНЕНИЕ
        // !АВТОСЕЙВ
        //

        else if( Command == "autosave" || Command == "автосохранение" || Command == "автосейв" )
        {
            if( PayloadLower == "on" )
            {
                SendAllChat( gLanguage->AutoSaveEnabled( ) );
                m_AutoSave = true;
            }
            else if( PayloadLower == "off" )
            {
                SendAllChat( gLanguage->AutoSaveDisabled( ) );
                m_AutoSave = false;
            }
            else
            {
                m_AutoSave = !m_AutoSave;

                if( m_AutoSave )
                    SendAllChat( gLanguage->AutoSaveEnabled( ) );
                else
                    SendAllChat( gLanguage->AutoSaveDisabled( ) );
            }
        }

        //
        // !AUTOSTART
        // !AS
        // !АВТОСТАРТ
        // !АС
        //

        else if( ( Command == "autostart" || Command == "as" || Command == "автостарт" || Command == "ас" ) && !m_GameLoading && !m_GameLoaded )
        {
            if( Payload.empty( ) || PayloadLower == "off" || PayloadLower == "0" )
            {
                m_AutoStartPlayers = 0;

                if( m_CountDownStarted && !m_NormalCountdown )
                    m_CountDownStarted = false;

                SendAllChat( gLanguage->AutoStartDisabled( string( 1, m_UNM->m_CommandTrigger ) ) );
            }
            else
            {
                uint32_t AutoStartPlayers = UTIL_ToUInt32( Payload );

                if( AutoStartPlayers >= 1 && AutoStartPlayers < 13 )
                {
                    if( m_AutoStartPlayers != 0 )
                    {
                        if( AutoStartPlayers > m_AutoStartPlayers && m_CountDownStarted && !m_NormalCountdown )
                        {
                            m_CountDownStarted = false;
                            SendAllChat( gLanguage->AutostartCanceled( ) );
                        }

                        m_AutoStartPlayers = AutoStartPlayers;
                        SendAllChat( gLanguage->AutoStartEnabled( UTIL_ToString( AutoStartPlayers ) ) );
                    }
                    else
                    {
                        m_AutoStartPlayers = AutoStartPlayers;
                        SendChat( player->GetPID( ), string( 1, m_UNM->m_CommandTrigger ) + "unlock/" + string( 1, m_UNM->m_CommandTrigger ) + "lock - разрешить/запретить голосование за старт" );
                        SendAllChat( gLanguage->AutoStartEnabled( UTIL_ToString( AutoStartPlayers ) ) );
                    }
                }
                else
                {
                    SendChat( player->GetPID( ), "Используй значение от 1 до 12 чтобы включить автостарт, 0 - выключить" );
                    return HideCommand;
                }
            }
        }

        //
        // !CHECK
        // !ПРОВЕРИТЬ
        // !ПРОВЕРКА
        //

        else if( Command == "check" || Command == "проверить" || Command == "проверка" )
        {
            if( !Payload.empty( ) )
            {
                CGamePlayer *LastMatch = nullptr;
                uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

                if( Matches == 0 )
                    SendAllChat( gLanguage->UnableToCheckPlayerNoMatchesFound( Payload ) );
                else if( Matches == 1 )
                    SendAllChat( gLanguage->CheckedPlayer( LastMatch->GetName( ), LastMatch->GetNumPings( ) > 0 ? UTIL_ToString( LastMatch->GetPing( m_UNM->m_LCPings ) ) + "ms" : "N/A", "N/A", IsDefaultOwner( LastMatch->GetName( ) ) ? "Yes" : "No", IsOwner( LastMatch->GetName( ) ) ? "Yes" : "No", LastMatch->GetSpoofed( ) ? "Yes" : "No", LastMatch->GetJoinedRealm( ).empty( ) ? "N/A" : LastMatch->GetJoinedRealm( ), LastMatch->GetReserved( ) ? "Yes" : "No" ) );
                else
                    SendAllChat( gLanguage->UnableToCheckPlayerFoundMoreThanOneMatch( Payload ) );
            }
            else
                SendChat( player, gLanguage->CheckedPlayer( User, player->GetNumPings( ) > 0 ? UTIL_ToString( player->GetPing( m_UNM->m_LCPings ) ) + "ms" : "N/A", "N/A", DefaultOwnerCheck ? "Yes" : "No", OwnerCheck ? "Yes" : "No", player->GetSpoofed( ) ? "Yes" : "No", player->GetJoinedRealm( ).empty( ) ? "N/A" : player->GetJoinedRealm( ), player->GetReserved( ) ? "Yes" : "No" ) );

            return HideCommand;
        }

        //
        // !CLEARHCL
        //

        else if( Command == "clearhcl" && !m_CountDownStarted )
        {
            m_HCLCommandString.clear( );
            SendAllChat( gLanguage->ClearingHCL( ) );
        }

        //
        // !CLOSE (close slot)
        // !ЗАКРЫТЬ
        //

        else if( ( Command == "close" || Command == "закрыть" ) && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
        {
            // close as many slots as specified, e.g. "5 10" closes slots 5 and 10

            stringstream SS;
            SS << Payload;
            bool failed = false;
            bool succes = false;
            bool msgself = false;
            bool msgcomp = false;
            bool msgfake = false;

            while( !SS.eof( ) )
            {
                string sSID;
                uint32_t SID;
                SS >> sSID;
                bool canclose = true;

                if( SS.fail( ) )
                {
                    if( !succes )
                        SendChat( player, "bad input to [" + Command + "] command" );

                    return HideCommand;
                }

                SID = UTIL_ToUInt32( sSID );

                if( SID == 0 )
                    failed = true;
                else if( SID - 1 < m_Slots.size( ) )
                {
                    succes = true;
                    CGamePlayer *Player = GetPlayerFromSID( static_cast<unsigned char>( SID - 1 ) );

                    if( Player )
                    {
                        if( Player->GetName( ) == User )
                        {
                            if( !msgself )
                            {
                                SendChat( player->GetPID( ), "Нельзя кикнуть самого себя!" );
                                msgself = true;
                            }

                            canclose = false;
                        }
                    }
                    else
                    {
                        if( m_Slots[SID - 1].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID - 1].GetComputer( ) == 1 )
                        {
                            if( !msgcomp )
                            {
                                SendChat( player->GetPID( ), "You have closed computer slot, type " + string( 1, m_UNM->m_CommandTrigger ) + "comp " + UTIL_ToString( SID ) + " to make it a comp again" );
                                msgcomp = true;
                            }
                        }
                        else if( m_Slots[SID - 1].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED )
                        {
                            if( !msgfake )
                            {
                                SendChat( player->GetPID( ), gLanguage->DoDFInstead( string( 1, m_UNM->m_CommandTrigger ) ) );
                                msgfake = true;
                            }

                            canclose = false;	// here to prevent kicking a fakeplayer which resulted as incorrect slots info and cause all players to be dropped when the game starts
                        }
                    }

                    if( canclose )
                        CloseSlot( static_cast<unsigned char>( SID - 1 ), true );
                }
            }

            if( failed && !succes )
                SendChat( player, "bad input to [" + Command + "] command" );
        }

        //
        // !CLOSEALL
        //

        else if( Command == "closeall" && !m_GameLoading && !m_GameLoaded )
        {
            CloseAllSlots( );
        }

        //
        // !COMP (computer slot)
        // !COMPUTER
        // !ADDBOT
        // !КОМП
        //

        else if( ( Command == "comp" || Command == "computer" || Command == "addbot" || Command == "комп" ) && !m_GameLoading && !m_GameLoaded && !m_SaveGame )
        {
            if( m_Slots.empty( ) )
            {
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна для этой карты!" );
                return HideCommand;
            }
            else if( Payload.find_first_not_of( " " ) == string::npos )
            {
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <colour>" );
                return HideCommand;
            }

            // extract the slot and the skill
            // e.g. "1 2" -> slot: "1", skill: "2"

            uint32_t SID;
            uint32_t Skill = 1;
            stringstream SS;
            SS << Payload;
            SS >> SID;

            if( SS.fail( ) )
                SendChat( player->GetPID( ), "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot(1-12)> <skill(0-2)>" );
            else
            {
                if( !SS.eof( ) )
                    SS >> Skill;

                if( SS.fail( ) )
                    SendChat( player->GetPID( ), "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot(1-12)> <skill(0-2)>" );
                else if( SID - 1 >= m_Slots.size( ) )
                    SendChat( player->GetPID( ), "Указан несуществующий слот" );
                else if( Skill > 3 )
                    SendChat( player->GetPID( ), "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot(1-12)> <skill(0-2)>" );
                else
                {
                    if( Skill == 3 )
                        Skill = 2;

                    CGamePlayer *Player = GetPlayerFromSID( static_cast<unsigned char>( SID - 1 ) );

                    if( Player )
                    {
                        if( Player->GetName( ) == User )
                            SendChat( player->GetPID( ), "Нельзя кикнуть самого себя!" );
                        else
                        {
                            OpenSlot( static_cast<unsigned char>( SID - 1 ), true );
                            ComputerSlot( static_cast<unsigned char>( SID - 1 ), static_cast<unsigned char>(Skill), true );
                        }
                    }
                    else
                    {
                        if( m_Slots[SID - 1].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID - 1].GetComputer( ) == 1 )
                        {
                            OpenSlot( static_cast<unsigned char>( SID - 1 ), true );
                            ComputerSlot( static_cast<unsigned char>( SID - 1 ), static_cast<unsigned char>(Skill), true );
                        }
                        else if( m_Slots[SID - 1].GetSlotStatus( ) == SLOTSTATUS_OPEN || m_Slots[SID - 1].GetSlotStatus( ) == SLOTSTATUS_CLOSED )
                            ComputerSlot( static_cast<unsigned char>( SID - 1 ), static_cast<unsigned char>(Skill), true );
                        else
                            SendChat( player->GetPID( ), gLanguage->DoDFInstead( string( 1, m_UNM->m_CommandTrigger ) ) );	// here to prevent kicking a fakeplayer which resulted as incorrect slots info and cause all players to be dropped when the game starts
                    }
                }
            }
        }

        //
        // !COMPCOLOUR (computer colour change)
        // !COMPCOLOR
        // !BOTCOLOUR
        // !BOTCOLOR
        // !EDITCOLOUR
        // !EDITCOLOR
        // !COLOUREDIT
        // !COLOREDIT
        //

        else if( ( Command == "compcolour" || Command == "compcolor" || Command == "botcolour" || Command == "botcolor" || Command == "editcolour" || Command == "editcolor" || Command == "colouredit" || Command == "coloredit" ) && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded && !m_SaveGame )
        {
            if( m_Slots.empty( ) )
            {
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна для этой карты!" );
                return HideCommand;
            }
            else if( Payload.find_first_not_of( " " ) == string::npos )
            {
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <colour>" );
                return HideCommand;
            }

            // extract the slot and the colour
            // e.g. "1 2" -> slot: "1", colour: "2"

            uint32_t Slot;
            uint32_t Colour;
            stringstream SS;
            SS << Payload;
            SS >> Slot;

            if( SS.fail( ) )
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <colour>" );
            else
            {
                if( SS.eof( ) )
                    SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <colour>" );
                else
                {
                    SS >> Colour;

                    if( SS.fail( ) || Colour == 0 || Colour > 12 )
                        SendChat( player, "Некорректное значение <colour>. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <1/2/3/4/5/6/7/8/9/10/11/12>" );
                    else
                    {
                        unsigned char SID = static_cast<unsigned char>( Slot - 1 );

                        if( SID < m_Slots.size( ) )
                        {
                            if( ( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 ) || Command == "editcolour" || Command == "editcolor" || Command == "colouredit" || Command == "coloredit" )
                            {
                                ColourSlot( SID, static_cast<unsigned char>( Colour - 1 ) );
                                SendChat( player, "Цвет для слота [" + UTIL_ToString(Slot) + "] был изменён" );
                            }
                            else if( Command == "compcolour" || Command == "botcolour" )
                                SendChat( player, "Необходимо указать номер слота занятого ботом! Чтобы редактировать любой другой слот, используйте: " + string( 1, m_UNM->m_CommandTrigger ) + "editcolour" );
                            else
                                SendChat( player, "Необходимо указать номер слота занятого ботом! Чтобы редактировать любой другой слот, используйте: " + string( 1, m_UNM->m_CommandTrigger ) + "editcolor" );
                        }
                        else
                            SendChat( player, "Указан несуществующий номер слота!" );
                    }
                }
            }
        }

        //
        // !COMPHANDICAP (computer handicap change)
        // !BOTHANDICAP
        // !EDITHANDICAP
        // !HANDICAPEDIT
        //

        else if( ( Command == "comphandicap" || Command == "bothandicap" || Command == "edithandicap" || Command == "handicapedit" ) && !m_GameLoading && !m_GameLoaded && !m_SaveGame )
        {
            if( m_Slots.empty( ) )
            {
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна для этой карты!" );
                return HideCommand;
            }
            else if( Payload.find_first_not_of( " " ) == string::npos )
            {
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <handicap>" );
                return HideCommand;
            }

            // extract the slot and the handicap
            // e.g. "1 50" -> slot: "1", handicap: "50"

            uint32_t Slot;
            uint32_t Handicap;
            stringstream SS;
            SS << Payload;
            SS >> Slot;

            if( SS.fail( ) )
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <handicap>" );
            else
            {
                if( ( Slot - 1 ) >= m_Slots.size( ) )
                    SendChat( player, "Указан несуществующий номер слота!" );
                else if( SS.eof( ) )
                    SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <handicap>" );
                else
                {
                    SS >> Handicap;

                    if( SS.fail( ) || ( Handicap != 50 && Handicap != 60 && Handicap != 70 && Handicap != 80 && Handicap != 90 && Handicap != 100 ) )
                        SendChat( player, "Некорректное значение <handicap>. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <50/60/70/80/90/100>" );
                    else
                    {
                        unsigned char SID = static_cast<unsigned char>( Slot - 1 );

                        if( ( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 ) || Command == "edithandicap" || Command == "handicapedit" )
                        {
                            m_Slots[SID].SetHandicap( static_cast<unsigned char>(Handicap) );
                            SendAllSlotInfo( );
                            SendChat( player, "Гандикап для слота [" + UTIL_ToString(Slot) + "] был изменён" );
                        }
                        else
                            SendChat( player, "Необходимо указать номер слота занятого ботом! Чтобы редактировать любой другой слот, используйте: " + string( 1, m_UNM->m_CommandTrigger ) + "edithandicap" );
                    }
                }
            }
        }

        //
        // !COMPRACE (computer race change)
        // !BOTRACE
        // !EDITRACE
        // !RACEEDIT
        //

        else if( ( Command == "comprace" || Command == "botrace" || Command == "editrace" || Command == "raceedit" ) && !m_GameLoading && !m_GameLoaded && !m_SaveGame )
        {
            if( m_Slots.empty( ) )
            {
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна для этой карты!" );
                return HideCommand;
            }
            else if( Payload.find_first_not_of( " " ) == string::npos )
            {
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <race>" );
                return HideCommand;
            }

            // extract the slot and the race
            // e.g. "1 human" -> slot: "1", race: "human"

            uint32_t Slot;
            string Race;
            stringstream SS;
            SS << Payload;
            SS >> Slot;

            if( SS.fail( ) )
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <race>" );
            else
            {
                if( SS.eof( ) )
                    SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <race>" );
                else
                {
                    getline( SS, Race );
                    string::size_type Start = Race.find_first_not_of( " " );

                    if( Start != string::npos )
                        Race = Race.substr( Start );
                    else
                    {
                        SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <race>" );
                        return HideCommand;
                    }

                    transform( Race.begin( ), Race.end( ), Race.begin( ), static_cast<int(*)(int)>(tolower) );
                    unsigned char SID = static_cast<unsigned char>( Slot - 1 );

                    if( Race.find_first_not_of( " " ) == string::npos )
                        SendChat( player, "Некорректное значение <race>. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <0/1/2/3/4>" );
                    else if( m_Map->GetMapFlags( ) & MAPFLAG_RANDOMRACES )
                        SendChat( player, "Конфиг данной карты запрещает игрокам выбирать расу!" );
                    else if( SID >= m_Slots.size( ) )
                        SendChat( player, "Указан несуществующий номер слота!" );
                    else
                    {
                        if( ( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 ) || Command == "editrace" || Command == "raceedit" )
                        {
                            if( Race == "human" || Race == "1" )
                            {
                                m_Slots[SID].SetRace( m_UNM->m_PlayersCanChangeRace ? 65 : SLOTRACE_HUMAN );
                                SendAllSlotInfo( );
                                SendChat( player, "Раса для слота [" + UTIL_ToString(Slot) + "] была изменена" );
                            }
                            else if( Race == "orc" || Race == "2" )
                            {
                                m_Slots[SID].SetRace( m_UNM->m_PlayersCanChangeRace ? 66 : SLOTRACE_ORC );
                                SendAllSlotInfo( );
                                SendChat( player, "Раса для слота [" + UTIL_ToString(Slot) + "] была изменена" );
                            }
                            else if( Race == "night elf" || Race == "elf" || Race == "4" || Race == "5" )
                            {
                                m_Slots[SID].SetRace( m_UNM->m_PlayersCanChangeRace ? 68 : SLOTRACE_NIGHTELF );
                                SendAllSlotInfo( );
                                SendChat( player, "Раса для слота [" + UTIL_ToString(Slot) + "] была изменена" );
                            }
                            else if( Race == "undead" || Race == "3" )
                            {
                                m_Slots[SID].SetRace( m_UNM->m_PlayersCanChangeRace ? 72 : SLOTRACE_UNDEAD );
                                SendAllSlotInfo( );
                                SendChat( player, "Раса для слота [" + UTIL_ToString(Slot) + "] была изменена" );
                            }
                            else if( Race == "random" || Race == "0" )
                            {
                                m_Slots[SID].SetRace( m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM );
                                SendAllSlotInfo( );
                                SendChat( player, "Раса для слота [" + UTIL_ToString(Slot) + "] была изменена" );
                            }
                            else
                                SendChat( player, "Некорректное значение <race>. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <0/1/2/3/4>" );
                        }
                        else
                            SendChat( player, "Необходимо указать номер слота занятого ботом! Чтобы редактировать любой другой слот, используйте: " + string( 1, m_UNM->m_CommandTrigger ) + "editrace" );
                    }
                }
            }
        }

        //
        // !COMPTEAM (computer team change)
        // !BOTTEAM
        // !COMPCLAN
        // !BOTCLAN
        // !EDITTEAM
        // !TEAMEDIT
        // !EDITCLAN
        // !CLANEDIT
        //

        else if( ( Command == "compteam" || Command == "botteam" || Command == "compclan" || Command == "botclan" || Command == "editteam" || Command == "teamedit" || Command == "editclan" || Command == "clanedit" ) && !m_GameLoading && !m_GameLoaded && !m_SaveGame )
        {
            if( m_Slots.empty( ) )
            {
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна для этой карты!" );
                return HideCommand;
            }
            else if( Payload.find_first_not_of( " " ) == string::npos )
            {
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <team>" );
                return HideCommand;
            }

            // extract the slot and the team
            // e.g. "1 2" -> slot: "1", team: "2"

            uint32_t Slot;
            uint32_t Team;
            stringstream SS;
            SS << Payload;
            SS >> Slot;

            if( SS.fail( ) )
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <team>" );
            else
            {
                if( SS.eof( ) )
                    SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <team>" );
                else
                {
                    SS >> Team;

                    if( SS.fail( ) || Team == 0 || Team > m_GetMapNumTeams )
                    {
                        if( m_GetMapNumTeams == 0 )
                            SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна для этой карты!" );
                        else if( m_GetMapNumTeams == 1 )
                            SendChat( player, "Некорректное значение <team>. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <1>" );
                        else
                            SendChat( player, "Некорректное значение <team>. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <1-" + UTIL_ToString( m_GetMapNumTeams ) + ">" );
                    }
                    else
                    {
                        unsigned char SID = static_cast<unsigned char>( Slot - 1 );

                        if( Team < 12 && SID < m_Slots.size( ) )
                        {
                            if( ( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 ) || Command == "editteam" || Command == "teamedit" || Command == "editclan" || Command == "clanedit" )
                            {
                                m_Slots[SID].SetTeam( static_cast<unsigned char>( Team - 1 ) );
                                SendAllSlotInfo( );
                                SendChat( player, "Клан для слота [" + UTIL_ToString(Slot) + "] был изменён" );
                            }
                            else if( Command == "compteam" || Command == "botteam" )
                                SendChat( player, "Необходимо указать номер слота занятого ботом! Чтобы редактировать любой другой слот, используйте: " + string( 1, m_UNM->m_CommandTrigger ) + "editteam" );
                            else
                                SendChat( player, "Необходимо указать номер слота занятого ботом! Чтобы редактировать любой другой слот, используйте: " + string( 1, m_UNM->m_CommandTrigger ) + "editclan" );
                        }
                        else
                            SendChat( player, "Указан несуществующий номер слота!" );
                    }
                }
            }
        }

        //
        // !SLOTEDIT
        // !EDITSLOT
        // !SE
        // !ES
        //

        else if( ( Command == "slotedit" || Command == "editslot" || Command == "se" || Command == "es" ) && !m_GameLoading && !m_GameLoaded && !m_SaveGame )
        {
            string CommandHelp = ": " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <race> <team> <colour> <handicap>";

            if( m_Slots.empty( ) )
            {
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна для этой карты!" );
                return HideCommand;
            }
            else if( Payload.find_first_not_of( " " ) == string::npos )
            {
                SendChat( player, "Некорректный ввод команды. Правильное использование" + CommandHelp );
                return HideCommand;
            }

            uint32_t Slot;
            string RaceS = string( );
            string TeamS = string( );
            string ColourS = string( );
            string HandicapS = string( );
            stringstream SS;
            SS << PayloadLower;
            SS >> Slot;

            if( SS.fail( ) )
            {
                if( m_Slots.size( ) == 1 )
                    SendChat( player, "Некорректно указан номер слота, используйте значение \"1\"" + CommandHelp );
                else
                    SendChat( player, "Некорректно указан номер слота, используйте значение 1-" + UTIL_ToString( m_Slots.size( ) ) + CommandHelp );
            }
            else
            {
                if( SS.eof( ) )
                    SendChat( player, "Не указан параметр <race>, используйте значение 0-4" + CommandHelp );
                else
                {
                    SS >> RaceS;

                    if( SS.fail( ) )
                        SendChat( player, "Некорректно указан параметр <race>, используйте значение 0-4" + CommandHelp );
                    else
                    {
                        if( SS.eof( ) )
                        {
                            if( m_GetMapNumTeams == 0 )
                                SendChat( player, "Не указан параметр <team>" + CommandHelp );
                            else if( m_GetMapNumTeams == 1 )
                                SendChat( player, "Не указан параметр <team>, используйте значение \"1\"" + CommandHelp );
                            else
                                SendChat( player, "Не указан параметр <team>, используйте значение 1-" + UTIL_ToString( m_GetMapNumTeams ) + CommandHelp );
                        }
                        else
                        {
                            SS >> TeamS;

                            if( SS.fail( ) )
                            {
                                if( m_GetMapNumTeams == 0 )
                                    SendChat( player, "Некорректно указан параметр <team>" + CommandHelp );
                                else if( m_GetMapNumTeams == 1 )
                                    SendChat( player, "Некорректно указан параметр <team>, используйте значение \"1\"" + CommandHelp );
                                else
                                    SendChat( player, "Некорректно указан параметр <team>, используйте значение 1-" + UTIL_ToString( m_GetMapNumTeams ) + CommandHelp );
                            }
                            else
                            {
                                if( SS.eof( ) )
                                    SendChat( player, "Не указан параметр <colour>, используйте значение 1-12" + CommandHelp );
                                else
                                {
                                    if( ( ( RaceS == "night" || RaceS == "ночные" ) && ( TeamS == "elves" || TeamS == "elfs" || TeamS == "elfes" || TeamS == "elf" || TeamS == "эльфы" || TeamS == "эльф" || TeamS == "ельфы" || TeamS == "ельф" ) ) || ( ( RaceS == "случайная" || RaceS == "рандом" || RaceS == "random" ) && ( TeamS == "раса" || TeamS == "расса" || TeamS == "race" ) ) )
                                    {
                                        if( RaceS == "night" || RaceS == "ночные" )
                                            RaceS = "nightelfs";
                                        else
                                            RaceS = "random";

                                        TeamS = string( );
                                        SS >> TeamS;

                                        if( SS.fail( ) )
                                        {
                                            if( m_GetMapNumTeams == 0 )
                                                SendChat( player, "Некорректно указан параметр <team>" + CommandHelp );
                                            else if( m_GetMapNumTeams == 1 )
                                                SendChat( player, "Некорректно указан параметр <team>, используйте значение \"1\"" + CommandHelp );
                                            else
                                                SendChat( player, "Некорректно указан параметр <team>, используйте значение 1-" + UTIL_ToString( m_GetMapNumTeams ) + CommandHelp );

                                            return HideCommand;
                                        }
                                        else if( SS.eof( ) )
                                        {
                                            SendChat( player, "Не указан параметр <colour>, используйте значение 1-12" + CommandHelp );
                                            return HideCommand;
                                        }
                                    }

                                    SS >> ColourS;

                                    if( SS.fail( ) )
                                        SendChat( player, "Некорректно указан параметр <colour>, используйте значение 1-12" + CommandHelp );
                                    else
                                    {
                                        if( SS.eof( ) )
                                            HandicapS = "100";
                                        else
                                        {
                                            SS >> HandicapS;

                                            if( SS.fail( ) )
                                                HandicapS = "100";
                                            else
                                            {
                                                if( HandicapS == "зеленый" || HandicapS == "зелёный" || HandicapS == "синий" || HandicapS == "green" || HandicapS == "blue" )
                                                {
                                                    if( ( HandicapS == "зеленый" || HandicapS == "зелёный" || HandicapS == "green" ) && ( ColourS == "темно" || ColourS == "тёмно" || ColourS == "dark" ) )
                                                        ColourS = "темно-зеленый";
                                                    else if( ( HandicapS == "синий" || HandicapS == "blue" ) && ( ColourS == "светло" || ColourS == "light" ) )
                                                        ColourS = "светло-синий";

                                                    if( SS.eof( ) )
                                                        HandicapS = "100";
                                                    else
                                                    {
                                                        HandicapS = string( );
                                                        SS >> HandicapS;

                                                        if( SS.fail( ) )
                                                            HandicapS = "100";
                                                    }
                                                }
                                            }
                                        }

                                        uint32_t Race;
                                        uint32_t Team = UTIL_ToUInt32( TeamS );
                                        uint32_t Colour;
                                        uint32_t Handicap;

                                        if( m_Slots.empty( ) || Slot == 0 || Slot > m_Slots.size( ) )
                                        {
                                            if( m_Slots.size( ) == 1 )
                                                SendChat( player, "Некорректно указан номер слота, используйте значение \"1\"" + CommandHelp );
                                            else
                                                SendChat( player, "Некорректно указан номер слота, используйте значение 1-" + UTIL_ToString( m_Slots.size( ) ) + CommandHelp );

                                            return HideCommand;
                                        }

                                        if( RaceS == "0" || RaceS == "random" || RaceS == "рандом" || RaceS == "случайная" || RaceS == "случайно" )
                                            Race = 0;
                                        else if( RaceS == "1" || RaceS == "альянс" || RaceS == "люди" || RaceS == "хуман" || RaceS == "хуманы" || RaceS == "human" || RaceS == "humans" || RaceS == "alliance" || RaceS == "aliance" )
                                            Race = 1;
                                        else if( RaceS == "2" || RaceS == "орда" || RaceS == "орки" || RaceS == "орк" || RaceS == "orc" || RaceS == "orcs" || RaceS == "horde" )
                                            Race = 2;
                                        else if( RaceS == "3" || RaceS == "нежить" || RaceS == "андид" || RaceS == "плеть" || RaceS == "мертвецы" || RaceS == "мертвые" || RaceS == "undead" || RaceS == "dead" )
                                            Race = 3;
                                        else if( RaceS == "4" || RaceS == "5" || RaceS == "ночныеэльфы" || RaceS == "ночные-эльфы" || RaceS == "ночныеельфы" || RaceS == "ночные-ельфы" || RaceS == "эльфы" || RaceS == "ельфы" || RaceS == "эльф" || RaceS == "ельф" || RaceS == "ночные" || RaceS == "ночной" || RaceS == "nightelves" || RaceS == "nightelfs" || RaceS == "nightelfes" || RaceS == "nightelf" || RaceS == "night-elves" || RaceS == "night-elfs" || RaceS == "night-elfes" || RaceS == "night-elf" || RaceS == "night"  || RaceS == "elves" || RaceS == "elfs" || RaceS == "elfes" || RaceS == "elf" )
                                            Race = 4;
                                        else
                                        {
                                            SendChat( player, "Некорректно указан параметр <race>, используйте значение 0-4" + CommandHelp );
                                            return HideCommand;
                                        }

                                        if( Team == 0 || Team > m_GetMapNumTeams )
                                        {
                                            if( m_GetMapNumTeams == 0 )
                                                SendChat( player, "Некорректно указан параметр <team>" + CommandHelp );
                                            else if( m_GetMapNumTeams == 1 )
                                                SendChat( player, "Некорректно указан параметр <team>, используйте значение \"1\"" + CommandHelp );
                                            else
                                                SendChat( player, "Некорректно указан параметр <team>, используйте значение 1-" + UTIL_ToString( m_GetMapNumTeams ) + CommandHelp );

                                            return HideCommand;
                                        }

                                        if( ColourS == "red" || ColourS == "красный" || ColourS == "0" || ColourS == "1" )
                                            Colour = 1;
                                        else if( ColourS == "blue" || ColourS == "синий" || ColourS == "2" )
                                            Colour = 2;
                                        else if( ColourS == "teal" || ColourS == "бирюзовый" || ColourS == "голубой" || ColourS == "3" )
                                            Colour = 3;
                                        else if( ColourS == "purple" || ColourS == "фиолетовый" || ColourS == "4" )
                                            Colour = 4;
                                        else if( ColourS == "yellow" || ColourS == "желтый" || ColourS == "жёлтый" || ColourS == "5" )
                                            Colour = 5;
                                        else if( ColourS == "orange" || ColourS == "оранжевый" || ColourS == "6" )
                                            Colour = 6;
                                        else if( ColourS == "green" || ColourS == "зеленый" || ColourS == "зелёный" || ColourS == "7" )
                                            Colour = 7;
                                        else if( ColourS == "pink" || ColourS == "розовый" || ColourS == "8" )
                                            Colour = 8;
                                        else if( ColourS == "gray" || ColourS == "серый" || ColourS == "grey" || ColourS == "9" )
                                            Colour = 9;
                                        else if( ColourS == "light-blue" || ColourS == "светло-синий" || ColourS == "lightblue" || ColourS == "светлосиний" || ColourS == "10" )
                                            Colour = 10;
                                        else if( ColourS == "dark-green" || ColourS == "темно-зеленый" || ColourS == "тёмно-зеленый" || ColourS == "темно-зелёный" || ColourS == "тёмно-зелёный" || ColourS == "darkgreen" || ColourS == "темнозеленый" || ColourS == "тёмнозеленый" || ColourS == "темнозелёный" || ColourS == "тёмнозелёный" || ColourS == "11" )
                                            Colour = 11;
                                        else if( ColourS == "brown" || ColourS == "коричневый" || ColourS == "12" )
                                            Colour = 12;
                                        else
                                        {
                                            SendChat( player, "Некорректно указан параметр <colour>, используйте значение 1-12" + CommandHelp );
                                            return HideCommand;
                                        }

                                        if( HandicapS == "100" )
                                            Handicap = 100;
                                        else if( HandicapS == "90" )
                                            Handicap = 90;
                                        else if( HandicapS == "80" )
                                            Handicap = 80;
                                        else if( HandicapS == "70" )
                                            Handicap = 70;
                                        else if( HandicapS == "60" )
                                            Handicap = 60;
                                        else if( HandicapS == "50" )
                                            Handicap = 50;
                                        else
                                        {
                                            SendChat( player, "Некорректно указан параметр <handicap>, используйте значение 50/60/70/80/90/100" + CommandHelp );
                                            return HideCommand;
                                        }

                                        unsigned char CSlot = static_cast<unsigned char>( Slot - 1 );
                                        unsigned char CRace;
                                        unsigned char CTeam = static_cast<unsigned char>( Team - 1 );
                                        unsigned char CColour = static_cast<unsigned char>( Colour - 1 );
                                        unsigned char CHandicap = static_cast<unsigned char>( Handicap );

                                        if( Race == 1 )
                                            CRace = m_UNM->m_PlayersCanChangeRace ? 65 : SLOTRACE_HUMAN;
                                        else if( Race == 2 )
                                            CRace = m_UNM->m_PlayersCanChangeRace ? 66 : SLOTRACE_ORC;
                                        else if( Race == 4 )
                                            CRace = m_UNM->m_PlayersCanChangeRace ? 68 : SLOTRACE_NIGHTELF;
                                        else if( Race == 3 )
                                            CRace = m_UNM->m_PlayersCanChangeRace ? 72 : SLOTRACE_UNDEAD;
                                        else
                                            CRace = m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM;

                                        if( !(m_Map->GetMapFlags( ) & MAPFLAG_RANDOMRACES) )
                                        {
                                            SendChat( player, "Слот изменен!" );
                                            m_Slots[CSlot].SetRace( CRace );
                                        }
                                        else
                                            SendChat( player, "Слот изменен, кроме параметра <race> (конфиг данной карты запрещает игрокам выбирать расу!)" );

                                        m_Slots[CSlot].SetTeam( CTeam );
                                        m_Slots[CSlot].SetHandicap( CHandicap );
                                        ColourSlot( CSlot, CColour, true );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        //
        // !DOWNLOAD
        // !DL
        //

        else if( ( Command == "download" || Command == "dl" ) && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
        {
            CGamePlayer *LastMatch = nullptr;
            uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

            if( Matches == 0 )
                SendAllChat( gLanguage->UnableToStartDownloadNoMatchesFound( Payload ) );
            else if( Matches == 1 )
            {
                if( !LastMatch->GetDownloadStarted( ) && !LastMatch->GetDownloadFinished( ) )
                {
                    unsigned char SID = GetSIDFromPID( LastMatch->GetPID( ) );

                    if( SID < m_Slots.size( ) && m_Slots[SID].GetDownloadStatus( ) != 100 )
                    {
                        // inform the client that we are willing to send the map

                        CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] map download started for player [" + LastMatch->GetName( ) + "]", 6, m_CurrentGameCounter );
                        Send( LastMatch, m_Protocol->SEND_W3GS_STARTDOWNLOAD( GetHostPID( ) ) );
                        LastMatch->SetDownloadAllowed( true );
                        LastMatch->SetDownloadStarted( true );
                        LastMatch->SetStartedDownloadingTicks( GetTicks( ) );
                    }
                }
            }
            else
                SendAllChat( gLanguage->UnableToStartDownloadFoundMoreThanOneMatch( Payload ) );
        }

        //
        // !DLINFO
        // !DLI
        //

        else if( Command == "dlinfo" || Command == "dli" )
        {
            if( Payload.empty( ) )
                m_UNM->m_ShowDownloadsInfo = !m_UNM->m_ShowDownloadsInfo;
            else if( PayloadLower == "on" || PayloadLower == "1" || PayloadLower == "enable" || PayloadLower == "on" )
                m_UNM->m_ShowDownloadsInfo = true;
            else if( PayloadLower == "off" || PayloadLower == "0" || PayloadLower == "disable" || PayloadLower == "off" )
                m_UNM->m_ShowDownloadsInfo = false;
            else
                m_UNM->m_ShowDownloadsInfo = !m_UNM->m_ShowDownloadsInfo;

            if( m_UNM->m_ShowDownloadsInfo )
                SendAllChat( "Show Downloads Info = ON" );
            else
                SendAllChat( "Show Downloads Info = OFF" );
        }

        //
        // !DLINFOTIME
        // !DLIT
        //

        else if( ( Command == "dlinfotime" || Command == "dlit" ) && !Payload.empty( ) )
        {
            uint32_t itime = m_UNM->m_ShowDownloadsInfoTime;
            m_UNM->m_ShowDownloadsInfoTime = UTIL_ToUInt32( Payload );
            SendAllChat( "Show Downloads Info Time = " + Payload + " s, previously " + UTIL_ToString( itime ) + " s" );
        }

        //
        // !DLTSPEED
        // !DLTS
        //

        else if( ( Command == "dltspeed" || Command == "dlts" ) && !Payload.empty( ) )
        {
            uint32_t tspeed = m_UNM->m_totaldownloadspeed;

            if( PayloadLower == "max" )
                m_UNM->m_totaldownloadspeed = 0;
            else
                m_UNM->m_totaldownloadspeed = UTIL_ToUInt32( Payload );

            SendAllChat( "Total download speed = " + Payload + " KB/s, previously " + UTIL_ToString( tspeed ) + " KB/s" );
        }

        //
        // !DLSPEED
        // !DLS
        //

        else if( ( Command == "dlspeed" || Command == "dls" ) && !Payload.empty( ) )
        {
            uint32_t tspeed = m_UNM->m_clientdownloadspeed;

            if( PayloadLower == "max" )
                m_UNM->m_clientdownloadspeed = 0;
            else
                m_UNM->m_clientdownloadspeed = UTIL_ToUInt32( Payload );

            SendAllChat( "Maximum player download speed = " + Payload + " KB/s, previously " + UTIL_ToString( tspeed ) + " KB/s" );
        }

        //
        // !DLMAX
        // !DLM
        //

        else if( ( Command == "dlmax" || Command == "dlm" ) && !Payload.empty( ) )
        {
            uint32_t t = m_UNM->m_maxdownloaders;

            if( PayloadLower == "max" )
                m_UNM->m_maxdownloaders = 0;
            else
                m_UNM->m_maxdownloaders = UTIL_ToUInt32( Payload );

            SendAllChat( "Maximum concurrent downloads = " + Payload + ", previously " + UTIL_ToString( t ) );
        }

        //
        // !DROP
        // !ДРОП
        // !ДРОПНУТЬ
        // !ВЫКИНУТЬ
        // !КИК
        // !КИКНУТЬ
        //

        else if( ( ( Command == "drop" || Command == "дроп" || Command == "дропнуть" ) || ( ( Command == "выкинуть" || Command == "кик" || Command == "кикнуть" ) && Payload.empty( ) ) ) && m_GameLoaded )
        {
            StopLaggers( "lagged out (dropped by admin)" );
        }

        //
        // !DROPIFDESYNC
        // !DD
        // !DID
        // !DROPDESYNC
        // !DESYNCDROP
        // !DESYNC
        // !DKD
        //

        else if( Command == "dropifdesync" || Command == "dd" || Command == "did" || Command == "dropdesync" || Command == "desyncdrop" || Command == "desync" || Command == "dkd" )
        {
            if( Payload.empty( ) )
                m_DropIfDesync = !m_DropIfDesync;
            else if( PayloadLower == "on" || PayloadLower == "1" || PayloadLower == "enable" || PayloadLower == "on" )
                m_DropIfDesync = true;
            else if( PayloadLower == "off" || PayloadLower == "0" || PayloadLower == "disable" || PayloadLower == "off" )
                m_DropIfDesync = false;
            else
                m_DropIfDesync = !m_DropIfDesync;

            if( m_DropIfDesync )
                SendAllChat( "В случае десинхронизации игроки будут кикнуты!" );
            else
                SendAllChat( "В случае десинхронизации игроки не будут кикнуты!" );
        }

        //
        // !ENDN
        //

        else if( Command == "endn" && m_GameLoaded )
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] игра была завершена администратором", 6, m_CurrentGameCounter );
            StopPlayers( "был отключен (игра была завершена администратором)" );
            return HideCommand;
        }

        //
        // !ENDS
        // !A
        // !ОТМЕНА
        //

        else if( ( Command == "ends" || Command == "a" || Command == "отмена" ) && m_GameLoaded )
        {
            if( m_GameEndCountDownStarted )
            {
                m_GameEndCountDownStarted = false;
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] завершение игры отменено администратором", 6, m_CurrentGameCounter );
                SendAllChat( "Админ отменил завершение игры" );
            }
        }

        //
        // !END
        // !ЗАВЕРШИТЬ
        // !ЗАКОНЧИТЬ
        // !ЗАВЕРШЕНИЕ
        // !КОНЕЦ
        //

        else if( ( Command == "end" || Command == "завершить" || Command == "закончить" || Command == "конец" ) && m_GameLoaded )
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] игра будет завершена через 5 секунд администратором", 6, m_CurrentGameCounter );
            SendAllChat( "Игра завершится через 5 секунд" );
            m_GameEndCountDownStarted = true;
            m_GameEndCountDownCounter = 5;
            m_GameEndLastCountDownTicks = GetTicks( );
            return HideCommand;
        }

        //
        // !REPLAYS
        // !NOREPLAYS
        // !SAVEREPLAYS
        // !REPLAYSSAVE
        //

        else if( ( Command == "replays" || Command == "noreplays" || Command == "savereplays" || Command == "replayssave" ) && !m_GameLoaded && !m_GameLoading )
        {
            if( m_GameLoaded || m_GameLoading )
            {
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна в начатой игре!" );
                return HideCommand;
            }

            if( m_SaveGame )
            {
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна в сохраненной игре!" );
                return HideCommand;
            }

            m_SaveReplays = !m_SaveReplays;

            if( m_SaveReplays )
            {
                m_Replay = new CReplay( );
                SendAllChat( "Replay save enabled" );
            }
            else
            {
                delete m_Replay;
                m_Replay = nullptr;
                SendAllChat( "Replay save disabled" );
            }
        }

        //
        // !OBSPLAYER
        // !OBSERVER
        // !OBS
        // !OP
        // !REFEREE
        //

        else if( ( Command == "obsplayer" || Command == "observer" || Command == "obs" || Command == "op" || Command == "referee" ) && !m_CountDownStarted )
        {
            if( m_ObsPlayerPID == 255 )
                CreateObsPlayer( Payload );
            else
                DeleteObsPlayer( );
        }

        //
        // !OPPAUSE
        // !OPP
        //

        else if( ( Command == "oppause" || Command == "opp" ) && m_ObsPlayerPID != 255 && !m_Pause && !m_PauseEnd && !m_GameLoading && m_GameLoaded && !m_Lagging )
        {
            BYTEARRAY CRC;
            BYTEARRAY Action;
            Action.push_back( 1 );
            m_Actions.push( new CIncomingAction( m_ObsPlayerPID, CRC, Action ) );
        }

        //
        // !OPRESUME
        // !OPR
        //

        else if( ( Command == "opresume" || Command == "opr" ) && m_ObsPlayerPID != 255 && !m_Pause && !m_PauseEnd && !m_GameLoading && m_GameLoaded && !m_Lagging )
        {
            BYTEARRAY CRC;
            BYTEARRAY Action;
            Action.push_back( 2 );
            m_Actions.push( new CIncomingAction( m_ObsPlayerPID, CRC, Action ) );
        }

        //
        // !PAUSE
        // !ПАУЗА
        // !СТОП
        //

        else if( Command == "pause" || Command == "пауза" || Command == "стоп" )
        {
            if( !m_Pause && !m_PauseEnd && !m_GameLoading && m_GameLoaded && !m_Lagging )
            {
                m_Pause = true;
                m_PauseEnd = false;
                SendAllChat( "Пауза поставлена!" );
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
            else
                SendAllChat( "Сейчас невозможно поставить паузу!" );
        }

        //
        // !RESUME
        // !UNPAUSE
        // !ПРОДОЛЖИТЬ
        // !ВОЗОБНОВИТЬ
        //

        else if( ( Command == "resume" || Command == "unpause" || Command == "продолжить" || Command == "возобновить" ) && m_Pause && !m_PauseEnd && !m_GameLoading && m_GameLoaded )
        {
            m_PauseEnd = true;
            SendAllChat( "Пауза снята!" );
        }

        //
        // !FAKEPLAYER
        // !ADDFAKEPLAYER
        // !FAKEPLAYERADD
        // !ADDFAKE
        // !FAKEADD
        // !FPADD
        // !FAKE
        // !FPA
        // !FP
        // !AF
        //

        else if( ( Command == "fakeplayer" || Command == "addfakeplayer" || Command == "fakeplayeradd" || Command == "addfake" || Command == "fakeadd" || Command == "fpadd" || Command == "fake" || Command == "fpa" || Command == "fp" || Command == "af" ) && !m_CountDownStarted )
            CreateFakePlayer( Payload );

        //
        // !DELETEFAKEPLAYER
        // !FAKEPLAYERDETELE
        // !DF
        // !FPDEL
        // !FPDELETE
        // !FPREMOVE
        // !FPKICK
        // !DELFP
        // !DELETEFP
        // !REMOVEFP
        // !KICKFP
        // !FPD
        // !FPR
        // !FPK
        // !DFP
        // !RFP
        // !KFP
        //

        else if( ( Command == "deletefakeplayer" || Command == "fakeplayerdelete" || Command == "df" || Command == "fpdel" || Command == "fpdelete" || Command == "fpremove" || Command == "fpkick" || Command == "delfp" || Command == "deletefp" || Command == "removefp" || Command == "kickfp" || Command == "fpd" || Command == "fpr" || Command == "fpk" || Command == "dfp" || Command == "rfp" || Command == "kfp" ) && m_FakePlayers.size( ) >= 1 && !m_GameLoading && ( ( !m_CountDownStarted && !m_GameLoaded ) || m_GameLoaded ) )
            DeleteAFakePlayer( );

        //
        // !DELETEFAKEPLAYERS
        // !FAKEPLAYERSDETELE
        // !DFS
        // !FPSDEL
        // !FPSDELETE
        // !FPSREMOVE
        // !FPSKICK
        // !DELFPS
        // !DELETEFPS
        // !REMOVEFPS
        // !KICKFPS
        // !FPSD
        // !FPSR
        // !FPSK
        // !DFPS
        // !RFPS
        // !KFPS
        // !FAKESDEL
        // !FAKESDELETE
        // !FAKESREMOVE
        // !FAKESKICK
        // !DELFAKES
        // !DELETEFAKES
        // !REMOVEFAKES
        // !KICKFAKES
        // !FAKESD
        // !FAKESR
        // !FAKESK
        // !DFAKES
        // !RFAKES
        // !KFAKES
        //

        else if( ( Command == "deletefakeplayers" || Command == "fakeplayersdelete" || Command == "dfs" || Command == "fpsdel" || Command == "fpsdelete" || Command == "fpsremove" || Command == "fpskick" || Command == "delfps" || Command == "deletefps" || Command == "removefps" || Command == "kickfps" || Command == "fpsd" || Command == "fpsr" || Command == "fpsk" || Command == "dfps" || Command == "rfps" || Command == "kfps" || Command == "fakesdel" || Command == "fakesdelete" || Command == "fakesremove" || Command == "fakeskick" || Command == "delfakes" || Command == "deletefakes" || Command == "removefakes" || Command == "kickfakes" || Command == "fakesd" || Command == "fakesr" || Command == "fakesk" || Command == "dfakes" || Command == "rfakes" || Command == "kfakes" ) && m_FakePlayers.size( ) >= 1 && !m_GameLoading && ( ( !m_CountDownStarted && !m_GameLoaded ) || m_GameLoaded ) )
            DeleteFakePlayer( );

        //
        // !FPPAUSE
        // !FPP
        //

        else if( ( Command == "fppause" || Command == "fpp" ) && m_FakePlayers.size( ) >= 1 && !m_Pause && !m_PauseEnd && !m_GameLoading && m_GameLoaded && !m_Lagging )
        {
            BYTEARRAY CRC, Action;
            Action.push_back( 1 );
            m_Actions.push( new CIncomingAction( m_FakePlayers[UTIL_GenerateRandomNumberUInt32( 0, m_FakePlayers.size( ) - 1 )], CRC, Action ) );
        }

        //
        // !FPRESUME
        // !FPR
        //

        else if( ( Command == "fpresume" || Command == "fpr" ) && m_FakePlayers.size( ) >= 1 && !m_Pause && !m_PauseEnd && !m_GameLoading && m_GameLoaded && !m_Lagging )
        {
            BYTEARRAY CRC, Action;
            Action.push_back( 2 );
            m_Actions.push( new CIncomingAction( m_FakePlayers[0], CRC, Action ) );
        }

        //
        // !FPCONTROL
        // !FPCONTROLL
        // !FPC
        //

        else if( ( Command == "fpcontrol" || Command == "fpcontroll" || Command == "fpc" ) && m_FakePlayers.size( ) >= 1 && m_GameLoaded )
        {
            if( Payload.empty( ) )
            {
                SendChat( player->GetPID( ), "Некорректный ввод! Юзай: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <FakePlayerNumbers/all>" );
                return HideCommand;
            }

            uint32_t testsize = 0;
            uint32_t userslot = 0;

            for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
            {
                if( i->GetPID( ) == player->GetPID( ) )
                {
                    userslot = testsize;
                    break;
                }
                testsize++;
            }

            if( userslot != testsize )
            {
                SendChat( player->GetPID( ), "Ошибка при передаче контроля!" );
                return HideCommand;
            }

            BYTEARRAY CRC, Action;
            bool ok = false;
            bool nonexistent = false;
            string numbers = string( );
            Action = UTIL_ExtractHexNumbers( "50 " + UTIL_ToString( userslot ) + " 7f 0 0 0" );

            if( PayloadLower == "all" || PayloadLower == "0" )
            {
                for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
                    m_Actions.push( new CIncomingAction( m_FakePlayers[i], CRC, Action ) );

                ok = true;
            }
            else
            {
                string testsid = string( );
                stringstream SS;
                SS << Payload;

                while( !SS.eof( ) )
                {
                    uint32_t SID;
                    SS >> SID;

                    if( SS.fail( ) )
                    {
                        SendChat( player, "bad input to [" + Command + "] command" );
                        break;
                    }
                    else if( SID != 0 && SID - 1 < m_FakePlayers.size( ) )
                    {
                        if( testsid.find( "@" + UTIL_ToString( SID ) + "@" ) == string::npos )
                        {
                            m_Actions.push( new CIncomingAction( m_FakePlayers[SID - 1], CRC, Action ) );
                            ok = true;
                            numbers += UTIL_ToString( SID ) + ",";
                            testsid += "@" + UTIL_ToString( SID ) + "@ ";
                        }
                    }
                    else
                        nonexistent = true;
                }
            }

            if( ok && ( numbers.empty( ) || numbers == "1,2,3,4,5,6,7,8,9,10,11," ) )
                SendChat( player->GetPID( ), "Запрос на передачу контроля отправлен всем фейк-плеерам!" );
            else if( ok && numbers.size( ) > 3 )
                SendChat( player->GetPID( ), "Запрос на передачу контроля отправлен фейк-плеерам №" + numbers.substr( 0, numbers.size( ) - 1 ) );
            else if( ok && numbers.size( ) > 1 )
                SendChat( player->GetPID( ), "Запрос на передачу контроля отправлен фейк-плееру №" + numbers.substr( 0, numbers.size( ) - 1 ) );
            else if( nonexistent )
                SendChat( player->GetPID( ), "Указан несуществующий номер фейк-плеера!" );
            else
                SendChat( player->GetPID( ), "Некорректный ввод! Юзай: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <FakePlayerNumbers/all>" );
        }

        //
        // !FPSHARE
        // !FPS
        // !FPSHARECONTROL
        // !FPSHARECONTROLL
        // !FPSC
        // !FPCONTROLSHARE
        // !FPCONTROLLSHARE
        // !FPCS
        //

        else if( ( Command == "fpshare" || Command == "fps" || Command == "fpsharecontrol" || Command == "fpsharecontroll" || Command == "fpsc" || Command == "fpcontrolshare" || Command == "fpcontrollshare" || Command == "fpcs" ) && m_FakePlayers.size( ) >= 1 && m_GameLoaded )
        {
            string FP;
            uint32_t FPNumber = 0;
            bool AllFP = false;
            string PLAYER;
            bool AllPLAYER = false;
            uint32_t Matches = 0;
            CGamePlayer *LastMatch = nullptr;
            stringstream SS;
            SS << PayloadLower;
            SS >> FP;

            if( PayloadLower == "all" || PayloadLower == "all all" || PayloadLower == "0" || PayloadLower == "0 0" )
            {
                AllFP = true;
                AllPLAYER = true;
            }
            else if( Payload.empty( ) || SS.fail( ) || SS.eof( ) )
            {
                SendChat( player->GetPID( ), "Некорректный ввод! Юзай: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <FakePlayer(№/all)> <player(name/all)>" );
                return HideCommand;
            }
            else
            {
                SS >> PLAYER;

                if( SS.fail( ) || FP.find_first_not_of( " " ) == string::npos || PLAYER.find_first_not_of( " " ) == string::npos )
                {
                    SendChat( player->GetPID( ), "Некорректный ввод! Юзай: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <FakePlayer(№/all)> <player(name/all)>" );
                    return HideCommand;
                }

                if( FP == "all" || FP == "0" )
                    AllFP = true;
                else
                {
                    FPNumber = UTIL_ToUInt32( FP );

                    if( FPNumber == 0 )
                    {
                        SendChat( player->GetPID( ), "Некорректный ввод! Юзай: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <FakePlayer(№/all)> <resources(1-4294967295/all)>" );
                        return HideCommand;
                    }

                    FPNumber = FPNumber - 1;

                    if( FPNumber >= m_FakePlayers.size( ) )
                    {
                        SendChat( player->GetPID( ), "Указан несуществующий номер фейк-плеера!" );
                        return HideCommand;
                    }
                }

                if( PLAYER == "all" || PLAYER == "0" )
                    AllPLAYER = true;
                else
                {
                    if( PLAYER.size( ) < 3 && UTIL_ToUInt32( PLAYER ) > 0 && UTIL_ToUInt32( PLAYER ) < 13 )
                    {
                        LastMatch = GetPlayerFromSID( static_cast<unsigned char>( UTIL_ToUInt32( PLAYER ) - 1 ) );

                        if( LastMatch )
                            Matches = GetPlayerFromNamePartial( LastMatch->GetName( ), &LastMatch );
                        else
                        {
                            SendChat( player->GetPID( ), "Слот [" + PLAYER + "] пуст" );
                            return HideCommand;
                        }
                    }
                    else
                        Matches = GetPlayerFromNamePartial( PLAYER, &LastMatch );

                    if( Matches == 0 )
                    {
                        SendChat( player->GetPID( ), "По запросу \"" + PLAYER + "\" игроков не найдено." );
                        return HideCommand;
                    }
                    else if( Matches != 1 )
                    {
                        SendChat( player->GetPID( ), "По запросу \"" + PLAYER + "\" найдено несколько игроков, уточните запрос." );
                        return HideCommand;
                    }
                }
            }

            BYTEARRAY CRC, Action;
            uint32_t userslot = 0;
            CGamePlayer *Player = nullptr;
            Player = GetPlayerFromPID( m_LastPlayerJoined );

            if( Player == nullptr )
                Player = GetPlayerFromSID( static_cast<unsigned char>( UTIL_ToUInt32( PLAYER ) - 1 ) );

            for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
            {
                CGamePlayer *Player = nullptr;
                Player = GetPlayerFromSID( static_cast<unsigned char>(userslot) );

                if( Player != nullptr && ( AllPLAYER || ( LastMatch != nullptr && i->GetPID( ) == LastMatch->GetPID( ) ) ) )
                {
                    Action = UTIL_ExtractHexNumbers( "50 " + UTIL_ToString( userslot ) + " 7f 0 0 0" );

                    for( uint32_t j = 0; j < m_FakePlayers.size( ); j++ )
                    {
                        if( AllFP || j == FPNumber )
                            m_Actions.push( new CIncomingAction( m_FakePlayers[j], CRC, Action ) );
                    }
                }

                userslot++;
            }

            if( AllPLAYER )
            {
                if( AllFP )
                    SendChat( player->GetPID( ), "Зaпpoc нa пepeдaчу кoнтpoля вceм игpoкaм oтпpaвлeн вceм фeйкплeepaм." );
                else
                    SendChat( player->GetPID( ), "Зaпpoc нa пepeдaчу кoнтpoля вceм игpoкaм oтпpaвлeн фeйкплeepу №" + UTIL_ToString( FPNumber + 1 ) );
            }
            else if( LastMatch != nullptr )
            {
                if( AllFP )
                    SendChat( player->GetPID( ), "Зaпpoc нa пepeдaчу кoнтpoля игpoку [" + LastMatch->GetName( ) + "] oтпpaвлeн вceм фeйкплeepaм." );
                else
                    SendChat( player->GetPID( ), "Зaпpoc нa пepeдaчу кoнтpoля игpoку [" + LastMatch->GetName( ) + "] oтпpaвлeн фeйкплeepу №" + UTIL_ToString( FPNumber + 1 ) );
            }
            else
            {
                if( AllFP )
                    SendChat( player->GetPID( ), "Зaпpoc нa пepeдaчу кoнтpoля игpoку oтпpaвлeн вceм фeйкплeepaм." );
                else
                    SendChat( player->GetPID( ), "Зaпpoc нa пepeдaчу кoнтpoля игpoку oтпpaвлeн фeйкплeepу №" + UTIL_ToString( FPNumber + 1 ) );
            }
        }

        //
        // !FPTRANSFERRESOURCES
        // !FPTRADERESOURCES
        // !FPTR
        // !FPTRANSFER
        // !FPTRADE
        // !FPT
        // !FPTRANSFERGOLD
        // !FPTRADEGOLD
        // !FPTG
        // !FPTRANSFERLUMBER
        // !FPTRADELUMBER
        // !FPTL
        //

        else if( ( Command == "fptransferresources" || Command == "fptraderesources" || Command == "fptr" || Command == "fptransfer" || Command == "fptrade" || Command == "fpt" || Command == "fptransfergold" || Command == "fptradegold" || Command == "fptg" || Command == "fptransferlumber" || Command == "fptradelumber" || Command == "fptl" ) && m_FakePlayers.size( ) >= 1 && m_GameLoaded )
        {
            string FP;
            uint32_t FPNumber = 0;
            bool AllFP = false;
            string RS;
            uint64_t Resources;
            stringstream SS;
            SS << PayloadLower;
            SS >> FP;

            if( PayloadLower == "all" || PayloadLower == "all all" || PayloadLower == "0" || PayloadLower == "0 0" )
            {
                AllFP = true;
                Resources = 4294967295;
            }
            else if( Payload.empty( ) || SS.fail( ) || SS.eof( ) )
            {
                SendChat( player->GetPID( ), "Некорректный ввод! Юзай: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <FakePlayer(№/all)> <resources(1-4294967295/all)>" );
                return HideCommand;
            }
            else
            {
                SS >> RS;

                if( SS.fail( ) || FP.find_first_not_of( " " ) == string::npos || RS.find_first_not_of( " " ) == string::npos )
                {
                    SendChat( player->GetPID( ), "Некорректный ввод! Юзай: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <FakePlayer(№/all)> <resources(1-4294967295/all)>" );
                    return HideCommand;
                }

                if( FP == "all" || FP == "0" )
                    AllFP = true;
                else
                {
                    FPNumber = UTIL_ToUInt32( FP );
                    if( FPNumber == 0 )
                    {
                        SendChat( player->GetPID( ), "Некорректный ввод! Юзай: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <FakePlayer(№/all)> <resources(1-4294967295/all)>" );
                        return HideCommand;
                    }

                    FPNumber = FPNumber - 1;

                    if( FPNumber >= m_FakePlayers.size( ) )
                    {
                        SendChat( player->GetPID( ), "Указан несуществующий номер фейк-плеера!" );
                        return HideCommand;
                    }
                }

                if( RS == "all" || RS == "0" )
                    Resources = 4294967295;
                else
                {
                    Resources = UTIL_ToUInt64( RS );
                    if( Resources == 0 )
                    {
                        SendChat( player->GetPID( ), "Некорректный ввод! Юзай: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <FakePlayer(№/all)> <resources(1-4294967295/all)>" );
                        return HideCommand;
                    }
                    else if( Resources < 1 || Resources > 4294967295 )
                    {
                        SendChat( player->GetPID( ), "Некорректный ввод! Юзай: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <FakePlayer(№/all)> <resources(1-4294967295/all)>" );
                        return HideCommand;
                    }
                }
            }

            uint32_t testsize = 0;
            uint32_t userslot = 0;

            for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
            {
                if( i->GetPID( ) == player->GetPID( ) )
                {
                    userslot = testsize;
                    break;
                }
                testsize++;
            }

            if( userslot != testsize )
            {
                SendChat( player->GetPID( ), "Ошибка при передаче ресурсов!" );
                return HideCommand;
            }

            uint32_t resources = (uint32_t)Resources;
            string temp;
            stringstream STS;
            STS << std::hex << resources;
            STS >> temp;
            string result = temp;

            if( result.size( ) == 1 || result.size( ) == 2 )
                result = result + " 0 0 0";
            else if( result.size( ) == 3 )
                result = result.substr( 1 ) + " " + result.substr( 0, 1 ) + " 0 0";
            else if( result.size( ) == 4 )
                result = result.substr( 2 ) + " " + result.substr( 0, 2 ) + " 0 0";
            else if( result.size( ) == 5 )
                result = result.substr( 3 ) + " " + result.substr( 1, 2 ) + " " + result.substr( 0, 1 ) + " 0";
            else if( result.size( ) == 6 )
                result = result.substr( 4 ) + " " + result.substr( 2, 2 ) + " " + result.substr( 0, 2 ) + " 0";
            else if( result.size( ) == 7 )
                result = result.substr( 5 ) + " " + result.substr( 3, 2 ) + " " + result.substr( 1, 2 ) + " " + result.substr( 0, 1 );
            else if( result.size( ) == 8 )
                result = result.substr( 6 ) + " " + result.substr( 4, 2 ) + " " + result.substr( 2, 2 ) + " " + result.substr( 0, 2 );
            else
            {
                SendChat( player->GetPID( ), "Некорректный ввод! Юзай: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <FakePlayer(№/all)> <resources(1-4294967295/all)>" );
                return HideCommand;
            }

            if( Command == "fptransfergold" || Command == "fptradegold" || Command == "fptg" )
                result = "51 " + UTIL_ToString( userslot ) + " " + result + " 0 0 0 0";
            else if( Command == "fptransferlumber" || Command == "fptradelumber" || Command == "fptl" )
                result = "51 " + UTIL_ToString( userslot ) + " 0 0 0 0 " + result;
            else
                result = "51 " + UTIL_ToString( userslot ) + " " + result + " " + result;

            BYTEARRAY CRC, Action;
            Action = UTIL_ExtractHexNumbers( result );
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] resource trade detected by [" + player->GetName( ) + "]", 6, m_CurrentGameCounter );
            SendAllChat( "[" + player->GetName( ) + "] has used resource trade" );

            for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
            {
                if( AllFP || i == FPNumber )
                    m_Actions.push( new CIncomingAction( m_FakePlayers[i], CRC, Action ) );
            }
        }

        //
        // !FPHCL
        // !HCLFP
        // !FPMOD
        // !FPMODE
        // !FPCOMMAND
        // !FAKEPLAYERMODE
        // !FAKEPLAYERCOMMAND
        // !FAKEPLAYERHCL
        // !HCLFAKEPLAYER
        //

        else if( ( Command == "fphcl" || Command == "hclfp" || Command == "fpmod" || Command == "fpmode" || Command == "fpcommand" || Command == "fakeplayermode" || Command == "fakeplayercommand" || Command == "fakeplayerhcl" || Command == "hclfakeplayer" ) && m_FakePlayers.size( ) >= 1 && m_GameLoaded )
        {
            if( !m_FPHCLUser.empty( ) )
            {
                if( player->GetName( ) == m_FPHCLUser )
                {
                    if( m_FPHCLNumber == 12 )
                        SendChat( player, "Напишите прямо сейчас в чат команду карты, которую требуется отправить от имени всех фейк-плееров!" );
                    else
                        SendChat( player, "Напишите прямо сейчас в чат команду карты, которую требуется отправить от имени фейк-плеера №" + UTIL_ToString(m_FPHCLNumber) );

                    if( GetTime( ) - m_FPHCLTime < 44 )
                        SendChat( player, "На это действие у вас осталось " + UTIL_ToString( 45 - ( GetTime( ) - m_FPHCLTime ) ) + " сек." );
                    else
                        SendChat( player, "На это действие у вас осталось 1 секунда." );
                }
                else if( GetTime( ) - m_FPHCLTime < 44 )
                    SendChat( player, "Сейчас команда недоступна, подождите пока [" + m_FPHCLUser + "] укажет мод для фейк-плеера (осталось " + UTIL_ToString( 45 - ( GetTime( ) - m_FPHCLTime ) ) + " сек.)" );
                else
                    SendChat( player, "Сейчас команда недоступна, подождите пока [" + m_FPHCLUser + "] укажет мод для фейк-плеера (осталось 1 секунда)" );
            }
            else
            {
                string fpnumbers = "1";
                uint32_t fpid = UTIL_ToUInt32( Payload );

                if( m_FakePlayers.size( ) > 1 )
                    fpnumbers += "-" + UTIL_ToString( m_FakePlayers.size( ) );

                if( Payload == "0" || fpid > m_FakePlayers.size( ) )
                    SendChat( player, "Указан несуществующий номер фейк-плеера, используйте значения <" + fpnumbers + "/all>" );
                else if( fpid == 0 && PayloadLower != "all" )
                    SendChat( player, "Укажите номер фейк-плеера, от имени которого требуется ввести команду: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <" + fpnumbers + "/all>" );
                else
                {
                    if( PayloadLower == "all" )
                    {
                        fpnumbers = "всех фейк-плееров!";
                        m_FPHCLNumber = 12;
                    }
                    else
                    {
                        fpnumbers = "фейк-плеера №" + UTIL_ToString(fpid);
                        m_FPHCLNumber = fpid-1;
                    }

                    m_FPHCLUser = User;
                    m_FPHCLTime = GetTime( );
                    SendChat( player, "Напишите прямо сейчас в чат команду карты, которую требуется отправить от имени " + fpnumbers + ". Если вы не хотите чтобы команда карты выполнилась также и от вашего имени, то добавьте в конец или в середину (но не в самое начало) сообщения следующий текст: !@#$% (на это действие у вас 45 сек.)" );
                }
            }
        }

        //
        // !FPCOPY
        // !FPMANUAL
        // !FPPET
        // !FAKEPLAYERCOPY
        // !FAKEPLAYERMANUAL
        // !FAKEPLAYERPET
        //

        else if( ( Command == "fpcopy" || Command == "fpmanual" || Command == "fppet" || Command == "fakeplayercopy" || Command == "fakeplayermanual" || Command == "fakeplayerpet" ) && m_FakePlayers.size( ) >= 1 && m_GameLoaded )
        {
            string fpnumbers = "1";
            uint32_t fpid = UTIL_ToUInt32( Payload );

            if( m_FakePlayers.size( ) > 1 )
                fpnumbers += "-" + UTIL_ToString( m_FakePlayers.size( ) );

            if( Payload == "0" || fpid > m_FakePlayers.size( ) )
            {
                if( m_FakePlayers.size( ) == 1 )
                    SendChat( player, "Указан несуществующий номер фейк-плеера, используйте значение <1>" );
                else
                    SendChat( player, "Указан несуществующий номер фейк-плеера, используйте значения <" + fpnumbers + ">" );
            }
            else if( fpid == 0 )
                SendChat( player, "Укажите номер фейк-плеера, от имени которого требуется управление выбранным юнитом: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <" + fpnumbers + "/all>" );
            else
            {
                for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
                {
                    if( i + 1 == fpid )
                    {
                        if( m_FakePlayersOwner[i].empty( ) || m_FakePlayersOwner[i] == player->GetName( ) )
                        {
                            if( Command.find( "fakeplayer" ) != string::npos )
                            {
                                if( m_FakePlayersOwner[i].empty( ) )
                                    SendChat( player, "Теперь выделите одного юнита, который будет управляться от имени [" + m_FakePlayersNames[i] + "] (для отмены юзай " + string( 1, m_UNM->m_CommandTrigger ) + "fakeplayerpforget <" + UTIL_ToString( fpid ) + ">)" );
                                else
                                    SendChat( player, "Теперь заново выделите юнита, который будет управляться от имени [" + m_FakePlayersNames[i] + "] (для отмены юзай " + string( 1, m_UNM->m_CommandTrigger ) + "fakeplayerforget <" + UTIL_ToString( fpid ) + ">)" );
                            }
                            else
                            {
                                if( m_FakePlayersOwner[i].empty( ) )
                                    SendChat( player, "Теперь выделите одного юнита, который будет управляться от имени [" + m_FakePlayersNames[i] + "] (для отмены юзай " + string( 1, m_UNM->m_CommandTrigger ) + "fpforget <" + UTIL_ToString( fpid ) + ">)" );
                                else
                                    SendChat( player, "Теперь заново выделите юнита, который будет управляться от имени [" + m_FakePlayersNames[i] + "] (для отмены юзай " + string( 1, m_UNM->m_CommandTrigger ) + "fpforget <" + UTIL_ToString( fpid ) + ">)" );
                            }

                            m_FakePlayersOwner[i] = player->GetName( );
                            m_FakePlayersStatus[i] = 1;
                            m_FakePlayersActSelection[i].clear( );
                        }
                        else
                            SendChat( player, "Указанный фейк-плеер уже используется игроком [" + m_FakePlayersOwner[i] + "]" );

                        break;
                    }
                }

                m_FakePlayersCopyUsed = true;
            }
        }

        //
        // !FAKEPLAYERFORGET
        // !FPFORGET
        //

        else if( ( Command == "fakeplayerpforget" || Command == "fpforget" ) && m_FakePlayers.size( ) >= 1 && m_GameLoaded )
        {
            string fpnumbers = "1";

            if( m_FakePlayers.size( ) > 1 )
                fpnumbers += "-" + UTIL_ToString( m_FakePlayers.size( ) );

            if( Payload.find_first_not_of( " " ) == string::npos )
            {
                SendChat( player, "Укажите номер фейк-плеера, от имени которого требуется отменить управление юнитом: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <" + fpnumbers + "/all>" );
                return HideCommand;
            }

            string fpids = string( );
            string nonexistentfp = string( );
            string nonusedfp = string( );
            string usedfp = string( );
            string fplastowner = string( );
            bool fpsomeowners = false;
            bool incorrectfp = false;
            bool fpidsall = false;
            stringstream SS;
            SS << PayloadLower;

            while( !SS.eof( ) )
            {
                string fpidstr;
                SS >> fpidstr;

                if( !SS.fail( ) )
                {
                    uint32_t fpid = UTIL_ToUInt32( fpidstr );

                    if( fpidstr == "0" || fpid > m_FakePlayers.size( ) )
                    {
                        if( nonexistentfp.empty( ) )
                            nonexistentfp = UTIL_ToString( fpid );
                        else if( !UTIL_StringSearch( nonexistentfp, UTIL_ToString( fpid ), 1 ) )
                            nonexistentfp += ", " + UTIL_ToString( fpid );
                    }
                    else if( fpid == 0 && fpidstr != "all" )
                        incorrectfp = true;
                    else
                    {
                        for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
                        {
                            if( i + 1 == fpid || fpidstr == "all" )
                            {
                                if( m_FakePlayersOwner[i] == player->GetName( ) )
                                {
                                    if( fpids.empty( ) )
                                        fpids = UTIL_ToString( fpid );
                                    else if( !UTIL_StringSearch( fpids, UTIL_ToString( fpid ), 1 ) )
                                        fpids += ", " + UTIL_ToString( fpid );

                                    m_FakePlayersOwner[i] = string( );
                                    m_FakePlayersStatus[i] = 0;
                                    m_FakePlayersActSelection[i].clear( );

                                    if( fpidstr != "all" )
                                        break;
                                }
                                else if( fpidstr != "all" )
                                {
                                    if( m_FakePlayersOwner[i].empty( ) )
                                    {
                                        if( nonusedfp.empty( ) )
                                            nonusedfp = UTIL_ToString( fpid );
                                        else if( !UTIL_StringSearch( nonusedfp, UTIL_ToString( fpid ), 1 ) )
                                            nonusedfp += ", " + UTIL_ToString( fpid );
                                    }
                                    else if( usedfp.empty( ) )
                                    {
                                        usedfp = UTIL_ToString( fpid );
                                        fplastowner = m_FakePlayersOwner[i];
                                    }
                                    else if( !UTIL_StringSearch( usedfp, UTIL_ToString( fpid ), 1 ) )
                                    {
                                        if( fplastowner != m_FakePlayersOwner[i] )
                                            fpsomeowners = true;

                                        usedfp += ", " + UTIL_ToString( fpid );
                                        fplastowner = m_FakePlayersOwner[i];
                                    }

                                    break;
                                }
                            }
                        }
                    }
                }
                else
                    incorrectfp = true;

                if( fpidstr == "all" )
                {
                    fpidsall = true;
                    break;
                }
            }

            if( nonusedfp.size( ) > 2 )
                SendChat( player, "Указанные фейк-плееры (" + nonusedfp + ") сейчас никем не используются." );
            else if( !nonusedfp.empty( ) )
                SendChat( player, "Указанный фейк-плеер (" + nonusedfp + ") сейчас никем не используется." );

            if( usedfp.size( ) > 2 && !fpsomeowners )
               SendChat( player, "Указанные фейк-плееры (" + usedfp + ") используются игроком [" + fplastowner + "]" );
            else if( usedfp.size( ) > 2 )
                SendChat( player, "Указанные фейк-плееры (" + usedfp + ") используются другими игроками." );
            else if( !usedfp.empty( ) )
                SendChat( player, "Указанный фейк-плеер (" + usedfp + ") используется игроком [" + fplastowner + "]" );

            if( fpids.size( ) > 2 )
                SendChat( player, "Управление от имени фейк-плееров [" + fpids + "] отменено!" );
            else if( !fpids.empty( ) )
                SendChat( player, "Управление от имени фейк-плеера №" + fpids + " отменено!" );
            else if( fpidsall )
                SendChat( player, "У вас нет подконтрольных фейк-плееров!" );
            else if( nonexistentfp.size( ) > 2 )
                SendChat( player, "Указаны несуществующие номера фейк-плееров (" + nonexistentfp + "), используйте значения <" + fpnumbers + "/all>" );
            else if( !nonexistentfp.empty( ) )
                SendChat( player, "Указан несуществующий номер фейк-плеера (" + nonexistentfp + "), используйте значения <" + fpnumbers + "/all>" );
            else if( incorrectfp && nonusedfp.empty( ) && usedfp.empty( ) )
                SendChat( player, "Укажите номер фейк-плеера, от имени которого требуется отменить управление юнитом: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <" + fpnumbers + "/all>" );

            bool checkfpcopyused = false;

            for( uint32_t i = 0; i < m_FakePlayers.size( ); i++ )
            {
                if( !m_FakePlayersOwner[i].empty( ) )
                    checkfpcopyused = true;
            }

            m_FakePlayersCopyUsed = checkfpcopyused;
        }

        //
        // !SPOOF
        // !СПУФ
        // !SPOOF2
        // !СПУФ2
        // !SPOOFALLY
        // !СПУФСОЮЗ
        //

        else if( Command == "spoof" || Command == "спуф" || Command == "spoof2" || Command == "спуф2" || Command == "spoofally" || Command == "спуфсоюз" )
        {
            if( !Payload.empty( ) )
            {
                string OwnerLower;
                string Victim;
                string Msg = string( );
                stringstream SS;
                SS << Payload;
                SS >> Victim;

                if( !SS.eof( ) )
                {
                    getline( SS, Msg );

                    if( Msg.size( ) > 0 && Msg[0] == ' ' )
                        Msg = Msg.substr( 1 );
                }

                if( !Msg.empty( ) )
                {
                    CGamePlayer *LastMatch = nullptr;
                    uint32_t Matches = GetPlayerFromNamePartial( Victim, &LastMatch );

                    if( Matches == 0 )
                        SendChat( player, "Not matches for spoof" );
                    else if( Matches == 1 )
                    {
                        if( IsDefaultOwner( LastMatch->GetName( ) ) )
                            SendChat( player, "You can't spoof an owner!" );
                        else if( Command == "spoof" || Command == "спуф" )
                            SendAllChat( LastMatch->GetPID( ), Msg );
                        else
                            SendAllyChat( LastMatch->GetPID( ), Msg );
                    }
                    else
                        SendChat( player, "Found more than one match" );
                }
                else
                    SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <name> <message>" );
            }
            else
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <name> <message>" );

            return true;
        }

        //
        // !IPS
        //

        else if( Command == "ips" )
        {
            if( PayloadLower == "all" )
            {
                string Froms;

                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    // we reverse the byte order on the IP because it's stored in network byte order

                    Froms += ( *i )->GetName( );
                    Froms += ": (";
                    Froms += ( *i )->GetExternalIPString( );
                    Froms += ")";

                    if( i != m_Players.end( ) - 1 )
                        Froms += ", ";
                }

                SendAllChat( Froms );
                return HideCommand;
            }
            else
            {
                string Froms;

                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    // we reverse the byte order on the IP because it's stored in network byte order

                    Froms += ( *i )->GetName( );
                    Froms += ": (";
                    Froms += ( *i )->GetExternalIPString( );
                    Froms += ")";

                    if( i != m_Players.end( ) - 1 )
                        Froms += ", ";
                }

                SendChat( player, Froms );
                return true;
            }
        }

        //
        // !LAST
        // !CHECKLAST
        // !CL
        // !LASTPLAYER
        // !LP
        // !FROMLAST
        // !FL
        // !PINGLAST
        // !PL
        //

        else if( Command == "last" || Command == "checklast" || Command == "cl" || Command == "lastplayer" || Command == "lp" || Command == "fromlast" || Command == "fl" || Command == "pinglast" || Command == "pl" )
        {
            string Pings;
            CGamePlayer *Player = nullptr;

            if( m_LastPlayerJoined != 255 )
                Player = GetPlayerFromPID( m_LastPlayerJoined );

            if( Player == nullptr )
                return HideCommand;

            if( ( Command == "pinglast" || Command == "pl" ) && PayloadLower == "detail" )
            {
                SendChat( player, Player->GetPingDetail( m_UNM->m_LCPings ) );
                return HideCommand;
            }

            Pings = Player->GetName( ) + ": ";

            if( Player->GetNumPings( ) > 0 )
                Pings += UTIL_ToString( Player->GetPing( m_UNM->m_LCPings ) ) + " ms";
            else
                Pings += "N/A";

            SendAllChat( Pings );
        }

        //
        // !SILENCE
        // !SIL
        // !ТИШИНА
        //

        else if( Command == "silence" || Command == "sil" || Command == "тишина" )
        {
            player->SetSilence( !player->GetSilence( ) );

            if( player->GetSilence( ) )
                SendChat( player->GetPID( ), "Silence ON" );
            else
                SendChat( player->GetPID( ), "Silence OFF" );
        }

        //
        // !UNMUTE
        // !UM
        // !РАЗМУТИТЬ
        // !АНМУТ
        // !UNMUTEME
        // !UMNE
        // !UNMUTESELF
        // !UMSELF
        //

        else if( Command == "unmute" || Command == "um" || Command == "размутить" || Command == "анмут" || Command == "unmuteme" || Command == "umme" || Command == "unmuteself" || Command == "umself" )
        {
            string MuteNameTest;

            if( Payload.empty( ) || Command == "unmuteme" || Command == "umme" || Command == "unmuteself" || Command == "umself" )
                MuteNameTest = User;
            else
                MuteNameTest = Payload;

            CGamePlayer *LastMatch = nullptr;
            uint32_t Matches = GetPlayerFromNamePartial( MuteNameTest, &LastMatch );

            if( Matches == 0 )
                SendChat( player, "no match found!" );
            else if( Matches == 1 )
            {
                string MuteName = LastMatch->GetName( );
                uint32_t sec = IsCensorMuted( MuteName );
                if( IsMuted( MuteName ) || LastMatch->GetMuted( ) || sec > 0 )
                {
                    if( sec > 0 && !m_UNM->m_AdminCanUnCensorMute && !DefaultOwnerCheck )
                    {
                        SendChat( player->GetPID( ), MuteName + " is censor muted for " + UTIL_ToString( sec ) + " more seconds, you can't unmute" );
                        DelFromMuted( MuteName );
                        if( m_UNM->m_AdminCanUnAutoMute || DefaultOwnerCheck )
                        {
                            LastMatch->m_MuteMessages.clear( );
                            LastMatch->SetMuted( false );
                        }
                        return HideCommand;
                    }
                    if( LastMatch->GetMuted( ) && !m_UNM->m_AdminCanUnAutoMute && !DefaultOwnerCheck )
                    {
                        SendChat( player->GetPID( ), MuteName + " is auto muted, you can't unmute" );
                        DelFromMuted( MuteName );
                        if( sec > 0 )
                            DelFromCensorMuted( MuteName );
                        return HideCommand;
                    }
                    if( m_UNM->m_ManualUnMutePrinting == 1 )
                        SendAllChat( gLanguage->RemovedPlayerFromTheMuteList( MuteName ) );
                    else if( m_UNM->m_ManualUnMutePrinting == 2 )
                        SendChat( LastMatch->GetPID( ), "Вы снова можете писать в чате" );
                    else if( m_UNM->m_ManualUnMutePrinting == 3 )
                    {
                        SendChat( LastMatch->GetPID( ), "Вы снова можете писать в чате" );
                        SendAllChat( gLanguage->RemovedPlayerFromTheMuteList( MuteName ) );
                    }
                    LastMatch->m_MuteMessages.clear( );
                    LastMatch->SetMuted( false );
                    DelFromMuted( MuteName );
                    if( sec > 0 )
                        DelFromCensorMuted( MuteName );
                }
                else
                {
                    if( MuteNameTest == User )
                        SendChat( player->GetPID( ), "Вы не в муте" );
                    else
                        SendChat( player->GetPID( ), MuteName + " is not muted" );
                }
            }
            else
                SendChat( player, "Can't unmute. Found more than one match" );
        }

        //
        // !MUTE
        // !M
        // !ЗАТКНУТЬ
        // !МУТ
        //

        else if( ( Command == "mute" || Command == "m" || Command == "заткнуть" || Command == "мут" ) && !Payload.empty( ) )
        {
            CGamePlayer *LastMatch = nullptr;
            uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

            if( Matches == 0 )
                SendChat( player, "no match found!" );
            else if( Matches == 1 )
            {
                if( LastMatch->GetName( ) != User )
                {
                    // admin can't mute another admin
                    if( ( IsDefaultOwner( LastMatch->GetName( ) ) ) )
                    {
                        SendChat( player->GetPID( ), "Нельзя замутить хоста!" );								// You can't mute an admin!
                        return HideCommand;
                    }
                    // admin can't mute owner game
                    if( IsOwner( LastMatch->GetName( ) ) && !DefaultOwnerCheck )
                    {
                        SendChat( player->GetPID( ), "Нельзя замутить владельца игры!" );						// You can't mute an owner game!
                        return HideCommand;
                    }
                }
                else
                {
                    // admin can mute yourself :)
                    string MuteName = LastMatch->GetName( );

                    if( IsMuted( MuteName ) )
                        SendChat( player->GetPID( ), "You is already muted" );
                    else
                    {
                        SendAllChat( "Игрок [" + User + "] дал мут сам себе :)" );								// You can mute yourself :)
                        AddToMuted( MuteName );
                    }

                    return HideCommand;
                }

                string MuteName = LastMatch->GetName( );

                if( IsMuted( MuteName ) )
                    SendChat( player->GetPID( ), MuteName + " is already muted" );
                else
                {
                    SendAllChat( gLanguage->AddedPlayerToTheMuteList( MuteName ) );
                    AddToMuted( MuteName );
                }
            }
            else
                SendChat( player, "Can't mute. Found more than one match" );
        }

        //
        // !HCL
        // !MODE
        // !МОД
        //

        else if( ( Command == "hcl" || Command == "mode" || Command == "мод" ) && !m_CountDownStarted )
        {
            if( !Payload.empty( ) )
            {
                if( Payload.size( ) <= m_Slots.size( ) )
                {
                    string HCLChars = "abcdefghijklmnopqrstuvwxyz0123456789 -=,.";

                    if( Payload.find_first_not_of( HCLChars ) == string::npos )
                    {
                        m_HCLCommandString = Payload;
                        SendAllChat( gLanguage->SettingHCL( m_HCLCommandString ) );
                    }
                    else
                        SendAllChat( gLanguage->UnableToSetHCLInvalid( ) );
                }
                else
                    SendAllChat( gLanguage->UnableToSetHCLTooLong( ) );
            }
            else
                SendAllChat( gLanguage->TheHCLIs( m_HCLCommandString ) );
        }

        //
        // !HELP
        // !ПОМОЩЬ
        //

        else if( Command == "help" || Command == "помощь" )
        {
            if( PayloadLower == "all" )
                SendAllChat( "Юзер-команды: " + string( 1, m_UNM->m_CommandTrigger ) + "p " + string( 1, m_UNM->m_CommandTrigger ) + "lat " + string( 1, m_UNM->m_CommandTrigger ) + "s " + string( 1, m_UNM->m_CommandTrigger ) + "cm " + string( 1, m_UNM->m_CommandTrigger ) + "ignore " + string( 1, m_UNM->m_CommandTrigger ) + "stats " + string( 1, m_UNM->m_CommandTrigger ) + "sd " + string( 1, m_UNM->m_CommandTrigger ) + "serv " + string( 1, m_UNM->m_CommandTrigger ) + "from " + string( 1, m_UNM->m_CommandTrigger ) + "gn " + string( 1, m_UNM->m_CommandTrigger ) + "ff " + string( 1, m_UNM->m_CommandTrigger ) + "rmk " + string( 1, m_UNM->m_CommandTrigger ) + "vk " + string( 1, m_UNM->m_CommandTrigger ) + "vs" );
            else
            {
                SendChat( player, "Aдмин-кoмaнды: " + string( 1, m_UNM->m_CommandTrigger ) + "lock " + string( 1, m_UNM->m_CommandTrigger ) + "unlock " + string( 1, m_UNM->m_CommandTrigger ) + "mute " + string( 1, m_UNM->m_CommandTrigger ) + "kick " + string( 1, m_UNM->m_CommandTrigger ) + "ban " + string( 1, m_UNM->m_CommandTrigger ) + "close " + string( 1, m_UNM->m_CommandTrigger ) + "open " + string( 1, m_UNM->m_CommandTrigger ) + "hold " + string( 1, m_UNM->m_CommandTrigger ) + "swap " + string( 1, m_UNM->m_CommandTrigger ) + "sp " + string( 1, m_UNM->m_CommandTrigger ) + "start " + string( 1, m_UNM->m_CommandTrigger ) + "as " + string( 1, m_UNM->m_CommandTrigger ) + "unhost " + string( 1, m_UNM->m_CommandTrigger ) + "end" );
                SendChat( player, "Юзер-команды: " + string( 1, m_UNM->m_CommandTrigger ) + "p " + string( 1, m_UNM->m_CommandTrigger ) + "lat " + string( 1, m_UNM->m_CommandTrigger ) + "s " + string( 1, m_UNM->m_CommandTrigger ) + "cm " + string( 1, m_UNM->m_CommandTrigger ) + "ignore " + string( 1, m_UNM->m_CommandTrigger ) + "stats " + string( 1, m_UNM->m_CommandTrigger ) + "sd " + string( 1, m_UNM->m_CommandTrigger ) + "serv " + string( 1, m_UNM->m_CommandTrigger ) + "from " + string( 1, m_UNM->m_CommandTrigger ) + "gn " + string( 1, m_UNM->m_CommandTrigger ) + "ff " + string( 1, m_UNM->m_CommandTrigger ) + "rmk " + string( 1, m_UNM->m_CommandTrigger ) + "vk " + string( 1, m_UNM->m_CommandTrigger ) + "vs" );
            }

            return HideCommand;
        }

        //
        // !HOLD (hold a specified slot for someone)
        // !H
        // !ХОЛД
        // !ЗАХОЛДИТЬ
        // !ЗАБРОНИРОВАТЬ
        // !ЗАНЯТЬ
        // !ЗАРЕЗЕРВИРОВАТЬ
        //

        else if( ( Command == "hold" || Command == "h" || Command == "холд" || Command == "захолдить" || Command == "забронировать" || Command == "занять" || Command == "зарезервировать" ) && !m_GameLoading && !m_GameLoaded )
        {
            if( Payload.find_first_not_of( " " ) == string::npos )
            {
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <nick>" );
                return HideCommand;
            }

            // hold as many players as specified, e.g. "2 Varlock 4 Kilranin" holds players "Varlock" and "Kilranin" on 2 and 4 slots

            bool failed = false;
            bool succes = false;
            stringstream SS;
            SS << Payload;

            while( !SS.eof( ) )
            {
                uint32_t HoldNumber;
                SS >> HoldNumber;

                if( SS.fail( ) )
                {
                    failed = true;
                    break;
                }
                else
                {
                    unsigned char HoldNr;
                    string HoldName;
                    SS >> HoldName;

                    if( SS.fail( ) )
                    {
                        failed = true;
                        break;
                    }
                    else
                    {
                        if( HoldNumber > m_Slots.size( ) )
                            HoldNumber = m_Slots.size( );

                        succes = true;
                        HoldNr = static_cast<unsigned char>( HoldNumber - 1 );
                        SendAllChat( gLanguage->AddedPlayerToTheHoldList( HoldName ) );
                        AddToReserved( HoldName, string( ), HoldNr );
                    }
                }
            }

            if( failed && !succes )
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot> <nick>" );
        }

        //
        // !HOLDS (hold a slot for someone)
        //

        else if( Command == "holds" && !m_GameLoading && !m_GameLoaded )
        {
            if( Payload.find_first_not_of( " " ) == string::npos )
            {
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <nick> <nick> <nick>..." );
                return HideCommand;
            }

            // hold as many players as specified, e.g. "Varlock Kilranin" holds players "Varlock" and "Kilranin"

            bool failed = false;
            bool succes = false;
            stringstream SS;
            SS << Payload;

            while( !SS.eof( ) )
            {
                string HoldName;
                SS >> HoldName;

                if( SS.fail( ) )
                {
                    failed = true;
                    break;
                }
                else
                {
                    succes = true;
                    SendAllChat( gLanguage->AddedPlayerToTheHoldList( HoldName ) );
                    AddToReserved( HoldName, string( ), 255 );
                }
            }

            if( failed && !succes )
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <nick> <nick> <nick>..." );
        }

        //
        // !UNHOLD (unhold a slot for someone)
        //

        else if( Command == "unhold" && !m_GameLoading && !m_GameLoaded )
        {
            if( Payload.find_first_not_of( " " ) == string::npos )
            {
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <nick> <nick> <nick>..." );
                return HideCommand;
            }

            // unhold as many players as specified, e.g. "Varlock Kilranin" unholds players "Varlock" and "Kilranin"

            bool failed = false;
            bool succes = false;
            stringstream SS;
            SS << Payload;

            while( !SS.eof( ) )
            {
                string HoldName;
                SS >> HoldName;

                if( SS.fail( ) )
                {
                    failed = true;
                    break;
                }
                else
                {
                    succes = true;
                    SendAllChat( "Removed " + HoldName + " from hold list" );
                    DelFromReserved( HoldName );
                }
            }

            if( failed && !succes )
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <nick> <nick> <nick>..." );
        }

        //
        // !KICK (kick a player)
        // !K
        // !КИКНУТЬ
        // !КИК
        // !ВЫКИНУТЬ
        //

        else if( ( Command == "kick" || Command == "k" || Command == "кикнуть" || Command == "кик" || Command == "выкинуть" ) && !Payload.empty( ) )
        {
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
                    return HideCommand;
                }
            }
            else
                Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

            if( Matches == 0 )
                SendChat( player->GetPID( ), gLanguage->UnableToKickNoMatchesFound( Payload ) );			// Unable to kick no matches found
            else if( Matches == 1 )
            {
                // admin can't kick yourself
                if( LastMatch->GetName( ) == User )
                {
                    SendChat( player->GetPID( ), "Нельзя кикнуть самого себя!" );
                    return HideCommand;
                }
                // admin can't kick another admin
                else if( IsOwner( LastMatch->GetName( ) ) && !DefaultOwnerCheck )
                {
                    SendChat( player->GetPID( ), "Нельзя кикнуть админа!" );
                    return HideCommand;
                }
                // admin can't kick owner game
                else if( IsDefaultOwner( LastMatch->GetName( ) ) )
                {
                    SendChat( player->GetPID( ), "Нельзя кикнуть владельца игры!" );
                    return HideCommand;
                }

                LastMatch->SetDeleteMe( true );
                LastMatch->SetLeftReason( "Кикнут админом " + User );

                if( !m_GameLoading && !m_GameLoaded )
                    LastMatch->SetLeftCode( PLAYERLEAVE_LOBBY );
                else
                    LastMatch->SetLeftCode( PLAYERLEAVE_LOST );

                if( !m_GameLoading && !m_GameLoaded )
                    OpenSlot( GetSIDFromPID( LastMatch->GetPID( ) ), false );
            }
            else
                SendChat( player->GetPID( ), gLanguage->UnableToKickFoundMoreThanOneMatch( Payload ) );	// UnableToKickFoundMoreThanOneMatch
        }

        //
        // !LOBBYBAN
        // !LOBYBAN
        // !BANLOBBY
        // !BANLOBY
        // !LB
        //

        else if( Command == "lobbyban" || Command == "lobyban" || Command == "banlobby" || Command == "banloby" || Command == "lb" )
        {
            CommandLobbyBan( player, Payload );
            return HideCommand;
        }

        //
        // !DRD (turn dynamic latency on or off)
        // !DLATENCY
        // !DDR
        //

        else if( Command == "drd" || Command == "dlatency" || Command == "ddr" )
        {
            if( PayloadLower == "on" )
            {
                m_UseDynamicLatency = true;
                SendAllChat( "Dynamic latency enabled" );
            }
            else if( PayloadLower == "off" )
            {
                m_UseDynamicLatency = false;
                SendAllChat( "Dynamic latency disabled" );
            }
            else
            {
                m_UseDynamicLatency = !m_UseDynamicLatency;

                if( m_UseDynamicLatency )
                    SendAllChat( "Dynamic latency enabled" );
                else
                    SendAllChat( "Dynamic latency disabled" );
            }

            m_DynamicLatencyLagging = false;
            m_DynamicLatencyBotLagging = false;
        }

        //
        // !LATENCY (set game latency)
        // !LAT
        // !L
        // !DELAY
        // !ЛАТЕНСИ
        // !ЗАДЕРЖКА
        // !ДЕЛЕЙ
        // !ЛАТ
        // !Л
        //

        else if( Command == "latency" || Command == "lat" || Command == "l" || Command == "delay" || Command == "латенси" || Command == "задержка" || Command == "делей" || Command == "лат" || Command == "л" )
        {
            uint64_t checkLatency = UTIL_ToUInt64( Payload );

            if( Payload.empty( ) )
            {
                if( m_UseDynamicLatency )
                    SendAllChat( "Dynamic latency is [" + UTIL_ToString( m_Latency ) + "]" );
                else
                    SendAllChat( gLanguage->LatencyIs( UTIL_ToString( m_Latency ) ) );
            }
            else if( Payload.find_first_not_of( "0123456789" ) != string::npos )
            {
                if( Command != "l" || ( !m_GameLoading && !m_GameLoaded ) )
                    SendChat( player, "херню ввел, вводи только числа " + UTIL_ToString( m_UNM->m_MinLatency ) + "-" + UTIL_ToString( m_UNM->m_MaxLatency ) );
            }
            else
            {
                if( m_UseDynamicLatency )
                {
                    m_UseDynamicLatency = false;
                    m_DynamicLatencyLagging = false;
                    m_DynamicLatencyBotLagging = false;
                    SendAllChat( "Динамический латенси отключен, чтобы включить - юзай: " + string( 1, m_UNM->m_CommandTrigger ) + "drd" );
                }

                if( checkLatency < m_UNM->m_MinLatency )
                {
                    m_Latency = m_UNM->m_MinLatency;
                    SendAllChat( gLanguage->SettingLatencyToMinimum( UTIL_ToString( m_Latency ) ) );
                }
                else if( checkLatency > m_UNM->m_MaxLatency )
                {
                    m_Latency = m_UNM->m_MaxLatency;
                    SendAllChat( gLanguage->SettingLatencyToMaximum( UTIL_ToString( m_Latency ) ) );
                }
                else
                {
                    m_Latency = (uint32_t)checkLatency;
                    SendAllChat( gLanguage->SettingLatencyTo( UTIL_ToString( m_Latency ) ) );
                }
            }

            return HideCommand;
        }

        //
        // !MESSAGES
        //

        else if( Command == "messages" )
        {
            if( PayloadLower == "on" )
            {
                SendAllChat( gLanguage->LocalAdminMessagesEnabled( ) );
                m_LocalAdminMessages = true;
            }
            else if( PayloadLower == "off" )
            {
                SendAllChat( gLanguage->LocalAdminMessagesDisabled( ) );
                m_LocalAdminMessages = false;
            }
            else
            {
                m_LocalAdminMessages = !m_LocalAdminMessages;

                if( m_LocalAdminMessages )
                    SendAllChat( gLanguage->LocalAdminMessagesEnabled( ) );
                else
                    SendAllChat( gLanguage->LocalAdminMessagesDisabled( ) );
            }
        }

        //
        // !COOKIE
        // !COOKIES
        // !GIVECOOKIE
        // !GIVECOOKIES
        // !COOKIEGIVE
        // !COOKIESGIVE
        //

        else if( Command == "cookie" || Command == "cookies" || Command == "givecookie" || Command == "givecookies" || Command == "cookiegive" || Command == "cookiesgive" )
        {
            if( !m_FunCommandsGame )
                SendChat( player, "Фан-команды отключены!" );
            else if( !m_UNM->m_EatGame )
                SendChat( player, gLanguage->CommandDisabled( string( 1, m_UNM->m_CommandTrigger ) + Command ) );
            else
            {
                if( Command == "cookie" || Command == "cookies" )
                {
                    uint32_t numCookies = player->GetCookies( );

                    if( Payload.empty( ) || PayloadLower == "check" )
                    {
                        if( numCookies > m_UNM->m_MaximumNumberCookies )
                        {
                            numCookies = m_UNM->m_MaximumNumberCookies;
                            player->SetCookies( numCookies );
                        }

                        if( numCookies > 0 )
                            SendChat( player, "У тебя осталось " + UTIL_ToString( numCookies ) + " печенья (" + string( 1, m_UNM->m_CommandTrigger ) + "eat - съесть)." );
                        else
                            SendChat( player, "У тебя нет печенья, но с тобой может поделиться хост (" + string( 1, m_UNM->m_CommandTrigger ) + "cookie <name>)." );
                    }
                    else if( PayloadLower == "eat" )
                    {
                        if( numCookies > m_UNM->m_MaximumNumberCookies )
                        {
                            numCookies = m_UNM->m_MaximumNumberCookies;
                            player->SetCookies( numCookies );
                        }

                        if( numCookies > 0 )
                        {
                            if( GetTime( ) - player->GetLastEat( ) < m_UNM->m_EatGameDelay )
                            {
                                SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
                                return HideCommand;
                            }

                            player->SetLastEat( GetTime( ) );
                            numCookies--;
                            player->SetCookies( numCookies );
                            SendAllChat( "[" + User + "] съел печенье. Это было вкусно! У него осталось " + UTIL_ToString( numCookies ) + " печенья." );
                        }
                        else
                            SendChat( player, "У тебя нет печенья! Печенье может дать хост (" + string( 1, m_UNM->m_CommandTrigger ) + "cookie <name>)." );
                    }
                    else
                    {
                        if( numCookies > 0 )
                            SendChat( player, "У тебя осталось " + UTIL_ToString( numCookies ) + " печенья (" + string( 1, m_UNM->m_CommandTrigger ) + "eat - съесть)." );
                        else
                            SendChat( player, "У тебя нет печенья, но с тобой может поделиться хост (" + string( 1, m_UNM->m_CommandTrigger ) + "cookie <name>)." );
                    }
                }
                else
                    SendChat( player, gLanguage->CommandDisabled( string( 1, m_UNM->m_CommandTrigger ) + Command ) );
            }

            return HideCommand;
        }

        //
        // !LISTEN
        // !ПРОСЛУШКА
        //

        else if( Command == "listen" || Command == "прослушка" )
        {
            if( IsListening( player->GetPID( ) ) )
            {
                DelFromListen( player->GetPID( ) );
                SendChat( player->GetPID( ), "Вы отключили прослушку, теперь вам недоступны союзный чат противников и чужие приватные сообщения!" );
            }
            else
            {
                AddToListen( player->GetPID( ) );
                SendChat( player->GetPID( ), "Вы включили прослушку, теперь вам доступны союзный чат противников и чужие приватные сообщения!" );
            }

            return true;
        }

        //
        // !MUTEALL
        //

        else if( Command == "muteall" && m_GameLoaded )
        {
            SendAllChat( gLanguage->GlobalChatMuted( ) );
            m_MuteAll = true;
        }

        //
        // !UNMUTEALL
        //

        else if( Command == "unmuteall" && m_GameLoaded )
        {
            SendAllChat( gLanguage->GlobalChatUnmuted( ) );
            m_MuteAll = false;
        }

        //
        // !МОЛЧАНИЕ
        // !ОБЩИЙЧАТ
        //

        else if( ( Command == "молчание" || Command == "общийчат" ) && m_GameLoaded )
        {
            m_MuteAll = !m_MuteAll;

            if( m_MuteAll )
                SendAllChat( gLanguage->GlobalChatMuted( ) );
            else
                SendAllChat( gLanguage->GlobalChatUnmuted( ) );
        }

        //
        // !OPEN (open slot)
        // !ОТКРЫТЬ
        //

        else if( ( Command == "open" || Command == "открыть" ) && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
        {
            // open as many slots as specified, e.g. "5 10" opens slots 5 and 10

            stringstream SS;
            SS << Payload;
            bool failed = false;
            bool succes = false;
            bool msgself = false;
            bool msgcomp = false;
            bool msgfake = false;

            while( !SS.eof( ) )
            {
                string sSID;
                uint32_t SID;
                SS >> sSID;
                bool canopen = true;

                if( SS.fail( ) )
                {
                    if( !succes )
                        SendChat( player, "bad input to [" + Command + "] command" );

                    return HideCommand;
                }

                SID = UTIL_ToUInt32( sSID );

                if( SID == 0 )
                    failed = true;
                else if( SID - 1 < m_Slots.size( ) )
                {
                    succes = true;
                    CGamePlayer *Player = GetPlayerFromSID( static_cast<unsigned char>( SID - 1 ) );

                    if( Player )
                    {
                        if( Player->GetName( ) == User )
                        {
                            if( !msgself )
                            {
                                SendChat( player->GetPID( ), "Нельзя кикнуть самого себя!" );
                                msgself = true;
                            }

                            canopen = false;
                        }
                    }
                    else
                    {
                        if( m_Slots[SID - 1].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID - 1].GetComputer( ) == 1 )
                        {
                            if( !msgcomp )
                            {
                                SendChat( player->GetPID( ), "You have opened computer slot, type " + string( 1, m_UNM->m_CommandTrigger ) + "comp " + UTIL_ToString( SID ) + " to make it a comp again" );
                                msgcomp = true;
                            }
                        }
                        else if( m_Slots[SID - 1].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED )
                        {
                            if( !msgfake )
                            {
                                SendChat( player->GetPID( ), gLanguage->DoDFInstead( string( 1, m_UNM->m_CommandTrigger ) ) );
                                msgfake = true;
                            }
                            canopen = false;	// here to prevent kicking a fakeplayer which resulted as incorrect slots info and cause all players to be dropped when the game starts
                        }
                    }

                    if( canopen )
                        OpenSlot( static_cast<unsigned char>( SID - 1 ), true );
                }
            }

            if( failed && !succes )
                SendChat( player, "bad input to [" + Command + "] command" );
        }

        //
        // !OPENALL
        //

        else if( Command == "openall" && !m_GameLoading && !m_GameLoaded )
        {
            OpenAllSlots( );
        }

        //
        // !PING
        // !P
        // !ПИНГ
        // !П
        //

        else if( Command == "ping" || Command == "p" || Command == "пинг" || Command == "п" )
        {
            if( PayloadLower == "detail" )
            {
                SendChat( player, player->GetPingDetail( m_UNM->m_LCPings ) );
                return HideCommand;
            }

            string Pings = string( );

            if( !Payload.empty( ) )
            {
                bool pingkick = false;

                if( !m_GameLoading && !m_GameLoaded && Payload.find_first_not_of( "0123456789" ) == string::npos )
                {
                    SendChat( player, "Чтобы кикнуть игроков с пингом выше указанного, используйте команду " + string( 1, m_UNM->m_CommandTrigger ) + "pingkick <ping>" );
                    pingkick = true;
                }

                CGamePlayer *LastMatch = nullptr;
                uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

                if( Matches == 0 )
                {
                    if( !pingkick )
                        SendChat( player, "No match found" );
                }
                else if( Matches == 1 )
                {
                    Pings = LastMatch->GetName( ) + ": ";

                    if( LastMatch->GetNumPings( ) > 0 )
                        Pings += UTIL_ToString( LastMatch->GetPing( m_UNM->m_LCPings ) ) + " ms";
                    else
                        Pings += "N/A";

                    SendAllChat( Pings );
                }
                else
                    SendChat( player, "Found more than one match" );

                return HideCommand;
            }

            // copy the m_Players vector so we can sort by descending ping so it's easier to find players with high pings

            vector<CGamePlayer *> SortedPlayers = m_Players;
            sort( SortedPlayers.begin( ), SortedPlayers.end( ), CGamePlayerSortDescByPing( ) );

            for( vector<CGamePlayer *> ::iterator i = SortedPlayers.begin( ); i != SortedPlayers.end( ); i++ )
            {
                Pings += ( *i )->GetNameTerminated( ) + ": ";

                if( ( *i )->GetNumPings( ) > 0 )
                    Pings += UTIL_ToString( ( *i )->GetPing( m_UNM->m_LCPings ) ) + "ms";
                else
                    Pings += "N/A";

                if( i != SortedPlayers.end( ) - 1 )
                    Pings += ", ";
            }

            SendAllChat( Pings );
            return HideCommand;
        }

        //
        // !PINGDETAIL
        // !PD
        //

        else if( Command == "pingdetail" || Command == "pd" )
        {
            if( !Payload.empty( ) )
            {
                CGamePlayer *LastMatch = nullptr;
                uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

                if( Matches == 0 )
                    SendChat( player, "No match found" );
                else if( Matches == 1 )
                    SendChat( player, "[" + LastMatch->GetName( ) + "] " + LastMatch->GetPingDetail( m_UNM->m_LCPings ) );
                else
                    SendChat( player, "Found more than one match" );

                return HideCommand;
            }

            // copy the m_Players vector so we can sort by descending ping so it's easier to find players with high pings

            vector<CGamePlayer *> SortedPlayers = m_Players;
            sort( SortedPlayers.begin( ), SortedPlayers.end( ), CGamePlayerSortDescByPing( ) );

            for( vector<CGamePlayer *> ::iterator i = SortedPlayers.begin( ); i != SortedPlayers.end( ); i++ )
                SendChat( player, "[" + ( *i )->GetName( ) + "] " + ( *i )->GetPingDetail( m_UNM->m_LCPings ) );

            return HideCommand;
        }

        //
        // !PINGKICK
        // !PK
        // !ПИНГКИК
        // !ПК
        // !KICKPING
        // !KP
        // !КИКПИНГ
        // !КП
        //

        if( Command == "pingkick" || Command == "pk" || Command == "пингкик" || Command == "пк" || Command == "kickping" || Command == "kp" || Command == "кикпинг" || Command == "кп" )
        {
            // kick players with ping higher than payload if payload isn't empty
            // we only do this if the game hasn't started since we don't want to kick players from a game in progress

            uint32_t Kicked = 0;
            uint32_t KickPing = 0;
            string Pings;

            if( m_GameLoading || m_GameLoaded )
                SendChat( player, "Данная команда недоступна в начатой игре!" );
            else if( Payload.empty( ) || Payload.find_first_not_of( "0123456789" ) != string::npos || Payload.find_first_not_of( "0" ) == string::npos )
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <ping>" );
            else
            {
                // copy the m_Players vector so we can sort by descending ping so it's easier to find players with high pings

                vector<CGamePlayer *> SortedPlayers = m_Players;
                sort( SortedPlayers.begin( ), SortedPlayers.end( ), CGamePlayerSortDescByPing( ) );
                KickPing = UTIL_ToUInt32( Payload );

                if( KickPing == 0 )
                    SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <ping>" );
                else
                {
                    for( vector<CGamePlayer *> ::iterator i = SortedPlayers.begin( ); i != SortedPlayers.end( ); i++ )
                    {
                        Pings += ( *i )->GetNameTerminated( ) + ": ";

                        if( ( *i )->GetNumPings( ) > 0 )
                        {
                            Pings += UTIL_ToString( ( *i )->GetPing( m_UNM->m_LCPings ) ) + "ms";

                            if( !( *i )->GetReserved( ) && ( *i )->GetPing( m_UNM->m_LCPings ) > KickPing )
                            {
                                ( *i )->SetDeleteMe( true );
                                ( *i )->SetLeftReason( "was kicked for excessive ping " + UTIL_ToString( ( *i )->GetPing( m_UNM->m_LCPings ) ) + " > " + UTIL_ToString( KickPing ) );
                                ( *i )->SetLeftCode( PLAYERLEAVE_LOBBY );
                                OpenSlot( GetSIDFromPID( ( *i )->GetPID( ) ), false );
                                Kicked++;
                            }
                        }
                        else
                            Pings += "N/A";

                        if( i != SortedPlayers.end( ) - 1 )
                            Pings += ", ";
                    }

                    SendAllChat( Pings );

                    if( Kicked > 0 )
                        SendAllChat( gLanguage->KickingPlayersWithPingsGreaterThan( UTIL_ToString( Kicked ), UTIL_ToString( KickPing ) ) );
                }
            }
        }

        //
        // !PREFIX
        // !CHAR
        // !ПРЕФИКС
        //

        else if( ( Command == "prefix" || Command == "char" || Command == "префикс" ) && !m_CountDownStarted && !m_GameLoading && !m_GameLoaded )
        {
            if( !m_UNM->m_AppleIcon )
            {
                SendChat( player->GetPID( ), "Команда отключена администратором" );
                return HideCommand;
            }

            if( Payload.empty( ) || PayloadLower == "check" )
            {
                if( Command == "prefix" )
                {
                    if( m_UNM->m_TempGameNameChar.empty( ) )
                        SendAllChat( "Префикс не задан (" + string( 1, m_UNM->m_CommandTrigger ) + "prefix list - список префиксов)" );
                    else
                        SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar + " (" + string( 1, m_UNM->m_CommandTrigger ) + "prefix list - список префиксов)" );
                }
                else if( Command == "char" )
                {
                    if( m_UNM->m_TempGameNameChar.empty( ) )
                        SendAllChat( "Префикс не задан (" + string( 1, m_UNM->m_CommandTrigger ) + "char list - список префиксов)" );
                    else
                        SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar + " (" + string( 1, m_UNM->m_CommandTrigger ) + "char list - список префиксов)" );
                }
                else if( Command == "префикс" )
                {
                    if( m_UNM->m_TempGameNameChar.empty( ) )
                        SendAllChat( "Префикс не задан (" + string( 1, m_UNM->m_CommandTrigger ) + "префикс <список> - список префиксов)" );
                    else
                        SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar + " (" + string( 1, m_UNM->m_CommandTrigger ) + "префикс <список> - список префиксов)" );
                }
            }
            else if( PayloadLower == "off" || PayloadLower == "disable" || PayloadLower == "убрать" || PayloadLower == "0" )
            {
                m_UNM->m_TempGameNameChar = string( );
                SendAllChat( "Префикс удален" );
            }
            else if( PayloadLower == "1" )
            {
                if( m_UNM->m_TempGameNameChar == "♠" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "♠";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "2" )
            {
                if( m_UNM->m_TempGameNameChar == "♣" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "♣";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "3" )
            {
                if( m_UNM->m_TempGameNameChar == "♥" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "♥";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "4" )
            {
                if( m_UNM->m_TempGameNameChar == "♦" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "♦";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "5" )
            {
                if( m_UNM->m_TempGameNameChar == "◊" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "◊";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "6" )
            {
                if( m_UNM->m_TempGameNameChar == "○" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "○";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "7" )
            {
                if( m_UNM->m_TempGameNameChar == "●" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "●";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "8" )
            {
                if( m_UNM->m_TempGameNameChar == "◄" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "◄";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "9" )
            {
                if( m_UNM->m_TempGameNameChar == "▼" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "▼";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "10" )
            {
                if( m_UNM->m_TempGameNameChar == "►" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "►";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "11" )
            {
                if( m_UNM->m_TempGameNameChar == "▲" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "▲";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "12" )
            {
                if( m_UNM->m_TempGameNameChar == "■" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "■";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "13" )
            {
                if( m_UNM->m_TempGameNameChar == "▪" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "▪";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "14" )
            {
                if( m_UNM->m_TempGameNameChar == "▫" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "▫";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "15" )
            {
                if( m_UNM->m_TempGameNameChar == "¤" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "¤";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "16" )
            {
                if( m_UNM->m_TempGameNameChar == "£" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "£";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "17" )
            {
                if( m_UNM->m_TempGameNameChar == "§" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "§";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "18" )
            {
                if( m_UNM->m_TempGameNameChar == "→" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "→";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "19" )
            {
                if( m_UNM->m_TempGameNameChar == "♀" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "♀";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "20" )
            {
                if( m_UNM->m_TempGameNameChar == "♂" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "♂";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "21" )
            {
                if( m_UNM->m_TempGameNameChar == "æ" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "æ";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "22" )
            {
                if( m_UNM->m_TempGameNameChar == "ζ" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "ζ";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "23" )
            {
                if( m_UNM->m_TempGameNameChar == "☻" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "☻";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "24" )
            {
                if( m_UNM->m_TempGameNameChar == "☺" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "☺";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "25" )
            {
                if( m_UNM->m_TempGameNameChar == "©" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "©";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else if( PayloadLower == "26" )
            {
                if( m_UNM->m_TempGameNameChar == "®" )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_TempGameNameChar = "®";
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
            else
                SendAllChat( "1: ♠ 2: ♣ 3: ♥ 4: ♦ 5: ◊ 6: ○ 7: ● 8: ◄ 9: ▼ 10: ► 11: ▲ 12: ■ 13: ▪ 14: ▫ 15: ¤ 16: £ 17: § 18: → 19: ♀ 20: ♂21: æ 22: ζ 23: ☻ 24:☺ 25:© 26:®" );
        }

        //
        // !PREFIXCUSTOM
        // !CHARCUSTOM
        // !CUSTOMPREFIX
        // !CUSTOMCHAR
        // !SETPREFIX
        // !SETCHAR
        //

        else if( ( Command == "prefixcustom" || Command == "charcustom" || Command == "customprefix" || Command == "customchar" || Command == "setprefix" || Command == "setchar" ) && !m_CountDownStarted && !m_GameLoading && !m_GameLoaded )
        {
            if( !m_UNM->m_AppleIcon )
            {
                SendChat( player->GetPID( ), "Команда отключена администратором" );
                return HideCommand;
            }

            if( Payload.empty( ) || PayloadLower == "check" )
            {
                if( m_UNM->m_TempGameNameChar.empty( ) )
                    SendAllChat( "Префикс не задан" );
                else
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
            }
            else if( PayloadLower == "off" || PayloadLower == "disable" || PayloadLower == "убрать" || PayloadLower == "0" )
            {
                m_UNM->m_TempGameNameChar = string( );
                SendAllChat( "Префикс удален" );
            }
            else
            {
                if( Payload == m_UNM->m_TempGameNameChar )
                    SendAllChat( "Текущий префикс: " + m_UNM->m_TempGameNameChar );
                else
                {
                    m_UNM->m_GameNameChar = Payload;
                    SendAllChat( "Префикс изменен: " + m_UNM->m_TempGameNameChar );
                }
            }
        }

        //
        // !PRIV (rehost as private game)
        // !PRI
        // !PRINHOUSE
        // !PRIH
        // !INHOUSEGAME
        // !INHOUSEG
        // !INHOUSE
        // !ПРИВ
        //

        else if( ( Command == "priv" || Command == "pri" || Command == "prinhouse" || Command == "prih" || Command == "inhousegame" || Command == "inhouseg" || Command == "inhouse" || Command == "прив" ) && !m_CountDownStarted && !m_SaveGame )
        {
            if( m_RehostTime > 0 )
            {
                uint32_t seconds;

                if( GetTime( ) - m_RehostTime >= 3 )
                    seconds = 0;
                else
                    seconds = 3 - ( GetTime( ) - m_RehostTime );

                SendChat( player->GetPID( ), "Подождите завершения рехоста игры (осталось: " + UTIL_ToString( seconds ) + " сек.)" );
                return HideCommand;
            }
            else if( GetTime( ) - m_LastPub < m_UNM->m_PubGameDelay )
            {
                uint32_t cooldown = m_UNM->m_PubGameDelay + m_LastPub - GetTime( );
                string second;
                string scooldown = UTIL_ToString( cooldown );

                if( cooldown >= 10 && cooldown <= 20 )
                    second = "секунд";
                else if( scooldown.substr( scooldown.length( ) - 1 ) == "1" )
                    second = "секунду";
                else if( scooldown.substr( scooldown.length( ) - 1 ) == "2" || scooldown.substr( scooldown.length( ) - 1 ) == "3" || scooldown.substr( scooldown.length( ) - 1 ) == "4" )
                    second = "секунды";
                else
                    second = "секунд";

                SendChat( player->GetPID( ), "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " снова станет доступной через " + UTIL_ToString( cooldown ) + " " + second );
                return HideCommand;
            }

            string GameName = Payload;

            if( !GameName.empty( ) )
            {
                if( GameName.size( ) < m_UNM->m_MinLengthGN )
                {
                    SendChat( player->GetPID( ), "Слишком короткое название (минимум " + UTIL_ToString( m_UNM->m_MinLengthGN ) + " симв.)" );
                    return HideCommand;
                }

                if( m_UNM->m_AppleIcon && !m_UNM->m_TempGameNameChar.empty( ) )
                    GameName = m_UNM->m_TempGameNameChar + " " + GameName;

                GameName = UTIL_SubStrFix( GameName, 0, 31 );

                if( m_UNM->m_FixGameNameForIccup && ( GameName.find( "aptb" ) != string::npos || GameName.find( "rdtb" ) != string::npos || GameName.find( "artb" ) != string::npos || GameName.find( "sdtb" ) != string::npos ) )
                {
                    SendChat( player->GetPID( ), "Название игры не может содержать 'aptb' 'rdtb' 'artb' 'sdtb'" );
                    return HideCommand;
                }
            }

            if( !GameName.empty( ) && GameName == m_GameName )
            {
                SendAllChat( "Вы не можете пересоздать игру с таким же названием!" );
                return HideCommand;
            }

            m_LastPub = GetTime( );

            if( GameName.empty( ) )
                GameName = m_UNM->IncGameNr( m_GameName, false, false, GetNumHumanPlayers( ), GetNumHumanPlayers( ) + GetSlotsOpen( ) );

            SetGameName( GameName );
            m_UNM->m_HostCounter++;

            if( m_UNM->m_MaxHostCounter > 0 && m_UNM->m_HostCounter > m_UNM->m_MaxHostCounter )
                m_UNM->m_HostCounter = 1;

            m_HostCounter = m_UNM->m_HostCounter;
            AddGameName( m_GameName );
            AutoSetHCL( );

            if( Command == "pri" || Command == "priv" || Command == "прив" )
            {
                if( m_GameState != GAME_PRIVATE )
                {
                    m_GameState = GAME_PRIVATE;
                    SendAllChat( "Тип игры изменен на приватный" );
                }
            }
            else if( m_GameState != static_cast<unsigned char>(m_UNM->m_gamestateinhouse) )
            {
                m_GameState = static_cast<unsigned char>(m_UNM->m_gamestateinhouse);
                SendAllChat( "Тип игры изменен на inhouse game" );
            }

            SendAllChat( "Имя игры изменено на \"" + GameName + "\"" );

            for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); ++i )
            {
                if( ( *i )->GetServerType( ) == 0 && ( *i )->GetLoggedIn( ) )
                {
                    // unqueue any existing game refreshes because we're going to assume the next successful game refresh indicates that the rehost worked
                    // this ignores the fact that it's possible a game refresh was just sent and no response has been received yet
                    // we assume this won't happen very often since the only downside is a potential false positive
                    // the game creation message will be sent on the next refresh

                    ( *i )->ClearQueueGameRehost( );

                    if( ( *i )->m_AllowAutoRehost == 2 )
                        ( *i )->QueueGameUncreate( );

                    ( *i )->QueueEnterChat( );
                    ( *i )->m_GameIsReady = false;
                    ( *i )->m_ResetUncreateTime = true;

                    if( ( *i )->m_RehostTimeFix != 0 )
                    {
                        for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                        {
                            if( ( *it )->m_RehostTimeFix != 0 && ( *it )->GetServer( ) == ( *i )->GetServer( ) )
                                ( *it )->m_LastRehostTimeFix = GetTicks( );
                        }
                    }

                    if( !( *i )->m_SpamSubBot )
                    {
                        ( *i )->m_GameName = m_GameName;
                        ( *i )->m_HostCounter = m_HostCounter;
                        ( *i )->m_GameNameAlreadyChanged = true;
                    }
                }
            }
        }

        //
        // !REHOST
        // !РЕХОСТ
        //

        else if( ( Command == "rehost" || Command == "рехост" ) && !m_CountDownStarted && !m_SaveGame )
        {
            if( m_RehostTime > 0 )
            {
                uint32_t seconds;

                if( GetTime( ) - m_RehostTime >= 3 )
                    seconds = 0;
                else
                    seconds = 3 - ( GetTime( ) - m_RehostTime );

                SendChat( player->GetPID( ), "Подождите завершения рехоста игры (осталось: " + UTIL_ToString( seconds ) + " сек.)" );
                return HideCommand;
            }
            else if( GetTime( ) - m_LastPub < m_UNM->m_PubGameDelay )
            {
                uint32_t cooldown = m_UNM->m_PubGameDelay + m_LastPub - GetTime( );
                string second;
                string scooldown = UTIL_ToString( cooldown );

                if( cooldown >= 10 && cooldown <= 20 )
                    second = "секунд";
                else if( scooldown.substr( scooldown.length( ) - 1 ) == "1" )
                    second = "секунду";
                else if( scooldown.substr( scooldown.length( ) - 1 ) == "2" || scooldown.substr( scooldown.length( ) - 1 ) == "3" || scooldown.substr( scooldown.length( ) - 1 ) == "4" )
                    second = "секунды";
                else
                    second = "секунд";

                SendChat( player->GetPID( ), "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " снова станет доступной через " + UTIL_ToString( cooldown ) + " " + second );
                return HideCommand;
            }

            string GameName = Payload;

            if( !GameName.empty( ) )
            {
                if( GameName.size( ) < m_UNM->m_MinLengthGN )
                {
                    SendChat( player->GetPID( ), "Слишком короткое название (минимум " + UTIL_ToString( m_UNM->m_MinLengthGN ) + " симв.)" );
                    return HideCommand;
                }

                if( m_UNM->m_AppleIcon && !m_UNM->m_TempGameNameChar.empty( ) )
                    GameName = m_UNM->m_TempGameNameChar + " " + GameName;

                GameName = UTIL_SubStrFix( GameName, 0, 31 );

                if( m_UNM->m_FixGameNameForIccup && ( GameName.find( "aptb" ) != string::npos || GameName.find( "rdtb" ) != string::npos || GameName.find( "artb" ) != string::npos || GameName.find( "sdtb" ) != string::npos ) )
                {
                    SendChat( player->GetPID( ), "Название игры не может содержать 'aptb' 'rdtb' 'artb' 'sdtb'" );
                    return HideCommand;
                }
            }

            if( !GameName.empty( ) && GameName == m_GameName )
            {
                SendAllChat( "Вы не можете пересоздать игру с таким же названием!" );
                return HideCommand;
            }

            m_LastPub = GetTime( );

            if( GameName.empty( ) )
                GameName = m_UNM->IncGameNr( m_GameName, false, false, GetNumHumanPlayers( ), GetNumHumanPlayers( ) + GetSlotsOpen( ) );

            SendAllChat( "Игра будет пересоздана через 3 секунды!" );
            m_RehostTime = GetTime( );
            m_UNM->m_newGameGameState = m_GameState;
            m_UNM->m_newGameServer = m_Server;
            m_UNM->m_newGameUser = User;
            m_UNM->m_newGameName = GameName;
        }

        //
        // !PUB (rehost as public game)
        // !PUBLIC
        // !PUBFORCE
        // !PUBF
        // !FORCEPUB
        // !FPUB
        // !PUBNOW
        // !PUBN
        // !NOWPUB
        // !NPUB
        // !ПАБ
        //

        else if( ( Command == "pub" || Command == "public" || Command == "pubforce" || Command == "pubf" || Command == "forcepub" || Command == "fpub" || Command == "pubnow" || Command == "pubn" || Command == "nowpub" || Command == "npub" || Command == "паб" ) && !m_CountDownStarted && !m_SaveGame )
        {
            if( m_RehostTime > 0 )
            {
                uint32_t seconds;

                if( GetTime( ) - m_RehostTime >= 3 )
                    seconds = 0;
                else
                    seconds = 3 - ( GetTime( ) - m_RehostTime );

                SendChat( player->GetPID( ), "Подождите завершения рехоста игры (осталось: " + UTIL_ToString( seconds ) + " сек.)" );
                return HideCommand;
            }
            else if( GetTime( ) - m_LastPub < m_UNM->m_PubGameDelay )
            {
                uint32_t cooldown = m_UNM->m_PubGameDelay + m_LastPub - GetTime( );
                string second;
                string scooldown = UTIL_ToString( cooldown );

                if( cooldown >= 10 && cooldown <= 20 )
                    second = "секунд";
                else if( scooldown.substr( scooldown.length( ) - 1 ) == "1" )
                    second = "секунду";
                else if( scooldown.substr( scooldown.length( ) - 1 ) == "2" || scooldown.substr( scooldown.length( ) - 1 ) == "3" || scooldown.substr( scooldown.length( ) - 1 ) == "4" )
                    second = "секунды";
                else
                    second = "секунд";

                SendChat( player->GetPID( ), "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " снова станет доступной через " + UTIL_ToString( cooldown ) + " " + second );

                if( !player->GetPubCommandWarned( ) )
                {
                    player->SetPubCommandWarned( true );
                    SendChat( player->GetPID( ), "Koмaндa " + string( 1, m_UNM->m_CommandTrigger ) + "pub <gamename> нe дeлaeт pexocт, a лишь измeняeт нaзвaниe игpы нa укaзaннoe или тeкущee+1, кoтopoe будeт иcпoльзoвaннo пpи cлeдующeм aвтopexocтe" );
                    SendChat( player->GetPID( ), "Koмaндa " + string( 1, m_UNM->m_CommandTrigger ) + "pubforce <gamename> дeлaeт тoжe caмoe + пpинудитeльнo удaляeт игpы c пpeдыдущим нaзвaниeм нa Battle.net/PVPGN cepвepax гдe xocтит бoт" );
                    SendChat( player->GetPID( ), "Koмaндa " + string( 1, m_UNM->m_CommandTrigger ) + "pubnow <gamename> (дocтупнa тoлькo pут-aдминaм) пoвтopяeт !pubforce + пpинудитeльнo дeлaeт внeoчepeднoй pexocт" );
                    SendChat( player->GetPID( ), "Кроме того, все вышеперечисленные команды изменяют тип игры на \"публичный\"" );
                }

                return HideCommand;
            }

            string GameName = Payload;

            if( !GameName.empty( ) )
            {
                if( GameName.size( ) < m_UNM->m_MinLengthGN )
                {
                    SendChat( player->GetPID( ), "Слишком короткое название (минимум " + UTIL_ToString( m_UNM->m_MinLengthGN ) + " симв.)" );
                    return HideCommand;
                }

                if( m_UNM->m_AppleIcon && !m_UNM->m_TempGameNameChar.empty( ) )
                    GameName = m_UNM->m_TempGameNameChar + " " + GameName;

                GameName = UTIL_SubStrFix( GameName, 0, 31 );

                if( m_UNM->m_FixGameNameForIccup && ( GameName.find( "aptb" ) != string::npos || GameName.find( "rdtb" ) != string::npos || GameName.find( "artb" ) != string::npos || GameName.find( "sdtb" ) != string::npos ) )
                {
                    SendChat( player->GetPID( ), "Название игры не может содержать 'aptb' 'rdtb' 'artb' 'sdtb'" );
                    return HideCommand;
                }
            }

            if( !GameName.empty( ) && GameName == m_GameName )
            {
                SendAllChat( "Вы не можете пересоздать игру с таким же названием!" );
                return HideCommand;
            }

            if( GameName.empty( ) )
                GameName = m_UNM->IncGameNr( m_GameName, false, false, GetNumHumanPlayers( ), GetNumHumanPlayers( ) + GetSlotsOpen( ) );

            m_LastPub = GetTime( );
            SetGameName( GameName );
            m_UNM->m_HostCounter++;

            if( m_UNM->m_MaxHostCounter > 0 && m_UNM->m_HostCounter > m_UNM->m_MaxHostCounter )
                m_UNM->m_HostCounter = 1;

            m_HostCounter = m_UNM->m_HostCounter;
            AddGameName( m_GameName );
            AutoSetHCL( );

            if( m_GameState != GAME_PUBLIC )
            {
                m_GameState = GAME_PUBLIC;
                SendAllChat( "Тип игры изменен на публичный" );
            }

            SendAllChat( "Имя игры изменено на \"" + GameName + "\"" );

            for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); ++i )
            {
                if( ( *i )->GetLoggedIn( ) && !( *i )->m_SpamSubBot )
                {
                    ( *i )->m_GameName = m_GameName;
                    ( *i )->m_HostCounter = m_HostCounter;
                    ( *i )->m_GameNameAlreadyChanged = true;
                }
            }

            if( Command != "pub" && Command != "public" && Command != "паб" )
            {
                for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); ++i )
                {
                    if( ( *i )->GetServerType( ) == 0 && ( *i )->GetLoggedIn( ) && ( *i )->m_AllowManualRehost )
                    {
                        // unqueue any existing game refreshes because we're going to assume the next successful game refresh indicates that the rehost worked
                        // this ignores the fact that it's possible a game refresh was just sent and no response has been received yet
                        // we assume this won't happen very often since the only downside is a potential false positive
                        // the game creation message will be sent on the next refresh

                        ( *i )->ClearQueueGameRehost( );

                        if( ( *i )->m_AllowAutoRehost == 2 )
                            ( *i )->QueueGameUncreate( );

                        ( *i )->QueueEnterChat( );
                        ( *i )->m_GameIsReady = false;
                        ( *i )->m_ResetUncreateTime = true;

                        if( ( *i )->m_RehostTimeFix != 0 )
                        {
                            for( vector<CBNET *> ::iterator it = m_UNM->m_BNETs.begin( ); it != m_UNM->m_BNETs.end( ); it++ )
                            {
                                if( ( *it )->m_RehostTimeFix != 0 && ( *it )->GetServer( ) == ( *i )->GetServer( ) )
                                    ( *it )->m_LastRehostTimeFix = GetTicks( );
                            }
                        }

                        if( Command == "pubnow" || Command == "pubn" || Command == "nowpub" || Command == "npub" )
                            ( *i )->m_LastRefreshTime = 0;
                    }
                }
            }
        }

        //
        // !REFRESH (turn on or off refresh messages)
        //

        else if( Command == "refresh" && !m_CountDownStarted )
        {
            if( PayloadLower == "on" )
            {
                SendAllChat( gLanguage->RefreshMessagesEnabled( ) );
                m_RefreshMessages = true;
            }
            else if( PayloadLower == "off" )
            {
                SendAllChat( gLanguage->RefreshMessagesDisabled( ) );
                m_RefreshMessages = false;
            }
            else
            {
                m_RefreshMessages = !m_RefreshMessages;

                if( m_RefreshMessages )
                    SendAllChat( gLanguage->RefreshMessagesEnabled( ) );
                else
                    SendAllChat( gLanguage->RefreshMessagesDisabled( ) );
            }
        }

        //
        // !SENDLAN
        //

        else if( Command == "sendlan" && !Payload.empty( ) && !m_CountDownStarted )
        {
            // extract the ip and the port
            // e.g. "1.2.3.4 6112" -> ip: "1.2.3.4", port: "6112"

            string IP;
            uint16_t Port = m_UNM->m_BroadcastPort;
            stringstream SS;
            SS << Payload;
            SS >> IP;

            if( !SS.eof( ) )
                SS >> Port;

            if( SS.fail( ) )
                SendChat( player, "bad inputs to [" + Command + "] command" );
            else
            {
                uint32_t FixedHostCounter = m_HostCounter & 0x0FFFFFFF;

                // we send 12 for SlotsTotal because this determines how many PID's Warcraft 3 allocates
                // we need to make sure Warcraft 3 allocates at least SlotsTotal + 1 but at most 12 PID's
                // this is because we need an extra PID for the virtual host player (but we always delete the virtual host player when the 12th person joins)
                // however, we can't send 13 for SlotsTotal because this causes Warcraft 3 to crash when sharing control of units
                // nor can we send SlotsTotal because then Warcraft 3 crashes when playing maps with less than 12 PID's (because of the virtual host player taking an extra PID)
                // we also send 12 for SlotsOpen because Warcraft 3 assumes there's always at least one player in the game (the host)
                // so if we try to send accurate numbers it'll always be off by one and results in Warcraft 3 assuming the game is full when it still needs one more player
                // the easiest solution is to simply send 12 for both so the game will always show up as (1/12) players

                // construct the correct W3GS_GAMEINFO packet

                if( m_SaveGame )
                {
                    // note: the PrivateGame flag is not set when broadcasting to LAN (as you might expect)

                    uint32_t MapGameType = MAPGAMETYPE_SAVEDGAME;
                    BYTEARRAY MapWidth;
                    MapWidth.push_back( 0 );
                    MapWidth.push_back( 0 );
                    BYTEARRAY MapHeight;
                    MapHeight.push_back( 0 );
                    MapHeight.push_back( 0 );
                    m_UNM->m_UDPSocket->SendTo( IP, Port, m_Protocol->SEND_W3GS_GAMEINFO( m_UNM->m_TFT, m_UNM->m_LANWar3Version, UTIL_CreateByteArray( MapGameType, false ), m_Map->GetMapGameFlags( ), MapWidth, MapHeight, m_GameName, m_DefaultOwner, GetTime( ) - m_CreationTime, "Save\\Multiplayer\\" + m_SaveGame->GetFileNameNoPath( ), m_SaveGame->GetMagicNumber( ), 12, 12, m_HostPort, FixedHostCounter, m_EntryKey, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]" );
                }
                else
                {
                    uint32_t MapGameType = MAPGAMETYPE_UNKNOWN0;
                    m_UNM->m_UDPSocket->SendTo( IP, Port, m_Protocol->SEND_W3GS_GAMEINFO( m_UNM->m_TFT, m_UNM->m_LANWar3Version, UTIL_CreateByteArray( MapGameType, false ), m_Map->GetMapGameFlags( ), m_Map->GetMapWidth( ), m_Map->GetMapHeight( ), m_GameName, m_DefaultOwner, GetTime( ) - m_CreationTime, m_Map->GetMapPath( ), m_Map->GetMapCRC( ), 12, 12, m_HostPort, FixedHostCounter, m_EntryKey, "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]", m_CurrentGameCounter ), "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "]" );
                }
            }
        }

        //
        // !MARSAUTO
        // !AUTOMARS
        // !AUTOINSULT
        //

        else if( Command == "marsauto" || Command == "automars" || Command == "autoinsult" )
        {
            if( m_GameLoaded || m_GameLoading )
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна в начатой игре!" );
            else if( !m_FunCommandsGame && !m_autoinsultlobby )
                SendChat( player, "Фан-команды отключены!" );
            else
            {
                m_autoinsultlobby = !m_autoinsultlobby;

                if( m_autoinsultlobby )
                    SendChat( player->GetPID( ), "Auto insult ON" );
                else
                    SendChat( player->GetPID( ), "Auto insult OFF" );
            }
        }

        //
        // !SP
        // !ПЕРЕМЕШАТЬ
        //

        else if( ( Command == "sp" || Command == "перемешать" ) && !m_CountDownStarted )
        {
            SendAllChat( gLanguage->ShufflingPlayers( ) );
            ShuffleSlots( );
        }

        //
        // !STARTF
        // !STARTFORCE
        // !FORCESTART
        // !SF
        // !FS
        //

        else if( ( Command == "startf" || Command == "startforce" || Command == "forcestart" || Command == "sf" || Command == "fs" ) && !m_CountDownStarted )
        {
            if( GetTicks( ) - m_LastLeaverTicks < 300 )
            {
                SendAllChat( gLanguage->SomeOneJustLeft( ) );
                return HideCommand;
            }

            if( m_UNM->m_FakePlayersLobby && !m_FakePlayers.empty( ) )
                DeleteFakePlayer( );

            // skip checks and start the game

            ReCalculateTeams( );
            StartCountDown( true );
            return HideCommand;
        }

        //
        // !STARTN
        // !STARTNOW
        // !SN
        //

        else if( ( Command == "startn" || Command == "startnow" || Command == "sn" ) && !m_CountDownStarted )
        {
            if( GetTicks( ) - m_LastLeaverTicks < 300 )
            {
                SendAllChat( gLanguage->SomeOneJustLeft( ) );
                return HideCommand;
            }

            if( m_UNM->m_FakePlayersLobby && !m_FakePlayers.empty( ) )
                DeleteFakePlayer( );

            // skip checks and start the game right now

            ReCalculateTeams( );
            m_CountDownStarted = true;
            m_CountDownCounter = 0;
            return HideCommand;
        }

        //
        // !START
        // !СТАРТ
        // !НАЧАТЬ
        //

        else if( ( Command == "start" || Command == "старт" || Command == "начать" ) && !m_CountDownStarted )
        {
            if( GetTicks( ) - m_LastLeaverTicks < 300 )
            {
                SendAllChat( gLanguage->SomeOneJustLeft( ) );
                return HideCommand;
            }

            if( m_UNM->m_FakePlayersLobby && !m_FakePlayers.empty( ) )
                DeleteFakePlayer( );

            ReCalculateTeams( );

            // if the player sent "!start force" skip the checks and start the countdown
            // else if the player sent "!start now" skip checks and start the game right now
            // otherwise check that the game is ready to start

            if( PayloadLower == "force" )
                StartCountDown( true );
            else if( PayloadLower == "now" )
            {

                m_CountDownStarted = true;
                m_CountDownCounter = 0;
            }
            else
                StartCountDown( false );

            return HideCommand;
        }

        //
        // !SWAP (swap slots)
        // !СВАП
        // !СВАПНУТЬ
        // !ПОМЕНЯТЬ
        // !МЕНЯТЬ
        // !ЗАМЕНА
        // !ЗАМЕНИТЬ
        //

        else if( ( Command == "swap" || Command == "свап" || Command == "свапнуть" || Command == "поменять" || Command == "менять" || Command == "замена" || Command == "заменить" ) && !m_GameLoading && !m_GameLoaded )
        {
            uint32_t SID1;
            uint32_t SID2;
            stringstream SS;
            SS << Payload;
            SS >> SID1;

            if( SS.fail( ) )
                SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot1> <slot2>" );
            else
            {
                if( SS.eof( ) )
                    SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot1> <slot2>" );
                else
                {
                    SS >> SID2;

                    if( SS.fail( ) )
                        SendChat( player, "Некорректный ввод команды. Правильное использование: " + string( 1, m_UNM->m_CommandTrigger ) + Command + " <slot1> <slot2>" );
                    else
                        SwapSlots( static_cast<unsigned char>( SID1 - 1 ), static_cast<unsigned char>( SID2 - 1 ) );
                }
            }

            return HideCommand;
        }

        //
        // !SYNCLIMIT (set game synclimit)
        // !SYNC
        //

        else if( Command == "synclimit" || Command == "sync" )
        {
            uint64_t checkSyncLimit = UTIL_ToUInt64( Payload );
            uint32_t minSyncLimit = m_UNM->m_MinSyncLimit;
            uint32_t maxSyncLimit = m_UNM->m_MaxSyncLimit;

            if( Payload.empty( ) )
                SendAllChat( gLanguage->SyncLimitIs( UTIL_ToString( m_SyncLimit ) ) );
            else if( Payload.find_first_not_of( "0123456789" ) != string::npos )
                SendChat( player, "херню ввел, вводи только числа " + UTIL_ToString( minSyncLimit ) + "-" + UTIL_ToString( maxSyncLimit ) );
            else
            {
                if( checkSyncLimit < minSyncLimit )
                {
                    m_SyncLimit = minSyncLimit;
                    SendAllChat( gLanguage->SettingSyncLimitToMinimum( UTIL_ToString( minSyncLimit ) ) );
                }
                else if( checkSyncLimit > maxSyncLimit )
                {
                    m_SyncLimit = maxSyncLimit;
                    SendAllChat( gLanguage->SettingSyncLimitToMaximum( UTIL_ToString( maxSyncLimit ) ) );
                }
                else
                {
                    m_SyncLimit = (uint32_t)checkSyncLimit;
                    SendAllChat( gLanguage->SettingSyncLimitTo( UTIL_ToString( m_SyncLimit ) ) );
                }
            }

            return HideCommand;
        }

        //
        // !UNHOST
        // !UH
        // !АНХОСТ
        //

        else if( ( Command == "unhost" || Command == "uh" || Command == "анхост" ) && !m_CountDownStarted )
        {
            m_Exiting = true;
        }

        //
        // !COMMANDS
        // !COMMANDSGAME
        // !GAMECOMMANDS
        // !CMDS
        //

        else if( Command == "commands" || Command == "commandsgame" || Command == "gamecommands" || Command == "cmds" )
        {
            m_NonAdminCommandsGame = !m_NonAdminCommandsGame;

            if( m_UNM->m_NonAdminCommandsGame )
                SendAllChat( "Non-admin commands ON" );
            else
                SendAllChat( "Non-admin commands OFF" );
        }

        //
        // !DOWNLOADS
        // !NODL
        // !NODOWNLOADS
        // !DLDS
        //

        else if( Command == "downloads" || Command == "nodl" || Command == "nodownloads" || Command == "dlds" )
        {
            if( m_GameLoaded || m_GameLoading )
            {
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна в начатой игре!" );
                return HideCommand;
            }

            if( m_AllowDownloads == 3 )
            {
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " отключена, скачивание карт запрещено!" );
                return HideCommand;
            }

            if( Payload.empty( ) )
            {
                if( m_AllowDownloads == 1 )
                {
                    m_AllowDownloads = 0;
                    SendAllChat( "Скачивание карт запрещено" );
                }
                else
                {
                    m_AllowDownloads = 1;
                    SendAllChat( "Скачивание карт разрешено" );
                }
            }
            else
            {
                if( PayloadLower == "0" || PayloadLower == "off" )
                {
                    SendAllChat( gLanguage->MapDownloadsDisabled( ) );
                    m_AllowDownloads = 0;
                }
                else if( PayloadLower == "1" || PayloadLower == "on" )
                {
                    SendAllChat( gLanguage->MapDownloadsEnabled( ) );
                    m_AllowDownloads = 1;
                }
                else if( PayloadLower == "2" || PayloadLower == "alternative" || PayloadLower == "alt" )
                {
                    SendAllChat( gLanguage->MapDownloadsConditional( ) );
                    m_AllowDownloads = 2;
                }
                else
                    SendChat( player, "Используй значения от 0 до 2" );
            }
        }

        //
        // !OVERRIDE
        // !OR
        //

        else if( Command == "override" || Command == "or" )
        {
            m_GameOverCanceled = !m_GameOverCanceled;

            if( m_GameOverCanceled )
            {
                SendAllChat( "Game over timer canceled" );
                m_GameOverTime = 0;
            }
        }

        //
        // !RECALC
        // !RCT
        // !RC
        //

        else if( Command == "recalc" || Command == "rct" || Command == "rc" )
        {
            ReCalculateTeams( );
            SendAllChat( "Teams recalculated!" );
        }

        //
        // !GAMENAME
        // !GN
        // !GAMENAMETEST
        // !GNTEST
        //

        else if( Command == "gamename" || Command == "gn" || Command == "gamenametest" || Command == "gntest" )
        {
            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            {
                string gamename;

                if( Command == "gamenametest" || Command == "gntest" || PayloadLower == "test" || m_GameLoading || m_GameLoaded )
                    gamename = m_UNM->IncGameNrDecryption( m_GameName );
                else
                    gamename = GetValidGameName( ( *i )->GetJoinedRealm( ) );

                SendChat( ( *i )->GetPID( ), "Текущее название игры: \"" + gamename + "\"" );
            }

            return HideCommand;
        }

        //
        // !SERVER
        // !SERV
        // !REALM
        // !СЕРВЕР
        // !РЕАЛМ
        //

        else if( Command == "server" || Command == "serv" || Command == "realm" || Command == "сервер" || Command == "реалм" )
        {
            string servermsg = string( );
            string serveralias = string( );

            if( !Payload.empty( ) )
            {
                CGamePlayer *LastMatch = nullptr;
                uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

                if( Matches == 0 )
                    servermsg = gLanguage->UnableToCheckPlayerNoMatchesFound( Payload );
                else if( Matches == 1 )
                {
                    servermsg = LastMatch->GetName( );
                    servermsg += ": ";
                    serveralias = LastMatch->GetJoinedRealm( ).empty( ) ? "N/A" : LastMatch->GetJoinedRealm( );

                    if( serveralias != "N/A" )
                    {
                        for( vector<CBNET *> ::iterator j = m_UNM->m_BNETs.begin( ); j != m_UNM->m_BNETs.end( ); j++ )
                        {
                            if( ( *j )->GetServer( ) == serveralias )
                            {
                                serveralias = ( *j )->GetServerAlias( );
                                break;
                            }
                        }
                    }

                    servermsg += serveralias;
                }
                else
                    servermsg = gLanguage->UnableToCheckPlayerFoundMoreThanOneMatch( Payload );
            }
            else
            {
                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                {
                    servermsg += ( *i )->GetName( );
                    servermsg += ": ";
                    serveralias = ( *i )->GetJoinedRealm( ).empty( ) ? "N/A" : ( *i )->GetJoinedRealm( );

                    if( serveralias != "N/A" )
                    {
                        for( vector<CBNET *> ::iterator j = m_UNM->m_BNETs.begin( ); j != m_UNM->m_BNETs.end( ); j++ )
                        {
                            if( ( *j )->GetServer( ) == serveralias )
                            {
                                serveralias = ( *j )->GetServerAlias( );
                                break;
                            }
                        }
                    }

                    servermsg += serveralias;

                    if( i != m_Players.end( ) - 1 )
                        servermsg += ", ";
                }

            }

            SendAllChat( servermsg );
            return HideCommand;
        }

        //
        // !VIRTUALHOST
        // !VH
        //

        else if( ( Command == "virtualhost" || Command == "vh" ) && !m_CountDownStarted )
        {
            string vhn = string( );

            if( !Payload.empty( ) )
                vhn = UTIL_SubStrFix( Payload, 0, 15 );
            if( vhn.empty( ) )
                SendChat( player, "Текущее имя виртуального хоста: [" + m_VirtualHostName + "]" );
            else
            {
                DeleteVirtualHost( );
                m_VirtualHostName = vhn;
                SendChat( player, "Имя виртуального хоста изменено!" );
            }

            return HideCommand;
        }

        //
        // !VOTECANCEL
        // !VOTEKICKCANCEL
        //

        else if( ( Command == "votecancel" || Command == "votekickcancel" ) && !m_KickVotePlayer.empty( ) )
        {
            SendAllChat( gLanguage->VoteKickCancelled( m_KickVotePlayer ) );
            m_KickVotePlayer.clear( );
            m_StartedKickVoteTime = 0;
        }

        //
        // !DELETEVIRTUALHOST
        // !DONTDELETEVIRTUALHOST
        // !DVH
        // !DVP
        // !REMOVEVIRTUALHOST
        // !DONTREMOVEVIRTUALHOST
        // !RVH
        // !RVP
        // !SOLO
        //

        else if( Command == "deletevirtualhost" || Command == "dontdeletevirtualhost" || Command == "dvh" || Command == "dvp" || Command == "removevirtualhost" || Command == "dontremovevirtualhost" || Command == "rvh" || Command == "rvp" || Command == "solo" )
        {
            if( m_GameLoading || m_GameLoaded )
                SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна в начатой игре!" );
            else if( m_DontDeleteVirtualHost )
            {
                m_DontDeleteVirtualHost = false;
                SendChat( player->GetPID( ), "Виртуальный хост будет удален, если игра начнется с 1 игроком." );
            }
            else
            {
                m_DontDeleteVirtualHost = true;
                SendChat( player->GetPID( ), "Виртуальный хост не будет удален, если игра начнется с 1 игроком." );
            }
        }

        //
        // !DELETELASTPLAYER
        // !DONTDELETELASTPLAYER
        // !DLP
        // !REMOVELASTPLAYER
        // !DONTREMOVELASTPLAYER
        // !RLP
        //

        else if( Command == "deletelastplayer" || Command == "dontdeletelastplayer" || Command == "dlp" || Command == "removelastplayer" || Command == "dontremovelastplayer" || Command == "rlp" )
        {
            if( m_DontDeletePenultimatePlayer )
            {
                m_DontDeletePenultimatePlayer = false;
                SendChat( player->GetPID( ), "Предпоследний игрок, покинувший игру, не будет заменен фейк-плеером." );
                SendChat( player->GetPID( ), "Предупреждение: после лива предпоследнего игрока команды бота перестанут работать в данной игре!" );
            }
            else
            {
                m_DontDeletePenultimatePlayer = true;
                SendChat( player->GetPID( ), "Предпоследний игрок, покинувший игру, будет заменен фейк-плеером. Команды бота в игре останутся доступными!" );
            }
        }

        //
        // !FUNCOMMANDS
        // !FUNCOMMANDSGAME
        //

        else if( Command == "funcommands" || Command == "funcommandsgame" )
        {
            m_FunCommandsGame = !m_FunCommandsGame;

            if( m_FunCommandsGame )
                SendAllChat( "Фан-команды включены!" );
            else
                SendAllChat( "Фан-команды отключены!" );
        }

    }
	else
	{
		if( !player->GetSpoofed( ) )
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] non-spoofchecked user [" + User + "] sent command [" + Command + "] with payload [" + Payload + "]", 6, m_CurrentGameCounter );
		else
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] non-admin [" + User + "] sent command [" + Command + "] with payload [" + Payload + "]", 6, m_CurrentGameCounter );

        if( !m_UNM->m_NonAdminCommandsGame && Command != "yes" && Command != "y" && Command != "votekick" && Command != "vk" && Command != "rmk" && Command != "remake" )
            return HideCommand;
	}

	/*********************
	* NON ADMIN COMMANDS *
	*********************/

	//
	// !OWNER (set game owner)
	// !ADMIN
	//

	if( Command == "owner" || Command == "admin" )
	{
        if( OwnerCheck || DefaultOwnerCheck )
		{
            if( Payload.empty( ) )
                SendAllChat( CurrentOwners( ) );
			else
			{
				if( Payload.size( ) > 15 )
				{
					SendChat( player->GetPID( ), "Указан слишком длинный ник (максимум 15 символов)" );
					return HideCommand;
				}

				string sUser = Payload;
				CGamePlayer *LastMatch = nullptr;
				uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

                if( Matches > 1 )
                {
                    SendChat( player->GetPID( ), gLanguage->UnableToCheckPlayerFoundMoreThanOneMatch( Payload ) );
                    return HideCommand;
                }
				else if( Matches == 1 )
					sUser = LastMatch->GetName( );
				else
					sUser = UTIL_SubStrFix( sUser, 0, 15 );

                if( IsOwner( sUser ) )
                {
                    if( sUser == User )
                        SendChat( player->GetPID( ), "Вы уже являетесь владельцем игры!" );
                    else
                        SendChat( player->GetPID( ), "Игрок [" + sUser + "] уже является владельцем игры!" );

                    return HideCommand;
                }

                if( m_UNM->m_OptionOwner == 0 )
                {
                    SendChat( player->GetPID( ), "Команда " + string( 1, m_UNM->m_CommandTrigger ) + "Owner отключена!" );
                    return HideCommand;
                }
                else if( m_UNM->m_OptionOwner == 1 )
                {
                    SendAllChat( gLanguage->SettingGameOwnerTo( sUser ) );
                    m_OwnerName = sUser;
                }
                else if( m_UNM->m_OptionOwner == 2 )
                {
                    SendAllChat( gLanguage->SettingGameOwnerTo( sUser ) );

                    if( !IsDefaultOwner( sUser ) && m_DefaultOwner != m_VirtualHostName )
                        m_OwnerName = m_DefaultOwner + " " + sUser;
                    else
                        m_OwnerName = sUser;
                }
                else if( m_UNM->m_OptionOwner == 3 )
                {
                    SendAllChat( gLanguage->SettingGameOwnerTo( sUser ) );
                    m_OwnerName += " " + sUser;
                }
                else if( m_UNM->m_OptionOwner == 4 )
                {
                    SendAllChat( gLanguage->SettingGameOwnerTo( sUser ) );
                    m_OwnerName = User + " " + sUser;
                }
			}
		}
		else
            SendChat( player->GetPID( ), CurrentOwners( ) );
	}

	//
	// !УТК
	// !УТКА
	// !DUCK
	// !UTK
	// !YTK
	//

	else if( Command == "утк" || Command == "утка" || Command == "duck" || Command == "utk" || Command == "ytk" )
	{
        if( GetTime( ) - player->GetLastPicture( ) < m_UNM->m_PictureGameDelay )
            SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            player->SetLastPicture( GetTime( ) );

            if( PayloadLower == "2" || PayloadLower == "specular" || PayloadLower == "reversed" || PayloadLower == "mirror" )
            {
                SendAllChat( "    __( ^ )>" );
                SendAllChat( "~(_____)" );
                SendAllChat( "      |_ |_" );
            }
            else
            {
                SendAllChat( "<( ^ )__" );
                SendAllChat( " (_____)~" );
                SendAllChat( "  _/_/" );
            }
        }
	}

	//
	// !КТУ
	// !УТК2
	// !УТКА2
	// !KCUD
	// !DUCK2
	// !KTU
	// !KTY
	// !UTK2
	// !YTK2
	//

	else if( Command == "кту" || Command == "утк2" || Command == "утка2" || Command == "kcud" || Command == "duck2" || Command == "ktu" || Command == "kty" || Command == "utk2" || Command == "ytk2" )
	{
        if( GetTime( ) - player->GetLastPicture( ) < m_UNM->m_PictureGameDelay )
            SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            player->SetLastPicture( GetTime( ) );
            SendAllChat( "    __( ^ )>" );
            SendAllChat( "~(_____)" );
            SendAllChat( "      |_ |_" );
        }
	}

	//
	// !МЫШЬ
	// !MOUSE
	//

	else if( Command == "мышь" || Command == "mouse" )
	{
        if( GetTime( ) - player->GetLastPicture( ) < m_UNM->m_PictureGameDelay )
            SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            player->SetLastPicture( GetTime( ) );

            if( PayloadLower == "2" || PayloadLower == "specular" || PayloadLower == "reversed" || PayloadLower == "mirror" )
                SendAllChat( "<\"__)~" );
            else
                SendAllChat( "~(__\">" );
        }
	}

	//
	// !МЫШЬ2
	// !MOUSE2
	//

	else if( Command == "мышь2" || Command == "mouse2" )
	{
        if( GetTime( ) - player->GetLastPicture( ) < m_UNM->m_PictureGameDelay )
            SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            player->SetLastPicture( GetTime( ) );
            SendAllChat( "<\"__)~" );
        }
	}

	//
	// !ЗАЯЦ
	// !ЗАЙ
	// !ЗАЯ
	// !ЗАЙЧИК
	// !RABBIT
	// !BUNNY
	// !HARE
	//

	else if( Command == "заяц" || Command == "зай" || Command == "зая" || Command == "зайчик" || Command == "rabbit" || Command == "bunny" || Command == "hare" )
	{
        if( GetTime( ) - player->GetLastPicture( ) < m_UNM->m_PictureGameDelay )
            SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            player->SetLastPicture( GetTime( ) );

            if( PayloadLower == "2" || PayloadLower == "3" || PayloadLower == "два" || PayloadLower == "три" || PayloadLower == "two" || PayloadLower == "three" )
            {
                SendAllChat( "---/)/)---(\\.../)---(\\(\\" );
                SendAllChat( "--(':'=)---(=';'=)---(=':')" );
                SendAllChat( "(\")(\")..)-(\").--.(\")-(..(\")(\")" );
            }
            else
            {
                SendAllChat( "(\\__/)" );
                SendAllChat( " (='.'=)" );
                SendAllChat( "(\")_(\")" );
            }
        }
	}

	//
	// !ЗАЙЦЫ
	// !ЗАИ
	// !ЗАЙИ
	// !ЗАЙЧИКИ
	// !RABBITS
	// !BUNNYS
	// !HARES
	//

	else if( Command == "зайцы" || Command == "заи" || Command == "зайи" || Command == "зайчики" || Command == "rabbits" || Command == "bunnys" || Command == "hares" )
	{
        if( GetTime( ) - player->GetLastPicture( ) < m_UNM->m_PictureGameDelay )
            SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            player->SetLastPicture( GetTime( ) );
            SendAllChat( "---/)/)---(\\.../)---(\\(\\" );
            SendAllChat( "--(':'=)---(=';'=)---(=':')" );
            SendAllChat( "(\")(\")..)-(\").--.(\")-(..(\")(\")" );
        }
	}

	//
	// !БЕЛКА
	// !БЕЛОЧКА
	// !SQUIRREL
	//

	else if( Command == "белка" || Command == "белочка" || Command == "squirrel" )
	{
        if( GetTime( ) - player->GetLastPicture( ) < m_UNM->m_PictureGameDelay )
            SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            player->SetLastPicture( GetTime( ) );
            SendAllChat( "( \\" );
            SendAllChat( "(_ \\  ( '>" );
            SendAllChat( "   ) \\/_)=" );
            SendAllChat( "   (_(_ )_" );
        }
	}

	//
	// !КРАБ
	// !КРАБИК
	// !CRAB
	//

	else if( Command == "краб" || Command == "крабик" || Command == "crab" )
	{
        if( GetTime( ) - player->GetLastPicture( ) < m_UNM->m_PictureGameDelay )
            SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            player->SetLastPicture( GetTime( ) );
            SendAllChat( "(\\/)_0_0_(\\/)" );
        }
	}

	//
	// !DATE
	// !TIME
	//

	else if( Command == "date" || Command == "time" )
	{
		struct tm * timeinfo;
		char buffer[150];
		string sDate;
		time_t Now = time( nullptr );
		timeinfo = localtime( &Now );
		strftime( buffer, 150, "Date: %d %B %Y Time: %H:%M:%S", timeinfo );
		sDate = buffer;
		SendChat( player, sDate );
	}

	//
	// !GAMETIME
	// !GT
	//

	else if( ( Command == "gametime" || Command == "gt" ) && m_GameLoaded )
	{
		string MinString = UTIL_ToString( ( m_GameTicks / 1000 ) / 60 );
		string SecString = UTIL_ToString( ( m_GameTicks / 1000 ) % 60 );

		if( SecString.size( ) == 1 )
			SecString.insert( 0, "0" );

		SendChat( player, "Текущее время игры: " + MinString + "м. " + SecString + "с." );
	}

	//
	// !GAMENAME
	// !GN
    // !GAMENAMETEST
    // !GNTEST
	//

    else if( Command == "gamename" || Command == "gn" || Command == "gamenametest" || Command == "gntest" )
    {
        string gamename;

        if( Command == "gamenametest" || Command == "gntest" || PayloadLower == "test" || m_GameLoading || m_GameLoaded )
            gamename = m_UNM->IncGameNrDecryption( m_GameName );
        else
            gamename = GetValidGameName( UserServer );

        SendChat( player, "Текущее название игры: \"" + gamename + "\"" );
    }

	//
	// !GAMES
	// !INFOGAMES
	// !PLAYERS
	//

	else if( Command == "games" || Command == "infogames" || Command == "players" )
	{
		uint32_t NowOnline = 0;

		for( uint32_t i = 0; i < m_UNM->m_Games.size( ); i++ )
            NowOnline += m_UNM->m_Games[i]->GetNumHumanPlayers( );

		if( m_UNM->m_CurrentGame )
            NowOnline += m_UNM->m_CurrentGame->GetNumHumanPlayers( );

        SendChat( player, "Активных игр [" + UTIL_ToString( m_UNM->m_Games.size( ) ) + "] → Игроков онлайн [" + UTIL_ToString( NowOnline ) + "]" );
	}

	//
	// !VIRTUALHOST
	// !VH
	//

	else if( ( Command == "virtualhost" || Command == "vh" ) && !m_CountDownStarted )
        SendChat( player, "Текущее имя виртуального хоста: [" + m_VirtualHostName + "]" );

	//
	// !HELP
	// !ПОМОЩЬ
	//

	else if( Command == "help" || Command == "помощь" )
	{
        SendChat( player, "Юзер-команды: " + string( 1, m_UNM->m_CommandTrigger ) + "p " + string( 1, m_UNM->m_CommandTrigger ) + "lat " + string( 1, m_UNM->m_CommandTrigger ) + "s " + string( 1, m_UNM->m_CommandTrigger ) + "cm " + string( 1, m_UNM->m_CommandTrigger ) + "ignore " + string( 1, m_UNM->m_CommandTrigger ) + "stats " + string( 1, m_UNM->m_CommandTrigger ) + "sd " + string( 1, m_UNM->m_CommandTrigger ) + "serv " + string( 1, m_UNM->m_CommandTrigger ) + "from " + string( 1, m_UNM->m_CommandTrigger ) + "gn " + string( 1, m_UNM->m_CommandTrigger ) + "ff " + string( 1, m_UNM->m_CommandTrigger ) + "rmk " + string( 1, m_UNM->m_CommandTrigger ) + "vk " + string( 1, m_UNM->m_CommandTrigger ) + "vs" );
	}

    //
    // !INFO
    // !RULES
    //

    else if( Command == "info" || Command == "rules" )
	{
        // read from info.txt if available

        string file;
        ifstream in;

#ifdef WIN32
        if( ( OwnerCheck || DefaultOwnerCheck ) && PayloadLower != "all" )
            file = "text_files\\" + Command + "_admin.txt";
        else
            file = "text_files\\" + Command + ".txt";

        in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
        if( ( AdminCheck || RootAdminCheck ) && PayloadLower != "all" )
            file = "text_files/" + Command + "_admin.txt";
        else
            file = "text_files/" + Command + ".txt";

        in.open( file );
#endif

        if( !in.fail( ) )
        {
            // don't print more than 30 lines

            uint32_t Count = 0;
            string Line;

            while( !in.eof( ) && Count < 30 )
            {
                getline( in, Line );
                Line = UTIL_FixFileLine( Line );

                if( ( OwnerCheck || DefaultOwnerCheck ) && PayloadLower == "all" )
                {
                    if( !Line.empty( ) )
                        SendChat( player, Line );
                    else if( !in.eof( ) )
                        SendChat( player, " " );
                }
                else if( !Line.empty( ) )
                    SendAllChat( Line );
                else if( !in.eof( ) )
                    SendAllChat( " " );

                ++Count;
            }
        }
        else
        {
            CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] warning - unable to read file [" + file + "]", 6, m_CurrentGameCounter );
            SendChat( player, "Не удалось открыть файл [" + file + "]" );
        }

        in.close( );
	}

	//
	// !IMAGE
	// !PICTURE
	// !IMG
	// !PIC
	//

    else if( ( Command == "image" || Command == "picture" || Command == "img" || Command == "pic" ) && ( OwnerCheck || DefaultOwnerCheck ) )
	{
		string file;

#ifdef WIN32
		file = "text_files\\text_images\\";
#else
		file = "text_files/text_images/";
#endif

		if( Payload.empty( ) )
			file += "image.txt";
		else if( Payload.find_first_not_of( "0123456789" ) != string::npos )
		{
			SendChat( player, "Некорректный ввод команды, используйте \"" + string( 1, m_UNM->m_CommandTrigger ) + Command + " <ImageNumber>\"" );
			return HideCommand;
		}
		else
		{
			uint32_t filenumber;
			stringstream SS;
			SS << Payload;
			SS >> filenumber;
			if( SS.fail( ) )
			{
				SendChat( player, "Некорректный ввод команды, используйте \"" + string( 1, m_UNM->m_CommandTrigger ) + Command + " <ImageNumber>\"" );
				return HideCommand;
			}
			else
				file += "image" + UTIL_ToString( filenumber ) + ".txt";
		}

		// read from file if available

        std::ifstream in;

#ifdef WIN32
        in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
        in.open( file );
#endif

		if( !in.fail( ) )
		{
			// don't print more than 200 lines

			uint32_t Count = 0;
			string Line;

			while( !in.eof( ) && Count < 200 )
			{
				getline( in, Line );
                Line = UTIL_FixFileLine( Line );

				if( Line.empty( ) )
				{
					if( !in.eof( ) )
						SendAllChat( " " );
				}
				else
					SendAllChat( Line );

				++Count;
			}
		}
		else
            SendChat( player, "Не удалось открыть файл [" + file + "]" );

		in.close( );
    }

    //
    // !BUTILOCHKA
    // !BUT
    // !БУТЫЛОЧКА
    // !БУТ
    //

    else if( Command == "butilochka" || Command == "but" || Command == "бутылочка" || Command == "бут" )
    {
        if( !m_FunCommandsGame )
            SendChat( player, "Фан-команды отключены!" );
        else if( GetTime( ) - player->GetLastBut( ) < m_UNM->m_ButGameDelay )
            SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            vector<string> randoms;
            string nam1 = User;
            string nam2 = Payload;
            string nam3 = string( );
            CGamePlayer *Player = GetPlayerFromNamePartial( Payload );

            // if no user is specified, randomize one != with the user giving the command

            if( nam2.empty( ) || Player == nullptr )
            {
                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    randoms.push_back( ( *i )->GetName( ) );

                if( randoms.empty( ) )
                {
                    if( nam1 != m_UNM->m_VirtualHostName )
                        randoms.push_back( m_UNM->m_VirtualHostName );

                    if( randoms.empty( ) )
                        randoms.push_back( "unm" );
                }

                std::random_device rand_dev;
                std::mt19937 rng( rand_dev( ) );
                shuffle( randoms.begin( ), randoms.end( ), rng );
                nam2 = randoms[0];
                randoms.clear( );
            }
            else
                nam2 = Player->GetName( );

            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            {
                if( ( *i )->GetName( ) != nam1 && ( *i )->GetName( ) != nam2 )
                    randoms.push_back( ( *i )->GetName( ) );
            }

            if( randoms.empty( ) )
            {
                if( nam1 != m_UNM->m_VirtualHostName && nam2 != m_UNM->m_VirtualHostName )
                    randoms.push_back( m_UNM->m_VirtualHostName );

                if( randoms.empty( ) )
                    randoms.push_back( "unm" );
            }

            std::random_device rand_dev;
            std::mt19937 rng( rand_dev( ) );
            shuffle( randoms.begin( ), randoms.end( ), rng );
            nam3 = randoms[0];
            player->SetLastBut( GetTime( ) );
            string msg = m_UNM->GetBut( );
            UTIL_Replace( msg, "$USER$", nam1 );
            UTIL_Replace( msg, "$VICTIM$", nam2 );
            UTIL_Replace( msg, "$RANDOM$", nam3 );
            SendAllChat( msg );
        }
    }

    //
    // !MARS
    //

    else if( Command == "mars" )
    {
        if( !m_FunCommandsGame )
            SendChat( player, "Фан-команды отключены!" );
        else if( GetTime( ) - player->GetLastMars( ) < m_UNM->m_MarsGameDelay )
            SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            vector<string> randoms;
            string nam1 = User;
            string nam2 = Payload;
            string nam3 = string( );
            CGamePlayer *Player = GetPlayerFromNamePartial( Payload );

            // if no user is specified, randomize one != with the user giving the command

            if( nam2.empty( ) || Player == nullptr )
            {
                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    randoms.push_back( ( *i )->GetName( ) );

                if( randoms.empty( ) )
                {
                    if( nam1 != m_UNM->m_VirtualHostName )
                        randoms.push_back( m_UNM->m_VirtualHostName );

                    if( randoms.empty( ) )
                        randoms.push_back( "unm" );
                }

                std::random_device rand_dev;
                std::mt19937 rng( rand_dev( ) );
                shuffle( randoms.begin( ), randoms.end( ), rng );
                nam2 = randoms[0];
                randoms.clear( );
            }
            else
                nam2 = Player->GetName( );

            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            {
                if( ( *i )->GetName( ) != nam1 && ( *i )->GetName( ) != nam2 )
                    randoms.push_back( ( *i )->GetName( ) );
            }

            if( randoms.empty( ) )
            {
                if( nam1 != m_UNM->m_VirtualHostName && nam2 != m_UNM->m_VirtualHostName )
                    randoms.push_back( m_UNM->m_VirtualHostName );

                if( randoms.empty( ) )
                    randoms.push_back( "unm" );
            }

            std::random_device rand_dev;
            std::mt19937 rng( rand_dev( ) );
            shuffle( randoms.begin( ), randoms.end( ), rng );
            nam3 = randoms[0];
            player->SetLastMars( GetTime( ) );
            string msg = m_UNM->GetMars( );
            UTIL_Replace( msg, "$USER$", nam1 );
            UTIL_Replace( msg, "$VICTIM$", nam2 );
            UTIL_Replace( msg, "$RANDOM$", nam3 );
            SendAllChat( msg );
        }
    }

	//
	// !SLAP
	//

	else if( Command == "slap" )
	{
		if( !m_FunCommandsGame )
			SendChat( player, "Фан-команды отключены!" );
        else if( GetTime( ) - player->GetLastSlap( ) < m_UNM->m_SlapGameDelay )
			SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            vector<string> randoms;
            string nam1 = User;
            string nam2 = Payload;
            string nam3 = string( );
            CGamePlayer *Player = GetPlayerFromNamePartial( Payload );

            // if no user is specified, randomize one != with the user giving the command

            if( nam2.empty( ) || Player == nullptr )
            {
                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
                    randoms.push_back( ( *i )->GetName( ) );

                if( randoms.empty( ) )
                {
                    if( nam1 != m_UNM->m_VirtualHostName )
                        randoms.push_back( m_UNM->m_VirtualHostName );

                    if( randoms.empty( ) )
                        randoms.push_back( "unm" );
                }

                std::random_device rand_dev;
                std::mt19937 rng( rand_dev( ) );
                shuffle( randoms.begin( ), randoms.end( ), rng );
                nam2 = randoms[0];
                randoms.clear( );
            }
            else
                nam2 = Player->GetName( );

            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            {
                if( ( *i )->GetName( ) != nam1 && ( *i )->GetName( ) != nam2 )
                    randoms.push_back( ( *i )->GetName( ) );
            }

            if( randoms.empty( ) )
            {
                if( nam1 != m_UNM->m_VirtualHostName && nam2 != m_UNM->m_VirtualHostName )
                    randoms.push_back( m_UNM->m_VirtualHostName );

                if( randoms.empty( ) )
                    randoms.push_back( "unm" );
            }

            std::random_device rand_dev;
            std::mt19937 rng( rand_dev( ) );
            shuffle( randoms.begin( ), randoms.end( ), rng );
            nam3 = randoms[0];
            player->SetLastSlap( GetTime( ) );
            string msg = m_UNM->GetSlap( );
            UTIL_Replace( msg, "$USER$", nam1 );
            UTIL_Replace( msg, "$VICTIM$", nam2 );
            UTIL_Replace( msg, "$RANDOM$", nam3 );
            SendAllChat( msg );
        }
	}

	//
	// !ROLL
	//

	else if( Command == "roll" )
	{
        if( IsMuted( player->GetName( ) ) || player->GetMuted( ) || IsCensorMuted( player->GetName( ) ) > 0 )
			SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " не доступна для игроков в муте!" );
        else if( GetTime( ) - player->GetLastRoll( ) < m_UNM->m_RollGameDelay )
			SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
        else
        {
            player->SetLastRoll( GetTime( ) );

            if( Payload.empty( ) )
                SendAllChat( User + " rolled " + UTIL_ToString( UTIL_GenerateRandomNumberUInt32( 1, 100 ) ) + " из 100" );
            else if( Payload.find_first_not_of( "0123456789" ) == string::npos && UTIL_ToUInt64( Payload ) >= 1 )
                SendAllChat( User + " rolled " + UTIL_ToString( UTIL_GenerateRandomNumberUInt64( 1, UTIL_ToUInt64( Payload ) ) ) + " из " + UTIL_ToString( UTIL_ToUInt64( Payload ) ) );
            else
                SendAllChat( User + " rolled " + UTIL_ToString( UTIL_GenerateRandomNumberUInt32( 1, 100 ) ) + " из 100, for \"" + Payload + "\"" );
        }
	}

	//
	// !T9
	// !MSGCORRECT
	// !MSGEDIT
	// !MSGAUTOCORRECT
	// !MSGAUTOEDIT
	//

	else if( Command == "t9" || Command == "msgcorrect" || Command == "msgedit" || Command == "msgautocorrect" || Command == "msgautoedit" )
	{
		if( m_UNM->m_MsgAutoCorrectMode != 2 && m_UNM->m_MsgAutoCorrectMode != 3 )
            SendChat( player, gLanguage->CommandDisabled( string( 1, m_UNM->m_CommandTrigger ) + Command ) );
        else if( player->m_MsgAutoCorrect )
		{
			player->m_MsgAutoCorrect = false;
			SendChat( player, "Автоисправление ошибок отключено!" );
		}
		else
		{
			player->m_MsgAutoCorrect = true;
			SendChat( player, "Автоисправление ошибок включено!" );
		}
	}

	//
	// !IGNORE
	//

	else if( Command == "ignore" && !Payload.empty( ) )
	{
		CGamePlayer *LastMatch = nullptr;
		uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

		if( Matches == 0 )
			SendChat( player, "Unable to ignore player [" + Payload + "]. No matches found." );
		else if( Matches == 1 )
		{
			player->Ignore( LastMatch->GetName( ) );
			SendChat( player, "You have ignored player [" + LastMatch->GetName( ) + "]. You will not be able to send or receive messages from the player." );
		}
		else
			SendChat( player, "Unable to ignore player [" + Payload + "]. Found more than one match." );
	}

	//
	// !UNIGNORE
	//

	else if( Command == "unignore" && !Payload.empty( ) )
	{
		CGamePlayer *LastMatch = nullptr;
		uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

		if( Matches == 0 )
			SendChat( player, "Unable to unignore player [" + Payload + "]. No matches found." );
		else if( Matches == 1 )
		{
			player->UnIgnore( LastMatch->GetName( ) );
			SendChat( player, "You have unignored player [" + LastMatch->GetName( ) + "]." );
		}
		else
			SendChat( player, "Unable to unignore player [" + Payload + "]. Found more than one match." );
	}

	//
	// !IGNOREALL
	// !IA
	//

	else if( Command == "ignoreall" || Command == "ia" )
	{
		for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
			player->Ignore( ( *i )->GetName( ) );

		SendChat( player, "Ты теперь игнорируешь всех игроков. !uia - снять игнор." );
	}

	//
	// !UNIGNOREALL
	// !UIA
	//

	else if( Command == "unignoreall" || Command == "uia" )
	{
		for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
			player->UnIgnore( ( *i )->GetName( ) );

		SendChat( player, "Ты больше не игнорируешь других игроков." );
	}

	//
	// !LATENCY (set game latency)
	// !LAT
	// !L
	// !DELAY
	// !ЛАТЕНСИ
	// !ЗАДЕРЖКА
	// !ДЕЛЕЙ
	// !ЛАТ
	// !Л
	//

	else if( Command == "latency" || Command == "lat" || Command == "l" || Command == "delay" || Command == "латенси" || Command == "задержка" || Command == "делей" || Command == "лат" || Command == "л" )
	{
        if( m_Players.size( ) == 1 )
        {
            uint64_t checkLatency = UTIL_ToUInt64( Payload );

            if( Payload.empty( ) )
            {
                if( m_UseDynamicLatency )
                    SendAllChat( "Dynamic latency is [" + UTIL_ToString( m_Latency ) + "]" );
                else
                    SendAllChat( gLanguage->LatencyIs( UTIL_ToString( m_Latency ) ) );
            }
            else if( Payload.find_first_not_of( "0123456789" ) != string::npos )
            {
                if( Command != "l" || ( !m_GameLoading && !m_GameLoaded ) )
                    SendChat( player, "херню ввел, вводи только числа " + UTIL_ToString( m_UNM->m_MinLatency ) + "-" + UTIL_ToString( m_UNM->m_MaxLatency ) );
            }
            else
            {
                if( m_UseDynamicLatency )
                {
                    m_UseDynamicLatency = false;
                    m_DynamicLatencyLagging = false;
                    m_DynamicLatencyBotLagging = false;
                    SendAllChat( "Динамический латенси отключен, чтобы включить - юзай: " + string( 1, m_UNM->m_CommandTrigger ) + "drd" );
                }

                if( checkLatency < m_UNM->m_MinLatency )
                {
                    m_Latency = m_UNM->m_MinLatency;
                    SendAllChat( gLanguage->SettingLatencyToMinimum( UTIL_ToString( m_Latency ) ) );
                }
                else if( checkLatency > m_UNM->m_MaxLatency )
                {
                    m_Latency = m_UNM->m_MaxLatency;
                    SendAllChat( gLanguage->SettingLatencyToMaximum( UTIL_ToString( m_Latency ) ) );
                }
                else
                {
                    m_Latency = (uint32_t)checkLatency;
                    SendAllChat( gLanguage->SettingLatencyTo( UTIL_ToString( m_Latency ) ) );
                }
            }
        }
        else if( Payload.find_first_not_of( " " ) != string::npos )
        {
            if( m_UseDynamicLatency )
                SendChat( player, "Dynamic latency is [" + UTIL_ToString( m_Latency ) + "]. Изменить latency может только админ." );
            else
                SendChat( player, gLanguage->LatencyIs( UTIL_ToString( m_Latency ) ) + " Изменить latency может только админ." );
        }
		else if( m_UseDynamicLatency )
			SendChat( player, "Dynamic latency is [" + UTIL_ToString( m_Latency ) + "]" );
		else
            SendChat( player, gLanguage->LatencyIs( UTIL_ToString( m_Latency ) ) );
	}

	//
	// !SYNCLIMIT (check game synclimit for users)
	// !SYNC
	//

	else if( Command == "synclimit" || Command == "sync" )
    {
        if( m_Players.size( ) == 1 )
        {
            uint64_t checkSyncLimit = UTIL_ToUInt64( Payload );
            uint32_t minSyncLimit = m_UNM->m_MinSyncLimit;
            uint32_t maxSyncLimit = m_UNM->m_MaxSyncLimit;

            if( Payload.empty( ) )
                SendChat( player->GetPID( ), gLanguage->SyncLimitIs( UTIL_ToString( m_SyncLimit ) ) );
            else if( Payload.find_first_not_of( "0123456789" ) != string::npos )
                SendChat( player, "херню ввел, вводи только числа " + UTIL_ToString( minSyncLimit ) + "-" + UTIL_ToString( maxSyncLimit ) );
            else
            {
                if( checkSyncLimit < minSyncLimit )
                {
                    m_SyncLimit = minSyncLimit;
                    SendChat( player->GetPID( ), gLanguage->SettingSyncLimitToMinimum( UTIL_ToString( minSyncLimit ) ) );
                }
                else if( checkSyncLimit > maxSyncLimit )
                {
                    m_SyncLimit = maxSyncLimit;
                    SendChat( player->GetPID( ), gLanguage->SettingSyncLimitToMaximum( UTIL_ToString( maxSyncLimit ) ) );
                }
                else
                {
                    m_SyncLimit = (uint32_t)checkSyncLimit;
                    SendChat( player->GetPID( ), gLanguage->SettingSyncLimitTo( UTIL_ToString( m_SyncLimit ) ) );
                }
            }
        }
        else if( Payload.find_first_not_of( " " ) != string::npos )
            SendChat( player, gLanguage->SyncLimitIs( UTIL_ToString( m_SyncLimit ) ) + " Изменить synclimit может только админ." );
        else
            SendChat( player, gLanguage->SyncLimitIs( UTIL_ToString( m_SyncLimit ) ) );
    }

	//
	// !PING
	// !P
	// !ПИНГ
	// !П
    // !PINGDETAIL
	//

    else if( Command == "ping" || Command == "p" || Command == "пинг" || Command == "п" || Command == "pingdetail" )
    {
        if( Command == "pingdetail" || PayloadLower == "detail" )
        {
            SendChat( player, player->GetPingDetail( m_UNM->m_LCPings ) );
            return HideCommand;
        }

        string Pings = string( );

        if( !Payload.empty( ) )
        {
            CGamePlayer *LastMatch = nullptr;
            uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

            if( Matches == 0 )
                SendChat( player, "No match found" );
            else if( Matches == 1 )
            {
                Pings = LastMatch->GetName( ) + ": ";

                if( LastMatch->GetNumPings( ) > 0 )
                    Pings += UTIL_ToString( LastMatch->GetPing( m_UNM->m_LCPings ) ) + " ms";
                else
                    Pings += "N/A";

                SendChat( player->GetPID( ), Pings );
            }
            else
                SendChat( player, "Found more than one match" );

            return HideCommand;
        }

        // copy the m_Players vector so we can sort by descending ping so it's easier to find players with high pings

        vector<CGamePlayer *> SortedPlayers = m_Players;
        sort( SortedPlayers.begin( ), SortedPlayers.end( ), CGamePlayerSortDescByPing( ) );

        for( vector<CGamePlayer *> ::iterator i = SortedPlayers.begin( ); i != SortedPlayers.end( ); i++ )
        {
            Pings += ( *i )->GetNameTerminated( ) + ": ";

            if( ( *i )->GetNumPings( ) > 0 )
                Pings += UTIL_ToString( ( *i )->GetPing( m_UNM->m_LCPings ) ) + "ms";
            else
                Pings += "N/A";

            if( i != SortedPlayers.end( ) - 1 )
                Pings += ", ";
        }

        SendChat( player->GetPID( ), Pings );
    }

	//
	// !SERVER
	// !SERV
	// !REALM
	// !СЕРВЕР
	// !РЕАЛМ
	//

    else if( Command == "server" || Command == "serv" || Command == "realm" || Command == "сервер" || Command == "реалм" )
    {
        string servermsg = string( );
        string serveralias = string( );

        if( !Payload.empty( ) )
        {
            CGamePlayer *LastMatch = nullptr;
            uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

            if( Matches == 0 )
                servermsg = gLanguage->UnableToCheckPlayerNoMatchesFound( Payload );
            else if( Matches == 1 )
            {
                servermsg = LastMatch->GetName( );
                servermsg += ": ";
                serveralias = LastMatch->GetJoinedRealm( ).empty( ) ? "N/A" : LastMatch->GetJoinedRealm( );

                if( serveralias != "N/A" )
                {
                    for( vector<CBNET *> ::iterator j = m_UNM->m_BNETs.begin( ); j != m_UNM->m_BNETs.end( ); j++ )
                    {
                        if( ( *j )->GetServer( ) == serveralias )
                        {
                            serveralias = ( *j )->GetServerAlias( );
                            break;
                        }
                    }
                }

                servermsg += serveralias;
            }
            else
                servermsg = gLanguage->UnableToCheckPlayerFoundMoreThanOneMatch( Payload );
        }
        else
        {
            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
            {
                servermsg += ( *i )->GetName( );
                servermsg += ": ";
                serveralias = ( *i )->GetJoinedRealm( ).empty( ) ? "N/A" : ( *i )->GetJoinedRealm( );

                if( serveralias != "N/A" )
                {
                    for( vector<CBNET *> ::iterator j = m_UNM->m_BNETs.begin( ); j != m_UNM->m_BNETs.end( ); j++ )
                    {
                        if( ( *j )->GetServer( ) == serveralias )
                        {
                            serveralias = ( *j )->GetServerAlias( );
                            break;
                        }
                    }
                }

                servermsg += serveralias;

                if( i != m_Players.end( ) - 1 )
                    servermsg += ", ";
            }

        }

        SendChat( player, servermsg );
    }

	//
	// !CHECKME
	// !CHECK
	// !SC
	// !CM
	// !SPOOFCHECK
	// !SPOOFC
	//

	else if( Command == "checkme" || Command == "check" || Command == "sc" || Command == "cm" || Command == "spoofcheck" || Command == "spoofc" )
        SendChat( player, gLanguage->CheckedPlayer( User, player->GetNumPings( ) > 0 ? UTIL_ToString( player->GetPing( m_UNM->m_LCPings ) ) + "ms" : "N/A", "N/A", DefaultOwnerCheck ? "Yes" : "No", OwnerCheck ? "Yes" : "No", player->GetSpoofed( ) ? "Yes" : "No", player->GetJoinedRealm( ).empty( ) ? "N/A" : player->GetJoinedRealm( ), player->GetReserved( ) ? "Yes" : "No" ) );

	//
	// !VERSION
    // !ABOUT
	// !V
	//

    else if( Command == "version" || Command == "about" || Command == "v" )
        SendChat( player, "UNM bot v" + m_UNM->m_Version + " (created by motherfuunnick)" );

	//
	// !EAT
	// !COOKIEEAT
	// !EATCOOKIE
	//

	else if( Command == "eat" || Command == "cookieeat" || Command == "eatcookie" )
	{
		if( !m_FunCommandsGame )
			SendChat( player, "Фан-команды отключены!" );
		else if( !m_UNM->m_EatGame )
            SendChat( player, gLanguage->CommandDisabled( string( 1, m_UNM->m_CommandTrigger ) + Command ) );
        else
        {
            uint32_t numCookies = player->GetCookies( );

            if( numCookies > m_UNM->m_MaximumNumberCookies )
            {
                numCookies = m_UNM->m_MaximumNumberCookies;
                player->SetCookies( numCookies );
            }

            if( numCookies > 0 )
            {
                if( GetTime( ) - player->GetLastEat( ) < m_UNM->m_EatGameDelay )
                {
                    SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
                    return HideCommand;
                }

                player->SetLastEat( GetTime( ) );
                numCookies--;
                player->SetCookies( numCookies );
                SendAllChat( "[" + User + "] съел печенье. Это было вкусно! У него осталось " + UTIL_ToString( numCookies ) + " печенья." );
            }
            else
                SendChat( player, "У тебя нет печенья! Печенье может дать хост (" + string( 1, m_UNM->m_CommandTrigger ) + "cookie <name>)." );
        }
	}

	//
	// !CHECKCOOKIE
	// !CHECKCOOKIES
	// !COOKIECHECK
	// !COOKIESCHECK
	//

	else if( Command == "checkcookie" || Command == "checkcookies" || Command == "cookiecheck" || Command == "cookiescheck" )
	{
		if( !m_FunCommandsGame )
			SendChat( player, "Фан-команды отключены!" );
        else if( !m_UNM->m_EatGame )
            SendChat( player, gLanguage->CommandDisabled( string( 1, m_UNM->m_CommandTrigger ) + Command ) );
        else
        {
            uint32_t numCookies = player->GetCookies( );

            if( numCookies > m_UNM->m_MaximumNumberCookies )
            {
                numCookies = m_UNM->m_MaximumNumberCookies;
                player->SetCookies( numCookies );
            }

            if( numCookies > 0 )
                SendChat( player, "У тебя осталось " + UTIL_ToString( numCookies ) + " печенья (" + string( 1, m_UNM->m_CommandTrigger ) + "eat - съесть)." );
            else
                SendChat( player, "У тебя нет печенья!" );
        }
	}

	//
	// !COOKIE
	// !COOKIES
	//

	else if( Command == "cookie" || Command == "cookies" || Command == "givecookie" || Command == "givecookies" || Command == "cookiegive" || Command == "cookiesgive" )
	{
        if( !m_FunCommandsGame )
			SendChat( player, "Фан-команды отключены!" );
        else if( !m_UNM->m_EatGame )
            SendChat( player, gLanguage->CommandDisabled( string( 1, m_UNM->m_CommandTrigger ) + Command ) );
		else if( PayloadLower == "eat" )
        {
            uint32_t numCookies = player->GetCookies( );

			if( numCookies > m_UNM->m_MaximumNumberCookies )
			{
                numCookies = m_UNM->m_MaximumNumberCookies;
				player->SetCookies( numCookies );
			}

			if( numCookies > 0 )
			{
				if( GetTime( ) - player->GetLastEat( ) < m_UNM->m_EatGameDelay )
				{
					SendChat( player, "Заклинание " + string( 1, m_UNM->m_CommandTrigger ) + Command + " еще не готово :D" );
					return HideCommand;
				}

				player->SetLastEat( GetTime( ) );
				numCookies--;
				player->SetCookies( numCookies );
				SendAllChat( "[" + User + "] съел печенье. Это было вкусно! У него осталось " + UTIL_ToString( numCookies ) + " печенья." );
			}
            else
                SendChat( player, "У тебя нет печенья! Печенье может дать хост (" + string( 1, m_UNM->m_CommandTrigger ) + "cookie <name>)." );
		}
		else
        {
            uint32_t numCookies = player->GetCookies( );

            if( numCookies > m_UNM->m_MaximumNumberCookies )
            {
                numCookies = m_UNM->m_MaximumNumberCookies;
                player->SetCookies( numCookies );
            }

            if( numCookies > 0 )
                SendChat( player, "У тебя осталось " + UTIL_ToString( numCookies ) + " печенья (" + string( 1, m_UNM->m_CommandTrigger ) + "eat - съесть)." );
            else
                SendChat( player, "У тебя нет печенья, но с тобой может поделиться хост (" + string( 1, m_UNM->m_CommandTrigger ) + "cookie <name>)." );
		}
	}

	//
	// !VOTESTART
	// !VS
	// !GO
	// !ГО
	// !READY
	// !RDY
	//

	else if( Command == "votestart" || Command == "vs" || Command == "go" || Command == "го" || Command == "ready" || Command == "rdy" )
	{
        if( m_GameLoaded || m_GameLoading )
            return HideCommand;

        if( !m_UNM->m_VoteStartAllowed )
            SendChat( player, gLanguage->CommandDisabled( string( 1, m_UNM->m_CommandTrigger ) + Command ) );
        else if( m_CountDownStarted )
            SendChat( player, "Команда " + string( 1, m_UNM->m_CommandTrigger ) + Command + " недоступна во время отсчёта старта игры!" );
        else
        {
            if( m_StartedVoteStartTime == 0 )
            {
                // need > minplayers to START a votestart

                if( GetNumHumanPlayers( ) < m_UNM->m_VoteStartMinPlayers )
                {
                    // need at least eight players to votestart

                    if( m_UNM->m_VoteStartMinPlayers < 5 )
                        SendChat( player, "Нельзя начать голосование за старт - нужно как минимум " + UTIL_ToString( m_UNM->m_VoteStartMinPlayers ) + " игрока" );
                    else
                        SendChat( player, "Нельзя начать голосование за старт - нужно как минимум " + UTIL_ToString( m_UNM->m_VoteStartMinPlayers ) + " игроков" );

                    return HideCommand;
                }

                for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
                    ( *i )->SetStartVote( false );

                m_StartedVoteStartTime = GetTime( );
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] голосование за досрочный старт начато игроком [" + User + "]", 6, m_CurrentGameCounter );
            }

            player->SetStartVote( true );
            uint32_t VotesNeeded;
            uint32_t Votes = 0;

            for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
            {
                if( ( *i )->GetStartVote( ) )
                    ++Votes;
            }

            if( m_UNM->m_VoteStartPercentalVoting )
                VotesNeeded = (uint32_t)ceil( float( float( GetNumHumanPlayers( ) * m_UNM->m_VoteStartPercent ) / 100 ) );
            else
                VotesNeeded = m_UNM->m_VoteStartMinPlayers;

            if( Votes < VotesNeeded )
                SendAllChat( User + " проголосовал за досрочный старт. Еще нужно: " + UTIL_ToString( VotesNeeded - Votes ) + " голосов." );
            else
            {
                StartCountDown( true );
                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] голосование набрало необходимое число голосов, запуск игры", 6, m_CurrentGameCounter );
            }
        }
	}

	//
	// !WIM
	// !WVS
	//

	else if( ( Command == "wim" || Command == "wvs" ) && !m_CountDownStarted )
	{
		string wiim;

		for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
		{
			if( !( *i )->GetStartVote( ) )
			{
				if( wiim.empty( ) )
					wiim = ( *i )->GetName( );
				else
					wiim += ", " + ( *i )->GetName( );
			}
		}

		if( !IsMuted( User ) && !player->GetMuted( ) && IsCensorMuted( User ) == 0 )
		{
			if( m_StartedVoteStartTime == 0 )
				SendChat( player, "Голосование за досрочный старт неактивно" );
			else if( !wiim.empty( ) )
                SendAllChat( gLanguage->PlayerNeedsToReady( wiim, string( 1, m_UNM->m_CommandTrigger ) ) );
			else
                SendAllChat( gLanguage->EverybodyIsReady( UTIL_ToString( GetNumHumanPlayers( ) ), string( 1, m_UNM->m_CommandTrigger ) ) );
		}
		else
		{
			if( m_StartedVoteStartTime == 0 )
				SendChat( player, "Голосование за досрочный старт неактивно" );
			else if( !wiim.empty( ) )
                SendChat( player, gLanguage->PlayerNeedsToReady( wiim, string( 1, m_UNM->m_CommandTrigger ) ) );
			else
                SendChat( player, gLanguage->EverybodyIsReady( UTIL_ToString( GetNumHumanPlayers( ) ), string( 1, m_UNM->m_CommandTrigger ) ) );
		}
	}

	//
    // !VOTEKICK
	// !VK
	//

	else if( ( Command == "votekick" || Command == "vk" ) && m_UNM->m_VoteKickAllowed && !Payload.empty( ) )
	{
		if( !m_KickVotePlayer.empty( ) )
            SendChat( player, gLanguage->UnableToVoteKickAlreadyInProgress( ) );
		else if( m_Players.size( ) == 2 )
            SendChat( player, gLanguage->UnableToVoteKickNotEnoughPlayers( ) );
		else
		{
			CGamePlayer *LastMatch = nullptr;
			uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

			if( Matches == 0 )
                SendChat( player, gLanguage->UnableToVoteKickNoMatchesFound( Payload ) );
			else if( Matches == 1 )
			{
				if( LastMatch->GetReserved( ) && !m_GameLoaded )
                    SendChat( player, gLanguage->UnableToVoteKickPlayerIsReserved( LastMatch->GetName( ) ) );
				else
				{
					m_KickVotePlayer = LastMatch->GetName( );
					m_StartedKickVoteTime = GetTime( );

					for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
						( *i )->SetKickVote( false );

					player->SetKickVote( true );
                    CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] votekick against player [" + m_KickVotePlayer + "] started by player [" + User + "]", 6, m_CurrentGameCounter );
                    SendAllChat( gLanguage->StartedVoteKick( LastMatch->GetName( ), User, UTIL_ToString( (uint32_t)ceil( float( float( GetNumHumanPlayers( ) * m_UNM->m_VoteKickPercentage ) / 100 ) ) ) ) );
                    SendAllChat( gLanguage->TypeYesToVote( string( 1, m_UNM->m_CommandTrigger ) ) );
				}
			}
			else
                SendChat( player, gLanguage->UnableToVoteKickFoundMoreThanOneMatch( Payload ) );
		}
	}

	//
	// !YES
	// !Y
	//

	else if( ( Command == "yes" || Command == "y" ) && !m_KickVotePlayer.empty( ) && player->GetName( ) != m_KickVotePlayer && !player->GetKickVote( ) )
	{
		player->SetKickVote( true );
        uint32_t VotesNeeded = (uint32_t)ceil( float( float( GetNumHumanPlayers( ) * m_UNM->m_VoteKickPercentage ) / 100 ) );
		uint32_t Votes = 0;

		for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
		{
			if( ( *i )->GetKickVote( ) )
				Votes++;
		}

		if( Votes >= VotesNeeded )
		{
			CGamePlayer *Victim = GetPlayerFromName( m_KickVotePlayer, false );

			if( Victim )
			{
				Victim->SetDeleteMe( true );
                Victim->SetLeftReason( gLanguage->WasKickedByVote( ) );

				if( !m_GameLoading && !m_GameLoaded )
					Victim->SetLeftCode( PLAYERLEAVE_LOBBY );
				else
					Victim->SetLeftCode( PLAYERLEAVE_LOST );

				if( !m_GameLoading && !m_GameLoaded )
					OpenSlot( GetSIDFromPID( Victim->GetPID( ) ), false );

                CONSOLE_Print( "[GAME: " + m_UNM->IncGameNrDecryption( m_GameName ) + "] votekick against player [" + m_KickVotePlayer + "] passed with " + UTIL_ToString( Votes ) + "/" + UTIL_ToString( GetNumHumanPlayers( ) ) + " votes", 6, m_CurrentGameCounter );
                SendAllChat( gLanguage->VoteKickPassed( m_KickVotePlayer ) );
			}
			else
                SendAllChat( gLanguage->ErrorVoteKickingPlayer( m_KickVotePlayer ) );

			m_KickVotePlayer.clear( );
			m_StartedKickVoteTime = 0;
		}
		else
            SendAllChat( gLanguage->VoteKickAcceptedNeedMoreVotes( m_KickVotePlayer, User, UTIL_ToString( VotesNeeded - Votes ) ) );
	}

	//
	// !APM
	// !ACTIONS
	// !CLICKS
	// !АПМ
	// !КЛИКИ
	//

	else if( ( Command == "apm" || Command == "actions" || Command == "clicks" || Command == "апм" || Command == "клики" ) && m_GameLoaded )
	{
		uint32_t GameTimeInSeconds = m_GameTicks / 1000;

		if( GameTimeInSeconds == 0 )
			GameTimeInSeconds = 1;

		if( !Payload.empty( ) )
		{
			CGamePlayer *LastMatch = nullptr;
			uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

			if( Matches == 0 )
                SendChat( player, gLanguage->UnableToCheckPlayerNoMatchesFound( Payload ) );
			else if( Matches == 1 )
				SendChat( player, "Игрок [" + LastMatch->GetName( ) + "] сделал " + UTIL_ToString( LastMatch->m_ActionsSum ) + " кликов и имеет " + UTIL_ToString( ( LastMatch->m_ActionsSum * 60 ) / GameTimeInSeconds ) + " apm" );
			else
                SendChat( player, gLanguage->UnableToCheckPlayerFoundMoreThanOneMatch( Payload ) );
		}
		else
			SendChat( player, "Вы сделали " + UTIL_ToString( player->m_ActionsSum ) + " кликов и имеете " + UTIL_ToString( ( player->m_ActionsSum * 60 ) / GameTimeInSeconds ) + " apm" );
	}

	//
	// !BOT
	// !BOTNAME
	// !NAMEBOT
	// !BOTNAMETEST
	// !TESTBOTNAME
	// !NAMEBOTTEST
	// !TESTNAMEBOT
	//

	else if( Command == "bot" || Command == "botname" || Command == "namebot" || Command == "botnametest" || Command == "testbotname" || Command == "namebottest" || Command == "testnamebot" )
	{
		if( m_UNM->m_BotNameCustom.find_first_not_of( " " ) != string::npos )
            SendChat( player, gLanguage->BotName( m_UNM->m_BotNameCustom ) );
		else if( Command == "botnametest" || Command == "testbotname" || Command == "namebottest" || Command == "testnamebot" || PayloadLower == "test" )
		{
			string BotName = string( );
			string MainBotName = string( );
			bool SpamBotMode = false;
			uint32_t numbots = 0;

            if( m_UNM->IsBnetServer( UserServer ) )
			{
				for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
				{
					if( MainBotName.empty( ) && !( *i )->m_SpamSubBot && ( *i )->GetServer( ) == UserServer )
						MainBotName = ( *i )->GetName( );

					if( ( *i )->GetServer( ) == UserServer )
						numbots++;

					if( BotName.empty( ) && ( *i )->GetHostCounterID( ) == player->GetJoinedRealmID( ) )
					{
						if( !( *i )->m_SpamSubBot )
							MainBotName = ( *i )->GetName( );

						BotName = ( *i )->GetName( );
					}
				}

				if( numbots > 1 )
					SpamBotMode = true;
			}

			if( BotName.empty( ) )
			{
				if( m_UNM->m_BroadcastHostName == 0 )
					BotName = m_VirtualHostName;
				else if( m_UNM->m_BroadcastHostName == 1 )
					BotName = m_CreatorName;
				else if( m_UNM->m_BroadcastHostName == 2 )
					BotName = m_DefaultOwner;
				else if( m_UNM->m_BroadcastHostName == 3 )
					BotName = m_UNM->m_BroadcastHostNameCustom;

				if( BotName.find_first_not_of( " " ) == string::npos )
					BotName = m_VirtualHostName;

				if( BotName.find_first_not_of( " " ) != string::npos )
                    SendChat( player, gLanguage->BotName( BotName ) );
			}
			else if( !MainBotName.empty( ) )
			{
				if( m_UNM->IsIccupServer( UserServer ) )
				{
					if( BotName == MainBotName )
						SendChat( player, "Вы зашли в игру созданную ботом [" + BotName + "], за этим ботом следует выполнять /follow" );
					else
					{
						SendChat( player, "Вы зашли в игру созданную ботом [" + BotName + "]" );
						SendChat( player, "Бот за которым следует выполнять /follow [" + MainBotName + "]" );
					}
				}
				else if( BotName == MainBotName && SpamBotMode )
					SendChat( player, "Вы зашли в игру созданную ботом [" + BotName + "], который является главным в своей хост-группе" );
				else if( BotName == MainBotName )
					SendChat( player, "Вы зашли в игру созданную ботом [" + BotName + "]" );
				else
				{
					SendChat( player, "Вы зашли в игру созданную ботом [" + BotName + "]" );
					SendChat( player, "В данной хост-группе главным ботом является [" + MainBotName + "]" );
				}
			}
			else
			{
				SendChat( player, "Вы зашли в игру созданную ботом [" + BotName + "]" );

				if( SpamBotMode )
					SendChat( player, "В данной хост-группе отсутствует главный бот" );
			}
		}
		else
		{
			string BotName = string( );

            if( m_UNM->IsBnetServer( UserServer ) )
			{
				for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
				{
					if( !( *i )->m_SpamSubBot && ( *i )->GetServer( ) == UserServer )
					{
						BotName = ( *i )->GetName( );
						break;
					}
				}

				if( BotName.empty( ) )
				{
					for( vector<CBNET *> ::iterator i = m_UNM->m_BNETs.begin( ); i != m_UNM->m_BNETs.end( ); i++ )
					{
						if( ( *i )->GetHostCounterID( ) == player->GetJoinedRealmID( ) )
						{
							BotName = ( *i )->GetName( );
							break;
						}
					}
				}
			}

			if( BotName.empty( ) )
			{
				if( m_UNM->m_BroadcastHostName == 0 )
					BotName = m_VirtualHostName;
				else if( m_UNM->m_BroadcastHostName == 1 )
					BotName = m_CreatorName;
				else if( m_UNM->m_BroadcastHostName == 2 )
					BotName = m_DefaultOwner;
				else if( m_UNM->m_BroadcastHostName == 3 )
					BotName = m_UNM->m_BroadcastHostNameCustom;

				if( BotName.find_first_not_of( " " ) == string::npos )
					BotName = m_VirtualHostName;
			}

			if( BotName.find_first_not_of( " " ) != string::npos )
                SendChat( player, gLanguage->BotName( BotName ) );
		}
    }

    return HideCommand;
}

void CGame::EventGameStarted( )
{
    for( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); i++ )
    {
        if( ( *i )->GetSocket( ) && GetTime( ) - ( *i )->GetSocket( )->GetLastRecv( ) >= m_UNM->m_DropWaitTimeLoad )
        {
            ( *i )->SetDeleteMe( true );
            ( *i )->SetLeftReason( gLanguage->HasLostConnectionTimedOut( ) );
            ( *i )->SetLeftCode( PLAYERLEAVE_DISCONNECT );
            OpenSlot( GetSIDFromPID( ( *i )->GetPID( ) ), false );
            m_CountDownStarted = false;
            SendAllChat( gLanguage->CountDownAborted( ) );
            return;
        }
    }

    CBaseGame::EventGameStarted( );
}
