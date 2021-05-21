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

#ifndef UNM_H
#define UNM_H

#include <QWidget>
#include <QDialog>
#include <QObject>
#include <QThread>
#include <QLabel>
#include <QListWidget>
#include <QTextEdit>
#include <QScrollBar>
#include <QtWebSockets/QWebSocket>
#include <QtNetworkAuth/QOAuth2AuthorizationCodeFlow>
#include <QtNetworkAuth/QOAuthHttpServerReplyHandler>
#include "customdynamicfontsizelabel.h"
#include "includes.h"
#include "config.h"
#include "configsection.h"

class ExampleObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY( uint32_t running READ running WRITE setRunning NOTIFY runningChanged )
    uint32_t m_running;
    std::string m_URL;
    std::string m_MapsPath;
    std::string m_Command;
    uint32_t m_BnetID;
    std::string m_UserName;
    bool m_Whisper;
    std::string m_LogFileName;
    bool m_ThreadEnd;

public:
    explicit ExampleObject( QObject *parent = nullptr );
    uint32_t running( ) const;
    void setThreadEnd( bool ThreadEnd );
    void setOptions( QList<QString> options );
    void setLogFileName( QString logfilename );

signals:
    void finishedUNM( bool mainThread );    // Сигнал, по которому будет производить полный выход из программы
    void finished( );    // Сигнал, по которому будем завершать поток, после завершения метода run
    void finished2( );    // Сигнал, по которому будем завершать поток, после завершения метода run
    void runningChanged( uint32_t running );
    void sendMessage( QString message );
    void toLog( QString message );
    void toGameLog( unsigned int gameID, QString message, bool toMainLog );
    void toChat( QString message, QString msg, int bnetID, int mainBnetID );
    void addChannelUser( QString message, int bnetID );
    void removeChannelUser( QString message, int bnetID );
    void clearChannelUser( int bnetID );
    void bnetDisconnect( int bnetID );
    void bnetReconnect( int bnetID );
    void setCommandsTips( bool first );
    void threadReady( bool mainThread );
    void setServer( QString message );
    void setLogin( QString message );
    void setPassword( QString message );
    void setRunAH( bool runAH );
    void setRememberMe( bool rememberMe );
    void logonSuccessful( );
    void logonFailed( QString message );
    void UpdateValuesInGUISignal( QList<QString> cfgvars );
    void downloadMap( QList<QString> options );
    void backupLog( QString filename );
    void restartUNM( bool now );
    void addChatTab( unsigned int bnetID, QString bnetCommandTrigger, QString serverName, QString userName, unsigned int timeWaitingToReconnect );
    void changeBnetUsername( unsigned int bnetID, QString userName );
    void addGame( unsigned int gameID, QStringList gameInfoInit, QList<QStringList> playersInfo );
    void deleteGame( unsigned int gameID );
    void updateGame( unsigned int gameID, QStringList gameInfo, QList<QStringList> playersInfo );
    void updateGameMapInfo( unsigned int gameID, QString mapPath );
    void toGameChat( QString message, QString extramessage, QString user, unsigned int type, unsigned int gameID );
    void addBnetBot( unsigned int bnetID, QString botName );
    void updateBnetBot( unsigned int bnetID, QString botName, unsigned int type );
    void deleteBnetBot( unsigned int bnetID, QString botName );
    void addGameToList( QList<unsigned int> bnetIDs, unsigned int gameID, bool gproxy, unsigned int gameIcon, QStringList gameInfo );
    void updateGameToList( QList<unsigned int> bnetIDs, unsigned int gameID, QStringList gameInfo, bool deleteFromCurrentGames );
    void deleteGameFromList( QList<unsigned int> bnetIDs, unsigned int gameID, unsigned int type );
    void addGameToListIrina( unsigned int gameID, bool gameStarted, unsigned int gameIcon, QStringList gameInfo );
    void updateGameToListIrina( unsigned int gameID, bool gameStarted, QStringList gameInfo, bool deleteFromCurrentGames );
    void deleteGameFromListIrina( unsigned int gameID, unsigned int type );
    void sendGetgame( unsigned int gameID, unsigned char packetType );
    void setSelectGameData( unsigned int gameID, QString mapName, QString mapPlayers, QString mapAuthor, QString mapDescription, QPixmap mapImage );
    void setSelectGameDataBadTga( unsigned int gameID, QString mapName, QString mapPlayers, QString mapAuthor, QString mapDescription );
    void authButtonReset( );
    void deleteOldMessages( );
    void closeConnect( );
    void openConnect( );
    void sendLogin( );
    void badLogon( );
    void gameListTablePosFix( QList<unsigned int> bnetIDs );
    void gameListTablePosFixIrina( );
    void successLogon( );
    void sendPing( );
    void sendGamelist( );
    void successInit( );

public slots:
    void run( ); // Метод с полезной нагрузкой, который может выполняться в цикле
    void setRunning( uint32_t running );
};

namespace Ui {
class AnotherWindow;
}

class AnotherWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AnotherWindow( QWidget *parent = nullptr );
    ~AnotherWindow( );

signals:
    void firstWindow( );
    void runThread( bool runAH );
    void saveLogData( );

public slots:
    void set_server( QString message );
    void set_login( QString message );
    void set_password( QString message );
    void set_run_ah( bool runAH );
    void set_remember_me( bool rememberMe );
    void logon_successful( );
    void logon_failed( QString message );

private slots:
    void on_logon_clicked( );
    void on_server_returnPressed( );
    void on_login_returnPressed( );
    void on_password_returnPressed( );
    void on_help_clicked( );
    void on_GoToConfigFile_clicked( );
    void on_settings_clicked( );

private:
    Ui::AnotherWindow *ui;
    void closeEvent( QCloseEvent *event );
};

namespace Ui {
class PresettingWindow;
}

class PresettingWindow : public QDialog
{
    Q_OBJECT

public:
    explicit PresettingWindow( QWidget *parent = nullptr );
    void war3PathLineAutoSet( QString directory );
    ~PresettingWindow( );

private slots:
    void on_war3PathButton_clicked( );
    void on_mapsPathButton_clicked( );
    void on_mapsCFGPathButton_clicked( );
    void on_savegamesPathButton_clicked( );
    void on_replaysPathButton_clicked();
    void on_war3PathLine_returnPressed( );
    void on_skipButton_clicked( );
    void on_aplyButton_clicked( );
    void on_war3PathLine_textEdited( const QString &arg1 );
    void on_mapsPathLine_textEdited( const QString &arg1 );
    void on_mapsCFGPathLine_textEdited( const QString &arg1 );
    void on_savegamesPathLine_textEdited( const QString &arg1 );
    void on_replaysPathLine_textEdited( const QString &arg1 );
    void on_languageBox_currentIndexChanged( int index );
    void on_hostPortSpin_valueChanged( int arg1 );

private:
    Ui::PresettingWindow *ui;
    bool languageChanged;
    bool war3PathChanged;
    bool mapsPathChanged;
    bool mapsCFGPathChanged;
    bool savePathChanged;
    bool replaysPathChanged;
    bool hostPortChanged;
};

namespace Ui {
class HelpLogonWindow;
}

class HelpLogonWindow : public QDialog
{
    Q_OBJECT

public:
    explicit HelpLogonWindow( QWidget *parent = nullptr );
    ~HelpLogonWindow( );

private slots:
    void on_iCCupAHinfo_clicked( );
    void on_closeButton_clicked( );

private:
    Ui::HelpLogonWindow *ui;
    bool m_iCCupAHinfo;
};

namespace Ui {
class LogSettingsWindow;
}

class LogSettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LogSettingsWindow( QWidget *parent = nullptr );
    ~LogSettingsWindow( );

private:
    Ui::LogSettingsWindow *ui;
};

namespace Ui {
class StyleSettingWindow;
}

class StyleSettingWindow : public QDialog
{
    Q_OBJECT

public:
    explicit StyleSettingWindow( QWidget *parent = nullptr );
    ~StyleSettingWindow( );

private slots:
    void on_colorNormalMsgReset_clicked( );
    void on_colorPrivateMsgReset_clicked( );
    void on_colorNotifyMsgReset_clicked( );
    void on_colorWarningMsgReset_clicked( );
    void on_colorPreSentMsgReset_clicked( );
    void on_defaultScrollBarReset_clicked( );
    void on_cancelButton_clicked( );
    void on_applyButton_clicked( );

private:
    Ui::StyleSettingWindow *ui;
};

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY( QPoint previousPosition READ previousPosition WRITE setPreviousPosition NOTIFY previousPositionChanged )

    enum MouseType{
        None = 0,
        Top,
        Bottom,
        Left,
        Right,
        Move
    };

public:
    explicit Widget( QWidget *parent = nullptr );
    ~Widget();
    uint32_t GetTabIndexByGameNumber( uint32_t gameID, bool &tabfound );
    void ChangeCurrentMainTab( int index );
    void ChangeCurrentGameTab( int index );
    QPoint previousPosition( ) const;
    QStringList m_StringList;

public slots:
    void write_to_log( QString message );
    void write_to_game_log( unsigned int gameID, QString message, bool toMainLog );
    void write_to_chat( QString message, QString msg, int bnetID, int mainBnetID );
    void add_channel_user( QString message, int bnetID );
    void remove_channel_user( QString message, int bnetID );
    void clear_channel_user( int bnetID );
    void bnet_disconnect( int bnetID );
    void bnet_reconnect( int bnetID );
    void set_commands_tips( bool first );
    void save_log_data( );
    void ThirdThreadFinished( );
    void UpdateValuesInGUI( QList<QString> cfgvars );
    void setPreviousPosition( QPoint previousPosition );
    void download_map( QList<QString> options );
    void backup_log( QString logfilename );
    void restart_UNM( bool now );
    void add_chat_tab( unsigned int bnetID, QString bnetCommandTrigger, QString serverName, QString userName, unsigned int timeWaitingToReconnect );
    void change_bnet_username( unsigned int bnetID, QString userName );
    void add_game( unsigned int gameID, QStringList gameInfoInit, QList<QStringList> playersInfo );
    void delete_game( unsigned int gameID );
    void update_game( unsigned int gameID, QStringList gameInfo, QList<QStringList> playersInfo );
    void update_game_map_info( unsigned int gameID, QString mapPath );
    void write_to_game_chat( QString message, QString extramessage, QString user, unsigned int type, unsigned int gameID );
    void add_bnet_bot( unsigned int bnetID, QString botName );
    void update_bnet_bot( unsigned int bnetID, QString botName, unsigned int type );
    void delete_bnet_bot( unsigned int bnetID, QString botName );
    void add_game_to_list( QList<unsigned int> bnetIDs, unsigned int gameID, bool gproxy, unsigned int gameIcon, QStringList gameInfo );
    void update_game_to_list( QList<unsigned int> bnetIDs, unsigned int gameID, QStringList gameInfo, bool deleteFromCurrentGames );
    void delete_game_from_list( QList<unsigned int> bnetIDs, unsigned int gameID, unsigned int type );
    void delete_old_messages( );
    void game_list_table_pos_fix( QList<unsigned int> bnetIDs );

private slots:
    void show_main_window( );
    void finished_unm( bool mainThread );
    void thread_ready( bool mainThread );
    void run_thread( bool runAH );
    void on_analyze_clicked( );
    void on_clearfield_clicked( );
    void Spell01ClickSlot( bool rightButton );
    void Spell02ClickSlot( bool rightButton );
    void Spell03ClickSlot( bool rightButton );
    void Spell04ClickSlot( bool rightButton );
    void Spell05ClickSlot( bool rightButton );
    void Spell06ClickSlot( bool rightButton );
    void Spell07ClickSlot( bool rightButton );
    void Spell08ClickSlot( bool rightButton );
    void Spell09ClickSlot( bool rightButton );
    void Spell10ClickSlot( bool rightButton );
    void Spell11ClickSlot( bool rightButton );
    void Spell12ClickSlot( bool rightButton );
    void SpellBook01ClickSlot( bool rightButton );
    void SpellBook02ClickSlot( bool rightButton );
    void SpellBook03ClickSlot( bool rightButton );
    void SpellBook04ClickSlot( bool rightButton );
    void SpellBook05ClickSlot( bool rightButton );
    void SpellBook06ClickSlot( bool rightButton );
    void SpellBook07ClickSlot( bool rightButton );
    void SpellBook08ClickSlot( bool rightButton );
    void SpellBook09ClickSlot( bool rightButton );
    void SpellBook10ClickSlot( bool rightButton );
    void SpellBook11ClickSlot( bool rightButton );
    void SpellBook12ClickSlot( bool rightButton );
    void Shop01ClickSlot( bool rightButton );
    void Shop02ClickSlot( bool rightButton );
    void Shop03ClickSlot( bool rightButton );
    void Shop04ClickSlot( bool rightButton );
    void Shop05ClickSlot( bool rightButton );
    void Shop06ClickSlot( bool rightButton );
    void Shop07ClickSlot( bool rightButton );
    void Shop08ClickSlot( bool rightButton );
    void Shop09ClickSlot( bool rightButton );
    void Shop10ClickSlot( bool rightButton );
    void Shop11ClickSlot( bool rightButton );
    void Shop12ClickSlot( bool rightButton );
    void Item1ClickSlot( bool rightButton );
    void Item2ClickSlot( bool rightButton );
    void Item3ClickSlot( bool rightButton );
    void Item4ClickSlot( bool rightButton );
    void Item5ClickSlot( bool rightButton );
    void Item6ClickSlot( bool rightButton );
    void on_ShowPesettingWindowApply_clicked( );
    void on_ShowPesettingWindowSave_clicked( );
    void on_ShowPesettingWindowRefresh_clicked( );
    void on_ShowPesettingWindowReset_clicked( );
    void on_EnableLoggingToGUIApply_clicked( );
    void on_EnableLoggingToGUISave_clicked( );
    void on_EnableLoggingToGUIRefresh_clicked( );
    void on_EnableLoggingToGUIReset_clicked( );
    void on_LogReductionApply_clicked( );
    void on_LogReductionSave_clicked( );
    void on_LogReductionRefresh_clicked( );
    void on_LogReductionReset_clicked( );
    void on_MaxLogSizeApply_clicked( );
    void on_MaxLogSizeSave_clicked( );
    void on_MaxLogSizeRefresh_clicked( );
    void on_MaxLogSizeReset_clicked( );
    void on_MaxLogTimeApply_clicked( );
    void on_MaxLogTimeSave_clicked( );
    void on_MaxLogTimeRefresh_clicked( );
    void on_MaxLogTimeReset_clicked( );
    void on_MinLengthGNApply_clicked( );
    void on_MinLengthGNSave_clicked( );
    void on_MinLengthGNRefresh_clicked( );
    void on_MinLengthGNReset_clicked( );
    void on_SlotInfoInGameNameApply_clicked( );
    void on_SlotInfoInGameNameSave_clicked( );
    void on_SlotInfoInGameNameRefresh_clicked( );
    void on_SlotInfoInGameNameReset_clicked( );
    void on_patch23Apply_clicked( );
    void on_patch23Save_clicked( );
    void on_patch23Refresh_clicked( );
    void on_patch23Reset_clicked( );
    void on_patch21Apply_clicked( );
    void on_patch21Save_clicked( );
    void on_patch21Refresh_clicked( );
    void on_patch21Reset_clicked( );
    void on_broadcastinlanApply_clicked( );
    void on_broadcastinlanSave_clicked( );
    void on_broadcastinlanRefresh_clicked( );
    void on_broadcastinlanReset_clicked( );
    void on_LANWar3VersionApply_clicked( );
    void on_LANWar3VersionSave_clicked( );
    void on_LANWar3VersionRefresh_clicked( );
    void on_LANWar3VersionReset_clicked( );
    void on_ShowRealSlotCountApply_clicked( );
    void on_ShowRealSlotCountSave_clicked( );
    void on_ShowRealSlotCountRefresh_clicked( );
    void on_ShowRealSlotCountReset_clicked( );
    void on_BroadcastHostNameApply_clicked( );
    void on_BroadcastHostNameSave_clicked( );
    void on_BroadcastHostNameRefresh_clicked( );
    void on_BroadcastHostNameReset_clicked( );
    void on_BroadcastHostNameCustomLine_returnPressed( );
    void on_BroadcastHostNameCustomApply_clicked( );
    void on_BroadcastHostNameCustomSave_clicked( );
    void on_BroadcastHostNameCustomRefresh_clicked( );
    void on_BroadcastHostNameCustomReset_clicked( );
    void on_BroadcastPortApply_clicked( );
    void on_BroadcastPortSave_clicked( );
    void on_BroadcastPortRefresh_clicked( );
    void on_BroadcastPortReset_clicked( );
    void on_HostPortApply_clicked( );
    void on_HostPortSave_clicked( );
    void on_HostPortRefresh_clicked( );
    void on_HostPortReset_clicked( );
    void on_DefaultMapLine_returnPressed( );
    void on_DefaultMapApply_clicked( );
    void on_DefaultMapSave_clicked( );
    void on_DefaultMapRefresh_clicked( );
    void on_DefaultMapReset_clicked( );
    void on_BindAddressLine_returnPressed( );
    void on_BindAddressApply_clicked( );
    void on_BindAddressSave_clicked( );
    void on_BindAddressRefresh_clicked( );
    void on_BindAddressReset_clicked( );
    void on_HideIPAddressesApply_clicked( );
    void on_HideIPAddressesSave_clicked( );
    void on_HideIPAddressesRefresh_clicked( );
    void on_HideIPAddressesReset_clicked( );
    void on_LanguageFileLine_returnPressed( );
    void on_LanguageFileApply_clicked( );
    void on_LanguageFileSave_clicked( );
    void on_LanguageFileRefresh_clicked( );
    void on_LanguageFileReset_clicked( );
    void on_Warcraft3PathLine_returnPressed( );
    void on_Warcraft3PathApply_clicked( );
    void on_Warcraft3PathSave_clicked( );
    void on_Warcraft3PathRefresh_clicked( );
    void on_Warcraft3PathReset_clicked( );
    void on_MapCFGPathLine_returnPressed( );
    void on_MapCFGPathApply_clicked( );
    void on_MapCFGPathSave_clicked( );
    void on_MapCFGPathRefresh_clicked( );
    void on_MapCFGPathReset_clicked( );
    void on_SaveGamePathLine_returnPressed( );
    void on_SaveGamePathApply_clicked( );
    void on_SaveGamePathSave_clicked( );
    void on_SaveGamePathRefresh_clicked( );
    void on_SaveGamePathReset_clicked( );
    void on_MapPathLine_returnPressed( );
    void on_MapPathApply_clicked( );
    void on_MapPathSave_clicked( );
    void on_MapPathRefresh_clicked( );
    void on_MapPathReset_clicked( );
    void on_ReplayPathLine_returnPressed( );
    void on_ReplayPathApply_clicked( );
    void on_ReplayPathSave_clicked( );
    void on_ReplayPathRefresh_clicked( );
    void on_ReplayPathReset_clicked( );
    void on_SaveReplaysApply_clicked( );
    void on_SaveReplaysSave_clicked( );
    void on_SaveReplaysRefresh_clicked( );
    void on_SaveReplaysReset_clicked( );
    void on_ReplayBuildNumberApply_clicked( );
    void on_ReplayBuildNumberSave_clicked( );
    void on_ReplayBuildNumberRefresh_clicked( );
    void on_ReplayBuildNumberReset_clicked( );
    void on_ReplayWar3VersionApply_clicked( );
    void on_ReplayWar3VersionSave_clicked( );
    void on_ReplayWar3VersionRefresh_clicked( );
    void on_ReplayWar3VersionReset_clicked( );
    void on_ReplaysByNameApply_clicked( );
    void on_ReplaysByNameSave_clicked( );
    void on_ReplaysByNameRefresh_clicked( );
    void on_ReplaysByNameReset_clicked( );
    void on_InvalidTriggersGameLine_returnPressed( );
    void on_InvalidTriggersGameApply_clicked( );
    void on_InvalidTriggersGameSave_clicked( );
    void on_InvalidTriggersGameRefresh_clicked( );
    void on_InvalidTriggersGameReset_clicked( );
    void on_InvalidReplayCharsLine_returnPressed( );
    void on_InvalidReplayCharsApply_clicked( );
    void on_InvalidReplayCharsSave_clicked( );
    void on_InvalidReplayCharsRefresh_clicked( );
    void on_InvalidReplayCharsReset_clicked( );
    void on_MaxHostCounterApply_clicked( );
    void on_MaxHostCounterSave_clicked( );
    void on_MaxHostCounterRefresh_clicked( );
    void on_MaxHostCounterReset_clicked( );
    void on_RehostCharLine_returnPressed( );
    void on_RehostCharApply_clicked( );
    void on_RehostCharSave_clicked( );
    void on_RehostCharRefresh_clicked( );
    void on_RehostCharReset_clicked( );
    void on_GameNameCharHideApply_clicked( );
    void on_GameNameCharHideSave_clicked( );
    void on_GameNameCharHideRefresh_clicked( );
    void on_GameNameCharHideReset_clicked( );
    void on_DefaultGameNameCharLine_returnPressed( );
    void on_DefaultGameNameCharApply_clicked( );
    void on_DefaultGameNameCharSave_clicked( );
    void on_DefaultGameNameCharRefresh_clicked( );
    void on_DefaultGameNameCharReset_clicked( );
    void on_ResetGameNameCharApply_clicked( );
    void on_ResetGameNameCharSave_clicked( );
    void on_ResetGameNameCharRefresh_clicked( );
    void on_ResetGameNameCharReset_clicked( );
    void on_AppleIconApply_clicked( );
    void on_AppleIconSave_clicked( );
    void on_AppleIconRefresh_clicked( );
    void on_AppleIconReset_clicked( );
    void on_FixGameNameForIccupApply_clicked( );
    void on_FixGameNameForIccupSave_clicked( );
    void on_FixGameNameForIccupRefresh_clicked( );
    void on_FixGameNameForIccupReset_clicked( );
    void on_AllowAutoRenameMapsApply_clicked( );
    void on_AllowAutoRenameMapsSave_clicked( );
    void on_AllowAutoRenameMapsRefresh_clicked( );
    void on_AllowAutoRenameMapsReset_clicked( );
    void on_OverrideMapDataFromCFGApply_clicked( );
    void on_OverrideMapDataFromCFGSave_clicked( );
    void on_OverrideMapDataFromCFGRefresh_clicked( );
    void on_OverrideMapDataFromCFGReset_clicked( );
    void on_OverrideMapPathFromCFGApply_clicked( );
    void on_OverrideMapPathFromCFGSave_clicked( );
    void on_OverrideMapPathFromCFGRefresh_clicked( );
    void on_OverrideMapPathFromCFGReset_clicked( );
    void on_OverrideMapSizeFromCFGApply_clicked( );
    void on_OverrideMapSizeFromCFGSave_clicked( );
    void on_OverrideMapSizeFromCFGRefresh_clicked( );
    void on_OverrideMapSizeFromCFGReset_clicked( );
    void on_OverrideMapInfoFromCFGApply_clicked( );
    void on_OverrideMapInfoFromCFGSave_clicked( );
    void on_OverrideMapInfoFromCFGRefresh_clicked( );
    void on_OverrideMapInfoFromCFGReset_clicked( );
    void on_OverrideMapCrcFromCFGApply_clicked( );
    void on_OverrideMapCrcFromCFGSave_clicked( );
    void on_OverrideMapCrcFromCFGRefresh_clicked( );
    void on_OverrideMapCrcFromCFGReset_clicked( );
    void on_OverrideMapSha1FromCFGApply_clicked( );
    void on_OverrideMapSha1FromCFGSave_clicked( );
    void on_OverrideMapSha1FromCFGRefresh_clicked( );
    void on_OverrideMapSha1FromCFGReset_clicked( );
    void on_OverrideMapSpeedFromCFGApply_clicked( );
    void on_OverrideMapSpeedFromCFGSave_clicked( );
    void on_OverrideMapSpeedFromCFGRefresh_clicked( );
    void on_OverrideMapSpeedFromCFGReset_clicked( );
    void on_OverrideMapVisibilityFromCFGApply_clicked( );
    void on_OverrideMapVisibilityFromCFGSave_clicked( );
    void on_OverrideMapVisibilityFromCFGRefresh_clicked( );
    void on_OverrideMapVisibilityFromCFGReset_clicked( );
    void on_OverrideMapFlagsFromCFGApply_clicked( );
    void on_OverrideMapFlagsFromCFGSave_clicked( );
    void on_OverrideMapFlagsFromCFGRefresh_clicked( );
    void on_OverrideMapFlagsFromCFGReset_clicked( );
    void on_OverrideMapFilterTypeFromCFGApply_clicked( );
    void on_OverrideMapFilterTypeFromCFGSave_clicked( );
    void on_OverrideMapFilterTypeFromCFGRefresh_clicked( );
    void on_OverrideMapFilterTypeFromCFGReset_clicked( );
    void on_OverrideMapGameTypeFromCFGApply_clicked( );
    void on_OverrideMapGameTypeFromCFGSave_clicked( );
    void on_OverrideMapGameTypeFromCFGRefresh_clicked( );
    void on_OverrideMapGameTypeFromCFGReset_clicked( );
    void on_OverrideMapWidthFromCFGApply_clicked( );
    void on_OverrideMapWidthFromCFGSave_clicked( );
    void on_OverrideMapWidthFromCFGRefresh_clicked( );
    void on_OverrideMapWidthFromCFGReset_clicked( );
    void on_OverrideMapHeightFromCFGApply_clicked( );
    void on_OverrideMapHeightFromCFGSave_clicked( );
    void on_OverrideMapHeightFromCFGRefresh_clicked( );
    void on_OverrideMapHeightFromCFGReset_clicked( );
    void on_OverrideMapTypeFromCFGApply_clicked( );
    void on_OverrideMapTypeFromCFGSave_clicked( );
    void on_OverrideMapTypeFromCFGRefresh_clicked( );
    void on_OverrideMapTypeFromCFGReset_clicked( );
    void on_OverrideMapDefaultHCLFromCFGApply_clicked( );
    void on_OverrideMapDefaultHCLFromCFGSave_clicked( );
    void on_OverrideMapDefaultHCLFromCFGRefresh_clicked( );
    void on_OverrideMapDefaultHCLFromCFGReset_clicked( );
    void on_OverrideMapLoadInGameFromCFGApply_clicked( );
    void on_OverrideMapLoadInGameFromCFGSave_clicked( );
    void on_OverrideMapLoadInGameFromCFGRefresh_clicked( );
    void on_OverrideMapLoadInGameFromCFGReset_clicked( );
    void on_OverrideMapNumPlayersFromCFGApply_clicked( );
    void on_OverrideMapNumPlayersFromCFGSave_clicked( );
    void on_OverrideMapNumPlayersFromCFGRefresh_clicked( );
    void on_OverrideMapNumPlayersFromCFGReset_clicked( );
    void on_OverrideMapNumTeamsFromCFGApply_clicked( );
    void on_OverrideMapNumTeamsFromCFGSave_clicked( );
    void on_OverrideMapNumTeamsFromCFGRefresh_clicked( );
    void on_OverrideMapNumTeamsFromCFGReset_clicked( );
    void on_OverrideMapSlotsFromCFGApply_clicked( );
    void on_OverrideMapSlotsFromCFGSave_clicked( );
    void on_OverrideMapSlotsFromCFGRefresh_clicked( );
    void on_OverrideMapSlotsFromCFGReset_clicked( );
    void on_OverrideMapObserversFromCFGApply_clicked( );
    void on_OverrideMapObserversFromCFGSave_clicked( );
    void on_OverrideMapObserversFromCFGRefresh_clicked( );
    void on_OverrideMapObserversFromCFGReset_clicked( );
    void on_PlayersCanChangeRaceApply_clicked( );
    void on_PlayersCanChangeRaceSave_clicked( );
    void on_PlayersCanChangeRaceRefresh_clicked( );
    void on_PlayersCanChangeRaceReset_clicked( );
    void on_HoldPlayersForRMKApply_clicked( );
    void on_HoldPlayersForRMKSave_clicked( );
    void on_HoldPlayersForRMKRefresh_clicked( );
    void on_HoldPlayersForRMKReset_clicked( );
    void on_ForceLoadInGameApply_clicked( );
    void on_ForceLoadInGameSave_clicked( );
    void on_ForceLoadInGameRefresh_clicked( );
    void on_ForceLoadInGameReset_clicked( );
    void on_MsgAutoCorrectModeApply_clicked( );
    void on_MsgAutoCorrectModeSave_clicked( );
    void on_MsgAutoCorrectModeRefresh_clicked( );
    void on_MsgAutoCorrectModeReset_clicked( );
    void on_SpoofChecksApply_clicked( );
    void on_SpoofChecksSave_clicked( );
    void on_SpoofChecksRefresh_clicked( );
    void on_SpoofChecksReset_clicked( );
    void on_CheckMultipleIPUsageApply_clicked( );
    void on_CheckMultipleIPUsageSave_clicked( );
    void on_CheckMultipleIPUsageRefresh_clicked( );
    void on_CheckMultipleIPUsageReset_clicked( );
    void on_SpoofCheckIfGameNameIsIndefiniteApply_clicked( );
    void on_SpoofCheckIfGameNameIsIndefiniteSave_clicked( );
    void on_SpoofCheckIfGameNameIsIndefiniteRefresh_clicked( );
    void on_SpoofCheckIfGameNameIsIndefiniteReset_clicked( );
    void on_MaxLatencyApply_clicked( );
    void on_MaxLatencySave_clicked( );
    void on_MaxLatencyRefresh_clicked( );
    void on_MaxLatencyReset_clicked( );
    void on_MinLatencyApply_clicked( );
    void on_MinLatencySave_clicked( );
    void on_MinLatencyRefresh_clicked( );
    void on_MinLatencyReset_clicked( );
    void on_LatencyApply_clicked( );
    void on_LatencySave_clicked( );
    void on_LatencyRefresh_clicked( );
    void on_LatencyReset_clicked( );
    void on_UseDynamicLatencyApply_clicked( );
    void on_UseDynamicLatencySave_clicked( );
    void on_UseDynamicLatencyRefresh_clicked( );
    void on_UseDynamicLatencyReset_clicked( );
    void on_DynamicLatencyHighestAllowedApply_clicked( );
    void on_DynamicLatencyHighestAllowedSave_clicked( );
    void on_DynamicLatencyHighestAllowedRefresh_clicked( );
    void on_DynamicLatencyHighestAllowedReset_clicked( );
    void on_DynamicLatencyLowestAllowedApply_clicked( );
    void on_DynamicLatencyLowestAllowedSave_clicked( );
    void on_DynamicLatencyLowestAllowedRefresh_clicked( );
    void on_DynamicLatencyLowestAllowedReset_clicked( );
    void on_DynamicLatencyRefreshRateApply_clicked( );
    void on_DynamicLatencyRefreshRateSave_clicked( );
    void on_DynamicLatencyRefreshRateRefresh_clicked( );
    void on_DynamicLatencyRefreshRateReset_clicked( );
    void on_DynamicLatencyConsolePrintApply_clicked( );
    void on_DynamicLatencyConsolePrintSave_clicked( );
    void on_DynamicLatencyConsolePrintRefresh_clicked( );
    void on_DynamicLatencyConsolePrintReset_clicked( );
    void on_DynamicLatencyPercentPingMaxApply_clicked( );
    void on_DynamicLatencyPercentPingMaxSave_clicked( );
    void on_DynamicLatencyPercentPingMaxRefresh_clicked( );
    void on_DynamicLatencyPercentPingMaxReset_clicked( );
    void on_DynamicLatencyDifferencePingMaxApply_clicked( );
    void on_DynamicLatencyDifferencePingMaxSave_clicked( );
    void on_DynamicLatencyDifferencePingMaxRefresh_clicked( );
    void on_DynamicLatencyDifferencePingMaxReset_clicked( );
    void on_DynamicLatencyMaxSyncApply_clicked( );
    void on_DynamicLatencyMaxSyncSave_clicked( );
    void on_DynamicLatencyMaxSyncRefresh_clicked( );
    void on_DynamicLatencyMaxSyncReset_clicked( );
    void on_DynamicLatencyAddIfMaxSyncApply_clicked( );
    void on_DynamicLatencyAddIfMaxSyncSave_clicked( );
    void on_DynamicLatencyAddIfMaxSyncRefresh_clicked( );
    void on_DynamicLatencyAddIfMaxSyncReset_clicked( );
    void on_DynamicLatencyAddIfLagApply_clicked( );
    void on_DynamicLatencyAddIfLagSave_clicked( );
    void on_DynamicLatencyAddIfLagRefresh_clicked( );
    void on_DynamicLatencyAddIfLagReset_clicked( );
    void on_DynamicLatencyAddIfBotLagApply_clicked( );
    void on_DynamicLatencyAddIfBotLagSave_clicked( );
    void on_DynamicLatencyAddIfBotLagRefresh_clicked( );
    void on_DynamicLatencyAddIfBotLagReset_clicked( );
    void on_MaxSyncLimitApply_clicked( );
    void on_MaxSyncLimitSave_clicked( );
    void on_MaxSyncLimitRefresh_clicked( );
    void on_MaxSyncLimitReset_clicked( );
    void on_MinSyncLimitApply_clicked( );
    void on_MinSyncLimitSave_clicked( );
    void on_MinSyncLimitRefresh_clicked( );
    void on_MinSyncLimitReset_clicked( );
    void on_SyncLimitApply_clicked( );
    void on_SyncLimitSave_clicked( );
    void on_SyncLimitRefresh_clicked( );
    void on_SyncLimitReset_clicked( );
    void on_AutoSaveApply_clicked( );
    void on_AutoSaveSave_clicked( );
    void on_AutoSaveRefresh_clicked( );
    void on_AutoSaveReset_clicked( );
    void on_AutoKickPingApply_clicked( );
    void on_AutoKickPingSave_clicked( );
    void on_AutoKickPingRefresh_clicked( );
    void on_AutoKickPingReset_clicked( );
    void on_LCPingsApply_clicked( );
    void on_LCPingsSave_clicked( );
    void on_LCPingsRefresh_clicked( );
    void on_LCPingsReset_clicked( );
    void on_PingDuringDownloadsApply_clicked( );
    void on_PingDuringDownloadsSave_clicked( );
    void on_PingDuringDownloadsRefresh_clicked( );
    void on_PingDuringDownloadsReset_clicked( );
    void on_AllowDownloadsApply_clicked( );
    void on_AllowDownloadsSave_clicked( );
    void on_AllowDownloadsRefresh_clicked( );
    void on_AllowDownloadsReset_clicked( );
    void on_maxdownloadersApply_clicked( );
    void on_maxdownloadersSave_clicked( );
    void on_maxdownloadersRefresh_clicked( );
    void on_maxdownloadersReset_clicked( );
    void on_totaldownloadspeedApply_clicked( );
    void on_totaldownloadspeedSave_clicked( );
    void on_totaldownloadspeedRefresh_clicked( );
    void on_totaldownloadspeedReset_clicked( );
    void on_clientdownloadspeedApply_clicked( );
    void on_clientdownloadspeedSave_clicked( );
    void on_clientdownloadspeedRefresh_clicked( );
    void on_clientdownloadspeedReset_clicked( );
    void on_PlaceAdminsHigherApply_clicked( );
    void on_PlaceAdminsHigherSave_clicked( );
    void on_PlaceAdminsHigherRefresh_clicked( );
    void on_PlaceAdminsHigherReset_clicked( );
    void on_ShuffleSlotsOnStartApply_clicked( );
    void on_ShuffleSlotsOnStartSave_clicked( );
    void on_ShuffleSlotsOnStartRefresh_clicked( );
    void on_ShuffleSlotsOnStartReset_clicked( );
    void on_FakePlayersLobbyApply_clicked( );
    void on_FakePlayersLobbySave_clicked( );
    void on_FakePlayersLobbyRefresh_clicked( );
    void on_FakePlayersLobbyReset_clicked( );
    void on_MoreFPsLobbyApply_clicked( );
    void on_MoreFPsLobbySave_clicked( );
    void on_MoreFPsLobbyRefresh_clicked( );
    void on_MoreFPsLobbyReset_clicked( );
    void on_NormalCountdownApply_clicked( );
    void on_NormalCountdownSave_clicked( );
    void on_NormalCountdownRefresh_clicked( );
    void on_NormalCountdownReset_clicked( );
    void on_CountDownMethodApply_clicked( );
    void on_CountDownMethodSave_clicked( );
    void on_CountDownMethodRefresh_clicked( );
    void on_CountDownMethodReset_clicked( );
    void on_CountDownStartCounterApply_clicked( );
    void on_CountDownStartCounterSave_clicked( );
    void on_CountDownStartCounterRefresh_clicked( );
    void on_CountDownStartCounterReset_clicked( );
    void on_CountDownStartForceCounterApply_clicked( );
    void on_CountDownStartForceCounterSave_clicked( );
    void on_CountDownStartForceCounterRefresh_clicked( );
    void on_CountDownStartForceCounterReset_clicked( );
    void on_VoteStartAllowedApply_clicked( );
    void on_VoteStartAllowedSave_clicked( );
    void on_VoteStartAllowedRefresh_clicked( );
    void on_VoteStartAllowedReset_clicked( );
    void on_VoteStartPercentalVotingApply_clicked( );
    void on_VoteStartPercentalVotingSave_clicked( );
    void on_VoteStartPercentalVotingRefresh_clicked( );
    void on_VoteStartPercentalVotingReset_clicked( );
    void on_CancelVoteStartIfLeaveApply_clicked( );
    void on_CancelVoteStartIfLeaveSave_clicked( );
    void on_CancelVoteStartIfLeaveRefresh_clicked( );
    void on_CancelVoteStartIfLeaveReset_clicked( );
    void on_StartAfterLeaveIfEnoughVotesApply_clicked( );
    void on_StartAfterLeaveIfEnoughVotesSave_clicked( );
    void on_StartAfterLeaveIfEnoughVotesRefresh_clicked( );
    void on_StartAfterLeaveIfEnoughVotesReset_clicked( );
    void on_VoteStartMinPlayersApply_clicked( );
    void on_VoteStartMinPlayersSave_clicked( );
    void on_VoteStartMinPlayersRefresh_clicked( );
    void on_VoteStartMinPlayersReset_clicked( );
    void on_VoteStartPercentApply_clicked( );
    void on_VoteStartPercentSave_clicked( );
    void on_VoteStartPercentRefresh_clicked( );
    void on_VoteStartPercentReset_clicked( );
    void on_VoteStartDurationApply_clicked( );
    void on_VoteStartDurationSave_clicked( );
    void on_VoteStartDurationRefresh_clicked( );
    void on_VoteStartDurationReset_clicked( );
    void on_VoteKickAllowedApply_clicked( );
    void on_VoteKickAllowedSave_clicked( );
    void on_VoteKickAllowedRefresh_clicked( );
    void on_VoteKickAllowedReset_clicked( );
    void on_VoteKickPercentageApply_clicked( );
    void on_VoteKickPercentageSave_clicked( );
    void on_VoteKickPercentageRefresh_clicked( );
    void on_VoteKickPercentageReset_clicked( );
    void on_OptionOwnerApply_clicked( );
    void on_OptionOwnerSave_clicked( );
    void on_OptionOwnerRefresh_clicked( );
    void on_OptionOwnerReset_clicked( );
    void on_AFKkickApply_clicked( );
    void on_AFKkickSave_clicked( );
    void on_AFKkickRefresh_clicked( );
    void on_AFKkickReset_clicked( );
    void on_AFKtimerApply_clicked( );
    void on_AFKtimerSave_clicked( );
    void on_AFKtimerRefresh_clicked( );
    void on_AFKtimerReset_clicked( );
    void on_ReconnectWaitTimeApply_clicked( );
    void on_ReconnectWaitTimeSave_clicked( );
    void on_ReconnectWaitTimeRefresh_clicked( );
    void on_ReconnectWaitTimeReset_clicked( );
    void on_DropVotePercentApply_clicked( );
    void on_DropVotePercentSave_clicked( );
    void on_DropVotePercentRefresh_clicked( );
    void on_DropVotePercentReset_clicked( );
    void on_DropVoteTimeApply_clicked( );
    void on_DropVoteTimeSave_clicked( );
    void on_DropVoteTimeRefresh_clicked( );
    void on_DropVoteTimeReset_clicked( );
    void on_DropWaitTimeApply_clicked( );
    void on_DropWaitTimeSave_clicked( );
    void on_DropWaitTimeRefresh_clicked( );
    void on_DropWaitTimeReset_clicked( );
    void on_DropWaitTimeSumApply_clicked( );
    void on_DropWaitTimeSumSave_clicked( );
    void on_DropWaitTimeSumRefresh_clicked( );
    void on_DropWaitTimeSumReset_clicked( );
    void on_DropWaitTimeGameApply_clicked( );
    void on_DropWaitTimeGameSave_clicked( );
    void on_DropWaitTimeGameRefresh_clicked( );
    void on_DropWaitTimeGameReset_clicked( );
    void on_DropWaitTimeLoadApply_clicked( );
    void on_DropWaitTimeLoadSave_clicked( );
    void on_DropWaitTimeLoadRefresh_clicked( );
    void on_DropWaitTimeLoadReset_clicked( );
    void on_DropWaitTimeLobbyApply_clicked( );
    void on_DropWaitTimeLobbySave_clicked( );
    void on_DropWaitTimeLobbyRefresh_clicked( );
    void on_DropWaitTimeLobbyReset_clicked( );
    void on_AllowLaggingTimeApply_clicked( );
    void on_AllowLaggingTimeSave_clicked( );
    void on_AllowLaggingTimeRefresh_clicked( );
    void on_AllowLaggingTimeReset_clicked( );
    void on_AllowLaggingTimeIntervalApply_clicked( );
    void on_AllowLaggingTimeIntervalSave_clicked( );
    void on_AllowLaggingTimeIntervalRefresh_clicked( );
    void on_AllowLaggingTimeIntervalReset_clicked( );
    void on_UserCanDropAdminApply_clicked( );
    void on_UserCanDropAdminSave_clicked( );
    void on_UserCanDropAdminRefresh_clicked( );
    void on_UserCanDropAdminReset_clicked( );
    void on_DropIfDesyncApply_clicked( );
    void on_DropIfDesyncSave_clicked( );
    void on_DropIfDesyncRefresh_clicked( );
    void on_DropIfDesyncReset_clicked( );
    void on_gameoverminpercentApply_clicked( );
    void on_gameoverminpercentSave_clicked( );
    void on_gameoverminpercentRefresh_clicked( );
    void on_gameoverminpercentReset_clicked( );
    void on_gameoverminplayersApply_clicked( );
    void on_gameoverminplayersSave_clicked( );
    void on_gameoverminplayersRefresh_clicked( );
    void on_gameoverminplayersReset_clicked( );
    void on_gameovermaxteamdifferenceApply_clicked( );
    void on_gameovermaxteamdifferenceSave_clicked( );
    void on_gameovermaxteamdifferenceRefresh_clicked( );
    void on_gameovermaxteamdifferenceReset_clicked( );
    void on_LobbyTimeLimitApply_clicked( );
    void on_LobbyTimeLimitSave_clicked( );
    void on_LobbyTimeLimitRefresh_clicked( );
    void on_LobbyTimeLimitReset_clicked( );
    void on_LobbyTimeLimitMaxApply_clicked( );
    void on_LobbyTimeLimitMaxSave_clicked( );
    void on_LobbyTimeLimitMaxRefresh_clicked( );
    void on_LobbyTimeLimitMaxReset_clicked( );
    void on_PubGameDelayApply_clicked( );
    void on_PubGameDelaySave_clicked( );
    void on_PubGameDelayRefresh_clicked( );
    void on_PubGameDelayReset_clicked( );
    void on_BotCommandTriggerLine_returnPressed( );
    void on_BotCommandTriggerApply_clicked( );
    void on_BotCommandTriggerSave_clicked( );
    void on_BotCommandTriggerRefresh_clicked( );
    void on_BotCommandTriggerReset_clicked( );
    void on_VirtualHostNameLine_returnPressed( );
    void on_VirtualHostNameApply_clicked( );
    void on_VirtualHostNameSave_clicked( );
    void on_VirtualHostNameRefresh_clicked( );
    void on_VirtualHostNameReset_clicked( );
    void on_ObsPlayerNameLine_returnPressed( );
    void on_ObsPlayerNameApply_clicked( );
    void on_ObsPlayerNameSave_clicked( );
    void on_ObsPlayerNameRefresh_clicked( );
    void on_ObsPlayerNameReset_clicked( );
    void on_DontDeleteVirtualHostApply_clicked( );
    void on_DontDeleteVirtualHostSave_clicked( );
    void on_DontDeleteVirtualHostRefresh_clicked( );
    void on_DontDeleteVirtualHostReset_clicked( );
    void on_DontDeletePenultimatePlayerApply_clicked( );
    void on_DontDeletePenultimatePlayerSave_clicked( );
    void on_DontDeletePenultimatePlayerRefresh_clicked( );
    void on_DontDeletePenultimatePlayerReset_clicked( );
    void on_NumRequiredPingsPlayersApply_clicked( );
    void on_NumRequiredPingsPlayersSave_clicked( );
    void on_NumRequiredPingsPlayersRefresh_clicked( );
    void on_NumRequiredPingsPlayersReset_clicked( );
    void on_NumRequiredPingsPlayersASApply_clicked( );
    void on_NumRequiredPingsPlayersASSave_clicked( );
    void on_NumRequiredPingsPlayersASRefresh_clicked( );
    void on_NumRequiredPingsPlayersASReset_clicked( );
    void on_AntiFloodMuteApply_clicked( );
    void on_AntiFloodMuteSave_clicked( );
    void on_AntiFloodMuteRefresh_clicked( );
    void on_AntiFloodMuteReset_clicked( );
    void on_AdminCanUnAutoMuteApply_clicked( );
    void on_AdminCanUnAutoMuteSave_clicked( );
    void on_AdminCanUnAutoMuteRefresh_clicked( );
    void on_AdminCanUnAutoMuteReset_clicked( );
    void on_AntiFloodMuteSecondsApply_clicked( );
    void on_AntiFloodMuteSecondsSave_clicked( );
    void on_AntiFloodMuteSecondsRefresh_clicked( );
    void on_AntiFloodMuteSecondsReset_clicked( );
    void on_AntiFloodRigidityTimeApply_clicked( );
    void on_AntiFloodRigidityTimeSave_clicked( );
    void on_AntiFloodRigidityTimeRefresh_clicked( );
    void on_AntiFloodRigidityTimeReset_clicked( );
    void on_AntiFloodRigidityNumberApply_clicked( );
    void on_AntiFloodRigidityNumberSave_clicked( );
    void on_AntiFloodRigidityNumberRefresh_clicked( );
    void on_AntiFloodRigidityNumberReset_clicked( );
    void on_AdminCanUnCensorMuteApply_clicked( );
    void on_AdminCanUnCensorMuteSave_clicked( );
    void on_AdminCanUnCensorMuteRefresh_clicked( );
    void on_AdminCanUnCensorMuteReset_clicked( );
    void on_CensorMuteApply_clicked( );
    void on_CensorMuteSave_clicked( );
    void on_CensorMuteRefresh_clicked( );
    void on_CensorMuteReset_clicked( );
    void on_CensorMuteAdminsApply_clicked( );
    void on_CensorMuteAdminsSave_clicked( );
    void on_CensorMuteAdminsRefresh_clicked( );
    void on_CensorMuteAdminsReset_clicked( );
    void on_CensorApply_clicked( );
    void on_CensorSave_clicked( );
    void on_CensorRefresh_clicked( );
    void on_CensorReset_clicked( );
    void on_CensorDictionaryApply_clicked( );
    void on_CensorDictionarySave_clicked( );
    void on_CensorDictionaryRefresh_clicked( );
    void on_CensorDictionaryReset_clicked( );
    void on_CensorMuteFirstSecondsApply_clicked( );
    void on_CensorMuteFirstSecondsSave_clicked( );
    void on_CensorMuteFirstSecondsRefresh_clicked( );
    void on_CensorMuteFirstSecondsReset_clicked( );
    void on_CensorMuteSecondSecondsApply_clicked( );
    void on_CensorMuteSecondSecondsSave_clicked( );
    void on_CensorMuteSecondSecondsRefresh_clicked( );
    void on_CensorMuteSecondSecondsReset_clicked( );
    void on_CensorMuteExcessiveSecondsApply_clicked( );
    void on_CensorMuteExcessiveSecondsSave_clicked( );
    void on_CensorMuteExcessiveSecondsRefresh_clicked( );
    void on_CensorMuteExcessiveSecondsReset_clicked( );
    void on_NewCensorCookiesApply_clicked( );
    void on_NewCensorCookiesSave_clicked( );
    void on_NewCensorCookiesRefresh_clicked( );
    void on_NewCensorCookiesReset_clicked( );
    void on_NewCensorCookiesNumberApply_clicked( );
    void on_NewCensorCookiesNumberSave_clicked( );
    void on_NewCensorCookiesNumberRefresh_clicked( );
    void on_NewCensorCookiesNumberReset_clicked( );
    void on_EatGameApply_clicked( );
    void on_EatGameSave_clicked( );
    void on_EatGameRefresh_clicked( );
    void on_EatGameReset_clicked( );
    void on_MaximumNumberCookiesApply_clicked( );
    void on_MaximumNumberCookiesSave_clicked( );
    void on_MaximumNumberCookiesRefresh_clicked( );
    void on_MaximumNumberCookiesReset_clicked( );
    void on_NonAdminCommandsGameApply_clicked( );
    void on_NonAdminCommandsGameSave_clicked( );
    void on_NonAdminCommandsGameRefresh_clicked( );
    void on_NonAdminCommandsGameReset_clicked( );
    void on_PlayerServerPrintJoinApply_clicked( );
    void on_PlayerServerPrintJoinSave_clicked( );
    void on_PlayerServerPrintJoinRefresh_clicked( );
    void on_PlayerServerPrintJoinReset_clicked( );
    void on_FunCommandsGameApply_clicked( );
    void on_FunCommandsGameSave_clicked( );
    void on_FunCommandsGameRefresh_clicked( );
    void on_FunCommandsGameReset_clicked( );
    void on_WarningPacketValidationFailedApply_clicked( );
    void on_WarningPacketValidationFailedSave_clicked( );
    void on_WarningPacketValidationFailedRefresh_clicked( );
    void on_WarningPacketValidationFailedReset_clicked( );
    void on_ResourceTradePrintApply_clicked( );
    void on_ResourceTradePrintSave_clicked( );
    void on_ResourceTradePrintRefresh_clicked( );
    void on_ResourceTradePrintReset_clicked( );
    void on_PlayerBeforeStartPrintApply_clicked( );
    void on_PlayerBeforeStartPrintSave_clicked( );
    void on_PlayerBeforeStartPrintRefresh_clicked( );
    void on_PlayerBeforeStartPrintReset_clicked( );
    void on_PlayerBeforeStartPrintJoinApply_clicked( );
    void on_PlayerBeforeStartPrintJoinSave_clicked( );
    void on_PlayerBeforeStartPrintJoinRefresh_clicked( );
    void on_PlayerBeforeStartPrintJoinReset_clicked( );
    void on_PlayerBeforeStartPrivatePrintJoinApply_clicked( );
    void on_PlayerBeforeStartPrivatePrintJoinSave_clicked( );
    void on_PlayerBeforeStartPrivatePrintJoinRefresh_clicked( );
    void on_PlayerBeforeStartPrivatePrintJoinReset_clicked( );
    void on_PlayerBeforeStartPrivatePrintJoinSDApply_clicked( );
    void on_PlayerBeforeStartPrivatePrintJoinSDSave_clicked( );
    void on_PlayerBeforeStartPrivatePrintJoinSDRefresh_clicked( );
    void on_PlayerBeforeStartPrivatePrintJoinSDReset_clicked( );
    void on_PlayerBeforeStartPrintLeaveApply_clicked( );
    void on_PlayerBeforeStartPrintLeaveSave_clicked( );
    void on_PlayerBeforeStartPrintLeaveRefresh_clicked( );
    void on_PlayerBeforeStartPrintLeaveReset_clicked( );
    void on_BotChannelPrivatePrintJoinApply_clicked( );
    void on_BotChannelPrivatePrintJoinSave_clicked( );
    void on_BotChannelPrivatePrintJoinRefresh_clicked( );
    void on_BotChannelPrivatePrintJoinReset_clicked( );
    void on_BotNamePrivatePrintJoinApply_clicked( );
    void on_BotNamePrivatePrintJoinSave_clicked( );
    void on_BotNamePrivatePrintJoinRefresh_clicked( );
    void on_BotNamePrivatePrintJoinReset_clicked( );
    void on_InfoGamesPrivatePrintJoinApply_clicked( );
    void on_InfoGamesPrivatePrintJoinSave_clicked( );
    void on_InfoGamesPrivatePrintJoinRefresh_clicked( );
    void on_InfoGamesPrivatePrintJoinReset_clicked( );
    void on_GameNamePrivatePrintJoinApply_clicked( );
    void on_GameNamePrivatePrintJoinSave_clicked( );
    void on_GameNamePrivatePrintJoinRefresh_clicked( );
    void on_GameNamePrivatePrintJoinReset_clicked( );
    void on_GameModePrivatePrintJoinApply_clicked( );
    void on_GameModePrivatePrintJoinSave_clicked( );
    void on_GameModePrivatePrintJoinRefresh_clicked( );
    void on_GameModePrivatePrintJoinReset_clicked( );
    void on_RehostPrintingApply_clicked( );
    void on_RehostPrintingSave_clicked( );
    void on_RehostPrintingRefresh_clicked( );
    void on_RehostPrintingReset_clicked( );
    void on_RehostFailedPrintingApply_clicked( );
    void on_RehostFailedPrintingSave_clicked( );
    void on_RehostFailedPrintingRefresh_clicked( );
    void on_RehostFailedPrintingReset_clicked( );
    void on_PlayerFinishedLoadingPrintingApply_clicked( );
    void on_PlayerFinishedLoadingPrintingSave_clicked( );
    void on_PlayerFinishedLoadingPrintingRefresh_clicked( );
    void on_PlayerFinishedLoadingPrintingReset_clicked( );
    void on_LobbyAnnounceUnoccupiedApply_clicked( );
    void on_LobbyAnnounceUnoccupiedSave_clicked( );
    void on_LobbyAnnounceUnoccupiedRefresh_clicked( );
    void on_LobbyAnnounceUnoccupiedReset_clicked( );
    void on_RelayChatCommandsApply_clicked( );
    void on_RelayChatCommandsSave_clicked( );
    void on_RelayChatCommandsRefresh_clicked( );
    void on_RelayChatCommandsReset_clicked( );
    void on_DetourAllMessagesToAdminsApply_clicked( );
    void on_DetourAllMessagesToAdminsSave_clicked( );
    void on_DetourAllMessagesToAdminsRefresh_clicked( );
    void on_DetourAllMessagesToAdminsReset_clicked( );
    void on_AdminMessagesApply_clicked( );
    void on_AdminMessagesSave_clicked( );
    void on_AdminMessagesRefresh_clicked( );
    void on_AdminMessagesReset_clicked( );
    void on_LocalAdminMessagesApply_clicked( );
    void on_LocalAdminMessagesSave_clicked( );
    void on_LocalAdminMessagesRefresh_clicked( );
    void on_LocalAdminMessagesReset_clicked( );
    void on_WarnLatencyApply_clicked( );
    void on_WarnLatencySave_clicked( );
    void on_WarnLatencyRefresh_clicked( );
    void on_WarnLatencyReset_clicked( );
    void on_RefreshMessagesApply_clicked( );
    void on_RefreshMessagesSave_clicked( );
    void on_RefreshMessagesRefresh_clicked( );
    void on_RefreshMessagesReset_clicked( );
    void on_GameAnnounceApply_clicked( );
    void on_GameAnnounceSave_clicked( );
    void on_GameAnnounceRefresh_clicked( );
    void on_GameAnnounceReset_clicked( );
    void on_ShowDownloadsInfoApply_clicked( );
    void on_ShowDownloadsInfoSave_clicked( );
    void on_ShowDownloadsInfoRefresh_clicked( );
    void on_ShowDownloadsInfoReset_clicked( );
    void on_ShowFinishDownloadingInfoApply_clicked( );
    void on_ShowFinishDownloadingInfoSave_clicked( );
    void on_ShowFinishDownloadingInfoRefresh_clicked( );
    void on_ShowFinishDownloadingInfoReset_clicked( );
    void on_ShowDownloadsInfoTimeApply_clicked( );
    void on_ShowDownloadsInfoTimeSave_clicked( );
    void on_ShowDownloadsInfoTimeRefresh_clicked( );
    void on_ShowDownloadsInfoTimeReset_clicked( );
    void on_autoinsultlobbyApply_clicked( );
    void on_autoinsultlobbySave_clicked( );
    void on_autoinsultlobbyRefresh_clicked( );
    void on_autoinsultlobbyReset_clicked( );
    void on_autoinsultlobbysafeadminsApply_clicked( );
    void on_autoinsultlobbysafeadminsSave_clicked( );
    void on_autoinsultlobbysafeadminsRefresh_clicked( );
    void on_autoinsultlobbysafeadminsReset_clicked( );
    void on_GameOwnerPrivatePrintJoinApply_clicked( );
    void on_GameOwnerPrivatePrintJoinSave_clicked( );
    void on_GameOwnerPrivatePrintJoinRefresh_clicked( );
    void on_GameOwnerPrivatePrintJoinReset_clicked( );
    void on_AutoUnMutePrintingApply_clicked( );
    void on_AutoUnMutePrintingSave_clicked( );
    void on_AutoUnMutePrintingRefresh_clicked( );
    void on_AutoUnMutePrintingReset_clicked( );
    void on_ManualUnMutePrintingApply_clicked( );
    void on_ManualUnMutePrintingSave_clicked( );
    void on_ManualUnMutePrintingRefresh_clicked( );
    void on_ManualUnMutePrintingReset_clicked( );
    void on_GameIsOverMessagePrintingApply_clicked( );
    void on_GameIsOverMessagePrintingSave_clicked( );
    void on_GameIsOverMessagePrintingRefresh_clicked( );
    void on_GameIsOverMessagePrintingReset_clicked( );
    void on_manualhclmessageApply_clicked( );
    void on_manualhclmessageSave_clicked( );
    void on_manualhclmessageRefresh_clicked( );
    void on_manualhclmessageReset_clicked( );
    void on_GameAnnounceIntervalApply_clicked( );
    void on_GameAnnounceIntervalSave_clicked( );
    void on_GameAnnounceIntervalRefresh_clicked( );
    void on_GameAnnounceIntervalReset_clicked( );
    void on_NoMapPrintDelayApply_clicked( );
    void on_NoMapPrintDelaySave_clicked( );
    void on_NoMapPrintDelayRefresh_clicked( );
    void on_NoMapPrintDelayReset_clicked( );
    void on_GameLoadedPrintoutApply_clicked( );
    void on_GameLoadedPrintoutSave_clicked( );
    void on_GameLoadedPrintoutRefresh_clicked( );
    void on_GameLoadedPrintoutReset_clicked( );
    void on_StillDownloadingPrintApply_clicked( );
    void on_StillDownloadingPrintSave_clicked( );
    void on_StillDownloadingPrintRefresh_clicked( );
    void on_StillDownloadingPrintReset_clicked( );
    void on_NotSpoofCheckedPrintApply_clicked( );
    void on_NotSpoofCheckedPrintSave_clicked( );
    void on_NotSpoofCheckedPrintRefresh_clicked( );
    void on_NotSpoofCheckedPrintReset_clicked( );
    void on_NotPingedPrintApply_clicked( );
    void on_NotPingedPrintSave_clicked( );
    void on_NotPingedPrintRefresh_clicked( );
    void on_NotPingedPrintReset_clicked( );
    void on_StillDownloadingASPrintApply_clicked( );
    void on_StillDownloadingASPrintSave_clicked( );
    void on_StillDownloadingASPrintRefresh_clicked( );
    void on_StillDownloadingASPrintReset_clicked( );
    void on_NotSpoofCheckedASPrintApply_clicked( );
    void on_NotSpoofCheckedASPrintSave_clicked( );
    void on_NotSpoofCheckedASPrintRefresh_clicked( );
    void on_NotSpoofCheckedASPrintReset_clicked( );
    void on_NotPingedASPrintApply_clicked( );
    void on_NotPingedASPrintSave_clicked( );
    void on_NotPingedASPrintRefresh_clicked( );
    void on_NotPingedASPrintReset_clicked( );
    void on_StillDownloadingASPrintDelayApply_clicked( );
    void on_StillDownloadingASPrintDelaySave_clicked( );
    void on_StillDownloadingASPrintDelayRefresh_clicked( );
    void on_StillDownloadingASPrintDelayReset_clicked( );
    void on_NotSpoofCheckedASPrintDelayApply_clicked( );
    void on_NotSpoofCheckedASPrintDelaySave_clicked( );
    void on_NotSpoofCheckedASPrintDelayRefresh_clicked( );
    void on_NotSpoofCheckedASPrintDelayReset_clicked( );
    void on_NotPingedASPrintDelayApply_clicked( );
    void on_NotPingedASPrintDelaySave_clicked( );
    void on_NotPingedASPrintDelayRefresh_clicked( );
    void on_NotPingedASPrintDelayReset_clicked( );
    void on_PlayerBeforeStartPrintDelayApply_clicked( );
    void on_PlayerBeforeStartPrintDelaySave_clicked( );
    void on_PlayerBeforeStartPrintDelayRefresh_clicked( );
    void on_PlayerBeforeStartPrintDelayReset_clicked( );
    void on_RehostPrintingDelayApply_clicked( );
    void on_RehostPrintingDelaySave_clicked( );
    void on_RehostPrintingDelayRefresh_clicked( );
    void on_RehostPrintingDelayReset_clicked( );
    void on_ButGameDelayApply_clicked( );
    void on_ButGameDelaySave_clicked( );
    void on_ButGameDelayRefresh_clicked( );
    void on_ButGameDelayReset_clicked( );
    void on_MarsGameDelayApply_clicked( );
    void on_MarsGameDelaySave_clicked( );
    void on_MarsGameDelayRefresh_clicked( );
    void on_MarsGameDelayReset_clicked( );
    void on_SlapGameDelayApply_clicked( );
    void on_SlapGameDelaySave_clicked( );
    void on_SlapGameDelayRefresh_clicked( );
    void on_SlapGameDelayReset_clicked( );
    void on_RollGameDelayApply_clicked( );
    void on_RollGameDelaySave_clicked( );
    void on_RollGameDelayRefresh_clicked( );
    void on_RollGameDelayReset_clicked( );
    void on_PictureGameDelayApply_clicked( );
    void on_PictureGameDelaySave_clicked( );
    void on_PictureGameDelayRefresh_clicked( );
    void on_PictureGameDelayReset_clicked( );
    void on_EatGameDelayApply_clicked( );
    void on_EatGameDelaySave_clicked( );
    void on_EatGameDelayRefresh_clicked( );
    void on_EatGameDelayReset_clicked( );
    void on_BotChannelCustomLine_returnPressed( );
    void on_BotChannelCustomApply_clicked( );
    void on_BotChannelCustomSave_clicked( );
    void on_BotChannelCustomRefresh_clicked( );
    void on_BotChannelCustomReset_clicked( );
    void on_BotNameCustomLine_returnPressed( );
    void on_BotNameCustomApply_clicked( );
    void on_BotNameCustomSave_clicked( );
    void on_BotNameCustomRefresh_clicked( );
    void on_BotNameCustomReset_clicked( );
    void on_RejectColoredNameApply_clicked( );
    void on_RejectColoredNameSave_clicked( );
    void on_RejectColoredNameRefresh_clicked( );
    void on_RejectColoredNameReset_clicked( );
    void on_DenyMinDownloadRateApply_clicked( );
    void on_DenyMinDownloadRateSave_clicked( );
    void on_DenyMinDownloadRateRefresh_clicked( );
    void on_DenyMinDownloadRateReset_clicked( );
    void on_DenyMaxDownloadTimeApply_clicked( );
    void on_DenyMaxDownloadTimeSave_clicked( );
    void on_DenyMaxDownloadTimeRefresh_clicked( );
    void on_DenyMaxDownloadTimeReset_clicked( );
    void on_DenyMaxMapsizeTimeApply_clicked( );
    void on_DenyMaxMapsizeTimeSave_clicked( );
    void on_DenyMaxMapsizeTimeRefresh_clicked( );
    void on_DenyMaxMapsizeTimeReset_clicked( );
    void on_DenyMaxReqjoinTimeApply_clicked( );
    void on_DenyMaxReqjoinTimeSave_clicked( );
    void on_DenyMaxReqjoinTimeRefresh_clicked( );
    void on_DenyMaxReqjoinTimeReset_clicked( );
    void on_DenyMaxLoadTimeApply_clicked( );
    void on_DenyMaxLoadTimeSave_clicked( );
    void on_DenyMaxLoadTimeRefresh_clicked( );
    void on_DenyMaxLoadTimeReset_clicked( );
    void on_PrefixNameApply_clicked( );
    void on_PrefixNameSave_clicked( );
    void on_PrefixNameRefresh_clicked( );
    void on_PrefixNameReset_clicked( );
    void on_MaxGamesApply_clicked( );
    void on_MaxGamesSave_clicked( );
    void on_MaxGamesRefresh_clicked( );
    void on_MaxGamesReset_clicked( );
    void on_IPBlackListFileLine_returnPressed( );
    void on_IPBlackListFileApply_clicked( );
    void on_IPBlackListFileSave_clicked( );
    void on_IPBlackListFileRefresh_clicked( );
    void on_IPBlackListFileReset_clicked( );
    void on_gamestateinhouseApply_clicked( );
    void on_gamestateinhouseSave_clicked( );
    void on_gamestateinhouseRefresh_clicked( );
    void on_gamestateinhouseReset_clicked( );
    void on_autohclfromgamenameApply_clicked( );
    void on_autohclfromgamenameSave_clicked( );
    void on_autohclfromgamenameRefresh_clicked( );
    void on_autohclfromgamenameReset_clicked( );
    void on_forceautohclindotaApply_clicked( );
    void on_forceautohclindotaSave_clicked( );
    void on_forceautohclindotaRefresh_clicked( );
    void on_forceautohclindotaReset_clicked( );
    void on_GameStartedMessagePrintingApply_clicked( );
    void on_GameStartedMessagePrintingSave_clicked( );
    void on_GameStartedMessagePrintingRefresh_clicked( );
    void on_GameStartedMessagePrintingReset_clicked( );
    void on_SelectItemBox_currentIndexChanged( int index );
    void on_designeGUISave_clicked( );
    void on_designeGUISettings_clicked( );
    void on_designeGUIRefresh_clicked( );
    void on_designeGUIReset_clicked( );
    void on_MainTab_currentChanged( int index );
    void on_designeGUICombo_currentIndexChanged( int index );
    void on_BnetCommandsRadio_clicked( );
    void on_GameCommandsRadio_clicked( );
    void on_RequiredAccesCombo_currentIndexChanged( int index );
    void on_GameOwnerCheck_stateChanged( int arg1 );
    void on_ChangelogCombo_currentIndexChanged( int index );
    void on_logBackup_clicked( );
    void on_logClear_clicked( );
    void on_logEnable_clicked( );
    void on_quickSetupWizard_clicked( );
    void on_About_UNM_clicked( );
    void on_About_Qt_clicked( );
    void on_logSettings_clicked( );
    void on_restartUNM_clicked( );
    void on_shutdownUNM_clicked( );
    void on_SearchLine_returnPressed( );
    void on_SearchLine_textChanged( const QString &arg1 );
    void on_SearchButton_clicked( );
    void on_ChatTab_currentChanged( int index );

signals:
    void previousPositionChanged( QPoint previousPosition );

private:
    Ui::Widget *ui;
    AnotherWindow *sWindow;
    QList<QWidget*> WidgetsList;
    QList<QLabel*> LabelsList;
    QList<CustomDynamicFontSizeLabel*> HotKeyWidgetsList;
    std::vector<std::string> HotKeyCvarList;
    CConfigSection m_WFECFG;
    QThread thread_1;               // первый поток
    QThread thread_2;               // второй поток
    QThread thread_3;               // третий поток
    ExampleObject exampleObject_1;  // первый объект, который будет работать в первом потоке
    ExampleObject exampleObject_2;  // второй объект, который будет работать во втором потоке
    ExampleObject exampleObject_3;  // третий объект, который будет работать в третем потоке
    bool ready;
    bool m_IgnoreChangeStyle;
    bool m_SimplifiedStyle;
    QString m_StyleDefault;
    QString m_ScrollAreaStyleDefault;
    QString m_ScrollBarStyleDefault;
    void QListFillingProcess( );
    void UpdateAllValuesInGUI( );
    void TabsResizeFix( int type );
    void ChangeStyleGUI( QString styleName = QString( ) );
    void RefreshStylesList( QString currentstyle, bool first = false );
    MouseType m_leftMouseButtonPressed;
    QPoint m_previousPosition;
    MouseType checkResizableField( QMouseEvent *event );
    void CommandListFilling( );
    void ChangelogFilling( int version );
    void scrollTo( int i );
    uint32_t m_LoggingMode;
    QLabel *m_CornerLabel;
    QLabel *m_DarkenedWidget;
    bool m_CornerLabelHaveImage;
    int m_CurrentPossSearch;
    uint32_t m_GameTabColor;
    uint32_t m_WaitForKey;
    bool LShift;
    bool LCtrl;
    bool LAlt;
    bool RShift;
    bool RCtrl;
    bool RAlt;
    void WFEWriteCFG( bool first );
    bool GetKeyName( uint32_t scanCode, std::string &keyName );
    void ClickOnKeySlot( uint32_t keyID );
    void ClickOnKeyPostSlot( std::string keyName, bool cancel = false );

protected:
    void keyPressEvent( QKeyEvent *event );
    void keyReleaseEvent( QKeyEvent *event );
    void mousePressEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
    bool eventFilter( QObject *object, QEvent *event );
    virtual void closeEvent( QCloseEvent *event );
    virtual void resizeEvent( QResizeEvent *event );
    void paintEvent( QPaintEvent * );
};

namespace Ui {
class ChatWidget;
}

class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget( Widget *parent = nullptr, unsigned int BnetID = 0, QString BnetCommandTrigger = QString( "\\" ), QString ServerName = QString( ), QString UserName = QString( ), unsigned int TimeWaitingToReconnect = 0 );
    void write_to_chat_direct( QString message, QString msg );
    void add_bnet_bot_direct( QString botName );
    void update_bnet_bot_direct( QString botName, unsigned int type );
    void delete_bnet_bot_direct( QString botName );
    void add_game_to_list_direct( unsigned int gameID, bool gproxy, unsigned int gameIcon, QStringList gameInfo );
    void update_game_to_list_direct( unsigned int gameID, QStringList gameInfo );
    void delete_game_from_list_direct( unsigned int gameID );
    void bnet_disconnect_direct( );
    void bnet_reconnect_direct( );
    void deletingOldMessages( );
    void game_list_table_pos_fix_direct( );
    Widget *m_MainWidget;
    QString GetBnetCommandTrigger( );
    QLineEdit *GetEntryField( );
    QListWidget *GetChatMembers( );
    unsigned int CurrentTabIndex( );
    void TabsResizeFix( );
    QString m_ServerName;
    QString m_UserName;
    ~ChatWidget( );

private slots:
    void on_tabWidget_currentChanged( int index );
    void on_chatmembers_customContextMenuRequested( const QPoint &pos );
    void on_entryfield_textChanged( const QString &arg1 );
    void on_entryfield_textEdited( const QString &arg1 );
    void on_send_clicked( );
    void on_entryfield_returnPressed( );
    void cellClickedCustom( int iRow, int iColumn );
    void cellDoubleClickedCustom( int iRow, int iColumn );
    void on_mapChooseButton_clicked( );
    void on_mapChooseFindButton_clicked( );
    void on_createGameHelpLabel_linkActivated( const QString &link );
    void on_mapChooseLine_returnPressed( );
    void on_mapUploadButton_clicked( );
    void on_mapUploadLine_returnPressed( );
    void on_botWebSiteButton_clicked( );
    void on_botVkGroupButton_clicked( );
    void on_createGameButton_clicked( );
    void on_bnetBotsCombo_currentTextChanged( const QString &arg1 );
    void on_mapListWidget_itemClicked( QListWidgetItem *item );

private:
    Ui::ChatWidget *ui;
    unsigned int m_BnetID;
    QString m_BnetCommandTrigger;
    unsigned int m_TimeWaitingToReconnect;
    QString m_ReplyTarget;
    QVector<QIcon> m_Icons;
    std::vector<int32_t> m_ChatLinesToDelete;
    std::vector<uint32_t> m_ChatLinesToDeleteTime;
    std::vector<QString> m_ChatLinesToDeleteText;
    QString m_CurrentBotMap;
    QString m_CurrentBotWebSite;
    QString m_CurrentBotVK;
    uint32_t m_CurrentBotStatus;
    void sending_messages_to_battlenet( );
};

namespace Ui {
class ChatWidgetIrina;
}

class ChatWidgetIrina : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidgetIrina( Widget *parent = nullptr );
    QWebSocket *m_WebSocket;
    void write_to_chat_direct( QString message );
    void add_game_to_list_irina( unsigned int gameID, bool gameStarted, unsigned int gameIcon, QStringList gameInfo );
    void update_game_to_list_irina( unsigned int gameID, bool gameStarted, QStringList gameInfo, bool deleteFromCurrentGames );
    void delete_game_from_list_irina( unsigned int gameID, unsigned int type );
    void send_getgame( unsigned int gameID, unsigned char packetType );
    void set_select_game_data( unsigned int gameID, QString mapName, QString mapPlayers, QString mapAuthor, QString mapDescription, QPixmap mapImage );
    void set_select_game_data_bad_tga( unsigned int gameID, QString mapName, QString mapPlayers, QString mapAuthor, QString mapDescription );
    void close_connect( );
    void open_connect( );
    void send_login( );
    void bad_logon( );
    void game_list_table_pos_fix_irina( );
    void success_logon( );
    void send_ping( );
    void send_gamelist( );
    void success_init( );
    void TabsResizeFix( );
    Widget *m_MainWidget;
    ~ChatWidgetIrina( );

public slots:
    void irinaOAuth2_granted( );
    void irinaOAuth2_error( const QString &, const QString &, const QUrl & );
    void auth_button_reset( );

private slots:
    void onConnected( );
    void onDisconnected( );
    void onBytes( const QByteArray &message );
    void onMessage( const QString &message );
    void on_send_clicked( );
    void on_entryfield_returnPressed( );
    void cellClickedCustom( int iRow, int iColumn );
    void cellDoubleClickedCustom( int iRow, int iColumn );
    void on_authButton_clicked( );

private:
    Ui::ChatWidgetIrina *ui;
    QString m_ServerName;
    uint32_t m_ConnectionStatus;
    QOAuth2AuthorizationCodeFlow *irinaOAuth2;
    QOAuthHttpServerReplyHandler *irinaReplyHandler;
    uint32_t m_CurrentGameID;
    QVector<QIcon> m_Icons;
};

namespace Ui {
class GameWidget;
}

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget( Widget *parent = nullptr, unsigned int gameID = 0, QStringList gameInfoInit = { }, QList<QStringList> playersInfo = { } );
    Widget *m_MainWidget;
    unsigned int m_GameID;
    bool m_GameStarted;
    void update_game_direct( QStringList gameInfo, QList<QStringList> playersInfo );
    void update_game_map_info_direct( QString mapPath );
    void write_to_log_direct( QString message );
    void write_to_game_chat_direct( QString message, QString extramessage, QString user, unsigned int type );
    void logfieldSetColor( );
    ~GameWidget( );

private slots:
    void on_war3Run_clicked( );

private:
    Ui::GameWidget *ui;
};


//
// CUNM
//

class CUDPSocket;
class CTCPServer;
class CTCPSocket;
class CGPSProtocol;
class CCRC32;
class CSHA1;
class CBNET;
class CIrinaWS;
class CBaseGame;
class CLanguage;
class CMap;
class CSaveGame;
class CConfig;
class CConfigSection;
class CTCPClient;
class CIncomingGameHost;
class CGameProtocol;
class CCommandPacket;

class CUNM
{
public:
    CTCPServer *m_LocalServer;
    CTCPSocket *m_LocalSocket;
    CTCPClient *m_RemoteSocket;
    CGameProtocol *m_GameProtocol;
    std::queue<CCommandPacket *> m_LocalPackets;
    std::queue<CCommandPacket *> m_RemotePackets;
    std::queue<CCommandPacket *> m_PacketBuffer;
    std::vector<unsigned char> m_Laggers;
    uint32_t m_TotalPacketsReceivedFromLocal;
    uint32_t m_TotalPacketsReceivedFromRemote;
    uint16_t m_Port;
    uint16_t m_GPPort;
    uint32_t m_LastConnectionAttemptTime;
    uint32_t m_LastRefreshTime;
    std::string m_RemoteServerIP;
    uint16_t m_RemoteServerPort;
    uint32_t m_FakeLobbyTime;
    bool m_FakeLobby;
    bool m_GameIsReliable;
    bool m_GameStarted;
    bool m_LeaveGameSent;
    bool m_ActionReceived;
    bool m_Synchronized;
    unsigned char m_PID;
    unsigned char m_ChatPID;
    uint32_t m_ReconnectKey;
    unsigned char m_NumEmptyActions;
    unsigned char m_NumEmptyActionsUsed;
    uint32_t m_LastAckTime;
    uint32_t m_LastActionTime;
    std::string m_JoinedName;
    std::string m_HostName;
    CUDPSocket *m_UDPSocket;                                        // a UDP socket for sending broadcasts and other junk (used with !sendlan)
    CTCPServer *m_ReconnectSocket;                                  // listening socket for GProxy++ reliable reconnects
    std::vector<CTCPSocket *> m_ReconnectSockets;                        // vector of sockets attempting to reconnect (connected but not identified yet)
    CGPSProtocol *m_GPSProtocol;
    CCRC32 *m_CRC;                                                  // for calculating CRC's
    CSHA1 *m_SHA;                                                   // for calculating SHA1's
    std::vector<CBNET *> m_BNETs;                                        // all our battle.net connections (there can be more than one)
    CBaseGame *m_CurrentGame;                                       // this game is still in the lobby state
    CIrinaWS *m_IrinaServer;
    std::vector<CBaseGame *> m_Games;                                    // these games are in progress
    std::vector<BYTEARRAY> m_LocalAddresses;                             // vector of local IP addresses
    CMap *m_Map;                                                    // the currently loaded map
    CSaveGame *m_SaveGame;                                          // the save game to use
    std::vector<PIDPlayer> m_EnforcePlayers;                             // vector of pids to force players to use in the next game (used with saved games)
    bool m_Exiting;                                                 // set to true to force unm to shutdown next update (used by SignalCatcher)
    bool m_ExitingNice;                                             // set to true to force unm to disconnect from all battle.net connections and wait for all games to finish before shutting down
    std::string m_ComputerName;
    std::string m_Version;                                               // UNM version std::string
    uint32_t m_HostCounter;                                         // the current host counter (a unique number to identify a game, incremented each time a game is created)
    uint32_t m_MaxHostCounter;
    bool m_broadcastinlan;
    bool m_AntiFloodMute;                                           // config value: auto mute for flooding or not
    uint32_t m_AntiFloodMuteSeconds;                                // config value: time in seconds the auto mute for flooding
    uint32_t m_AntiFloodRigidityTime;                               // config value: period counting messaging frequency to identify flood in seconds
    uint32_t m_AntiFloodRigidityNumber;                             // config value: how many messages player must send for a specified period of time to get mute for flood
    bool m_AdminCanUnAutoMute;                                      // config value: admin can remove auto mute or not
    bool m_AdminCanUnCensorMute;                                    // config value: admin can remove censor mute or not
    bool m_NewCensorCookies;                                        // config value: new censor system will give cookies or not
    uint32_t m_NewCensorCookiesNumber;                              // config value: how many times need swear to get cookies
    uint32_t m_MsgAutoCorrectMode;                                  // config value: auto correct spelling mode
    bool m_Censor;                                                  // config value: censor bad words or not
    uint32_t m_CensorDictionary;                                    // config value: bad words dictionary
    std::string m_CensorWords;
    bool m_CensorMute;
    bool m_CensorMuteAdmins;
    uint32_t m_CensorMuteFirstSeconds;
    uint32_t m_CensorMuteSecondSeconds;
    uint32_t m_CensorMuteExcessiveSeconds;
    std::vector<std::string> m_CensoredWords;
    bool m_ShuffleSlotsOnStart;
    std::string m_Warcraft3Path;                                         // config value: Warcraft 3 path
    bool m_TFT;                                                     // config value: TFT enabled or not
    std::string m_MapType;                                               // config value: desired filename extension
    std::string m_BindAddress;                                           // config value: the address to host games on
    uint16_t m_HostPort;                                            // config value: the port to host games on
    uint16_t m_BroadcastPort;                                       // config value: the port broadcasting to lan
    bool m_Reconnect;                                               // config value: GProxy++ reliable reconnects enabled or not
    uint16_t m_ReconnectPort;                                       // config value: the port to listen for GProxy++ reliable reconnects on
    uint32_t m_ReconnectWaitTime;                                   // config value: the maximum number of minutes to wait for a GProxy++ reliable reconnect
    uint32_t m_MaxGames;                                            // config value: maximum number of games in progress
    std::string BotCommandTrigger;                                       // config value: the command trigger inside games
    char m_CommandTrigger;                                          // config value: the command trigger inside games
    char m_CommandTrigger1;                                         // config value: the command trigger inside games
    char m_CommandTrigger2;                                         // config value: the command trigger inside games
    char m_CommandTrigger3;                                         // config value: the command trigger inside games
    char m_CommandTrigger4;                                         // config value: the command trigger inside games
    char m_CommandTrigger5;                                         // config value: the command trigger inside games
    char m_CommandTrigger6;                                         // config value: the command trigger inside games
    char m_CommandTrigger7;                                         // config value: the command trigger inside games
    char m_CommandTrigger8;                                         // config value: the command trigger inside games
    char m_CommandTrigger9;                                         // config value: the command trigger inside games
    std::string m_MapCFGPath;                                            // config value: map cfg path
    std::string m_SaveGamePath;                                          // config value: savegame path
    std::string m_MapPath;                                               // config value: map path
    std::string m_ReplayPath;                                            // config value: replay path
    bool m_SaveReplays;                                             // config value: save replays or not
    bool m_AllowAutoRenameMaps;                                     // config value: allow automatic renaming of maps or not
    bool m_OverrideMapDataFromCFG;                                  // config value: allow override map data from map config file (if exists) or not
    bool m_OverrideMapPathFromCFG;                                  // config value: override map path from cfg or not
    bool m_OverrideMapSizeFromCFG;                                  // config value: override map size from cfg or not
    bool m_OverrideMapInfoFromCFG;                                  // config value: override map info from cfg or not
    bool m_OverrideMapCrcFromCFG;                                   // config value: override map crc from cfg or not
    bool m_OverrideMapSha1FromCFG;                                  // config value: override map sha1 from cfg or not
    bool m_OverrideMapSpeedFromCFG;                                 // config value: override map speed from cfg or not
    bool m_OverrideMapVisibilityFromCFG;                            // config value: override map visibility from cfg or not
    bool m_OverrideMapFlagsFromCFG;                                 // config value: override map flags from cfg or not
    bool m_OverrideMapFilterTypeFromCFG;                            // config value: override map filter type from cfg or not
    bool m_OverrideMapGameTypeFromCFG;                              // config value: override map game type from cfg or not
    bool m_OverrideMapWidthFromCFG;                                 // config value: override map width from cfg or not
    bool m_OverrideMapHeightFromCFG;                                // config value: override map height from cfg or not
    bool m_OverrideMapTypeFromCFG;                                  // config value: override map type from cfg or not
    bool m_OverrideMapDefaultHCLFromCFG;                            // config value: override map default HCL from cfg or not
    bool m_OverrideMapLoadInGameFromCFG;                            // config value: override map load in game from cfg or not
    bool m_OverrideMapNumPlayersFromCFG;                            // config value: override map num players from cfg or not
    bool m_OverrideMapNumTeamsFromCFG;                              // config value: override map num teams from cfg or not
    bool m_OverrideMapSlotsFromCFG;                                 // config value: override map slots from cfg or not
    bool m_OverrideMapObserversFromCFG;                             // config value: override map observers from cfg or not
    bool m_PlayersCanChangeRace;                                    // config value: players can change the race in custom maps
    std::string m_VirtualHostName;                                       // config value: virtual host name
    uint32_t m_BroadcastHostName;                                   // config value: host name broadcasting to lan
    std::string m_BroadcastHostNameCustom;                               // config value: host custom name broadcasting to lan
    bool m_DontDeleteVirtualHost;
    bool m_DontDeletePenultimatePlayer;
    bool m_ResourceTradePrint;
    bool m_WaitForEndGame;
    std::string m_ObsPlayerName;                                         // config value: obs player name
    bool m_HideIPAddresses;                                         // config value: hide IP addresses from players
    bool m_CheckMultipleIPUsage;                                    // config value: check for multiple IP address usage
    uint32_t m_SpoofChecks;                                         // config value: do automatic spoof checks or not
    uint32_t m_NumRequiredPingsPlayers;                             // config value: number required pings players for start
    uint32_t m_NumRequiredPingsPlayersAS;                           // config value: number required pings players for auto start
    uint32_t m_CountDownStartCounter;                               // config value: countdown start counter
    uint32_t m_CountDownStartForceCounter;                          // config value: countdown start force counter
    uint32_t m_CountDownMethod;                                     // config value: countdown method
    uint32_t m_ShowPesettingWindow;
    uint32_t m_OptionOwner;                                         // config value: option !owner command for admins
    uint32_t m_AFKtimer;                                            // config value: AFK timer
    uint32_t m_AFKkick;                                             // config value: AFK kick or not
    bool m_RefreshMessages;                                         // config value: display refresh messages or not (by default)
    bool m_AutoSave;                                                // config value: auto save before someone disconnects
    uint32_t m_AllowDownloads;                                      // config value: allow map downloads or not
    bool m_PingDuringDownloads;                                     // config value: ping during map downloads or not
    bool m_LCPings;                                                 // config value: use LC style pings (divide actual pings by two)
    bool m_UserCanDropAdmin;                                        // config value: user can drop admin if he is lagging or not
    uint32_t m_DropVotePercent;                                     // config value: value in percent for votedrop
    uint32_t m_DropVoteTime;                                        // config value: accept drop votes after this amount of seconds
    uint32_t m_DropWaitTime;                                        // config value: how long to wait after start lag screen before auto drop lagging player
    uint32_t m_DropWaitTimeSum;                                     // config value: how long player can lag (in total) after start lag screen before auto drop
    uint32_t m_DropWaitTimeGame;                                    // config value: how long to wait after player began to lag before auto drop lagging player in started game
    uint32_t m_DropWaitTimeLoad;                                    // config value: how long to wait after player began to lag before auto drop lagging player during loading
    uint32_t m_DropWaitTimeLobby;                                   // config value: how long to wait after player began to lag before auto drop lagging player in lobby
    uint32_t m_AllowLaggingTime;                                    // config value: how many seconds we allow players to lag in certain period of time before counter m_DropWaitTimeSum starts counting
    uint32_t m_AllowLaggingTimeInterval;                            // config value: time interval in minutes for m_AllowLaggingTime
    uint32_t m_NoMapPrintDelay;                                     // config value: delay for print message "doesn't have the map and map downloads are disabled"
    uint32_t m_AutoKickPing;                                        // config value: auto kick players with ping higher than this
    std::string m_IPBlackListFile;                                       // config value: IP blacklist file
    uint32_t m_Latency;                                             // config value: the latency (by default)
    uint32_t m_MinLatency;                                          // config value: the minimum allowed latency if you are using default system of latency
    uint32_t m_MaxLatency;                                          // config value: the maximum allowed latency if you are using default system of latency
    bool m_WarnLatency;                                             // config value: warn (print messages in log) if the bot does not have enough resources for a stable game on a low latency
    uint32_t m_SyncLimit;                                           // config value: the maximum number of packets a player can fall out of sync before starting the lag screen (by default)
    uint32_t m_MinSyncLimit;                                        // config value: the minimum allowed synclimit
    uint32_t m_MaxSyncLimit;                                        // config value: the maximum allowed synclimit
    bool m_VoteStartAllowed;                                        // config value: if votestarts are allowed or not
    uint32_t m_VoteStartMinPlayers;                                 // config value: minimum number of players before users can !votestart
    bool m_VoteStartPercentalVoting;                                // config value: votestart percental (true) or absolute (false)
    uint32_t m_VoteStartPercent;                                    // config value: value in percent for votestart
    uint32_t m_VoteStartDuration;                                   // config value: the duration of the voting for game start
    bool m_CancelVoteStartIfLeave;                                  // config value: cancel voting for game start if someone leave
    bool m_StartAfterLeaveIfEnoughVotes;                            // config value: auto start game if enough votes for game start after someone left the lobby
    bool m_VoteKickAllowed;                                         // config value: if votekicks are allowed or not
    uint32_t m_VoteKickPercentage;                                  // config value: percentage of players required to vote yes for a votekick to pass
    std::string m_DefaultMap;                                            // config value: default map (map.cfg)
    bool m_LocalAdminMessages;                                      // config value: send local admin messages or not
    bool m_AdminMessages;                                           // config value: send admin messages or not
    unsigned char m_LANWar3Version;                                 // config value: LAN warcraft 3 version
    uint32_t m_ReplayWar3Version;                                   // config value: replay warcraft 3 version (for saving replays)
    uint16_t m_ReplayBuildNumber;                                   // config value: replay build number (for saving replays)
    uint32_t m_DenyMinDownloadRate;                                 // config value: the allowed minimum download rate in KB/s
    uint32_t m_DenyMaxDownloadTime;                                 // config value: the allowed maximum download time in seconds
    uint32_t m_DenyMaxMapsizeTime;                                  // config value: the maximum time in seconds to wait for a mapsize packet
    uint32_t m_DenyMaxReqjoinTime;                                  // config value: the maximum time in seconds to wait for a reqjoin packet
    uint32_t m_DenyMaxIPUsage;                                      // config value: the maximum number of users from the same IP address to accept
    uint32_t m_DenyMaxLoadTime;                                     // config value: the maximum time in seconds to wait for players to load
    bool m_StillDownloadingPrint;                                   // config value: print StillDownloading messages at start or not
    bool m_NotSpoofCheckedPrint;                                    // config value: print NotSpoofChecked messages at start or not
    bool m_NotPingedPrint;                                          // config value: print NotPinged messages at start or not
    bool m_StillDownloadingASPrint;                                 // config value: print StillDownloading messages at auto start or not
    bool m_NotSpoofCheckedASPrint;                                  // config value: print NotSpoofChecked messages at auto start or not
    bool m_NotPingedASPrint;                                        // config value: print NotPinged messages at auto start or not
    uint32_t m_StillDownloadingASPrintDelay;                        // config value: delay for print StillDownloading messages
    uint32_t m_NotSpoofCheckedASPrintDelay;                         // config value: delay for print NotSpoofChecked messages
    uint32_t m_NotPingedASPrintDelay;                               // config value: delay for print NotPinged messages
    uint32_t m_TryAutoStartDelay;                                   // config value: delay for try auto start
    bool m_PlayerBeforeStartPrint;                                  // config value: print WaitingForPlayersBeforeStart messages or not
    uint32_t m_PlayerBeforeStartPrintDelay;                         // config value: delay for print WaitingForPlayersBeforeStart messages
    bool m_PlayerBeforeStartPrintJoin;                              // config value: print WaitingForPlayersBeforeStart messages when someone join or not
    bool m_PlayerBeforeStartPrivatePrintJoin;                       // config value: print WaitingForPlayersBeforeStart private messages when someone join or not
    bool m_PlayerBeforeStartPrivatePrintJoinSD;                     // config value: enable smart delay for print WaitingForPlayersBeforeStart private messages when someone join or not (important only if used m_PlayerBeforeStartPrint)
    bool m_PlayerBeforeStartPrintLeave;                             // config value: print WaitingForPlayersBeforeStart messages when someone leave or not
    bool m_BotChannelPrivatePrintJoin;                              // config value: print BotChannel private messages when someone join or not
    std::string m_BotChannelCustom;                                      // config value: custom BotChannel message
    bool m_BotNamePrivatePrintJoin;                                 // config value: print BotName private messages when someone join or not
    std::string m_BotNameCustom;                                         // config value: custom BotName message
    bool m_InfoGamesPrivatePrintJoin;                               // config value: print InfoGames private messages when someone join or not
    bool m_GameNamePrivatePrintJoin;                                // config value: print GameName private messages when someone join or not
    uint32_t m_GameOwnerPrivatePrintJoin;                           // config value: print GameOwner private messages when someone join or not
    bool m_GameModePrivatePrintJoin;                                // config value: print GameMode private messages when someone join or not
    bool m_RehostPrinting;                                          // config value: print rehost messages in game or not
    uint32_t m_RehostPrintingDelay;                                 // config value: delay for print rehost messages in game
    bool m_RehostFailedPrinting;                                    // config value: print rehost failed messages in game or not
    bool m_PlayerFinishedLoadingPrinting;                           // config value: print messages when player finished loading or not
    uint32_t m_AutoUnMutePrinting;                                  // config value: print messages when player was auto unmuted
    uint32_t m_ManualUnMutePrinting;                                // config value: print messages when player was unmuted by admin
    bool m_GameStartedMessagePrinting;                              // config value: print message on bnet channel when the game has started or not
    uint32_t m_GameIsOverMessagePrinting;                           // config value: print message on bnet channel when the game is over or not
    bool m_WarningPacketValidationFailed;                           // config value: print warning in log if player packets validation failed in game (if player actions size > 1024)
    bool m_FunCommandsGame;                                         // config value: allow use fun commands in game or not
    uint32_t m_ButGameDelay;                                        // config value: cooldown for command !but in game
    uint32_t m_MarsGameDelay;                                       // config value: cooldown for command !mars in game
    uint32_t m_SlapGameDelay;                                       // config value: cooldown for command !slap in game
    uint32_t m_RollGameDelay;                                       // config value: cooldown for command !roll in game
    uint32_t m_PictureGameDelay;                                    // config value: cooldown for command like !duck in game
    uint32_t m_MaximumNumberCookies;                                // config value: the maximum number of cookies which can have each player
    bool m_EatGame;                                                 // config value: allow use the command !eat in game or not
    uint32_t m_EatGameDelay;                                        // config value: cooldown for command !eat in game
    uint32_t m_PubGameDelay;                                        // config value: cooldown for command !pub in game
    uint32_t m_MinLengthGN;                                         // config value: allowed the minimum length of game name
    uint32_t m_SlotInfoInGameName;                                  // config value: add info about occupied/free slots in gamename or not
    uint32_t m_ActualRehostPrintingDelay;                           // counts the number of checks before printing again
    uint32_t m_MoreFPsLobby;
    uint32_t m_PlaceAdminsHigher;                                   // config value: auto swap admins as high as possible when they join in the lobby or not
    bool m_RelayChatCommands;                                       // config value: show/hide issued commands
    bool m_FixGameNameForIccup;                                     // config value: prevent host games with unacceptable game name on iccup
    bool m_AppleIcon;
    bool m_ResetGameNameChar;
    bool m_FakePlayersLobby;
    bool m_PrefixName;
    bool m_PlayerServerPrintJoin;
    bool m_ReplaysByName;
    bool m_RejectColoredName;
    bool m_ForceLoadInGame;
    bool m_ShowRealSlotCount;
    std::string m_InvalidTriggersGame;
    std::string m_InvalidReplayChars;
    std::string m_RehostChar;
    std::string m_DefaultGameNameChar;
    std::string m_GameNameChar;
    std::string m_TempGameNameChar;
    uint32_t m_GameNameCharHide;                                    // config value: hide game name char or not
    std::string m_HostMapConfig[1000];
    std::string m_HostMapCode[1000];
    std::string m_LastGameName;
    bool m_LogBackup;
    bool m_LogBackupForce;
    int64_t m_MaxLogSize;
    uint32_t m_MaxLogTime;
    uint32_t m_LastCheckLogTime;
    uint32_t m_LastCheckLogForSizeTime;
    uint32_t m_AuthorizationPassedTime;
    uint32_t m_DeleteOldMessagesTime;
    std::deque<std::string> m_LogMessages;
    std::vector<std::string> m_CachedSpoofedIPs;
    std::vector<std::string> m_CachedSpoofedNames;
    std::vector<std::string> m_Announce;                                      // our game announce messages
    std::vector<std::string> m_Welcome;                                       // our game welcome messages
    std::vector<std::string> m_FPNames;                                       // our fake player names
    std::vector<std::string> m_FPNamesLast;                                   // our last fake player names
    std::vector<std::string> m_Mars;                                          // our mars messages
    std::vector<std::string> m_MarsLast;                                      // our last mars messages
    std::vector<std::string> m_But;                                           // our but messages
    std::vector<std::string> m_ButLast;                                       // our last but messages
    std::vector<std::string> m_Slap;                                          // our slap messages
    std::vector<std::string> m_SlapLast;                                      // our last slap messages
    uint32_t m_GameLoadedPrintout;                                  // config value: how many secs should unm wait to printout the GameLoaded msg
    uint32_t m_GameAnnounce;                                        // config value: print announce messages in game or not
    uint32_t m_GameAnnounceInterval;                                // config value: interval between printing announce messages in game
    bool m_NonAdminCommandsGame;                                    // config value: non admin commands in games available or not
    bool m_SpoofCheckIfGameNameIsIndefinite;                        // config value: users can spoofchecked in game with indefinite gamename on Battle.net/PVPGN servers
    bool m_autohclfromgamename;                                     // config value: auto set HCL based on gamename, ignore map_defaulthcl
    bool m_forceautohclindota;
    uint32_t m_manualhclmessage;
    uint32_t m_gameoverminpercent;                                  // config value: initiate game over timer when percent of people remaining is less than
    uint32_t m_gameoverminplayers;                                  // config value: initiate game over timer when there are less than this number of players
    uint32_t m_gameovermaxteamdifference;                           // config value: initiate game over timer if team unbalance is greater than this
    uint32_t m_totaldownloadspeed;                                  // config value: total download speed allowed per all clients
    uint32_t m_clientdownloadspeed;                                 // config value: max download speed per client
    uint32_t m_maxdownloaders;                                      // config value: max clients allowed to download at once
    uint32_t m_NewRefreshTime;                                      // config value: send refresh every n seconds
    bool m_patch23;                                                 // config value: use for patch 1.23
    bool m_patch21;                                                 // config value: use for patch 1.21
    uint32_t m_gamestateinhouse;
    bool m_DetourAllMessagesToAdmins;
    bool m_NormalCountdown;
    bool m_UseDynamicLatency;
    uint32_t m_DynamicLatencyRefreshRate;
    uint32_t m_DynamicLatencyConsolePrint;
    uint32_t m_DynamicLatencyPercentPingMax;
    uint32_t m_DynamicLatencyDifferencePingMax;
    uint32_t m_DynamicLatencyMaxSync;
    uint32_t m_DynamicLatencyAddIfMaxSync;
    uint32_t m_DynamicLatencyAddIfLag;
    uint32_t m_DynamicLatencyAddIfBotLag;
    uint32_t m_DynamicLatencyLowestAllowed;
    uint32_t m_DynamicLatencyHighestAllowed;
    bool m_LobbyAnnounceUnoccupied;
    bool m_autoinsultlobby;
    bool m_autoinsultlobbysafeadmins;
    bool m_ShowDownloadsInfo;                                       // config value: show info on downloads in progress
    bool m_ShowFinishDownloadingInfo;                               // config value: show info on finished downloading
    uint32_t m_ShowDownloadsInfoTime;
    bool m_HoldPlayersForRMK;
    std::vector<std::string> m_PlayerNamesfromRMK;
    std::vector<std::string> m_PlayerServersfromRMK;
    bool m_newGame;
    std::string m_newGameUser;
    std::string m_newGameServer;
    unsigned char m_newGameGameState;
    std::string m_newGameName;
    bool m_LogReduction;
    uint32_t m_LobbyTimeLimit;
    uint32_t m_LobbyTimeLimitMax;
    std::string m_CurrentGameName;
    std::string m_s;
    std::string m_st;
    std::string m_s1;
    std::string m_s2;
    std::string m_s3;
    std::string m_s4;
    std::string m_s5;
    std::string m_s6;
    std::string m_s7;
    std::string m_s8;
    std::string m_s9;
    std::string m_s0;
    uint32_t m_CurrentGameCounter;
    uint32_t m_UsedBnetID;
    uint32_t m_UniqueGameID;
    uint32_t m_CurrentGameID;
    uint32_t m_LastWar3RunTicks;
    CUNM( CConfig *CFG );
    ~CUNM( );
    void ReadAnnounce( );
    void ReadWelcome( );
    void ReadMars( );
    std::string GetMars( );
    void ReadBut( );
    std::string GetBut( );
    void ReadSlap( );
    std::string GetSlap( );
    void ReadFP( );
    std::string GetFPName( );
    std::string GetRehostChar( );
    bool IsIccupServer( std::string servername );
    std::string HostMapCode( std::string name );
    void ReloadConfig( CConfig *CFG );
    void AddSpoofedIP( std::string name, std::string ip );
    bool IsSpoofedIP( std::string name, std::string ip );
    void ParseCensoredWords( );
    std::string Censor( std::string msg );
    std::string CensorMessage( std::string msg );
    bool CensorCheck( std::string message );
    bool IsCorrectNumInSegm( std::string const & s, int min, int max );
    bool IPAdrIsCorrect( std::string const & s );
    bool IPIsValid( std::string ip );
    std::string IncGameNr( std::string name, bool spambot, bool needEncrypt, uint32_t occupiedslots, uint32_t allslots );
    std::string IncGameNrEncryption( std::string name );
    std::string IncGameNrDecryption( std::string name, bool force = false );
    std::string CheckMapConfig( std::string map, bool force = false );
    std::string FindMap( std::string map, std::string mapdefault = std::string( ) );
    void MapsScan( bool ConfigsScan, std::string Pattern, std::string &FilePath, std::string &File, std::string &Stem, std::string &FoundMaps, uint32_t &Matches );
    void MapLoad( std::string FilePath, std::string File, std::string Stem, std::string ServerAliasWithUserName, bool loadcfg, bool melee = false );
    void MapAutoRename( std::string &FilePath, std::string &File, std::string toLog );
    uint32_t TimeDifference( uint32_t sub, uint32_t minimum = 0 );
    uint32_t TicksDifference( uint32_t sub, uint32_t minimum = 0 );
    bool IsBnetServer( std::string server );

    // processing functions

    bool Update( unsigned long usecBlock );

    // events

    void EventBNETGameRefreshed( CBNET *bnet );
    void EventBNETGameRefreshFailed( CBNET *bnet );
    void EventGameDeleted( CBaseGame *game );

    // other functions

    void ExtractScripts( );
    void CreateGame( CMap *map, unsigned char gameState, bool saveGame, std::string gameName, std::string ownerName, std::string creatorName, std::string creatorServer, bool whisper, std::string hostmapcode );
    CTCPServer *m_GameBroadcastersListener;			// listening socket for game broadcasters
    std::vector<CTCPSocket *> m_GameBroadcasters;		// vector of sockets that broadcast the games
    bool m_DropIfDesync;							// config value: Drop desynced players

    // gproxy

    void ExtractLocalPackets( );
    void ProcessLocalPackets( );
    void ExtractRemotePackets( );
    void ProcessRemotePackets( );
    void SendLocalChat( std::string message );
    void SendEmptyAction( );
    void BackToBnetChannel( );
};

#endif // UNM_H
