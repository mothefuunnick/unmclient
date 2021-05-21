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
#include "config.h"
#include "language.h"

using namespace std;

//
// CLanguage
//

CLanguage::CLanguage( string nCFGFile )
{
	m_CFG = new CConfig( );
	m_CFG->Read( nCFGFile );
}

CLanguage::~CLanguage( )
{
	delete m_CFG;
}

void CLanguage::Replace( string &Text, string Key, string Value )
{
	// don't allow any infinite loops

	if( Value.find( Key ) != string::npos )
		return;

	string::size_type KeyStart = Text.find( Key );

	while( KeyStart != string::npos )
	{
		Text.replace( KeyStart, Key.size( ), Value );
		KeyStart = Text.find( Key );
	}
}

string CLanguage::UnableToCreateGameTryAnotherName( string server, string gamename )
{
	string Out = m_CFG->GetString( "lang_0001", "lang_0001" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::UserIsAlreadyAnAdmin( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0002", "lang_0002" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::AddedUserToAdminDatabase( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0003", "lang_0003" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::ErrorAddingUserToAdminDatabase( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0004", "lang_0004" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::YouDontHaveAccessToThatCommand( )
{
	return m_CFG->GetString( "lang_0005", "lang_0005" );
}

string CLanguage::UserIsAlreadyBanned( string server, string victim )
{
	string Out = m_CFG->GetString( "lang_0006", "lang_0006" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::BannedUser( string server, string victim )
{
	string Out = m_CFG->GetString( "lang_0007", "lang_0007" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::ErrorBanningUser( string server, string victim )
{
	string Out = m_CFG->GetString( "lang_0008", "lang_0008" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::UserIsAnAdmin( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0009", "lang_0009" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::UserIsNotAnAdmin( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0010", "lang_0010" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::UserWasBannedOnByBecause( string server, string victim, string date, string date2, string admin, string reason, string expiredate )
{
	if( expiredate.empty( ) )
	{
		string Out = m_CFG->GetString( "lang_0011", "lang_0011" );
		Replace( Out, "$SERVER$", server );
		Replace( Out, "$VICTIM$", victim );
		Replace( Out, "$DATE$", date );
		Replace( Out, "$ADMIN$", admin );
		Replace( Out, "$REASON$", reason );
		return Out;
	}
	else
	{
        string Out = m_CFG->GetString( "lang_0222", "lang_0222" );
		Replace( Out, "$SERVER$", server );
		Replace( Out, "$VICTIM$", victim );
		Replace( Out, "$DATE$", date );
		Replace( Out, "$ADMIN$", admin );
		Replace( Out, "$REASON$", reason );
		Replace( Out, "$EXPIREDATE$", expiredate );
		Replace( Out, "$DAYSREMAINING$", date2 );
		return Out;
	}
}

string CLanguage::UserIsNotBanned( string server, string victim )
{
	string Out = m_CFG->GetString( "lang_0012", "lang_0012" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::ThereAreNoAdmins( string server )
{
	string Out = m_CFG->GetString( "lang_0013", "lang_0013" );
	Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage::ThereIsAdmin( string server )
{
	string Out = m_CFG->GetString( "lang_0014", "lang_0014" );
	Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage::ThereAreAdmins( string server, string count )
{
	string Out = m_CFG->GetString( "lang_0015", "lang_0015" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$COUNT$", count );
	return Out;
}

string CLanguage::ThereAreNoBannedUsers( string server )
{
	string Out = m_CFG->GetString( "lang_0016", "lang_0016" );
	Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage::ThereIsBannedUser( string server )
{
	string Out = m_CFG->GetString( "lang_0017", "lang_0017" );
	Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage::ThereAreBannedUsers( string server, string count )
{
	string Out = m_CFG->GetString( "lang_0018", "lang_0018" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$COUNT$", count );
	return Out;
}

string CLanguage::YouCantDeleteTheRootAdmin( )
{
	return m_CFG->GetString( "lang_0019", "lang_0019" );
}

string CLanguage::DeletedUserFromAdminDatabase( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0020", "lang_0020" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::ErrorDeletingUserFromAdminDatabase( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0021", "lang_0021" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::UnbannedUser( string victim )
{
	string Out = m_CFG->GetString( "lang_0022", "lang_0022" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::ErrorUnbanningUser( string victim )
{
	string Out = m_CFG->GetString( "lang_0023", "lang_0023" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::GameNumberIs( string number, string description )
{
	string Out = m_CFG->GetString( "lang_0024", "lang_0024" );
	Replace( Out, "$NUMBER$", number );
	Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage::GameNumberDoesntExist( string number )
{
	string Out = m_CFG->GetString( "lang_0025", "lang_0025" );
	Replace( Out, "$NUMBER$", number );
	return Out;
}

string CLanguage::GameIsInTheLobby( string description, string current, string max )
{
	string Out = m_CFG->GetString( "lang_0026", "lang_0026" );
	Replace( Out, "$DESCRIPTION$", description );
	Replace( Out, "$CURRENT$", current );
	Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage::ThereIsNoGameInTheLobby( string current, string max )
{
	string Out = m_CFG->GetString( "lang_0027", "lang_0027" );
	Replace( Out, "$CURRENT$", current );
	Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage::UnableToLoadConfigFilesOutside( )
{
	return m_CFG->GetString( "lang_0028", "lang_0028" );
}

string CLanguage::LoadingConfigFile( string file )
{
	string Out = m_CFG->GetString( "lang_0029", "lang_0029" );
	Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage::UnableToLoadConfigFileDoesntExist( string file )
{
	string Out = m_CFG->GetString( "lang_0030", "lang_0030" );
	Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage::CreatingPrivateGame( string gamename, string user )
{
	string Out = m_CFG->GetString( "lang_0031", "lang_0031" );
	Replace( Out, "$GAMENAME$", gamename );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::CreatingPublicGame( string gamename, string user )
{
	string Out = m_CFG->GetString( "lang_0032", "lang_0032" );
	Replace( Out, "$GAMENAME$", gamename );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::UnableToUnhostGameCountdownStarted( string description )
{
	string Out = m_CFG->GetString( "lang_0033", "lang_0033" );
	Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage::UnhostingGame( string description )
{
	string Out = m_CFG->GetString( "lang_0034", "lang_0034" );
	Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage::UnableToUnhostGameNoGameInLobby( )
{
	return m_CFG->GetString( "lang_0035", "lang_0035" );
}

string CLanguage::AllFrom( string players, string country )
{
	string Out = m_CFG->GetString( "lang_0036", "lang_0036" );
    Replace( Out, "$PLAYERS$", players );
    Replace( Out, "$COUNTRY$", country );
	return Out;
}

string CLanguage::AllFrom2( string players, string country )
{
	string Out = m_CFG->GetString( "lang_0037", "lang_0037" );
    Replace( Out, "$PLAYERS$", players );
    Replace( Out, "$COUNTRY$", country );
	return Out;
}

string CLanguage::UnableToCreateGameAnotherGameInLobby( string gamename, string description )
{
	string Out = m_CFG->GetString( "lang_0038", "lang_0038" );
	Replace( Out, "$GAMENAME$", gamename );
	Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage::UnableToCreateGameMaxGamesReached( string gamename, string max )
{
	string Out = m_CFG->GetString( "lang_0039", "lang_0039" );
	Replace( Out, "$GAMENAME$", gamename );
	Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage::GameIsOver( string description )
{
	string Out = m_CFG->GetString( "lang_0040", "lang_0040" );
	Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage::SpoofCheckByReplying( )
{
	return m_CFG->GetString( "lang_0041", "lang_0041" );
}

string CLanguage::GameRefreshed( )
{
	return m_CFG->GetString( "lang_0042", "lang_0042" );
}

string CLanguage::SpoofPossibleIsAway( string user )
{
	string Out = m_CFG->GetString( "lang_0043", "lang_0043" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::SpoofPossibleIsUnavailable( string user )
{
	string Out = m_CFG->GetString( "lang_0044", "lang_0044" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::SpoofPossibleIsRefusingMessages( string user )
{
	string Out = m_CFG->GetString( "lang_0045", "lang_0045" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::SpoofDetectedIsNotInGame( string user )
{
	string Out = m_CFG->GetString( "lang_0046", "lang_0046" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::SpoofDetectedIsInPrivateChannel( string user )
{
	string Out = m_CFG->GetString( "lang_0047", "lang_0047" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::SpoofDetectedIsInAnotherGame( string user )
{
	string Out = m_CFG->GetString( "lang_0048", "lang_0048" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::CountDownAborted( )
{
	return m_CFG->GetString( "lang_0049", "lang_0049" );
}

string CLanguage::TryingToJoinTheGameButBanned( string victim, string admin )
{
	string Out = m_CFG->GetString( "lang_0050", "lang_0050" );
	Replace( Out, "$VICTIM$", victim );
	Replace( Out, "$ADMIN$", admin );
	return Out;
}

string CLanguage::UnableToBanNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0051", "lang_0051" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::PlayerWasBannedByPlayer( string server, string victim, string user, string expiredate )
{
	if( !expiredate.empty( ) && expiredate != "0" )
	{
        string Out = m_CFG->GetString( "lang_0241", "lang_0241" );
		Replace( Out, "$SERVER$", server );
		Replace( Out, "$VICTIM$", victim );
		Replace( Out, "$USER$", user );
		Replace( Out, "$BANDAYTIME$", expiredate );
		return Out;
	}
	else
	{
		string Out = m_CFG->GetString( "lang_0052", "lang_0052" );
		Replace( Out, "$SERVER$", server );
		Replace( Out, "$VICTIM$", victim );
		Replace( Out, "$USER$", user );
		return Out;
	}
}

string CLanguage::UnableToBanFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0053", "lang_0053" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::AddedPlayerToTheHoldList( string user )
{
	string Out = m_CFG->GetString( "lang_0054", "lang_0054" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::UnableToKickNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0055", "lang_0055" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::UnableToKickFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0056", "lang_0056" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::SettingLatencyToMinimum( string min )
{
	string Out = m_CFG->GetString( "lang_0057", "lang_0057" );
	Replace( Out, "$MIN$", min );
	return Out;
}

string CLanguage::SettingLatencyToMaximum( string max )
{
	string Out = m_CFG->GetString( "lang_0058", "lang_0058" );
	Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage::SettingLatencyTo( string latency )
{
	string Out = m_CFG->GetString( "lang_0059", "lang_0059" );
	Replace( Out, "$LATENCY$", latency );
	return Out;
}

string CLanguage::KickingPlayersWithPingsGreaterThan( string total, string ping )
{
	string Out = m_CFG->GetString( "lang_0060", "lang_0060" );
	Replace( Out, "$TOTAL$", total );
	Replace( Out, "$PING$", ping );
	return Out;
}

string CLanguage::HasPlayedGamesWithThisBot( string user, string firstgame, string lastgame, string totalgames, string hoursplayed, string minutesplayed, string hoursloaded, string minutesloaded, string avgloadingtime, string avgstay, string level )
{
	string Out = m_CFG->GetString( "lang_0061", "lang_0061" );
	Replace( Out, "$USER$", user );
	Replace( Out, "$FIRSTGAME$", firstgame );
	Replace( Out, "$LASTGAME$", lastgame );
	Replace( Out, "$TOTALGAMES$", totalgames );
	Replace( Out, "$HOURSPLAYED$", hoursplayed );
	Replace( Out, "$MINUTESPLAYED$", minutesplayed );
    Replace( Out, "$HOURSLOADED$", hoursloaded );
    Replace( Out, "$MINUTESLOADED$", minutesloaded);
    Replace( Out, "$AVGLOADINGTIME$", avgloadingtime );
    Replace( Out, "$AVGSTAY$", avgstay );
	Replace( Out, "$LEVEL$", level );
	return Out;
}

string CLanguage::HasntPlayedGamesWithThisBot( string user )
{
	string Out = m_CFG->GetString( "lang_0062", "lang_0062" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::AutokickingPlayerForExcessivePing( string victim, string ping )
{
	string Out = m_CFG->GetString( "lang_0063", "lang_0063" );
	Replace( Out, "$VICTIM$", victim );
	Replace( Out, "$PING$", ping );
	return Out;
}

string CLanguage::SpoofCheckAcceptedFor( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0064", "lang_0064" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::PlayersNotYetSpoofChecked( string notspoofchecked )
{
	string Out = m_CFG->GetString( "lang_0065", "lang_0065" );
	Replace( Out, "$NOTSPOOFCHECKED$", notspoofchecked );
	return Out;
}

string CLanguage::OnlyOwnersCanStart( )
{
	return m_CFG->GetString( "lang_0066", "lang_0066" );
}

string CLanguage::SpoofCheckByWhispering( string hostname )
{
	string Out = m_CFG->GetString( "lang_0067", "lang_0067" );
	Replace( Out, "$HOSTNAME$", hostname );
	return Out;
}

string CLanguage::EveryoneHasBeenSpoofChecked( )
{
	return m_CFG->GetString( "lang_0068", "lang_0068" );
}

string CLanguage::PlayersNotYetPinged( string number, string notpinged )
{
	string Out = m_CFG->GetString( "lang_0069", "lang_0069" );
	Replace( Out, "$NUMBER$", number );
	Replace( Out, "$NOTPINGED$", notpinged );
	return Out;
}

string CLanguage::EveryoneHasBeenPinged( )
{
	return m_CFG->GetString( "lang_0070", "lang_0070" );
}

string CLanguage::ShortestLoadByPlayer( string user, string loadingtime )
{
	string Out = m_CFG->GetString( "lang_0071", "lang_0071" );
	Replace( Out, "$USER$", user );
	Replace( Out, "$LOADINGTIME$", loadingtime );
	return Out;
}

string CLanguage::LongestLoadByPlayer( string user, string loadingtime )
{
	string Out = m_CFG->GetString( "lang_0072", "lang_0072" );
	Replace( Out, "$USER$", user );
	Replace( Out, "$LOADINGTIME$", loadingtime );
	return Out;
}

string CLanguage::YourLoadingTimeWas( string loadingtime )
{
	string Out = m_CFG->GetString( "lang_0073", "lang_0073" );
	Replace( Out, "$LOADINGTIME$", loadingtime );
	return Out;
}

string CLanguage::HasPlayedDotAGamesWithThisBot( string user, string totalgames, string totalwins, string totallosses, string totalkills, string totaldeaths, string totalcreepkills, string totalcreepdenies, string totalassists, string totalneutralkills, string totaltowerkills, string totalraxkills, string totalcourierkills, string avgkills, string avgdeaths, string avgcreepkills, string avgcreepdenies, string avgassists, string avgneutralkills, string avgtowerkills, string avgraxkills, string avgcourierkills )
{
	string Out = m_CFG->GetString( "lang_0074", "lang_0074" );
	Replace( Out, "$USER$", user );
	Replace( Out, "$TOTALGAMES$", totalgames );
	Replace( Out, "$TOTALWINS$", totalwins );
	Replace( Out, "$TOTALLOSSES$", totallosses );
	Replace( Out, "$TOTALKILLS$", totalkills );
	Replace( Out, "$TOTALDEATHS$", totaldeaths );
	Replace( Out, "$TOTALCREEPKILLS$", totalcreepkills );
	Replace( Out, "$TOTALCREEPDENIES$", totalcreepdenies );
	Replace( Out, "$TOTALASSISTS$", totalassists );
	Replace( Out, "$TOTALNEUTRALKILLS$", totalneutralkills );
	Replace( Out, "$TOTALTOWERKILLS$", totaltowerkills );
	Replace( Out, "$TOTALRAXKILLS$", totalraxkills );
	Replace( Out, "$TOTALCOURIERKILLS$", totalcourierkills );
	Replace( Out, "$AVGKILLS$", avgkills );
	Replace( Out, "$AVGDEATHS$", avgdeaths );
	Replace( Out, "$AVGCREEPKILLS$", avgcreepkills );
	Replace( Out, "$AVGCREEPDENIES$", avgcreepdenies );
	Replace( Out, "$AVGASSISTS$", avgassists );
	Replace( Out, "$AVGNEUTRALKILLS$", avgneutralkills );
	Replace( Out, "$AVGTOWERKILLS$", avgtowerkills );
	Replace( Out, "$AVGRAXKILLS$", avgraxkills );
	Replace( Out, "$AVGCOURIERKILLS$", avgcourierkills );
	return Out;
}

string CLanguage::HasntPlayedDotAGamesWithThisBot( string user )
{
	string Out = m_CFG->GetString( "lang_0075", "lang_0075" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::WasKickedForReservedPlayer( string reserved )
{
	string Out = m_CFG->GetString( "lang_0076", "lang_0076" );
	Replace( Out, "$RESERVED$", reserved );
	return Out;
}

string CLanguage::WasKickedForOwnerPlayer( string owner )
{
	string Out = m_CFG->GetString( "lang_0077", "lang_0077" );
	Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage::WasKickedByPlayer( string user )
{
	string Out = m_CFG->GetString( "lang_0078", "lang_0078" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::HasLostConnectionPlayerError( string error )
{
	string Out = m_CFG->GetString( "lang_0079", "lang_0079" );
	Replace( Out, "$ERROR$", error );
	return Out;
}

string CLanguage::HasLostConnectionSocketError( string error )
{
	string Out = m_CFG->GetString( "lang_0080", "lang_0080" );
	Replace( Out, "$ERROR$", error );
	return Out;
}

string CLanguage::HasLostConnectionClosedByRemoteHost( )
{
	return m_CFG->GetString( "lang_0081", "lang_0081" );
}

string CLanguage::HasLeftVoluntarily( )
{
	return m_CFG->GetString( "lang_0082", "lang_0082" );
}

string CLanguage::EndingGame( string description )
{
	string Out = m_CFG->GetString( "lang_0083", "lang_0083" );
	Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage::HasLostConnectionTimedOut( )
{
	return m_CFG->GetString( "lang_0084", "lang_0084" );
}

string CLanguage::GlobalChatMuted( )
{
	return m_CFG->GetString( "lang_0085", "lang_0085" );
}

string CLanguage::GlobalChatUnmuted( )
{
	return m_CFG->GetString( "lang_0086", "lang_0086" );
}

string CLanguage::ShufflingPlayers( )
{
	return m_CFG->GetString( "lang_0087", "lang_0087" );
}

string CLanguage::UnableToLoadConfigFileGameInLobby( string trigger )
{
	string Out = m_CFG->GetString( "lang_0088", "lang_0088" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::PlayersStillDownloading( string stilldownloading )
{
	string Out = m_CFG->GetString( "lang_0089", "lang_0089" );
	Replace( Out, "$STILLDOWNLOADING$", stilldownloading );
	return Out;
}

string CLanguage::RefreshMessagesEnabled( )
{
	return m_CFG->GetString( "lang_0090", "lang_0090" );
}

string CLanguage::RefreshMessagesDisabled( )
{
	return m_CFG->GetString( "lang_0091", "lang_0091" );
}

string CLanguage::AtLeastOneGameActiveUseForceToShutdown( string trigger )
{
	string Out = m_CFG->GetString( "lang_0092", "lang_0092" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::CurrentlyLoadedMapCFGIs( string mapcfg )
{
	string Out = m_CFG->GetString( "lang_0093", "lang_0093" );
	Replace( Out, "$MAPCFG$", mapcfg );
	return Out;
}

string CLanguage::LaggedOutDroppedByAdmin( )
{
	return m_CFG->GetString( "lang_0094", "lang_0094" );
}

string CLanguage::LaggedOutDroppedByVote( )
{
	return m_CFG->GetString( "lang_0095", "lang_0095" );
}

string CLanguage::PlayerVotedToDropLaggers( string user )
{
	string Out = m_CFG->GetString( "lang_0096", "lang_0096" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::LatencyIs( string latency )
{
	string Out = m_CFG->GetString( "lang_0097", "lang_0097" );
	Replace( Out, "$LATENCY$", latency );
	return Out;
}

string CLanguage::SyncLimitIs( string synclimit )
{
	string Out = m_CFG->GetString( "lang_0098", "lang_0098" );
	Replace( Out, "$SYNCLIMIT$", synclimit );
	return Out;
}

string CLanguage::SettingSyncLimitToMinimum( string min )
{
	string Out = m_CFG->GetString( "lang_0099", "lang_0099" );
	Replace( Out, "$MIN$", min );
	return Out;
}

string CLanguage::SettingSyncLimitToMaximum( string max )
{
	string Out = m_CFG->GetString( "lang_0100", "lang_0100" );
	Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage::SettingSyncLimitTo( string synclimit )
{
	string Out = m_CFG->GetString( "lang_0101", "lang_0101" );
	Replace( Out, "$SYNCLIMIT$", synclimit );
	return Out;
}

string CLanguage::UnableToCreateGameNotLoggedIn( string gamename )
{
	string Out = m_CFG->GetString( "lang_0102", "lang_0102" );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::AdminLoggedIn( )
{
	return m_CFG->GetString( "lang_0103", "lang_0103" );
}

string CLanguage::AdminInvalidPassword( string attempt )
{
	string Out = m_CFG->GetString( "lang_0104", "lang_0104" );
	Replace( Out, "$ATTEMPT$", attempt );
	return Out;
}

string CLanguage::ConnectingToBNET( string server )
{
	string Out = m_CFG->GetString( "lang_0105", "lang_0105" );
	Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage::ConnectedToBNET( string server )
{
    string Out = m_CFG->GetString( "lang_0106", "lang_0106" );
	Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage::DisconnectedFromBNET( string server )
{
	string Out = m_CFG->GetString( "lang_0107", "lang_0107" );
	Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage::LoggedInToBNET( string server )
{
	string Out = m_CFG->GetString( "lang_0108", "lang_0108" );
	Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage::BNETGameHostingSucceeded( string server )
{
	string Out = m_CFG->GetString( "lang_0109", "lang_0109" );
	Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage::BNETGameHostingFailed( string server, string gamename )
{
	string Out = m_CFG->GetString( "lang_0110", "lang_0110" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::ConnectingToBNETTimedOut( string server )
{
	string Out = m_CFG->GetString( "lang_0111", "lang_0111" );
	Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage::PlayerDownloadedTheMap( string user, string seconds, string rate )
{
	string Out = m_CFG->GetString( "lang_0112", "lang_0112" );
	Replace( Out, "$USER$", user );
	Replace( Out, "$SECONDS$", seconds );
	Replace( Out, "$RATE$", rate );
	return Out;
}

string CLanguage::UnableToCreateGameNameTooLong( string gamename )
{
	string Out = m_CFG->GetString( "lang_0113", "lang_0113" );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::SettingGameOwnerTo( string owner )
{
	string Out = m_CFG->GetString( "lang_0114", "lang_0114" );
	Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage::TheGameIsLocked( )
{
	return m_CFG->GetString( "lang_0115", "lang_0115" );
}

string CLanguage::GameLocked( )
{
	return m_CFG->GetString( "lang_0116", "lang_0116" );
}

string CLanguage::GameUnlocked( )
{
	return m_CFG->GetString( "lang_0117", "lang_0117" );
}

string CLanguage::UnableToStartDownloadNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0118", "lang_0118" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::UnableToStartDownloadFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0119", "lang_0119" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::UnableToSetGameOwner( string owner )
{
	string Out = m_CFG->GetString( "lang_0120", "lang_0120" );
	Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage::UnableToCheckPlayerNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0121", "lang_0121" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::CheckedPlayer( string victim, string ping, string from, string admin, string owner, string spoofed, string realm, string reserved )
{
	string Out = m_CFG->GetString( "lang_0122", "lang_0122" );
	Replace( Out, "$VICTIM$", victim );
	Replace( Out, "$PING$", ping );
	Replace( Out, "$FROM$", from );
	Replace( Out, "$ADMIN$", admin );
	Replace( Out, "$OWNER$", owner );
	Replace( Out, "$SPOOFED$", spoofed );
    Replace( Out, "$REALM$", realm );
	Replace( Out, "$RESERVED$", reserved );
	return Out;
}

string CLanguage::UnableToCheckPlayerFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0123", "lang_0123" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::TheGameIsLockedBNET( )
{
	return m_CFG->GetString( "lang_0124", "lang_0124" );
}

string CLanguage::UnableToCreateGameDisabled( string gamename, string reason )
{
	string Out = m_CFG->GetString( "lang_0125", "lang_0125" );
	Replace( Out, "$GAMENAME$", gamename );
	Replace( Out, "$REASON$", reason );
	return Out;
}

string CLanguage::BotDisabled( )
{
	return m_CFG->GetString( "lang_0126", "lang_0126" );
}

string CLanguage::BotEnabled( )
{
	return m_CFG->GetString( "lang_0127", "lang_0127" );
}

string CLanguage::UnableToCreateGameInvalidMap( string gamename )
{
	string Out = m_CFG->GetString( "lang_0128", "lang_0128" );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::WaitingForPlayersBeforeAutoStartVS( string players, string playersleft, string trigger )
{
	string Out = m_CFG->GetString( "lang_0129", "lang_0129" );
	Replace( Out, "$PLAYERS$", players );
	Replace( Out, "$PLAYERSLEFT$", playersleft );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::AutoStartDisabled( string trigger )
{
	string Out = m_CFG->GetString( "lang_0130", "lang_0130" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::AutoStartEnabled( string players )
{
	string Out = m_CFG->GetString( "lang_0131", "lang_0131" );
	Replace( Out, "$PLAYERS$", players );
	return Out;
}

string CLanguage::AnnounceMessageEnabled( )
{
	return m_CFG->GetString( "lang_0132", "lang_0132" );
}

string CLanguage::AnnounceMessageDisabled( )
{
	return m_CFG->GetString( "lang_0133", "lang_0133" );
}

string CLanguage::AutoHostEnabled( )
{
	return m_CFG->GetString( "lang_0134", "lang_0134" );
}

string CLanguage::AutoHostDisabled( )
{
	return m_CFG->GetString( "lang_0135", "lang_0135" );
}

string CLanguage::UnableToLoadSaveGamesOutside( )
{
	return m_CFG->GetString( "lang_0136", "lang_0136" );
}

string CLanguage::UnableToLoadSaveGameGameInLobby( )
{
	return m_CFG->GetString( "lang_0137", "lang_0137" );
}

string CLanguage::LoadingSaveGame( string file )
{
	string Out = m_CFG->GetString( "lang_0138", "lang_0138" );
	Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage::UnableToLoadSaveGameDoesntExist( string file )
{
	string Out = m_CFG->GetString( "lang_0139", "lang_0139" );
	Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage::UnableToCreateGameInvalidSaveGame( string gamename )
{
	string Out = m_CFG->GetString( "lang_0140", "lang_0140" );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::UnableToCreateGameSaveGameMapMismatch( string gamename )
{
	string Out = m_CFG->GetString( "lang_0141", "lang_0141" );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::AutoSaveEnabled( )
{
	return m_CFG->GetString( "lang_0142", "lang_0142" );
}

string CLanguage::AutoSaveDisabled( )
{
	return m_CFG->GetString( "lang_0143", "lang_0143" );
}

string CLanguage::DesyncDetected( )
{
	return m_CFG->GetString( "lang_0144", "lang_0144" );
}

string CLanguage::UnableToMuteNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0145", "lang_0145" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::MutedPlayer( string victim, string user )
{
	string Out = m_CFG->GetString( "lang_0146", "lang_0146" );
	Replace( Out, "$VICTIM$", victim );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::UnmutedPlayer( string victim, string user )
{
	string Out = m_CFG->GetString( "lang_0147", "lang_0147" );
	Replace( Out, "$VICTIM$", victim );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::UnableToMuteFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0148", "lang_0148" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::PlayerIsSavingTheGame( string player )
{
	string Out = m_CFG->GetString( "lang_0149", "lang_0149" );
	Replace( Out, "$PLAYER$", player );
	return Out;
}

string CLanguage::UpdatingClanList( )
{
	return m_CFG->GetString( "lang_0150", "lang_0150" );
}

string CLanguage::UpdatingFriendsList( )
{
	return m_CFG->GetString( "lang_0151", "lang_0151" );
}

string CLanguage::MultipleIPAddressUsageDetected( string player, string others )
{
	string Out = m_CFG->GetString( "lang_0152", "lang_0152" );
	Replace( Out, "$PLAYER$", player );
	Replace( Out, "$OTHERS$", others );
	return Out;
}

string CLanguage::UnableToVoteKickAlreadyInProgress( )
{
	return m_CFG->GetString( "lang_0153", "lang_0153" );
}

string CLanguage::UnableToVoteKickNotEnoughPlayers( )
{
	return m_CFG->GetString( "lang_0154", "lang_0154" );
}

string CLanguage::UnableToVoteKickNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0155", "lang_0155" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::UnableToVoteKickPlayerIsReserved( string victim )
{
	string Out = m_CFG->GetString( "lang_0156", "lang_0156" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::StartedVoteKick( string victim, string user, string votesneeded )
{
	string Out = m_CFG->GetString( "lang_0157", "lang_0157" );
	Replace( Out, "$VICTIM$", victim );
	Replace( Out, "$USER$", user );
	Replace( Out, "$VOTESNEEDED$", votesneeded );
	return Out;
}

string CLanguage::UnableToVoteKickFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0158", "lang_0158" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::VoteKickPassed( string victim )
{
	string Out = m_CFG->GetString( "lang_0159", "lang_0159" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::ErrorVoteKickingPlayer( string victim )
{
	string Out = m_CFG->GetString( "lang_0160", "lang_0160" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::VoteKickAcceptedNeedMoreVotes( string victim, string user, string votes )
{
	string Out = m_CFG->GetString( "lang_0161", "lang_0161" );
	Replace( Out, "$VICTIM$", victim );
	Replace( Out, "$USER$", user );
	Replace( Out, "$VOTES$", votes );
	return Out;
}

string CLanguage::VoteKickCancelled( string victim )
{
	string Out = m_CFG->GetString( "lang_0162", "lang_0162" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::VoteKickExpired( string victim )
{
	string Out = m_CFG->GetString( "lang_0163", "lang_0163" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::WasKickedByVote( )
{
	return m_CFG->GetString( "lang_0164", "lang_0164" );
}

string CLanguage::TypeYesToVote( string commandtrigger )
{
	string Out = m_CFG->GetString( "lang_0165", "lang_0165" );
	Replace( Out, "$COMMANDTRIGGER$", commandtrigger );
	return Out;
}

string CLanguage::PlayersNotYetPingedAutoStart( string notpinged )
{
	string Out = m_CFG->GetString( "lang_0166", "lang_0166" );
	Replace( Out, "$NOTPINGED$", notpinged );
	return Out;
}

string CLanguage::WasKickedForNotSpoofChecking( )
{
	return m_CFG->GetString( "lang_0167", "lang_0167" );
}

string CLanguage::WasKickedForHavingFurthestScore( string score, string average )
{
	string Out = m_CFG->GetString( "lang_0168", "lang_0168" );
	Replace( Out, "$SCORE$", score );
	Replace( Out, "$AVERAGE$", average );
	return Out;
}

string CLanguage::PlayerHasScore( string player, string score )
{
	string Out = m_CFG->GetString( "lang_0169", "lang_0169" );
	Replace( Out, "$PLAYER$", player );
	Replace( Out, "$SCORE$", score );
	return Out;
}

string CLanguage::RatedPlayersSpread( string rated, string total, string spread )
{
	string Out = m_CFG->GetString( "lang_0170", "lang_0170" );
	Replace( Out, "$RATED$", rated );
	Replace( Out, "$TOTAL$", total );
	Replace( Out, "$SPREAD$", spread );
	return Out;
}

string CLanguage::ErrorListingMaps( )
{
	return m_CFG->GetString( "lang_0171", "lang_0171" );
}

string CLanguage::FoundMaps( string maps )
{
	string Out = m_CFG->GetString( "lang_0172", "lang_0172" );
	Replace( Out, "$MAPS$", maps );
	return Out;
}

string CLanguage::NoMapsFound( )
{
	return m_CFG->GetString( "lang_0173", "lang_0173" );
}

string CLanguage::ErrorListingMapConfigs( )
{
	return m_CFG->GetString( "lang_0174", "lang_0174" );
}

string CLanguage::FoundMapConfigs( string mapconfigs )
{
	string Out = m_CFG->GetString( "lang_0175", "lang_0175" );
	Replace( Out, "$MAPCONFIGS$", mapconfigs );
	return Out;
}

string CLanguage::NoMapConfigsFound( )
{
	return m_CFG->GetString( "lang_0176", "lang_0176" );
}

string CLanguage::PlayerFinishedLoading( string user )
{
	string Out = m_CFG->GetString( "lang_0177", "lang_0177" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::PleaseWaitPlayersStillLoading( )
{
	return m_CFG->GetString( "lang_0178", "lang_0178" );
}

string CLanguage::MapDownloadsDisabled( )
{
	return m_CFG->GetString( "lang_0179", "lang_0179" );
}

string CLanguage::MapDownloadsEnabled( )
{
	return m_CFG->GetString( "lang_0180", "lang_0180" );
}

string CLanguage::MapDownloadsConditional( )
{
	return m_CFG->GetString( "lang_0181", "lang_0181" );
}

string CLanguage::SettingHCL( string HCL )
{
	string Out = m_CFG->GetString( "lang_0182", "lang_0182" );
	Replace( Out, "$HCL$", HCL );
	return Out;
}

string CLanguage::UnableToSetHCLInvalid( )
{
	return m_CFG->GetString( "lang_0183", "lang_0183" );
}

string CLanguage::UnableToSetHCLTooLong( )
{
	return m_CFG->GetString( "lang_0184", "lang_0184" );
}

string CLanguage::TheHCLIs( string HCL )
{
	string Out = m_CFG->GetString( "lang_0185", "lang_0185" );
	Replace( Out, "$HCL$", HCL );
	return Out;
}

string CLanguage::TheHCLIsTooLongUseForceToStart( string trigger )
{
	string Out = m_CFG->GetString( "lang_0186", "lang_0186" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::ClearingHCL( )
{
	return m_CFG->GetString( "lang_0187", "lang_0187" );
}

string CLanguage::TryingToRehostAsPrivateGame( string gamename )
{
	string Out = m_CFG->GetString( "lang_0188", "lang_0188" );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::TryingToRehostAsPublicGame( string gamename )
{
	string Out = m_CFG->GetString( "lang_0189", "lang_0189" );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::RehostWasSuccessful( )
{
	return m_CFG->GetString( "lang_0190", "lang_0190" );
}

string CLanguage::TryingToJoinTheGameButBannedByName( string victim )
{
	string Out = m_CFG->GetString( "lang_0191", "lang_0191" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::TryingToJoinTheGameButBannedByIP( string victim, string ip, string bannedname )
{
	string Out = m_CFG->GetString( "lang_0192", "lang_0192" );
	Replace( Out, "$VICTIM$", victim );
	Replace( Out, "$IP$", ip );
	Replace( Out, "$BANNEDNAME$", bannedname );
	return Out;
}

string CLanguage::HasBannedName( string victim )
{
	string Out = m_CFG->GetString( "lang_0193", "lang_0193" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::HasBannedIP( string victim, string ip, string bannedname )
{
	string Out = m_CFG->GetString( "lang_0194", "lang_0194" );
	Replace( Out, "$VICTIM$", victim );
	Replace( Out, "$IP$", ip );
	Replace( Out, "$BANNEDNAME$", bannedname );
	return Out;
}

string CLanguage::PlayersInGameState( string number, string players )
{
	string Out = m_CFG->GetString( "lang_0195", "lang_0195" );
	Replace( Out, "$NUMBER$", number );
	Replace( Out, "$PLAYERS$", players );
	return Out;
}

string CLanguage::ValidServers( string servers )
{
	string Out = m_CFG->GetString( "lang_0196", "lang_0196" );
	Replace( Out, "$SERVERS$", servers );
	return Out;
}

string CLanguage::TeamCombinedScore( string team, string score )
{
	string Out = m_CFG->GetString( "lang_0197", "lang_0197" );
	Replace( Out, "$TEAM$", team );
	Replace( Out, "$SCORE$", score );
	return Out;
}

string CLanguage::BalancingSlotsCompleted( )
{
	return m_CFG->GetString( "lang_0198", "lang_0198" );
}

string CLanguage::PlayerWasKickedForFurthestScore( string name, string score, string average )
{
	string Out = m_CFG->GetString( "lang_0199", "lang_0199" );
	Replace( Out, "$NAME$", name );
	Replace( Out, "$SCORE$", score );
	Replace( Out, "$AVERAGE$", average );
	return Out;
}

string CLanguage::LocalAdminMessagesEnabled( )
{
	return m_CFG->GetString( "lang_0200", "lang_0200" );
}

string CLanguage::LocalAdminMessagesDisabled( )
{
	return m_CFG->GetString( "lang_0201", "lang_0201" );
}

string CLanguage::WasDroppedDesync( )
{
	return m_CFG->GetString( "lang_0202", "lang_0202" );
}

string CLanguage::WasKickedForHavingLowestScore( string score )
{
	string Out = m_CFG->GetString( "lang_0203", "lang_0203" );
	Replace( Out, "$SCORE$", score );
	return Out;
}

string CLanguage::PlayerWasKickedForLowestScore( string name, string score )
{
	string Out = m_CFG->GetString( "lang_0204", "lang_0204" );
	Replace( Out, "$NAME$", name );
	Replace( Out, "$SCORE$", score );
	return Out;
}

string CLanguage::ReloadingConfigurationFiles( )
{
	return m_CFG->GetString( "lang_0205", "lang_0205" );
}

string CLanguage::CountDownAbortedSomeoneLeftRecently( )
{
	return m_CFG->GetString( "lang_0206", "lang_0206" );
}

string CLanguage::UnableToCreateGameMustEnforceFirst( string gamename )
{
	string Out = m_CFG->GetString( "lang_0207", "lang_0207" );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::UnableToLoadReplaysOutside( )
{
	return m_CFG->GetString( "lang_0208", "lang_0208" );
}

string CLanguage::LoadingReplay( string file )
{
	string Out = m_CFG->GetString( "lang_0209", "lang_0209" );
	Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage::UnableToLoadReplayDoesntExist( string file )
{
	string Out = m_CFG->GetString( "lang_0210", "lang_0210" );
	Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage::CommandTrigger( string trigger )
{
	string Out = m_CFG->GetString( "lang_0211", "lang_0211" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::CantEndGameOwnerIsStillPlaying( string owner )
{
	string Out = m_CFG->GetString( "lang_0212", "lang_0212" );
	Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage::CantUnhostGameOwnerIsPresent( string owner )
{
	string Out = m_CFG->GetString( "lang_0213", "lang_0213" );
	Replace( Out, "$OWNER$", owner );
	return Out;
}


string CLanguage::WasAutomaticallyDroppedAfterSeconds( string seconds )
{
	string Out = m_CFG->GetString( "lang_0214", "lang_0214" );
	Replace( Out, "$SECONDS$", seconds );
	return Out;
}

string CLanguage::HasLostConnectionTimedOutGProxy( )
{
	return m_CFG->GetString( "lang_0215", "lang_0215" );
}

string CLanguage::HasLostConnectionSocketErrorGProxy( string error )
{
	string Out = m_CFG->GetString( "lang_0216", "lang_0216" );
	Replace( Out, "$ERROR$", error );
	return Out;
}

string CLanguage::HasLostConnectionClosedByRemoteHostGProxy( )
{
	return m_CFG->GetString( "lang_0217", "lang_0217" );
}

string CLanguage::WaitForReconnectSecondsRemain( string seconds )
{
	string Out = m_CFG->GetString( "lang_0218", "lang_0218" );
	Replace( Out, "$SECONDS$", seconds );
	return Out;
}

string CLanguage::WasUnrecoverablyDroppedFromGProxy( )
{
	return m_CFG->GetString( "lang_0219", "lang_0219" );
}

string CLanguage::PlayerReconnectedWithGProxy( string name )
{
	string Out = m_CFG->GetString( "lang_0220", "lang_0220" );
	Replace( Out, "$NAME$", name );
	return Out;
}

string CLanguage::HasLeftTheGame( )
{
    return m_CFG->GetString( "lang_0221", "lang_0221" );
}

string CLanguage::DisplayWarnsCount( string WarnCount )
{
    string Out = m_CFG->GetString( "lang_0223", "lang_0223" );
	Replace( Out, "$WARNNUM$", WarnCount );
	return Out;
}

string CLanguage::WarnedUser( string Server, string Victim, string WarnCount )
{
    string Out = m_CFG->GetString( "lang_0224", "lang_0224" );
	Replace( Out, "$SERVER$", Server );
	Replace( Out, "$VICTIM$", Victim );
	Replace( Out, "$WARNNUM$", WarnCount );
	return Out;
}

string CLanguage::ErrorWarningUser( string server, string victim )
{
    string Out = m_CFG->GetString( "lang_0225", "lang_0225" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::UnableToWarnNoMatchesFound( string victim )
{
    string Out = m_CFG->GetString( "lang_0226", "lang_0226" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::PlayerWasWarnedByPlayer( string victim, string user, string warnnum )
{
    string Out = m_CFG->GetString( "lang_0227", "lang_0227" );
	Replace( Out, "$VICTIM$", victim );
	Replace( Out, "$USER$", user );
	Replace( Out, "$WARNNUM$", warnnum );
	return Out;
}

string CLanguage::UnableToWarnFoundMoreThanOneMatch( string victim )
{
    string Out = m_CFG->GetString( "lang_0228", "lang_0228" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::UnwarnedUser( string victim )
{
    string Out = m_CFG->GetString( "lang_0229", "lang_0229" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::ErrorUnwarningUser( string victim )
{
    string Out = m_CFG->GetString( "lang_0230", "lang_0230" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::UserReachedWarnQuota( string victim, string bantime )
{
	string Out;

    if( bantime != "0" )
	{
        Out = m_CFG->GetString( "lang_0231", "lang_0231" );
		Replace( Out, "$VICTIM$", victim );
		Replace( Out, "$BANDAYTIME$", bantime );
	}
	else
	{
		// ban is permanent, don't display "0 bandaytime"
        Out = m_CFG->GetString( "lang_0232", "lang_0232" );
		Replace( Out, "$VICTIM$", victim );
	}

	return Out;
}

string CLanguage::IncorrectCommandSyntax( )
{
    string Out = m_CFG->GetString( "lang_0233", "lang_0233" );
	return Out;
}

string CLanguage::TempBannedUser( string server, string victim, string bantime )
{
    string Out;

    if( bantime != "0" )
	{
        Out = m_CFG->GetString( "lang_0234", "lang_0234" );
		Replace( Out, "$SERVER$", server );
		Replace( Out, "$VICTIM$", victim );
		Replace( Out, "$BANDAYTIME$", bantime );
	}
	else
	{
		// ban is permanent, don't display "0 bandaytime"
		Out = BannedUser( server, victim );
	}

	return Out;
}

string CLanguage::UserIsNotWarned( string victim )
{
    string Out = m_CFG->GetString( "lang_0235", "lang_0235" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::UserWarnReasons( string victim, string warnnum )
{
    string Out = m_CFG->GetString( "lang_0236", "lang_0236" );
	Replace( Out, "$VICTIM$", victim );
	Replace( Out, "$WARNNUM$", warnnum );
	return Out;
}

string CLanguage::UserBanWarnReasons( string victim )
{
    string Out = m_CFG->GetString( "lang_0237", "lang_0237" );
	Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage::CannotAutoWarn( )
{
    string Out = m_CFG->GetString( "lang_0238", "lang_0238" );
	return Out;
}

string CLanguage::AutoWarnEnabled( )
{
    string Out = m_CFG->GetString( "lang_0239", "lang_0239" );
	return Out;
}

string CLanguage::AutoWarnDisabled( )
{
    string Out = m_CFG->GetString( "lang_0240", "lang_0240" );
	return Out;
}

string CLanguage::ShamanJoinedTheChannel( string user )
{
    string Out = m_CFG->GetString( "lang_0242", "lang_0242" );
    Replace( Out, "$USER$", user );
    return Out;
}

string CLanguage::ChieftainJoinedTheChannel( string user )
{
    string Out = m_CFG->GetString( "lang_0243", "lang_0243" );
    Replace( Out, "$USER$", user );
    return Out;
}

string CLanguage::SafeJoinedTheChannel( string user )
{
    string Out = m_CFG->GetString( "lang_0244", "lang_0244" );
    Replace( Out, "$USER$", user );
    return Out;
}

string CLanguage::AdminJoinedTheChannel( string user )
{
    string Out = m_CFG->GetString( "lang_0245", "lang_0245" );
    Replace( Out, "$USER$", user );
    return Out;
}

string CLanguage::RootAdminJoinedTheChannel( string user )
{
    string Out = m_CFG->GetString( "lang_0246", "lang_0246" );
    Replace( Out, "$USER$", user );
    return Out;
}

string CLanguage::PlayerMutedForBeingFoulMouthed( string user, string time )
{
    string Out = m_CFG->GetString( "lang_0247", "lang_0247" );
    Replace( Out, "$USER$", user );
    Replace( Out, "$TIME$", time );
    return Out;
}

string CLanguage::PlayerIsNotInTheSafeList( string user )
{
    string Out = m_CFG->GetString( "lang_0248", "lang_0248" );
    Replace( Out, "$USER$", user );
    return Out;
}

string CLanguage::PlayerIsInTheSafeList( string user, string voucher )
{
    string Out = m_CFG->GetString( "lang_0249", "lang_0249" );
    Replace( Out, "$USER$", user );
    Replace( Out, "$VOUCHER$", voucher );
    return Out;
}

string CLanguage::AddedPlayerToTheSafeList( string user )
{
    string Out = m_CFG->GetString( "lang_0250", "lang_0250" );
    Replace( Out, "$USER$", user );
    return Out;
}

string CLanguage::RemovedPlayerFromTheSafeList( string user )
{
    string Out = m_CFG->GetString( "lang_0251", "lang_0251" );
    Replace( Out, "$USER$", user );
    return Out;
}

string CLanguage::HasPlayedDotAGamesWithThisBot2( string user, string totalgames, string winspergame, string lossespergame, string killspergame, string deathspergame, string creepkillspergame, string creepdeniespergame, string assistspergame, string neutralkillspergame, string towerkillspergame, string raxkillspergame, string courierkillspergame, string score, string rank )
{
    string Out;

    if( score == "-10000.00" || score == "0" || score == "0.00" )
        Out = m_CFG->GetString( "lang_0257", "lang_0257" );
    else
        Out = m_CFG->GetString( "lang_0252", "lang_0252" );

    double k = UTIL_ToDouble( killspergame );
    double d = UTIL_ToDouble( deathspergame );
    double a = UTIL_ToDouble( assistspergame );
    string title1 = "Helper";
    string title2 = "Attacker";

    if( k >= d && k >= a )
        title1 = "Attacker";
    if( d >= a && d >= k )
        title1 = "Suicider";
    if( a >= k && a >= d )
        title1 = "Helper";

    if( ( a >= k && a <= d ) || ( a >= d && a <= k ) )
        title2 = "Helper";
    if( ( k >= d && k <= a ) || ( k >= a && k <= d ) )
        title2 = "Attacker";
    if( ( d >= k && d <= a ) || ( d >= a && d <= k ) )
        title2 = "Suicider";

    Replace( Out, "$USER$", user );
    Replace( Out, "$TOTALGAMES$", totalgames );
    Replace( Out, "$TITLE1$", title1 );
    Replace( Out, "$TITLE2$", title2 );
    Replace( Out, "$WPG$", winspergame );
    Replace( Out, "$LPG$", lossespergame );
    Replace( Out, "$KPG$", killspergame );
    Replace( Out, "$DPG$", deathspergame );
    Replace( Out, "$CKPG$", creepkillspergame );
    Replace( Out, "$CDPG$", creepdeniespergame );
    Replace( Out, "$APG$", assistspergame );
    Replace( Out, "$NKPG$", neutralkillspergame );
    Replace( Out, "$TKPG$", towerkillspergame );
    Replace( Out, "$RKPG$", raxkillspergame );
    Replace( Out, "$CouKPG$", courierkillspergame );
    string Rank = string( );
    string Score = string( );

    if( rank != "0" )
        Rank = "#" + rank;

    if( score != "-10000.00" & score != "0.00" )
        Score = score;

    Replace( Out, "$SCORE$", Score );
    Replace( Out, "$RANK$", Rank );
    return Out;
}

string CLanguage::RemovedPlayerFromTheMuteList( string user )
{
    string Out = m_CFG->GetString( "lang_0253", "lang_0253" );
    Replace( Out, "$USER$", user );
    return Out;
}

string CLanguage::AddedPlayerToTheMuteList( string user )
{
    string Out = m_CFG->GetString( "lang_0254", "lang_0254" );
    Replace( Out, "$USER$", user );
    return Out;
}

string CLanguage::AutokickingPlayerForDeniedProvider( string victim, string provider )
{
    string Out = m_CFG->GetString( "lang_0255", "lang_0255" );
    Replace( Out, "$VICTIM$", victim );
    Replace( Out, "$PROVIDER$", provider );
    return Out;
}

string CLanguage::AutokickingPlayerForDeniedCountry( string victim, string country )
{
    string Out = m_CFG->GetString( "lang_0256", "lang_0256" );
    Replace( Out, "$VICTIM$", victim );
    Replace( Out, "$COUNTRY$", country );
    return Out;
}

string CLanguage::AutokickingPlayerForDeniedScore( string victim, string score, string reqscore )
{
    string Out = m_CFG->GetString( "lang_0258", "lang_0258" );
    Replace( Out, "$NAME$", victim );
    Replace( Out, "$SCORE$", score );
    Replace( Out, "$REQSCORE$", reqscore );
    return Out;
}

string CLanguage::PlayerIsNoted( string user, string note )
{
    string Out = m_CFG->GetString( "lang_0259", "lang_0259" );
	Replace( Out, "$USER$", user );
	Replace( Out, "$NOTE$", note );
	return Out;
}

string CLanguage::PlayerIsNotNoted( string user )
{
    string Out = m_CFG->GetString( "lang_0260", "lang_0260" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::AddedPlayerToNoteList( string user )
{
    string Out = m_CFG->GetString( "lang_0261", "lang_0261" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::ChangedPlayerNote( string user )
{
    string Out = m_CFG->GetString( "lang_0262", "lang_0262" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::RemovedPlayerFromNoteList( string user )
{
    string Out = m_CFG->GetString( "lang_0263", "lang_0263" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::PlayerNeedsToReady( string user, string trigger )
{
    string Out = m_CFG->GetString( "lang_0264", "lang_0264" );
	Replace( Out, "$USER$", user );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::EverybodyIsReady( string user, string trigger )
{
    string Out = m_CFG->GetString( "lang_0265", "lang_0265" );
	Replace( Out, "$USER$", user );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::PlayerIsReady( string user, string trigger )
{
    string Out = m_CFG->GetString( "lang_0266", "lang_0266" );
	Replace( Out, "$USER$", user );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::EverybodyIsReadyToStartGame( )
{
    string Out = m_CFG->GetString( "lang_0267", "lang_0267" );
	return Out;
}

string CLanguage::WimAdvise( string trigger )
{
    string Out = m_CFG->GetString( "lang_0268", "lang_0268" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::ReadyState( string vote, string player, string voteneed )
{
    string Out = m_CFG->GetString( "lang_0269", "lang_0269" );
	Replace( Out, "$VOTES$", vote );
	Replace( Out, "$PLAYER$", player );
	Replace( Out, "$VOTENEED$", voteneed );
	return Out;
}

string CLanguage::RdyState( string vote, string trigger )
{
    string Out = m_CFG->GetString( "lang_0270", "lang_0270" );
	Replace( Out, "$VOTES$", vote );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::FFOath( string user )
{
    string Out = m_CFG->GetString( "lang_0271", "lang_0271" );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::FFResignAndWin( string loser, string winner )
{
    string Out = m_CFG->GetString( "lang_0272", "lang_0272" );
	Replace( Out, "$TEAM_X$", loser );
	Replace( Out, "$TEAM_Y$", winner );
	return Out;
}

string CLanguage::FFCounter( string team )
{
    string Out = m_CFG->GetString( "lang_0273", "lang_0273" );
	Replace( Out, "$TEAM$", team );
	return Out;
}

string CLanguage::BothTeamsMustHaveAPlayer( )
{
    string Out = m_CFG->GetString( "lang_0274", "lang_0274" );
	return Out;
}

string CLanguage::SomeOneJustLeft( )
{
    string Out = m_CFG->GetString( "lang_0275", "lang_0275" );
	return Out;
}

string CLanguage::FeatureBlocked( )
{
    string Out = m_CFG->GetString( "lang_0276", "lang_0276" );
	return Out;
}

string CLanguage::OwnerForLobbyControl( string trigger )
{
    string Out = m_CFG->GetString( "lang_0277", "lang_0277" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::Publicity( string web, string groupid )
{
    string Out = m_CFG->GetString( "lang_0278", "lang_0278" );
	Replace( Out, "$WEB$", web );
	Replace( Out, "$GROUPID$", groupid );
	return Out;
}

string CLanguage::AdviseToStartGameWithOwnerRight( string trigger )
{
    string Out = m_CFG->GetString( "lang_0279", "lang_0279" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::AdviseWhenLobbyFull( string trigger )
{
    string Out = m_CFG->GetString( "lang_0280", "lang_0280" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::AtLeastXPlayersToStartGame( string num )
{
    string Out = m_CFG->GetString( "lang_0281", "lang_0281" );
	Replace( Out, "$X$", num );
	return Out;
}

string CLanguage::TeamImba( string trigger )
{
    string Out = m_CFG->GetString( "lang_0282", "lang_0282" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::TotalLaggingTimeLimit( )
{
    string Out = m_CFG->GetString( "lang_0283", "lang_0283" );
	return Out;
}

string CLanguage::CommandDisabled( string command )
{
    string Out = m_CFG->GetString( "lang_0284", "lang_0284" );
    Replace( Out, "$COMMAND$", command );
	return Out;
}

string CLanguage::NoAdminKick( )
{
    string Out = m_CFG->GetString( "lang_0285", "lang_0285" );
	return Out;
}

string CLanguage::DoDFInstead( string trigger )
{
    string Out = m_CFG->GetString( "lang_0286", "lang_0286" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::UnableToCommand( string trigger )
{
    string Out = m_CFG->GetString( "lang_0287", "lang_0287" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::InvalidNickname( )
{
    string Out = m_CFG->GetString( "lang_0288", "lang_0288" );
	return Out;
}

string CLanguage::AutostartCanceled( )
{
    string Out = m_CFG->GetString( "lang_0289", "lang_0289" );
	return Out;
}

string CLanguage::KickMsgForLowDLRate( string name, string dlrate, string dlratequota )
{
    string Out = m_CFG->GetString( "lang_0290", "lang_0290" );
	Replace( Out, "$NAME$", name );
	Replace( Out, "$DLRATE$", dlrate );
	Replace( Out, "$DLRATEQUOTA$", dlratequota );
	return Out;
}

string CLanguage::StartDisabledButVSAndTOEnabled( string trigger )
{
    string Out = m_CFG->GetString( "lang_0291", "lang_0291" );
	Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage::StartDisabledButVSEnabled( string trigger )
{
    string Out = m_CFG->GetString( "lang_0292", "lang_0292" );
    Replace( Out, "$TRIGGER$", trigger );
    return Out;
}

string CLanguage::StartDisabledButTOEnabled( string trigger )
{
    string Out = m_CFG->GetString( "lang_0293", "lang_0293" );
    Replace( Out, "$TRIGGER$", trigger );
    return Out;
}

string CLanguage::StartDisabled( string trigger )
{
    string Out = m_CFG->GetString( "lang_0294", "lang_0294" );
    Replace( Out, "$TRIGGER$", trigger );
    return Out;
}

string CLanguage::AutoBanEnabled( string ban, string timer )
{
    string Out = m_CFG->GetString( "lang_0295", "lang_0295" );
	Replace( Out, "$BAN$", ban );
	Replace( Out, "$TIMER$", timer );
	return Out;
}

string CLanguage::PlayerMutedForBeingFloodMouthed( string name, string time )
{
    string Out = m_CFG->GetString( "lang_0296", "lang_0296" );
	Replace( Out, "$NAME$", name );
	Replace( Out, "$TIME$", time );
	return Out;
}

string CLanguage::RemovedPlayerFromTheAutoMuteList( string name )
{
    string Out = m_CFG->GetString( "lang_0297", "lang_0297" );
	Replace( Out, "$NAME$", name );
	return Out;
}

string CLanguage::BotChannel( string channel )
{
    string Out = m_CFG->GetString( "lang_0298", "lang_0298" );
	Replace( Out, "$CHANNEL$", channel );
	return Out;
}

string CLanguage::GameOwner( string owner )
{
    string Out = m_CFG->GetString( "lang_0299", "lang_0299" );
	Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage::GameName( string gamename )
{
    string Out = m_CFG->GetString( "lang_0300", "lang_0300" );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::GameMode( string HCL )
{
    string Out = m_CFG->GetString( "lang_0301", "lang_0301" );
	Replace( Out, "$HCL$", HCL );
	return Out;
}

string CLanguage::PlayersNotYetPingedRus( string number, string notpinged )
{
    string Out = m_CFG->GetString( "lang_0302", "lang_0302" );
	Replace( Out, "$NUMBER$", number );
	Replace( Out, "$NOTPINGED$", notpinged );
	return Out;
}

string CLanguage::GameIsStarted( string description )
{
    string Out = m_CFG->GetString( "lang_0303", "lang_0303" );
	Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage::WaitingForPlayersBeforeAutoStart( string players, string playersleft )
{
    string Out = m_CFG->GetString( "lang_0304", "lang_0304" );
	Replace( Out, "$PLAYERS$", players );
	Replace( Out, "$PLAYERSLEFT$", playersleft );
	return Out;
}

string CLanguage::AutoCreatingPrivateGame( string gamename )
{
    string Out = m_CFG->GetString( "lang_0305", "lang_0305" );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::AutoCreatingPublicGame( string gamename )
{
    string Out = m_CFG->GetString( "lang_0306", "lang_0306" );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::GameOwners( string owners )
{
    string Out = m_CFG->GetString( "lang_0307", "lang_0307" );
	Replace( Out, "$OWNERS$", owners );
	return Out;
}

string CLanguage::GameHost( string host )
{
    string Out = m_CFG->GetString( "lang_0308", "lang_0308" );
	Replace( Out, "$HOST$", host );
	return Out;
}

string CLanguage::AddedUserToAdminDatabase2( string server, string user )
{
    string Out = m_CFG->GetString( "lang_0309", "lang_0309" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage::AddedUserToAdminDatabase3( string server, string user, string date )
{
    string Out = m_CFG->GetString( "lang_0310", "lang_0310" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	Replace( Out, "$DATE$", date );
	return Out;
}

string CLanguage::AddedUserToAdminDatabase4( string server, string user, string date )
{
    string Out = m_CFG->GetString( "lang_0311", "lang_0311" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	Replace( Out, "$DATE$", date );
	return Out;
}

string CLanguage::AddedUserToAdminDatabase5( string server, string user, string date )
{
    string Out = m_CFG->GetString( "lang_0312", "lang_0312" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	Replace( Out, "$DATE$", date );
	return Out;
}

string CLanguage::AddedUserToAdminDatabase6( string server, string user, string date )
{
    string Out = m_CFG->GetString( "lang_0313", "lang_0313" );
	Replace( Out, "$SERVER$", server );
	Replace( Out, "$USER$", user );
	Replace( Out, "$DATE$", date );
	return Out;
}

string CLanguage::KickMsgForSlowDL( string name )
{
    string Out = m_CFG->GetString( "lang_0314", "lang_0314" );
	Replace( Out, "$NAME$", name );
	return Out;
}

string CLanguage::KickMsgForLongDL( string name, string dltime, string dltimequota )
{
    string Out = m_CFG->GetString( "lang_0315", "lang_0315" );
	Replace( Out, "$NAME$", name );
	Replace( Out, "$DLTIME$", dltime );
	Replace( Out, "$DLTIMEQUOTA$", dltimequota );
	return Out;
}

string CLanguage::BotName( string name )
{
    string Out = m_CFG->GetString( "lang_0316", "lang_0316" );
	Replace( Out, "$NAME$", name );
	return Out;
}

string CLanguage::CMCGWinners( )
{
    string Out = m_CFG->GetString( "lang_0317", "lang_0317" );
	return Out;
}

string CLanguage::CMCGWinnersNotYet( )
{
    string Out = m_CFG->GetString( "lang_0318", "lang_0318" );
	return Out;
}

string CLanguage::CMCGClearWinners( string map )
{
    string Out = m_CFG->GetString( "lang_0319", "lang_0319" );
	Replace( Out, "$MAP$", map );
	return Out;
}

string CLanguage::CMCGSetWinner( string map, string winner )
{
    string Out = m_CFG->GetString( "lang_0320", "lang_0320" );
	Replace( Out, "$MAP$", map );
	Replace( Out, "$WINNER$", winner );
	return Out;
}

string CLanguage::CMCGSetWinners( string map, string winners )
{
    string Out = m_CFG->GetString( "lang_0321", "lang_0321" );
	Replace( Out, "$MAP$", map );
	Replace( Out, "$WINNERS$", winners );
	return Out;
}

string CLanguage::CMCGCongratsWinners( string map, string winners )
{
    string Out = m_CFG->GetString( "lang_0322", "lang_0322" );
	Replace( Out, "$MAP$", map );
	Replace( Out, "$WINNERS$", winners );
	return Out;
}

string CLanguage::CMCGAdmins( string admins )
{
    string Out = m_CFG->GetString( "lang_0323", "lang_0323" );
	Replace( Out, "$ADMINS$", admins );
	return Out;
}

string CLanguage::CMCGAdminsNotYet( )
{
    string Out = m_CFG->GetString( "lang_0324", "lang_0324" );
	return Out;
}

string CLanguage::CMCGAwards( )
{
    string Out = m_CFG->GetString( "lang_0325", "lang_0325" );
	return Out;
}

string CLanguage::CMCGSchedule( )
{
    string Out = m_CFG->GetString( "lang_0326", "lang_0326" );
	return Out;
}

string CLanguage::CMCGTimeUntilStart( string time )
{
    string Out = m_CFG->GetString( "lang_0327", "lang_0327" );
	Replace( Out, "$TIME$", time );
	return Out;
}

string CLanguage::CMCGTimeFromStart( string time )
{
    string Out = m_CFG->GetString( "lang_0328", "lang_0328" );
	Replace( Out, "$TIME$", time );
	return Out;
}

string CLanguage::CMCGBeginRightNow( )
{
    string Out = m_CFG->GetString( "lang_0329", "lang_0329" );
	return Out;
}

string CLanguage::CMCGStartRightNow( )
{
    string Out = m_CFG->GetString( "lang_0330", "lang_0330" );
	return Out;
}

string CLanguage::CMCGMaps( string maps )
{
    string Out = m_CFG->GetString( "lang_0331", "lang_0331" );
	Replace( Out, "$MAPS$", maps );
	return Out;
}

string CLanguage::CMCGMapsNotYet( )
{
    string Out = m_CFG->GetString( "lang_0332", "lang_0332" );
	return Out;
}

string CLanguage::CMCGNextMap( string map, string admin )
{
    string Out = m_CFG->GetString( "lang_0333", "lang_0333" );
	Replace( Out, "$MAP$", map );
	Replace( Out, "$ADMIN$", admin );
	return Out;
}

string CLanguage::CMCGNextMap( string map )
{
    string Out = m_CFG->GetString( "lang_0334", "lang_0334" );
	Replace( Out, "$MAP$", map );
	return Out;
}

string CLanguage::CMCGCurrentMap( string map )
{
    string Out = m_CFG->GetString( "lang_0335", "lang_0335" );
	Replace( Out, "$MAP$", map );
	return Out;
}

string CLanguage::CMCGLastMap( string map )
{
    string Out = m_CFG->GetString( "lang_0336", "lang_0336" );
	Replace( Out, "$MAP$", map );
	return Out;
}

string CLanguage::CMCGAllMap( string maps )
{
    string Out = m_CFG->GetString( "lang_0337", "lang_0337" );
	Replace( Out, "$MAPS$", maps );
	return Out;
}

string CLanguage::CMCGSetMaps( string maps )
{
    string Out = m_CFG->GetString( "lang_0338", "lang_0338" );
	Replace( Out, "$MAPS$", maps );
	return Out;
}

string CLanguage::CMCGIncorrectAddMap( string trigger, string command )
{
    string Out = m_CFG->GetString( "lang_0339", "lang_0339" );
	Replace( Out, "$TRIGGER$", trigger );
	Replace( Out, "$COMMAND$", command );
	return Out;
}

string CLanguage::CMCGIncorrectCodeMap( string code )
{
    string Out = m_CFG->GetString( "lang_0340", "lang_0340" );
	Replace( Out, "$CODE$", code );
	return Out;
}

string CLanguage::CMCGFollowToAdmin( string admin, string gamename )
{
    string Out = m_CFG->GetString( "lang_0341", "lang_0341" );
	Replace( Out, "$ADMIN$", admin );
	Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage::CMCGFollowToAdmin( string admin )
{
    string Out = m_CFG->GetString( "lang_0342", "lang_0342" );
	Replace( Out, "$ADMIN$", admin );
	return Out;
}

string CLanguage::CMCGFollowToAdmin( )
{
    string Out = m_CFG->GetString( "lang_0343", "lang_0343" );
	return Out;
}

string CLanguage::CMCGSetMapsHelp( string trigger, string command )
{
    string Out = m_CFG->GetString( "lang_0344", "lang_0344" );
	Replace( Out, "$TRIGGER$", trigger );
	Replace( Out, "$COMMAND$", command );
	return Out;
}

string CLanguage::CMCGClearMapsHelp( string trigger, string command )
{
    string Out = m_CFG->GetString( "lang_0345", "lang_0345" );
	Replace( Out, "$TRIGGER$", trigger );
	Replace( Out, "$COMMAND$", command );
	return Out;
}

string CLanguage::CMCGClearMaps( )
{
    string Out = m_CFG->GetString( "lang_0346", "lang_0346" );
	return Out;
}

string CLanguage::CMCGSetAdminsHelp( string trigger, string command )
{
    string Out = m_CFG->GetString( "lang_0347", "lang_0347" );
	Replace( Out, "$TRIGGER$", trigger );
	Replace( Out, "$COMMAND$", command );
	return Out;
}

string CLanguage::CMCGClearAdminsHelp( string trigger, string command )
{
    string Out = m_CFG->GetString( "lang_0348", "lang_0348" );
	Replace( Out, "$TRIGGER$", trigger );
	Replace( Out, "$COMMAND$", command );
	return Out;
}

string CLanguage::CMCGClearAdmins( )
{
    string Out = m_CFG->GetString( "lang_0349", "lang_0349" );
	return Out;
}

string CLanguage::CMCGSetAdmins( string admins )
{
    string Out = m_CFG->GetString( "lang_0350", "lang_0350" );
	Replace( Out, "$ADMINS$", admins );
	return Out;
}

string CLanguage::CMCGFindOutRules( string trigger, string command )
{
    string Out = m_CFG->GetString( "lang_0351", "lang_0351" );
	Replace( Out, "$TRIGGER$", trigger );
	Replace( Out, "$COMMAND$", command );
	return Out;
}

string CLanguage::CountryFullName( string code, uint32_t casename )
{
    if( casename == 0 || m_CFG->GetString( code + UTIL_ToString( casename ), string( ) ).empty( ) )
        return m_CFG->GetString( code, code );
    else
        return m_CFG->GetString( code + UTIL_ToString( casename ), code );
}

