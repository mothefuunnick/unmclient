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

#ifndef GAME_H
#define GAME_H

//
// CGame
//

class CGame : public CBaseGame
{
protected:
	uint32_t m_GameOverTime;								// GetTime when the game was over as reported by the stats class
	bool m_DotaGame;

public:
    CGame( CUNM *nUNM, CMap *nMap, CSaveGame *nSaveGame, uint16_t nHostPort, unsigned char nGameState, std::string nGameName, std::string nOwnerName, std::string nCreatorName, std::string nCreatorServer );
	virtual ~CGame( );

	virtual bool Update( void *fd, void *send_fd );
	virtual void EventPlayerDeleted( CGamePlayer *player );
	virtual void EventPlayerLeft( CGamePlayer *player, uint32_t reason );
	virtual void EventPlayerAction( CGamePlayer *player, CIncomingAction *action );
    virtual bool EventPlayerBotCommand( CGamePlayer *player, std::string command, std::string payload );
	virtual void EventGameStarted( );
};

#endif
