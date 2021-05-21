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
#include "ui_unm.h"
#include "ui_anotherwindow.h"
#include "ui_presettingwindow.h"
#include "ui_helplogonwindow.h"
#include "ui_logsettingswindow.h"
#include "ui_stylesettingwindow.h"
#include "ui_chatwidget.h"
#include "ui_chatwidgetirina.h"
#include "ui_gamewidget.h"
#include "util.h"
#include "unmcrc32.h"
#include "unmsha1.h"
#include "csvparser.h"
#include "config.h"
#include "configsection.h"
#include "language.h"
#include "socket.h"
#include "bnet.h"
#include "irinaws.h"
#include "map.h"
#include "packed.h"
#include "savegame.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "gpsprotocol.h"
#include "game_base.h"
#include "game.h"
#include "bnetprotocol.h"
#include "commandpacket.h"
#include "customtablewidgetitem.h"
#include "customstyledItemdelegate.h"
#include "customstyledItemdelegateirina.h"
#include <cstring>
#include <signal.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstdint>
#include <regex>

#define __STORMLIB_SELF__
#include "src/stormlib/StormLib.h"

#ifdef WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <wininet.h>
#include <winsock.h>
#include "Winsock2.h"
#include "Ws2tcpip.h"
#pragma comment(lib, "winmm.lib")
#endif

#include <time.h>

#ifndef WIN32
#include <sys/time.h>
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#ifndef WIN32
#define _GNU_SOURCE
#endif

#define CURL_STATICLIB
#include <curl/curl.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

#define DISCORD_IDENTIFIER "657648034456272966"
#define DISCORD_SHARED_KEY "S9y9hmZOxzR3k0MR0IbcKKih3T7-CWk5"

CLanguage *gLanguage = nullptr;
string gLanguageFile = string( );
static CUNM *gUNM = nullptr;
static string gWindowTitle;
static string gCFGFile;
static string gLogFile;
static uint32_t gEnableLoggingToGUI;
static bool gUNMLoaded = false;
static uint32_t gLogMethod;
static ofstream *gLog = nullptr;
ExampleObject *gToLog = new ExampleObject( );
static uint32_t IccupLoadersLaunchTime = 0;
static vector<unsigned long> RunnedIccupLoaders;
static uint32_t RunIccupLoadersTimeout = 100;
static vector<uint16_t> IccupLoadersPorts;
static bool DelayedRunIccupLoaders = false;
static unsigned int TimerResolution = 0;
static string LogonDataServer = string( );
static string LogonDataLogin = string( );
static string LogonDataPassword = string( );
bool LogonDataRunAH = false;
static bool LogonDataRememberMe = false;
bool AuthorizationPassed = false;
bool SocketLogging = false;
static bool unmLoaded = false;
static bool gSkipPresetting = false;
static QString gBNETCommandTrigger = "\\";
static string gCurrentStyleName = "dafault";
static QString gNormalMessagesColor = "#000000";
static QString gPrivateMessagesColor = "#00ff00";
static QString gNotifyMessagesColor = "#0000ff";
static QString gWarningMessagesColor = "#ff0000";
static QString gPreSentMessagesColor = "#808080";
static bool gDefaultScrollBarForce = false;
static string globalwebdata = string( );
static uint32_t gBotID = 1;
string gAltThreadState = string( );
static string gAltThreadStateMSG = string( );
bool gExitProcessStarted = false;
bool gRestartAfterExiting = false;
QString gOAuthDiscordToken = QString( );
QString gOAuthDiscordName = QString( );
int gGameRoleStarted = Qt::UserRole;
int gGameRoleGProxy = Qt::UserRole;
int gGameRoleIcon = Qt::UserRole + 1;
int gGameRoleID = Qt::UserRole + 3;
int gGameRolePlayers = Qt::UserRole + 4;
int gGameRoleHost = Qt::UserRole + 4;

void CloseIccupLoaders( )
{
    if( gUNM && gUNM != nullptr )
    {
        while( !RunnedIccupLoaders.empty( ) )
        {
            DWORD ExitCode;
            HANDLE hp;
            bool ret = true;

            hp = OpenProcess( PROCESS_ALL_ACCESS, true, RunnedIccupLoaders.back( ) );

            if( hp )
            {
                GetExitCodeProcess( hp, &ExitCode );
                ret = TerminateProcess( hp, ExitCode );
            }

            CloseHandle( hp );
            RunnedIccupLoaders.pop_back( );
        }
    }
}

BOOL WINAPI CtrlHandler( DWORD fdwCtrlType )
{
    switch( fdwCtrlType )
    {
        // Handle the CTRL-C signal.
        case CTRL_C_EVENT:
        CloseIccupLoaders( );
        return TRUE;

        // CTRL-CLOSE: confirm that the user wants to exit.
        case CTRL_CLOSE_EVENT:
        CloseIccupLoaders( );
        return TRUE;

        // Pass other signals to the next handler.
        case CTRL_BREAK_EVENT:
        CloseIccupLoaders( );
        return TRUE;

        case CTRL_LOGOFF_EVENT:
        CloseIccupLoaders( );
        return FALSE;

        case CTRL_SHUTDOWN_EVENT:
        CloseIccupLoaders( );
        return FALSE;

        default:
        return FALSE;
    }
}

void ShowInExplorer( QString path, bool selectForlder )
{
    QFileInfo info( path );

#if defined(Q_OS_WIN)
    QStringList args;

    if ( !info.isDir( ) || selectForlder )
        args << "/select,";

    args << QDir::toNativeSeparators(path);

    if( QProcess::startDetached( "explorer", args ) )
        return;
#elif defined(Q_OS_MAC)
       QStringList args;
       args << "-e";
       args << "tell application \"Finder\"";
       args << "-e";
       args << "activate";
       args << "-e";
       args << "select POSIX file \"" + path + "\"";
       args << "-e";
       args << "end tell";
       args << "-e";
       args << "return";

       if( !QProcess::execute( "/usr/bin/osascript", args ) )
           return;
#endif

    QDesktopServices::openUrl( QUrl::fromLocalFile( info.isDir( ) ? path : info.path( ) ) );
}

void RunIccupLoaders( )
{
    uint32_t LastRunTime = 0;

    for( uint32_t i = 0; i < IccupLoadersPorts.size( ); i++ )
    {
        HANDLE g_hChildStd_IN_Rd = nullptr;
        HANDLE g_hChildStd_IN_Wr = nullptr;
        HANDLE g_hChildStd_OUT_Rd = nullptr;
        HANDLE g_hChildStd_OUT_Wr = nullptr;
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof( SECURITY_ATTRIBUTES );
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = nullptr;

        if( !CreatePipe( &g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0 ) || !SetHandleInformation( g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0 ) || !CreatePipe( &g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0 ) || !SetHandleInformation( g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0 ) )
            CONSOLE_Print( "[UNM] не удалось наладить связь с IccReconnectLoader.exe (запуск не состоялся)" );
        else
        {
            PROCESS_INFORMATION piProcInfo;
            STARTUPINFO siStartInfo;
            ZeroMemory( &piProcInfo, sizeof( PROCESS_INFORMATION ) );
            ZeroMemory( &siStartInfo, sizeof( STARTUPINFO ) );
            siStartInfo.cb = sizeof( STARTUPINFO );
            siStartInfo.hStdError = g_hChildStd_OUT_Wr;
            siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
            siStartInfo.hStdInput = g_hChildStd_IN_Rd;
            siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

            if( RunIccupLoadersTimeout < 100 && LastRunTime != 0 && GetTicks( ) - LastRunTime < RunIccupLoadersTimeout )
                std::this_thread::sleep_for( std::chrono::milliseconds( RunIccupLoadersTimeout - ( GetTicks( ) - LastRunTime ) ) );

            LastRunTime = GetTicks( );
            string IccReconnectLoader;
            string CommandLine = "iCCupLoader";

#ifdef WIN32
            IccReconnectLoader = "iCCupLoader\\IccReconnectLoader.exe";
#else
            IccReconnectLoader = "iCCupLoader/IccReconnectLoader.exe";
#endif

            if( !CreateProcessA( IccReconnectLoader.c_str( ), nullptr, nullptr, nullptr, TRUE, CREATE_NO_WINDOW + HIGH_PRIORITY_CLASS, nullptr, CommandLine.c_str( ), (LPSTARTUPINFOA)&siStartInfo, &piProcInfo ) )
                CONSOLE_Print( "[UNM] не удалось запустить IccReconnectLoader.exe" );
            else
            {
                CONSOLE_Print( "[UNM] IccReconnectLoader.exe был успешно запущен!" );
                RunnedIccupLoaders.push_back( piProcInfo.dwProcessId );
                CloseHandle( piProcInfo.hProcess );
                CloseHandle( piProcInfo.hThread );

                if( GetStdHandle( STD_INPUT_HANDLE ) == INVALID_HANDLE_VALUE )
                    CONSOLE_Print( "[UNM] не удалось установить связь с IccReconnectLoader.exe" );
                else
                {
                    DWORD dwRead, dwWritten;
                    CHAR chBuf[8] = "";
                    string port = UTIL_ToString( IccupLoadersPorts[i] ) + "\n";
                    strcpy( chBuf, port.c_str( ) );
                    dwRead = strlen( chBuf );

                    if( !WriteFile( g_hChildStd_IN_Wr, chBuf, dwRead, &dwWritten, nullptr ) )
                        CONSOLE_Print( "[UNM] не удалось задать порт в IccReconnectLoader.exe" );
                    else
                        CONSOLE_Print( "[UNM] в IccReconnectLoader.exe был задан порт " + UTIL_ToString( IccupLoadersPorts[i] ) );
                }
            }
        }
    }

    IccupLoadersLaunchTime = GetTicks( );
    DelayedRunIccupLoaders = false;

    if( !RunnedIccupLoaders.empty( ) )
        SetConsoleCtrlHandler( CtrlHandler, TRUE );
}

uint32_t GetTime( )
{
    return GetTicks( ) / 1000;
}

uint32_t GetTicks( )
{
#ifdef WIN32
    // don't use GetTickCount anymore because it's not accurate enough (~16ms resolution)
    // don't use QueryPerformanceCounter anymore because it isn't guaranteed to be strictly increasing on some systems and thus requires "smoothing" code
    // use timeGetTime instead, which typically has a high resolution (5ms or more) but we request a lower resolution on startup
    return timeGetTime( );
#elif __APPLE__
    uint64_t current = mach_absolute_time( );
    static mach_timebase_info_data_t info = { 0, 0 };
    // get timebase info
    if( info.denom == 0 )
        mach_timebase_info( &info );
    uint64_t elapsednano = current * ( info.numer / info.denom );
    // convert ns to ms
    return elapsednano / 1e6;
#else
    uint32_t ticks;
    struct timespec t;
    clock_gettime( CLOCK_MONOTONIC, &t );
    ticks = t.tv_sec * 1000;
    ticks += t.tv_nsec / 1000000;
    return ticks;
#endif
}

bool Patch21( )
{
    return gUNM->m_patch21;
}

void SignalCatcher2( int s )
{
    CONSOLE_Print( "[!!!] caught signal " + UTIL_ToString( s ) + ", exiting NOW" );

    if( gUNM )
    {
        if( gUNM->m_Exiting )
            exit( 1 );
        else
            gUNM->m_Exiting = true;
    }
    else
        exit( 1 );
}

void SignalCatcher( int s )
{
    signal( SIGINT, SignalCatcher2 );
    CONSOLE_Print( "[!!!] caught signal " + UTIL_ToString( s ) + ", exiting nicely" );

    if( gUNM )
        gUNM->m_ExitingNice = true;
    else
        exit( 1 );
}

void ApplicationShutdown( )
{
    while( !RunnedIccupLoaders.empty( ) )
    {
        DWORD ExitCode;
        HANDLE hp;
        bool ret = true;

        hp = OpenProcess( PROCESS_ALL_ACCESS, true, RunnedIccupLoaders.back( ) );

        if( hp )
        {
            GetExitCodeProcess( hp, &ExitCode );
            ret = TerminateProcess( hp, ExitCode );
        }

        CloseHandle( hp );
        RunnedIccupLoaders.pop_back( );
    }

    if( gUNM != nullptr )
    {
        delete gUNM;
        gUNM = nullptr;
    }

    // shutdown unm

    remove( "process.ini" );
    CONSOLE_Print( "[UNM] shutting down" );

#ifdef WIN32
    // shutdown winsock

    CONSOLE_Print( "[UNM] shutting down winsock" );
    WSACleanup( );

    // shutdown timer

    timeEndPeriod( TimerResolution );
#endif

    if( gLog != nullptr )
    {
        gLog->close( );
        delete gLog;
        gLog = nullptr;
    }

    if( gToLog != nullptr )
    {
        delete gToLog;
        gToLog = nullptr;
    }
}

size_t write_data( void *ptr, size_t size, size_t nmemb, void *stream )
{
    size_t written = fwrite( ptr, size, nmemb, (FILE *)stream );
    return written;
}

size_t write_data_record( void *ptr, size_t size, size_t nmemb, void *stream )
{
    std::string currentwebdata = reinterpret_cast<const char* const>(ptr);
    globalwebdata += currentwebdata;
    size_t written = fwrite( ptr, size, nmemb, (FILE *)stream );
    return written;
}

string DownloadMap( string url, string mappath, string command )
{
    globalwebdata = string( );
    string testurl = UTIL_UrlFix( url );

    if( testurl.empty( ) )
        return "Указана некорректная ссылка!";

    bool directlink = true;
    string MapNumber = string( );

    if( testurl.size( ) >= 18 && testurl.size( ) <= 27 && testurl.substr( 0, 17 ) == "epicwar.com/maps/" )
    {
        MapNumber = testurl.substr( 17 );

        if( MapNumber.find_first_not_of( "0123456789" ) == string::npos )
        {
            uint32_t MapNumberInt = UTIL_ToUInt32( MapNumber );

            if( MapNumberInt > 0 && MapNumberInt < 4294967295 )
                directlink = false;
        }
    }

    CURL *curl_handle;
    CURLcode res;
    curl_global_init( CURL_GLOBAL_ALL );

    /* init the curl session */
    curl_handle = curl_easy_init( );

    if( curl_handle )
    {
        /* set URL to get here */
        curl_easy_setopt( curl_handle, CURLOPT_URL, testurl.c_str( ) );

        /* we tell libcurl to follow redirection */
        curl_easy_setopt( curl_handle, CURLOPT_FOLLOWLOCATION, 1L );

        /* disable progress meter, set to 0L to enable it */
        curl_easy_setopt( curl_handle, CURLOPT_NOPROGRESS, 1L );

        /* complete connection within 15 seconds */
        curl_easy_setopt( curl_handle, CURLOPT_CONNECTTIMEOUT, 15L );

        /* complete within 7 minutes */
        curl_easy_setopt( curl_handle, CURLOPT_TIMEOUT, 420L );

        if( directlink )
        {
            FILE *pagefile;
            string MapName = UTIL_GetMapNameFromUrl( testurl );
            bool mapnamefound = false;

            /* send all data to this function */
            curl_easy_setopt( curl_handle, CURLOPT_WRITEFUNCTION, write_data );

            if( MapName.size( ) > 4 && ( MapName.substr( MapName.size( ) - 4 ) == ".w3x" || MapName.substr( MapName.size( ) - 4 ) == ".w3m" ) )
            {
                mapnamefound = true;

                if( UTIL_FileExists( mappath + MapName ) )
                {
                    /* cleanup curl stuff */
                    curl_easy_cleanup( curl_handle );
                    curl_global_cleanup( );
                    return "Карта [" + MapName + "] уже имеется на диске, чтобы перекачать заново - удалите прежнию версию (" + command.front( ) + "delmap " + MapName + ")";
                }
            }

            string NewMapName = MapName;
            MapName = MapName + "." + UTIL_ToString( gBotID ) + "download";
            string MapPathName = mappath + MapName;

            /* open the file */
            pagefile = fopen( MapPathName.c_str( ), "wb" );

            if( pagefile )
            {
                /* write the page body to this file handle */
                curl_easy_setopt( curl_handle, CURLOPT_WRITEDATA, pagefile );

                /* Perform the request, res will get the return code */
                res = curl_easy_perform( curl_handle );

                if( res != CURLE_OK )
                {
                    string strerror;

                    if( res == CURLE_BAD_FUNCTION_ARGUMENT )
                        strerror = "Истек тайм-аут соединения.";
                    else
                        strerror = "Не удалось скачать карту. Проверьте указанную ссылку и попробуйте еще раз.";

                    /* close the header file */
                    fclose( pagefile );

                    /* cleanup curl stuff */
                    curl_easy_cleanup( curl_handle );
                    curl_global_cleanup( );
                    return strerror;
                }

                if( !mapnamefound )
                {
                    /* получим прямую ссылку на файл, чтобы определить название карты */
                    char *directurl;
                    curl_easy_getinfo( curl_handle, CURLINFO_EFFECTIVE_URL, &directurl );
                    string MapNameTest = directurl;
                    MapNameTest = UTIL_UrlFix( directurl );
                    MapNameTest = UTIL_GetMapNameFromUrl( MapNameTest );

                    if( !MapNameTest.empty( ) )
                        NewMapName = MapNameTest;
                }

                /* close the header file */
                fclose( pagefile );

                /* cleanup curl stuff */
                curl_easy_cleanup( curl_handle );
                curl_global_cleanup( );

                if( NewMapName != MapName )
                {
                    if( UTIL_FileExists( mappath + NewMapName ) )
                    {
                        if( NewMapName.size( ) > 4 && ( NewMapName.substr( NewMapName.size( ) - 4 ) == ".w3x" || NewMapName.substr( NewMapName.size( ) - 4 ) == ".w3m" ) )
                        {
                            UTIL_FileDelete( mappath + MapName );
                            return "Карта [" + NewMapName + "] уже имеется на диске, чтобы перекачать заново - удалите прежнию версию (" + command.front( ) + "delmap " + NewMapName + ")";
                        }
                        else
                        {
                            if( UTIL_FileDelete( mappath + NewMapName ) )
                            {
                                if( UTIL_FileRename( mappath + MapName, mappath + NewMapName ) )
                                    return "Скачивание карты [" + NewMapName + "] завершено!";
                                else
                                    return "Ошибка при переименовании скачанного файла [" + MapName + "]";
                            }
                            else
                            {
                                UTIL_FileDelete( mappath + MapName );
                                return "Ошибка при удалении прежней версии карты [" + NewMapName + "]";
                            }
                        }
                    }
                    else
                    {
                        if( UTIL_FileRename( mappath + MapName, mappath + NewMapName ) )
                            return "Скачивание карты [" + NewMapName + "] завершено!";
                        else
                            return "Ошибка при переименовании скачанного файла [" + MapName + "]";
                    }
                }
                else
                    return "Скачивание карты [" + NewMapName + "] завершено!";
            }
            else
            {
                /* cleanup curl stuff */
                curl_easy_cleanup( curl_handle );
                curl_global_cleanup( );
                return "Ошибка при сохранении на диск скачиваемого файла [" + MapName + "]";
            }
        }
        else
        {
            /* send all data to this function */
            curl_easy_setopt( curl_handle, CURLOPT_WRITEFUNCTION, write_data_record );

            /* Perform the request, res will get the return code */
            res = curl_easy_perform( curl_handle );

            if( res != CURLE_OK )
            {
                string strerror;

                if( res == CURLE_BAD_FUNCTION_ARGUMENT )
                    strerror = "Истек тайм-аут соединения.";
                else
                    strerror = "Не удалось скачать карту. Проверьте указанную ссылку и попробуйте еще раз.";

                /* cleanup curl stuff */
                curl_easy_cleanup( curl_handle );
                curl_global_cleanup( );
                globalwebdata = string( );
                return strerror;
            }

            if( !globalwebdata.empty( ) )
            {
                string stringpoint = "<a href=\"/maps/download/" + MapNumber + "/";
                string::size_type mapidentifierstart = globalwebdata.find( stringpoint );
                string mapidentifier = string( );

                if( mapidentifierstart != string::npos )
                {
                    globalwebdata = globalwebdata.substr( mapidentifierstart + stringpoint.size( ) );
                    string::size_type mapidentifierend = globalwebdata.find( "/" );

                    if( mapidentifierend != string::npos && mapidentifierend > 0 )
                    {
                        mapidentifier = globalwebdata.substr( 0, mapidentifierend );
                        globalwebdata = globalwebdata.substr( mapidentifierend + 1 );

                        string::size_type mapnameend = globalwebdata.find( "\"><" );

                        if( mapnameend != string::npos && mapnameend > 0 )
                            mapidentifier += "/" + globalwebdata.substr( 0, mapnameend );
                        else
                            mapidentifier = string( );
                    }
                }

                globalwebdata = string( );

                if( !mapidentifier.empty( ) )
                {
                    string newurl = "epicwar.com/maps/download/" + MapNumber + "/" + mapidentifier;

                    if( newurl != url )
                    {
                        /* cleanup curl stuff */
                        curl_easy_cleanup( curl_handle );
                        curl_global_cleanup( );
                        return DownloadMap( newurl, mappath, command );
                    }
                }
            }

            /* cleanup curl stuff */
            curl_easy_cleanup( curl_handle );
            curl_global_cleanup( );
            return "Укажите прямую ссылку для скачивания: " + command + " <URL>";
        }
    }
    else
        return "Возникла ошибка при инициализации соединения";
}

void CONSOLE_Print( string message, uint32_t toGUI, uint32_t gameID )
{
    // 0 - default
    // 1 - force to gui
    // 2 - only to gui
    // 3 - ignore gui
    // 4 - cancel if m_LogReduction is enabled
    // 5 - log backup (push_front)
    // 6 - copy to game tab in gui
    // 7 - copy to game tab in gui, but cancel if m_LogReduction is enabled

    if( gUNM != nullptr && gUNM && gUNM->m_LogReduction && ( toGUI == 4 || toGUI == 7 ) )
        return;

    string backupmessage;

    if( gUNM && gUNM != nullptr && gUNM->m_LogBackup )
        backupmessage = message;

    time_t Now = time( nullptr );
    string Time = asctime( localtime( &Now ) );
    Time.erase( Time.size( ) - 6 ); // erase the newline and year + space
    uint32_t TimeSize = Time.size( ) + 3;
    message.insert( 0, "[" + Time + "] " );

    if( toGUI != 3 && ( gEnableLoggingToGUI == 2 || ( !gUNMLoaded && gEnableLoggingToGUI == 1 ) || toGUI == 1 || toGUI == 2 ) )
    {
        if( toGUI == 6 || toGUI == 7 )
            emit gToLog->toGameLog( gameID, QString::fromUtf8( message.c_str( ) ), true );
        else
            emit gToLog->toLog( QString::fromUtf8( message.c_str( ) ) );

        if( toGUI == 2 )
            return;
    }

    if( gUNM && gUNM != nullptr && gUNM->m_LogBackup )
    {
        if( toGUI == 5 )
            gUNM->m_LogMessages.push_front( backupmessage );
        else
            gUNM->m_LogMessages.push_back( backupmessage );

        return;
    }

    // logging

    if( !gLogFile.empty( ) )
    {
        if( gLogMethod == 1 )
        {
            ofstream Log;

#ifdef WIN32
            Log.open( UTIL_UTF8ToCP1251( gLogFile.c_str( ) ), ios::app );
#else
            Log.open( gLogFile, ios::app );
#endif

            if( !Log.fail( ) )
            {
                bool found = false;

                if( message.size( ) > TimeSize && message[TimeSize] == '[' )
                {
                    uint32_t n = 1;

                    for( uint32_t i = TimeSize + 1; i < message.size( ); i++ )
                    {
                        if( message[i] == '[' )
                            found = true;
                        else if( message[i] == ']' )
                        {
                            if( found )
                            {
                                if( n >= 13 && ( message[i-1] == '0' || message[i-1] == '1' || message[i-1] == '2' || message[i-1] == '3' || message[i-1] == '4' || message[i-1] == '5' || message[i-1] == '6' || message[i-1] == '7' || message[i-1] == '8' || message[i-1] == '9' ) && ( message[i-2] == '/' || message[i-2] == '0' || message[i-2] == '1' || message[i-2] == '2' || message[i-2] == '3' || message[i-2] == '4' || message[i-2] == '5' || message[i-2] == '6' || message[i-2] == '7' || message[i-2] == '8' || message[i-2] == '9' ) && ( message[i-3] == '/' || message[i-3] == '0' || message[i-3] == '1' || message[i-3] == '2' || message[i-3] == '3' || message[i-3] == '4' || message[i-3] == '5' || message[i-3] == '6' || message[i-3] == '7' || message[i-3] == '8' || message[i-3] == '9' ) )
                                    found = false;
                                else
                                {
                                    if( n < 38 )
                                        message.insert( 1 + TimeSize, 38 - n, ' ' );

                                    break;
                                }
                            }
                            else
                            {
                                if( n < 38 )
                                    message.insert( 1 + TimeSize, 38 - n , ' ' );

                                break;
                            }
                        }

                        n++;
                    }
                }

                Log << message << endl;
                Log.close( );
            }
        }
        else if( gLogMethod == 2 )
        {
            if( gLog && !gLog->fail( ) )
            {
                bool found = false;

                if( message.size( ) > TimeSize && message[TimeSize] == '[' )
                {
                    uint32_t n = 1;

                    for( uint32_t i = TimeSize + 1; i < message.size( ); i++ )
                    {
                        if( message[i] == '[' )
                            found = true;
                        else if( message[i] == ']' )
                        {
                            if( found )
                            {
                                if( n >= 13 && ( message[i-1] == '0' || message[i-1] == '1' || message[i-1] == '2' || message[i-1] == '3' || message[i-1] == '4' || message[i-1] == '5' || message[i-1] == '6' || message[i-1] == '7' || message[i-1] == '8' || message[i-1] == '9' ) && ( message[i-2] == '/' || message[i-2] == '0' || message[i-2] == '1' || message[i-2] == '2' || message[i-2] == '3' || message[i-2] == '4' || message[i-2] == '5' || message[i-2] == '6' || message[i-2] == '7' || message[i-2] == '8' || message[i-2] == '9' ) && ( message[i-3] == '/' || message[i-3] == '0' || message[i-3] == '1' || message[i-3] == '2' || message[i-3] == '3' || message[i-3] == '4' || message[i-3] == '5' || message[i-3] == '6' || message[i-3] == '7' || message[i-3] == '8' || message[i-3] == '9' ) )
                                    found = false;
                                else
                                {
                                    if( n < 38 )
                                        message.insert( 1 + TimeSize, 38 - n, ' ' );

                                    break;
                                }
                            }
                            else
                            {
                                if( n < 38 )
                                    message.insert( 1 + TimeSize, 38 - n, ' ' );

                                break;
                            }
                        }

                        n++;
                    }
                }

                *gLog << message << endl;
                gLog->flush( );
            }
        }
    }
}

//
// main
//

int main( int argc, char *argv[] )
{
    QApplication a( argc, argv );
    Widget w;
    return a.exec( );
}

ExampleObject::ExampleObject( QObject *parent ) :
    QObject( parent ),
    m_running( 0 ),
    m_URL( string( ) ),
    m_MapsPath( string( ) ),
    m_Command( string( ) ),
    m_BnetID( 0 ),
    m_UserName( string( ) ),
    m_Whisper( false ),
    m_LogFileName( string( ) ),
    m_ThreadEnd( false )
{

}

uint32_t ExampleObject::running( ) const
{
    return m_running;
}

void ExampleObject::run( )
{
    if( m_running == 0 )
    {
        ofstream myfile;

#ifdef WIN32
        string ProcessID = UTIL_ToString( GetCurrentProcessId( ) );
        myfile.open( UTIL_UTF8ToCP1251( "process.ini" ) );
#else
        string ProcessID = UTIL_ToString( getpid( ) );
        myfile.open( "process.ini" );
#endif
        myfile << "[UNM]" << endl;
        myfile << "ProcessID = " << ProcessID << endl;
        myfile << "ProcessStatus = Online";
        myfile.close( );

        // read config file
        gCFGFile = "unm.cfg";
        CConfig CFG;
        CFG.Read( gCFGFile, 1 );
        bool GUI_RememberMe = CFG.GetInt( "gui_rememberme", 1 ) != 0;
        emit gToLog->setRememberMe( GUI_RememberMe );

        if( GUI_RememberMe )
        {
            bool GUI_RunAH = CFG.GetInt( "bnet_runiccuploader", 0 ) != 0;
            emit gToLog->setRunAH( GUI_RunAH );
            string GUI_Server = CFG.GetString( "bnet_server", string( ) );
            QString QT_Server = QString::fromUtf8( GUI_Server.c_str( ) );
            emit gToLog->setServer( QT_Server );
            string GUI_Login = CFG.GetString( "bnet_username", string( ) );
            QString QT_Login = QString::fromUtf8( GUI_Login.c_str( ) );
            emit gToLog->setLogin( QT_Login );
            string GUI_Password = CFG.GetString( "bnet_password", string( ) );
            QString QT_Password = QString::fromUtf8( GUI_Password.c_str( ) );
            emit gToLog->setPassword( QT_Password );
        }

        gLogFile = CFG.GetString( "bot_log", string( ) );
        gLogMethod = CFG.GetUInt( "bot_logmethod", 2 );
        SocketLogging = CFG.GetInt( "bot_logsocket", 0 ) != 0;
        gEnableLoggingToGUI = CFG.GetUInt( "bot_enableloggingtogui", 1 );

        if( !gLogFile.empty( ) && gLogMethod == 2 )
        {
            // log method 1: open, append, and close the log for every message
            // this works well on Linux but poorly on Windows, particularly as the log file grows in size
            // the log file can be edited/moved/deleted while UNM is running
            // log method 2: open the log on startup, flush the log for every message, close the log on shutdown
            // the log file CANNOT be edited/moved/deleted while UNM is running

            gLog = new ofstream( );
            gLog->open( gLogFile.c_str( ), ios::app );
        }

        if( gLogFile.empty( ) )
            CONSOLE_Print( "Запись логов в данном окне отключена в конфиге клиента (не задано имя лог-файла - bot_log)", 2 );
        else if( gLogMethod == 0 )
            CONSOLE_Print( "Логирование отключено в конфиге клиента (не определен метод логирования - bot_logmethod = 0)", 2 );
        else if( gEnableLoggingToGUI != 1 && gEnableLoggingToGUI != 2 )
        {
            CONSOLE_Print( "Запись логов в данном окне отключена в конфиге клиента.", 2 );
            CONSOLE_Print( "Запись логов в GUI отключена в конфиге клиента.", 3 );
        }

        CONSOLE_Print( "[UNM] starting up" );
        CONSOLE_Print( "[UNM] Process ID: " + ProcessID );

        if( !gLogFile.empty( ) )
        {
            if( gLogMethod == 1 )
                CONSOLE_Print( "[UNM] using log method 1, logging is enabled and [" + gLogFile + "] will not be locked" );
            else if( gLogMethod == 2 )
            {
                if( gLog->fail( ) )
                {
                    CONSOLE_Print( "[UNM] using log method 2 but unable to open [" + gLogFile + "] for appending, logging is disabled" );
                    gLog->close( );
                    delete gLog;
                    gLog = nullptr;
                }
                else
                    CONSOLE_Print( "[UNM] using log method 2, logging is enabled and [" + gLogFile + "] is now locked" );
            }
        }
        else
            CONSOLE_Print( "[UNM] no log file specified, logging is disabled" );

        if( !std::filesystem::exists( "text_files" ) )
        {
            if( std::filesystem::create_directory( "text_files" ) )
                CONSOLE_Print( "[UNM] Была создана папка \"text_files\"" );
            else
                CONSOLE_Print( "[UNM] Не удалось создать папку \"text_files\"" );
        }

        // catch SIGABRT and SIGINT
        signal( SIGINT, SignalCatcher );

#ifndef WIN32
        // disable SIGPIPE since some systems like OS X don't define MSG_NOSIGNAL
        signal( SIGPIPE, SIG_IGN );
#endif

#ifdef WIN32

        // initialize timer resolution
        // attempt to set the resolution as low as possible from 1ms to 5ms
        bool fail = false;

        for( uint32_t i = 1; i <= 5; i++ )
        {
            if( timeBeginPeriod( i ) == TIMERR_NOERROR )
            {
                TimerResolution = i;
                break;
            }
            else if( i < 5 )
            {
                if( fail )
                {
                    fail = false;
                    CONSOLE_Print( "[UNM] error setting Windows timer resolution to " + UTIL_ToString( i ) + " milliseconds, trying a higher resolution" );
                }
                else
                {
                    i--;
                    fail = true;
                    CONSOLE_Print( "[UNM] error setting Windows timer resolution to " + UTIL_ToString( i ) + " milliseconds, let's try again" );
                }
            }
            else
            {
                CONSOLE_Print( "[UNM] error setting Windows timer resolution" );
                emit threadReady(true);
                return;
            }
        }

        CONSOLE_Print( "[UNM] using Windows timer with resolution " + UTIL_ToString( TimerResolution ) + " milliseconds" );
#elif __APPLE__
        // not sure how to get the resolution
#else
        // print the timer resolution

        struct timespec Resolution;

        if( clock_getres( CLOCK_MONOTONIC, &Resolution ) == -1 )
            CONSOLE_Print( "[UNM] error getting monotonic timer resolution" );
        else
            CONSOLE_Print( "[UNM] using monotonic timer with resolution " + UTIL_ToString( (double)( Resolution.tv_nsec / 1000 ), 2 ) + " microseconds" );
#endif

#ifdef WIN32
        // initialize winsock

        CONSOLE_Print( "[UNM] starting winsock" );
        WSADATA wsadata;

        if( WSAStartup( MAKEWORD( 2, 2 ), &wsadata ) != 0 )
        {
            CONSOLE_Print( "[UNM] error starting winsock" );
            emit threadReady(true);
            return;
        }

        // increase process priority

        CONSOLE_Print( "[UNM] setting process priority to \"high\"" );
        SetPriorityClass( GetCurrentProcess( ), HIGH_PRIORITY_CLASS );
#endif

        // initialize unm

        gUNM = new CUNM( &CFG );
        struct tm * timeinfo;
        char buffer[150];
        string sDate = string( );
        string sDatenet = string( );
        time_t Now = time( nullptr );
        timeinfo = localtime( &Now );
        strftime( buffer, 150, "[UNM] Local Date: %d %B %Y Local Time: %H:%M:%S", timeinfo );
        sDate = buffer;
        CONSOLE_Print( sDate );
        unmLoaded = true;
        emit threadReady(true);
        return;
    }
    else if( m_running == 1 )
    {
        if( !RunnedIccupLoaders.empty( ) && GetTicks( ) - IccupLoadersLaunchTime < 1000 )
        {
            CONSOLE_Print( "[UNM] Ожидаем загрузки IccReconnectLoader.exe..." );
            std::this_thread::sleep_for( std::chrono::milliseconds( 1000 - ( GetTicks( ) - IccupLoadersLaunchTime ) ) );
        }

        if( !gLogFile.empty( ) && gLogMethod != 0 && gEnableLoggingToGUI == 1 )
        {
            CONSOLE_Print( "[UNM] Дальнейшая запись логов в данном окне отключена в конфиге клиента.", 2 );
            CONSOLE_Print( "[UNM] Дальнейшая запись логов в GUI отключена в конфиге клиента.", 3 );
        }

        gUNMLoaded = true;

        while( true )
        {
            // block for 50ms on all sockets - if you intend to perform any timed actions more frequently you should change this
            // that said it's likely we'll loop more often than this due to there being data waiting on one of the sockets but there aren't any guarantees

            if( gUNM->Update( 50000 ) )
                break;
        }

        emit finishedUNM( true );
        return;
    }
    else if( m_running == 2 || m_running == 3 )
    {
        if( m_running == 3 && DelayedRunIccupLoaders )
            RunIccupLoaders( );

        emit threadReady(false);
        return;
    }
    else if( m_running == 4 )
    {
        vector<string> AllLinesCFG;
        string Line;
        ifstream in;

#ifdef WIN32
        in.open( UTIL_UTF8ToCP1251( gCFGFile.c_str( ) ) );
#else
        in.open( gCFGFile );
#endif

        bool emptycheck = false;
        bool findServer = false;
        bool findLogin = false;
        bool findPassword = false;
        bool findRunAH = false;
        bool findRememberMe = false;
        int32_t lineNumberFirstAnyParameterFound = -1;
        int32_t lineNumberBnetPortParameter = -1;
        int32_t totalLines = 0;

        if( !in.fail( ) )
        {
            while( !in.eof( ) )
            {
                getline( in, Line );
                Line = UTIL_FixFileLine( Line );

                if( lineNumberFirstAnyParameterFound == -1 && Line.find_first_not_of( " " ) != string::npos && Line[0] != '#' )
                    lineNumberFirstAnyParameterFound = totalLines;

                if( !emptycheck && Line.find_first_not_of( " " ) != string::npos && Line[0] != '#' )
                    emptycheck = true;

                if( Line.size( ) >= 9 && Line.substr( 0, 9 ) == "bnet_port" )
                    lineNumberBnetPortParameter = totalLines + 1;

                if( Line.size( ) >= 13 && Line.substr( 0, 11 ) == "bnet_server" && LogonDataRememberMe )
                {
                    findServer = true;
                    AllLinesCFG.push_back( "bnet_server = " + LogonDataServer );
                }
                else if( Line.size( ) >= 13 && Line.substr( 0, 13 ) == "bnet_username" && LogonDataRememberMe )
                {
                    findLogin = true;
                    AllLinesCFG.push_back( "bnet_username = " + LogonDataLogin );
                }
                else if( Line.size( ) >= 13 && Line.substr( 0, 13 ) == "bnet_password" && LogonDataRememberMe )
                {
                    findPassword = true;
                    AllLinesCFG.push_back( "bnet_password = " + LogonDataPassword );
                }
                else if( Line.size( ) >= 19 && Line.substr( 0, 19 ) == "bnet_runiccuploader" && LogonDataRememberMe )
                {
                    findRunAH = true;
                    AllLinesCFG.push_back( LogonDataRunAH ? "bnet_runiccuploader = 1" : "bnet_runiccuploader = 0" );
                }
                else if( Line.size( ) >= 14 && Line.substr( 0, 14 ) == "gui_rememberme" )
                {
                    findRememberMe = true;
                    AllLinesCFG.push_back( LogonDataRememberMe ? "gui_rememberme = 1" : "gui_rememberme = 0" );
                }
                else
                    AllLinesCFG.push_back( Line );

                totalLines++;
            }
        }

        in.close( );

        if( emptycheck )
        {
            if( !findRememberMe )
                AllLinesCFG.insert( AllLinesCFG.begin( ) + lineNumberFirstAnyParameterFound, LogonDataRememberMe ? "gui_rememberme = 1" : "gui_rememberme = 0" );

            if( LogonDataRememberMe )
            {
                if( !findRunAH )
                {
                    if( lineNumberBnetPortParameter == -1 )
                        AllLinesCFG.insert( AllLinesCFG.begin( ) + lineNumberFirstAnyParameterFound, LogonDataRunAH ? "bnet_runiccuploader = 1" : "bnet_runiccuploader = 0" );
                    else
                        AllLinesCFG.insert( AllLinesCFG.begin( ) + lineNumberBnetPortParameter, LogonDataRunAH ? "bnet_runiccuploader = 1" : "bnet_runiccuploader = 0" );
                }

                if( !findServer )
                    AllLinesCFG.insert( AllLinesCFG.begin( ) + lineNumberFirstAnyParameterFound, "bnet_server = " + LogonDataServer );

                if( !findLogin )
                    AllLinesCFG.insert( AllLinesCFG.begin( ) + lineNumberFirstAnyParameterFound, "bnet_username = " + LogonDataLogin );

                if( !findPassword )
                    AllLinesCFG.insert( AllLinesCFG.begin( ) + lineNumberFirstAnyParameterFound, "bnet_password = " + LogonDataPassword );
            }

            while( !AllLinesCFG.empty( ) && AllLinesCFG.back( ).empty( ) )
                AllLinesCFG.erase( AllLinesCFG.end( ) - 1 );

            ofstream in2;
            in2.open( gCFGFile.c_str( ), std::ofstream::out | std::ofstream::trunc );

            for( uint32_t i = 0; i != AllLinesCFG.size( ); i++ )
                in2 << AllLinesCFG[i] << endl;

            in2.close( );
            CONSOLE_Print( "[GUI] связка логин/пароль была сохранена в конфигурационный файл [" + gCFGFile + "]" );
        }
        else
            CONSOLE_Print( "[GUI] не удалось сохранить связку логин/пароль в конфигурационный файл [" + gCFGFile + "]" );

        AllLinesCFG.clear( );
        emit finished( );
        return;
    }
    else if( m_running == 5 )
    {
        emit threadReady( true );
        return;
    }
    else if( m_running == 6 || m_running == 7 )
    {
        if( m_running == 6 )
        {
            if( gUNM && gUNM != nullptr && gUNM->m_BNETs.size( ) > m_BnetID )
            {
                string result = DownloadMap( m_URL, m_MapsPath, m_Command );
                gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandWhisperQueue.push_back( m_Whisper );
                gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandUserQueue.push_back( m_UserName );
                gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandQueue.push_back( result );
            }

            gAltThreadStateMSG = string( );
        }
        else
        {
            if( gLogMethod == 2 )
            {
                gLog->close( );
                delete gLog;
                gLog = nullptr;
            }

            if( UTIL_FileRename( gLogFile, m_LogFileName ) )
                gAltThreadStateMSG = "[UNM] бэкап лог файла [" + m_LogFileName + "] прошел успешно";
            else
                gAltThreadStateMSG = "[UNM] ошибка при бэкапе лог файла [" + m_LogFileName + "]";
        }

        m_URL = string( );
        m_MapsPath = string( );
        m_Command = string( );
        m_BnetID = 0;
        m_UserName = string( );
        m_Whisper = false;
        m_LogFileName = string( );

        if( gAltThreadState == "@FinishedUNM" )
        {
            gAltThreadStateMSG = string( );
            emit finishedUNM( false );
        }
        else
            emit finished2( );

        return;
    }
}

void ExampleObject::setRunning( uint32_t running )
{
    if( m_running == running )
        return;

    m_running = running;
    emit runningChanged( running );
}

void ExampleObject::setThreadEnd( bool ThreadEnd )
{
    m_ThreadEnd = ThreadEnd;
}

void ExampleObject::setOptions( QList<QString> options )
{
    m_URL = options[0].toStdString( );
    m_MapsPath = options[1].toStdString( );
    m_Command = options[2].toStdString( );
    m_BnetID = UTIL_QSToUInt32( options[3] );
    m_UserName = options[4].toStdString( );

    if( options[5] == "0" )
        m_Whisper = false;
    else
        m_Whisper = true;
}

void ExampleObject::setLogFileName( QString logfilename )
{
    m_LogFileName = logfilename.toStdString( );
}

Widget::Widget( QWidget *parent ) :
    QWidget( parent ),
    ui( new Ui::Widget ),
    ready( false ),
    m_IgnoreChangeStyle( false ),
    m_StyleDefault( "QAbstractSpinBox{ qproperty-alignment: 'AlignVCenter | AlignLeft'; } QWidget{ font-weight: normal; }" ),
    m_ScrollAreaStyleDefault( " QScrollArea { background: transparent; } QScrollArea>QWidget>QWidget { background: transparent; } QScrollArea>QWidget>QScrollBar { background: palette(base); }" ),
    m_ScrollBarStyleDefault( "QScrollBar:horizontal { max-height: 20px; margin: 0px 20px 0px 20px; background:#f0f0f0; } QScrollBar:vertical { max-width: 20px; margin: 20px 0px 20px 0px; background:#f0f0f0; } QScrollBar::handle:horizontal { background:#cdcdcd; min-width: 25px; } QScrollBar::handle:horizontal:hover { background:#a7a7a7; min-width: 25px; } QScrollBar::handle:horizontal:pressed { background:#646464; min-width: 25px; } QScrollBar::handle:vertical { background: #cdcdcd; min-height: 25px; } QScrollBar::handle:vertical:hover { background: #a7a7a7; min-height: 25px; } QScrollBar::handle:vertical:pressed { background: #646464; min-height: 25px; } QScrollBar::add-line:horizontal { background: #f0f0f0; width: 20px; subcontrol-position: right; subcontrol-origin: margin; } QScrollBar::add-line:horizontal:hover { background: #dadada; width: 20px; subcontrol-position: right; subcontrol-origin: margin; } QScrollBar::add-line:horizontal:pressed { background: #656565; width: 20px; subcontrol-position: right; subcontrol-origin: margin; } QScrollBar::add-line:vertical { background: #f0f0f0; height: 20px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::add-line:vertical:hover { background: #dadada; height: 20px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::add-line:vertical:pressed { background: #656565; height: 20px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::sub-line:horizontal { background: #f0f0f0; width: 20px; subcontrol-position: left; subcontrol-origin: margin; } QScrollBar::sub-line:horizontal:hover { background: #dadada; width: 20px; subcontrol-position: left; subcontrol-origin: margin; } QScrollBar::sub-line:horizontal:pressed { background: #656565; width: 20px; subcontrol-position: left; subcontrol-origin: margin; } QScrollBar::sub-line:vertical { background: #f0f0f0; height: 20px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar::sub-line:vertical:hover { background: #dadada; height: 20px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar::sub-line:vertical:pressed { background: #656565; height: 20px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar::left-arrow:horizontal { width: 6px; height: 7px; border-image: url(:/window/left_arrow_gray.png); } QScrollBar::right-arrow:horizontal { width: 6px; height: 7px; border-image: url(:/window/right_arrow_gray.png); } QScrollBar::up-arrow:vertical { width: 7px; height: 6px; border-image: url(:/window/up_arrow_gray.png); } QScrollBar::down-arrow:vertical { width: 7px; height: 6px; border-image: url(:/window/down_arrow_gray.png); } QScrollBar::left-arrow:horizontal:hover { width: 6px; height: 7px; border-image: url(:/window/left_arrow_black.png); } QScrollBar::right-arrow:horizontal:hover { width: 6px; height: 7px; border-image: url(:/window/right_arrow_black.png); } QScrollBar::up-arrow:vertical:hover { width: 7px; height: 6px; border-image: url(:/window/up_arrow_black.png); } QScrollBar::down-arrow:vertical:hover { width: 7px; height: 6px; border-image: url(:/window/down_arrow_black.png); } QScrollBar::left-arrow:horizontal:pressed { width: 6px; height: 7px; border-image: url(:/window/left_arrow_white.png); } QScrollBar::right-arrow:horizontal:pressed { width: 6px; height: 7px; border-image: url(:/window/right_arrow_white.png); } QScrollBar::up-arrow:vertical:pressed { width: 7px; height: 6px; border-image: url(:/window/up_arrow_white.png); } QScrollBar::down-arrow:vertical:pressed { width: 7px; height: 6px; border-image: url(:/window/down_arrow_white.png); } QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal, QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; } " ),
    m_leftMouseButtonPressed( None ),
    m_CornerLabel( nullptr ),
    m_CornerLabelHaveImage( true ),
    m_CurrentPossSearch( -1 ),
    m_GameTabColor( 0 ),
    m_WaitForKey( 0 ),
    LShift( false ),
    LCtrl( false ),
    LAlt( false ),
    RShift( false ),
    RCtrl( false ),
    RAlt( false )
{
    CConfig CFGStyles;
    CConfig CFGMain;

#ifdef WIN32
    CFGStyles.Read( "styles\\config.cfg" );
    CFGMain.Read( "unm.cfg", 0 );
#else
    CFGStyles.Read( "styles/config.cfg" );
    CFGMain.Read( "unm.cfg", 0 );
#endif

    ChangeStyleGUI( QString::fromUtf8( CFGStyles.GetString( "currentstyle", "default" ).c_str( ) ) );
    sWindow = new AnotherWindow( );
    connect( sWindow, &AnotherWindow::firstWindow, this, &Widget::show_main_window );
    connect( sWindow, &AnotherWindow::runThread, this, &Widget::run_thread );
    connect( sWindow, &AnotherWindow::saveLogData, this, &Widget::save_log_data );
    sWindow->show( );
    qRegisterMetaType<QList<QString>>( "QList<QString>" );
    qRegisterMetaType<QList<QStringList>>( "QList<QStringList>" );
    qRegisterMetaType<QList<unsigned int>>( "QList<unsigned int>" );
    gLanguageFile = UTIL_FixFilePath( CFGMain.GetString( "bot_language", "text_files\\Languages\\Russian.cfg" ) );
    gOAuthDiscordToken = QString::fromUtf8( CFGMain.GetString( "discord_token", string( ) ).c_str( ) );
    gLanguage = new CLanguage( gLanguageFile );
    ui->setupUi( this );
    m_SimplifiedStyle = CFGStyles.GetInt( "simplified_style", 0 ) != 0;

    if( !m_SimplifiedStyle )
    {
        this->layout( )->setMargin( 9 );
        this->setWindowFlags( Qt::FramelessWindowHint );      // Отключаем оформление окна
        this->setAttribute( Qt::WA_TranslucentBackground );   // Делаем фон главного виджета прозрачным

        // Создаём эффект тени

        QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect( this );
        shadowEffect->setBlurRadius( 9 ); // Устанавливаем радиус размытия
        shadowEffect->setOffset( 0 );     // Устанавливаем смещение тени
        ui->CentralWidget->setGraphicsEffect( shadowEffect );   // Устанавливаем эффект тени на окно
        ui->btn_minimize->setStyleSheet( "QToolButton{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_minimize_gray.png) } QToolButton:hover{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_minimize_white.png) } QToolButton:pressed{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_minimize_orange.png) }" );
        ui->btn_maximize->setStyleSheet( "QToolButton{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_maximize_gray.png) } QToolButton:hover{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_maximize_white.png) } QToolButton:pressed{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_maximize_orange.png) }" );
        ui->btn_close->setStyleSheet( "QToolButton{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/close_gray.png) } QToolButton:hover{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/close_white.png) } QToolButton:pressed{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/close_orange.png) }" );

        // коннекты для кнопок сворачивания/максимизации/минимизации/закрытия
        // Сворачивание окна приложения в панель задач

        connect( ui->btn_minimize, &QToolButton::clicked, this, &QWidget::showMinimized );
        connect( ui->btn_maximize, &QToolButton::clicked, [this]( )
        {
            // При нажатии на кнопку максимизации/нормализации окна
            // Делаем проверку на то, в каком состоянии находится окно и переключаем его режим

            if( this->isMaximized( ) )
            {
                // Заметьте, каждый раз устанавливаем новый стиль в эту кнопку

                ui->btn_maximize->setStyleSheet( "QToolButton{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_maximize_gray.png) } QToolButton:hover{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_maximize_white.png) } QToolButton:pressed{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_maximize_orange.png) }" );
                this->layout( )->setMargin( 9 );
                this->showNormal( );
            }
            else
            {
                ui->btn_maximize->setStyleSheet( "QToolButton{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_restore_gray.png) } QToolButton:hover{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_restore_white.png) } QToolButton:pressed{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_restore_orange.png) }" );
                this->layout( )->setMargin( 0 );
                this->showMaximized( );
            }
        });

        // Закрытие окна приложения

        connect( ui->btn_close, &QToolButton::clicked, this, &QWidget::close );
    }
    else
    {
        ui->label_title->hide( );
        ui->btn_minimize->hide( );
        ui->btn_maximize->hide( );
        ui->btn_close->hide( );
    }

    RefreshStylesList( QString::fromUtf8( CFGStyles.GetString( "currentstyle", "default" ).c_str( ) ), true );
    QListFillingProcess( );
    CommandListFilling( );
    m_CornerLabel = new QLabel( this );
    m_CornerLabel->setObjectName( "CornerLabel" );
    m_CornerLabel->setTextInteractionFlags( Qt::NoTextInteraction );
    m_CornerLabel->setMouseTracking( true );
    m_CornerLabel->setAlignment( Qt::AlignBottom | Qt::AlignHCenter );
    m_CornerLabel->setTextFormat( Qt::RichText );
    m_CornerLabel->setText( "<img src=\":/mainwindow/unm.ico\" width=\"48\" height=\"48\"><br><font size=\"3\" face=\"Calibri\">unm client<br>v1.2.10</font>" );
    qApp->installEventFilter( this );
    connect( &thread_1, &QThread::started, &exampleObject_1, &ExampleObject::run );
    connect( &thread_2, &QThread::started, &exampleObject_2, &ExampleObject::run );
    connect( &thread_3, &QThread::started, &exampleObject_3, &ExampleObject::run );
    connect( &exampleObject_1, &ExampleObject::finished, &thread_1, &QThread::quit );
    connect( &exampleObject_2, &ExampleObject::finished, &thread_2, &QThread::quit );
    connect( &exampleObject_1, &ExampleObject::finishedUNM, this, &Widget::finished_unm );
    connect( &exampleObject_2, &ExampleObject::finishedUNM, this, &Widget::finished_unm );
    connect( &exampleObject_3, &ExampleObject::finishedUNM, this, &Widget::finished_unm );
    connect( &exampleObject_3, &ExampleObject::finished2, this, &Widget::ThirdThreadFinished );
    connect( gToLog, &ExampleObject::downloadMap, this, &Widget::download_map );
    connect( gToLog, &ExampleObject::restartUNM, this, &Widget::restart_UNM );
    connect( gToLog, &ExampleObject::backupLog, this, &Widget::backup_log );
    connect( gToLog, &ExampleObject::toLog, this, &Widget::write_to_log );
    connect( gToLog, &ExampleObject::toGameLog, this, &Widget::write_to_game_log );
    connect( gToLog, &ExampleObject::toChat, this, &Widget::write_to_chat );
    connect( gToLog, &ExampleObject::addChannelUser, this, &Widget::add_channel_user );
    connect( gToLog, &ExampleObject::removeChannelUser, this, &Widget::remove_channel_user );
    connect( gToLog, &ExampleObject::clearChannelUser, this, &Widget::clear_channel_user );
    connect( gToLog, &ExampleObject::bnetDisconnect, this, &Widget::bnet_disconnect );
    connect( gToLog, &ExampleObject::bnetReconnect, this, &Widget::bnet_reconnect );
    connect( gToLog, &ExampleObject::setCommandsTips, this, &Widget::set_commands_tips );
    connect( gToLog, &ExampleObject::UpdateValuesInGUISignal, this, &Widget::UpdateValuesInGUI );
    connect( gToLog, &ExampleObject::setServer, sWindow, &AnotherWindow::set_server );
    connect( gToLog, &ExampleObject::setLogin, sWindow, &AnotherWindow::set_login );
    connect( gToLog, &ExampleObject::setPassword, sWindow, &AnotherWindow::set_password );
    connect( gToLog, &ExampleObject::setRunAH, sWindow, &AnotherWindow::set_run_ah );
    connect( gToLog, &ExampleObject::setRememberMe, sWindow, &AnotherWindow::set_remember_me );
    connect( gToLog, &ExampleObject::logonSuccessful, sWindow, &AnotherWindow::logon_successful );
    connect( gToLog, &ExampleObject::logonFailed, sWindow, &AnotherWindow::logon_failed );
    connect( gToLog, &ExampleObject::addChatTab, this, &Widget::add_chat_tab );
    connect( gToLog, &ExampleObject::changeBnetUsername, this, &Widget::change_bnet_username );
    connect( gToLog, &ExampleObject::addGame, this, &Widget::add_game );
    connect( gToLog, &ExampleObject::deleteGame, this, &Widget::delete_game );
    connect( gToLog, &ExampleObject::updateGame, this, &Widget::update_game );
    connect( gToLog, &ExampleObject::updateGameMapInfo, this, &Widget::update_game_map_info );
    connect( gToLog, &ExampleObject::toGameChat, this, &Widget::write_to_game_chat );
    connect( gToLog, &ExampleObject::addBnetBot, this, &Widget::add_bnet_bot );
    connect( gToLog, &ExampleObject::updateBnetBot, this, &Widget::update_bnet_bot );
    connect( gToLog, &ExampleObject::deleteBnetBot, this, &Widget::delete_bnet_bot );
    connect( gToLog, &ExampleObject::addGameToList, this, &Widget::add_game_to_list );
    connect( gToLog, &ExampleObject::updateGameToList, this, &Widget::update_game_to_list );
    connect( gToLog, &ExampleObject::deleteGameFromList, this, &Widget::delete_game_from_list );
    connect( gToLog, &ExampleObject::deleteOldMessages, this, &Widget::delete_old_messages );
    connect( gToLog, &ExampleObject::gameListTablePosFix, this, &Widget::game_list_table_pos_fix );
    connect( &exampleObject_1, &ExampleObject::threadReady, this, &Widget::thread_ready );
    connect( &exampleObject_2, &ExampleObject::threadReady, this, &Widget::thread_ready );
    exampleObject_1.moveToThread( &thread_1 );
    exampleObject_2.moveToThread( &thread_2 );
    exampleObject_3.moveToThread( &thread_3 );
    exampleObject_1.setRunning( 0 );
    thread_1.start( );
    connect( ui->Spell01, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell01ClickSlot );
    connect( ui->Spell02, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell02ClickSlot );
    connect( ui->Spell03, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell03ClickSlot );
    connect( ui->Spell04, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell04ClickSlot );
    connect( ui->Spell05, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell05ClickSlot );
    connect( ui->Spell06, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell06ClickSlot );
    connect( ui->Spell07, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell07ClickSlot );
    connect( ui->Spell08, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell08ClickSlot );
    connect( ui->Spell09, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell09ClickSlot );
    connect( ui->Spell10, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell10ClickSlot );
    connect( ui->Spell11, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell11ClickSlot );
    connect( ui->Spell12, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Spell12ClickSlot );
    connect( ui->SpellBook01, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook01ClickSlot );
    connect( ui->SpellBook02, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook02ClickSlot );
    connect( ui->SpellBook03, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook03ClickSlot );
    connect( ui->SpellBook04, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook04ClickSlot );
    connect( ui->SpellBook05, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook05ClickSlot );
    connect( ui->SpellBook06, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook06ClickSlot );
    connect( ui->SpellBook07, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook07ClickSlot );
    connect( ui->SpellBook08, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook08ClickSlot );
    connect( ui->SpellBook09, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook09ClickSlot );
    connect( ui->SpellBook10, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook10ClickSlot );
    connect( ui->SpellBook11, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook11ClickSlot );
    connect( ui->SpellBook12, &CustomDynamicFontSizeLabel::clicked, this, &Widget::SpellBook12ClickSlot );
    connect( ui->Shop01, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop01ClickSlot );
    connect( ui->Shop02, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop02ClickSlot );
    connect( ui->Shop03, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop03ClickSlot );
    connect( ui->Shop04, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop04ClickSlot );
    connect( ui->Shop05, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop05ClickSlot );
    connect( ui->Shop06, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop06ClickSlot );
    connect( ui->Shop07, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop07ClickSlot );
    connect( ui->Shop08, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop08ClickSlot );
    connect( ui->Shop09, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop09ClickSlot );
    connect( ui->Shop10, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop10ClickSlot );
    connect( ui->Shop11, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop11ClickSlot );
    connect( ui->Shop12, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Shop12ClickSlot );
    connect( ui->Item1, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Item1ClickSlot );
    connect( ui->Item2, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Item2ClickSlot );
    connect( ui->Item3, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Item3ClickSlot );
    connect( ui->Item4, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Item4ClickSlot );
    connect( ui->Item5, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Item5ClickSlot );
    connect( ui->Item6, &CustomDynamicFontSizeLabel::clicked, this, &Widget::Item6ClickSlot );
    HotKeyWidgetsList.push_back( ui->Spell01 ); HotKeyWidgetsList.push_back( ui->Spell02 ); HotKeyWidgetsList.push_back( ui->Spell03 ); HotKeyWidgetsList.push_back( ui->Spell04 ); HotKeyWidgetsList.push_back( ui->Spell05 ); HotKeyWidgetsList.push_back( ui->Spell06 ); HotKeyWidgetsList.push_back( ui->Spell07 ); HotKeyWidgetsList.push_back( ui->Spell08 ); HotKeyWidgetsList.push_back( ui->Spell09 ); HotKeyWidgetsList.push_back( ui->Spell10 ); HotKeyWidgetsList.push_back( ui->Spell11 ); HotKeyWidgetsList.push_back( ui->Spell12 ); HotKeyWidgetsList.push_back( ui->SpellBook01 ); HotKeyWidgetsList.push_back( ui->SpellBook02 ); HotKeyWidgetsList.push_back( ui->SpellBook03 ); HotKeyWidgetsList.push_back( ui->SpellBook04 ); HotKeyWidgetsList.push_back( ui->SpellBook05 ); HotKeyWidgetsList.push_back( ui->SpellBook06 ); HotKeyWidgetsList.push_back( ui->SpellBook07 ); HotKeyWidgetsList.push_back( ui->SpellBook08 ); HotKeyWidgetsList.push_back( ui->SpellBook09 ); HotKeyWidgetsList.push_back( ui->SpellBook10 ); HotKeyWidgetsList.push_back( ui->SpellBook11 ); HotKeyWidgetsList.push_back( ui->SpellBook12 ); HotKeyWidgetsList.push_back( ui->Shop01 ); HotKeyWidgetsList.push_back( ui->Shop02 ); HotKeyWidgetsList.push_back( ui->Shop03 ); HotKeyWidgetsList.push_back( ui->Shop04 ); HotKeyWidgetsList.push_back( ui->Shop05 ); HotKeyWidgetsList.push_back( ui->Shop06 ); HotKeyWidgetsList.push_back( ui->Shop07 ); HotKeyWidgetsList.push_back( ui->Shop08 ); HotKeyWidgetsList.push_back( ui->Shop09 ); HotKeyWidgetsList.push_back( ui->Shop10 ); HotKeyWidgetsList.push_back( ui->Shop11 ); HotKeyWidgetsList.push_back( ui->Shop12 ); HotKeyWidgetsList.push_back( ui->Item1 ); HotKeyWidgetsList.push_back( ui->Item2 ); HotKeyWidgetsList.push_back( ui->Item3 ); HotKeyWidgetsList.push_back( ui->Item4 ); HotKeyWidgetsList.push_back( ui->Item5 ); HotKeyWidgetsList.push_back( ui->Item6 );
    HotKeyCvarList.push_back( "A_X0Y0" ); HotKeyCvarList.push_back( "A_X1Y0" ); HotKeyCvarList.push_back( "A_X2Y0" ); HotKeyCvarList.push_back( "A_X3Y0" ); HotKeyCvarList.push_back( "A_X0Y1" ); HotKeyCvarList.push_back( "A_X1Y1" ); HotKeyCvarList.push_back( "A_X2Y1" ); HotKeyCvarList.push_back( "A_X3Y1" ); HotKeyCvarList.push_back( "A_X0Y2" ); HotKeyCvarList.push_back( "A_X1Y2" ); HotKeyCvarList.push_back( "A_X2Y2" ); HotKeyCvarList.push_back( "A_X3Y2" ); HotKeyCvarList.push_back( "SA_X0Y0" ); HotKeyCvarList.push_back( "SA_X1Y0" ); HotKeyCvarList.push_back( "SA_X2Y0" ); HotKeyCvarList.push_back( "SA_X3Y0" ); HotKeyCvarList.push_back( "SA_X0Y1" ); HotKeyCvarList.push_back( "SA_X1Y1" ); HotKeyCvarList.push_back( "SA_X2Y1" ); HotKeyCvarList.push_back( "SA_X3Y1" ); HotKeyCvarList.push_back( "SA_X0Y2" ); HotKeyCvarList.push_back( "SA_X1Y2" ); HotKeyCvarList.push_back( "SA_X2Y2" ); HotKeyCvarList.push_back( "SA_X3Y2" ); HotKeyCvarList.push_back( "NS_X0Y0" ); HotKeyCvarList.push_back( "NS_X1Y0" ); HotKeyCvarList.push_back( "NS_X2Y0" ); HotKeyCvarList.push_back( "NS_X3Y0" ); HotKeyCvarList.push_back( "NS_X0Y1" ); HotKeyCvarList.push_back( "NS_X1Y1" ); HotKeyCvarList.push_back( "NS_X2Y1" ); HotKeyCvarList.push_back( "NS_X3Y1" ); HotKeyCvarList.push_back( "NS_X0Y2" ); HotKeyCvarList.push_back( "NS_X1Y2" ); HotKeyCvarList.push_back( "NS_X2Y2" ); HotKeyCvarList.push_back( "NS_X3Y2" ); HotKeyCvarList.push_back( "I_Num7" ); HotKeyCvarList.push_back( "I_Num8" ); HotKeyCvarList.push_back( "I_Num4" ); HotKeyCvarList.push_back( "I_Num5" ); HotKeyCvarList.push_back( "I_Num1" ); HotKeyCvarList.push_back( "I_Num2" );
    m_DarkenedWidget = new QLabel( this );
    m_DarkenedWidget->setObjectName( "DarkenedWidget" );
    m_DarkenedWidget->setWordWrap( true );
    m_DarkenedWidget->setAlignment( Qt::AlignVCenter | Qt::AlignHCenter );
    m_DarkenedWidget->setGeometry( 0, 0, width( ), height( ) );
    m_DarkenedWidget->hide( );
    m_DarkenedWidget->setStyleSheet( "background-color: rgba(0,0,0,70%); color: white; font-weight: bold; font-size: 18px;" );
    m_DarkenedWidget->setText( "А теперь нажмите клавишу на клавиатуре для назначения хоткея.\r\nТакже можно использовать комбинацию клавиш, используя следующие модификаторы: SHIFT, ALT, CTRL. Кликните левой кнопкой мышки чтобы убрать хоткей, правой - для отмены." );
    WFEWriteCFG( true );
}

void Widget::WFEWriteCFG( bool first )
{
#ifdef WIN32
    m_WFECFG.Read( "wfe\\WFEConfig.ini", first );
#else
    m_WFECFG.Read( "wfe/WFEConfig.ini", first );
#endif

    if( first )
    {
        for( uint32_t i = 0; i < HotKeyWidgetsList.size( ); i++ )
        {
            HotKeyWidgetsList[i]->setText( QString::fromUtf8( m_WFECFG.GetString( "UNM_KEYBINDS", HotKeyCvarList[i], m_WFECFG.GetString( "KEYBINDS", HotKeyCvarList[i], string( ) ) ).c_str( ) ) );

            if( m_WFECFG.GetString( "UNM_SMARTCAST", HotKeyCvarList[i], m_WFECFG.GetString( "SMARTCAST", HotKeyCvarList[i], "no" ) ) == "yes" )
                HotKeyWidgetsList[i]->SetGreenBorder( true );
        }

        if( m_WFECFG.GetString( "UNM_KEYBINDS", "HOTKEYS", "yes" ) == "no" )
            ui->HotKeysBox->setChecked( false );

        if( m_WFECFG.GetString( "UNM_KEYBINDS", "SPELLS", "yes" ) == "no" )
            ui->SpellsBox->setChecked( false );

        if( m_WFECFG.GetString( "UNM_KEYBINDS", "SPELBOOK", "yes" ) == "no" )
            ui->SpellBookBox->setChecked( false );

        if( m_WFECFG.GetString( "UNM_KEYBINDS", "SHOP", "yes" ) == "no" )
            ui->ShopBox->setChecked( false );

        if( m_WFECFG.GetString( "UNM_KEYBINDS", "INVENTORY", "yes" ) == "no" )
            ui->ItemsBox->setChecked( false );
    }

    // writing new file

    string file;
    ofstream newfile;

#ifdef WIN32
    file = "wfe\\WFEConfig.ini";
    newfile.open( UTIL_UTF8ToCP1251( file.c_str( ) ), ios::trunc );
#else
    file = "wfe/WFEConfig.ini";
    newfile.open( file, ios::trunc );
#endif

    string Section = "FUNCTIONS";
    string SectionUNM = "UNM_KEYBINDS";
    string SectionUNMSmart = "UNM_SMARTCAST";
    newfile << "[FUNCTIONS]" << endl;
    newfile << "LANGUAGE = " << m_WFECFG.GetString( Section, "LANGUAGE", "RU" ) << endl;
    newfile << "ISHIDETOTRAY = " << m_WFECFG.GetString( Section, "ISHIDETOTRAY", "no" ) << endl;
    newfile << "FPSLIMIT = " << m_WFECFG.GetString( Section, "FPSLIMIT", "64" ) << endl;
    newfile << "FPSTYPE = " << m_WFECFG.GetString( Section, "FPSTYPE", "Off" ) << endl;
    newfile << "ENFORCEHOTKEYS = " << m_WFECFG.GetString( Section, "ENFORCEHOTKEYS", "no" ) << endl;
    newfile << "WIDESCREEN = " << m_WFECFG.GetString( Section, "WIDESCREEN", "no" ) << endl;
    newfile << "MOUSELOCK = " << m_WFECFG.GetString( Section, "MOUSELOCK", "no" ) << endl;
    newfile << "SINGLEPLAYERPAUSE = " << m_WFECFG.GetString( Section, "SINGLEPLAYERPAUSE", "no" ) << endl;
    newfile << "BLPLIMITREMOVAL = " << m_WFECFG.GetString( Section, "BLPLIMITREMOVAL", "no" ) << endl;
    newfile << "AUTOCAST = " << m_WFECFG.GetString( Section, "AUTOCAST", "no" ) << endl;
    newfile << "AUTOCASTMOUSEDELAY = " << m_WFECFG.GetString( Section, "AUTOCASTMOUSEDELAY", "20" ) << endl;
    newfile << "INSTANTRMC = " << m_WFECFG.GetString( Section, "INSTANTRMC", "no" ) << endl;
    newfile << "HPREGEN = " << m_WFECFG.GetString( Section, "HPREGEN", "no" ) << endl;
    newfile << "MPREGEN = " << m_WFECFG.GetString( Section, "MPREGEN", "no" ) << endl;
    newfile << "CUSTOMATTACKTIP = " << m_WFECFG.GetString( Section, "CUSTOMATTACKTIP", "no" ) << endl;
    newfile << "EXTENDEDATTACKTIP = " << m_WFECFG.GetString( Section, "EXTENDEDATTACKTIP", "no" ) << endl;
    newfile << "CUSTOMDEFENSETIP = " << m_WFECFG.GetString( Section, "CUSTOMDEFENSETIP", "no" ) << endl;
    newfile << "EXTENDEDDEFENSETIP = " << m_WFECFG.GetString( Section, "EXTENDEDDEFENSETIP", "no" ) << endl;
    newfile << "WRITEMAGICRESIST = " << m_WFECFG.GetString( Section, "WRITEMAGICRESIST", "no" ) << endl;
    newfile << "REMOVEMAPSIZELIMIT = " << m_WFECFG.GetString( Section, "REMOVEMAPSIZELIMIT", "yes" ) << endl;
    newfile << "LEGACYCONFIG = " << m_WFECFG.GetString( Section, "LEGACYCONFIG", "no" ) << endl;
    newfile << "LOADINGNOTIFICATION = " << m_WFECFG.GetString( Section, "LOADINGNOTIFICATION", "no" ) << endl;
    newfile << endl;
    Section = "COOLDOWNUI";
    newfile << "[COOLDOWNUI]" << endl;
    newfile << "ISENABLED = " << m_WFECFG.GetString( Section, "ISENABLED", "no" ) << endl;
    newfile << "SHOWINDICATOR = " << m_WFECFG.GetString( Section, "SHOWINDICATOR", "On" ) << endl;
    newfile << "DISPLAYMINUTES = " << m_WFECFG.GetString( Section, "DISPLAYMINUTES", "Off" ) << endl;
    newfile << "REFRESHTIME = " << m_WFECFG.GetString( Section, "REFRESHTIME", "100" ) << endl;
    newfile << "TEXTCOLOUR = " << m_WFECFG.GetString( Section, "TEXTCOLOUR", "0xFFFFFFFF" ) << endl;
    newfile << "SHADOWCOLOUR = " << m_WFECFG.GetString( Section, "SHADOWCOLOUR", "0xFF000000" ) << endl;
    newfile << endl;
    Section = "HEALTHBAR";
    newfile << "[HEALTHBAR]" << endl;
    newfile << "ISCOLOURENABLED = " << m_WFECFG.GetString( Section, "ISCOLOURENABLED", "no" ) << endl;
    newfile << "YOURCOLOUR = " << m_WFECFG.GetString( Section, "YOURCOLOUR", "0xFF00FF00" ) << endl;
    newfile << "ALLYCOLOUR = " << m_WFECFG.GetString( Section, "ALLYCOLOUR", "0xFF00FF00" ) << endl;
    newfile << "ENEMYCOLOUR = " << m_WFECFG.GetString( Section, "ENEMYCOLOUR", "0xFF00FF00" ) << endl;
    newfile << "NEUTRALAGGRESSIVECOLOUR = " << m_WFECFG.GetString( Section, "NEUTRALAGGRESSIVECOLOUR", "0xFF00FF00" ) << endl;
    newfile << "NEUTRALPASSIVECOLOUR = " << m_WFECFG.GetString( Section, "NEUTRALPASSIVECOLOUR", "0xFF00FF00" ) << endl;
    newfile << "HEIGHT = " << m_WFECFG.GetString( Section, "HEIGHT", ".004" ) << endl;
    newfile << endl;
    Section = "MANABAR";
    newfile << "[MANABAR]" << endl;
    newfile << "ISENABLED = " << m_WFECFG.GetString( Section, "ISENABLED", "no" ) << endl;
    newfile << "ISHEROONLY = " << m_WFECFG.GetString( Section, "ISHEROONLY", "Off" ) << endl;
    newfile << "HEIGHT = " << m_WFECFG.GetString( Section, "HEIGHT", ".004" ) << endl;
    newfile << "ISCOLOURENABLED = " << m_WFECFG.GetString( Section, "ISCOLOURENABLED", string( ) ) << endl;
    newfile << "COLOUR = " << m_WFECFG.GetString( Section, "COLOUR", "0xFF0000FF" ) << endl;
    newfile << endl;
    Section = "INDICATORS";
    newfile << "[INDICATORS]" << endl;
    newfile << "ISCOLOURENABLED = " << m_WFECFG.GetString( Section, "ISCOLOURENABLED", "no" ) << endl;
    newfile << "ATTACKCOLOUR = " << m_WFECFG.GetString( Section, "ATTACKCOLOUR", "0xFF00FFFF" ) << endl;
    newfile << "SPELLAOECOLOUR = " << m_WFECFG.GetString( Section, "SPELLAOECOLOUR", "0xFF00FFFF" ) << endl;
    newfile << "SIGHTRADIUSCOLOUR = " << m_WFECFG.GetString( Section, "SIGHTRADIUSCOLOUR", "0xFF00FFFF" ) << endl;
    newfile << endl;
    Section = "DAMAGEDRAW";
    newfile << "[DAMAGEDRAW]" << endl;
    newfile << "ISENABLED = " << m_WFECFG.GetString( Section, "ISENABLED", "no" ) << endl;
    newfile << "ISHEROONLY = " << m_WFECFG.GetString( Section, "ISHEROONLY", "Off" ) << endl;
    newfile << "PHYSICALCOLOUR = " << m_WFECFG.GetString( Section, "PHYSICALCOLOUR", "0xFFFF0000" ) << endl;
    newfile << "MAGICALCOLOUR = " << m_WFECFG.GetString( Section, "MAGICALCOLOUR", "0xFF0000FF" ) << endl;
    newfile << "PHYSICALOFFSET = " << m_WFECFG.GetString( Section, "PHYSICALOFFSET", "-100" ) << endl;
    newfile << "MAGICALOFFSET = " << m_WFECFG.GetString( Section, "MAGICALOFFSET", "50" ) << endl;
    newfile << "HEIGHT = " << m_WFECFG.GetString( Section, "HEIGHT", "25" ) << endl;
    newfile << "ANGLE = " << m_WFECFG.GetString( Section, "ANGLE", "90" ) << endl;
    newfile << "SPEED = " << m_WFECFG.GetString( Section, "SPEED", "100" ) << endl;
    newfile << "SIZE = " << m_WFECFG.GetString( Section, "SIZE", "11" ) << endl;
    newfile << "DURATION = " << m_WFECFG.GetString( Section, "DURATION", "2" ) << endl;
    newfile << "ISALLY = " << m_WFECFG.GetString( Section, "ISALLY", "yes" ) << endl;
    newfile << "ISENEMY = " << m_WFECFG.GetString( Section, "ISENEMY", "yes" ) << endl;
    newfile << "ISMY = " << m_WFECFG.GetString( Section, "ISMY", "yes" ) << endl;
    newfile << endl;
    Section = "HOTKEYS";
    newfile << "[HOTKEYS]" << endl;
    newfile << "SELFCAST = " << m_WFECFG.GetString( Section, "SELFCAST", "ALT" ) << endl;
    newfile << "AUTOCAST = " << m_WFECFG.GetString( Section, "AUTOCAST", "CTRL" ) << endl;
    newfile << "ISHEROONLY = " << m_WFECFG.GetString( Section, "ISHEROONLY", "Off" ) << endl;
    newfile << "UTILWINDOWMODE = " << m_WFECFG.GetString( Section, "UTILWINDOWMODE", "CTRL + Enter" ) << endl;
    newfile << "UTILMOUSELOCK = " << m_WFECFG.GetString( Section, "UTILMOUSELOCK", "CTRL + NumPad3" ) << endl;
    newfile << "UTILWIDESCREEN = " << m_WFECFG.GetString( Section, "UTILWIDESCREEN", "CTRL + NumPad6" ) << endl;
    newfile << "UTILPAUSE = " << m_WFECFG.GetString( Section, "UTILPAUSE", "CTRL + NumPad9" ) << endl;
    newfile << "UTILUI = " << m_WFECFG.GetString( Section, "UTILUI", "CTRL + ~" ) << endl;
    newfile << "UTILTOGGLEMULTIBOARD = " << m_WFECFG.GetString( Section, "UTILTOGGLEMULTIBOARD", string( ) ) << endl;
    newfile << "UTILSHOWHIDEMULTIBOARD = " << m_WFECFG.GetString( Section, "UTILSHOWHIDEMULTIBOARD", string( ) ) << endl;
    newfile << "UTILUPDATE = " << m_WFECFG.GetString( Section, "UTILUPDATE", "NumPad9" ) << endl;
    newfile << "CAMERAOVERRRIDE = " << m_WFECFG.GetString( Section, "CAMERAOVERRRIDE", "NumPad0" ) << endl;
    newfile << "CAMERARESTORE = " << m_WFECFG.GetString( Section, "CAMERARESTORE", "CTRL + NumPad0" ) << endl;
    newfile << "CAMERATILT = " << m_WFECFG.GetString( Section, "CAMERATILT", string( ) ) << endl;
    newfile << "CAMERADISTANCE = " << m_WFECFG.GetString( Section, "CAMERADISTANCE", "CTRL" ) << endl;
    newfile << "CAMERAHEIGHT = " << m_WFECFG.GetString( Section, "CAMERAHEIGHT", "SHIFT" ) << endl;
    newfile << "CAMERAROTATION = " << m_WFECFG.GetString( Section, "CAMERAROTATION", "ALT" ) << endl;
    newfile << "UNITSAVE = " << m_WFECFG.GetString( Section, "UNITSAVE", "NumPad3" ) << endl;
    newfile << "UNITSELECT = " << m_WFECFG.GetString( Section, "UNITSELECT", string( ) ) << endl;
    newfile << "UNITAUTOSELECTION = " << m_WFECFG.GetString( Section, "UNITAUTOSELECTION", "NumPad6" ) << endl;
    newfile << "UNITCLEARSELECTION = " << m_WFECFG.GetString( Section, "UNITCLEARSELECTION", "~" ) << endl;
    newfile << "UTILCLEARMESSAGES = " << m_WFECFG.GetString( Section, "UTILCLEARMESSAGES", "Back" ) << endl;
    newfile << "UTILCLEARCHAT = " << m_WFECFG.GetString( Section, "UTILCLEARCHAT", string( ) ) << endl;
    newfile << "DRAWATTACKONHOVER = " << m_WFECFG.GetString( Section, "DRAWATTACKONHOVER", string( ) ) << endl;
    newfile << "DRAWATTACK = " << m_WFECFG.GetString( Section, "DRAWATTACK", string( ) ) << endl;
    newfile << "DRAWSPELLAOEONHOVER = " << m_WFECFG.GetString( Section, "DRAWSPELLAOEONHOVER", string( ) ) << endl;
    newfile << "DRAWSPELLAOE = " << m_WFECFG.GetString( Section, "DRAWSPELLAOE", string( ) ) << endl;
    newfile << "DRAWSIGHTRADIUS = " << m_WFECFG.GetString( Section, "DRAWSIGHTRADIUS", string( ) ) << endl;
    newfile << endl;
    Section = "KEYBINDS";
    newfile << "[KEYBINDS]" << endl;
    newfile << "===========================Normal===========================" << endl;

    if( m_WFECFG.GetString( SectionUNM, "HOTKEYS", "yes" ) == "yes" && m_WFECFG.GetString( SectionUNM, "SPELLS", "yes" ) == "yes" )
    {
        newfile << HotKeyCvarList[0] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[0], m_WFECFG.GetString( Section, HotKeyCvarList[0], string( ) ) ) << endl;
        newfile << HotKeyCvarList[1] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[1], m_WFECFG.GetString( Section, HotKeyCvarList[1], string( ) ) ) << endl;
        newfile << HotKeyCvarList[2] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[2], m_WFECFG.GetString( Section, HotKeyCvarList[2], string( ) ) ) << endl;
        newfile << HotKeyCvarList[3] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[3], m_WFECFG.GetString( Section, HotKeyCvarList[3], string( ) ) ) << endl;
        newfile << HotKeyCvarList[4] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[4], m_WFECFG.GetString( Section, HotKeyCvarList[4], string( ) ) ) << endl;
        newfile << HotKeyCvarList[5] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[5], m_WFECFG.GetString( Section, HotKeyCvarList[5], string( ) ) ) << endl;
        newfile << HotKeyCvarList[6] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[6], m_WFECFG.GetString( Section, HotKeyCvarList[6], string( ) ) ) << endl;
        newfile << HotKeyCvarList[7] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[7], m_WFECFG.GetString( Section, HotKeyCvarList[7], string( ) ) ) << endl;
        newfile << HotKeyCvarList[8] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[8], m_WFECFG.GetString( Section, HotKeyCvarList[8], string( ) ) ) << endl;
        newfile << HotKeyCvarList[9] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[9], m_WFECFG.GetString( Section, HotKeyCvarList[9], string( ) ) ) << endl;
        newfile << HotKeyCvarList[10] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[10], m_WFECFG.GetString( Section, HotKeyCvarList[10], string( ) ) ) << endl;
        newfile << HotKeyCvarList[11] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[11], m_WFECFG.GetString( Section, HotKeyCvarList[11], string( ) ) ) << endl;
    }
    else
    {
        newfile << HotKeyCvarList[0] << " = " << endl;
        newfile << HotKeyCvarList[1] << " = " << endl;
        newfile << HotKeyCvarList[2] << " = " << endl;
        newfile << HotKeyCvarList[3] << " = " << endl;
        newfile << HotKeyCvarList[4] << " = " << endl;
        newfile << HotKeyCvarList[5] << " = " << endl;
        newfile << HotKeyCvarList[6] << " = " << endl;
        newfile << HotKeyCvarList[7] << " = " << endl;
        newfile << HotKeyCvarList[8] << " = " << endl;
        newfile << HotKeyCvarList[9] << " = " << endl;
        newfile << HotKeyCvarList[10] << " = " << endl;
        newfile << HotKeyCvarList[11] << " = " << endl;
    }

    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "==========================Inventory=========================" << endl;

    if( m_WFECFG.GetString( SectionUNM, "HOTKEYS", "yes" ) == "yes" && m_WFECFG.GetString( SectionUNM, "INVENTORY", "yes" ) == "yes" )
    {
        newfile << HotKeyCvarList[36] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[36], m_WFECFG.GetString( Section, HotKeyCvarList[36], string( ) ) ) << endl;
        newfile << HotKeyCvarList[37] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[37], m_WFECFG.GetString( Section, HotKeyCvarList[37], string( ) ) ) << endl;
        newfile << HotKeyCvarList[38] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[38], m_WFECFG.GetString( Section, HotKeyCvarList[38], string( ) ) ) << endl;
        newfile << HotKeyCvarList[39] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[39], m_WFECFG.GetString( Section, HotKeyCvarList[39], string( ) ) ) << endl;
        newfile << HotKeyCvarList[40] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[40], m_WFECFG.GetString( Section, HotKeyCvarList[40], string( ) ) ) << endl;
        newfile << HotKeyCvarList[41] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[41], m_WFECFG.GetString( Section, HotKeyCvarList[41], string( ) ) ) << endl;
    }
    else
    {
        newfile << HotKeyCvarList[36] << " = " << endl;
        newfile << HotKeyCvarList[37] << " = " << endl;
        newfile << HotKeyCvarList[38] << " = " << endl;
        newfile << HotKeyCvarList[39] << " = " << endl;
        newfile << HotKeyCvarList[40] << " = " << endl;
        newfile << HotKeyCvarList[41] << " = " << endl;
    }

    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "===================Spellbook/Learn Ability==================" << endl;

    if( m_WFECFG.GetString( SectionUNM, "HOTKEYS", "yes" ) == "yes" && m_WFECFG.GetString( SectionUNM, "SPELBOOK", "yes" ) == "yes" )
    {
        newfile << HotKeyCvarList[12] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[12], m_WFECFG.GetString( Section, HotKeyCvarList[12], string( ) ) ) << endl;
        newfile << HotKeyCvarList[13] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[13], m_WFECFG.GetString( Section, HotKeyCvarList[13], string( ) ) ) << endl;
        newfile << HotKeyCvarList[14] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[14], m_WFECFG.GetString( Section, HotKeyCvarList[14], string( ) ) ) << endl;
        newfile << HotKeyCvarList[15] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[15], m_WFECFG.GetString( Section, HotKeyCvarList[15], string( ) ) ) << endl;
        newfile << HotKeyCvarList[16] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[16], m_WFECFG.GetString( Section, HotKeyCvarList[16], string( ) ) ) << endl;
        newfile << HotKeyCvarList[17] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[17], m_WFECFG.GetString( Section, HotKeyCvarList[17], string( ) ) ) << endl;
        newfile << HotKeyCvarList[18] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[18], m_WFECFG.GetString( Section, HotKeyCvarList[18], string( ) ) ) << endl;
        newfile << HotKeyCvarList[19] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[19], m_WFECFG.GetString( Section, HotKeyCvarList[19], string( ) ) ) << endl;
        newfile << HotKeyCvarList[20] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[20], m_WFECFG.GetString( Section, HotKeyCvarList[20], string( ) ) ) << endl;
        newfile << HotKeyCvarList[21] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[21], m_WFECFG.GetString( Section, HotKeyCvarList[21], string( ) ) ) << endl;
        newfile << HotKeyCvarList[22] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[22], m_WFECFG.GetString( Section, HotKeyCvarList[22], string( ) ) ) << endl;
        newfile << HotKeyCvarList[23] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[23], m_WFECFG.GetString( Section, HotKeyCvarList[23], string( ) ) ) << endl;
    }
    else
    {
        newfile << HotKeyCvarList[12] << " = " << endl;
        newfile << HotKeyCvarList[13] << " = " << endl;
        newfile << HotKeyCvarList[14] << " = " << endl;
        newfile << HotKeyCvarList[15] << " = " << endl;
        newfile << HotKeyCvarList[16] << " = " << endl;
        newfile << HotKeyCvarList[17] << " = " << endl;
        newfile << HotKeyCvarList[18] << " = " << endl;
        newfile << HotKeyCvarList[19] << " = " << endl;
        newfile << HotKeyCvarList[20] << " = " << endl;
        newfile << HotKeyCvarList[21] << " = " << endl;
        newfile << HotKeyCvarList[22] << " = " << endl;
        newfile << HotKeyCvarList[23] << " = " << endl;
    }

    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "========================Neutral/Shops=======================" << endl;

    if( m_WFECFG.GetString( SectionUNM, "HOTKEYS", "yes" ) == "yes" && m_WFECFG.GetString( SectionUNM, "SHOP", "yes" ) == "yes" )
    {
        newfile << HotKeyCvarList[24] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[24], m_WFECFG.GetString( Section, HotKeyCvarList[24], string( ) ) ) << endl;
        newfile << HotKeyCvarList[25] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[25], m_WFECFG.GetString( Section, HotKeyCvarList[25], string( ) ) ) << endl;
        newfile << HotKeyCvarList[26] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[26], m_WFECFG.GetString( Section, HotKeyCvarList[26], string( ) ) ) << endl;
        newfile << HotKeyCvarList[27] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[27], m_WFECFG.GetString( Section, HotKeyCvarList[27], string( ) ) ) << endl;
        newfile << HotKeyCvarList[28] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[28], m_WFECFG.GetString( Section, HotKeyCvarList[28], string( ) ) ) << endl;
        newfile << HotKeyCvarList[29] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[29], m_WFECFG.GetString( Section, HotKeyCvarList[29], string( ) ) ) << endl;
        newfile << HotKeyCvarList[30] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[30], m_WFECFG.GetString( Section, HotKeyCvarList[30], string( ) ) ) << endl;
        newfile << HotKeyCvarList[31] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[31], m_WFECFG.GetString( Section, HotKeyCvarList[31], string( ) ) ) << endl;
        newfile << HotKeyCvarList[32] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[32], m_WFECFG.GetString( Section, HotKeyCvarList[32], string( ) ) ) << endl;
        newfile << HotKeyCvarList[33] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[33], m_WFECFG.GetString( Section, HotKeyCvarList[33], string( ) ) ) << endl;
        newfile << HotKeyCvarList[34] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[34], m_WFECFG.GetString( Section, HotKeyCvarList[34], string( ) ) ) << endl;
        newfile << HotKeyCvarList[35] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[35], m_WFECFG.GetString( Section, HotKeyCvarList[35], string( ) ) ) << endl;
    }
    else
    {
        newfile << HotKeyCvarList[24] << " = " << endl;
        newfile << HotKeyCvarList[25] << " = " << endl;
        newfile << HotKeyCvarList[26] << " = " << endl;
        newfile << HotKeyCvarList[27] << " = " << endl;
        newfile << HotKeyCvarList[28] << " = " << endl;
        newfile << HotKeyCvarList[29] << " = " << endl;
        newfile << HotKeyCvarList[30] << " = " << endl;
        newfile << HotKeyCvarList[31] << " = " << endl;
        newfile << HotKeyCvarList[32] << " = " << endl;
        newfile << HotKeyCvarList[33] << " = " << endl;
        newfile << HotKeyCvarList[34] << " = " << endl;
        newfile << HotKeyCvarList[35] << " = " << endl;
    }
    newfile << "============================================================" << endl;
    newfile << endl;
    Section = "SMARTCAST";
    newfile << "[SMARTCAST]" << endl;
    newfile << "===========================Normal===========================" << endl;
	
	if( m_WFECFG.GetString( SectionUNM, "HOTKEYS", "yes" ) == "yes" && m_WFECFG.GetString( SectionUNM, "SPELLS", "yes" ) == "yes" )
    {
        newfile << HotKeyCvarList[0] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[0], m_WFECFG.GetString( Section, HotKeyCvarList[0], "no" ) ) << endl;
        newfile << HotKeyCvarList[1] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[1], m_WFECFG.GetString( Section, HotKeyCvarList[1], "no" ) ) << endl;
        newfile << HotKeyCvarList[2] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[2], m_WFECFG.GetString( Section, HotKeyCvarList[2], "no" ) ) << endl;
        newfile << HotKeyCvarList[3] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[3], m_WFECFG.GetString( Section, HotKeyCvarList[3], "no" ) ) << endl;
        newfile << HotKeyCvarList[4] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[4], m_WFECFG.GetString( Section, HotKeyCvarList[4], "no" ) ) << endl;
        newfile << HotKeyCvarList[5] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[5], m_WFECFG.GetString( Section, HotKeyCvarList[5], "no" ) ) << endl;
        newfile << HotKeyCvarList[6] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[6], m_WFECFG.GetString( Section, HotKeyCvarList[6], "no" ) ) << endl;
        newfile << HotKeyCvarList[7] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[7], m_WFECFG.GetString( Section, HotKeyCvarList[7], "no" ) ) << endl;
        newfile << HotKeyCvarList[8] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[8], m_WFECFG.GetString( Section, HotKeyCvarList[8], "no" ) ) << endl;
        newfile << HotKeyCvarList[9] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[9], m_WFECFG.GetString( Section, HotKeyCvarList[9], "no" ) ) << endl;
        newfile << HotKeyCvarList[10] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[10], m_WFECFG.GetString( Section, HotKeyCvarList[10], "no" ) ) << endl;
        newfile << HotKeyCvarList[11] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[11], m_WFECFG.GetString( Section, HotKeyCvarList[11], "no" ) ) << endl;
	}
	else
	{
		newfile <<  HotKeyCvarList[0] << " = no" << endl;
		newfile <<  HotKeyCvarList[1] << " = no" << endl;
		newfile <<  HotKeyCvarList[2] << " = no" << endl;
		newfile <<  HotKeyCvarList[3] << " = no" << endl;
		newfile <<  HotKeyCvarList[4] << " = no" << endl;
		newfile <<  HotKeyCvarList[5] << " = no" << endl;
		newfile <<  HotKeyCvarList[6] << " = no" << endl;
		newfile <<  HotKeyCvarList[7] << " = no" << endl;
		newfile <<  HotKeyCvarList[8] << " = no" << endl;
		newfile <<  HotKeyCvarList[9] << " = no" << endl;
		newfile <<  HotKeyCvarList[10] << " = no" << endl;
		newfile <<  HotKeyCvarList[11] << " = no" << endl;
	}

    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "==========================Inventory=========================" << endl;
	
	if( m_WFECFG.GetString( SectionUNM, "HOTKEYS", "yes" ) == "yes" && m_WFECFG.GetString( SectionUNM, "INVENTORY", "yes" ) == "yes" )
    {
		newfile << HotKeyCvarList[36] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[36], m_WFECFG.GetString( Section, HotKeyCvarList[36], "no" ) ) << endl;
		newfile << HotKeyCvarList[37] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[37], m_WFECFG.GetString( Section, HotKeyCvarList[37], "no" ) ) << endl;
		newfile << HotKeyCvarList[38] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[38], m_WFECFG.GetString( Section, HotKeyCvarList[38], "no" ) ) << endl;
		newfile << HotKeyCvarList[39] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[39], m_WFECFG.GetString( Section, HotKeyCvarList[39], "no" ) ) << endl;
		newfile << HotKeyCvarList[40] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[40], m_WFECFG.GetString( Section, HotKeyCvarList[40], "no" ) ) << endl;
		newfile << HotKeyCvarList[41] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[41], m_WFECFG.GetString( Section, HotKeyCvarList[41], "no" ) ) << endl;
	}
	else
	{
		newfile << HotKeyCvarList[36] << " = no" << endl;
		newfile << HotKeyCvarList[37] << " = no" << endl;
		newfile << HotKeyCvarList[38] << " = no" << endl;
		newfile << HotKeyCvarList[39] << " = no" << endl;
		newfile << HotKeyCvarList[40] << " = no" << endl;
		newfile << HotKeyCvarList[41] << " = no" << endl;
	}

    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "===================Spellbook/Learn Ability==================" << endl;
	
	if( m_WFECFG.GetString( SectionUNM, "HOTKEYS", "yes" ) == "yes" && m_WFECFG.GetString( SectionUNM, "SPELBOOK", "yes" ) == "yes" )
    {
		newfile << HotKeyCvarList[12] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[12], m_WFECFG.GetString( Section, HotKeyCvarList[12], "no" ) ) << endl;
		newfile << HotKeyCvarList[13] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[13], m_WFECFG.GetString( Section, HotKeyCvarList[13], "no" ) ) << endl;
		newfile << HotKeyCvarList[14] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[14], m_WFECFG.GetString( Section, HotKeyCvarList[14], "no" ) ) << endl;
		newfile << HotKeyCvarList[15] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[15], m_WFECFG.GetString( Section, HotKeyCvarList[15], "no" ) ) << endl;
		newfile << HotKeyCvarList[16] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[16], m_WFECFG.GetString( Section, HotKeyCvarList[16], "no" ) ) << endl;
		newfile << HotKeyCvarList[17] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[17], m_WFECFG.GetString( Section, HotKeyCvarList[17], "no" ) ) << endl;
		newfile << HotKeyCvarList[18] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[18], m_WFECFG.GetString( Section, HotKeyCvarList[18], "no" ) ) << endl;
		newfile << HotKeyCvarList[19] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[19], m_WFECFG.GetString( Section, HotKeyCvarList[19], "no" ) ) << endl;
		newfile << HotKeyCvarList[20] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[20], m_WFECFG.GetString( Section, HotKeyCvarList[20], "no" ) ) << endl;
		newfile << HotKeyCvarList[21] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[21], m_WFECFG.GetString( Section, HotKeyCvarList[21], "no" ) ) << endl;
		newfile << HotKeyCvarList[22] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[22], m_WFECFG.GetString( Section, HotKeyCvarList[22], "no" ) ) << endl;
		newfile << HotKeyCvarList[23] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[23], m_WFECFG.GetString( Section, HotKeyCvarList[23], "no" ) ) << endl;
	}
	else
	{
		newfile << HotKeyCvarList[12] << " = no" << endl;
		newfile << HotKeyCvarList[13] << " = no" << endl;
		newfile << HotKeyCvarList[14] << " = no" << endl;
		newfile << HotKeyCvarList[15] << " = no" << endl;
		newfile << HotKeyCvarList[16] << " = no" << endl;
		newfile << HotKeyCvarList[17] << " = no" << endl;
		newfile << HotKeyCvarList[18] << " = no" << endl;
		newfile << HotKeyCvarList[19] << " = no" << endl;
		newfile << HotKeyCvarList[20] << " = no" << endl;
		newfile << HotKeyCvarList[21] << " = no" << endl;
		newfile << HotKeyCvarList[22] << " = no" << endl;
		newfile << HotKeyCvarList[23] << " = no" << endl;
	}

    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "========================Neutral/Shops=======================" << endl;
	
	if( m_WFECFG.GetString( SectionUNM, "HOTKEYS", "yes" ) == "yes" && m_WFECFG.GetString( SectionUNM, "SHOP", "yes" ) == "yes" )
    {
		newfile << HotKeyCvarList[24] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[24], m_WFECFG.GetString( Section, HotKeyCvarList[24], "no" ) ) << endl;
		newfile << HotKeyCvarList[25] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[25], m_WFECFG.GetString( Section, HotKeyCvarList[25], "no" ) ) << endl;
		newfile << HotKeyCvarList[26] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[26], m_WFECFG.GetString( Section, HotKeyCvarList[26], "no" ) ) << endl;
		newfile << HotKeyCvarList[27] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[27], m_WFECFG.GetString( Section, HotKeyCvarList[27], "no" ) ) << endl;
		newfile << HotKeyCvarList[28] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[28], m_WFECFG.GetString( Section, HotKeyCvarList[28], "no" ) ) << endl;
		newfile << HotKeyCvarList[29] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[29], m_WFECFG.GetString( Section, HotKeyCvarList[29], "no" ) ) << endl;
		newfile << HotKeyCvarList[30] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[30], m_WFECFG.GetString( Section, HotKeyCvarList[30], "no" ) ) << endl;
		newfile << HotKeyCvarList[31] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[31], m_WFECFG.GetString( Section, HotKeyCvarList[31], "no" ) ) << endl;
		newfile << HotKeyCvarList[32] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[32], m_WFECFG.GetString( Section, HotKeyCvarList[32], "no" ) ) << endl;
		newfile << HotKeyCvarList[33] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[33], m_WFECFG.GetString( Section, HotKeyCvarList[33], "no" ) ) << endl;
		newfile << HotKeyCvarList[34] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[34], m_WFECFG.GetString( Section, HotKeyCvarList[34], "no" ) ) << endl;
		newfile << HotKeyCvarList[35] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[35], m_WFECFG.GetString( Section, HotKeyCvarList[35], "no" ) ) << endl;
	}
	else
	{
		newfile << HotKeyCvarList[24] << " = no" << endl;
		newfile << HotKeyCvarList[25] << " = no" << endl;
		newfile << HotKeyCvarList[26] << " = no" << endl;
		newfile << HotKeyCvarList[27] << " = no" << endl;
		newfile << HotKeyCvarList[28] << " = no" << endl;
		newfile << HotKeyCvarList[29] << " = no" << endl;
		newfile << HotKeyCvarList[30] << " = no" << endl;
		newfile << HotKeyCvarList[31] << " = no" << endl;
		newfile << HotKeyCvarList[32] << " = no" << endl;
		newfile << HotKeyCvarList[33] << " = no" << endl;
		newfile << HotKeyCvarList[34] << " = no" << endl;
		newfile << HotKeyCvarList[35] << " = no" << endl;
	}
	
    newfile << "============================================================" << endl;
    newfile << endl;
    Section = "CAMERA";
    newfile << "[CAMERA]" << endl;
    newfile << "CAMERAMODIFIABLE = " << m_WFECFG.GetString( Section, "CAMERAMODIFIABLE", "yes" ) << endl;
    newfile << "DISTANCESTEP = " << m_WFECFG.GetString( Section, "DISTANCESTEP", "50" ) << endl;
    newfile << "HEIGHTSTEP = " << m_WFECFG.GetString( Section, "HEIGHTSTEP", "50" ) << endl;
    newfile << "YAWSTEP = " << m_WFECFG.GetString( Section, "YAWSTEP", "10" ) << endl;
    newfile << "PITCHSTEP = " << m_WFECFG.GetString( Section, "PITCHSTEP", "10" ) << endl;
    newfile << "FARZ = " << m_WFECFG.GetString( Section, "FARZ", "5000" ) << endl;
    newfile << "FOGZ = " << m_WFECFG.GetString( Section, "FOGZ", "5000" ) << endl;
    newfile << endl;
    Section = "DELAY";
    newfile << "[DELAY]" << endl;
    newfile << "GAMESTART = " << m_WFECFG.GetString( Section, "GAMESTART", "6" ) << endl;
    newfile << "LAN = " << m_WFECFG.GetString( Section, "LAN", "100" ) << endl;
    newfile << "BATTLE.NET = " << m_WFECFG.GetString( Section, "BATTLE.NET", "250" ) << endl;
    newfile << "RMCCLICK = " << m_WFECFG.GetString( Section, "RMCCLICK", "5" ) << endl;
    newfile << endl;
    Section = "INTERFACE";
    newfile << "[INTERFACE]" << endl;
    newfile << "ISCUSTOMUIDRAW = " << m_WFECFG.GetString( Section, "ISCUSTOMUIDRAW", "no" ) << endl;
    newfile << "ISBLACKBARS = " << m_WFECFG.GetString( Section, "ISBLACKBARS", "yes" ) << endl;
    newfile << "ISINFOBAR = " << m_WFECFG.GetString( Section, "ISINFOBAR", "yes" ) << endl;
    newfile << "ISITEMBAR = " << m_WFECFG.GetString( Section, "ISITEMBAR", "yes" ) << endl;
    newfile << "ISABILITYBAR = " << m_WFECFG.GetString( Section, "ISABILITYBAR", "yes" ) << endl;
    newfile << "ISTIMEOFDAYINDICATOR = " << m_WFECFG.GetString( Section, "ISTIMEOFDAYINDICATOR", "yes" ) << endl;
    newfile << "ISINVENTORYCOVER = " << m_WFECFG.GetString( Section, "ISINVENTORYCOVER", "yes" ) << endl;
    newfile << "ISHEROBAR = " << m_WFECFG.GetString( Section, "ISHEROBAR", "yes" ) << endl;
    newfile << "ISPEONBAR = " << m_WFECFG.GetString( Section, "ISPEONBAR", "yes" ) << endl;
    newfile << "ISPORTRAIT = " << m_WFECFG.GetString( Section, "ISPORTRAIT", "yes" ) << endl;
    newfile << "ISCONSOLE = " << m_WFECFG.GetString( Section, "ISCONSOLE", "yes" ) << endl;
    newfile << "ISRESOURCEBAR = " << m_WFECFG.GetString( Section, "ISRESOURCEBAR", "yes" ) << endl;
    newfile << "ISUPPERMENUBAR = " << m_WFECFG.GetString( Section, "ISUPPERMENUBAR", "yes" ) << endl;
    newfile << "ISMINIMAP = " << m_WFECFG.GetString( Section, "ISMINIMAP", "yes" ) << endl;
    newfile << "ISMINIMAPBUTTONS = " << m_WFECFG.GetString( Section, "ISMINIMAPBUTTONS", "yes" ) << endl;
    newfile << "ISTOOLTIP = " << m_WFECFG.GetString( Section, "ISTOOLTIP", "yes" ) << endl;
    newfile << "ISATTACHTOOLTIP = " << m_WFECFG.GetString( Section, "ISATTACHTOOLTIP", "no" ) << endl;
    newfile << endl;
    Section = "LAUNCHER";
    newfile << "[LAUNCHER]" << endl;
    newfile << "GAMEPATH = " << m_WFECFG.GetString( Section, "GAMEPATH", string( ) ) << endl;
    newfile << "ARGUMENTS = " << m_WFECFG.GetString( Section, "ARGUMENTS", string( ) ) << endl;
    newfile << "BORDERLESS = " << m_WFECFG.GetString( Section, "BORDERLESS", "no" ) << endl;
    newfile << "MAINWINDOWNAME = " << m_WFECFG.GetString( Section, "MAINWINDOWNAME", "Warcraft III" ) << endl;
    newfile << "WINDOWNAME = " << m_WFECFG.GetString( Section, "WINDOWNAME", "Warcraft III" ) << endl;
    newfile << "DLLNAME = " << m_WFECFG.GetString( Section, "DLLNAME", "WFEDll.dll" ) << endl;
    newfile << "AUTOINJECT = " << m_WFECFG.GetString( Section, "AUTOINJECT", "no" ) << endl;
    newfile << "LIBRARIESPATH = " << m_WFECFG.GetString( Section, "LIBRARIESPATH", "D:\\Downloads\\WFE v2.23\\Libraries\\" ) << endl;
    newfile << endl;
    Section = "QUICKCHAT";
    newfile << "[QUICKCHAT]" << endl;
    newfile << "SENDTOALL = " << m_WFECFG.GetString( Section, "SENDTOALL", "no" ) << endl;
	newfile << endl;
    Section = "KEYBINDS";
    newfile << "[" + SectionUNM + "]" << endl;
    newfile << "===========================Normal===========================" << endl;
    newfile << HotKeyCvarList[0] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[0], m_WFECFG.GetString( Section, HotKeyCvarList[0], string( ) ) ) << endl;
    newfile << HotKeyCvarList[1] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[1], m_WFECFG.GetString( Section, HotKeyCvarList[1], string( ) ) ) << endl;
    newfile << HotKeyCvarList[2] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[2], m_WFECFG.GetString( Section, HotKeyCvarList[2], string( ) ) ) << endl;
    newfile << HotKeyCvarList[3] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[3], m_WFECFG.GetString( Section, HotKeyCvarList[3], string( ) ) ) << endl;
    newfile << HotKeyCvarList[4] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[4], m_WFECFG.GetString( Section, HotKeyCvarList[4], string( ) ) ) << endl;
    newfile << HotKeyCvarList[5] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[5], m_WFECFG.GetString( Section, HotKeyCvarList[5], string( ) ) ) << endl;
    newfile << HotKeyCvarList[6] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[6], m_WFECFG.GetString( Section, HotKeyCvarList[6], string( ) ) ) << endl;
    newfile << HotKeyCvarList[7] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[7], m_WFECFG.GetString( Section, HotKeyCvarList[7], string( ) ) ) << endl;
    newfile << HotKeyCvarList[8] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[8], m_WFECFG.GetString( Section, HotKeyCvarList[8], string( ) ) ) << endl;
    newfile << HotKeyCvarList[9] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[9], m_WFECFG.GetString( Section, HotKeyCvarList[9], string( ) ) ) << endl;
    newfile << HotKeyCvarList[10] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[10], m_WFECFG.GetString( Section, HotKeyCvarList[10], string( ) ) ) << endl;
    newfile << HotKeyCvarList[11] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[11], m_WFECFG.GetString( Section, HotKeyCvarList[11], string( ) ) ) << endl;
    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "==========================Inventory=========================" << endl;
    newfile << HotKeyCvarList[36] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[36], m_WFECFG.GetString( Section, HotKeyCvarList[36], string( ) ) ) << endl;
    newfile << HotKeyCvarList[37] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[37], m_WFECFG.GetString( Section, HotKeyCvarList[37], string( ) ) ) << endl;
    newfile << HotKeyCvarList[38] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[38], m_WFECFG.GetString( Section, HotKeyCvarList[38], string( ) ) ) << endl;
    newfile << HotKeyCvarList[39] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[39], m_WFECFG.GetString( Section, HotKeyCvarList[39], string( ) ) ) << endl;
    newfile << HotKeyCvarList[40] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[40], m_WFECFG.GetString( Section, HotKeyCvarList[40], string( ) ) ) << endl;
    newfile << HotKeyCvarList[41] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[41], m_WFECFG.GetString( Section, HotKeyCvarList[41], string( ) ) ) << endl;
    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "===================Spellbook/Learn Ability==================" << endl;
    newfile << HotKeyCvarList[12] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[12], m_WFECFG.GetString( Section, HotKeyCvarList[12], string( ) ) ) << endl;
    newfile << HotKeyCvarList[13] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[13], m_WFECFG.GetString( Section, HotKeyCvarList[13], string( ) ) ) << endl;
    newfile << HotKeyCvarList[14] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[14], m_WFECFG.GetString( Section, HotKeyCvarList[14], string( ) ) ) << endl;
    newfile << HotKeyCvarList[15] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[15], m_WFECFG.GetString( Section, HotKeyCvarList[15], string( ) ) ) << endl;
    newfile << HotKeyCvarList[16] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[16], m_WFECFG.GetString( Section, HotKeyCvarList[16], string( ) ) ) << endl;
    newfile << HotKeyCvarList[17] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[17], m_WFECFG.GetString( Section, HotKeyCvarList[17], string( ) ) ) << endl;
    newfile << HotKeyCvarList[18] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[18], m_WFECFG.GetString( Section, HotKeyCvarList[18], string( ) ) ) << endl;
    newfile << HotKeyCvarList[19] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[19], m_WFECFG.GetString( Section, HotKeyCvarList[19], string( ) ) ) << endl;
    newfile << HotKeyCvarList[20] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[20], m_WFECFG.GetString( Section, HotKeyCvarList[20], string( ) ) ) << endl;
    newfile << HotKeyCvarList[21] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[21], m_WFECFG.GetString( Section, HotKeyCvarList[21], string( ) ) ) << endl;
    newfile << HotKeyCvarList[22] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[22], m_WFECFG.GetString( Section, HotKeyCvarList[22], string( ) ) ) << endl;
    newfile << HotKeyCvarList[23] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[23], m_WFECFG.GetString( Section, HotKeyCvarList[23], string( ) ) ) << endl;
    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "========================Neutral/Shops=======================" << endl;
    newfile << HotKeyCvarList[24] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[24], m_WFECFG.GetString( Section, HotKeyCvarList[24], string( ) ) ) << endl;
    newfile << HotKeyCvarList[25] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[25], m_WFECFG.GetString( Section, HotKeyCvarList[25], string( ) ) ) << endl;
    newfile << HotKeyCvarList[26] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[26], m_WFECFG.GetString( Section, HotKeyCvarList[26], string( ) ) ) << endl;
    newfile << HotKeyCvarList[27] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[27], m_WFECFG.GetString( Section, HotKeyCvarList[27], string( ) ) ) << endl;
    newfile << HotKeyCvarList[28] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[28], m_WFECFG.GetString( Section, HotKeyCvarList[28], string( ) ) ) << endl;
    newfile << HotKeyCvarList[29] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[29], m_WFECFG.GetString( Section, HotKeyCvarList[29], string( ) ) ) << endl;
    newfile << HotKeyCvarList[30] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[30], m_WFECFG.GetString( Section, HotKeyCvarList[30], string( ) ) ) << endl;
    newfile << HotKeyCvarList[31] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[31], m_WFECFG.GetString( Section, HotKeyCvarList[31], string( ) ) ) << endl;
    newfile << HotKeyCvarList[32] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[32], m_WFECFG.GetString( Section, HotKeyCvarList[32], string( ) ) ) << endl;
    newfile << HotKeyCvarList[33] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[33], m_WFECFG.GetString( Section, HotKeyCvarList[33], string( ) ) ) << endl;
    newfile << HotKeyCvarList[34] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[34], m_WFECFG.GetString( Section, HotKeyCvarList[34], string( ) ) ) << endl;
    newfile << HotKeyCvarList[35] << " = " << m_WFECFG.GetString( SectionUNM, HotKeyCvarList[35], m_WFECFG.GetString( Section, HotKeyCvarList[35], string( ) ) ) << endl;
    newfile << "============================================================" << endl;
    newfile << endl;
    Section = "SMARTCAST";
    newfile << "[" + SectionUNMSmart + "]" << endl;
    newfile << "===========================Normal===========================" << endl;
    newfile << HotKeyCvarList[0] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[0], m_WFECFG.GetString( Section, HotKeyCvarList[0], "no" ) ) << endl;
    newfile << HotKeyCvarList[1] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[1], m_WFECFG.GetString( Section, HotKeyCvarList[1], "no" ) ) << endl;
    newfile << HotKeyCvarList[2] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[2], m_WFECFG.GetString( Section, HotKeyCvarList[2], "no" ) ) << endl;
    newfile << HotKeyCvarList[3] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[3], m_WFECFG.GetString( Section, HotKeyCvarList[3], "no" ) ) << endl;
    newfile << HotKeyCvarList[4] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[4], m_WFECFG.GetString( Section, HotKeyCvarList[4], "no" ) ) << endl;
    newfile << HotKeyCvarList[5] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[5], m_WFECFG.GetString( Section, HotKeyCvarList[5], "no" ) ) << endl;
    newfile << HotKeyCvarList[6] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[6], m_WFECFG.GetString( Section, HotKeyCvarList[6], "no" ) ) << endl;
    newfile << HotKeyCvarList[7] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[7], m_WFECFG.GetString( Section, HotKeyCvarList[7], "no" ) ) << endl;
    newfile << HotKeyCvarList[8] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[8], m_WFECFG.GetString( Section, HotKeyCvarList[8], "no" ) ) << endl;
    newfile << HotKeyCvarList[9] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[9], m_WFECFG.GetString( Section, HotKeyCvarList[9], "no" ) ) << endl;
    newfile << HotKeyCvarList[10] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[10], m_WFECFG.GetString( Section, HotKeyCvarList[10], "no" ) ) << endl;
    newfile << HotKeyCvarList[11] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[11], m_WFECFG.GetString( Section, HotKeyCvarList[11], "no" ) ) << endl;
    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "==========================Inventory=========================" << endl;
	newfile << HotKeyCvarList[36] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[36], m_WFECFG.GetString( Section, HotKeyCvarList[36], "no" ) ) << endl;
	newfile << HotKeyCvarList[37] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[37], m_WFECFG.GetString( Section, HotKeyCvarList[37], "no" ) ) << endl;
	newfile << HotKeyCvarList[38] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[38], m_WFECFG.GetString( Section, HotKeyCvarList[38], "no" ) ) << endl;
	newfile << HotKeyCvarList[39] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[39], m_WFECFG.GetString( Section, HotKeyCvarList[39], "no" ) ) << endl;
	newfile << HotKeyCvarList[40] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[40], m_WFECFG.GetString( Section, HotKeyCvarList[40], "no" ) ) << endl;
	newfile << HotKeyCvarList[41] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[41], m_WFECFG.GetString( Section, HotKeyCvarList[41], "no" ) ) << endl;
    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "===================Spellbook/Learn Ability==================" << endl;
	newfile << HotKeyCvarList[12] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[12], m_WFECFG.GetString( Section, HotKeyCvarList[12], "no" ) ) << endl;
	newfile << HotKeyCvarList[13] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[13], m_WFECFG.GetString( Section, HotKeyCvarList[13], "no" ) ) << endl;
	newfile << HotKeyCvarList[14] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[14], m_WFECFG.GetString( Section, HotKeyCvarList[14], "no" ) ) << endl;
	newfile << HotKeyCvarList[15] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[15], m_WFECFG.GetString( Section, HotKeyCvarList[15], "no" ) ) << endl;
	newfile << HotKeyCvarList[16] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[16], m_WFECFG.GetString( Section, HotKeyCvarList[16], "no" ) ) << endl;
	newfile << HotKeyCvarList[17] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[17], m_WFECFG.GetString( Section, HotKeyCvarList[17], "no" ) ) << endl;
	newfile << HotKeyCvarList[18] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[18], m_WFECFG.GetString( Section, HotKeyCvarList[18], "no" ) ) << endl;
	newfile << HotKeyCvarList[19] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[19], m_WFECFG.GetString( Section, HotKeyCvarList[19], "no" ) ) << endl;
	newfile << HotKeyCvarList[20] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[20], m_WFECFG.GetString( Section, HotKeyCvarList[20], "no" ) ) << endl;
	newfile << HotKeyCvarList[21] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[21], m_WFECFG.GetString( Section, HotKeyCvarList[21], "no" ) ) << endl;
	newfile << HotKeyCvarList[22] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[22], m_WFECFG.GetString( Section, HotKeyCvarList[22], "no" ) ) << endl;
	newfile << HotKeyCvarList[23] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[23], m_WFECFG.GetString( Section, HotKeyCvarList[23], "no" ) ) << endl;
    newfile << "============================================================" << endl;
    newfile << endl;
    newfile << "========================Neutral/Shops=======================" << endl;
	newfile << HotKeyCvarList[24] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[24], m_WFECFG.GetString( Section, HotKeyCvarList[24], "no" ) ) << endl;
	newfile << HotKeyCvarList[25] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[25], m_WFECFG.GetString( Section, HotKeyCvarList[25], "no" ) ) << endl;
	newfile << HotKeyCvarList[26] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[26], m_WFECFG.GetString( Section, HotKeyCvarList[26], "no" ) ) << endl;
	newfile << HotKeyCvarList[27] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[27], m_WFECFG.GetString( Section, HotKeyCvarList[27], "no" ) ) << endl;
	newfile << HotKeyCvarList[28] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[28], m_WFECFG.GetString( Section, HotKeyCvarList[28], "no" ) ) << endl;
	newfile << HotKeyCvarList[29] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[29], m_WFECFG.GetString( Section, HotKeyCvarList[29], "no" ) ) << endl;
	newfile << HotKeyCvarList[30] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[30], m_WFECFG.GetString( Section, HotKeyCvarList[30], "no" ) ) << endl;
	newfile << HotKeyCvarList[31] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[31], m_WFECFG.GetString( Section, HotKeyCvarList[31], "no" ) ) << endl;
	newfile << HotKeyCvarList[32] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[32], m_WFECFG.GetString( Section, HotKeyCvarList[32], "no" ) ) << endl;
	newfile << HotKeyCvarList[33] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[33], m_WFECFG.GetString( Section, HotKeyCvarList[33], "no" ) ) << endl;
	newfile << HotKeyCvarList[34] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[34], m_WFECFG.GetString( Section, HotKeyCvarList[34], "no" ) ) << endl;
	newfile << HotKeyCvarList[35] << " = " << m_WFECFG.GetString( SectionUNMSmart, HotKeyCvarList[35], m_WFECFG.GetString( Section, HotKeyCvarList[35], "no" ) ) << endl;
    newfile << "============================================================" << endl;
    newfile.close( );
}

bool Widget::GetKeyName( uint32_t scanCode, string &keyName )
{
    keyName = string( );

    if( ( scanCode > 88 && scanCode < 347 ) || scanCode == 0 || scanCode > 348 )
        return false;

    switch( scanCode )
    {
        case 1:
            keyName = "Escape";
            break;
        case 2:
            keyName = "1";
            break;
        case 3:
            keyName = "2";
            break;
        case 4:
            keyName = "3";
            break;
        case 5:
            keyName = "4";
            break;
        case 6:
            keyName = "5";
            break;
        case 7:
            keyName = "6";
            break;
        case 8:
            keyName = "7";
            break;
        case 9:
            keyName = "8";
            break;
        case 10:
            keyName = "9";
            break;
        case 11:
            keyName = "0";
            break;
        case 14:
            keyName = "Back";
            break;
        case 15:
            keyName = "Tab";
            break;
        case 16:
            keyName = "Q";
            break;
        case 17:
            keyName = "W";
            break;
        case 18:
            keyName = "E";
            break;
        case 19:
            keyName = "R";
            break;
        case 20:
            keyName = "T";
            break;
        case 21:
            keyName = "Y";
            break;
        case 22:
            keyName = "U";
            break;
        case 23:
            keyName = "I";
            break;
        case 24:
            keyName = "O";
            break;
        case 25:
            keyName = "P";
            break;
        case 30:
            keyName = "A";
            break;
        case 31:
            keyName = "S";
            break;
        case 32:
            keyName = "D";
            break;
        case 33:
            keyName = "F";
            break;
        case 34:
            keyName = "G";
            break;
        case 35:
            keyName = "H";
            break;
        case 36:
            keyName = "J";
            break;
        case 37:
            keyName = "K";
            break;
        case 38:
            keyName = "L";
            break;
        case 41:
            keyName = "~";
            break;
        case 44:
            keyName = "Z";
            break;
        case 45:
            keyName = "X";
            break;
        case 46:
            keyName = "C";
            break;
        case 47:
            keyName = "V";
            break;
        case 48:
            keyName = "B";
            break;
        case 49:
            keyName = "N";
            break;
        case 50:
            keyName = "M";
            break;
        case 57:
            keyName = "Space";
            break;
        case 58:
            keyName = "CapsLock";
            break;
        case 59:
            keyName = "F1";
            break;
        case 60:
            keyName = "F2";
            break;
        case 61:
            keyName = "F3";
            break;
        case 62:
            keyName = "F4";
            break;
        case 63:
            keyName = "F5";
            break;
        case 64:
            keyName = "F6";
            break;
        case 65:
            keyName = "F7";
            break;
        case 66:
            keyName = "F8";
            break;
        case 67:
            keyName = "F9";
            break;
        case 68:
            keyName = "F10";
            break;
        case 71:
            keyName = "NumPad7";
            break;
        case 72:
            keyName = "NumPad8";
            break;
        case 73:
            keyName = "NumPad9";
            break;
        case 75:
            keyName = "NumPad4";
            break;
        case 76:
            keyName = "NumPad5";
            break;
        case 77:
            keyName = "NumPad6";
            break;
        case 79:
            keyName = "NumPad1";
            break;
        case 80:
            keyName = "NumPad2";
            break;
        case 81:
            keyName = "NumPad3";
            break;
        case 82:
            keyName = "NumPad0";
            break;
        case 87:
            keyName = "F11";
            break;
        case 88:
            keyName = "F12";
            break;
        case 347:
            keyName = "LWin";
            break;
        case 348:
            keyName = "RWin";
        default:
            return false;
    }

    if( keyName.empty( ) )
        return false;
    else
    {
        if( LCtrl )
            keyName = "LCtrl + " + keyName;
        else if( LShift )
            keyName = "LShift + " + keyName;
        else if( LAlt )
            keyName = "LAlt + " + keyName;
        else if( RCtrl )
            keyName = "RCtrl + " + keyName;
        else if( RShift )
            keyName = "RShift + " + keyName;
        else if( RAlt )
            keyName = "RAlt + " + keyName;

        return true;
    }
}

void Widget::ClickOnKeySlot( uint32_t keyID )
{
    LShift = false;
    LCtrl = false;
    LAlt = false;
    RShift = false;
    RCtrl = false;
    RAlt = false;
    m_DarkenedWidget->resize( width( ), height( ) );
    m_DarkenedWidget->show( );
    setFocus( );
    m_WaitForKey = keyID;
}

void Widget::ClickOnKeyPostSlot( string keyName, bool cancel )
{
    LShift = false;
    LCtrl = false;
    LAlt = false;
    RShift = false;
    RCtrl = false;
    RAlt = false;
    m_WFECFG.Set( "UNM_KEYBINDS", HotKeyCvarList[m_WaitForKey-1], keyName );
    WFEWriteCFG( false );

    if( !cancel )
        HotKeyWidgetsList[m_WaitForKey-1]->setText( QString::fromUtf8( keyName.c_str( ) ) );

    m_DarkenedWidget->hide( );
    m_WaitForKey = 0;
}

void Widget::keyPressEvent( QKeyEvent *event )
{
    if( m_WaitForKey > 0 )
    {
        if( event->nativeScanCode( ) == 42 )
            LShift = true;
        else if( event->nativeScanCode( ) == 29 )
            LCtrl = true;
        else if( event->nativeScanCode( ) == 56 )
            LAlt = true;
        else if( event->nativeScanCode( ) == 54 )
            RShift = true;
        else if( event->nativeScanCode( ) == 285 )
            RCtrl = true;
        else if( event->nativeScanCode( ) == 312 )
            RAlt = true;
        else
        {
            string keyName;

            if( GetKeyName( event->nativeScanCode( ), keyName ) )
                ClickOnKeyPostSlot( keyName );
            else
            {

            }
        }
    }
}

void Widget::keyReleaseEvent( QKeyEvent *event )
{
    if( m_WaitForKey > 0 )
    {
        if( event->nativeScanCode( ) == 42 )
            LShift = false;
        else if( event->nativeScanCode( ) == 29 )
            LCtrl = false;
        else if( event->nativeScanCode( ) == 56 )
            LAlt = false;
        else if( event->nativeScanCode( ) == 54 )
            RShift = false;
        else if( event->nativeScanCode( ) == 285 )
            RCtrl = false;
        else if( event->nativeScanCode( ) == 312 )
            RAlt = false;
        else
        {
            string keyName;

            if( GetKeyName( event->nativeScanCode( ), keyName ) )
                ClickOnKeyPostSlot( keyName );
            else
            {

            }
        }
    }
}

void Widget::mousePressEvent( QMouseEvent *event )
{
    if( m_WaitForKey > 0 )
    {
        if( event->button( ) == Qt::LeftButton )
        {
            ClickOnKeyPostSlot( string( ) );
        }
        else if( event->button( ) == Qt::RightButton )
            ClickOnKeyPostSlot( string( ), true );
        else
        {

        }
    }

    // При клике левой кнопкой мыши

    if( !m_SimplifiedStyle && event->button( ) == Qt::LeftButton )
    {
        // Определяем, в какой области произошёл клик

        m_leftMouseButtonPressed = checkResizableField( event );
        setPreviousPosition( event->pos( ) ); // и устанавливаем позицию клика
    }
}

void Widget::mouseReleaseEvent( QMouseEvent *event )
{
    // При отпускании левой кнопки мыши сбрасываем состояние клика

    if( !m_SimplifiedStyle && event->button( ) == Qt::LeftButton )
        m_leftMouseButtonPressed = None;
}

void Widget::Spell01ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell01->SetGreenBorder( !ui->Spell01->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[0], ui->Spell01->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 1 );
    }
}

void Widget::Spell02ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell02->SetGreenBorder( !ui->Spell02->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[1], ui->Spell02->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 2 );
    }
}

void Widget::Spell03ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell03->SetGreenBorder( !ui->Spell03->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[2], ui->Spell03->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 3 );
    }
}

void Widget::Spell04ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell04->SetGreenBorder( !ui->Spell04->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[3], ui->Spell04->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 4 );
    }
}

void Widget::Spell05ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell05->SetGreenBorder( !ui->Spell05->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[4], ui->Spell05->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 5 );
    }
}

void Widget::Spell06ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell06->SetGreenBorder( !ui->Spell06->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[5], ui->Spell06->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 6 );
    }
}

void Widget::Spell07ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell07->SetGreenBorder( !ui->Spell07->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[6], ui->Spell07->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 7 );
    }
}

void Widget::Spell08ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell08->SetGreenBorder( !ui->Spell08->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[7], ui->Spell08->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 8 );
    }
}

void Widget::Spell09ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell09->SetGreenBorder( !ui->Spell09->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[8], ui->Spell09->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 9 );
    }
}

void Widget::Spell10ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell10->SetGreenBorder( !ui->Spell10->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[9], ui->Spell10->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 10 );
    }
}

void Widget::Spell11ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell11->SetGreenBorder( !ui->Spell11->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[10], ui->Spell11->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 11 );
    }
}

void Widget::Spell12ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Spell12->SetGreenBorder( !ui->Spell12->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[11], ui->Spell12->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 12 );
    }
}

void Widget::SpellBook01ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook01->SetGreenBorder( !ui->SpellBook01->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[12], ui->SpellBook01->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 13 );
    }
}

void Widget::SpellBook02ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook02->SetGreenBorder( !ui->SpellBook02->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[13], ui->SpellBook02->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 14 );
    }
}

void Widget::SpellBook03ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook03->SetGreenBorder( !ui->SpellBook03->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[14], ui->SpellBook03->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 15 );
    }
}

void Widget::SpellBook04ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook04->SetGreenBorder( !ui->SpellBook04->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[15], ui->SpellBook04->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 16 );
    }
}

void Widget::SpellBook05ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook05->SetGreenBorder( !ui->SpellBook05->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[16], ui->SpellBook05->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 17 );
    }
}

void Widget::SpellBook06ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook06->SetGreenBorder( !ui->SpellBook06->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[17], ui->SpellBook06->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 18 );
    }
}

void Widget::SpellBook07ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook07->SetGreenBorder( !ui->SpellBook07->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[18], ui->SpellBook07->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 19 );
    }
}

void Widget::SpellBook08ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook08->SetGreenBorder( !ui->SpellBook08->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[19], ui->SpellBook08->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 20 );
    }
}

void Widget::SpellBook09ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook09->SetGreenBorder( !ui->SpellBook09->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[20], ui->SpellBook09->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 21 );
    }
}

void Widget::SpellBook10ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook10->SetGreenBorder( !ui->SpellBook10->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[21], ui->SpellBook10->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 22 );
    }
}

void Widget::SpellBook11ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook11->SetGreenBorder( !ui->SpellBook11->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[22], ui->SpellBook11->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 23 );
    }
}

void Widget::SpellBook12ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->SpellBook12->SetGreenBorder( !ui->SpellBook12->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[23], ui->SpellBook12->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 24 );
    }
}

void Widget::Shop01ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop01->SetGreenBorder( !ui->Shop01->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[24], ui->Shop01->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 25 );
    }
}

void Widget::Shop02ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop02->SetGreenBorder( !ui->Shop02->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[25], ui->Shop02->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 26 );
    }
}

void Widget::Shop03ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop03->SetGreenBorder( !ui->Shop03->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[26], ui->Shop03->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 27 );
    }
}

void Widget::Shop04ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop04->SetGreenBorder( !ui->Shop04->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[27], ui->Shop04->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 28 );
    }
}

void Widget::Shop05ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop05->SetGreenBorder( !ui->Shop05->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[28], ui->Shop05->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 29 );
    }
}

void Widget::Shop06ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop06->SetGreenBorder( !ui->Shop06->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[29], ui->Shop06->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 30 );
    }
}

void Widget::Shop07ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop07->SetGreenBorder( !ui->Shop07->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[30], ui->Shop07->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 31 );
    }
}

void Widget::Shop08ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop08->SetGreenBorder( !ui->Shop08->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[31], ui->Shop08->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 32 );
    }
}

void Widget::Shop09ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop09->SetGreenBorder( !ui->Shop09->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[32], ui->Shop09->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 33 );
    }
}

void Widget::Shop10ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop10->SetGreenBorder( !ui->Shop10->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[33], ui->Shop10->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 34 );
    }
}

void Widget::Shop11ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop11->SetGreenBorder( !ui->Shop11->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[34], ui->Shop11->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 35 );
    }
}

void Widget::Shop12ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Shop12->SetGreenBorder( !ui->Shop12->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[35], ui->Shop12->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 36 );
    }
}

void Widget::Item1ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Item1->SetGreenBorder( !ui->Item1->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[36], ui->Item1->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 37 );
    }
}

void Widget::Item2ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Item2->SetGreenBorder( !ui->Item2->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[37], ui->Item2->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 38 );
    }
}

void Widget::Item3ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Item3->SetGreenBorder( !ui->Item3->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[38], ui->Item3->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 39 );
    }
}

void Widget::Item4ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Item4->SetGreenBorder( !ui->Item4->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[39], ui->Item4->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 40 );
    }
}

void Widget::Item5ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Item5->SetGreenBorder( !ui->Item5->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[40], ui->Item5->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 41 );
    }
}

void Widget::Item6ClickSlot( bool rightButton )
{
    if( m_WaitForKey == 0 )
    {
        if( rightButton )
        {
            ui->Item6->SetGreenBorder( !ui->Item6->GetGreenBorder( ) );
            m_WFECFG.Set( "UNM_SMARTCAST", HotKeyCvarList[41], ui->Item6->GetGreenBorder( ) ? "yes" : "no" );
            WFEWriteCFG( false );
        }
        else
            ClickOnKeySlot( 42 );
    }
}

bool Widget::eventFilter( QObject *object, QEvent *event )
{
    if( event->type( ) == QEvent::MouseMove )
    {
        if( m_SimplifiedStyle )
            return false;

        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>( event );

        if( mouseEvent )
        {
            if( object == this )
            {
                // При перемещении мыши, проверяем статус нажатия левой кнопки мыши

                switch( m_leftMouseButtonPressed )
                {
                case Move:
                {
                    // При этом проверяем, не максимизировано ли окно

                    if( isMaximized( ) )
                    {
                        // При перемещении из максимизированного состояния
                        // Необходимо вернуть окно в нормальное состояние и установить стили кнопки
                        // А также путём нехитрых вычислений пересчитать позицию окна,
                        // чтобы оно оказалось под курсором

                        ui->btn_maximize->setStyleSheet( "QToolButton{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_maximize_gray.png) } QToolButton:hover{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_maximize_white.png) } QToolButton:pressed{ min-width: 14px; min-height: 14px; max-width: 14px; max-height: 14px; width: 14px; height: 14px; background: transparent; border: transparent; color: transparent; border-image: url(:/window/window_maximize_orange.png) }" );
                        this->layout( )->setMargin( 9 );
                        auto part = mouseEvent->screenPos( ).x( ) / width( );
                        this->showNormal( );
                        auto offsetX = width( ) * part;
                        setGeometry( mouseEvent->screenPos( ).x( ) - offsetX, 0, width(), height( ) );
                        setPreviousPosition( QPoint( offsetX, mouseEvent->windowPos( ).y( ) ) );
                    }
                    else
                    {
                        // Если окно не максимизировано, то просто перемещаем его относительно
                        // последней запомненной позиции, пока не отпустим кнопку мыши

                        auto dx = mouseEvent->windowPos( ).x( ) - m_previousPosition.x( );
                        auto dy = mouseEvent->windowPos( ).y( ) - m_previousPosition.y( );
                        setGeometry( x( ) + dx, y( ) + dy, width( ), height( ) );
                    }
                    break;
                }
                case Top:
                {
                    // Для изменения размеров также проверяем на максимизацию
                    // поскольку мы же не можем изменить размеры у максимизированного окна

                    if( !isMaximized( ) )
                    {
                        auto dy = mouseEvent->windowPos( ).y( ) - m_previousPosition.y( );
                        setGeometry( x( ), y( ) + dy, width( ), height( ) - dy );
                    }
                    break;
                }
                case Bottom:
                {
                    if( !isMaximized( ) )
                    {
                        auto dy = mouseEvent->windowPos( ).y( ) - m_previousPosition.y( );
                        setGeometry( x( ), y( ), width( ), height( ) + dy );
                        setPreviousPosition( mouseEvent->windowPos( ).toPoint( ) );
                    }
                    break;
                }
                case Left:
                {
                    if( !isMaximized( ) )
                    {
                        auto dx = mouseEvent->windowPos( ).x( ) - m_previousPosition.x( );
                        setGeometry( x( ) + dx, y( ), width( ) - dx, height( ) );
                    }
                    break;
                }
                case Right:
                {
                    if( !isMaximized( ) )
                    {
                        auto dx = mouseEvent->windowPos( ).x( ) - m_previousPosition.x( );
                        setGeometry( x( ), y( ), width( ) + dx, height( ) );
                        setPreviousPosition( mouseEvent->windowPos( ).toPoint( ) );
                    }
                    break;
                }
                default:
                    // Если курсор перемещается по окну без зажатой кнопки,
                    // то просто отслеживаем в какой области он находится
                    // и изменяем его курсор

                    checkResizableField( mouseEvent );
                    break;
                }
            }
            else if( m_leftMouseButtonPressed == None )
            {
                if( object == ui->btn_minimize || object == ui->btn_maximize || object == ui->btn_close )
                {
                    setCursor( QCursor( ) );
                    return true;
                }
                else
                    checkResizableField( mouseEvent );
            }
        }
    }
    else if( event->type( ) == QEvent::KeyPress && ui->MainTab->currentIndex( ) == 0 )
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        if( keyEvent && keyEvent->nativeScanCode( ) == 49 && keyEvent->modifiers( ).testFlag( Qt::AltModifier ) && ui->ChatTab->count( ) > 0 && ui->ChatTab->currentIndex( ) > 0 && (dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->CurrentTabIndex( ) == 0 ) ) // 0 = irina, без чата
        {
            QList<QListWidgetItem *> selectedUsers = dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetChatMembers( )->selectedItems( );

            if( selectedUsers.size( ) == 1 )
            {
                QString currentText = dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetEntryField( )->text( );
                QString username = selectedUsers[0]->text( );

                if( currentText.length( ) > dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetEntryField( )->maxLength( ) )
                    currentText = currentText.left( dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetEntryField( )->maxLength( ) );

                if( currentText.length( ) + username.length( ) > dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetEntryField( )->maxLength( ) )
                    username = username.left( dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetEntryField( )->maxLength( ) - currentText.length( ) );

                int currentPos = dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetEntryField( )->cursorPosition( );

                if( currentPos > currentText.length( ) )
                    currentPos = currentText.length( );

                currentText.insert( currentPos, username );
                dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetEntryField( )->setText( currentText );
                dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetEntryField( )->setCursorPosition( currentPos + username.length( ) );
                dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetEntryField( )->setFocus( );
            }

            return true;
        }
    }
    else if( event->type( ) == QEvent::Wheel && ui->MainTab->currentIndex( ) == 4 )
    {
        if( object->objectName( ) == "" || object->objectName( ) == "Main" || object->objectName( ) == "MainscrollArea" || object->objectName( ) == "MainscrollAreaWidgetContents" || object->objectName( ) == "MainTab" || object->objectName( ) == "Games" || object->objectName( ) == "GamesscrollArea" || object->objectName( ) == "GamesscrollAreaWidgetContents" || object->objectName( ) == "GamesTab" || object->objectName( ) == "QComboBoxPrivateContainerClassWindow" || object->objectName( ) == "qt_scrollarea_vcontainer" || object->objectName( ) == "qt_scrollarea_viewport" || object->objectName( ) == "qt_tabwidget_stackedwidget" || object->objectName( ) == "qt_tabwidget_tabbar" || object->objectName( ) == "Settings" || object->objectName( ) == "SettingsTab" || object->objectName( ) == "Widget" || object->objectName( ) == "WidgetWindow" )
            return false;
        else
            return true;
    }

    return false;
}

Widget::~Widget( )
{
    ApplicationShutdown( );

    if( sWindow != nullptr && sWindow )
    {
        delete sWindow;
        sWindow = nullptr;
    }

    if( m_CornerLabel != nullptr && m_CornerLabel )
    {
        delete m_CornerLabel;
        m_CornerLabel = nullptr;
    }

    if( m_DarkenedWidget != nullptr && m_DarkenedWidget )
    {
        delete m_DarkenedWidget;
        m_DarkenedWidget = nullptr;
    }

    delete ui;
}

uint32_t Widget::GetTabIndexByGameNumber( uint32_t gameID, bool &tabfound )
{
    for( int32_t i = 0; i < ui->GameTab->count( ); ++i )
    {
        if( dynamic_cast<GameWidget*>( ui->GameTab->widget( i ) )->m_GameID == gameID )
        {
            tabfound = true;
            return i;
        }
    }

    return ui->GameTab->count( );
}

void Widget::ChangeCurrentMainTab( int index )
{
    if( index >= ui->MainTab->count( ) )
        index = ui->MainTab->count( ) - 1;

    if( ui->MainTab->currentIndex( ) != index )
        ui->MainTab->setCurrentIndex( index );
}

void Widget::ChangeCurrentGameTab( int index )
{
    if( index >= ui->GameTab->count( ) )
        index = ui->GameTab->count( ) - 1;

    if( ui->GameTab->currentIndex( ) != index )
        ui->GameTab->setCurrentIndex( index );
}

QPoint Widget::previousPosition( ) const
{
    return m_previousPosition;
}

void Widget::setPreviousPosition( QPoint previousPosition )
{
    if( m_previousPosition == previousPosition )
        return;

    m_previousPosition = previousPosition;
    emit previousPositionChanged( previousPosition );
}

Widget::MouseType Widget::checkResizableField( QMouseEvent *event )
{
    if( isMaximized( ) )
    {
        setCursor( QCursor( ) );
        return Move;
    }

    QPointF position = event->screenPos( );  // Определяем позицию курсора на экране
    qreal x = this->x( );                    // координаты окна приложения, ...
    qreal y = this->y( );                    // ... то есть координату левого верхнего угла окна
    qreal width = this->width( );            // А также ширину ...
    qreal height = this->height( );          // ... и высоту окна

    // Определяем области, в которых может находиться курсор мыши
    // По ним будет определён статус клика

    QRectF rectTop( x + 9, y, width - 18, 7 );
    QRectF rectBottom( x + 9, y + height - 7, width - 18, 7 );
    QRectF rectLeft( x, y + 9, 7, height - 18 );
    QRectF rectRight( x + width - 7, y + 9, 7, height - 18 );
    QRectF rectInterface( x + 9, y + 9, width - 18, height - 18 );

    // И в зависимости от области, в которой находится курсор
    // устанавливаем внешний вид курсора и возвращаем его статус

    if( rectTop.contains( position ) )
    {
        setCursor( Qt::SizeVerCursor );
        return Top;
    }
    else if( rectBottom.contains( position ) )
    {
        setCursor( Qt::SizeVerCursor );
        return Bottom;
    }
    else if( rectLeft.contains( position ) )
    {
        setCursor( Qt::SizeHorCursor );
        return Left;
    }
    else if( rectRight.contains( position ) )
    {
        setCursor( Qt::SizeHorCursor );
        return Right;
    }
    else if( rectInterface.contains( position ) )
    {
        setCursor( QCursor( ) );
        return Move;
    }
    else
    {
        setCursor( QCursor( ) );
        return None;
    }
}

void Widget::closeEvent( QCloseEvent *event )
{
    event->ignore( );

    if( gExitProcessStarted )
    {
        QMessageBox messageBox( QMessageBox::Question, tr( "Выход отсюда" ), tr( "Уже начался процесс выхода из программы.\nОкно автоматически закроется по завершению всех процессов." ), QMessageBox::Yes | QMessageBox::No, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "Выйти сейчас" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "Подождать" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::Yes )
        {
            gExitProcessStarted = true;
            qApp->quit( );

            if( gRestartAfterExiting )
                QProcess::startDetached( qApp->arguments( )[0], qApp->arguments( ) );
        }
    }
    else if( gUNM == nullptr )
    {
        QMessageBox messageBox( QMessageBox::Question, tr( "Выход отсюда" ), tr( "Закрыть все это?" ), QMessageBox::Yes | QMessageBox::No, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "Вырубить" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "Передумать" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::Yes )
        {
            gExitProcessStarted = true;
            qApp->quit( );

            if( gRestartAfterExiting )
                QProcess::startDetached( qApp->arguments( )[0], qApp->arguments( ) );
        }
    }
    else if( !gUNM->m_Games.empty( ) )
    {
        QMessageBox messageBox( QMessageBox::Question, tr( "Выход отсюда" ), tr( "Имеются начатые игры в процессе!\nВыйти немедленно?" ), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "Выйти сейчас" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "Подождать и выйти" ) );
        messageBox.setButtonText( QMessageBox::Cancel, tr( "Передумать" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::Yes )
        {
            gExitProcessStarted = true;
            gUNM->m_Exiting = true;
        }
        else if( resBtn == QMessageBox::No )
        {
            gExitProcessStarted = true;
            gUNM->m_ExitingNice = true;
        }
    }
    else if( gUNM->m_CurrentGame )
    {
        QMessageBox messageBox( QMessageBox::Question, tr( "Выход отсюда" ), tr( "Имеется неначатая игра процессе!\nВыйти немедленно?" ), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "Выйти" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "Выйти безопасно" ) );
        messageBox.setButtonText( QMessageBox::Cancel, tr( "Передумать" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::Yes )
        {
            gExitProcessStarted = true;
            gUNM->m_Exiting = true;
        }
        else if( resBtn == QMessageBox::No )
        {
            gExitProcessStarted = true;
            gUNM->m_ExitingNice = true;
        }
    }
    else
    {
        QMessageBox messageBox( QMessageBox::Question, tr( "Выход отсюда" ), tr( "Закрыть все это?" ), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "Выйти" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "Выйти безопасно" ) );
        messageBox.setButtonText( QMessageBox::Cancel, tr( "Передумать" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::Yes )
        {
            gExitProcessStarted = true;
            gUNM->m_Exiting = true;
        }
        else if( resBtn == QMessageBox::No )
        {
            gExitProcessStarted = true;
            gUNM->m_ExitingNice = true;
        }
    }
}

void Widget::paintEvent( QPaintEvent * )
{
    if( m_CornerLabel != nullptr && m_CornerLabel )
        m_CornerLabel->setGeometry( 0 + this->layout( )->margin(  ), height( ) - 77 - this->layout( )->margin(  ), 70, 75 );
}

void Widget::on_analyze_clicked( )
{
    /*if( !message.isEmpty( ) && message.front( ) == " " )
    {
        message.remove( 0, 1 );
        message.prepend( "&nbsp;" );
    }*/

    ui->outputfield->clear( );
    ui->outputfield->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
    ui->outputfield->appendPlainText( "UNM General Information" );
    ui->outputfield->appendPlainText( QString( ) );
    string CurrentMap;

    if( gUNM && gUNM != nullptr && gUNM->m_Map && gUNM->m_Map != nullptr && !gUNM->m_Map->GetMapLocalPath( ).empty( ) && gUNM->m_Map->GetMapFileSize( ) >= 0 )
        CurrentMap = "Текущая карта: " + gUNM->m_Map->GetMapLocalPath( ) + " (размер: " + UTIL_ToString( gUNM->m_Map->GetMapFileSize( ) ) + " байт)";
    else
        CurrentMap = "Текущая карта: не загружена...";

    QString QCurrentMap = QString::fromUtf8( CurrentMap.c_str( ) );
    ui->outputfield->appendPlainText( QCurrentMap );
    uint32_t CurrentLat = 50;

    for( vector<CBaseGame *> ::iterator i = gUNM->m_Games.begin( ); i != gUNM->m_Games.end( ); i++ )
    {
        if( ( *i )->GetNextTimedActionTicks( ) < CurrentLat )
            CurrentLat = ( *i )->GetNextTimedActionTicks( );
    }

    if( gUNM->m_CurrentGame && gUNM->m_CurrentGame->GetNextTimedActionTicks( ) < CurrentLat )
        CurrentLat = gUNM->m_CurrentGame->GetNextTimedActionTicks( );

    ui->outputfield->appendPlainText( "Текущий интервал обновления: " + UTIL_ToQString( CurrentLat ) + "мс" );
}

void Widget::on_clearfield_clicked( )
{
    ui->outputfield->clear( );
}

void Widget::on_BnetCommandsRadio_clicked( )
{
    ui->GameOwnerCheck->hide( );
    CommandListFilling( );
}

void Widget::on_GameCommandsRadio_clicked( )
{
    if( ui->RequiredAccesCombo->currentIndex( ) == 0 )
        ui->GameOwnerCheck->hide( );
    else
        ui->GameOwnerCheck->show( );

    CommandListFilling( );
}

void Widget::on_RequiredAccesCombo_currentIndexChanged( int index )
{
    if( index == 0 || ui->BnetCommandsRadio->isChecked( ) )
        ui->GameOwnerCheck->hide( );
    else
        ui->GameOwnerCheck->show( );

    CommandListFilling( );
}

void Widget::on_GameOwnerCheck_stateChanged( int )
{
    CommandListFilling( );
}

void Widget::CommandListFilling( )
{
    ui->CommandList->clear( );
    ui->CommandList->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );

    if( ui->BnetCommandsRadio->isChecked( ) || ui->GameCommandsRadio->isChecked( ) )
    {
        if( ui->RequiredAccesCombo->currentIndex( ) == 0 )
        {
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "about - посмотреть версию клиента" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "addipbl <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "addipblacklist <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "addiptobl <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "addiptoblacklist <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "autoinsult - включить marsauto в неначатой игре" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "automars - включить marsauto в неначатой игре" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "blacklistedip <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "blacklistip <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "blip <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "bnetchat <on/off> - включить межсерверный чат" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "bunny - нарисовать зайца" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "bunnys - нарисовать зайцев" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "but - сыграть в бутылочку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "butilochka - сыграть в бутылочку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "cfgcreate <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "cg - получить детальную информацию о всех играх, созданных через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "cgames - получить детальную информацию о всех играх, созданных через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "char <check/off/on/number> - задать префикс названия игры" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "charcustom <prefix-char> - задать кастомный префикс названия игры" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "chat <on/off> - включить межсерверный чат" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "chatbnet <on/off> - включить межсерверный чат" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "chatgame <gameid> - включить чат между игрой и каналом" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "configcreate <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "crab - нарисовать краба" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "createcfg <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "createconfig <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "currentgames - получить детальную информацию о всех играх, созданных через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "customchar <prefix-char> - задать кастомный префикс названия игры" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "customprefix <prefix-char> - задать кастомный префикс названия игры" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "date - посмотреть дату и время" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "deletemap <mapcfgname> - удалить карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "deletemapcfg <mapcfgname> - удалить конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "delmap <mapcfgname> - удалить карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "delmapcfg <mapcfgname> - удалить конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "dlmap <url> - скачать карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "dm <url> - скачать карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "dmap <mapcfgname> - удалить карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "dmapcfg <mapcfgname> - удалить конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "download <url> - скачать карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "downloadmap <url> - скачать карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "duck2 - нарисовать зеркальную утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "duck - нарисовать утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "end <gameid> - завершить начатую игру" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "enforcesg <replayname> - загрузить данные из реплея для !hostsg" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "exit <nice/normal/force> - закрыть клиент" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "fam - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "fixallmap - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "fixallmaps - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "fixmap - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "fixmaps - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "fm - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "fmap - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "gamechat <gameid> - включить чат между игрой и каналом" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "games - проверить текущий онлайн в играх, созданных через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "gc - обновить список соклановцев" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "get <cvarname> - получить значение квара в конфиге клиента" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "getclan - обновить список соклановцев" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "getfriends - обновить список друзей" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "getgame <gameid> - получить информацию об игре, созданной через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "getgames - получить информацию о всех играх, созданных через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "getnames - получить список названий всех текущих игр, созданных через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "gfs - обновить список друзей" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "gg <gameid> - получить информацию об игре, созданной через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "ggs - получить информацию о всех играх, созданных через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "gns - получить список названий всех текущих игр, созданных через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "hare - нарисовать зайца" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "hares - нарисовать зайцев" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "home - вернуться на первоначальный канал" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "host <mapcode> <gamename> - создать игру" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "hostsg <gamename> - создать игру по сохранению" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "hsg <gamename> - создать игру по сохранению" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "image <imagenumber> - отправить картинку на канал" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "img <imagenumber> - отправить картинку на канал" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "infogames - проверить текущий онлайн в играх, созданных через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "ipbl <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "ipblacklist <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "ipblacklisted <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "iptobl <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "iptoblacklist <ip> - добавить ip в blacklist" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "kcud - нарисовать зеркальную утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "ktu - нарисовать зеркальную утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "kty - нарисовать зеркальную утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "l - перезагрузить языковый файл" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "lang - перезагрузить языковый файл" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "language - перезагрузить языковый файл" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "lm <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "lmap <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "load <cfg> - загрузить конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "loadm <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "loadmap <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "loadsg <savegamename> - загрузить сохранение игры" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "lsg <savegamename> - загрузить сохранение игры" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "map <map> - загрузить карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mapcfgd <mapcfgname> - удалить конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mapcfgdel <mapcfgname> - удалить конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mapcfgdelete <mapcfgname> - удалить конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mapcfgremove <mapcfgname> - удалить конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mapd <mapcfgname> - удалить карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mapdel <mapcfgname> - удалить карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mapdelete <mapcfgname> - удалить карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mapl <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mapload <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mapremove <mapcfgname> - удалить карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mars <name> - шуткануть" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "ml <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mload <mapname> - создать конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mouse2 - нарисовать зеркальную мышь" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "mouse - нарисовать мышь" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "noreplays - включить сохранение реплеев на стороне клиента (в играх созданных через команды \\pub или \\host)" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "p <gamename> - создать игру" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "pic <imagenumber> - отправить картинку на канал" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "picture <imagenumber> - отправить картинку на канал" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "players - проверить текущий онлайн в играх, созданных через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "pr <gamename> - создать приватную игру" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "prefix <check/off/on/number> - задать префикс названия игры" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "prefixcustom <prefix-char> - задать кастомный префикс названия игры" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "pri <gamename> - создать приватную игру" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "priv <gamename> - создать приватную игру" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "pub <gamename> - создать игру" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "quit <nice/normal/force> - выключить клиент" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "rabbit - нарисовать зайца" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "rabbits - нарисовать зайцев" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "ram - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "rcfg - перезагрузить конфиг клиента" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "rd <seconds> - задать интервал авторехоста (актуально для игр созданных через команды \\pub или \\host)" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "reboot <now/yes/safe/wait> - перезапустить клиент" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "refreshdelay <seconds> - задать интервал авторехоста (актуально для игр созданных через команды \\pub или \\host)" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "rehostdelay <seconds> - задать интервал авторехоста (актуально для игр созданных через команды \\pub или \\host)" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "reload - перезагрузить конфиг клиента" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "reloadcfg - перезагрузить конфиг клиента" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "reloadconfig - перезагрузить конфиг клиента" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "removemap <mapcfgname> - удалить карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "removemapcfg <mapcfgname> - удалить конфиг карты" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "renameallmap - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "renameallmaps - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "renamemap - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "renamemaps - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "replays - включить сохранение реплеев на стороне клиента (в играх созданных через команды \\pub или \\host)" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "replayssave - включить сохранение реплеев на стороне клиента (в играх созданных через команды \\pub или \\host)" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "restart <now/yes/safe/wait> - перезапустить клиент" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "rm - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "rmap - проверить и исправить названия всех карт" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "roll <max> <reason> - роллить" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "savereplays - включить сохранение реплеев на стороне клиента (в играх созданных через команды \\pub или \\host)" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "saygame <gameid> <message> - отправить сообщение в игру, созданную через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "saygames <message> - отправить сообщение во все игры, созданные через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "set <cvarname> <value> - поменять значение квара в конфиге клиента" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "setchar <prefix-char> - задать кастомный префикс названия игры" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "setnew <cvarname> <value> - добавить/изменить квар в конфиге клиента" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "setprefix <prefix-char> - задать кастомный префикс названия игры" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "sg <gameid> <message> - отправить сообщение в игру, созданную через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "sgs <message> - отправить сообщение во все игры, созданные через команды \\pub или \\host" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "slap <name> - подшутить" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "squirrel - нарисовать белку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "time - посмотреть дату и время" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "uh - отменить не начатую игру (созданную через команды \\pub или \\host)" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "unhost - отменить не начатую игру (созданную через команды \\pub или \\host)" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "utk2 - нарисовать зеркальную утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "utk - нарисовать утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "v - посмотреть версию клиента" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "version - посмотреть версию клиента" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "wimage <username> <imagenumber> - отправить картинку в лс" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "wimg <username> <imagenumber> - отправить картинку в лс" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "wpic <username> <imagenumber> - отправить картинку в лс" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "wpicture <username> <imagenumber> - отправить картинку в лс" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "ytk2 - нарисовать зеркальную утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "ytk - нарисовать утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "белка - нарисовать белку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "белочка - нарисовать белку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "бут - сыграть в бутылочку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "бутылочка - сыграть в бутылочку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "заи - нарисовать зайцев" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "зай - нарисовать зайца" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "зайи - нарисовать зайцев" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "зайцы - нарисовать зайцев" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "зайчик - нарисовать зайца" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "зайчики - нарисовать зайцев" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "зая - нарисовать зайца" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "заяц - нарисовать зайца" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "карта <map> - загрузить карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "краб - нарисовать краба" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "крабик - нарисовать краба" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "кту - нарисовать зеркальную утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "мышь2 - нарисовать зеркальную мышь" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "мышь - нарисовать мышь" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "префикс <check/off/on/number> - задать префикс названия игры" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "скачать <url> - скачать карту" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "утк2 - нарисовать зеркальную утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "утк - нарисовать утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "утка2 - нарисовать зеркальную утку" );
            ui->CommandList->appendPlainText( gBNETCommandTrigger + "утка - нарисовать утку" );
        }
        else
        {

        }
    }

    ui->CommandList->verticalScrollBar( )->setValue( 0 );
}

void Widget::on_ChangelogCombo_currentIndexChanged( int index )
{
    ui->ChangelogText->clear( );
    ui->ChangelogText->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
    ChangelogFilling( index );
}

void Widget::ChangelogFilling( int version )
{
    if( version == 0 )
        ui->ChangelogText->appendPlainText( "Выберите пункт выше." );
    else if( version == 1 )
    {
/*        for( int i = 2; i < ui->ChangelogCombo->count( ); i++ )
        {
            ChangelogFilling( i );

            if( i + 1 != ui->ChangelogCombo->count( ) )
                ui->ChangelogText->appendPlainText( QString( ) );
        }*/
    }
/*    else if( version == 2 )
    {
        ui->ChangelogText->appendPlainText( "version 1.2.10 (09.12.2020)" );
    }*/

    ui->ChangelogText->verticalScrollBar( )->setValue( 0 );
}

void Widget::on_SelectItemBox_currentIndexChanged( int index )
{
    ui->HelpText->clear( );
    ui->HelpText->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );

    if( index == 0 )
        ui->HelpText->appendPlainText( "Выберите пункт выше." );
    else if( index == 1 )
    {
        vector<string> defaultcfg;
        string file;
        std::ifstream in;

#ifdef WIN32
        file = "text_files\\default.cfg";
        in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
        file = "text_files/default.cfg";
        in.open( file );
#endif

        if( !in.fail( ) )
        {
            string Line;

            while( !in.eof( ) )
            {
                getline( in, Line );
                Line = UTIL_FixFileLine( Line );

                if( Line == "### YOUR SETTINGS" )
                    break;
                else if( Line.size( ) > 2 && Line[0] == '#' && Line[1] != ' ' && Line[1] != '#' )
                    Line = Line.substr( 1 );

                defaultcfg.push_back( Line );
            }

            if( defaultcfg.empty( ) )
                defaultcfg.push_back( "Ошибка при чтении файла [" + file + "]" );
            else
            {
                while( defaultcfg.back( ).find_first_not_of( " #" ) == string::npos )
                {
                    defaultcfg.pop_back( );

                    if( defaultcfg.empty( ) )
                    {
                        defaultcfg.push_back( "Ошибка при чтении файла [" + file + "]" );
                        break;
                    }
                }
            }
        }
        else
            defaultcfg.push_back( "Не удалось прочитать файл [" + file + "]" );

        in.close( );

        for( uint32_t i = 0; i < defaultcfg.size( ); i++ )
            ui->HelpText->appendPlainText( QString::fromUtf8( defaultcfg[i].c_str( ) ) );
    }
    else if( index == 2 )
    {
        ui->HelpText->appendPlainText( "В окне входа представляется возможным указать главные настройки подключения к Battle.net/PVPGN серверу (адрес сервера, логин и пароль), однако стоит понимать, что это далеко не все опции. Полный список настроек для соединения с сервером (например, тип сервера, версия, патч...) находится в конфиг-файле [unm.cfg]. Также в конфиге можно установить больше подключений (добавить другие сервера или подключиться с нескольких аккаунтов на один сервер)." );
        ui->HelpText->appendPlainText( "Если не удалось подключиться к серверу, первым делом проверьте правильность введенных данных. Далее необходимо заглянуть в конфиг и проверить указана верная ли версия сервера. Необходимо смотреть на квары, начинающиеся с \"bnet_\". Обратите внимание, квары \"bnet_\" отвечают за первое подключения, квары \"bnet2_\" - за второе, \"bnet3_\" - третье, можно задать неограниченное количество подключений. Квары \"bnet_custom_\" позволяют указать тип и версию сервера (необходимые значения гуглим для каждого сервера отдельно). С полным описанием всех кваров можно ознакомиться в базовом конфиг-файле [text_files\\default.cfg]. Имейте в виду, строки начинающиеся с символа \"#\" не читаются клиентом, в таком случае будут использоваться стандартные значения кваров. Стандартные значения каждого квара изложены в базовом конфиг-файле." );
        ui->HelpText->appendPlainText( "Также клиент может запустить анти-хак, необходимый для подключения к PVPGN серверу iCCup. Галочка \"Запустить анти-хак\" в окне входа эквивалентна квару в конфиге \"bnet_runiccuploader\". Активация этой галочки запускает анти-хак только для первого подключения (bnet_server), если в конфиге задано более одного подключения к айкапу - следует включить запуск анти-хака для каждого (bnetX_runiccuploader = 1, где X = номер подключения к айкапу). Запуск анти-хака необходимо включать только для сервера iCCup, для других серверов эта опция будет отключена автоматически. Также следует обратить внимания на квар \"bnet_port\", значения для каждого подключения к iCCup - должно быть разным, а значение для других серверов должно быть всегда \"bnetX_port = 6112\". Если ваш аккаунт на айкапе имеет cg2, это позволяет подключаться к айкапу без запуска анти-хака, также для данного подключения следует указать \"bnetX_port = 6112\"." );
        ui->HelpText->appendPlainText( "Клиент автоматически не обновляет анти-хак, это следует делать вручную." );
    }
    else if( index == 3 )
    {
        ui->HelpText->appendPlainText( "iCCup Reconnecnt Loader (анти-хак):" );
        ui->HelpText->appendPlainText( "IccReconnecntLoader.exe не обновляются сам, его нужно обновлять вручную. Для этого нужно скопировать файлы \"iccwc3.icc\" и \"Reconnect.dll\" из папки с обычным лаунчером последней версии в папку \"iCCupLoader\" (которая расположена в корневой папке клиента)." );
        ui->HelpText->appendPlainText( "IccReconnecntLoader позволяет выбрать уникальный порт, который будет использовать античит для подключения клиента к айкапу. Таким образом, используя разные порты, мы можем подключить одновременно несколько логинов (или параллельно запущенных клиентов) с одного пк на iCCup. Также, используя обычный лаунчер, мы сможем зайти через war3 с того же пк, на котором запущен клиент (или несколько клиентов)." );
        ui->HelpText->appendPlainText( "Как пользоваться IccReconnecntLoader.exe:" );
        ui->HelpText->appendPlainText( "В конфиге клиента для каждого логина iCCup включаем автозапуск анти-хака (квар bnet_runiccuploader) и выбираем уникальный порт, который будет использоваться античитом для подключения клиента к айкапу (квар bnet_port), запускаем клиент и на этапе ввода логина/пароля ставим галочку напротив \"Запустить анти-хак\", заходим..." );
        ui->HelpText->appendPlainText( "Следует понимать, что галочка \"Запустить анти-хак\" отвечает только за первый по счету логин, указанный в конфиге (bnet_server), разрешить/запретить автозапуск анти-хака для остальных логинов (bnet2_server, bnet3_server и последующих) имеется возможным только в конфиге клиента (квар bnetX_runiccuploader)" );
        ui->HelpText->appendPlainText( "Также можно запускать анти-чит вручную: в конфиге клиента отключаем автозапуск анти-хака для каждого логина iCCup (квар bnet_runiccuploader), запускаем \"IccReconnecntLoader.exe\" (находится в папке \"iCCupLoader\"), в открывшимся окне указываем порт (если собираемся подключить к айкапу несколько логинов, то запускаем несколько экземпляров \"IccReconnecntLoader.exe\" - по одному на каждый логин, и в каждом экземпляре указываем уникальный порт), далее запускаем клиент и на этапе ввода логина/пароля снимаем галочку напротив \"Запустить анти-хак\", заходим..." );
        ui->HelpText->appendPlainText( "Обратите внимание, если вы используете автозапуск анти-хака, то вы не должны его запускать вручную, и наоборот, если вы запускаете анти-хак вручную вы должны убрать галочку/отключить автозапуск анти-хака в конфиге (для каждого логина, для которого хотите запустить анти-хак вручную). Кроме того, если у вас имеется cg2 вы можете подключаться к айкапу без использования анти-хака (bnet_runiccuploader = 0, bnet_port = 6112). Для других серверов значение квара bnet_runiccuploader будет игнорироваться, а вот bnet_port должен быть всегда 6112" );
    }

    ui->HelpText->verticalScrollBar( )->setValue( 0 );
}

void Widget::write_to_log( QString message )
{
    message.replace( "<", "&lt;" );
    message.replace( ">", "&gt;" );
    message.replace( "  ", " &nbsp;" );
    message.replace( "  ", "&nbsp; " );
    ui->logfield->appendHtml( "<font color = " + gNormalMessagesColor + ">" + message + "</font>" );
}

void Widget::write_to_game_log( unsigned int gameID, QString message, bool toMainLog )
{
    for( int32_t n = 0; n < ui->GameTab->count( ); ++n )
    {
        if( dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->m_GameID == gameID )
        {
            dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->write_to_log_direct( message );
            break;
        }
    }

    if( toMainLog )
    {
        message.replace( "<", "&lt;" );
        message.replace( ">", "&gt;" );
        message.replace( "  ", " &nbsp;" );
        message.replace( "  ", "&nbsp; " );
        ui->logfield->appendHtml( "<font color = " + gNormalMessagesColor + ">" + message + "</font>" );
    }
}

void Widget::write_to_chat( QString message, QString msg, int bnetID, int mainBnetID )
{
    dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->write_to_chat_direct( message, msg );

    if( mainBnetID != -1 )
        dynamic_cast<ChatWidget*>( ui->ChatTab->widget( mainBnetID ) )->write_to_chat_direct( message, msg );
}

void Widget::add_channel_user( QString message, int bnetID )
{
    dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->GetChatMembers( )->addItem( message );
}

void Widget::remove_channel_user( QString message, int bnetID )
{
    auto itemsToRemove = dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->GetChatMembers( )->findItems( message, Qt::MatchExactly );

    for( auto item : itemsToRemove )
        delete item;
}

void Widget::clear_channel_user( int bnetID )
{
    dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->GetChatMembers( )->clear( );
}

void Widget::bnet_disconnect( int bnetID )
{
    dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->bnet_disconnect_direct( );
}

void Widget::bnet_reconnect( int bnetID )
{
    dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->bnet_reconnect_direct( );
}

void Widget::set_commands_tips( bool frist )
{
    if( !frist )
    {
        for( int32_t n = 1; n < ui->ChatTab->count( ); ++n )
        {
            auto p = dynamic_cast<ChatWidget*>( ui->ChatTab->widget( n ) )->GetEntryField( )->completer( );

            if( p )
                p->deleteLater( );
        }
    }

    QString str = QString( );
    str = "about - посмотреть версию клиента,.addipbl <ip> - добавить ip в blacklist,.addipblacklist <ip> - добавить ip в blacklist,.addiptobl <ip> - добавить ip в blacklist,.addiptoblacklist <ip> - добавить ip в blacklist,.autoinsult - включить marsauto в неначатой игре,.automars - включить marsauto в неначатой игре,.blacklistedip <ip> - добавить ip в blacklist,.blacklistip <ip> - добавить ip в blacklist,.blip <ip> - добавить ip в blacklist,.bnetchat <on/off> - включить межсерверный чат,.bunny - нарисовать зайца,.bunnys - нарисовать зайцев,.but - сыграть в бутылочку,.butilochka - сыграть в бутылочку,.cfgcreate <mapname> - создать конфиг карты,.cg - получить детальную информацию о всех играх, созданных через команды \\pub или \\host,.cgames - получить детальную информацию о всех играх, созданных через команды \\pub или \\host,.char <check/off/on/number> - задать префикс названия игры,.charcustom <prefix-char> - задать кастомный префикс названия игры,.chat <on/off> - включить межсерверный чат,.chatbnet <on/off> - включить межсерверный чат,.chatgame <gameid> - включить чат между игрой и каналом,.configcreate <mapname> - создать конфиг карты,.crab - нарисовать краба,.createcfg <mapname> - создать конфиг карты,.";
    str += "createconfig <mapname> - создать конфиг карты,.currentgames - получить детальную информацию о всех играх, созданных через команды \\pub или \\host,.customchar <prefix-char> - задать кастомный префикс названия игры,.customprefix <prefix-char> - задать кастомный префикс названия игры,.date - посмотреть дату и время,.deletemap <mapcfgname> - удалить карту,.deletemapcfg <mapcfgname> - удалить конфиг карты,.delmap <mapcfgname> - удалить карту,.delmapcfg <mapcfgname> - удалить конфиг карты,.dlmap <url> - скачать карту,.dm <url> - скачать карту,.dmap <mapcfgname> - удалить карту,.dmapcfg <mapcfgname> - удалить конфиг карты,.download <url> - скачать карту,.downloadmap <url> - скачать карту,.duck2 - нарисовать зеркальную утку,.duck - нарисовать утку,.end <gameid> - завершить начатую игру,.enforcesg <replayname> - загрузить данные из реплея для !hostsg,.exit <nice/normal/force> - закрыть клиент,.fam - проверить и исправить названия всех карт,.fixallmap - проверить и исправить названия всех карт,.fixallmaps - проверить и исправить названия всех карт,.fixmap - проверить и исправить названия всех карт,.fixmaps - проверить и исправить названия всех карт,.fm - проверить и исправить названия всех карт,.fmap - проверить и исправить названия всех карт,.gamechat <gameid> - включить чат между игрой и каналом,.games - проверить текущий онлайн в играх, созданных через команды \\pub или \\host,.gc - обновить список соклановцев,.get <cvarname> - получить значение квара в конфиге клиента,.getclan - обновить список соклановцев,.getfriends - обновить список друзей,.getgame <gameid> - получить информацию об игре, созданной через команды \\pub или \\host,.getgames - получить информацию о всех играх, созданных через команды \\pub или \\host,.getnames - получить список названий всех текущих игр, созданных через команды \\pub или \\host,.gfs - обновить список друзей,.gg <gameid> - получить информацию об игре, созданной через команды \\pub или \\host,.ggs - получить информацию о всех играх, созданных через команды \\pub или \\host,.gns - получить список названий всех текущих игр, созданных через команды \\pub или \\host,.hare - нарисовать зайца,.hares - нарисовать зайцев,.home - вернуться на первоначальный канал,.host <mapcode> <gamename> - создать игру,.";
    str += "hostsg <gamename> - создать игру по сохранению,.hsg <gamename> - создать игру по сохранению,.image <imagenumber> - отправить картинку на канал,.img <imagenumber> - отправить картинку на канал,.infogames - проверить текущий онлайн в играх, созданных через команды \\pub или \\host,.ipbl <ip> - добавить ip в blacklist,.ipblacklist <ip> - добавить ip в blacklist,.ipblacklisted <ip> - добавить ip в blacklist,.iptobl <ip> - добавить ip в blacklist,.iptoblacklist <ip> - добавить ip в blacklist,.kcud - нарисовать зеркальную утку,.ktu - нарисовать зеркальную утку,.kty - нарисовать зеркальную утку,.l - перезагрузить языковый файл,.lang - перезагрузить языковый файл,.language - перезагрузить языковый файл,.lm <mapname> - создать конфиг карты,.lmap <mapname> - создать конфиг карты,.load <cfg> - загрузить конфиг карты,.loadm <mapname> - создать конфиг карты,.loadmap <mapname> - создать конфиг карты,.loadsg <savegamename> - загрузить сохранение игры,.lsg <savegamename> - загрузить сохранение игры,.map <map> - загрузить карту,.mapcfgd <mapcfgname> - удалить конфиг карты,.mapcfgdel <mapcfgname> - удалить конфиг карты,.mapcfgdelete <mapcfgname> - удалить конфиг карты,.mapcfgremove <mapcfgname> - удалить конфиг карты,.mapd <mapcfgname> - удалить карту,.mapdel <mapcfgname> - удалить карту,.mapdelete <mapcfgname> - удалить карту,.mapl <mapname> - создать конфиг карты,.mapload <mapname> - создать конфиг карты,.mapremove <mapcfgname> - удалить карту,.mars <name> - шуткануть,.ml <mapname> - создать конфиг карты,.mload <mapname> - создать конфиг карты,.mouse2 - нарисовать зеркальную мышь,.mouse - нарисовать мышь,.noreplays - включить сохранение реплеев на стороне клиента (в играх созданных через команды \\pub или \\host),.p <gamename> - создать игру,.";
    str += "pic <imagenumber> - отправить картинку на канал,.picture <imagenumber> - отправить картинку на канал,.players - проверить текущий онлайн в играх, созданных через команды \\pub или \\host,.pr <gamename> - создать приватную игру,.prefix <check/off/on/number> - задать префикс названия игры,.prefixcustom <prefix-char> - задать кастомный префикс названия игры,.pri <gamename> - создать приватную игру,.priv <gamename> - создать приватную игру,.pub <gamename> - создать игру,.quit <nice/normal/force> - выключить клиент,.rabbit - нарисовать зайца,.rabbits - нарисовать зайцев,.ram - проверить и исправить названия всех карт,.rcfg - перезагрузить конфиг клиента,.rd <seconds> - задать интервал авторехоста (актуально для игр созданных через команды \\pub или \\host),.reboot <now/yes/safe/wait> - перезапустить клиент,.refreshdelay <seconds> - задать интервал авторехоста (актуально для игр созданных через команды \\pub или \\host),.rehostdelay <seconds> - задать интервал авторехоста (актуально для игр созданных через команды \\pub или \\host),.reload - перезагрузить конфиг клиента,.reloadcfg - перезагрузить конфиг клиента,.reloadconfig - перезагрузить конфиг клиента,.removemap <mapcfgname> - удалить карту,.removemapcfg <mapcfgname> - удалить конфиг карты,.renameallmap - проверить и исправить названия всех карт,.renameallmaps - проверить и исправить названия всех карт,.renamemap - проверить и исправить названия всех карт,.renamemaps - проверить и исправить названия всех карт,.replays - включить сохранение реплеев на стороне клиента (в играх созданных через команды \\pub или \\host),.replayssave - включить сохранение реплеев на стороне клиента (в играх созданных через команды \\pub или \\host),.restart <now/yes/safe/wait> - перезапустить клиент,.rm - проверить и исправить названия всех карт,.rmap - проверить и исправить названия всех карт,.roll <max> <reason> - роллить,.savereplays - включить сохранение реплеев на стороне клиента (в играх созданных через команды \\pub или \\host),.saygame <gameid> <message> - отправить сообщение в игру, созданную через команды \\pub или \\host,.saygames <message> - отправить сообщение во все игры, созданные через команды \\pub или \\host,.set <cvarname> <value> - поменять значение квара в конфиге клиента,.setchar <prefix-char> - задать кастомный префикс названия игры,.setnew <cvarname> <value> - добавить/изменить квар в конфиге клиента,.setprefix <prefix-char> - задать кастомный префикс названия игры,.sg <gameid> <message> - отправить сообщение в игру, созданную через команды \\pub или \\host,.sgs <message> - отправить сообщение во все игры, созданные через команды \\pub или \\host,.slap <name> - подшутить,.squirrel - нарисовать белку,.";
    str += "time - посмотреть дату и время,.uh - отменить не начатую игру (созданную через команды \\pub или \\host),.unhost - отменить не начатую игру (созданную через команды \\pub или \\host),.utk2 - нарисовать зеркальную утку,.utk - нарисовать утку,.v - посмотреть версию клиента,.version - посмотреть версию клиента,.wimage <username> <imagenumber> - отправить картинку в лс,.wimg <username> <imagenumber> - отправить картинку в лс,.wpic <username> <imagenumber> - отправить картинку в лс,.wpicture <username> <imagenumber> - отправить картинку в лс,.ytk2 - нарисовать зеркальную утку,.ytk - нарисовать утку,.белка - нарисовать белку,.белочка - нарисовать белку,.бут - сыграть в бутылочку,.бутылочка - сыграть в бутылочку,.заи - нарисовать зайцев,.зай - нарисовать зайца,.зайи - нарисовать зайцев,.зайцы - нарисовать зайцев,.зайчик - нарисовать зайца,.зайчики - нарисовать зайцев,.зая - нарисовать зайца,.заяц - нарисовать зайца,.карта <map> - загрузить карту,.краб - нарисовать краба,.крабик - нарисовать краба,.кту - нарисовать зеркальную утку,.мышь2 - нарисовать зеркальную мышь,.мышь - нарисовать мышь,.префикс <check/off/on/number> - задать префикс названия игры,.скачать <url> - скачать карту,.утк2 - нарисовать зеркальную утку,.утк - нарисовать утку,.утка2 - нарисовать зеркальную утку,.утка - нарисовать утку";
    m_StringList = str.split(",.");

    for( int32_t n = 1; n < ui->ChatTab->count( ); ++n )
    {
        QStringList nStringList = m_StringList;

        for( int32_t i = 0; i < nStringList.size( ); ++i )
             nStringList[i] = dynamic_cast<ChatWidget*>( ui->ChatTab->widget( n ) )->GetBnetCommandTrigger( ) + nStringList[i];

        QCompleter *fileEditCompleter = new QCompleter( nStringList, this );
        fileEditCompleter->setCaseSensitivity( Qt::CaseInsensitive );
        fileEditCompleter->setCompletionMode( QCompleter::PopupCompletion );
        dynamic_cast<ChatWidget*>( ui->ChatTab->widget( n ) )->GetEntryField( )->setCompleter( fileEditCompleter );
    }
}

void Widget::show_main_window( )
{
    show( );
    TabsResizeFix( 6 );

    if( gEnableLoggingToGUI == 2 )
    {
        m_LoggingMode = 1;

        if( !gLogFile.empty( ) && gLogMethod != 0 )
            ui->logEnable->setText( "Отключить логирование в этом окне" );
    }
    else
    {
        m_LoggingMode = gEnableLoggingToGUI;

        if( !gLogFile.empty( ) && gLogMethod != 0 )
            ui->logEnable->setText( "Включить логирование в этом окне" );
    }

    if( gLogFile.empty( ) || gLogMethod == 0 )
    {
        ui->logBackup->setEnabled( false );
        ui->logClear->setEnabled( false );
        ui->logEnable->setEnabled( false );
        ui->logEnable->setText( "Включить логирование в этом окне" );
    }

    if( sWindow != nullptr && sWindow )
    {
        delete sWindow;
        sWindow = nullptr;
    }

    if( ui->MainTab->currentIndex( ) == 0 && ui->ChatTab->count( ) > 0 && ui->ChatTab->currentIndex( ) > 0 && (dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->CurrentTabIndex( ) == 0 ) )
        dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetEntryField( )->setFocus( );
}

void Widget::finished_unm( bool mainThread )
{
    gExitProcessStarted = true;

    if( mainThread )
    {
        thread_1.quit( );

        if( !gAltThreadState.empty( ) )
            gAltThreadState = "@FinishedUNM";
        else
        {
            qApp->quit( );

            if( gRestartAfterExiting )
                QProcess::startDetached( qApp->arguments( )[0], qApp->arguments( ) );
        }

    }
    else
    {
        thread_3.quit( );
        qApp->quit( );

        if( gRestartAfterExiting )
            QProcess::startDetached( qApp->arguments( )[0], qApp->arguments( ) );
    }
}

void Widget::thread_ready( bool mainThread )
{
    if( mainThread )
        thread_1.quit( );
    else
        thread_2.quit( );

    if( !ready )
        ready = true;
    else
    {
        if( gUNM && gUNM != nullptr && gUNM->m_BNETs.size( ) > 0 )
        {
            if( gWindowTitle.find_first_not_of( " " ) != string::npos )
            {
                if( gWindowTitle != "unm" )
                    gWindowTitle = "unm (" + gWindowTitle + ")";

                QString windowtitleqstr = QString::fromUtf8( gWindowTitle.c_str( ) );
                setWindowTitle( windowtitleqstr );
                ui->label_title->setText( windowtitleqstr );
                gWindowTitle.clear( );
            }

            if( ui->ChatTab->count( ) >= 2 )
                ui->ChatTab->setCurrentIndex( 1 );

            if( mainThread )
                QThread::msleep(50);

            if( gUNM->m_BNETs[0]->m_FirstConnect )
            {
                if( !gSkipPresetting )
                {
                    PresettingWindow pWindow;
                    pWindow.setModal( true );
                    pWindow.exec( );
                }

                UpdateValuesInGUI( { } );
                gUNM->m_BNETs[0]->ChangeLogonData( LogonDataServer, LogonDataLogin, LogonDataPassword, LogonDataRunAH );
                exampleObject_1.setRunning( 1 );
                thread_1.start( );
            }
            else
                gUNM->m_BNETs[0]->ChangeLogonData( LogonDataServer, LogonDataLogin, LogonDataPassword, LogonDataRunAH );
        }
        else
        {
            string messageforqstr = "Что-то пошло не так, требуется перезапуск.";
            QString messageqstr = QString::fromUtf8( messageforqstr.c_str( ) );
            emit gToLog->logonFailed( messageqstr );
        }
    }
}

void Widget::run_thread( bool runAH )
{
    if( runAH )
        exampleObject_2.setRunning( 3 );
    else
        exampleObject_2.setRunning( 2 );

    thread_2.start( );
}

void Widget::download_map( QList<QString> options )
{
    exampleObject_3.setRunning( 6 );
    exampleObject_3.setOptions( options );
    thread_3.start( );
}

void Widget::restart_UNM( bool now )
{
    if( now )
    {
        ui->restartUNM->setIcon( QIcon( ":/mainwindow/tools/restart_red.png" ) );
        qApp->quit( );
        QProcess::startDetached( qApp->arguments( )[0], qApp->arguments( ) );
    }
    else
        ui->restartUNM->setIcon( QIcon( ":/mainwindow/tools/restart_red.png" ) );
}

void Widget::add_chat_tab( unsigned int bnetID, QString bnetCommandTrigger, QString serverName, QString userName, unsigned int timeWaitingToReconnect )
{
    if( bnetCommandTrigger.isEmpty( ) )
        ui->ChatTab->addTab( new ChatWidgetIrina( this ), serverName );
    else if( !userName.isEmpty( ) )
        ui->ChatTab->addTab( new ChatWidget( this, bnetID, bnetCommandTrigger, serverName, userName, timeWaitingToReconnect ), serverName + " (" + userName + ")" );
    else
        ui->ChatTab->addTab( new ChatWidget( this, bnetID, bnetCommandTrigger, serverName, userName, timeWaitingToReconnect ), serverName );

    TabsResizeFix( 0 );
}

void Widget::change_bnet_username( unsigned int bnetID, QString userName )
{
    if( ui->ChatTab->count( ) > static_cast<int>(bnetID) )
    {
        dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->m_UserName = userName;
        QString tabName = dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->m_ServerName + " (" + userName + ")";
        ui->ChatTab->setTabText( bnetID, tabName );
    }
}

void Widget::add_game( unsigned int gameID, QStringList gameInfoInit, QList<QStringList> playersInfo )
{
    ui->GameTab->addTab( new GameWidget( this, gameID, gameInfoInit, playersInfo ), gameInfoInit[0] );

    if( ui->GameTab->count( ) == 1 )
    {
        m_GameTabColor = 1;
        ui->MainTab->setTabIcon( 1, QIcon( ":/mainwindow/games_yellow.png" ) );
    }
}

void Widget::delete_game( unsigned int gameID )
{
    for( int32_t n = 0; n < ui->GameTab->count( ); ++n )
    {
        if( dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->m_GameID == gameID )
        {
            (dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) ))->deleteLater( );
            ui->GameTab->removeTab( n );
            break;
        }
    }

    if( ui->GameTab->count( ) == 0 )
    {
        m_GameTabColor = 0;
        ui->MainTab->setTabIcon( 1, QIcon( ":/mainwindow/games_red.png" ) );
    }
    else if( ui->GameTab->count( ) > 1 || dynamic_cast<GameWidget*>( ui->GameTab->widget( 0 ) )->m_GameStarted )
    {
        m_GameTabColor = 2;
        ui->MainTab->setTabIcon( 1, QIcon( ":/mainwindow/games_green.png" ) );
    }
    else
    {
        m_GameTabColor = 1;
        ui->MainTab->setTabIcon( 1, QIcon( ":/mainwindow/games_yellow.png" ) );
    }
}

void Widget::update_game( unsigned int gameID, QStringList gameInfo, QList<QStringList> playersInfo )
{
    for( int32_t n = 0; n < ui->GameTab->count( ); ++n )
    {
        if( dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->m_GameID == gameID )
        {
            dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->update_game_direct( gameInfo, playersInfo );

            if( m_GameTabColor != 2 && ( ui->GameTab->count( ) > 1 || dynamic_cast<GameWidget*>( ui->GameTab->widget( 0 ) )->m_GameStarted ) )
            {
                m_GameTabColor = 2;
                ui->MainTab->setTabIcon( 1, QIcon( ":/mainwindow/games_green.png" ) );
            }

            return;
        }
    }
}

void Widget::update_game_map_info( unsigned int gameID, QString mapPath )
{
    for( int32_t n = 0; n < ui->GameTab->count( ); ++n )
    {
        if( dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->m_GameID == gameID )
        {
            dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->update_game_map_info_direct( mapPath );
            return;
        }
    }
}

void Widget::write_to_game_chat( QString message, QString extramessage, QString user, unsigned int type, unsigned int gameID )
{
    for( int32_t n = 0; n < ui->GameTab->count( ); ++n )
    {
        if( dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->m_GameID == gameID )
        {
            dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->write_to_game_chat_direct( message, extramessage, user, type );
            return;
        }
    }
}

void Widget::add_bnet_bot( unsigned int bnetID, QString botName )
{
    dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->add_bnet_bot_direct( botName );
}

void Widget::update_bnet_bot( unsigned int bnetID, QString botName, unsigned int type )
{
    dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->update_bnet_bot_direct( botName, type );
}

void Widget::delete_bnet_bot( unsigned int bnetID, QString botName )
{
    dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->delete_bnet_bot_direct( botName );
}

void Widget::add_game_to_list( QList<unsigned int> bnetIDs, unsigned int gameID, bool gproxy, unsigned int gameIcon, QStringList gameInfo )
{
    foreach( unsigned int bnetID, bnetIDs )
    {
        dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->add_game_to_list_direct( gameID, gproxy, gameIcon, gameInfo );
    }
}

void Widget::update_game_to_list( QList<unsigned int> bnetIDs, unsigned int gameID, QStringList gameInfo, bool deleteFromCurrentGames )
{
    foreach( unsigned int bnetID, bnetIDs )
    {
        dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->update_game_to_list_direct( gameID, gameInfo );
    }

    if( deleteFromCurrentGames )
    {
        for( int32_t n = 0; n < ui->GameTab->count( ); ++n )
        {
            if( dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->m_GameID == gameID )
            {
                (dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) ))->deleteLater( );
                ui->GameTab->removeTab( n );
                break;
            }
        }

        if( ui->GameTab->count( ) == 0 )
        {
            m_GameTabColor = 0;
            ui->MainTab->setTabIcon( 1, QIcon( ":/mainwindow/games_red.png" ) );
        }
        else if( ui->GameTab->count( ) > 1 || dynamic_cast<GameWidget*>( ui->GameTab->widget( 0 ) )->m_GameStarted )
        {
            m_GameTabColor = 2;
            ui->MainTab->setTabIcon( 1, QIcon( ":/mainwindow/games_green.png" ) );
        }
        else
        {
            m_GameTabColor = 1;
            ui->MainTab->setTabIcon( 1, QIcon( ":/mainwindow/games_yellow.png" ) );
        }
    }
}

void Widget::delete_game_from_list( QList<unsigned int> bnetIDs, unsigned int gameID, unsigned int type )
{
    // type 0 = delete from gamelist and from currentgamelist
    // type 1 = delete only from gamelist
    // type 2 = delete only from currentgamelist

    if( type != 2 )
    {
        foreach( unsigned int bnetID, bnetIDs )
        {
            dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->delete_game_from_list_direct( gameID );
        }
    }

    if( type != 1 )
    {
        for( int32_t n = 0; n < ui->GameTab->count( ); ++n )
        {
            if( dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->m_GameID == gameID )
            {
                (dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) ))->deleteLater( );
                ui->GameTab->removeTab( n );
                break;
            }
        }

        if( ui->GameTab->count( ) == 0 )
        {
            m_GameTabColor = 0;
            ui->MainTab->setTabIcon( 1, QIcon( ":/mainwindow/games_red.png" ) );
        }
        else if( ui->GameTab->count( ) > 1 || dynamic_cast<GameWidget*>( ui->GameTab->widget( 0 ) )->m_GameStarted )
        {
            m_GameTabColor = 2;
            ui->MainTab->setTabIcon( 1, QIcon( ":/mainwindow/games_green.png" ) );
        }
        else
        {
            m_GameTabColor = 1;
            ui->MainTab->setTabIcon( 1, QIcon( ":/mainwindow/games_yellow.png" ) );
        }
    }
}

void Widget::delete_old_messages( )
{
    for( int32_t n = 1; n < ui->ChatTab->count( ); ++n )
        dynamic_cast<ChatWidget*>( ui->ChatTab->widget( n ) )->deletingOldMessages( );
}

void Widget::game_list_table_pos_fix( QList<unsigned int> bnetIDs )
{
    foreach( unsigned int bnetID, bnetIDs )
    {
        dynamic_cast<ChatWidget*>( ui->ChatTab->widget( bnetID ) )->game_list_table_pos_fix_direct( );
    }
}

void Widget::backup_log( QString logfilename )
{
    exampleObject_3.setRunning( 7 );
    exampleObject_3.setLogFileName( logfilename );
    thread_3.start( );
}

void Widget::save_log_data( )
{
    exampleObject_2.setRunning( 4 );
    thread_2.start( );
}

void Widget::ThirdThreadFinished( )
{
    thread_3.quit( );
    gAltThreadState = "@ThreadFinished";
}

void Widget::scrollTo( int i )
{
    if( LabelsList[i]->accessibleDescription( ) == "0" )
    {
        if( ui->SettingsTab->currentIndex( ) != 0 )
            ui->SettingsTab->setCurrentIndex( 0 );

        ui->MainscrollArea->verticalScrollBar( )->setValue( LabelsList[i]->y( ) - static_cast<QGridLayout*>( ui->MainscrollAreaWidgetContents->layout( ) )->verticalSpacing( ) );
    }
    else if( LabelsList[i]->accessibleDescription( ) == "1" )
    {
        if( ui->SettingsTab->currentIndex( ) != 1 )
            ui->SettingsTab->setCurrentIndex( 1 );

        ui->GamesscrollArea->verticalScrollBar( )->setValue( LabelsList[i]->y( ) - static_cast<QGridLayout*>( ui->GamesscrollAreaWidgetContents->layout( ) )->verticalSpacing( ) );
    }
    else
        return;

    m_CurrentPossSearch = i;
}

void Widget::on_SearchLine_returnPressed( )
{
    if( m_CurrentPossSearch == -1 )
        return;

    LabelsList[m_CurrentPossSearch]->setStyleSheet( "QLabel { background-color: #ffff00; color: black; }" );

    if( m_CurrentPossSearch == LabelsList.size( ) - 1 )
    {
        for( int32_t i = 0; i < LabelsList.size( ); i++ )
        {
            if( LabelsList[i]->styleSheet( ) == "QLabel { background-color: #ffff00; color: black; }" )
            {
                scrollTo( i );
                LabelsList[i]->setStyleSheet( "QLabel { background-color: #ff8000; color: black; }" );
                return;
            }
        }
    }
    else
    {
        for( int32_t i = m_CurrentPossSearch + 1; i < LabelsList.size( ); i++ )
        {
            if( LabelsList[i]->styleSheet( ) == "QLabel { background-color: #ffff00; color: black; }" )
            {
                scrollTo( i );
                LabelsList[i]->setStyleSheet( "QLabel { background-color: #ff8000; color: black; }" );
                return;
            }
        }

        for( int32_t i = 0; i < m_CurrentPossSearch + 1; i++ )
        {
            if( LabelsList[i]->styleSheet( ) == "QLabel { background-color: #ffff00; color: black; }" )
            {
                scrollTo( i );
                LabelsList[i]->setStyleSheet( "QLabel { background-color: #ff8000; color: black; }" );
                return;
            }
        }
    }
}

void Widget::on_SearchLine_textChanged( const QString &arg1 )
{
    if( arg1.isEmpty( ) )
    {
        for( int32_t i = 0; i < LabelsList.size( ); i++ )
            LabelsList[i]->setStyleSheet( QString( ) );

        return;
    }

    m_CurrentPossSearch = -1;

    for( int32_t i = 0; i < LabelsList.size( ); i++ )
    {
        if( LabelsList[i]->text( ).contains( arg1, Qt::CaseInsensitive ) || LabelsList[i]->toolTip( ).contains( arg1, Qt::CaseInsensitive ) )
        {
            if( m_CurrentPossSearch == -1 )
            {
                scrollTo( i );
                LabelsList[i]->setStyleSheet( "QLabel { background-color: #ff8000; color: black; }" );
            }
            else
                LabelsList[i]->setStyleSheet( "QLabel { background-color: #ffff00; color: black; }" );
        }
        else
            LabelsList[i]->setStyleSheet( QString( ) );
    }
}

void Widget::on_SearchButton_clicked( )
{
    on_SearchLine_textChanged( ui->SearchLine->text( ) );
}

PresettingWindow::PresettingWindow( QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::PresettingWindow )
{
    ui->setupUi( this );
    gSkipPresetting = true;
    languageChanged = false;
    war3PathChanged = false;
    mapsPathChanged = false;
    mapsCFGPathChanged = false;
    savePathChanged = false;
    replaysPathChanged = false;
    hostPortChanged = false;

    if( gUNM && gUNM != nullptr )
    {
        if( gLanguageFile.size( ) >= 11 && gLanguageFile.substr( gLanguageFile.size( ) - 11 ) == "English.cfg" )
            ui->languageBox->setCurrentIndex( 0 );
        else if( gLanguageFile.size( ) >= 10 && gLanguageFile.substr( gLanguageFile.size( ) - 10 ) == "German.cfg" )
            ui->languageBox->setCurrentIndex( 2 );
        else if( gLanguageFile.size( ) >= 11 && gLanguageFile.substr( gLanguageFile.size( ) - 11 ) == "Spanish.cfg" )
            ui->languageBox->setCurrentIndex( 3 );
        else if( gLanguageFile.size( ) >= 11 && gLanguageFile.substr( gLanguageFile.size( ) - 11 ) == "Turkish.cfg" )
            ui->languageBox->setCurrentIndex( 4 );
        else if( gLanguageFile.size( ) >= 11 && gLanguageFile.substr( gLanguageFile.size( ) - 11 ) == "Serbian.cfg" )
            ui->languageBox->setCurrentIndex( 5 );
        else if( gLanguageFile.size( ) >= 14 && gLanguageFile.substr( gLanguageFile.size( ) - 14 ) == "Vietnamese.cfg" )
            ui->languageBox->setCurrentIndex( 6 );
        else
            ui->languageBox->setCurrentIndex( 1 );

        ui->war3PathLine->setText( QString::fromUtf8( gUNM->m_Warcraft3Path.c_str( ) ) );
        ui->mapsPathLine->setText( QString::fromUtf8( gUNM->m_MapPath.c_str( ) ) );
        ui->mapsCFGPathLine->setText( QString::fromUtf8( gUNM->m_MapCFGPath.c_str( ) ) );
        ui->savegamesPathLine->setText( QString::fromUtf8( gUNM->m_SaveGamePath.c_str( ) ) );
        ui->replaysPathLine->setText( QString::fromUtf8( gUNM->m_ReplayPath.c_str( ) ) );
        ui->hostPortSpin->setValue( gUNM->m_HostPort );
    }
}

void PresettingWindow::on_war3PathButton_clicked( )
{
    QString directory = QFileDialog::getExistingDirectory( this, tr( "Укажите путь к папке WarCraft III" ), QDir::currentPath( ) );

    if( !directory.isEmpty( ) )
    {
        war3PathChanged = true;
        ui->war3PathLine->setText( directory );
        war3PathLineAutoSet( directory );
    }
}

void PresettingWindow::on_mapsPathButton_clicked( )
{
    QString directory = QFileDialog::getExistingDirectory( this, tr( "Укажите путь к папке с картами" ), QDir::currentPath( ) );

    if( !directory.isEmpty( ) )
    {
        mapsPathChanged = true;
        ui->mapsPathLine->setText( directory );
    }
}

void PresettingWindow::on_mapsCFGPathButton_clicked( )
{
    QString directory = QFileDialog::getExistingDirectory( this, tr( "Укажите путь к папке с конфигами карт" ), QDir::currentPath( ) );

    if( !directory.isEmpty( ) )
    {
        mapsCFGPathChanged = true;
        ui->mapsCFGPathLine->setText( directory );
    }
}

void PresettingWindow::on_savegamesPathButton_clicked( )
{
    QString directory = QFileDialog::getExistingDirectory( this, tr( "Укажите путь к папке с сохраненными играми" ), QDir::currentPath( ) );

    if( !directory.isEmpty( ) )
    {
        savePathChanged = true;
        ui->savegamesPathLine->setText( directory );
    }
}

void PresettingWindow::on_replaysPathButton_clicked( )
{
    QString directory = QFileDialog::getExistingDirectory( this, tr( "Укажите путь к папке с реплеями" ), QDir::currentPath( ) );

    if( !directory.isEmpty( ) )
    {
        replaysPathChanged = true;
        ui->replaysPathLine->setText( directory );
    }
}

void PresettingWindow::on_war3PathLine_returnPressed( )
{
    war3PathChanged = true;
    war3PathLineAutoSet( ui->war3PathLine->text( ) );
}

void PresettingWindow::war3PathLineAutoSet( QString directory )
{
    if( !directory.isEmpty( ) )
    {
        string mapspathdirectorystr = ui->mapsPathLine->text( ).toStdString( );
        transform( mapspathdirectorystr.begin( ), mapspathdirectorystr.end( ), mapspathdirectorystr.begin( ), static_cast<int(*)(int)>(tolower) );

        if( mapspathdirectorystr.empty( ) || mapspathdirectorystr == "maps\\" || mapspathdirectorystr == "maps/" || mapspathdirectorystr == "maps" )
        {
            mapspathdirectorystr = directory.toStdString( );
#ifdef WIN32
            mapspathdirectorystr += "\\maps";
#else
            mapspathdirectorystr += "/maps";
#endif

            if( std::filesystem::exists( mapspathdirectorystr ) )
            {
                QString mapspathdirectory = QString::fromUtf8( mapspathdirectorystr.c_str( ) );
                ui->mapsPathLine->setText( mapspathdirectory );
                mapsPathChanged = true;
            }
        }
    }
}

void PresettingWindow::on_skipButton_clicked( )
{
    if( ui->skipCheckBox->isChecked( ) )
    {
        UTIL_SetVarInFile( "unm.cfg", "gui_showpesettingwindow", "0" );

        if( gUNM && gUNM != nullptr )
            gUNM->m_ShowPesettingWindow = 0;
    }

    close( );
}

void PresettingWindow::on_aplyButton_clicked( )
{
    vector<string> NonExistingDirectories;
    string FirstNonExistingDirectory = string( );
    string war3path = ui->war3PathLine->text( ).toStdString( );
    string mapspath = ui->mapsPathLine->text( ).toStdString( );
    string mapscfgpath = ui->mapsCFGPathLine->text( ).toStdString( );
    string savepath = ui->savegamesPathLine->text( ).toStdString( );
    string replayspath = ui->replaysPathLine->text( ).toStdString( );

    if( !std::filesystem::exists( war3path ) )
    {
        if( war3path.size( ) > 260 )
        {
            war3path = UTIL_SubStrFix( war3path, 0, 260 );
            war3path = "[" + war3path + "...]";
        }
        else
            war3path = "[" + war3path + "]";

        NonExistingDirectories.push_back( war3path );

        if( FirstNonExistingDirectory.empty( ) )
            FirstNonExistingDirectory = "Путь к WarCraft III";
    }

    if( !std::filesystem::exists( mapspath ) )
    {
        if( mapspath.size( ) > 260 )
        {
            mapspath = UTIL_SubStrFix( mapspath, 0, 260 );
            mapspath = "[" + mapspath + "...]";
        }
        else
            mapspath = "[" + mapspath + "]";

        NonExistingDirectories.push_back( mapspath );

        if( FirstNonExistingDirectory.empty( ) )
            FirstNonExistingDirectory = "Путь к картам";
    }

    if( !std::filesystem::exists( mapscfgpath ) )
    {
        if( mapscfgpath.size( ) > 260 )
        {
            mapscfgpath = UTIL_SubStrFix( mapscfgpath, 0, 260 );
            mapscfgpath = "[" + mapscfgpath + "...]";
        }
        else
            mapscfgpath = "[" + mapscfgpath + "]";

        NonExistingDirectories.push_back( mapscfgpath );

        if( FirstNonExistingDirectory.empty( ) )
            FirstNonExistingDirectory = "Путь к конфигам карт";
    }

    if( !std::filesystem::exists( savepath ) )
    {
        if( savepath.size( ) > 260 )
        {
            savepath = UTIL_SubStrFix( savepath, 0, 260 );
            savepath = "[" + savepath + "...]";
        }
        else
            savepath = "[" + savepath + "]";

        NonExistingDirectories.push_back( savepath );

        if( FirstNonExistingDirectory.empty( ) )
            FirstNonExistingDirectory = "Путь к сохранениям";
    }

    if( !std::filesystem::exists( replayspath ) )
    {
        if( replayspath.size( ) > 260 )
        {
            replayspath = UTIL_SubStrFix( replayspath, 0, 260 );
            replayspath = "[" + replayspath + "...]";
        }
        else
            replayspath = "[" + replayspath + "]";

        NonExistingDirectories.push_back( replayspath );

        if( FirstNonExistingDirectory.empty( ) )
            FirstNonExistingDirectory = "Путь к реплеям";
    }

    QString message = QString( );

    if( NonExistingDirectories.size( ) > 1 )
    {
        FirstNonExistingDirectory = "Следующие пути указанны неправильно:";

        for( uint32_t i = 0; i < NonExistingDirectories.size( ); i++ )
            FirstNonExistingDirectory += "\n" + NonExistingDirectories[i];

        message = QString::fromUtf8( FirstNonExistingDirectory.c_str( ) );
    }
    else if( NonExistingDirectories.size( ) == 1 )
        message = QString::fromUtf8( FirstNonExistingDirectory.c_str( ) ) + " " + QString::fromUtf8( NonExistingDirectories[0].c_str( ) ) + "] задан неверно!";

    if( !message.isEmpty( ) )
    {
        QMessageBox messageBox( QMessageBox::Question, "Каталог не найден", message, QMessageBox::Yes | QMessageBox::No, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "так надо!" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "ой, ща исправлю" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::No )
            return;
    }

    if( gUNM && gUNM != nullptr )
    {
        if( languageChanged )
        {
            int language = ui->languageBox->currentIndex( );

            if( language == 0 )
                gLanguageFile = UTIL_FixFilePath( "text_files\\Languages\\English.cfg" );
            else if( language == 2 )
                gLanguageFile = UTIL_FixFilePath( "text_files\\Languages\\German.cfg" );
            else if( language == 3 )
                gLanguageFile = UTIL_FixFilePath( "text_files\\Languages\\Spanish.cfg" );
            else if( language == 4 )
                gLanguageFile = UTIL_FixFilePath( "text_files\\Languages\\Turkish.cfg" );
            else if( language == 5 )
                gLanguageFile = UTIL_FixFilePath( "text_files\\Languages\\Serbian.cfg" );
            else if( language == 6 )
                gLanguageFile = UTIL_FixFilePath( "text_files\\Languages\\Vietnamese.cfg" );
            else
                gLanguageFile = UTIL_FixFilePath( "text_files\\Languages\\Russian.cfg" );

            if( gLanguage )
                delete gLanguage;

            gLanguage = new CLanguage( gLanguageFile );
            UTIL_SetVarInFile( "unm.cfg", "bot_language", gLanguageFile );
        }

        if( war3PathChanged )
        {
            string war3path = ui->war3PathLine->text( ).toStdString( );

            if( war3path.find_first_not_of( " " ) != string::npos )
            {
                gUNM->m_Warcraft3Path = UTIL_FixPath( war3path );
                UTIL_SetVarInFile( "unm.cfg", "bot_war3path", gUNM->m_Warcraft3Path );
            }
            else
            {
                gUNM->m_Warcraft3Path = UTIL_FixPath( "war3\\" );
                UTIL_DelVarInFile( "unm.cfg", "bot_war3path" );
            }
        }

        if( mapsPathChanged )
        {
            string mapspath = ui->mapsPathLine->text( ).toStdString( );

            if( mapspath.find_first_not_of( " " ) != string::npos )
            {
                gUNM->m_MapPath = UTIL_FixPath( mapspath );
                UTIL_SetVarInFile( "unm.cfg", "bot_mappath", gUNM->m_MapPath );
            }
            else
            {
                gUNM->m_MapPath = UTIL_FixPath( "maps\\" );
                UTIL_DelVarInFile( "unm.cfg", "bot_mappath" );
            }
        }

        if( mapsCFGPathChanged )
        {
            string mapscfgpath = ui->mapsCFGPathLine->text( ).toStdString( );

            if( mapscfgpath.find_first_not_of( " " ) != string::npos )
            {
                gUNM->m_MapCFGPath = UTIL_FixPath( mapscfgpath );
                UTIL_SetVarInFile( "unm.cfg", "bot_mapcfgpath", gUNM->m_MapCFGPath );
            }
            else
            {
                gUNM->m_MapCFGPath = UTIL_FixPath( "mapcfgs\\" );
                UTIL_DelVarInFile( "unm.cfg", "bot_mapcfgpath" );
            }
        }

        if( savePathChanged )
        {
            string savepath = ui->savegamesPathLine->text( ).toStdString( );

            if( savepath.find_first_not_of( " " ) != string::npos )
            {
                gUNM->m_SaveGamePath = UTIL_FixPath( savepath );
                UTIL_SetVarInFile( "unm.cfg", "bot_savegamepath", gUNM->m_SaveGamePath );
            }
            else
            {
                gUNM->m_SaveGamePath = UTIL_FixPath( "savegames\\" );
                UTIL_DelVarInFile( "unm.cfg", "bot_savegamepath" );
            }
        }

        if( replaysPathChanged )
        {
            string replayspath = ui->replaysPathLine->text( ).toStdString( );

            if( replayspath.find_first_not_of( " " ) != string::npos )
            {
                gUNM->m_ReplayPath = UTIL_FixPath( replayspath );
                UTIL_SetVarInFile( "unm.cfg", "bot_replaypath", gUNM->m_ReplayPath );
            }
            else
            {
                gUNM->m_ReplayPath = UTIL_FixPath( "replays\\" );
                UTIL_DelVarInFile( "unm.cfg", "bot_replaypath" );
            }
        }

        if( hostPortChanged )
        {
            gUNM->m_HostPort = ui->hostPortSpin->value( );
            UTIL_SetVarInFile( "unm.cfg", "bot_hostport", UTIL_ToString( gUNM->m_HostPort ) );
        }
    }

    close( );
}

void PresettingWindow::on_war3PathLine_textEdited( const QString & )
{
    war3PathChanged = true;
}

void PresettingWindow::on_mapsPathLine_textEdited( const QString & )
{
    mapsPathChanged = true;
}

void PresettingWindow::on_mapsCFGPathLine_textEdited( const QString & )
{
    mapsCFGPathChanged = true;
}

void PresettingWindow::on_savegamesPathLine_textEdited( const QString & )
{
    savePathChanged = true;
}

void PresettingWindow::on_replaysPathLine_textEdited( const QString & )
{
    replaysPathChanged = true;
}

void PresettingWindow::on_languageBox_currentIndexChanged( int )
{
    languageChanged = true;
}

void PresettingWindow::on_hostPortSpin_valueChanged( int )
{
    hostPortChanged = true;
}

PresettingWindow::~PresettingWindow( )
{
    delete ui;
}

HelpLogonWindow::HelpLogonWindow( QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::HelpLogonWindow ),
    m_iCCupAHinfo( true )
{
    ui->setupUi( this );
    on_iCCupAHinfo_clicked( );
}

void HelpLogonWindow::on_iCCupAHinfo_clicked( )
{
    m_iCCupAHinfo = !m_iCCupAHinfo;
    ui->helpField->clear( );
    ui->helpField->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );

    if( !m_iCCupAHinfo )
    {
        ui->helpField->appendPlainText( "В окне входа представляется возможным указать главные настройки подключения к Battle.net/PVPGN серверу (адрес сервера, логин и пароль), однако стоит понимать, что это далеко не все опции. Полный список настроек для соединения с сервером (например, тип сервера, версия, патч...) находится в конфиг-файле [unm.cfg]. Также в конфиге можно установить больше подключений (добавить другие сервера или подключиться с нескольких аккаунтов на один сервер)." );
        ui->helpField->appendPlainText( "Если не удалось подключиться к серверу, первым делом проверьте правильность введенных данных. Далее необходимо заглянуть в конфиг и проверить указана верная ли версия сервера. Необходимо смотреть на квары, начинающиеся с \"bnet_\". Обратите внимание, квары \"bnet_\" отвечают за первое подключения, квары \"bnet2_\" - за второе, \"bnet3_\" - третье, можно задать неограниченное количество подключений. Квары \"bnet_custom_\" позволяют указать тип и версию сервера (необходимые значения гуглим для каждого сервера отдельно). С полным описанием всех кваров можно ознакомиться в базовом конфиг-файле [text_files\\default.cfg]. Имейте в виду, строки начинающиеся с символа \"#\" не читаются клиентом, в таком случае будут использоваться стандартные значения кваров. Стандартные значения каждого квара изложены в базовом конфиг-файле." );
        ui->helpField->appendPlainText( "Также клиент может запустить анти-хак, необходимый для подключения к PVPGN серверу iCCup. Галочка \"Запустить анти-хак\" в окне входа эквивалентна квару в конфиге \"bnet_runiccuploader\". Активация этой галочки запускает анти-хак только для первого подключения (bnet_server), если в конфиге задано более одного подключения к айкапу - следует включить запуск анти-хака для каждого (bnetX_runiccuploader = 1, где X = номер подключения к айкапу). Запуск анти-хака необходимо включать только для сервера iCCup, для других серверов эта опция будет отключена автоматически. Также следует обратить внимания на квар \"bnet_port\", значения для каждого подключения к iCCup - должно быть разным, а значение для других серверов должно быть всегда \"bnetX_port = 6112\". Если ваш аккаунт на айкапе имеет cg2, это позволяет подключаться к айкапу без запуска анти-хака, также для данного подключения следует указать \"bnetX_port = 6112\"." );
        ui->helpField->appendPlainText( "Клиент автоматически не обновляет анти-хак, это следует делать вручную. Чтобы узнать подробней об обновлении анти-хака и об анти-хаке в целом нажмите кнопку снизу." );
        ui->iCCupAHinfo->setText( "Подробнее об анти-хаке..." );
    }
    else
    {
        ui->helpField->appendPlainText( "iCCup Reconnecnt Loader (анти-хак):" );
        ui->helpField->appendPlainText( "IccReconnecntLoader.exe не обновляются сам, его нужно обновлять вручную. Для этого нужно скопировать файлы \"iccwc3.icc\" и \"Reconnect.dll\" из папки с обычным лаунчером последней версии в папку \"iCCupLoader\" (которая расположена в корневой папке клиента)." );
        ui->helpField->appendPlainText( "IccReconnecntLoader позволяет выбрать уникальный порт, который будет использовать античит для подключения клиента к айкапу. Таким образом, используя разные порты, мы можем подключить одновременно несколько логинов (или параллельно запущенных клиентов) с одного пк на iCCup. Также, используя обычный лаунчер, мы сможем зайти через war3 с того же пк, на котором запущен клиент (или несколько клиентов)." );
        ui->helpField->appendPlainText( "Как пользоваться IccReconnecntLoader.exe:" );
        ui->helpField->appendPlainText( "В конфиге клиента для каждого логина iCCup включаем автозапуск анти-хака (квар bnet_runiccuploader) и выбираем уникальный порт, который будет использоваться античитом для подключения клиента к айкапу (квар bnet_port), запускаем клиент и на этапе ввода логина/пароля ставим галочку напротив \"Запустить анти-хак\", заходим..." );
        ui->helpField->appendPlainText( "Следует понимать, что галочка \"Запустить анти-хак\" отвечает только за первый по счету логин, указанный в конфиге (bnet_server), разрешить/запретить автозапуск анти-хака для остальных логинов (bnet2_server, bnet3_server и последующих) имеется возможным только в конфиге клиента (квар bnetX_runiccuploader)" );
        ui->helpField->appendPlainText( "Также можно запускать анти-чит вручную: в конфиге клиента отключаем автозапуск анти-хака для каждого логина iCCup (квар bnet_runiccuploader), запускаем \"IccReconnecntLoader.exe\" (находится в папке \"iCCupLoader\"), в открывшимся окне указываем порт (если собираемся подключить к айкапу несколько логинов, то запускаем несколько экземпляров \"IccReconnecntLoader.exe\" - по одному на каждый логин, и в каждом экземпляре указываем уникальный порт), далее запускаем клиент и на этапе ввода логина/пароля снимаем галочку напротив \"Запустить анти-хак\", заходим..." );
        ui->helpField->appendPlainText( "Обратите внимание, если вы используете автозапуск анти-хака, то вы не должны его запускать вручную, и наоборот, если вы запускаете анти-хак вручную вы должны убрать галочку/отключить автозапуск анти-хака в конфиге (для каждого логина, для которого хотите запустить анти-хак вручную). Кроме того, если у вас имеется cg2 вы можете подключаться к айкапу без использования анти-хака (bnet_runiccuploader = 0, bnet_port = 6112). Для других серверов значение квара bnet_runiccuploader будет игнорироваться, а вот bnet_port должен быть всегда 6112" );
        ui->iCCupAHinfo->setText( "Общая информация" );
    }

    ui->helpField->verticalScrollBar( )->setValue(0);
}

void HelpLogonWindow::on_closeButton_clicked( )
{
    close( );
}

HelpLogonWindow::~HelpLogonWindow( )
{
    delete ui;
}

LogSettingsWindow::LogSettingsWindow( QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::LogSettingsWindow )
{
    ui->setupUi( this );
}

LogSettingsWindow::~LogSettingsWindow( )
{
    delete ui;
}

StyleSettingWindow::StyleSettingWindow( QWidget *parent ) :
    QDialog( parent ),
    ui( new Ui::StyleSettingWindow )
{
    ui->setupUi( this );
    ui->badColorLabel->setStyleSheet( "QLabel { color: " + gWarningMessagesColor + "; }" );
    ui->colorNormalMsg->setText( gNormalMessagesColor );
    ui->colorPrivateMsg->setText( gPrivateMessagesColor );
    ui->colorNotifyMsg->setText( gNotifyMessagesColor );
    ui->colorWarningMsg->setText( gWarningMessagesColor );
    ui->colorPreSentMsg->setText( gPreSentMessagesColor );

    if( gDefaultScrollBarForce )
        ui->defaultScrollBar->setCurrentIndex( 0 );
    else
        ui->defaultScrollBar->setCurrentIndex( 1 );
}

StyleSettingWindow::~StyleSettingWindow( )
{
    delete ui;
}

void StyleSettingWindow::on_colorNormalMsgReset_clicked( )
{
    ui->colorNormalMsg->setText( gNormalMessagesColor );
}

void StyleSettingWindow::on_colorPrivateMsgReset_clicked( )
{
    ui->colorPrivateMsg->setText( gPrivateMessagesColor );
}

void StyleSettingWindow::on_colorNotifyMsgReset_clicked( )
{
    ui->colorNotifyMsg->setText( gNotifyMessagesColor );
}

void StyleSettingWindow::on_colorWarningMsgReset_clicked( )
{
    ui->colorWarningMsg->setText( gWarningMessagesColor );
}

void StyleSettingWindow::on_colorPreSentMsgReset_clicked( )
{
    ui->colorPreSentMsg->setText( gPreSentMessagesColor );
}

void StyleSettingWindow::on_defaultScrollBarReset_clicked( )
{
    if( gDefaultScrollBarForce )
        ui->defaultScrollBar->setCurrentIndex( 0 );
    else
        ui->defaultScrollBar->setCurrentIndex( 1 );
}

void StyleSettingWindow::on_cancelButton_clicked( )
{
    close( );
}

void StyleSettingWindow::on_applyButton_clicked( )
{
    QString textError = QString( );
    string NormalMessagesColor = ui->colorNormalMsg->text( ).toStdString( );
    string PrivateMessagesColor = ui->colorPrivateMsg->text( ).toStdString( );
    string NotifyMessagesColor = ui->colorNotifyMsg->text( ).toStdString( );
    string WarningMessagesColor = ui->colorWarningMsg->text( ).toStdString( );
    string PreSentMessagesColor = ui->colorPreSentMsg->text( ).toStdString( );
    transform( NormalMessagesColor.begin( ), NormalMessagesColor.end( ), NormalMessagesColor.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( PrivateMessagesColor.begin( ), PrivateMessagesColor.end( ), PrivateMessagesColor.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( NotifyMessagesColor.begin( ), NotifyMessagesColor.end( ), NotifyMessagesColor.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( WarningMessagesColor.begin( ), WarningMessagesColor.end( ), WarningMessagesColor.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( PreSentMessagesColor.begin( ), PreSentMessagesColor.end( ), PreSentMessagesColor.begin( ), static_cast<int(*)(int)>(tolower) );

    if( NormalMessagesColor.size( ) != 7 || NormalMessagesColor.front( ) != '#' || NormalMessagesColor.substr( 1 ).find_first_not_of( "0123456789abcdef" ) != string::npos )
    {
        ui->badColorLabel->setText( "Некорректно указан цвет для обычных сообщений!" );
        return;
    }
    else if( PrivateMessagesColor.size( ) != 7 || PrivateMessagesColor.front( ) != '#' || PrivateMessagesColor.substr( 1 ).find_first_not_of( "0123456789abcdef" ) != string::npos )
    {
        ui->badColorLabel->setText( "Некорректно указан цвет для приватных сообщений!" );
        return;
    }
    else if( NotifyMessagesColor.size( ) != 7 || NotifyMessagesColor.front( ) != '#' || NotifyMessagesColor.substr( 1 ).find_first_not_of( "0123456789abcdef" ) != string::npos )
    {
        ui->badColorLabel->setText( "Некорректно указан цвет для уведомлений!" );
        return;
    }
    else if( WarningMessagesColor.size( ) != 7 || WarningMessagesColor.front( ) != '#' || WarningMessagesColor.substr( 1 ).find_first_not_of( "0123456789abcdef" ) != string::npos )
    {
        ui->badColorLabel->setText( "Некорректно указан цвет для предупреждений!" );
        return;
    }
    else if( PreSentMessagesColor.size( ) != 7 || PreSentMessagesColor.front( ) != '#' || PreSentMessagesColor.substr( 1 ).find_first_not_of( "0123456789abcdef" ) != string::npos )
    {
        ui->badColorLabel->setText( "Некорректно указан цвет для сообщений в очереди на отправку!" );
        return;
    }
    else
        ui->badColorLabel->clear( );

    gNormalMessagesColor = QString::fromUtf8( NormalMessagesColor.c_str( ) );
    gPrivateMessagesColor = QString::fromUtf8( PrivateMessagesColor.c_str( ) );
    gNotifyMessagesColor = QString::fromUtf8( NotifyMessagesColor.c_str( ) );
    gWarningMessagesColor = QString::fromUtf8( WarningMessagesColor.c_str( ) );
    gPreSentMessagesColor = QString::fromUtf8( PreSentMessagesColor.c_str( ) );

    if( ui->defaultScrollBar->currentIndex( ) == 0 )
        gDefaultScrollBarForce = true;
    else
        gDefaultScrollBarForce = false;

    if( ui->saveCheckBox->isChecked( ) )
    {
        string stylescfg = UTIL_FixFilePath( "styles\\config.cfg" );

        UTIL_SetVarInFile( stylescfg, gCurrentStyleName + "_normal", NormalMessagesColor );
        UTIL_SetVarInFile( stylescfg, gCurrentStyleName + "_private", PrivateMessagesColor );
        UTIL_SetVarInFile( stylescfg, gCurrentStyleName + "_notify", NotifyMessagesColor );
        UTIL_SetVarInFile( stylescfg, gCurrentStyleName + "_warning", WarningMessagesColor );
        UTIL_SetVarInFile( stylescfg, gCurrentStyleName + "_present", PreSentMessagesColor );

        if( gDefaultScrollBarForce )
            UTIL_SetVarInFile( stylescfg, gCurrentStyleName + "_scrollbardefault", "1" );
        else
            UTIL_SetVarInFile( stylescfg, gCurrentStyleName + "_scrollbardefault", "0" );
    }

    close( );
}

AnotherWindow::AnotherWindow( QWidget *parent ) :
    QWidget( parent ),
    ui( new Ui::AnotherWindow )
{
    ui->setupUi( this );
    ui->warningServer->setStyleSheet( "QLabel { color: " + gWarningMessagesColor + "; }" );
    ui->warningLogin->setStyleSheet( "QLabel { color: " + gWarningMessagesColor + "; }" );
    ui->warningPassword->setStyleSheet( "QLabel { color: " + gWarningMessagesColor + "; }" );
    ui->warningServer->setMaximumHeight( 3 );
    ui->warningLogin->setMaximumHeight( 3 );
    ui->warningPassword->setMaximumHeight( 3 );
    resize( minimumWidth( ), minimumHeight( ) );
}

AnotherWindow::~AnotherWindow( )
{
    delete ui;
}

void AnotherWindow::closeEvent( QCloseEvent *event )
{
    if( !AuthorizationPassed )
    {
        event->ignore( );
        QMessageBox messageBox( QMessageBox::Question, tr( "Выход отсюда" ), tr( "Закрыть все это?" ), QMessageBox::Yes | QMessageBox::No, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "Вырубить" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "Передумать" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::Yes )
        {
            gExitProcessStarted = true;
            qApp->quit( );

            if( gRestartAfterExiting )
                QProcess::startDetached( qApp->arguments( )[0], qApp->arguments( ) );
        }
    }
}

void AnotherWindow::on_logon_clicked( )
{
    string server = ui->server->text( ).toStdString( );
    string login = ui->login->text( ).toStdString( );
    string password = ui->password->text( ).toStdString( );

    if( server.find( " " ) != string::npos )
    {
        ui->warningServer->setText( "Адрес сервера не может содержать пробелы." );
        ui->warningServer->setMaximumHeight( 13 );
    }
    else if( server.find_first_not_of( " " ) == string::npos )
    {
        ui->warningServer->setText( "Укажетие адрес сервера." );
        ui->warningServer->setMaximumHeight( 13 );
    }
    else
    {
        ui->warningServer->clear( );
        ui->warningServer->setMaximumHeight( 3 );
    }

    if( login.find( " " ) != string::npos )
    {
        ui->warningLogin->setText( "Логин не может содержать пробелы." );
        ui->warningLogin->setMaximumHeight( 13 );
    }
    else if( login.find_first_not_of( " " ) == string::npos )
    {
        ui->warningLogin->setText( "Укажетие логин." );
        ui->warningLogin->setMaximumHeight( 13 );
    }
    else
    {
        ui->warningLogin->clear( );
        ui->warningLogin->setMaximumHeight( 3 );
    }

    if( password.find( " " ) != string::npos )
    {
        ui->warningPassword->setText( "Пароль не может содержать пробелы." );
        ui->warningPassword->setMaximumHeight( 13 );
    }
    else if( password.find_first_not_of( " " ) == string::npos )
    {
        ui->warningPassword->setText( "Укажетие пароль." );
        ui->warningPassword->setMaximumHeight( 13 );
    }
    else
    {
        ui->warningPassword->clear( );
        ui->warningPassword->setMaximumHeight( 3 );
    }

    if( server.find_first_not_of( " " ) != string::npos && server.find( " " ) == string::npos && login.find_first_not_of( " " ) != string::npos && login.find( " " ) == string::npos && password.find_first_not_of( " " ) != string::npos && password.find( " " ) == string::npos )
    {
        ui->logon->setEnabled( false );
        ui->logon->setText( "Заходим..." );
        ui->warningLogon->setStyleSheet( "QLabel { color: " + gNormalMessagesColor + "; }" );
        ui->warningLogon->setText( "Пожалуйста, подождите..." );
        ui->warningServer->clear( );
        ui->warningLogin->clear( );
        ui->warningPassword->clear( );
        ui->warningServer->setMaximumHeight( 3 );
        ui->warningLogin->setMaximumHeight( 3 );
        ui->warningPassword->setMaximumHeight( 3 );
        LogonDataServer = server;
        LogonDataLogin = login;
        LogonDataPassword = password;
        LogonDataRememberMe = ui->rememberMe->isChecked( );
        LogonDataRunAH = ui->runAH->isChecked( );
        emit runThread( LogonDataRunAH );
    }
    else
    {
        ui->warningLogon->setStyleSheet( "QLabel { color: " + gWarningMessagesColor + "; }" );
        ui->warningLogon->setText( "Не все поля корректно заполнены!" );
    }
}

void AnotherWindow::logon_successful( )
{
    emit saveLogData( );
    AuthorizationPassed = true;
    ui->warningServer->clear( );
    ui->warningLogin->clear( );
    ui->warningPassword->clear( );
    ui->warningServer->setMaximumHeight( 3 );
    ui->warningLogin->setMaximumHeight( 3 );
    ui->warningPassword->setMaximumHeight( 3 );
    close( );
    destroy( );
    emit firstWindow( );
}

void AnotherWindow::logon_failed( QString message )
{
    if( message.mid( 0, 1 ) == "0" )
    {
        message = message.mid( 1 );
        QMessageBox::warning( this, "Опаньки", "Кажись, мы словили временный бан (~20 минут)\nза исчерпание лимита ввода неверного пароля :(" );
    }
    else if( message.mid( 0, 1 ) == "1" )
    {
        message = message.mid( 1 );
        QMessageBox::warning( this, "Внимание", "Осталась последняя попытка ввести верный пароль,\nв случае неудачи вас ждет временный бан на ~20 минут!" );
    }

    ui->warningLogon->setStyleSheet( "QLabel { color: " + gWarningMessagesColor + "; }" );
    ui->warningLogon->setText( message );
    ui->logon->setEnabled( true );
    ui->logon->setText( "Войти" );
}

void AnotherWindow::on_server_returnPressed( )
{
    string server = ui->server->text( ).toStdString( );

    if( !server.empty( ) )
        QTimer::singleShot( 0, ui->login, SLOT( setFocus( ) ) );
}

void AnotherWindow::on_login_returnPressed( )
{
    string login = ui->login->text( ).toStdString( );

    if( !login.empty( ) )
    {
        string server = ui->server->text( ).toStdString( );

        if( server.empty( ) )
            QTimer::singleShot( 0, ui->server, SLOT( setFocus( ) ) );
        else
            QTimer::singleShot( 0, ui->password, SLOT( setFocus( ) ) );
    }
}

void AnotherWindow::on_password_returnPressed( )
{
    string password = ui->password->text( ).toStdString( );

    if( !password.empty( ) )
    {
        string login = ui->login->text( ).toStdString( );
        string server = ui->server->text( ).toStdString( );

        if( server.find( " " ) != string::npos )
        {
            ui->warningServer->setText( "Адрес сервера не может содержать пробелы." );
            ui->warningServer->setMaximumHeight( 13 );
        }
        else if( server.find_first_not_of( " " ) == string::npos )
        {
            ui->warningServer->setText( "Укажетие адрес сервера." );
            ui->warningServer->setMaximumHeight( 13 );
        }
        else
        {
            ui->warningServer->clear( );
            ui->warningServer->setMaximumHeight( 3 );
        }

        if( login.find( " " ) != string::npos )
        {
            ui->warningLogin->setText( "Логин не может содержать пробелы." );
            ui->warningLogin->setMaximumHeight( 13 );
        }
        else if( login.find_first_not_of( " " ) == string::npos )
        {
            ui->warningLogin->setText( "Укажетие логин." );
            ui->warningLogin->setMaximumHeight( 13 );
        }
        else
        {
            ui->warningLogin->clear( );
            ui->warningLogin->setMaximumHeight( 3 );
        }

        if( password.find( " " ) != string::npos )
        {
            ui->warningPassword->setText( "Пароль не может содержать пробелы." );
            ui->warningPassword->setMaximumHeight( 13 );
        }
        else if( password.find_first_not_of( " " ) == string::npos )
        {
            ui->warningPassword->setText( "Укажетие пароль." );
            ui->warningPassword->setMaximumHeight( 13 );
        }
        else
        {
            ui->warningPassword->clear( );
            ui->warningPassword->setMaximumHeight( 3 );
        }

        if( server.empty( ) )
            QTimer::singleShot( 0, ui->server, SLOT( setFocus( ) ) );
        else if( login.empty( ) )
            QTimer::singleShot( 0, ui->login, SLOT( setFocus( ) ) );
        else
        {
            if( server.find_first_not_of( " " ) != string::npos && server.find( " " ) == string::npos && login.find_first_not_of( " " ) != string::npos && login.find( " " ) == string::npos && password.find_first_not_of( " " ) != string::npos && password.find( " " ) == string::npos )
            {
                ui->logon->setEnabled( false );
                ui->logon->setText( "Заходим..." );
                ui->warningLogon->setStyleSheet( "QLabel { color: " + gNormalMessagesColor + "; }" );
                ui->warningLogon->setText( "Пожалуйста, подождите..." );
                ui->warningServer->clear( );
                ui->warningLogin->clear( );
                ui->warningPassword->clear( );
                ui->warningServer->setMaximumHeight( 3 );
                ui->warningLogin->setMaximumHeight( 3 );
                ui->warningPassword->setMaximumHeight( 3 );
                LogonDataServer = server;
                LogonDataLogin = login;
                LogonDataPassword = password;
                LogonDataRememberMe = ui->rememberMe->isChecked( );
                LogonDataRunAH = ui->runAH->isChecked( );
                emit runThread( LogonDataRunAH );
            }
            else
            {
                ui->warningLogon->setStyleSheet( "QLabel { color: " + gNormalMessagesColor + "; }" );
                ui->warningLogon->setText( "Предупреждение: запуск анти-хака влечет за собой закрытие запущенного  Warcraft III!" );
            }
        }
    }
}

void AnotherWindow::on_help_clicked( )
{
    HelpLogonWindow hWindow;
    hWindow.setModal( true );
    hWindow.exec( );
}

void AnotherWindow::on_GoToConfigFile_clicked( )
{
    ShowInExplorer( QDir::currentPath( ) + "/unm.cfg" );
}

void AnotherWindow::set_server( QString message )
{
    ui->server->setText( message );
}

void AnotherWindow::set_login( QString message )
{
    ui->login->setText( message );
}

void AnotherWindow::set_password( QString message )
{
    ui->password->setText( message );
}

void AnotherWindow::set_run_ah( bool runAH )
{
    ui->runAH->setChecked( runAH );
}

void AnotherWindow::set_remember_me( bool rememberMe )
{
    ui->rememberMe->setChecked( rememberMe );
}

void AnotherWindow::on_settings_clicked( )
{
    PresettingWindow pWindow;
    pWindow.setModal( true );
    pWindow.exec( );
}

//
// CUNM
//

CUNM::CUNM( CConfig *CFG )
{
    m_LocalServer = new CTCPServer( );
    m_LocalSocket = nullptr;
    m_RemoteSocket = new CTCPClient( );
    m_RemoteSocket->SetNoDelay( true );
    m_UDPSocket = new CUDPSocket( );
    m_UDPSocket->SetBroadcastTarget( "127.0.0.1", "[GPROXY]" );
    m_UDPSocket->SetDontRoute( true );
    m_GameProtocol = new CGameProtocol( this );
    m_GPSProtocol = new CGPSProtocol( );
    m_TotalPacketsReceivedFromLocal = 0;
    m_TotalPacketsReceivedFromRemote = 0;
    m_Port = 6135;
    m_UsedBnetID = 0;
    m_UniqueGameID = 0;
    m_CurrentGameID = 0;
    m_LastWar3RunTicks = 0;
    m_LastConnectionAttemptTime = 0;
    m_LastRefreshTime = 0;
    m_RemoteServerPort = 0;
    m_FakeLobbyTime = 0;
    m_FakeLobby = false;
    m_GameIsReliable = false;
    m_GameStarted = false;
    m_LeaveGameSent = false;
    m_ActionReceived = false;
    m_Synchronized = true;
    m_GPPort = 0;
    m_PID = 255;
    m_ChatPID = 255;
    m_ReconnectKey = 0;
    m_NumEmptyActions = 0;
    m_NumEmptyActionsUsed = 0;
    m_LastAckTime = 0;
    m_LastActionTime = 0;
    m_Exiting = false;
    m_ExitingNice = false;
    uint32_t tryport = 100;

    while( tryport > 0 )
    {
        tryport--;

        if( m_LocalServer->Listen( string( ), m_Port, "[GPROXY]" ) )
        {
            CONSOLE_Print( "[GPROXY] listening on port " + UTIL_ToString( m_Port ) );
            break;
        }
        else if( tryport != 0 )
        {
            CONSOLE_Print( "[GPROXY] error listening on port " + UTIL_ToString( m_Port ) + ", try with next port..." );
            m_LocalServer->Reset( );
            m_Port++;
        }
        else
        {
            CONSOLE_Print( "[GPROXY] error listening on port " + UTIL_ToString( m_Port ) + ", unm will end" );
            m_LocalServer->Reset( );
            m_Exiting = true;
        }
    }

    m_ReconnectSocket = nullptr;
    m_GPSProtocol = new CGPSProtocol( );
    m_CRC = new CCRC32( );
    m_CRC->Initialize( );
    m_SHA = new CSHA1( );
    m_CurrentGameName = string( );
    m_CurrentGame = nullptr;
    m_IrinaServer = new CIrinaWS( this );

    // get a list of local IP addresses
    // this list is used elsewhere to determine if a player connecting to the bot is local or not

    CONSOLE_Print( "[UNM] attempting to find local IP addresses" );

#ifdef WIN32
    // use a more reliable Windows specific method since the portable method doesn't always work properly on Windows
    // code stolen from: http://tangentsoft.net/wskfaq/examples/getifaces.html

    SOCKET sd = WSASocket( AF_INET, SOCK_DGRAM, 0, nullptr, 0, 0 );

    if( sd == SOCKET_ERROR )
        CONSOLE_Print( "[UNM] error finding local IP addresses - failed to create socket (error code " + UTIL_ToString( WSAGetLastError( ) ) + ")" );
    else
    {
        INTERFACE_INFO InterfaceList[20];
        unsigned long nBytesReturned;

        if( WSAIoctl( sd, SIO_GET_INTERFACE_LIST, nullptr, 0, &InterfaceList, sizeof( InterfaceList ), &nBytesReturned, nullptr, nullptr ) == SOCKET_ERROR )
            CONSOLE_Print( "[UNM] error finding local IP addresses - WSAIoctl failed (error code " + UTIL_ToString( WSAGetLastError( ) ) + ")" );
        else
        {
            int nNumInterfaces = nBytesReturned / sizeof( INTERFACE_INFO );

            for( int i = 0; i < nNumInterfaces; i++ )
            {
                sockaddr_in *pAddress;
                pAddress = ( sockaddr_in * )&( InterfaceList[i].iiAddress );
                char strIPString[INET_ADDRSTRLEN];
                CONSOLE_Print( "[UNM] local IP address #" + UTIL_ToString( i + 1 ) + " is [" + string( inet_ntop( AF_INET, &pAddress->sin_addr, strIPString, sizeof( strIPString ) ) ) + "]" );
                m_LocalAddresses.push_back( UTIL_CreateByteArray( (uint32_t)pAddress->sin_addr.s_addr, false ) );
            }
        }

        closesocket( sd );
    }
#else
    // use a portable method

    char HostName[255];

    if( gethostname( HostName, 255 ) == SOCKET_ERROR )
        CONSOLE_Print( "[UNM] error finding local IP addresses - failed to get local hostname" );
    else
    {
        CONSOLE_Print( "[UNM] local hostname is [" + string( HostName ) + "]" );
        struct hostent *HostEnt = gethostbyname( HostName );

        if( !HostEnt )
            CONSOLE_Print( "[UNM] error finding local IP addresses - gethostbyname failed" );
        else
        {
            for( int i = 0; HostEnt->h_addr_list[i] != nullptr; i++ )
            {
                struct in_addr Address;
                memcpy( &Address, HostEnt->h_addr_list[i], sizeof( struct in_addr ) );
                CONSOLE_Print( "[UNM] local IP address #" + UTIL_ToString( i + 1 ) + " is [" + string( inet_ntoa( Address ) ) + "]" );
                m_LocalAddresses.push_back( UTIL_CreateByteArray( (uint32_t)Address.s_addr, false ) );
            }
        }
    }
#endif

    m_Version = "1.2.10";
    m_CurrentGameCounter = 0;
    m_HostCounter = 1;
    m_TFT = CFG->GetInt( "bot_tft", 1 ) != 0;

    if( m_TFT )
    {
        CONSOLE_Print( "[UNM] acting as Warcraft III: The Frozen Throne" );
        m_MapType = ".w3x";
    }
    else
    {
        CONSOLE_Print( "[UNM] acting as Warcraft III: Reign of Chaos" );
        m_MapType = ".w3m";
    }

    QString qComputerName = QHostInfo::localHostName( );
    m_ComputerName = qComputerName.toStdString( );

    if( m_ComputerName.find_first_not_of( " " ) == string::npos )
        m_ComputerName = "unm";

    gWindowTitle = string( );
    m_Reconnect = CFG->GetInt( "bot_reconnect", 1 ) != 0;
    uint32_t fixid = 0;
    emit gToLog->addChatTab( 0, QString( ), "IrinaBot.ru", QString( ), 0 );

    // load the battle.net connections
    // we're just loading the config data and creating the CBNET classes here, the connections are established later (in the Update function)

    for( uint32_t i = 1; i <= 255; i++ )
    {
        string Prefix;

        if( i == 1 )
            Prefix = "bnet_";
        else
            Prefix = "bnet" + UTIL_ToString( i ) + "_";

        string Server = CFG->GetValidString( Prefix + "server", string( ) );
        uint32_t ServerType = CFG->GetUInt( Prefix + "servertype", 0 );

        if( Server.empty( ) )
        {
            fixid++;
            continue;
        }

        if( ServerType == 0 && IsIccupServer( Server ) )
            Server = "iCCup";

        string CDKeyROC = CFG->GetValidString( Prefix + "cdkeyroc", "FFFFFFFFFFFFFFFFFFFFFFFFFF" );
        string CDKeyTFT = CFG->GetValidString( Prefix + "cdkeytft", "FFFFFFFFFFFFFFFFFFFFFFFFFF" );
        string UserName = CFG->GetValidString( Prefix + "username", string( ) );
        string UserPassword = CFG->GetValidString( Prefix + "password", string( ) );

        if( CDKeyROC.empty( ) )
        {
            CONSOLE_Print( "[UNM] missing " + Prefix + "cdkeyroc, skipping this battle.net connection" );
            fixid++;
            continue;
        }
        else if( m_TFT && CDKeyTFT.empty( ) )
        {
            CONSOLE_Print( "[UNM] missing " + Prefix + "cdkeytft, skipping this battle.net connection" );
            fixid++;
            continue;
        }
        else if( UserName.empty( ) && i != 1 )
        {
            CONSOLE_Print( "[UNM] missing " + Prefix + "username, skipping this battle.net connection" );
            fixid++;
            continue;
        }
        else if( UserPassword.empty( ) && i != 1 )
        {
            CONSOLE_Print( "[UNM] missing " + Prefix + "password, skipping this battle.net connection" );
            fixid++;
            continue;
        }

        bool HoldFriends = CFG->GetInt( Prefix + "holdfriends", 1 ) != 0;
        bool HoldClan = CFG->GetInt( Prefix + "holdclan", 1 ) != 0;
        bool SpamBotFixBnetCommands = CFG->GetInt( Prefix + "spambotfixbnetcommands", 1 ) != 0;
        bool AllowManualRehost = CFG->GetInt( Prefix + "allowmanualrehost", 1 ) != 0;
        bool AllowUseCustomMapGameType = CFG->GetInt( Prefix + "allowusecustommapgametype", 0 ) != 0;
        bool AllowUseCustomUpTime = CFG->GetInt( Prefix + "allowusecustomuptime", 0 ) != 0;
        bool RunIccupLoader = CFG->GetInt( Prefix + "runiccuploader", 0 ) != 0;
        bool DisableMuteCommandOnBnet = CFG->GetInt( Prefix + "disablemutecommandonbnet", 1 ) != 0;
        bool Reconnect = m_Reconnect ? (CFG->GetInt( Prefix + "reconnectgproxy", 1 ) != 0 ) : false;
        unsigned char War3Version = static_cast<unsigned char>( CFG->GetUInt( Prefix + "custom_war3version", 26 ) );
        BYTEARRAY EXEVersion = UTIL_ExtractNumbers( CFG->GetString( Prefix + "custom_exeversion", string( ) ), 4 );
        BYTEARRAY EXEVersionHash = UTIL_ExtractNumbers( CFG->GetString( Prefix + "custom_exeversionhash", string( ) ), 4 );
        string EXEInfo = CFG->GetString( Prefix + "custom_exeinfo", string( ) );
        uint16_t Port = CFG->GetUInt16( Prefix + "port", 6112 );
        uint32_t LocaleID;
        uint32_t PacketDelaySmall = 0;
        uint32_t PacketDelayMedium = 0;
        uint32_t PacketDelayBig = 0;
        uint32_t SpamBotMessagesSendDelay = 0;
        uint32_t AutoRehostDelay = 0;
        uint32_t RefreshDelay = 0;
        uint32_t HostDelay = 0;
        uint32_t AllowAutoRehost = CFG->GetUInt( Prefix + "allowautorehost", 2 );
        uint32_t AllowAutoRefresh = CFG->GetUInt( Prefix + "allowautorefresh", 1 );
        uint32_t TimeWaitingToReconnect = CFG->GetUInt( Prefix + "timewaitingtoreconnect", 60 );
        uint32_t CustomMapGameType = CFG->GetUInt( Prefix + "custommapgametype", 0 );
        uint32_t CustomUpTime = CFG->GetUInt( Prefix + "customuptime", 0 );
        uint32_t DisableDefaultNotifyMessagesInGUI = CFG->GetUInt( Prefix + "disabledefaultnotifymessagesingui", 2 );
        uint32_t SpamBotOnlyPM = CFG->GetUInt( Prefix + "spambotonlypm", 4 );
        uint32_t SpamBotMode = CFG->GetUInt( Prefix + "spambotmode", 1 );
        uint32_t SpamBotNet = CFG->GetUInt( Prefix + "spambotnet", 1 );
        uint32_t PacketSendDelay = CFG->GetUInt( Prefix + "packetsenddelay", 0 );
        uint32_t MessageLength = CFG->GetUInt( Prefix + "messagelength", 200 );
        uint32_t MaxMessageLength = CFG->GetUInt( Prefix + "maxmessagelength", 0 );

        if( MaxMessageLength == 0 )
            MaxMessageLength = MessageLength * 3;
        else if( MaxMessageLength < MessageLength )
            MaxMessageLength = MessageLength;

        uint32_t HostName = CFG->GetUInt( Prefix + "hostname", 0 );
        string HostNameCustom = CFG->GetString( Prefix + "hostnamecustom", string( ) );
        string ServerAlias = CFG->GetString( Prefix + "serveralias", string( ) );
        string CountryAbbrev = CFG->GetString( Prefix + "countryabbrev", "RUS" );
        string Country = CFG->GetString( Prefix + "country", "Russia" );
        string Locale = CFG->GetString( Prefix + "locale", "system" );
        string BNETCommandTrigger = CFG->GetString( Prefix + "commandtrigger", "\\" );
        string FirstChannel = CFG->GetString( Prefix + "firstchannel", "UNM" );
        string PasswordHashType = CFG->GetString( Prefix + "custom_passwordhashtype", "pvpgn" );
        string PVPGNRealmName = CFG->GetString( Prefix + "custom_pvpgnrealmname", "PvPGN Realm" );
        string SpamBotMessagesSendDelayString = CFG->GetString( Prefix + "spambotmessagessenddelay", "0.030" );
        string AutoRehostDelayString = CFG->GetString( Prefix + "autorehostdelay", "0" );
        string RefreshDelayString = CFG->GetString( Prefix + "refreshdelay", "15.050" );
        string HostDelayString = CFG->GetString( Prefix + "hostdelay", "0.050" );
        string PacketDelaySmallString = CFG->GetString( Prefix + "packetdelaysmall", "default" );
        string PacketDelayMediumString = CFG->GetString( Prefix + "packetdelaymedium", "default" );
        string PacketDelayBigString = CFG->GetString( Prefix + "packetdelaybig", "default" );
        transform( PacketDelaySmallString.begin( ), PacketDelaySmallString.end( ), PacketDelaySmallString.begin( ), static_cast<int(*)(int)>(tolower) );
        transform( PacketDelayMediumString.begin( ), PacketDelayMediumString.end( ), PacketDelayMediumString.begin( ), static_cast<int(*)(int)>(tolower) );
        transform( PacketDelayBigString.begin( ), PacketDelayBigString.end( ), PacketDelayBigString.begin( ), static_cast<int(*)(int)>(tolower) );
        transform( PasswordHashType.begin( ), PasswordHashType.end( ), PasswordHashType.begin( ), static_cast<int(*)(int)>(tolower) );

        if( SpamBotMessagesSendDelayString.find_first_not_of( " " ) != string::npos && SpamBotMessagesSendDelayString.find_first_not_of( "0" ) != string::npos )
        {
            string::size_type FractionalStart = SpamBotMessagesSendDelayString.find( "." );

            if( FractionalStart != string::npos )
            {
                string Integer = SpamBotMessagesSendDelayString.substr( 0, FractionalStart );
                string Fractional = SpamBotMessagesSendDelayString.substr( FractionalStart + 1 );

                if( Integer.find_first_not_of( " " ) == string::npos )
                    Integer = "0";

                if( Fractional.find_first_of( "0123456789" ) == string::npos )
                    SpamBotMessagesSendDelayString = Integer + "000";
                else if( Fractional.size( ) == 1 )
                    SpamBotMessagesSendDelayString = Integer + Fractional + "00";
                else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    SpamBotMessagesSendDelayString = Integer + Fractional + "0";
                else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    SpamBotMessagesSendDelayString = Integer + Fractional;
                else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                    SpamBotMessagesSendDelayString = Integer + Fractional.substr( 0, 3 );
                else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                    SpamBotMessagesSendDelayString = Integer + Fractional.substr( 0, 2 ) + "0";
                else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                    SpamBotMessagesSendDelayString = Integer + Fractional.substr( 0, 1 ) + "00";
                else
                    SpamBotMessagesSendDelayString = Integer + "000";
            }
            else
            {
                FractionalStart = SpamBotMessagesSendDelayString.find( "," );

                if( FractionalStart != string::npos )
                {
                    string Integer = SpamBotMessagesSendDelayString.substr( 0, FractionalStart );
                    string Fractional = SpamBotMessagesSendDelayString.substr( FractionalStart + 1 );

                    if( Integer.find_first_not_of( " " ) == string::npos )
                        Integer = "0";

                    if( Fractional.find_first_of( "0123456789" ) == string::npos )
                        SpamBotMessagesSendDelayString = Integer + "000";
                    else if( Fractional.size( ) == 1 )
                        SpamBotMessagesSendDelayString = Integer + Fractional + "00";
                    else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        SpamBotMessagesSendDelayString = Integer + Fractional + "0";
                    else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        SpamBotMessagesSendDelayString = Integer + Fractional;
                    else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                        SpamBotMessagesSendDelayString = Integer + Fractional.substr( 0, 3 );
                    else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                        SpamBotMessagesSendDelayString = Integer + Fractional.substr( 0, 2 ) + "0";
                    else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                        SpamBotMessagesSendDelayString = Integer + Fractional.substr( 0, 1 ) + "00";
                    else
                        SpamBotMessagesSendDelayString = Integer + "000";
                }
                else
                    SpamBotMessagesSendDelayString = SpamBotMessagesSendDelayString + "000";
            }

            SpamBotMessagesSendDelay = UTIL_ToUInt32( SpamBotMessagesSendDelayString );
        }

        if( AutoRehostDelayString.find_first_not_of( " " ) != string::npos && AutoRehostDelayString.find_first_not_of( "0" ) != string::npos )
        {
            string::size_type FractionalStart = AutoRehostDelayString.find( "." );

            if( FractionalStart != string::npos )
            {
                string Integer = AutoRehostDelayString.substr( 0, FractionalStart );
                string Fractional = AutoRehostDelayString.substr( FractionalStart + 1 );

                if( Integer.find_first_not_of( " " ) == string::npos )
                    Integer = "0";

                if( Fractional.find_first_of( "0123456789" ) == string::npos )
                    AutoRehostDelayString = Integer + "000";
                else if( Fractional.size( ) == 1 )
                    AutoRehostDelayString = Integer + Fractional + "00";
                else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    AutoRehostDelayString = Integer + Fractional + "0";
                else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    AutoRehostDelayString = Integer + Fractional;
                else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                    AutoRehostDelayString = Integer + Fractional.substr( 0, 3 );
                else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                    AutoRehostDelayString = Integer + Fractional.substr( 0, 2 ) + "0";
                else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                    AutoRehostDelayString = Integer + Fractional.substr( 0, 1 ) + "00";
                else
                    AutoRehostDelayString = Integer + "000";
            }
            else
            {
                FractionalStart = AutoRehostDelayString.find( "," );

                if( FractionalStart != string::npos )
                {
                    string Integer = AutoRehostDelayString.substr( 0, FractionalStart );
                    string Fractional = AutoRehostDelayString.substr( FractionalStart + 1 );

                    if( Integer.find_first_not_of( " " ) == string::npos )
                        Integer = "0";

                    if( Fractional.find_first_of( "0123456789" ) == string::npos )
                        AutoRehostDelayString = Integer + "000";
                    else if( Fractional.size( ) == 1 )
                        AutoRehostDelayString = Integer + Fractional + "00";
                    else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        AutoRehostDelayString = Integer + Fractional + "0";
                    else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        AutoRehostDelayString = Integer + Fractional;
                    else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                        AutoRehostDelayString = Integer + Fractional.substr( 0, 3 );
                    else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                        AutoRehostDelayString = Integer + Fractional.substr( 0, 2 ) + "0";
                    else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                        AutoRehostDelayString = Integer + Fractional.substr( 0, 1 ) + "00";
                    else
                        AutoRehostDelayString = Integer + "000";
                }
                else
                    AutoRehostDelayString = AutoRehostDelayString + "000";
            }

            AutoRehostDelay = UTIL_ToUInt32( AutoRehostDelayString );
        }

        if( RefreshDelayString.find_first_not_of( " " ) != string::npos && RefreshDelayString.find_first_not_of( "0" ) != string::npos )
        {
            string::size_type FractionalStart = RefreshDelayString.find( "." );

            if( FractionalStart != string::npos )
            {
                string Integer = RefreshDelayString.substr( 0, FractionalStart );
                string Fractional = RefreshDelayString.substr( FractionalStart + 1 );

                if( Integer.find_first_not_of( " " ) == string::npos )
                    Integer = "0";

                if( Fractional.find_first_of( "0123456789" ) == string::npos )
                    RefreshDelayString = Integer + "000";
                else if( Fractional.size( ) == 1 )
                    RefreshDelayString = Integer + Fractional + "00";
                else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    RefreshDelayString = Integer + Fractional + "0";
                else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    RefreshDelayString = Integer + Fractional;
                else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                    RefreshDelayString = Integer + Fractional.substr( 0, 3 );
                else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                    RefreshDelayString = Integer + Fractional.substr( 0, 2 ) + "0";
                else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                    RefreshDelayString = Integer + Fractional.substr( 0, 1 ) + "00";
                else
                    RefreshDelayString = Integer + "000";
            }
            else
            {
                FractionalStart = RefreshDelayString.find( "," );

                if( FractionalStart != string::npos )
                {
                    string Integer = RefreshDelayString.substr( 0, FractionalStart );
                    string Fractional = RefreshDelayString.substr( FractionalStart + 1 );

                    if( Integer.find_first_not_of( " " ) == string::npos )
                        Integer = "0";

                    if( Fractional.find_first_of( "0123456789" ) == string::npos )
                        RefreshDelayString = Integer + "000";
                    else if( Fractional.size( ) == 1 )
                        RefreshDelayString = Integer + Fractional + "00";
                    else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        RefreshDelayString = Integer + Fractional + "0";
                    else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        RefreshDelayString = Integer + Fractional;
                    else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                        RefreshDelayString = Integer + Fractional.substr( 0, 3 );
                    else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                        RefreshDelayString = Integer + Fractional.substr( 0, 2 ) + "0";
                    else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                        RefreshDelayString = Integer + Fractional.substr( 0, 1 ) + "00";
                    else
                        RefreshDelayString = Integer + "000";
                }
                else
                    RefreshDelayString = RefreshDelayString + "000";
            }

            RefreshDelay = UTIL_ToUInt32( RefreshDelayString );
        }

        if( HostDelayString.find_first_not_of( " " ) != string::npos && HostDelayString.find_first_not_of( "0" ) != string::npos )
        {
            string::size_type FractionalStart = HostDelayString.find( "." );

            if( FractionalStart != string::npos )
            {
                string Integer = HostDelayString.substr( 0, FractionalStart );
                string Fractional = HostDelayString.substr( FractionalStart + 1 );

                if( Integer.find_first_not_of( " " ) == string::npos )
                    Integer = "0";

                if( Fractional.find_first_of( "0123456789" ) == string::npos )
                    HostDelayString = Integer + "000";
                else if( Fractional.size( ) == 1 )
                    HostDelayString = Integer + Fractional + "00";
                else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    HostDelayString = Integer + Fractional + "0";
                else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    HostDelayString = Integer + Fractional;
                else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                    HostDelayString = Integer + Fractional.substr( 0, 3 );
                else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                    HostDelayString = Integer + Fractional.substr( 0, 2 ) + "0";
                else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                    HostDelayString = Integer + Fractional.substr( 0, 1 ) + "00";
                else
                    HostDelayString = Integer + "000";
            }
            else
            {
                FractionalStart = HostDelayString.find( "," );

                if( FractionalStart != string::npos )
                {
                    string Integer = HostDelayString.substr( 0, FractionalStart );
                    string Fractional = HostDelayString.substr( FractionalStart + 1 );

                    if( Integer.find_first_not_of( " " ) == string::npos )
                        Integer = "0";

                    if( Fractional.find_first_of( "0123456789" ) == string::npos )
                        HostDelayString = Integer + "000";
                    else if( Fractional.size( ) == 1 )
                        HostDelayString = Integer + Fractional + "00";
                    else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        HostDelayString = Integer + Fractional + "0";
                    else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        HostDelayString = Integer + Fractional;
                    else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                        HostDelayString = Integer + Fractional.substr( 0, 3 );
                    else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                        HostDelayString = Integer + Fractional.substr( 0, 2 ) + "0";
                    else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                        HostDelayString = Integer + Fractional.substr( 0, 1 ) + "00";
                    else
                        HostDelayString = Integer + "000";
                }
                else
                    HostDelayString = HostDelayString + "000";
            }

            HostDelay = UTIL_ToUInt32( HostDelayString );
        }

        if( PacketDelaySmallString == "default" && PasswordHashType == "pvpgn" )
            PacketDelaySmall = 1500;
        else if( PacketDelaySmallString == "default" )
            PacketDelaySmall = 2000;
        else if( PacketDelaySmallString.find_first_not_of( " " ) != string::npos && PacketDelaySmallString.find_first_not_of( "0" ) != string::npos )
        {
            string::size_type FractionalStart = PacketDelaySmallString.find( "." );

            if( FractionalStart != string::npos )
            {
                string Integer = PacketDelaySmallString.substr( 0, FractionalStart );
                string Fractional = PacketDelaySmallString.substr( FractionalStart + 1 );

                if( Integer.find_first_not_of( " " ) == string::npos )
                    Integer = "0";

                if( Fractional.find_first_of( "0123456789" ) == string::npos )
                    PacketDelaySmallString = Integer + "000";
                else if( Fractional.size( ) == 1 )
                    PacketDelaySmallString = Integer + Fractional + "00";
                else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelaySmallString = Integer + Fractional + "0";
                else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelaySmallString = Integer + Fractional;
                else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelaySmallString = Integer + Fractional.substr( 0, 3 );
                else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelaySmallString = Integer + Fractional.substr( 0, 2 ) + "0";
                else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelaySmallString = Integer + Fractional.substr( 0, 1 ) + "00";
                else
                    PacketDelaySmallString = Integer + "000";
            }
            else
            {
                FractionalStart = PacketDelaySmallString.find( "," );

                if( FractionalStart != string::npos )
                {
                    string Integer = PacketDelaySmallString.substr( 0, FractionalStart );
                    string Fractional = PacketDelaySmallString.substr( FractionalStart + 1 );

                    if( Integer.find_first_not_of( " " ) == string::npos )
                        Integer = "0";

                    if( Fractional.find_first_of( "0123456789" ) == string::npos )
                        PacketDelaySmallString = Integer + "000";
                    else if( Fractional.size( ) == 1 )
                        PacketDelaySmallString = Integer + Fractional + "00";
                    else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelaySmallString = Integer + Fractional + "0";
                    else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelaySmallString = Integer + Fractional;
                    else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelaySmallString = Integer + Fractional.substr( 0, 3 );
                    else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelaySmallString = Integer + Fractional.substr( 0, 2 ) + "0";
                    else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelaySmallString = Integer + Fractional.substr( 0, 1 ) + "00";
                    else
                        PacketDelaySmallString = Integer + "000";
                }
                else
                    PacketDelaySmallString = PacketDelaySmallString + "000";
            }

            PacketDelaySmall = UTIL_ToUInt32( PacketDelaySmallString );
        }

        if( PacketDelayMediumString == "default" && PasswordHashType == "pvpgn" )
            PacketDelayMedium = 2500;
        else if( PacketDelayMediumString == "default" )
            PacketDelayMedium = 3500;
        else if( PacketDelayMediumString.find_first_not_of( " " ) != string::npos && PacketDelayMediumString.find_first_not_of( "0" ) != string::npos )
        {
            string::size_type FractionalStart = PacketDelayMediumString.find( "." );

            if( FractionalStart != string::npos )
            {
                string Integer = PacketDelayMediumString.substr( 0, FractionalStart );
                string Fractional = PacketDelayMediumString.substr( FractionalStart + 1 );

                if( Integer.find_first_not_of( " " ) == string::npos )
                    Integer = "0";

                if( Fractional.find_first_of( "0123456789" ) == string::npos )
                    PacketDelayMediumString = Integer + "000";
                else if( Fractional.size( ) == 1 )
                    PacketDelayMediumString = Integer + Fractional + "00";
                else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelayMediumString = Integer + Fractional + "0";
                else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelayMediumString = Integer + Fractional;
                else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelayMediumString = Integer + Fractional.substr( 0, 3 );
                else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelayMediumString = Integer + Fractional.substr( 0, 2 ) + "0";
                else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelayMediumString = Integer + Fractional.substr( 0, 1 ) + "00";
                else
                    PacketDelayMediumString = Integer + "000";
            }
            else
            {
                FractionalStart = PacketDelayMediumString.find( "," );

                if( FractionalStart != string::npos )
                {
                    string Integer = PacketDelayMediumString.substr( 0, FractionalStart );
                    string Fractional = PacketDelayMediumString.substr( FractionalStart + 1 );

                    if( Integer.find_first_not_of( " " ) == string::npos )
                        Integer = "0";

                    if( Fractional.find_first_of( "0123456789" ) == string::npos )
                        PacketDelayMediumString = Integer + "000";
                    else if( Fractional.size( ) == 1 )
                        PacketDelayMediumString = Integer + Fractional + "00";
                    else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelayMediumString = Integer + Fractional + "0";
                    else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelayMediumString = Integer + Fractional;
                    else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelayMediumString = Integer + Fractional.substr( 0, 3 );
                    else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelayMediumString = Integer + Fractional.substr( 0, 2 ) + "0";
                    else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelayMediumString = Integer + Fractional.substr( 0, 1 ) + "00";
                    else
                        PacketDelayMediumString = Integer + "000";
                }
                else
                    PacketDelayMediumString = PacketDelayMediumString + "000";
            }

            PacketDelayMedium = UTIL_ToUInt32( PacketDelayMediumString );
        }

        if( PacketDelayBigString == "default" && PasswordHashType == "pvpgn" )
            PacketDelayBig = 3000;
        else if( PacketDelayBigString == "default" )
            PacketDelayBig = 4000;
        else if( PacketDelayBigString.find_first_not_of( " " ) != string::npos && PacketDelayBigString.find_first_not_of( "0" ) != string::npos )
        {
            string::size_type FractionalStart = PacketDelayBigString.find( "." );

            if( FractionalStart != string::npos )
            {
                string Integer = PacketDelayBigString.substr( 0, FractionalStart );
                string Fractional = PacketDelayBigString.substr( FractionalStart + 1 );

                if( Integer.find_first_not_of( " " ) == string::npos )
                    Integer = "0";

                if( Fractional.find_first_of( "0123456789" ) == string::npos )
                    PacketDelayBigString = Integer + "000";
                else if( Fractional.size( ) == 1 )
                    PacketDelayBigString = Integer + Fractional + "00";
                else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelayBigString = Integer + Fractional + "0";
                else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelayBigString = Integer + Fractional;
                else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelayBigString = Integer + Fractional.substr( 0, 3 );
                else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelayBigString = Integer + Fractional.substr( 0, 2 ) + "0";
                else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                    PacketDelayBigString = Integer + Fractional.substr( 0, 1 ) + "00";
                else
                    PacketDelayBigString = Integer + "000";
            }
            else
            {
                FractionalStart = PacketDelayBigString.find( "," );

                if( FractionalStart != string::npos )
                {
                    string Integer = PacketDelayBigString.substr( 0, FractionalStart );
                    string Fractional = PacketDelayBigString.substr( FractionalStart + 1 );

                    if( Integer.find_first_not_of( " " ) == string::npos )
                        Integer = "0";

                    if( Fractional.find_first_of( "0123456789" ) == string::npos )
                        PacketDelayBigString = Integer + "000";
                    else if( Fractional.size( ) == 1 )
                        PacketDelayBigString = Integer + Fractional + "00";
                    else if( Fractional.size( ) == 2 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelayBigString = Integer + Fractional + "0";
                    else if( Fractional.size( ) == 3 && Fractional.find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelayBigString = Integer + Fractional;
                    else if( Fractional.substr( 0, 3 ).find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelayBigString = Integer + Fractional.substr( 0, 3 );
                    else if( Fractional.substr( 0, 2 ).find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelayBigString = Integer + Fractional.substr( 0, 2 ) + "0";
                    else if( Fractional.substr( 0, 1 ).find_first_not_of( "0123456789" ) == string::npos )
                        PacketDelayBigString = Integer + Fractional.substr( 0, 1 ) + "00";
                    else
                        PacketDelayBigString = Integer + "000";
                }
                else
                    PacketDelayBigString = PacketDelayBigString + "000";
            }

            PacketDelayBig = UTIL_ToUInt32( PacketDelayBigString );
        }

        if( AutoRehostDelay == 0 )
        {
            if( RefreshDelay > HostDelay )
                AutoRehostDelay = RefreshDelay;
            else
                AutoRehostDelay = HostDelay;
        }

        if( Locale == "system" )
        {
#ifdef WIN32
            LocaleID = GetUserDefaultLangID( );
            CONSOLE_Print( "[UNM] using system locale of " + UTIL_ToString( LocaleID ) );
#else
            LocaleID = 1033;
            CONSOLE_Print( "[UNM] unable to get system locale, using default locale of 1033" );
#endif
        }
        else
            LocaleID = UTIL_ToUInt32( Locale );

        if( BNETCommandTrigger.find_first_not_of( " " ) == string::npos )
            BNETCommandTrigger = "\\";

        if( FirstChannel.empty( ) )
            FirstChannel = "UNM";

        if( AllowAutoRehost > 2 )
            AllowAutoRehost = 1;

        if( AllowAutoRefresh > 3 )
            AllowAutoRefresh = 3;

        if( SpamBotOnlyPM > 4 )
            SpamBotOnlyPM = 4;

        if( SpamBotMode > 2 )
            SpamBotMode = 0;

        if( PacketSendDelay > 4 )
            PacketSendDelay = 0;

        bool SpamSubBot = false;
        bool SpamSubBotChat = false;
        uint32_t MainBotID = i - 1 - fixid;
        int32_t MainBotChatID = -1;

        for( vector<CBNET *> ::iterator j = m_BNETs.begin( ); j != m_BNETs.end( ); j++ )
        {
            if( ( *j )->GetServer( ) == Server )
            {
                if( !SpamSubBot )
                {
                    SpamSubBot = true;
                    MainBotID = ( *j )->GetHostCounterID( ) - 1;
                }

                if( ( *j )->m_SpamBotNet == SpamBotNet )
                {
                    SpamSubBotChat = true;
                    MainBotChatID = ( *j )->GetHostCounterID( );
                    break;
                }
            }
        }

        CONSOLE_Print( "[UNM] found battle.net connection #" + UTIL_ToString( i - fixid ) + " for server [" + Server + "]" );
        m_BNETs.push_back( new CBNET( this, Server, Port, TimeWaitingToReconnect, ServerAlias, CDKeyROC, CDKeyTFT, CountryAbbrev, Country, LocaleID, UserName, UserPassword, FirstChannel, HostName, HostNameCustom, BNETCommandTrigger, HoldFriends, HoldClan, War3Version, EXEVersion, EXEVersionHash, EXEInfo, PasswordHashType, PVPGNRealmName, SpamBotFixBnetCommands, SpamSubBot, SpamSubBotChat, MainBotID, MainBotChatID, SpamBotOnlyPM, SpamBotMessagesSendDelay, SpamBotMode, SpamBotNet, PacketSendDelay, MessageLength, MaxMessageLength, AllowAutoRehost, AllowAutoRefresh, AllowManualRehost, AllowUseCustomMapGameType, AllowUseCustomUpTime, CustomMapGameType, CustomUpTime, AutoRehostDelay, RefreshDelay, HostDelay, PacketDelaySmall, PacketDelayMedium, PacketDelayBig, RunIccupLoader, DisableMuteCommandOnBnet, DisableDefaultNotifyMessagesInGUI, ServerType, Reconnect, i - fixid ) );

        if( gWindowTitle.empty( ) )
            gWindowTitle = UserName;

        string firstBNETCommandTrigger = BNETCommandTrigger.substr( 0, 1 );
        QString CommandTriggerqstr = QString::fromUtf8( firstBNETCommandTrigger.c_str( ) );
        QString Serverqstr = QString::fromUtf8( Server.c_str( ) );
        QString UserNameqstr = QString::fromUtf8( UserName.c_str( ) );
        emit gToLog->addChatTab( static_cast<unsigned int>(i - fixid - 1), CommandTriggerqstr, Serverqstr, UserNameqstr, static_cast<unsigned int>(TimeWaitingToReconnect) );
    }

    if( m_BNETs.empty( ) )
    {
        uint32_t LocaleID;

#ifdef WIN32
        LocaleID = GetUserDefaultLangID( );
#else
        LocaleID = 1033;
#endif

        CONSOLE_Print( "[UNM] warning - no battle.net/pvpgn connections found in config file, created a blank template for connect to pvpgn server iCCup..." );
        m_BNETs.push_back( new CBNET( this, "iCCup", 6112, 5, "iCCup", "FFFFFFFFFFFFFFFFFFFFFFFFFF", "FFFFFFFFFFFFFFFFFFFFFFFFFF", "RUS", "Russia", LocaleID, string( ), string( ), "UNM", 0, string( ), "-!.@", true, true, 26, BYTEARRAY( ), BYTEARRAY( ), string( ), "pvpgn", "PvPGN Realm", true, false, false, 0, -1, 4, 30, 0, 0, 3, 200, 600, 1, 3, true, false, false, 0, 0, 15000, 15000, 100, 1500, 2500, 3000, true, true, 2, 0, false, 1 ) );
        emit gToLog->addChatTab( 0, QString( "-" ), QString( "iCCup" ), QString( ), 5 );
    }

    m_newGame = false;
    m_newGameServer = m_BNETs[0]->GetServer( );
    m_newGameGameState = GAME_PUBLIC;
    m_newGameName = string( );
    gBNETCommandTrigger = m_BNETs[0]->GetCommandTrigger( );
    string LastServer = string( );
    uint32_t LastRehostTimeFix = 0;
    uint32_t RehostDelaySubBots = 0;
    uint32_t RehostNumberSubBots = 0;

    for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
    {
        bool serverfound = false;

        for( vector<CBNET *> ::iterator it = m_BNETs.begin( ); it != m_BNETs.end( ); it++ )
        {
            if( ( *it )->GetHostCounterID( ) != ( *i )->GetHostCounterID( ) && ( *it )->GetServer( ) == ( *i )->GetServer( ) && ( *it )->m_SpamBotNet == ( *i )->m_SpamBotNet )
            {
                serverfound = true;
                break;
            }
        }

        if( !serverfound )
        {
            ( *i )->m_SpamBotMode = 0;
            ( *i )->m_SpamBotNet = 0;
        }
    }

    for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
    {
        if( ( *i )->m_SpamSubBot && LastServer != ( *i )->GetServer( ) && ( *i )->m_RehostTimeFix == 0 && ( *i )->m_RefreshTimeFix == 0 )
        {
            LastServer = ( *i )->GetServer( );
            LastRehostTimeFix = 0;
            RehostDelaySubBots = 0;
            RehostNumberSubBots = 0;

            for( vector<CBNET *> ::iterator it = m_BNETs.begin( ); it != m_BNETs.end( ); it++ )
            {
                if( ( *i )->GetServer( ) == ( *it )->GetServer( ) )
                {
                    if( ( *it )->m_SpamSubBot )
                    {
                        RehostNumberSubBots++;

                        if( ( *it )->m_RefreshDelay >= ( *it )->m_AutoRehostDelay && ( *it )->m_RefreshDelay >= ( *it )->m_HostDelay * 2 )
                            RehostDelaySubBots += ( *it )->m_RefreshDelay;
                        else if( ( *it )->m_HostDelay * 2 >= ( *it )->m_AutoRehostDelay && ( *it )->m_HostDelay * 2 >= ( *it )->m_RefreshDelay )
                            RehostDelaySubBots += ( *it )->m_HostDelay * 2;
                        else
                            RehostDelaySubBots += ( *it )->m_AutoRehostDelay;
                    }
                    else if( ( *i )->m_HostDelay == ( *it )->m_HostDelay && ( *i )->m_AutoRehostDelay == ( *it )->m_AutoRehostDelay && ( *i )->m_RefreshDelay == ( *it )->m_RefreshDelay )
                    {
                        RehostNumberSubBots++;

                        if( ( *it )->m_RefreshDelay >= ( *it )->m_AutoRehostDelay && ( *it )->m_RefreshDelay >= ( *it )->m_HostDelay * 2 )
                            RehostDelaySubBots += ( *it )->m_RefreshDelay;
                        else if( ( *it )->m_HostDelay * 2 >= ( *it )->m_AutoRehostDelay && ( *it )->m_HostDelay * 2 >= ( *it )->m_RefreshDelay )
                            RehostDelaySubBots += ( *it )->m_HostDelay * 2;
                        else
                            RehostDelaySubBots += ( *it )->m_AutoRehostDelay;
                    }
                }
            }

            if( RehostNumberSubBots > 1 )
            {
                LastRehostTimeFix = RehostDelaySubBots / RehostNumberSubBots;
                LastRehostTimeFix = LastRehostTimeFix / RehostNumberSubBots;

                if( LastRehostTimeFix == 0 )
                    LastRehostTimeFix = 1;

                if( ( *i )->m_AutoRehostDelay > ( *i )->m_RefreshDelay && ( *i )->m_AutoRehostDelay > ( *i )->m_HostDelay * 2 )
                {
                    for( vector<CBNET *> ::iterator it = m_BNETs.begin( ); it != m_BNETs.end( ); it++ )
                    {
                        if( ( *it )->m_SpamSubBot && ( *i )->GetServer( ) == ( *it )->GetServer( ) )
                            ( *it )->m_RehostTimeFix = LastRehostTimeFix;
                        else if( ( *i )->GetServer( ) == ( *it )->GetServer( ) && ( *i )->m_HostDelay == ( *it )->m_HostDelay && ( *i )->m_AutoRehostDelay == ( *it )->m_AutoRehostDelay && ( *i )->m_RefreshDelay == ( *it )->m_RefreshDelay )
                            ( *it )->m_RehostTimeFix = LastRehostTimeFix;
                    }
                }
                else
                {
                    for( vector<CBNET *> ::iterator it = m_BNETs.begin( ); it != m_BNETs.end( ); it++ )
                    {
                        if( ( *it )->m_SpamSubBot && ( *i )->GetServer( ) == ( *it )->GetServer( ) )
                            ( *it )->m_RefreshTimeFix = LastRehostTimeFix;
                        else if( ( *i )->GetServer( ) == ( *it )->GetServer( ) && ( *i )->m_HostDelay == ( *it )->m_HostDelay && ( *i )->m_AutoRehostDelay == ( *it )->m_AutoRehostDelay && ( *i )->m_RefreshDelay == ( *it )->m_RefreshDelay )
                            ( *it )->m_RefreshTimeFix = LastRehostTimeFix;
                    }
                }
            }
        }
    }

    for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
    {
        for( vector<CBNET *> ::iterator it = m_BNETs.begin( ); it != m_BNETs.end( ); it++ )
        {
            if( ( *it )->GetServer( ) == ( *i )->GetServer( ) )
            {
                ( *i )->m_AllBotIDs.append( ( *it )->GetHostCounterID( ) );

                if( ( *it )->m_SpamBotNet == ( *i )->m_SpamBotNet )
                    ( *i )->m_AllChatBotIDs.push_back( ( *it )->GetHostCounterID( ) );
            }
        }
    }

    for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
    {
        if( IsIccupServer( ( *i )->GetServer( ) ) && ( ( *i )->m_RunIccupLoader || ( *i )->GetHostCounterID( ) == 1 ) )
            IccupLoadersPorts.push_back( ( *i )->m_Port );
    }

    if( IsIccupServer( m_BNETs[0]->GetServer( ) ) )
        DelayedRunIccupLoaders = true;

    for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
    {
        if( i != m_BNETs.begin( ) && IsIccupServer( ( *i )->GetServer( ) ) && ( *i )->m_RunIccupLoader )
        {
            RunIccupLoaders( );
            break;
        }
    }

    m_SaveGame = nullptr;
    m_s = "а";
    m_st = "б";
    m_s1 = "в";
    m_s2 = "г";
    m_s3 = "д";
    m_s4 = "е";
    m_s5 = "ж";
    m_s6 = "з";
    m_s7 = "и";
    m_s8 = "й";
    m_s9 = "к";
    m_s0 = "л";
    m_s = m_s.substr( 1 ) + m_st.substr( 1 );
    m_s1 = m_s1.substr( 1 );
    m_s2 = m_s2.substr( 1 );
    m_s3 = m_s3.substr( 1 );
    m_s4 = m_s4.substr( 1 );
    m_s5 = m_s5.substr( 1 );
    m_s6 = m_s6.substr( 1 );
    m_s7 = m_s7.substr( 1 );
    m_s8 = m_s8.substr( 1 );
    m_s9 = m_s9.substr( 1 );
    m_s0 = m_s0.substr( 1 );
    m_ReconnectPort = CFG->GetUInt16( "bot_reconnectport", 6114 );
    m_LastGameName = string( );
    m_ActualRehostPrintingDelay = 1;
    m_WaitForEndGame = false;
    m_LogBackup = false;
    m_LogBackupForce = false;
    m_LastCheckLogTime = GetTicks( );
    m_LastCheckLogForSizeTime = 0;
    m_AuthorizationPassedTime = 1;
    m_DeleteOldMessagesTime = GetTime( );
    ReloadConfig( CFG );

    if( m_ShowPesettingWindow == 0 )
        gSkipPresetting = true;
    else if( m_ShowPesettingWindow == 1 )
    {
        vector<string> Cvars;
        Cvars.push_back( "bot_language =" );
        Cvars.push_back( "bot_war3path =" );
        Cvars.push_back( "bot_mappath =" );
        Cvars.push_back( "bot_mapcfgpath =" );
        Cvars.push_back( "bot_savegamepath =" );
        Cvars.push_back( "bot_replaypath =" );
        Cvars.push_back( "bot_rootadmins =" );

        if( UTIL_CheckVarInFile( "unm.cfg", Cvars ) == Cvars.size( ) )
            gSkipPresetting = true;

    }
    else if( m_ShowPesettingWindow == 2 && m_Warcraft3Path != UTIL_FixPath( "war3\\" ) && m_MapPath != UTIL_FixPath( "maps\\" ) )
        gSkipPresetting = true;
    else if( m_ShowPesettingWindow == 3 && m_MapPath != UTIL_FixPath( "maps\\" ) )
        gSkipPresetting = true;
    else if( m_ShowPesettingWindow == 4 && std::filesystem::exists( m_Warcraft3Path ) && std::filesystem::exists( m_MapPath ) )
        gSkipPresetting = true;

    // extract common.j and blizzard.j from War3Patch.mpq if we can
    // these two files are necessary for calculating "map_crc" when loading maps so we make sure to do it before loading the default map
    // see CMap::Load for more information

    ExtractScripts( );

    // load the default maps (note: make sure to run ExtractScripts first)

    if( m_DefaultMap.size( ) < 4 || m_DefaultMap.substr( m_DefaultMap.size( ) - 4 ) != ".cfg" )
    {
        m_DefaultMap += ".cfg";
        CONSOLE_Print( "[UNM] adding \".cfg\" to default map -> new default is [" + m_DefaultMap + "]" );
    }

    CConfig MapCFG;
    string MapCFGPath = CheckMapConfig( m_DefaultMap, true );

    if( MapCFGPath.empty( ) )
        MapCFGPath = m_DefaultMap;

    string MapPath = FindMap( m_DefaultMap.substr( 0, m_DefaultMap.size( ) - 4 ) );
    MapCFG.Read( MapCFGPath );
    m_Map = new CMap( this, &MapCFG, m_DefaultMap, MapPath );

    if( m_BNETs.empty( ) )
        CONSOLE_Print( "[UNM] warning - no battle.net connections found and no admin game created" );

    CONSOLE_Print( "[UNM] UNM bot v" + m_Version );
    CONSOLE_Print( "[UNM] Created by mothefuunnick. If you have questions or suggestions regarding the bot, visit our group in VK: https://vk.com/trikiman" );
}

CUNM::~CUNM( )
{
    delete m_ReconnectSocket;

    for( vector<CTCPSocket *> ::iterator i = m_ReconnectSockets.begin( ); i != m_ReconnectSockets.end( ); i++ )
        delete *i;

    delete m_GPSProtocol;
    delete m_CRC;
    delete m_SHA;

    for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
        delete *i;

    while( !RunnedIccupLoaders.empty( ) )
    {
        DWORD ExitCode;
        HANDLE hp;
        bool ret = true;

        hp = OpenProcess( PROCESS_ALL_ACCESS, true, RunnedIccupLoaders.back( ) );

        if( hp )
        {
            GetExitCodeProcess( hp, &ExitCode );
            ret = TerminateProcess( hp, ExitCode );
        }

        CloseHandle( hp );
        RunnedIccupLoaders.pop_back( );
    }

    if( m_CurrentGame != nullptr )
        delete m_CurrentGame;

    for( vector<CBaseGame *> ::iterator i = m_Games.begin( ); i != m_Games.end( ); i++ )
        delete *i;

    delete gLanguage;
    delete m_Map;

    if( m_SaveGame != nullptr )
        delete m_SaveGame;

    delete m_UDPSocket;
    delete m_LocalServer;
    delete m_LocalSocket;
    delete m_RemoteSocket;

    delete m_GameProtocol;

    while( !m_LocalPackets.empty( ) )
    {
        delete m_LocalPackets.front( );
        m_LocalPackets.pop( );
    }

    while( !m_RemotePackets.empty( ) )
    {
        delete m_RemotePackets.front( );
        m_RemotePackets.pop( );
    }

    while( !m_PacketBuffer.empty( ) )
    {
        delete m_PacketBuffer.front( );
        m_PacketBuffer.pop( );
    }
}

bool CUNM::Update( unsigned long usecBlock )
{
    // try to exit nicely if requested to do so

    if( m_ExitingNice )
    {
        if( m_CurrentGame )
        {
            CONSOLE_Print( "[UNM] deleting current game in preparation for exiting nicely" );
            m_CurrentGameName.clear( );
            delete m_CurrentGame;
            m_CurrentGame = nullptr;
        }

        if( m_Games.empty( ) )
        {
            if( !m_BNETs.empty( ) )
            {
                CONSOLE_Print( "[UNM] deleting all battle.net connections in preparation for exiting nicely" );

                for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
                    delete *i;

                m_BNETs.clear( );
            }

            CONSOLE_Print( "[UNM] all games finished, exiting nicely" );
            m_Exiting = true;
        }
    }

    // create the GProxy++ reconnect listener

    if( AuthorizationPassed && m_Reconnect )
    {
        if( !m_ReconnectSocket )
        {
            bool Success = false;

            for( uint32_t i = 0; i < 3; i++ )
            {
                m_ReconnectSocket = new CTCPServer( );

                if( m_ReconnectSocket->Listen( m_BindAddress, m_ReconnectPort, "[RECONNECT]" ) )
                {
                    CONSOLE_Print( "[UNM] listening for GProxy++ reconnects on port " + UTIL_ToString( m_ReconnectPort ) );
                    Success = true;
                    break;
                }
                else
                {
                    CONSOLE_Print( "[UNM] error listening for GProxy++ reconnects on port " + UTIL_ToString( m_ReconnectPort ) );
                    delete m_ReconnectSocket;
                    m_ReconnectSocket = nullptr;
//                    m_ReconnectPort++;
                }
            }

            if( !Success )
            {
                CONSOLE_Print( "[UNM] failed to listen for GProxy++ reconnects too many times, giving up" );
                m_Reconnect = false;
            }
        }
        else if( m_ReconnectSocket->HasError( ) )
        {
            CONSOLE_Print( "[UNM] GProxy++ reconnect listener error (" + m_ReconnectSocket->GetErrorString( ) + ")" );
            delete m_ReconnectSocket;
            m_ReconnectSocket = nullptr;
            m_Reconnect = false;
        }
    }

    uint32_t NumFDs = 0;

    // take every socket we own and throw it in one giant select statement so we can block on all sockets

    int nfds = 0;
    fd_set fd;
    fd_set send_fd;
    FD_ZERO( &fd );
    FD_ZERO( &send_fd );

    if( AuthorizationPassed )
    {
        // 1. all battle.net sockets

        for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
            NumFDs += ( *i )->SetFD( &fd, &send_fd, &nfds );

        // 2. the current game's server and player sockets

        if( m_CurrentGame )
            NumFDs += m_CurrentGame->SetFD( &fd, &send_fd, &nfds );

        // 3. all running games' player sockets

        for( vector<CBaseGame *> ::iterator i = m_Games.begin( ); i != m_Games.end( ); i++ )
            NumFDs += ( *i )->SetFD( &fd, &send_fd, &nfds );

        // 4. the GProxy++ reconnect socket(s)

        if( m_Reconnect && m_ReconnectSocket )
        {
            m_ReconnectSocket->SetFD( &fd, &send_fd, &nfds );
            NumFDs++;
        }

        for( vector<CTCPSocket *> ::iterator i = m_ReconnectSockets.begin( ); i != m_ReconnectSockets.end( ); i++ )
        {
            if( !( *i )->HasError( ) && ( *i )->GetConnected( ) )
            {
                ( *i )->SetFD( &fd, &send_fd, &nfds );
                NumFDs++;
            }
        }

        // 5. the local server

        if( m_LocalServer )
        {
            m_LocalServer->SetFD( &fd, &send_fd, &nfds );
            NumFDs++;
        }

        // 6. the local socket

        if( m_LocalSocket && !m_LocalSocket->HasError( ) && m_LocalSocket->GetConnected( ) )
        {
            m_LocalSocket->SetFD( &fd, &send_fd, &nfds );
            NumFDs++;
        }

        // 7. the remote socket

        if( m_RemoteSocket && !m_RemoteSocket->HasError( ) && m_RemoteSocket->GetConnected( ) )
        {
            m_RemoteSocket->SetFD( &fd, &send_fd, &nfds );
            NumFDs++;
        }

        // before we call select we need to determine how long to block for
        // previously we just blocked for a maximum of the passed usecBlock microseconds
        // however, in an effort to make game updates happen closer to the desired latency setting we now use a dynamic block interval
        // note: we still use the passed usecBlock as a hard maximum

        for( vector<CBaseGame *> ::iterator i = m_Games.begin( ); i != m_Games.end( ); i++ )
        {
            if( ( *i )->GetNextTimedActionTicks( ) * 1000 < usecBlock )
                usecBlock = ( *i )->GetNextTimedActionTicks( ) * 1000;
        }

        if( m_CurrentGame && m_CurrentGame->GetNextTimedActionTicks( ) * 1000 < usecBlock )
            usecBlock = m_CurrentGame->GetNextTimedActionTicks( ) * 1000;

        // always block for at least 1ms just in case something goes wrong
        // this prevents the bot from sucking up all the available CPU if a game keeps asking for immediate updates
        // it's a bit ridiculous to include this check since, in theory, the bot is programmed well enough to never make this mistake
        // however, considering who programmed it, it's worthwhile to do it anyway

        if( usecBlock < 1000 )
            usecBlock = 1000;
    }
    else if( m_BNETs.size( ) > 0 )
        NumFDs += m_BNETs[0]->SetFD( &fd, &send_fd, &nfds );

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = usecBlock;
    struct timeval send_tv;
    send_tv.tv_sec = 0;
    send_tv.tv_usec = 0;

#ifdef WIN32
    select( 1, &fd, nullptr, nullptr, &tv );
    select( 1, nullptr, &send_fd, nullptr, &send_tv );
#else
    select( nfds + 1, &fd, nullptr, nullptr, &tv );
    select( nfds + 1, nullptr, &send_fd, nullptr, &send_tv );
#endif

    if( NumFDs == 0 )
    {
        // we don't have any sockets (i.e. we aren't connected to battle.net maybe due to a lost connection and there aren't any games running)
        // select will return immediately and we'll chew up the CPU if we let it loop so just sleep for 50ms to kill some time

        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    }

    bool AdminExit = false;
    bool BNETExit = false;

    if( AuthorizationPassed )
    {
        if( m_AuthorizationPassedTime != 0 )
        {
            if( m_AuthorizationPassedTime == 1 )
                m_AuthorizationPassedTime = GetTicks( );
            else if( GetTicks( ) > m_AuthorizationPassedTime && GetTicks( ) - m_AuthorizationPassedTime > 6999 )
            {
                m_AuthorizationPassedTime = 0;

                for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
                    emit gToLog->toChat( "Подсказка: используйте команды для управления клиентом и встроенным ботом. Для просмотра списка доступных команд введите \"" + ( *i )->GetCommandTrigger( ) + "\" в поле ввода.7", QString( ), ( *i )->GetHostCounterID( ), -1 );
            }
        }

        if( GetTime( ) > m_DeleteOldMessagesTime + 300 )
        {
            m_DeleteOldMessagesTime = GetTime( );
            emit gToLog->deleteOldMessages( );
        }

        // check log for backup

        if( gAltThreadState == "@ThreadFinished" )
        {
            if( !gAltThreadStateMSG.empty( ) )
            {
                if( gLogMethod == 2 )
                {
                    gLog = new ofstream( );
                    gLog->open( gLogFile.c_str( ), ios::app );

                    if( gLog->fail( ) )
                    {
                        CONSOLE_Print( "[UNM] возникла ошибка при восстановлении логирования - не удалось открыть [" + gLogFile + "]. Значение параметра \"bot_logmethod\" изменено на '1'", 5 );
                        gLog->close( );
                        delete gLog;
                        gLog = nullptr;
                        gLogMethod = 1;
                    }
                }

                CONSOLE_Print( gAltThreadStateMSG, 5 );
                m_LogBackup = false;

                for( uint32_t i = 0; i != m_LogMessages.size( ); i++ )
                    CONSOLE_Print( m_LogMessages[i], 3 );

                m_LogMessages.clear( );
            }

            gAltThreadState = string( );
            gAltThreadStateMSG = string( );
        }
        else if( ( ( ( GetTicks( ) - m_LastCheckLogForSizeTime >= 3600000 || m_LastCheckLogForSizeTime == 0 ) && m_MaxLogSize > 0 ) || m_LogBackupForce ) && !m_LogBackup && !gLogFile.empty( ) && gAltThreadState.empty( ) )
        {
            m_LastCheckLogForSizeTime = GetTicks( );

            if( m_LogBackupForce || UTIL_FileSize( gLogFile ) > m_MaxLogSize )
            {
                if( !std::filesystem::exists( "logs_old" ) )
                {
                    if( std::filesystem::create_directory( "logs_old" ) )
                        CONSOLE_Print( "[UNM] Была создана папка \"logs_old\"" );
                    else
                        CONSOLE_Print( "[UNM] Не удалось создать папку \"logs_old\"" );
                }

                m_LogBackupForce = false;
                time_t now = time( nullptr );
                struct tm tstruct;
                char buf[80];
                tstruct = *localtime( &now );
                strftime( buf, sizeof( buf ), "%d.%m.%y(%H-%M-%S)", &tstruct );
                string NewLogFileName = "logs_old\\" + string( buf ) + gLogFile;

                if( UTIL_FileExists( NewLogFileName ) )
                {
                    m_LastCheckLogTime = m_LastCheckLogTime + 60000;
                    CONSOLE_Print( "[UNM] ошибка при бэкапе лог файла [" + NewLogFileName + "] - файл уже существует! Операция перенесена на 1 минуту." );
                }
                else
                {
                    m_LastCheckLogTime = GetTicks( );
                    CONSOLE_Print( "[UNM] начался процесс бэкапа лог файла [" + NewLogFileName + "]" );
                    m_LogBackup = true;
                    gAltThreadState = "@LogBackup";
                    QString QNewLogFileName = QString::fromUtf8( NewLogFileName.c_str( ) );
                    emit gToLog->backupLog( QNewLogFileName );
                }
            }
        }
        else if( m_MaxLogTime > 0 && GetTicks( ) - m_LastCheckLogTime >= m_MaxLogTime && !m_LogBackup && !gLogFile.empty( ) && gAltThreadState.empty( ) )
        {
            if( !std::filesystem::exists( "logs_old" ) )
            {
                if( std::filesystem::create_directory( "logs_old" ) )
                    CONSOLE_Print( "[UNM] Была создана папка \"logs_old\"" );
                else
                    CONSOLE_Print( "[UNM] Не удалось создать папку \"logs_old\"" );
            }

            time_t now = time( nullptr );
            struct tm tstruct;
            char buf[80];
            tstruct = *localtime( &now );
            strftime( buf, sizeof( buf ), "%d.%m.%y(%H-%M-%S)", &tstruct );
            string NewLogFileName = "logs_old\\" + string( buf ) + gLogFile;

            if( UTIL_FileExists( NewLogFileName ) )
            {
                m_LastCheckLogTime = m_LastCheckLogTime + 60000;
                CONSOLE_Print( "[UNM] ошибка при бэкапе лог файла [" + NewLogFileName + "] - файл уже существует! Операция перенесена на 1 минуту." );
            }
            else
            {
                m_LastCheckLogTime = GetTicks( );
                CONSOLE_Print( "[UNM] начался процесс бэкапа лог файла [" + NewLogFileName + "]" );
                m_LogBackup = true;
                gAltThreadState = "@LogBackup";
                QString QNewLogFileName = QString::fromUtf8( NewLogFileName.c_str( ) );
                emit gToLog->backupLog( QNewLogFileName );
            }
        }

        // update current game

        if( m_CurrentGame )
        {
            if( m_CurrentGame->Update( &fd, &send_fd ) )
            {
                CONSOLE_Print( "[UNM] deleting current game [" + IncGameNrDecryption( m_CurrentGame->GetGameName( ) ) + "]" );
                uint32_t GameCounter = m_CurrentGame->GetGameCounter( );
                m_CurrentGameName.clear( );
                delete m_CurrentGame;
                m_CurrentGame = nullptr;

                for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
                {
                    ( *i )->QueueGameUncreate( GameCounter );
                    ( *i )->QueueEnterChat( );
                }

                if( m_newGame )
                {
                    CreateGame( m_Map, m_newGameGameState, false, m_newGameName, m_newGameUser, m_newGameUser, m_newGameServer, true, string( ) );

                    if( m_CurrentGame )
                        m_newGame = false;
                }
            }
            else if( m_CurrentGame )
                m_CurrentGame->UpdatePost( &send_fd );
        }

        // update running games

        for( vector<CBaseGame *> ::iterator i = m_Games.begin( ); i != m_Games.end( ); )
        {
            if( ( *i )->Update( &fd, &send_fd ) )
            {
                CONSOLE_Print( "[UNM] deleting game [" + IncGameNrDecryption( ( *i )->GetGameName( ) ) + "]" );

                EventGameDeleted( *i );
                delete *i;
                i = m_Games.erase( i );

                if( m_CurrentGame == nullptr && m_newGame )
                {
                    CreateGame( m_Map, m_newGameGameState, false, m_newGameName, m_newGameUser, m_newGameUser, m_newGameServer, true, string( ) );

                    if( m_CurrentGame )
                        m_newGame = false;
                }
            }
            else
            {
                ( *i )->UpdatePost( &send_fd );
                i++;
            }
        }

        //
        // accept new connections
        //

        CTCPSocket *NewSocket = m_LocalServer->Accept( &fd );

        if( NewSocket )
        {
            if( m_LocalSocket )
            {
                // someone's already connected, reject the new connection
                // we only allow one person to use the proxy at a time

                delete NewSocket;
            }
            else
            {
                CONSOLE_Print( "[GPROXY] local player connected" );
                m_LocalSocket = NewSocket;
                m_LocalSocket->SetNoDelay( true );
                m_TotalPacketsReceivedFromLocal = 0;
                m_TotalPacketsReceivedFromRemote = 0;
                m_GameIsReliable = false;
                m_GameStarted = false;
                m_LeaveGameSent = false;
                m_ActionReceived = false;
                m_Synchronized = true;
                m_GPPort = 0;
                m_PID = 255;
                m_ChatPID = 255;
                m_ReconnectKey = 0;
                m_NumEmptyActions = 0;
                m_NumEmptyActionsUsed = 0;
                m_LastAckTime = 0;
                m_LastActionTime = 0;
                m_JoinedName.clear( );
                m_HostName.clear( );

                while( !m_PacketBuffer.empty( ) )
                {
                    delete m_PacketBuffer.front( );
                    m_PacketBuffer.pop( );
                }
            }
        }

        if( m_LocalSocket )
        {
            //
            // handle proxying (reconnecting, etc...)
            //

            if( m_LocalSocket->HasError( ) || !m_LocalSocket->GetConnected( ) )
            {
                CONSOLE_Print( "[GPROXY] local player disconnected" );
                bool actualgame = false;

                // в списке текущих игр у нас должны быть только те игры, которые имеются в списке bnet-игр и которые броадкастятся сейчас в lan
                // но мы не удаляем игру в которой находимся из списка текущих игр, даже если она пропадает из списка bnet-игр
                // поэтому после выхода из игры ищем ее в списке bnet-игр на каждом сервере, если игру не удалось найти или она не броадкастится в лан, то удаляем ее из списка текущих игр

                for( vector<CBNET *> ::iterator j = m_BNETs.begin( ); j != m_BNETs.end( ); j++ )
                {
                    if( !( *j )->m_SpamSubBot )
                    {
                        for( vector<CIncomingGameHost *>::iterator i = (*j)->m_GPGames.begin( ); i != (*j)->m_GPGames.end( ); i++ )
                        {
                            if( ( *i )->GetBroadcasted( ) && ( *i )->GetUniqueGameID( ) == m_CurrentGameID )
                                actualgame = true;
                        }
                    }
                }

                if( !actualgame )
                    emit gToLog->deleteGameFromList( { }, m_CurrentGameID, 2 );

                m_CurrentGameID = 0;
                BackToBnetChannel( );
                m_FakeLobby = false;
                delete m_LocalSocket;
                m_LocalSocket = nullptr;

                // ensure a leavegame message was sent, otherwise the server may wait for our reconnection which will never happen
                // if one hasn't been sent it's because Warcraft III exited abnormally

                if( m_GameIsReliable && !m_LeaveGameSent )
                {
                    // note: we're not actually 100% ensuring the leavegame message is sent, we'd need to check that DoSend worked, etc...

                    BYTEARRAY LeaveGame;
                    LeaveGame.push_back( 0xF7 );
                    LeaveGame.push_back( 0x21 );
                    LeaveGame.push_back( 0x08 );
                    LeaveGame.push_back( 0x00 );
                    UTIL_AppendByteArray( LeaveGame, (uint32_t)PLAYERLEAVE_GPROXY, false );
                    m_RemoteSocket->PutBytes( LeaveGame );
                    m_RemoteSocket->DoSend( &send_fd, "[GPROXY]" );
                }

                m_RemoteSocket->Reset( );
                m_RemoteSocket->SetNoDelay( true );
                m_RemoteServerIP.clear( );
                m_RemoteServerPort = 0;
            }
            else
            {
                if( m_FakeLobby && GetTime( ) - m_FakeLobbyTime > 60 && GetTime( ) > m_FakeLobbyTime )
                {
                    bool actualgame = false;

                    // в списке текущих игр у нас должны быть только те игры, которые имеются в списке bnet-игр и которые броадкастятся сейчас в lan
                    // но мы не удаляем игру в которой находимся из списка текущих игр, даже если она пропадает из списка bnet-игр
                    // поэтому после выхода из игры ищем ее в списке bnet-игр на каждом сервере, если игру не удалось найти или она не броадкастится в лан, то удаляем ее из списка текущих игр

                    for( vector<CBNET *> ::iterator j = m_BNETs.begin( ); j != m_BNETs.end( ); j++ )
                    {
                        if( !( *j )->m_SpamSubBot )
                        {
                            for( vector<CIncomingGameHost *>::iterator i = (*j)->m_GPGames.begin( ); i != (*j)->m_GPGames.end( ); i++ )
                            {
                                if( ( *i )->GetBroadcasted( ) && ( *i )->GetUniqueGameID( ) == m_CurrentGameID )
                                    actualgame = true;
                            }
                        }
                    }

                    if( !actualgame )
                        emit gToLog->deleteGameFromList( { }, m_CurrentGameID, 2 );

                    m_CurrentGameID = 0;
                    m_FakeLobby = false;
                    m_LocalSocket->Disconnect( );
                    delete m_LocalSocket;
                    m_LocalSocket = nullptr;
                    return false;
                }

                m_LocalSocket->DoRecv( &fd, "[GPROXY]" );
                ExtractLocalPackets( );
                ProcessLocalPackets( );

                if( !m_RemoteServerIP.empty( ) )
                {
                    if( m_GameIsReliable && m_ActionReceived && GetTime( ) - m_LastActionTime >= 60 )
                    {
                        if( m_NumEmptyActionsUsed < m_NumEmptyActions )
                        {
                            SendEmptyAction( );
                            m_NumEmptyActionsUsed++;
                        }
                        else
                        {
                            SendLocalChat( "GProxy++ ran out of time to reconnect, Warcraft III will disconnect soon." );
                            CONSOLE_Print( "[GPROXY] ran out of time to reconnect" );
                        }

                        m_LastActionTime = GetTime( );
                    }

                    if( m_RemoteSocket->HasError( ) )
                    {
                        CONSOLE_Print( "[GPROXY] disconnected from remote server due to socket error" );

                        if( m_GameIsReliable && m_ActionReceived && m_GPPort > 0 )
                        {
                            SendLocalChat( "You have been disconnected from the server due to a socket error." );
                            uint32_t TimeRemaining = ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 - ( GetTime( ) - m_LastActionTime );

                            if( GetTime( ) - m_LastActionTime > ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 )
                                TimeRemaining = 0;

                            SendLocalChat( "GProxy++ is attempting to reconnect... (" + UTIL_ToString( TimeRemaining ) + " seconds remain)" );
                            CONSOLE_Print( "[GPROXY] attempting to reconnect" );
                            m_RemoteSocket->Reset( );
                            m_RemoteSocket->SetNoDelay( true );
                            m_RemoteSocket->Connect( string( ), m_RemoteServerIP, m_GPPort, "[GPROXY]" );
                            m_LastConnectionAttemptTime = GetTime( );
                        }
                        else
                        {
                            bool actualgame = false;

                            // в списке текущих игр у нас должны быть только те игры, которые имеются в списке bnet-игр и которые броадкастятся сейчас в lan
                            // но мы не удаляем игру в которой находимся из списка текущих игр, даже если она пропадает из списка bnet-игр
                            // поэтому после выхода из игры ищем ее в списке bnet-игр на каждом сервере, если игру не удалось найти или она не броадкастится в лан, то удаляем ее из списка текущих игр

                            for( vector<CBNET *> ::iterator j = m_BNETs.begin( ); j != m_BNETs.end( ); j++ )
                            {
                                if( !( *j )->m_SpamSubBot )
                                {
                                    for( vector<CIncomingGameHost *>::iterator i = (*j)->m_GPGames.begin( ); i != (*j)->m_GPGames.end( ); i++ )
                                    {
                                        if( ( *i )->GetBroadcasted( ) && ( *i )->GetUniqueGameID( ) == m_CurrentGameID )
                                            actualgame = true;
                                    }
                                }
                            }

                            if( !actualgame )
                                emit gToLog->deleteGameFromList( { }, m_CurrentGameID, 2 );

                            m_CurrentGameID = 0;
                            BackToBnetChannel( );
                            m_FakeLobby = false;
                            m_LocalSocket->Disconnect( );
                            delete m_LocalSocket;
                            m_LocalSocket = nullptr;
                            m_RemoteSocket->Reset( );
                            m_RemoteSocket->SetNoDelay( true );
                            m_RemoteServerIP.clear( );
                            m_RemoteServerPort = 0;
                            return false;
                        }
                    }

                    if( !m_RemoteSocket->GetConnecting( ) && !m_RemoteSocket->GetConnected( ) )
                    {
                        CONSOLE_Print( "[GPROXY] disconnected from remote server" );

                        if( m_GameIsReliable && m_ActionReceived && m_GPPort > 0 )
                        {
                            SendLocalChat( "You have been disconnected from the server." );
                            uint32_t TimeRemaining = ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 - ( GetTime( ) - m_LastActionTime );

                            if( GetTime( ) - m_LastActionTime > ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 )
                                TimeRemaining = 0;

                            SendLocalChat( "GProxy++ is attempting to reconnect... (" + UTIL_ToString( TimeRemaining ) + " seconds remain)" );
                            CONSOLE_Print( "[GPROXY] attempting to reconnect" );
                            m_RemoteSocket->Reset( );
                            m_RemoteSocket->SetNoDelay( true );
                            m_RemoteSocket->Connect( string( ), m_RemoteServerIP, m_GPPort, "[GPROXY]" );
                            m_LastConnectionAttemptTime = GetTime( );
                        }
                        else
                        {
                            bool actualgame = false;

                            // в списке текущих игр у нас должны быть только те игры, которые имеются в списке bnet-игр и которые броадкастятся сейчас в lan
                            // но мы не удаляем игру в которой находимся из списка текущих игр, даже если она пропадает из списка bnet-игр
                            // поэтому после выхода из игры ищем ее в списке bnet-игр на каждом сервере, если игру не удалось найти или она не броадкастится в лан, то удаляем ее из списка текущих игр

                            for( vector<CBNET *> ::iterator j = m_BNETs.begin( ); j != m_BNETs.end( ); j++ )
                            {
                                if( !( *j )->m_SpamSubBot )
                                {
                                    for( vector<CIncomingGameHost *>::iterator i = (*j)->m_GPGames.begin( ); i != (*j)->m_GPGames.end( ); i++ )
                                    {
                                        if( ( *i )->GetBroadcasted( ) && ( *i )->GetUniqueGameID( ) == m_CurrentGameID )
                                            actualgame = true;
                                    }
                                }
                            }

                            if( !actualgame )
                                emit gToLog->deleteGameFromList( { }, m_CurrentGameID, 2 );

                            m_CurrentGameID = 0;
                            BackToBnetChannel( );
                            m_FakeLobby = false;
                            m_LocalSocket->Disconnect( );
                            delete m_LocalSocket;
                            m_LocalSocket = nullptr;
                            m_RemoteSocket->Reset( );
                            m_RemoteSocket->SetNoDelay( true );
                            m_RemoteServerIP.clear( );
                            m_RemoteServerPort = 0;
                            return false;
                        }
                    }

                    if( m_RemoteSocket->GetConnected( ) )
                    {
                        if( m_GameIsReliable && m_ActionReceived && m_GPPort > 0 && GetTime( ) - m_RemoteSocket->GetLastRecv( ) >= 20 )
                        {
                            CONSOLE_Print( "[GPROXY] disconnected from remote server due to 20 second timeout" );
                            SendLocalChat( "You have been timed out from the server." );
                            uint32_t TimeRemaining = ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 - ( GetTime( ) - m_LastActionTime );

                            if( GetTime( ) - m_LastActionTime > ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 )
                                TimeRemaining = 0;

                            SendLocalChat( "GProxy++ is attempting to reconnect... (" + UTIL_ToString( TimeRemaining ) + " seconds remain)" );
                            CONSOLE_Print( "[GPROXY] attempting to reconnect" );
                            m_RemoteSocket->Reset( );
                            m_RemoteSocket->SetNoDelay( true );
                            m_RemoteSocket->Connect( string( ), m_RemoteServerIP, m_GPPort, "[GPROXY]" );
                            m_LastConnectionAttemptTime = GetTime( );
                        }
                        else
                        {
                            m_RemoteSocket->DoRecv( &fd, "[GPROXY]" );
                            ExtractRemotePackets( );
                            ProcessRemotePackets( );

                            if( m_GameIsReliable && m_ActionReceived && m_GPPort > 0 && GetTime( ) - m_LastAckTime >= 10 )
                            {
                                m_RemoteSocket->PutBytes( m_GPSProtocol->SEND_GPSC_ACK( m_TotalPacketsReceivedFromRemote ) );
                                m_LastAckTime = GetTime( );
                            }

                            m_RemoteSocket->DoSend( &send_fd, "[GPROXY]" );
                        }
                    }

                    if( m_RemoteSocket->GetConnecting( ) )
                    {
                        // we are currently attempting to connect

                        if( m_RemoteSocket->CheckConnect( ) )
                        {
                            // the connection attempt completed

                            if( m_GameIsReliable && m_ActionReceived )
                            {
                                // this is a reconnection, not a new connection
                                // if the server accepts the reconnect request it will send a GPS_RECONNECT back requesting a certain number of packets

                                SendLocalChat( "GProxy++ reconnected to the server!" );
                                SendLocalChat( "==================================================" );
                                CONSOLE_Print( "[GPROXY] reconnected to remote server" );

                                // note: even though we reset the socket when we were disconnected, we haven't been careful to ensure we never queued any data in the meantime
                                // therefore it's possible the socket could have data in the send buffer
                                // this is bad because the server will expect us to send a GPS_RECONNECT message first
                                // so we must clear the send buffer before we continue
                                // note: we aren't losing data here, any important messages that need to be sent have been put in the packet buffer
                                // they will be requested by the server if required

                                m_RemoteSocket->ClearSendBuffer( );
                                m_RemoteSocket->PutBytes( m_GPSProtocol->SEND_GPSC_RECONNECT( m_PID, m_ReconnectKey, m_TotalPacketsReceivedFromRemote ) );

                                // we cannot permit any forwarding of local packets until the game is synchronized again
                                // this will disable forwarding and will be reset when the synchronization is complete

                                m_Synchronized = false;
                            }
                            else
                                CONSOLE_Print( "[GPROXY] connected to remote server" );
                        }
                        else if( GetTime( ) - m_LastConnectionAttemptTime >= 10 )
                        {
                            // the connection attempt timed out (10 seconds)

                            CONSOLE_Print( "[GPROXY] connect to remote server timed out" );

                            if( m_GameIsReliable && m_ActionReceived && m_GPPort > 0 )
                            {
                                uint32_t TimeRemaining = ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 - ( GetTime( ) - m_LastActionTime );

                                if( GetTime( ) - m_LastActionTime > ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 )
                                    TimeRemaining = 0;

                                SendLocalChat( "GProxy++ is attempting to reconnect... (" + UTIL_ToString( TimeRemaining ) + " seconds remain)" );
                                CONSOLE_Print( "[GPROXY] attempting to reconnect" );
                                m_RemoteSocket->Reset( );
                                m_RemoteSocket->SetNoDelay( true );
                                m_RemoteSocket->Connect( string( ), m_RemoteServerIP, m_GPPort, "[GPROXY]" );
                                m_LastConnectionAttemptTime = GetTime( );
                            }
                            else
                            {
                                bool actualgame = false;

                                // в списке текущих игр у нас должны быть только те игры, которые имеются в списке bnet-игр и которые броадкастятся сейчас в lan
                                // но мы не удаляем игру в которой находимся из списка текущих игр, даже если она пропадает из списка bnet-игр
                                // поэтому после выхода из игры ищем ее в списке bnet-игр на каждом сервере, если игру не удалось найти или она не броадкастится в лан, то удаляем ее из списка текущих игр

                                for( vector<CBNET *> ::iterator j = m_BNETs.begin( ); j != m_BNETs.end( ); j++ )
                                {
                                    if( !( *j )->m_SpamSubBot )
                                    {
                                        for( vector<CIncomingGameHost *>::iterator i = (*j)->m_GPGames.begin( ); i != (*j)->m_GPGames.end( ); i++ )
                                        {
                                            if( ( *i )->GetBroadcasted( ) && ( *i )->GetUniqueGameID( ) == m_CurrentGameID )
                                                actualgame = true;
                                        }
                                    }
                                }

                                if( !actualgame )
                                    emit gToLog->deleteGameFromList( { }, m_CurrentGameID, 2 );

                                m_CurrentGameID = 0;
                                BackToBnetChannel( );
                                m_FakeLobby = false;
                                m_LocalSocket->Disconnect( );
                                delete m_LocalSocket;
                                m_LocalSocket = nullptr;
                                m_RemoteSocket->Reset( );
                                m_RemoteSocket->SetNoDelay( true );
                                m_RemoteServerIP.clear( );
                                m_RemoteServerPort = 0;
                                return false;
                            }
                        }
                    }
                }

                m_LocalSocket->DoSend( &send_fd, "[GPROXY]" );
            }
        }
        else
        {
            //
            // handle game listing
            //

            if( GetTime( ) - m_LastRefreshTime >= 1 )
            {
                for( vector<CBNET *> ::iterator j = m_BNETs.begin( ); j != m_BNETs.end( ); j++ )
                {
                    if( !( *j )->m_SpamSubBot )
                    {
                        for( vector<CIncomingGameHost *>::iterator i = (*j)->m_GPGames.begin( ); i != (*j)->m_GPGames.end( ); )
                        {
                            // expire games older than 180 seconds

                            if( GetTime( ) - ( *i )->GetReceivedTime( ) >= 180 )
                            {
                                // don't forget to remove it from the LAN list first

                                if( ( *i )->GetBroadcasted( ) )
                                    m_UDPSocket->Broadcast( 6112, m_GameProtocol->SEND_W3GS_DECREATEGAME( ( *i )->GetUniqueGameID( ) ), "[GPROXY: " + ( *i )->GetGameName( ) + "]" );

                                emit gToLog->deleteGameFromList( ( *j )->m_AllBotIDs, ( *i )->GetUniqueGameID( ), m_CurrentGameID != ( *i )->GetUniqueGameID( ) ? 0 : 1 );
                                delete *i;
                                i = (*j)->m_GPGames.erase( i );
                                continue;
                            }

                            if( ( *i )->GetBroadcasted( ) )
                            {
                                BYTEARRAY MapGameType;
                                UTIL_AppendByteArray( MapGameType, ( *i )->GetGameType( ), false );
                                UTIL_AppendByteArray( MapGameType, ( *i )->GetParameter( ), false );
                                BYTEARRAY MapFlags = UTIL_CreateByteArray( ( *i )->GetMapFlags( ), false );
                                BYTEARRAY MapWidth = UTIL_CreateByteArray( ( *i )->GetMapWidth( ), false );
                                BYTEARRAY MapHeight = UTIL_CreateByteArray( ( *i )->GetMapHeight( ), false );
                                string GameName = ( *i )->GetGameName( );

                                // colour reliable game names so they're easier to pick out of the list

                                if( ( *i )->GetGProxy( ) )
                                {
                                    GameName = "|cFF4080C0" + GameName;

                                    // unfortunately we have to truncate them
                                    // is this acceptable?

                                    if( GameName.size( ) > 31 )
                                        GameName = GameName.substr( 0, 31 );
                                }

                                m_UDPSocket->Broadcast( 6112, m_GameProtocol->SEND_W3GS_GAMEINFO( m_TFT, m_LANWar3Version, MapGameType, MapFlags, MapWidth, MapHeight, GameName, ( *i )->GetHostName( ), ( *i )->GetElapsedTime( ), ( *i )->GetMapPath( ), ( *i )->GetMapCRC( ), 12, 12, m_Port, ( *i )->GetUniqueGameID( ), ( *i )->GetUniqueGameID( ), "[GPROXY: " + ( *i )->GetGameName( ) + "]", 0 ), "[GPROXY: " + ( *i )->GetGameName( ) + "]" );
                            }

                            i++;
                        }
                    }
                }

                for( vector<CIrinaGame *>::iterator i = m_IrinaServer->m_GPGames.begin( ); i != m_IrinaServer->m_GPGames.end( ); i++ )
                {
                    // проверяем не должен ли нам прийти ответ от сервера с детальной информацией от этой игре
                    // если мы отправили серверу запрос на инфу о игре, но сервер нам ничего не отправил в течении 3-ёх секунд - больше не ждем пакет и отправляем сигнал в gui

                    // броадкастим игру в LAN

                    if( ( *i )->GetBroadcasted( ) && ( *i )->GetDataReceived( ) )
                        m_UDPSocket->Broadcast( 6112, m_IrinaServer->SEND_W3GS_GAMEINFO_IRINA( ( *i )->m_GameData, ( *i )->GetUniqueGameID( ), ( *i )->GetGameTicks( ) / 1000 ), "[GPROXY: " + ( *i )->GetGameName( ) + "]" );
                }

                m_LastRefreshTime = GetTime( );
            }
        }

        // update GProxy++ reliable reconnect sockets

        if( m_Reconnect && m_ReconnectSocket )
        {
            CTCPSocket *NewSocket = m_ReconnectSocket->Accept( &fd );

            if( NewSocket )
                m_ReconnectSockets.push_back( NewSocket );
        }

        for( vector<CTCPSocket *> ::iterator i = m_ReconnectSockets.begin( ); i != m_ReconnectSockets.end( ); )
        {
            if( ( *i )->HasError( ) || !( *i )->GetConnected( ) || GetTime( ) - ( *i )->GetLastRecv( ) >= 10 )
            {
                delete *i;
                i = m_ReconnectSockets.erase( i );
                continue;
            }

            ( *i )->DoRecv( &fd, "[RECONNECT]" );
            string *RecvBuffer = ( *i )->GetBytes( );
            BYTEARRAY Bytes = UTIL_CreateByteArray( (unsigned char *)RecvBuffer->c_str( ), RecvBuffer->size( ) );

            // a packet is at least 4 bytes

            if( Bytes.size( ) >= 4 )
            {
                if( Bytes[0] == GPS_HEADER_CONSTANT )
                {
                    // bytes 2 and 3 contain the length of the packet

                    uint16_t Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

                    if( Length >= 4 )
                    {
                        if( Bytes.size( ) >= Length )
                        {
                            if( Bytes[1] == CGPSProtocol::GPS_RECONNECT && Length == 13 )
                            {
                                unsigned char PID = Bytes[4];
                                uint32_t ReconnectKey = UTIL_ByteArrayToUInt32( Bytes, false, 5 );
                                uint32_t LastPacket = UTIL_ByteArrayToUInt32( Bytes, false, 9 );

                                // look for a matching player in a running game

                                CGamePlayer *Match = nullptr;

                                for( vector<CBaseGame *> ::iterator j = m_Games.begin( ); j != m_Games.end( ); j++ )
                                {
                                    if( ( *j )->GetGameLoaded( ) )
                                    {
                                        CGamePlayer *Player = ( *j )->GetPlayerFromPID( PID );

                                        if( Player && Player->GetGProxy( ) && Player->GetGProxyReconnectKey( ) == ReconnectKey )
                                        {
                                            Match = Player;
                                            break;
                                        }
                                    }
                                }

                                if( Match )
                                {
                                    // reconnect successful!

                                    *RecvBuffer = RecvBuffer->substr( Length );
                                    Match->EventGProxyReconnect( *i, LastPacket );
                                    i = m_ReconnectSockets.erase( i );
                                    continue;
                                }
                                else
                                {
                                    ( *i )->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_NOTFOUND ) );
                                    ( *i )->DoSend( &send_fd, "[RECONNECT]" );
                                    delete *i;
                                    i = m_ReconnectSockets.erase( i );
                                    continue;
                                }
                            }
                            else
                            {
                                ( *i )->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_INVALID ) );
                                ( *i )->DoSend( &send_fd, "[RECONNECT]" );
                                delete *i;
                                i = m_ReconnectSockets.erase( i );
                                continue;
                            }
                        }
                    }
                    else
                    {
                        ( *i )->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_INVALID ) );
                        ( *i )->DoSend( &send_fd, "[RECONNECT]" );
                        delete *i;
                        i = m_ReconnectSockets.erase( i );
                        continue;
                    }
                }
                else
                {
                    ( *i )->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_INVALID ) );
                    ( *i )->DoSend( &send_fd, "[RECONNECT]" );
                    delete *i;
                    i = m_ReconnectSockets.erase( i );
                    continue;
                }
            }

            ( *i )->DoSend( &send_fd, "[RECONNECT]" );
            i++;
        }

        // update battle.net connections

        for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
        {
            if( ( *i )->Update( &fd, &send_fd ) )
                BNETExit = true;
        }

        // update irina server connection

        if( m_IrinaServer->Update( ) )
        {
            CONSOLE_Print( "[UNM] deleting irina server connection" );
            delete m_IrinaServer;
            m_IrinaServer = nullptr;
        }
    }
    else if( m_BNETs.size( ) > 0 && m_BNETs[0]->Update( &fd, &send_fd ) )
        BNETExit = true;

    if( m_Exiting || AdminExit || BNETExit )
        return true;

    return false;
}

void CUNM::BackToBnetChannel( )
{
    if( !m_CurrentGame )
    {
        for( vector<CBNET *> ::iterator j = m_BNETs.begin( ); j != m_BNETs.end( ); j++ )
        {
            if( ( *j )->GetInGame( ) )
                ( *j )->QueueEnterChat( );
        }
    }
}

void CUNM::EventBNETGameRefreshed( CBNET *bnet )
{
    bnet->m_QuietRehost = true;

    if( m_CurrentGame )
    {
        m_LastGameName = m_CurrentGame->GetGameName( );
        CONSOLE_Print( "[GAME: " + IncGameNrDecryption( m_CurrentGame->GetGameName( ) ) + "] rehost worked on server [" + bnet->GetServerAlias( ) + "] by username [" + bnet->GetName( ) + "]", 6, m_CurrentGame->GetCurrentGameCounter( ) );

        if( m_RehostPrinting )
        {
            if( m_RehostPrintingDelay > m_ActualRehostPrintingDelay )
                m_ActualRehostPrintingDelay++;
            else
            {
                m_ActualRehostPrintingDelay = 1;
                m_CurrentGame->SendAllChat( "Пересоздано как \"" + IncGameNrDecryption( m_CurrentGame->GetGameName( ) ) + "\"" );
            }
        }
    }
}

void CUNM::EventBNETGameRefreshFailed( CBNET *bnet )
{
    // 1. поведение при ошибке рехоста не настраивается в конфиге
    // 2. делаем анхост (на сервере) немедленно после ошибки - аккаунт должен вернутся на канал!
    // 3. при этом имя игры мы не меняем!
    // 4. предупреждаем об ошибках в чат игры/админ-игры - согласно кварам в кфг
    // 5. предупреждаем об ошибках в лс создателя игры только о ошибках при первом рехосте (или если тип игры не публичныый) и только на мейн акке
    // 6. если ошибка при первом рехосте и на том сервере, с которого была созданна игра - то игру удаляем + изменяем предупреждение, которое отправляется создателю игру в лс

    if( m_CurrentGame )
    {
        m_LastGameName = m_CurrentGame->GetGameName( );
        CONSOLE_Print( "[GAME: " + IncGameNrDecryption( m_CurrentGame->GetGameName( ) ) + "] rehost error (startadvex3 failed) on server [" + bnet->GetServerAlias( ) + "] by username [" + bnet->GetName( ) + "]", 6, m_CurrentGame->GetCurrentGameCounter( ) );

        if( !bnet->m_SpamSubBot )
        {
            if( bnet->GetServer( ) == m_CurrentGame->GetCreatorServer( ) )
            {
                if( !bnet->m_QuietRehost )
                {
                    m_CurrentGame->SetExiting( true );
                    bnet->QueueChatCommand( "Невозможно создать игру [" + m_CurrentGame->GetGameName( ) + "] на сервере [" + bnet->GetServer( ) + "]. Игра отменена!", m_CurrentGame->GetCreatorName( ), true );
                }
                else if( m_CurrentGame->GetGameState( ) != GAME_PUBLIC )
                    bnet->QueueChatCommand( gLanguage->UnableToCreateGameTryAnotherName( bnet->GetServer( ), m_CurrentGame->GetGameName( ) ), m_CurrentGame->GetCreatorName( ), true );
            }
            else if( !bnet->m_QuietRehost || m_CurrentGame->GetGameState( ) != GAME_PUBLIC )
            {
                for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
                {
                    if( ( *i )->GetLoggedIn( ) && !( *i )->m_SpamSubBot && ( *i )->GetServer( ) == m_CurrentGame->GetCreatorServer( ) )
                    {
                        ( *i )->QueueChatCommand( gLanguage->UnableToCreateGameTryAnotherName( bnet->GetServer( ), m_CurrentGame->GetGameName( ) ), m_CurrentGame->GetCreatorName( ), true );
                        break;
                    }
                }
            }
        }

        if( !m_CurrentGame->GetExiting( ) )
        {
            bnet->ClearQueueGameRehost( );
            bnet->QueueEnterChat( );
            bnet->m_GameIsReady = false;
            bnet->m_ResetUncreateTime = true;

            if( bnet->m_RehostTimeFix != 0 )
            {
                for( vector<CBNET *> ::iterator it = m_BNETs.begin( ); it != m_BNETs.end( ); it++ )
                {
                    if( ( *it )->m_RehostTimeFix != 0 && ( *it )->GetServer( ) == bnet->GetServer( ) )
                        ( *it )->m_LastRehostTimeFix = GetTicks( );
                }
            }
        }

        if( m_RehostFailedPrinting )
            m_CurrentGame->SendAllChat( gLanguage->BNETGameHostingFailed( bnet->GetServer( ), m_CurrentGame->GetGameName( ) ) );
    }
}

void CUNM::EventGameDeleted( CBaseGame *game )
{
    for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
    {
        if( !( *i )->m_SpamSubBot )
        {
            if( m_WaitForEndGame )
                m_WaitForEndGame = false;

            if( m_GameIsOverMessagePrinting != 0 )
            {
                if( m_GameIsOverMessagePrinting == 1 || m_GameIsOverMessagePrinting == 2 )
                    ( *i )->QueueChatCommand( gLanguage->GameIsOver( game->GetDescription( ) ) );

                if( ( *i )->GetServer( ) == game->GetCreatorServer( ) && ( m_GameIsOverMessagePrinting == 1 || m_GameIsOverMessagePrinting == 3 ) )
                    ( *i )->QueueChatCommand( gLanguage->GameIsOver( game->GetDescription( ) ), game->GetCreatorName( ), true );
            }
        }
    }
}


void CUNM::ExtractScripts( )
{
    HANDLE PatchMPQ;
    string PatchMPQFileName = m_Warcraft3Path + "War3Patch.mpq";
    bool PatchMPQFileNameNewFound = UTIL_FileExists( m_Warcraft3Path + "War3x.mpq" );

    if( !UTIL_FileExists( PatchMPQFileName ) || ( PatchMPQFileNameNewFound && UTIL_FileExists( m_Warcraft3Path + "Warcraft III.exe" ) && !UTIL_FileExists( m_Warcraft3Path + "war3.exe" ) ) )
    {
        if( PatchMPQFileNameNewFound )
            PatchMPQFileName = m_Warcraft3Path + "War3x.mpq";
        else
        {
            CONSOLE_Print( "[UNM] warning - unable to load MPQ file [" + PatchMPQFileName + "] - файл не найден" );
            return;
        }
    }

    std::wstring WPatchMPQFileName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(PatchMPQFileName);

    if( SFileOpenArchive( WPatchMPQFileName.c_str( ), 0, MPQ_OPEN_FORCE_MPQ_V1, &PatchMPQ ) )
    {
        CONSOLE_Print( "[UNM] loading MPQ file [" + PatchMPQFileName + "]" );
        HANDLE SubFile;

        // common.j

        if( SFileOpenFileEx( PatchMPQ, "Scripts\\common.j", 0, &SubFile ) )
        {
            uint32_t FileLength = SFileGetFileSize( SubFile, nullptr );

            if( FileLength > 0 && FileLength != 0xFFFFFFFF )
            {
                char *SubFileData = new char[FileLength];
                DWORD BytesRead = 0;

                if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead, nullptr ) )
                {
                    CONSOLE_Print( "[UNM] extracting Scripts\\common.j from MPQ file to [" + m_MapCFGPath + "common.j]" );
                    UTIL_FileWrite( m_MapCFGPath + "common.j", reinterpret_cast<unsigned char *>(SubFileData), BytesRead );
                }
                else
                    CONSOLE_Print( "[UNM] warning - unable to extract Scripts\\common.j from MPQ file" );

                delete[] SubFileData;
            }

            SFileCloseFile( SubFile );
        }
        else
            CONSOLE_Print( "[UNM] couldn't find Scripts\\common.j in MPQ file" );

        // blizzard.j

        if( SFileOpenFileEx( PatchMPQ, "Scripts\\blizzard.j", 0, &SubFile ) )
        {
            uint32_t FileLength = SFileGetFileSize( SubFile, nullptr );

            if( FileLength > 0 && FileLength != 0xFFFFFFFF )
            {
                char *SubFileData = new char[FileLength];
                DWORD BytesRead = 0;

                if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead, nullptr ) )
                {
                    CONSOLE_Print( "[UNM] extracting Scripts\\blizzard.j from MPQ file to [" + m_MapCFGPath + "blizzard.j]" );
                    UTIL_FileWrite( m_MapCFGPath + "blizzard.j", reinterpret_cast<unsigned char *>(SubFileData), BytesRead );
                }
                else
                    CONSOLE_Print( "[UNM] warning - unable to extract Scripts\\blizzard.j from MPQ file" );

                delete[] SubFileData;
            }

            SFileCloseFile( SubFile );
        }
        else
            CONSOLE_Print( "[UNM] couldn't find Scripts\\blizzard.j in MPQ file" );

        SFileCloseArchive( PatchMPQ );
    }
    else
        CONSOLE_Print( "[UNM] warning - unable to load MPQ file [" + PatchMPQFileName + "] - error code " + UTIL_ToString( GetLastError( ) ) );
}

void CUNM::CreateGame( CMap *map, unsigned char gameState, bool saveGame, string gameName, string ownerName, string creatorName, string creatorServer, bool whisper, string hostmapcode )
{
    if( gameName.size( ) > 31 )
    {
        for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
        {
            if( !( *i )->m_SpamSubBot )
            {
                if( ( *i )->GetServer( ) == creatorServer )
                    ( *i )->QueueChatCommand( gLanguage->UnableToCreateGameNameTooLong( gameName ), creatorName, whisper );
            }
        }

        return;
    }

    if( !map->GetValid( ) )
    {
        for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
        {
            if( !( *i )->m_SpamSubBot )
            {
                if( ( *i )->GetServer( ) == creatorServer )
                    ( *i )->QueueChatCommand( gLanguage->UnableToCreateGameInvalidMap( gameName ), creatorName, whisper );
            }
        }

        return;
    }

    if( saveGame )
    {
        if( !m_SaveGame->GetValid( ) )
        {
            for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
            {
                if( !( *i )->m_SpamSubBot )
                {
                    if( ( *i )->GetServer( ) == creatorServer )
                        ( *i )->QueueChatCommand( gLanguage->UnableToCreateGameInvalidSaveGame( gameName ), creatorName, whisper );
                }
            }

            return;
        }

        string MapPath1 = m_SaveGame->GetMapPath( );
        string MapPath2 = map->GetMapPath( );
        MapPath1 = UTIL_StringToLower( MapPath1 );
        MapPath2 = UTIL_StringToLower( MapPath2 );

        if( MapPath1 != MapPath2 )
        {
            CONSOLE_Print( "[UNM] path mismatch, saved game path is [" + MapPath1 + "] but map path is [" + MapPath2 + "]" );

            for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
            {
                if( !( *i )->m_SpamSubBot )
                {
                    if( ( *i )->GetServer( ) == creatorServer )
                        ( *i )->QueueChatCommand( gLanguage->UnableToCreateGameSaveGameMapMismatch( gameName ), creatorName, whisper );
                }
            }

            return;
        }

        if( m_EnforcePlayers.empty( ) )
        {
            for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
            {
                if( !( *i )->m_SpamSubBot )
                {
                    if( ( *i )->GetServer( ) == creatorServer )
                        ( *i )->QueueChatCommand( gLanguage->UnableToCreateGameMustEnforceFirst( gameName ), creatorName, whisper );
                }
            }

            return;
        }
    }

    if( m_CurrentGame )
    {
        for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
        {
            if( !( *i )->m_SpamSubBot )
            {
                if( ( *i )->GetServer( ) == creatorServer )
                    ( *i )->QueueChatCommand( gLanguage->UnableToCreateGameAnotherGameInLobby( gameName, m_CurrentGame->GetDescription( ) ), creatorName, whisper );
            }
        }

        return;
    }

    if( m_Games.size( ) >= m_MaxGames )
    {
        for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
        {
            if( !( *i )->m_SpamSubBot )
            {
                if( ( *i )->GetServer( ) == creatorServer )
                    ( *i )->QueueChatCommand( gLanguage->UnableToCreateGameMaxGamesReached( gameName, UTIL_ToString( m_MaxGames ) ), creatorName, whisper );
            }
        }

        return;
    }
    else if( !saveGame && m_SaveGame != nullptr )
    {
        delete m_SaveGame;
        m_SaveGame = nullptr;
    }

    CONSOLE_Print( "[UNM] creating game [" + gameName + "]" );
    m_LastGameName = gameName;

    if( gameState != GAME_PRIVATE && gameState != GAME_PUBLIC )
        gameState = static_cast<unsigned char>(m_gamestateinhouse);

    for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
    {
        if( !( *i )->m_SpamSubBot )
        {
            if( saveGame )
                ( *i )->QueueGameCreate( );
            else
                ( *i )->QueueGameCreate( );
        }
    }

    if( saveGame )
        m_CurrentGame = new CGame( this, map, m_SaveGame, m_HostPort, gameState, gameName, ownerName, creatorName, creatorServer );
    else
        m_CurrentGame = new CGame( this, map, nullptr, m_HostPort, gameState, gameName, ownerName, creatorName, creatorServer );

    if( !m_CurrentGame || m_CurrentGame == nullptr )
    {
        for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
        {
            if( !( *i )->m_SpamSubBot )
            {
                if( ( *i )->GetServer( ) == creatorServer )
                    ( *i )->QueueChatCommand( "Не удалось создать игру из-за неизвестной ошибки.", creatorName, whisper );
            }
        }

        return;
    }
    else if( m_CurrentGame->m_ErrorListing )
    {
        for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
        {
            if( !( *i )->m_SpamSubBot )
            {
                if( ( *i )->GetServer( ) == creatorServer )
                    ( *i )->QueueChatCommand( "Не удалось создать игру из-за ошибки листинга.", creatorName, whisper );
            }
        }

        return;
    }

    if( m_SaveGame )
    {
        m_CurrentGame->SetEnforcePlayers( m_EnforcePlayers );
        m_EnforcePlayers.clear( );
    }

    // add game tab in GUI

    emit gToLog->addGame( m_CurrentGame->GetCurrentGameCounter( ), m_CurrentGame->GetGameDataInit( ), m_CurrentGame->GetPlayersData( ) );

    // add players from RMK if any

    if( m_HoldPlayersForRMK && !m_PlayerNamesfromRMK.empty( ) )
    {
        CONSOLE_Print( "[UNM] reserving players from last game" );

        for( uint32_t i = 0; i < m_PlayerNamesfromRMK.size( ); i++ )
            m_CurrentGame->AddToReserved( m_PlayerNamesfromRMK[i], m_PlayerServersfromRMK[i], 255 );

        m_PlayerNamesfromRMK.clear( );
        m_PlayerServersfromRMK.clear( );
    }


    // don't advertise the game if it's autohosted locally

    for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
    {
        if( !( *i )->m_SpamSubBot )
        {
            if( whisper && ( *i )->GetServer( ) == creatorServer )
            {
                if( gameState == GAME_PRIVATE )
                {
                    if( ownerName.find_first_not_of( " " ) == string::npos )
                        ( *i )->QueueChatCommand( gLanguage->AutoCreatingPrivateGame( gameName ), creatorName, whisper );
                    else
                        ( *i )->QueueChatCommand( gLanguage->CreatingPrivateGame( gameName, ownerName ), creatorName, whisper );
                }
                else if( gameState == GAME_PUBLIC )
                {
                    if( ownerName.find_first_not_of( " " ) == string::npos )
                        ( *i )->QueueChatCommand( gLanguage->AutoCreatingPublicGame( gameName ), creatorName, whisper );
                    else
                    {
                        string mapcode = string( );
                        bool random = false;

                        if( !hostmapcode.empty( ) )
                        {
                            mapcode = hostmapcode;

                            if( mapcode.size( ) > 7 && mapcode.substr( mapcode.size( ) - 7 ) == " random" )
                            {
                                random = true;
                                mapcode = mapcode.substr( 0, mapcode.size( ) - 7 );
                            }

                            if( random )
                                mapcode = "Рандомная карта: [" + mapcode + "]. ";
                            else
                                mapcode = "Выбрана карта: [" + mapcode + "]. ";
                        }

                        ( *i )->QueueChatCommand( mapcode + gLanguage->CreatingPublicGame( gameName, ownerName ), creatorName, whisper );
                    }
                }
            }
            else
            {
                // note that we send this chat message on all other bnet servers

                if( gameState == GAME_PRIVATE )
                {
                    if( ownerName.find_first_not_of( " " ) == string::npos )
                        ( *i )->QueueChatCommand( gLanguage->AutoCreatingPrivateGame( gameName ) );
                    else
                        ( *i )->QueueChatCommand( gLanguage->CreatingPrivateGame( gameName, ownerName ) );
                }
                else if( gameState == GAME_PUBLIC )
                {
                    if( ownerName.find_first_not_of( " " ) == string::npos )
                        ( *i )->QueueChatCommand( gLanguage->AutoCreatingPublicGame( gameName ) );
                    else
                    {
                        string mapcode2 = string( );
                        bool random = false;

                        if( !hostmapcode.empty( ) )
                        {
                            mapcode2 = hostmapcode;

                            if( mapcode2.size( ) > 7 && mapcode2.substr( mapcode2.size( ) - 7 ) == " random" )
                            {
                                random = true;
                                mapcode2 = mapcode2.substr( 0, mapcode2.size( ) - 7 );
                            }

                            if( random )
                                mapcode2 = "Рандомная карта: [" + mapcode2 + "]. ";
                            else
                                mapcode2 = "Выбрана карта: [" + mapcode2 + "]. ";
                        }

                        ( *i )->QueueChatCommand( mapcode2 + gLanguage->CreatingPublicGame( gameName, ownerName ) );
                    }
                }
            }
        }
    }

    // hold friends and/or clan members

    for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
    {
        if( ( *i )->GetServerType( ) == 0 )
        {
            if( ( *i )->GetHoldFriends( ) )
                ( *i )->HoldFriends( m_CurrentGame );

            if( ( *i )->GetHoldClan( ) )
                ( *i )->HoldClan( m_CurrentGame );

            if( ( *i )->GetLoggedIn( ) )
                ( *i )->m_QuietRehost = false;
            else
                ( *i )->m_QuietRehost = true;
        }
    }
}

void CUNM::ReloadConfig( CConfig *CFG )
{
    bool first = true;
    CConfig CFGF;

    if( CFG == nullptr )
    {
        first = false;
        CFGF.Read( "unm.cfg", 1 );
        CFG = &CFGF;
    }

    m_ShowPesettingWindow = CFG->GetUInt( "gui_showpesettingwindow", 4, 5 );
    m_SpoofChecks = CFG->GetUInt( "bot_spoofchecks", 2, 2 );
    m_CountDownMethod = CFG->GetUInt( "bot_countdownmethod", 1, 1 );
    m_AllowDownloads = CFG->GetUInt( "bot_allowdownloads", 1, 3 );
    m_MaxLatency = CFG->GetUInt( "bot_maxlatency", 100, 0, 10000 );
    m_MinLatency = CFG->GetUInt( "bot_minlatency", 0, 0, m_MaxLatency );
    m_Latency = CFG->GetUInt( "bot_latency", 35, m_MinLatency, m_MaxLatency );
    m_DynamicLatencyHighestAllowed = CFG->GetUInt( "bot_dynamiclatencyhighestallowed", 50, 0, 10000 );
    m_DynamicLatencyLowestAllowed = CFG->GetUInt( "bot_dynamiclatencylowestallowed", 15, 0, m_DynamicLatencyHighestAllowed );
    m_MaxSyncLimit = CFG->GetUInt( "bot_maxsynclimit", 1000000000, 10, 1000000000 );
    m_MinSyncLimit = CFG->GetUInt( "bot_minsynclimit", 10, 10, m_MaxSyncLimit );
    m_SyncLimit = CFG->GetUInt( "bot_synclimit", 600, m_MinSyncLimit, m_MaxSyncLimit );
    m_VoteStartMinPlayers = CFG->GetUInt( "bot_votestartminplayers", 2, 1, 12 );
    m_VoteStartPercent = CFG->GetUInt( "bot_votestartpercent", 80, 1, 100 );
    m_VoteKickPercentage = CFG->GetUInt( "bot_votekickpercentage", 66, 1, 100 );
    m_OptionOwner = CFG->GetUInt( "bot_optionowner", 3, 4 );
    m_AFKkick = CFG->GetUInt( "bot_afkkick", 1, 2 );
    m_MsgAutoCorrectMode = CFG->GetUInt( "bot_msgautocorrectmode", 2, 3 );
    m_manualhclmessage = CFG->GetUInt( "bot_manualhclmessage", 0, 2 );
    m_GameOwnerPrivatePrintJoin = CFG->GetUInt( "bot_gameownerprivateprintjoin", 1, 2 );
    m_AutoUnMutePrinting = CFG->GetUInt( "bot_autounmuteprinting", 2, 3 );
    m_ManualUnMutePrinting = CFG->GetUInt( "bot_manualunmuteprinting", 1, 3 );
    m_GameIsOverMessagePrinting = CFG->GetUInt( "bot_gameisovermessageprinting", 1, 3 );
    m_MoreFPsLobby = CFG->GetUInt( "bot_morefakeplayersinlobby", 3, 3 );
    m_GameNameCharHide = CFG->GetUInt( "bot_gamenamecharhide", 3, 3 );
    gEnableLoggingToGUI = CFG->GetUInt( "bot_enableloggingtogui", 1, 2 );
    m_PlaceAdminsHigher = CFG->GetUInt( "bot_placeadminshigher", 0, 3 );
    m_maxdownloaders = CFG->GetUInt( "bot_maxdownloaders", 12, 0, 12 );
    m_BroadcastHostName = CFG->GetUInt( "bot_broadcasthostname", 0, 3 );
    m_GameAnnounce = CFG->GetUInt( "bot_gameannounce", 0, 3 );
    m_CensorDictionary = CFG->GetUInt( "bot_censordictionary", 3, 3 );
    m_HostPort = CFG->GetUInt16( "bot_hostport", 6113 );
    m_BroadcastPort = CFG->GetUInt16( "bot_broadcastport", 6112 );
    m_MaxGames = CFG->GetUInt( "bot_maxgames", 15 );
    m_NumRequiredPingsPlayers = CFG->GetUInt( "bot_numrequiredpingsplayers", 1 );
    m_NumRequiredPingsPlayersAS = CFG->GetUInt( "bot_numrequiredpingsplayersas", 3 );
    m_CountDownStartCounter = CFG->GetUInt( "bot_countdownstartcounter", 5 );
    m_CountDownStartForceCounter = CFG->GetUInt( "bot_countdownstartforcecounter", 5 );
    m_AutoKickPing = CFG->GetUInt( "bot_autokickping", 300 );
    m_ReconnectWaitTime = CFG->GetUInt( "bot_reconnectwaittime", 2 );
    m_DynamicLatencyRefreshRate = CFG->GetUInt( "bot_dynamiclatencyrefreshrate", 5 );
    m_DynamicLatencyConsolePrint = CFG->GetUInt( "bot_dynamiclatencyconsoleprint", 600 );
    m_DynamicLatencyPercentPingMax = CFG->GetUInt( "bot_dynamiclatencypercentpm", 60 );
    m_DynamicLatencyDifferencePingMax = CFG->GetUInt( "bot_dynamiclatencydiffpingmax", 0 );
    m_DynamicLatencyMaxSync = CFG->GetUInt( "bot_dynamiclatencymaxsync", 0 );
    m_DynamicLatencyAddIfMaxSync = CFG->GetUInt( "bot_dynamiclatencyaddifmaxsync", 0 );
    m_DynamicLatencyAddIfLag = CFG->GetUInt( "bot_dynamiclatencyaddiflag", 0 );
    m_DynamicLatencyAddIfBotLag = CFG->GetUInt( "bot_dynamiclatencyaddifbotlag", 0 );
    m_VoteStartDuration = CFG->GetUInt( "bot_votestartduration", 0 );
    m_AFKtimer = CFG->GetUInt( "bot_afktimer", 300 );
    m_gamestateinhouse = CFG->GetUInt( "bot_gamestateinhouse", 999 );
    m_AntiFloodMuteSeconds = CFG->GetUInt( "bot_antifloodmuteseconds", 15 );
    m_AntiFloodRigidityTime = CFG->GetUInt( "bot_antifloodrigiditytime", 5 );
    m_AntiFloodRigidityNumber = CFG->GetUInt( "bot_antifloodrigiditynumber", 7 );
    m_NewCensorCookiesNumber = CFG->GetUInt( "bot_newcensorcookiesnumber", 5 );
    m_CensorMuteFirstSeconds = CFG->GetUInt( "bot_censormutefirstseconds", 5 );
    m_CensorMuteSecondSeconds = CFG->GetUInt( "bot_censormutesecondseconds", 10 );
    m_CensorMuteExcessiveSeconds = CFG->GetUInt( "bot_censormuteexcessiveseconds", 15 );
    m_MaxHostCounter = CFG->GetUInt( "bot_maxhostcounter", 999 );

    if( m_MaxHostCounter > 16777215 )
        m_MaxHostCounter = 16777215;

    m_StillDownloadingASPrintDelay = CFG->GetUInt( "bot_stilldownloadingasprintdelay", 5 );
    m_NotSpoofCheckedASPrintDelay = CFG->GetUInt( "bot_notspoofcheckedasprintdelay", 3 );
    m_NotPingedASPrintDelay = CFG->GetUInt( "bot_notpingedasprintdelay", 3 );
    m_TryAutoStartDelay = CFG->GetUInt( "bot_tryautostartdelay", 3 );
    m_PlayerBeforeStartPrintDelay = CFG->GetUInt( "bot_playerbeforestartprintdelay", 60 );
    m_RehostPrintingDelay = CFG->GetUInt( "bot_rehostprintingdelay", 12 );
    m_ButGameDelay = CFG->GetUInt( "bot_butgamedelay", 3 );
    m_MarsGameDelay = CFG->GetUInt( "bot_marsgamedelay", 3 );
    m_SlapGameDelay = CFG->GetUInt( "bot_slapgamedelay", 3 );
    m_RollGameDelay = CFG->GetUInt( "bot_rollgamedelay", 0 );
    m_PictureGameDelay = CFG->GetUInt( "bot_picturegamedelay", 3 );
    m_MaximumNumberCookies = CFG->GetUInt( "bot_maximumnumbercookies", 5 );
    m_EatGameDelay = CFG->GetUInt( "bot_eatgamedelay", 3 );
    m_PubGameDelay = CFG->GetUInt( "bot_pubgamedelay", 1 );
    m_MinLengthGN = CFG->GetUInt( "bot_minlengthgn", 1 );
    m_SlotInfoInGameName = CFG->GetUInt( "bot_slotinfoingamename", 1 );
    m_LobbyTimeLimit = CFG->GetUInt( "bot_lobbytimelimit", 180 );
    m_LobbyTimeLimitMax = CFG->GetUInt( "bot_lobbytimelimitmax", 0 );
    m_LANWar3Version = static_cast<unsigned char>( CFG->GetUInt( "lan_war3version", 26 ) );
    m_ReplayBuildNumber = CFG->GetUInt16( "replay_buildnumber", 6059 );
    m_ReplayWar3Version = CFG->GetUInt( "replay_war3version", 26 );
    m_DenyMinDownloadRate = CFG->GetUInt( "bot_mindownloadrate", 0 );
    m_DenyMaxDownloadTime = CFG->GetUInt( "bot_maxdownloadtime", 0 );
    m_DenyMaxMapsizeTime = CFG->GetUInt( "bot_maxmapsizetime", 0 );
    m_DenyMaxReqjoinTime = CFG->GetUInt( "bot_maxreqjointime", 10 );
    m_DenyMaxIPUsage = CFG->GetUInt( "bot_maxipusage", 0 );
    m_DenyMaxLoadTime = CFG->GetUInt( "bot_maxloadtime", 0 );
    m_GameLoadedPrintout = CFG->GetUInt( "bot_gameloadedprintout", 10800 );
    m_gameoverminpercent = CFG->GetUInt( "bot_gameoverminpercent", 0 );
    m_gameoverminplayers = CFG->GetUInt( "bot_gameoverminplayers", 0 );
    m_gameovermaxteamdifference = CFG->GetUInt( "bot_gameovermaxteamdifference", 0 );
    m_totaldownloadspeed = CFG->GetUInt( "bot_totaldownloadspeed", 24576 );
    m_clientdownloadspeed = CFG->GetUInt( "bot_clientdownloadspeed", 2048 );
    m_ShowDownloadsInfoTime = CFG->GetUInt( "bot_showdownloadsinfotime", 15 );
    m_DropVotePercent = CFG->GetUInt( "bot_dropvotepercent", 66 );
    m_DropVoteTime = CFG->GetUInt( "bot_dropvotetime", 5 );
    m_DropWaitTime = CFG->GetUInt( "bot_dropwaittime", 60 );
    m_DropWaitTimeSum = CFG->GetUInt( "bot_dropwaittimesum", 120 );
    m_DropWaitTimeGame = CFG->GetUInt( "bot_dropwaittimegame", 90 );
    m_DropWaitTimeLoad = CFG->GetUInt( "bot_dropwaittimeload", 30 );
    m_DropWaitTimeLobby = CFG->GetUInt( "bot_dropwaittimelobby", 15 );
    m_AllowLaggingTime = CFG->GetUInt( "bot_allowlaggingtime", 1 );
    m_AllowLaggingTimeInterval = CFG->GetUInt( "bot_allowlaggingtimeinterval", 1 );

	if( m_AllowLaggingTimeInterval > 71582788 )
		m_AllowLaggingTimeInterval = 71582788;

    m_AllowLaggingTimeInterval = m_AllowLaggingTimeInterval * 60;
    m_NoMapPrintDelay = CFG->GetUInt( "bot_nomapprintdelay", 120 );
    m_MaxLogSize = CFG->GetInt64( "bot_maxlogsize", 5000000 );
    m_MaxLogTime = CFG->GetUInt( "bot_maxlogtime", 0 );
    m_GameAnnounceInterval = CFG->GetUInt( "bot_gameannounceinterval", 60 );
    m_SaveReplays = CFG->GetInt( "bot_savereplays", 0 ) != 0;
    m_AllowAutoRenameMaps = CFG->GetInt( "bot_allowautorenamemaps", 1 ) != 0;
    m_OverrideMapDataFromCFG = CFG->GetInt( "bot_overridemapdatafromcfg", 1 ) != 0;
    m_OverrideMapPathFromCFG = CFG->GetInt( "bot_overridemappathfromcfg", 1 ) != 0;
    m_OverrideMapSizeFromCFG = CFG->GetInt( "bot_overridemapsizefromcfg", 1 ) != 0;
    m_OverrideMapInfoFromCFG = CFG->GetInt( "bot_overridemapinfofromcfg", 1 ) != 0;
    m_OverrideMapCrcFromCFG = CFG->GetInt( "bot_overridemapcrcfromcfg", 1 ) != 0;
    m_OverrideMapSha1FromCFG = CFG->GetInt( "bot_overridemapsha1fromcfg", 1 ) != 0;
    m_OverrideMapSpeedFromCFG = CFG->GetInt( "bot_overridemapspeedfromcfg", 0 ) != 0;
    m_OverrideMapVisibilityFromCFG = CFG->GetInt( "bot_overridemapvisibilityfromcfg", 0 ) != 0;
    m_OverrideMapFlagsFromCFG = CFG->GetInt( "bot_overridemapflagsfromcfg", 0 ) != 0;
    m_OverrideMapFilterTypeFromCFG = CFG->GetInt( "bot_overridemapfiltertypefromcfg", 0 ) != 0;
    m_OverrideMapGameTypeFromCFG = CFG->GetInt( "bot_overridemapgametypefromcfg", 1 ) != 0;
    m_OverrideMapWidthFromCFG = CFG->GetInt( "bot_overridemapwidthfromcfg", 1 ) != 0;
    m_OverrideMapHeightFromCFG = CFG->GetInt( "bot_overridemapheightfromcfg", 1 ) != 0;
    m_OverrideMapTypeFromCFG = CFG->GetInt( "bot_overridemaptypefromcfg", 1 ) != 0;
    m_OverrideMapDefaultHCLFromCFG = CFG->GetInt( "bot_overridemapdefaulthclfromcfg", 0 ) != 0;
    m_OverrideMapLoadInGameFromCFG = CFG->GetInt( "bot_overridemaploadingamefromcfg", 1 ) != 0;
    m_OverrideMapNumPlayersFromCFG = CFG->GetInt( "bot_overridemapnumplayersfromcfg", 1 ) != 0;
    m_OverrideMapNumTeamsFromCFG = CFG->GetInt( "bot_overridemapnumteamsfromcfg", 1 ) != 0;
    m_OverrideMapSlotsFromCFG = CFG->GetInt( "bot_overridemapslotsfromcfg", 1 ) != 0;
    m_OverrideMapObserversFromCFG = CFG->GetInt( "bot_overridemapobserversfromcfg", 1 ) != 0;
    m_PlayersCanChangeRace = CFG->GetInt( "bot_playerscanchangerace", 1 ) != 0;
    m_HideIPAddresses = CFG->GetInt( "bot_hideipaddresses", 0 ) != 0;
    m_CheckMultipleIPUsage = CFG->GetInt( "bot_checkmultipleipusage", 1 ) != 0;
    m_RefreshMessages = CFG->GetInt( "bot_refreshmessages", 0 ) != 0;
    m_AutoSave = CFG->GetInt( "bot_autosave", 0 ) != 0;
    m_PingDuringDownloads = CFG->GetInt( "bot_pingduringdownloads", 1 ) != 0;
    m_LCPings = CFG->GetInt( "bot_lcpings", 1 ) != 0;
    m_NormalCountdown = CFG->GetInt( "bot_normalcountdown", 0 ) != 0;
    m_DetourAllMessagesToAdmins = CFG->GetInt( "bot_detourallmessagestoadmins", 0 ) != 0;
    m_WarnLatency = CFG->GetInt( "bot_warnLatency", 0 ) != 0;
    m_UseDynamicLatency = CFG->GetInt( "bot_usedynamiclatency", 0 ) != 0;
    m_VoteStartAllowed = CFG->GetInt( "bot_votestartallowed", 1 ) != 0;
    m_VoteStartPercentalVoting = CFG->GetInt( "bot_votestartpercentalvoting", 1 ) != 0;
    m_CancelVoteStartIfLeave = CFG->GetInt( "bot_cancelvotestartifleave", 0 ) != 0;
    m_StartAfterLeaveIfEnoughVotes = CFG->GetInt( "bot_startafterleaveifenoughvotes", 0 ) != 0;
    m_VoteKickAllowed = CFG->GetInt( "bot_votekickallowed", 1 ) != 0;
    m_ForceLoadInGame = CFG->GetInt( "bot_forceloadingame", 1 ) != 0;
    m_ShowRealSlotCount = CFG->GetInt( "bot_showrealslotcountonlan", 1 ) != 0;
    m_LobbyAnnounceUnoccupied = CFG->GetInt( "bot_lobbyannounceunoccupied", 1 ) != 0;
    m_AntiFloodMute = CFG->GetInt( "bot_antifloodmute", 0 ) != 0;
    m_AdminCanUnAutoMute = CFG->GetInt( "bot_admincanunautomute", 1 ) != 0;
    m_AdminCanUnCensorMute = CFG->GetInt( "bot_admincanuncensormute", 1 ) != 0;
    m_NewCensorCookies = CFG->GetInt( "bot_newcensorcookies", 1 ) != 0;
    m_CensorMute = CFG->GetInt( "bot_censormute", 0 ) != 0;
    m_CensorMuteAdmins = CFG->GetInt( "bot_censormuteadmins", 1 ) != 0;
    m_Censor = CFG->GetInt( "bot_censor", 0 ) != 0;
    m_broadcastinlan = CFG->GetInt( "bot_broadcastlan", 1 ) != 0;
    m_autohclfromgamename = CFG->GetInt( "bot_autohclfromgamename", 0 ) != 0;
    m_forceautohclindota = CFG->GetInt( "bot_forceautohclindota", 0 ) != 0;
    m_ResourceTradePrint = CFG->GetInt( "bot_resourcetradeprint", 0 ) != 0;
    m_StillDownloadingPrint = CFG->GetInt( "bot_stilldownloadingprint", 1 ) != 0;
    m_NotSpoofCheckedPrint = CFG->GetInt( "bot_notspoofcheckedprint", 1 ) != 0;
    m_NotPingedPrint = CFG->GetInt( "bot_notpingedprint", 1 ) != 0;
    m_StillDownloadingASPrint = CFG->GetInt( "bot_stilldownloadingasprint", 1 ) != 0;
    m_NotSpoofCheckedASPrint = CFG->GetInt( "bot_notspoofcheckedasprint", 1 ) != 0;
    m_NotPingedASPrint = CFG->GetInt( "bot_notpingedasprint", 1 ) != 0;
    m_PlayerBeforeStartPrint = CFG->GetInt( "bot_playerbeforestartprint", 1 ) != 0;
    m_PlayerBeforeStartPrintJoin = CFG->GetInt( "bot_playerbeforestartprintjoin", 0 ) != 0;
    m_PlayerBeforeStartPrivatePrintJoin = CFG->GetInt( "bot_playerbeforestartprivateprintjoin", 1 ) != 0;
    m_PlayerBeforeStartPrivatePrintJoinSD = CFG->GetInt( "bot_playerbeforestartprivateprintjoinsd", 1 ) != 0;
    m_PlayerBeforeStartPrintLeave = CFG->GetInt( "bot_playerbeforestartprintleave", 0 ) != 0;
    m_BotChannelPrivatePrintJoin = CFG->GetInt( "bot_botchannelprivateprintjoin", 0 ) != 0;
    m_BotNamePrivatePrintJoin = CFG->GetInt( "bot_botnameprivateprintjoin", 0 ) != 0;
    m_InfoGamesPrivatePrintJoin = CFG->GetInt( "bot_infogamesprivateprintjoin", 0 ) != 0;
    m_GameNamePrivatePrintJoin = CFG->GetInt( "bot_gamenameprivateprintjoin", 0 ) != 0;
    m_GameModePrivatePrintJoin = CFG->GetInt( "bot_gamemodeprivateprintjoin", 1 ) != 0;
    m_RehostPrinting = CFG->GetInt( "bot_rehostprinting", 0 ) != 0;
    m_RehostFailedPrinting = CFG->GetInt( "bot_rehostfailedprinting", 0 ) != 0;
    m_PlayerFinishedLoadingPrinting = CFG->GetInt( "bot_playerfinishedloadingprinting", 0 ) != 0;
    m_GameStartedMessagePrinting = CFG->GetInt( "bot_gamestartedmessageprinting", 1 ) != 0;
    m_WarningPacketValidationFailed = CFG->GetInt( "bot_warningpacketvalidationfailed", 0 ) != 0;
    m_FunCommandsGame = CFG->GetInt( "bot_funcommandsgame", 1 ) != 0;
    m_EatGame = CFG->GetInt( "bot_eatgame", 1 ) != 0;
    m_FakePlayersLobby = CFG->GetInt( "bot_fakeplayersinlobby", 0 ) != 0;
    m_AppleIcon = CFG->GetInt( "bot_appleicon", 1 ) != 0;
    m_ResetGameNameChar = CFG->GetInt( "bot_resetgamenamechar", 1 ) != 0;
    m_FixGameNameForIccup = CFG->GetInt( "bot_fixgamenameforiccup", 1 ) != 0;
    m_PrefixName = CFG->GetInt( "bot_realmprefixname", 0 ) != 0;
    m_PlayerServerPrintJoin = CFG->GetInt( "bot_playerserverprintjoin", 0 ) != 0;
    m_ReplaysByName = CFG->GetInt( "bot_replayssavedbyname", 1 ) != 0;
    m_LogReduction = CFG->GetInt( "bot_logreduction", 0 ) != 0;
    m_RejectColoredName = CFG->GetInt( "bot_rejectcolorname", 0 ) != 0;
    m_AdminMessages = CFG->GetInt( "bot_adminmessages", 0 ) != 0;
    m_LocalAdminMessages = CFG->GetInt( "bot_localadminmessages", 0 ) != 0;
    m_RelayChatCommands = CFG->GetInt( "bot_relaychatcommands", 1 ) != 0;
    m_NonAdminCommandsGame = CFG->GetInt( "bot_nonadmincommandsgame", 1 ) != 0;
    m_SpoofCheckIfGameNameIsIndefinite = CFG->GetInt( "bot_spoofcheckifgnisindefinite", 0 ) != 0;
    m_autoinsultlobby = CFG->GetInt( "bot_autoinsultlobby", 0 ) != 0;
    m_autoinsultlobbysafeadmins = CFG->GetInt( "bot_autoinsultlobbysafeadmins", 0 ) != 0;
    m_ShowDownloadsInfo = CFG->GetInt( "bot_showdownloadsinfo", 0 ) != 0;
    m_ShowFinishDownloadingInfo = CFG->GetInt( "bot_showfinishdownloadinginfo", 0 ) != 0;
    m_ShuffleSlotsOnStart = CFG->GetInt( "bot_shuffleslotsonstart", 0 ) != 0;
    m_patch23 = CFG->GetInt( "bot_patch23ornewer", 1 ) != 0;
    m_patch21 = CFG->GetInt( "bot_patch21", 0 ) != 0;
    m_HoldPlayersForRMK = CFG->GetInt( "bot_holdplayersforrmk", 1 ) != 0;
    m_DropIfDesync = CFG->GetInt( "bot_dropifdesync", 0 ) != 0;
    m_DontDeleteVirtualHost = CFG->GetInt( "bot_dontdeletevirtualhost", 0 ) != 0;
    m_DontDeletePenultimatePlayer = CFG->GetInt( "bot_dontdeletepenultimateplayer", 0 ) != 0;
    m_UserCanDropAdmin = CFG->GetInt( "bot_usercandropadmin", 0 ) != 0;

    if( m_CensorDictionary == 0 )
        m_CensorWords = "***";
    else if( m_CensorDictionary == 2 )
        m_CensorWords = CFG->GetString( "bot_censorwords", "***" );
    else
    {
        string s;
        s = "3ae6 3aeb 3aluna 3alunu 3aluny 3alupa 3alupu 3alupy 3alyna 3alynu 3alyny 3alypa 3alypu 3alypy 6lia 6lya assfuck asshole bitch blia blya bue6 bueb bye6 byeb csu4ka csu4ki csu4ku csuchka csuchki csuchku csuka csuki csuku csy4ka csy4ki csy4ku csychka csychki csychku csyka csyki csyku cu4ap cu4ar cu4ka cu4ki cu4ku cuchap cuchar cuchka cuchki cuchku cuka cuki cuku cunt cy4ap cy4ar cy4ka cy4ki cy4ku cychap cychar cychka cychki cychku cyka cyki cyku dae6 daee6 dal6ae dal6oe dalbae dalboe dick dickhead doeb doeeb dol6ae dol6oe dolbae dolboe e6aca e6ah e6al e6an e6ap e6ar e6ash e6at e6aw e6eh' e6en' e6esh e6et e6ew e6hi e6hu e6hy e6i e6l0 e6lan e6li e6lo e6lu e6ni e6nu e6ny e6u e6y ebaca ebah ebal eban ebap ebar ebash ebat ebaw ebeh' eben' ebesh ebet ebew ebhi ebhu ebhy ebi ebl0 eblan ebli eblo eblu ebni ebnu ebny ebu eby faggot fuck fucking g0hd0h g0hd0n g0hdoh g0hdon g0nd0h g0nd0n g0ndoh g0ndon gahd0h gahd0n gahdoh gahdon gand0h gand0n gandoh gandon gohd0h gohd0n gohdoh gohdon gond0h gond0n gondoh gondon hax hex hu9 hue hui huia huy huya hy9 hye hyi hyia hyu i6aca i6ah i6al i6an i6ash i6at i6aw i6eh i6en i6esh i6et i6ew i6i i6l0 i6lan i6li i6lo i6lu i6u i6y ibaca ibah ibal iban ibash ibat ibaw ibeh iben ibesh ibet ibew ibi ibl0 iblan ibli iblo iblu ibu iby mahdabo mahdavo mandabo mandavo mokroschelka mokroshelka mokrowelka moron mudae6 mudaeb mudak mudeh muden mudila mudl0 mudlo mudoe6 mudoeb mudula mydae6 mydaeb mydak mydeh myden mydila mydl0 mydlo mydoe6 mydoeb mydula nah nax ne3da ne3de ne3di ne3du ne3dy ne3ta ne3te ne3ti ne3tu ne3ty nedek nedik neduk neh neree6 nereeb nesda nesde nesdi nesdu nesdy nesta neste nesti nestu nesty nex nezda nezde nezdi nezdu nezdy nezta nezte nezti neztu nezty ni3da ni3de ni3di ni3du ni3dy ni3je ni3ta ni3te ni3ti ni3tu ni3ty nidap nidar nidop nidor nidp nidr nigger nisda nisde nisdi nisdu nisdy nista niste nisti nistu nisty nizda nizde nizdi nizdu nizdy nizje nizta nizte nizti niztu nizty nu3da nu3de nu3di nu3du nu3dy nu3je nu3ta nu3te nu3ti nu3tu nu3ty nudap nudar nudop nudor nudp nudr nusda nusde nusdi nusdu nusdy nusta nuste nusti nustu nusty nuzda nuzde nuzdi nuzdu nuzdy nuzje nuzta nuzte nuzti nuztu nuzty ot'eb pe3da pe3de pe3di pe3du pe3dy pe3ta pe3te pe3ti pe3tu pe3ty pedek pedik peduk peree6 pereeb pesda pesde pesdi pesdu pesdy pesta peste pesti pestu pesty pezda pezde pezdi pezdu pezdy pezta pezte pezti peztu pezty pi3da pi3de pi3di pi3du pi3dy pi3je pi3ta pi3te pi3ti pi3tu pi3ty pidap pidar pidop pidor pidp pidr pisda pisde pisdi pisdu pisdy pista piste pisti pistu pisty pizda pizde pizdi pizdu pizdy pizje pizta pizte pizti piztu pizty poh pox pu3da pu3de pu3di pu3du pu3dy pu3je pu3ta pu3te pu3ti pu3tu pu3ty pudap pudar pudop pudor pudp pudr pusda pusde pusdi pusdu pusdy pussy pusta puste pusti pustu pusty puzda puzde puzdi puzdu puzdy puzje puzta puzte puzti puztu puzty r0hd0h r0hd0n r0hdoh r0hdon r0nd0h r0nd0n r0ndoh r0ndon rahd0h rahd0n rahdoh rahdon rand0h rand0n randoh randon rohd0h rohd0n rohdoh rohdon rond0h rond0n rondoh rondon shit shithead shitty shluh shlux shlyh shlyx su4ap su4ar su4ka su4ki su4ku suchap suchar suchka suchki suchku suka suki suku sy4ap sy4ar sy4ka sy4ki sy4ku sychap sychar sychka sychki sychku syka syki syku u6aca u6ah u6al u6an u6ash u6at u6aw u6eh u6en u6esh u6et u6ew u6i u6l0 u6lan u6li u6lo u6lu u6u u6y ubaca ubah ubal uban ubash ubat ubaw ubeh uben ubesh ubet ubew ubi ubl0 ublan ubli ublo ublu ubu uby ue6ak ue6ok uebak uebok vue6 vueb vye6 vyeb whore wluh wlux wlyh wlyx xu9 xue xui xuia xuy xuya xy9 xye xyi xyia xyu ye6ak ye6ok yebak yebok zae6 zaeb zaluna zalunu zaluny zalupa zalupu zalupy zalyna zalynu zalyny zalypa zalypu zalypy";
        s += " xye xyE xyё xyя xyю xyй xyи xyе xyЯ xyЮ xyЙ xyИ xyЕ xyЁ xYe xYE xYё xYя xYю xYй xYи xYе xYЯ xYЮ xYЙ xYИ xYЕ xYЁ xуe xуE xуё xуя xую xуй xуи xуе xуЯ xуЮ xуЙ xуИ xуЕ xуЁ xУe xУE xУё xУя xУю xУй xУи xУе xУЯ xУЮ xУЙ xУИ xУЕ xУЁ eби eбИ eБи eБИ cyky cyko cyke cykY cykO cykE cykу cykо cykи cykе cykа cykУ cykО cykИ cykЕ cykА cyKy cyKo cyKe cyKY cyKO cyKE cyKу cyKо cyKи cyKе cyKа cyKУ cyKО cyKИ cyKЕ cyKА cyкy cyкo cyкe cyкa cyкY cyкO cyкE cyкA cyку cyко cyки cyке cyка cyкУ cyкО cyкИ cyкЕ cyкА cyКy cyКo cyКe cyКa cyКY cyКO cyКE cyКA cyКу cyКо cyКи cyКе cyКа cyКУ cyКО cyКИ cyКЕ cyКА cYky cYko cYke cYkY cYkO cYkE cYkу cYkо cYkи cYkе cYkа cYkУ cYkО cYkИ cYkЕ cYkА cYKy cYKo cYKe cYKY cYKO cYKE cYKу cYKо cYKи cYKе cYKа cYKУ cYKО cYKИ cYKЕ cYKА cYкy cYкo cYкe cYкa cYкY cYкO cYкE cYкA cYку cYко cYки cYке cYка cYкУ cYкО cYкИ cYкЕ cYкА cYКy cYКo cYКe cYКa cYКY cYКO cYКE cYКA cYКу cYКо cYКи cYКе cYКа cYКУ cYКО cYКИ cYКЕ cYКА cуky cуko cуke cуkY cуkO cуkE cуkу cуkо cуkи cуkе cуkа cуkУ cуkО cуkИ cуkЕ cуkА cуKy cуKo cуKe cуKY cуKO cуKE cуKу cуKо cуKи cуKе cуKа cуKУ cуKО cуKИ cуKЕ cуKА cукy cукo cукe cукa cукY cукO cукE cукA cуку cуко cуки cуке cука cукУ cукО cукИ cукЕ cукА cуКy cуКo cуКe cуКa cуКY cуКO cуКE cуКA cуКу cуКо cуКи cуКе cуКа cуКУ cуКО cуКИ cуКЕ cуКА cУky cУko cУke cУkY cУkO cУkE cУkу cУkо cУkи cУkе cУkа cУkУ cУkО cУkИ cУkЕ cУkА cУKy cУKo cУKe cУKY cУKO cУKE cУKу cУKо cУKи cУKе cУKа cУKУ cУKО cУKИ cУKЕ cУKА cУкy cУкo cУкe cУкa cУкY cУкO cУкE cУкA cУку cУко cУки cУке cУка cУкУ cУкО cУкИ cУкЕ cУкА cУКy cУКo cУКe cУКa cУКY cУКO cУКE cУКA cУКу cУКо cУКи cУКе cУКа cУКУ cУКО cУКИ cУКЕ cУКА Xye XyE Xyё Xyя Xyю Xyй Xyи Xyе XyЯ XyЮ XyЙ XyИ XyЕ XyЁ XYe XYE XYё XYя XYю XYй XYи XYе XYЯ XYЮ XYЙ XYИ XYЕ XYЁ Xуe XуE Xуё Xуя Xую Xуй Xуи Xуе XуЯ XуЮ XуЙ XуИ XуЕ XуЁ XУe XУE XУё XУя XУю XУй XУи XУе XУЯ XУЮ XУЙ XУИ XУЕ XУЁ Eби EбИ EБи EБИ Cyky Cyko Cyke CykY CykO CykE Cykу Cykо Cykи Cykе Cykа CykУ CykО CykИ CykЕ CykА CyKy CyKo CyKe CyKY CyKO CyKE CyKу CyKо CyKи CyKе CyKа CyKУ CyKО CyKИ CyKЕ CyKА Cyкy Cyкo Cyкe Cyкa CyкY CyкO CyкE CyкA Cyку Cyко Cyки Cyке Cyка CyкУ CyкО CyкИ CyкЕ CyкА CyКy CyКo CyКe CyКa CyКY CyКO CyКE CyКA CyКу CyКо CyКи CyКе CyКа CyКУ CyКО CyКИ CyКЕ CyКА CYky CYko CYke CYkY CYkO CYkE CYkу CYkо CYkи CYkе CYkа CYkУ CYkО CYkИ CYkЕ CYkА CYKy CYKo CYKe CYKY CYKO CYKE CYKу CYKо CYKи CYKе CYKа CYKУ CYKО CYKИ CYKЕ CYKА CYкy CYкo CYкe CYкa CYкY CYкO CYкE CYкA CYку CYко CYки CYке CYка CYкУ CYкО CYкИ CYкЕ CYкА CYКy CYКo CYКe CYКa CYКY CYКO CYКE CYКA CYКу CYКо CYКи CYКе CYКа CYКУ CYКО CYКИ CYКЕ CYКА Cуky Cуko Cуke CуkY CуkO CуkE Cуkу Cуkо Cуkи Cуkе Cуkа CуkУ CуkО CуkИ CуkЕ CуkА CуKy CуKo CуKe CуKY CуKO CуKE CуKу CуKо CуKи CуKе CуKа CуKУ CуKО CуKИ CуKЕ CуKА Cукy Cукo Cукe Cукa CукY CукO CукE CукA Cуку Cуко Cуки Cуке Cука CукУ CукО CукИ CукЕ CукА CуКy CуКo CуКe CуКa CуКY CуКO CуКE CуКA CуКу CуКо CуКи CуКе CуКа CуКУ CуКО CуКИ CуКЕ CуКА CУky CУko CУke CУkY CУkO CУkE CУkу CУkо CУkи CУkе CУkа CУkУ CУkО CУkИ CУkЕ CУkА CУKy CУKo CУKe CУKY CУKO CУKE CУKу CУKо CУKи CУKе CУKа CУKУ CУKО CУKИ CУKЕ CУKА CУкy CУкo CУкe CУкa CУкY CУкO CУкE CУкA CУку CУко CУки CУке CУка CУкУ CУкО CУкИ CУкЕ CУкА CУКy CУКo CУКe CУКa CУКY CУКO CУКE CУКA CУКу CУКо CУКи CУКе CУКа CУКУ CУКО CУКИ CУКЕ CУКА 6ля 6ляд 6Ля 6Ляд 6ЛЯ 6ЛЯД ёбну чмо цyky цyko цyke цyka цykY цykO цykE цykA цyk цykу цykо цykи цykе цykа цykУ цykО цykИ цykЕ цykА цyKy цyKo цyKe цyKa цyKY цyKO цyKE цyKA цyK цyKу цyKо цyKи цyKе цyKа цyKУ цyKО цyKИ цyKЕ цyKА цyкy цyкo цyкe цyкa цyкY цyкO цyкE цyкA цyк цyку цyко цyки цyке цyка цyкУ цyкО цyкИ цyкЕ цyкА цyКy цyКo цyКe цyКa цyКY цyКO цyКE цyКA цyК цyКу цyКо цyКи цyКе цyКа цyКУ цyКО цyКИ цyКЕ цyКА цYky цYko цYke цYka цYkY цYkO цYkE цYkA цYk цYkу цYkо цYkи цYkе цYkа цYkУ цYkО цYkИ цYkЕ цYkА цYKy цYKo цYKe цYKa цYKY цYKO цYKE цYKA цYK цYKу цYKо цYKи цYKе цYKа цYKУ цYKО цYKИ цYKЕ цYKА цYкy цYкo цYкe цYкa цYкY цYкO цYкE цYкA цYк цYку цYко цYки цYке цYка цYкУ цYкО цYкИ цYкЕ цYкА цYКy цYКo цYКe цYКa цYКY цYКO цYКE цYКA цYК цYКу цYКо цYКи цYКе цYКа цYКУ цYКО цYКИ цYКЕ цYКА цуky цуko цуke цуka цуkY цуkO цуkE цуkA цуk цуkу цуkо цуkи цуkе цуkа цуkУ цуkО цуkИ цуkЕ цуkА цуKy цуKo цуKe цуKa цуKY цуKO цуKE цуKA цуK цуKу цуKо цуKи цуKе цуKа цуKУ цуKО цуKИ цуKЕ цуKА цукy цукo цукe цукa цукY цукO цукE цукA цук цуку цуко цуки цуке цука цукУ цукО цукИ цукЕ цукА цуКy цуКo цуКe цуКa цуКY цуКO цуКE цуКA цуК цуКу цуКо цуКи цуКе цуКа цуКУ цуКО цуКИ цуКЕ цуКА цУky цУko цУke цУka цУkY цУkO цУkE цУkA цУk цУkу цУkо цУkи цУkе цУkа цУkУ цУkО цУkИ цУkЕ цУkА цУKy цУKo цУKe цУKa цУKY цУKO цУKE цУKA цУK цУKу цУKо цУKи цУKе цУKа цУKУ цУKО цУKИ цУKЕ цУKА цУкy цУкo цУкe цУкa цУкY цУкO цУкE цУкA цУк цУку цУко цУки цУке цУка цУкУ цУкО цУкИ цУкЕ цУкА цУКy цУКo цУКe цУКa цУКY цУКO цУКE цУКA цУК цУКу цУКо цУКи цУКе цУКа цУКУ цУКО цУКИ цУКЕ цУКА хye хyE хyё хyя хyю хyй хyи хyе хyЯ хyЮ хyЙ хyИ хyЕ хyЁ хYe хYE хYё хYя хYю хYй хYи хYе хYЯ хYЮ хYЙ хYИ хYЕ хYЁ хуe хуE хуё хуя хую хуля хуй хуйня хуйло хуйла хуи хуище хуило хуе хуЯ хуЮ хуЙ хуИ хуЕ хуЁ хУe хУE хУё хУя хУю хУй хУи хУе хУЯ хУЮ хУЙ хУИ хУЕ хУЁ феееб уебище уебан сyky сyko сyke сyka сykY сykO сykE сykA сykу сykо сykи сykе сykа сykУ сykО сykИ сykЕ сykА сyKy сyKo сyKe сyKa сyKY сyKO сyKE сyKA сyKу сyKо сyKи сyKе сyKа сyKУ сyKО сyKИ сyKЕ сyKА сyкy сyкo сyкe сyкa сyкY сyкO сyкE сyкA сyку сyко сyки сyке сyка сyкУ сyкО сyкИ сyкЕ сyкА сyКy сyКo сyКe сyКa сyКY сyКO сyКE сyКA сyКу сyКо сyКи сyКе сyКа сyКУ сyКО сyКИ сyКЕ сyКА сYky сYko сYke сYka сYkY сYkO сYkE сYkA сYkу сYkо сYkи сYkе сYkа сYkУ сYkО сYkИ сYkЕ сYkА сYKy сYKo сYKe сYKa сYKY сYKO сYKE сYKA сYKу сYKо сYKи сYKе сYKа сYKУ сYKО сYKИ сYKЕ сYKА сYкy сYкo сYкe сYкa сYкY сYкO сYкE сYкA сYку сYко сYки сYке сYка сYкУ сYкО сYкИ сYкЕ сYкА сYКy сYКo сYКe сYКa сYКY сYКO сYКE сYКA сYКу сYКо сYКи сYКе сYКа сYКУ сYКО сYКИ сYКЕ сYКА сцука суky суko суka суkY суkO суkA суkу суkо суkи суkе суkа суkУ суkО суkИ суkЕ суkА суKy суKo суKa суKY суKO суKA суKу суKо суKи суKе суKа суKУ суKО суKИ суKЕ суKА сучара сукy сукo сукe сукa сукY сукO сукE сукA суку суко суки суке сука сукУ сукО сукИ сукЕ сукА суКy суКo суКe суКa суКY суКO суКE суКA суКу суКо суКи суКе суКа суКУ суКО суКИ суКЕ суКА сУky сУko сУka сУkY сУkO сУkA сУkу сУkо сУkи сУkе сУkа сУkУ сУkО сУkИ сУkЕ сУkА сУKy сУKo сУKa сУKY сУKO сУKA сУKу сУKо сУKи сУKе сУKа сУKУ сУKО сУKИ сУKЕ сУKА сУкy сУкo сУкe сУкa сУкY сУкO сУкE сУкA сУку сУко сУки сУке сУка сУкУ сУкО сУкИ сУкЕ сУкА сУКy сУКo сУКe сУКa сУКY сУКO сУКE сУКA сУКу сУКо сУКи сУКе сУКа сУКУ сУКО сУКИ сУКЕ сУКА похую похуй пи3д пи3Д пистапол писдит пизжен пизд пизда пизД пидp пидop пидoP пидoр пидoΠ пидP пидOp пидOP пидOр пидOΠ пидр пидоp пидоP пидор пидорас пидоΠ пидир пидер пидар пидарас пидОp пидОP пидОр пидОΠ пидΠ пиЗд пиЗД пиДp пиДop пиДoP пиДoр пиДoΠ пиДP пиДOp пиДOP пиДOр пиДOΠ пиДр пиДоp пиДоP пиДор пиДоΠ пиДОp пиДОP пиДОр пиДОΠ пиДΠ пестато перееб переебло пИ3д пИ3Д пИзд пИзД пИдp пИдop пИдoP пИдoр пИдoΠ пИдP пИдOp пИдOP пИдOр пИдOΠ пИдр пИдоp пИдоP пИдор пИдоΠ пИдОp пИдОP пИдОр пИдОΠ пИдΠ пИЗд пИЗД пИДp пИДop пИДoP пИДoр пИДoΠ пИДP пИДOp пИДOP пИДOр пИДOΠ пИДр пИДоp пИДоP пИДор пИДоΠ пИДОp пИДОP пИДОр пИДОΠ пИДΠ охуй отьеб отъеб отъебло отебло ниху нихе ниибаца неху нехуй нехе наху мудо мудн мудл муди муде муда мокрощелка ипись иннах изъеб ибанут заёб залупить залупа заеб заебу заебло ебыр ебырь ебущий ебну ебло ебли еблан еби ебеш ебень ебеный ебенный ебат ебать ебанут ебало ебали ебИ еБи еБИ долбо гребан выеб выебу бл9 бля блятc блят бляд блябу блЯ бЛ9 бЛя бЛЯ аебли Шлюха ШЛЮХА ЧМО";
        s += " Цyky Цyko Цyke Цyka ЦykY ЦykO ЦykE ЦykA Цyk Цykу Цykо Цykи Цykе Цykа ЦykУ ЦykО ЦykИ ЦykЕ ЦykА ЦyKy ЦyKo ЦyKe ЦyKa ЦyKY ЦyKO ЦyKE ЦyKA ЦyK ЦyKу ЦyKо ЦyKи ЦyKе ЦyKа ЦyKУ ЦyKО ЦyKИ ЦyKЕ ЦyKА Цyкy Цyкo Цyкe Цyкa ЦyкY ЦyкO ЦyкE ЦyкA Цyк Цyку Цyко Цyки Цyке Цyка ЦyкУ ЦyкО ЦyкИ ЦyкЕ ЦyкА ЦyКy ЦyКo ЦyКe ЦyКa ЦyКY ЦyКO ЦyКE ЦyКA ЦyК ЦyКу ЦyКо ЦyКи ЦyКе ЦyКа ЦyКУ ЦyКО ЦyКИ ЦyКЕ ЦyКА ЦYky ЦYko ЦYke ЦYka ЦYkY ЦYkO ЦYkE ЦYkA ЦYk ЦYkу ЦYkо ЦYkи ЦYkе ЦYkа ЦYkУ ЦYkО ЦYkИ ЦYkЕ ЦYkА ЦYKy ЦYKo ЦYKe ЦYKa ЦYKY ЦYKO ЦYKE ЦYKA ЦYK ЦYKу ЦYKо ЦYKи ЦYKе ЦYKа ЦYKУ ЦYKО ЦYKИ ЦYKЕ ЦYKА ЦYкy ЦYкo ЦYкe ЦYкa ЦYкY ЦYкO ЦYкE ЦYкA ЦYк ЦYку ЦYко ЦYки ЦYке ЦYка ЦYкУ ЦYкО ЦYкИ ЦYкЕ ЦYкА ЦYКy ЦYКo ЦYКe ЦYКa ЦYКY ЦYКO ЦYКE ЦYКA ЦYК ЦYКу ЦYКо ЦYКи ЦYКе ЦYКа ЦYКУ ЦYКО ЦYКИ ЦYКЕ ЦYКА Цуky Цуko Цуke Цуka ЦуkY ЦуkO ЦуkE ЦуkA Цуk Цуkу Цуkо Цуkи Цуkе Цуkа ЦуkУ ЦуkО ЦуkИ ЦуkЕ ЦуkА ЦуKy ЦуKo ЦуKe ЦуKa ЦуKY ЦуKO ЦуKE ЦуKA ЦуK ЦуKу ЦуKо ЦуKи ЦуKе ЦуKа ЦуKУ ЦуKО ЦуKИ ЦуKЕ ЦуKА Цукy Цукo Цукe Цукa ЦукY ЦукO ЦукE ЦукA Цук Цуку Цуко Цуки Цуке Цука ЦукУ ЦукО ЦукИ ЦукЕ ЦукА ЦуКy ЦуКo ЦуКe ЦуКa ЦуКY ЦуКO ЦуКE ЦуКA ЦуК ЦуКу ЦуКо ЦуКи ЦуКе ЦуКа ЦуКУ ЦуКО ЦуКИ ЦуКЕ ЦуКА ЦУky ЦУko ЦУke ЦУka ЦУkY ЦУkO ЦУkE ЦУkA ЦУk ЦУkу ЦУkо ЦУkи ЦУkе ЦУkа ЦУkУ ЦУkО ЦУkИ ЦУkЕ ЦУkА ЦУKy ЦУKo ЦУKe ЦУKa ЦУKY ЦУKO ЦУKE ЦУKA ЦУK ЦУKу ЦУKо ЦУKи ЦУKе ЦУKа ЦУKУ ЦУKО ЦУKИ ЦУKЕ ЦУKА ЦУкy ЦУкo ЦУкe ЦУкa ЦУкY ЦУкO ЦУкE ЦУкA ЦУк ЦУку ЦУко ЦУки ЦУке ЦУка ЦУкУ ЦУкО ЦУкИ ЦУкЕ ЦУкА ЦУКy ЦУКo ЦУКe ЦУКa ЦУКY ЦУКO ЦУКE ЦУКA ЦУК ЦУКу ЦУКо ЦУКи ЦУКе ЦУКа ЦУКУ ЦУКО ЦУКИ ЦУКЕ ЦУКА Хye ХyE Хyё Хyя Хyю Хyй Хyи Хyе ХyЯ ХyЮ ХyЙ ХyИ ХyЕ ХyЁ ХYe ХYE ХYё ХYя ХYю ХYй ХYи ХYе ХYЯ ХYЮ ХYЙ ХYИ ХYЕ ХYЁ Хуe ХуE Хуё Хуя Хую Хуля Хуй Хуйня Хуйло Хуйла Хуи Хуище Хуило Хуе ХуЯ ХуЮ ХуЙ ХуИ ХуЕ ХуЁ ХУe ХУE ХУё ХУя ХУю ХУй ХУи ХУе ХУЯ ХУЮ ХУЮШЕ ХУЛЯ ХУЙ ХУЙНЯ ХУЙЛО ХУЙЛА ХУИ ХУИЩЕ ХУИЛО ХУЕ ХУЁ Феееб ФЕЕЕБ Уебище Уебан УЕБИЩЕ УЕБАН Сyky Сyko Сyke Сyka СykY СykO СykE СykA Сykу Сykо Сykи Сykе Сykа СykУ СykО СykИ СykЕ СykА СyKy СyKo СyKe СyKa СyKY СyKO СyKE СyKA СyKу СyKо СyKи СyKе СyKа СyKУ СyKО СyKИ СyKЕ СyKА Сyкy Сyкo Сyкe Сyкa СyкY СyкO СyкE СyкA Сyку Сyко Сyки Сyке Сyка СyкУ СyкО СyкИ СyкЕ СyкА СyКy СyКo СyКe СyКa СyКY СyКO СyКE СyКA СyКу СyКо СyКи СyКе СyКа СyКУ СyКО СyКИ СyКЕ СyКА СYky СYko СYke СYka СYkY СYkO СYkE СYkA СYkу СYkо СYkи СYkе СYkа СYkУ СYkО СYkИ СYkЕ СYkА СYKy СYKo СYKe СYKa СYKY СYKO СYKE СYKA СYKу СYKо СYKи СYKе СYKа СYKУ СYKО СYKИ СYKЕ СYKА СYкy СYкo СYкe СYкa СYкY СYкO СYкE СYкA СYку СYко СYки СYке СYка СYкУ СYкО СYкИ СYкЕ СYкА СYКy СYКo СYКe СYКa СYКY СYКO СYКE СYКA СYКу СYКо СYКи СYКе СYКа СYКУ СYКО СYКИ СYКЕ СYКА Сцука Суky Суko Суka СуkY СуkO СуkA Суkу Суkо Суkи Суkе Суkа СуkУ СуkО СуkИ СуkЕ СуkА СуKy СуKo СуKa СуKY СуKO СуKA СуKу СуKо СуKи СуKе СуKа СуKУ СуKО СуKИ СуKЕ СуKА Сучара Сукy Сукo Сукe Сукa СукY СукO СукE СукA Суку Суко Суки Суке Сука СукУ СукО СукИ СукЕ СукА СуКy СуКo СуКe СуКa СуКY СуКO СуКE СуКA СуКу СуКо СуКи СуКе СуКа СуКУ СуКО СуКИ СуКЕ СуКА СЦУКА СУky СУko СУka СУkY СУkO СУkA СУkу СУkо СУkи СУkе СУkа СУkУ СУkО СУkИ СУkЕ СУkА СУKy СУKo СУKa СУKY СУKO СУKA СУKу СУKо СУKи СУKе СУKа СУKУ СУKО СУKИ СУKЕ СУKА СУкy СУкo СУкe СУкa СУкY СУкO СУкE СУкA СУку СУко СУки СУке СУка СУкУ СУкО СУкИ СУкЕ СУкА СУЧАРА СУЧАΠА СУКy СУКo СУКe СУКa СУКY СУКO СУКE СУКA СУКу СУКо СУКи СУКе СУКа СУКУ СУКО СУКИ СУКЕ СУКА Похую Похуй Пи3д Пи3Д Пистапол Писдит Пизжен Пизд Пизда ПизД Пидp Пидop ПидoP Пидoр ПидoΠ ПидP ПидOp ПидOP ПидOр ПидOΠ Пидр Пидоp ПидоP Пидор Пидорас ПидоΠ Пидир Пидер Пидар Пидарас ПидОp ПидОP ПидОр ПидОΠ ПидΠ ПиЗд ПиЗД ПиДp ПиДop ПиДoP ПиДoр ПиДoΠ ПиДP ПиДOp ПиДOP ПиДOр ПиДOΠ ПиДр ПиДоp ПиДоP ПиДор ПиДоΠ ПиДОp ПиДОP ПиДОр ПиДОΠ ПиДΠ Пестато Перееб Переебло ПОХУЮ ПОХУЙ ПИ3д ПИ3Д ПИзд ПИзД ПИдp ПИдop ПИдoP ПИдoр ПИдoΠ ПИдP ПИдOp ПИдOP ПИдOр ПИдOΠ ПИдр ПИдоp ПИдоP ПИдор ПИдоΠ ПИдОp ПИдОP ПИдОр ПИдОΠ ПИдΠ ПИСТАПОЛ ПИСДИШ ПИСДИТ ПИСДЕШ ПИЗд ПИЗЖЕН ПИЗД ПИЗДА ПИДp ПИДop ПИДoP ПИДoр ПИДoΠ ПИДP ПИДOp ПИДOP ПИДOр ПИДOР ПИДOΠ ПИДр ПИДоp ПИДоP ПИДор ПИДоΠ ПИДР ПИДОp ПИДОP ПИДОр ПИДОР ПИДОРАС ПИДОΠ ПИДОΠАС ПИДИР ПИДИΠ ПИДЕР ПИДЕΠ ПИДАР ПИДАРАС ПИДАΠ ПИДАΠАС ПИДΠ ПЕСТАТО ПЕРЕЕБ ПЕРЕЕБЛО ПЕΠЕЕБ ПЕΠЕЕБЛО Охуй Отьеб Отъеб Отъебло Отебло ОХУЙ ОТЬЕБ ОТЪЕБ ОТЪЕБЛО ОТЕБЛО Ниху Нихе Ниибаца Неху Нехуй Нехе Наху НИХУ НИХЕ НИИБАЦА НЕХУ НЕХУЙ НЕХЕ НАХУ Мудо Мудн Мудл Муди Муде Муда Мокрощелка МУДО МУДН МУДЛ МУДИ МУДЕ МУДА МОКРОЩЕЛКА МОКΠОЩЕЛКА МАНДАВОШКА Ипись Иннах Изъеб Ибанут ИПИСЬ ИННАХ ИЗЪЕБ ИБАНУТ Заёб Залупить Залупа Заеб Заебу Заебло ЗАЛУПИТЬ ЗАЛУПА ЗАЕБ ЗАЕБУ ЗАЕБЛО ЗАЁБ Ебыр Ебырь Ебущий Ебут Ебну Ебло Ебли Еблан Еби Ебеш Ебень Ебеный Ебенный Ебат Ебать Ебанут Ебало Ебали ЕбИ ЕБи ЕБЫР ЕБЫРЬ ЕБЫΠ ЕБЫΠЬ ЕБУЩИЙ ЕБУТ ЕБНУ ЕБЛО ЕБЛИ ЕБЛАН ЕБИ ЕБЕШ ЕБЕНЬ ЕБЕНЫЙ ЕБЕННЫЙ ЕБАТ ЕБАТЬ ЕБАНУТ ЕБАНАШКА ЕБАЛО ЕБАЛИ Долбо ДОЛБО Гребан Гонд ГРЕБАН ГОНД ГΠЕБАН Выеб Выебу ВЫЕБ ВЫЕБУ Бл9 Бля Блятc Блят Бляд Блябу БлЯ БЛ9 БЛя БЛЯ БЛЯТC БЛЯТ БЛЯД БЛЯБУ Аебли АЕБЛИ Ёбну ЁБНУ";

        if( m_CensorDictionary == 1 )
            m_CensorWords = s;
        else
            m_CensorWords = s + " " + CFG->GetString( "bot_censorwords", "***" );
    }

    ParseCensoredWords( );

    if( !first )
    {
        gLanguageFile = UTIL_FixFilePath( CFG->GetString( "bot_language", "text_files\\Languages\\Russian.cfg" ) );

        if( gLanguage )
            delete gLanguage;

        gLanguage = new CLanguage( gLanguageFile );
    }

    m_Warcraft3Path = UTIL_FixPath( CFG->GetString( "bot_war3path", "war3\\" ) );
    m_BindAddress = CFG->GetString( "bot_bindaddress", string( ) );
    BotCommandTrigger = CFG->GetString( "bot_commandtrigger", "-!.@" );

    if( BotCommandTrigger.empty( ) )
        BotCommandTrigger = "-";

    m_CommandTrigger = '\0';
    m_CommandTrigger1 = '\0';
    m_CommandTrigger2 = '\0';
    m_CommandTrigger3 = '\0';
    m_CommandTrigger4 = '\0';
    m_CommandTrigger5 = '\0';
    m_CommandTrigger6 = '\0';
    m_CommandTrigger7 = '\0';
    m_CommandTrigger8 = '\0';
    m_CommandTrigger9 = '\0';
    m_CommandTrigger = BotCommandTrigger[0];

    if( BotCommandTrigger.size( ) >= 2 )
        m_CommandTrigger1 = BotCommandTrigger[1];

    if( BotCommandTrigger.size( ) >= 3 )
        m_CommandTrigger2 = BotCommandTrigger[2];

    if( BotCommandTrigger.size( ) >= 4 )
        m_CommandTrigger3 = BotCommandTrigger[3];

    if( BotCommandTrigger.size( ) >= 5 )
        m_CommandTrigger4 = BotCommandTrigger[4];

    if( BotCommandTrigger.size( ) >= 6 )
        m_CommandTrigger5 = BotCommandTrigger[5];

    if( BotCommandTrigger.size( ) >= 7 )
        m_CommandTrigger6 = BotCommandTrigger[6];

    if( BotCommandTrigger.size( ) >= 8 )
        m_CommandTrigger7 = BotCommandTrigger[7];

    if( BotCommandTrigger.size( ) >= 9 )
        m_CommandTrigger8 = BotCommandTrigger[8];

    if( BotCommandTrigger.size( ) >= 10 )
        m_CommandTrigger9 = BotCommandTrigger[9];

    m_MapCFGPath = UTIL_FixPath( CFG->GetString( "bot_mapcfgpath", "mapcfgs\\" ) );
    m_SaveGamePath = UTIL_FixPath( CFG->GetString( "bot_savegamepath", "savegames\\" ) );
    m_MapPath = UTIL_FixPath( CFG->GetString( "bot_mappath", "maps\\" ) );
    m_ReplayPath = UTIL_FixPath( CFG->GetString( "bot_replaypath", "replays\\" ) );
    m_IPBlackListFile = UTIL_FixFilePath( CFG->GetString( "bot_ipblacklistfile", "text_files\\ipblacklist.txt" ) );
    m_DefaultMap = CFG->GetString( "bot_defaultmap", "IER 1.66a.cfg" );
    m_BotChannelCustom = CFG->GetString( "bot_botchannelcustom", string( ) );
    m_BotNameCustom = CFG->GetString( "bot_botnamecustom", string( ) );

    if( !m_GameStartedMessagePrinting )
    {
        for( vector<CBNET *> ::iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); i++ )
        {
            ( *i )->m_GameStartedMessagePrinting = false;
        }
    }

    m_InvalidTriggersGame = CFG->GetString( "bot_invalidtriggersgame", string( ) );
    m_InvalidReplayChars = CFG->GetString( "bot_invalidreplaychars", "/|\\:*?\"<>" );
    m_RehostChar = CFG->GetString( "bot_rehostchar", "*" );

    if( m_RehostChar.empty( ) || m_RehostChar.size( ) != 1 )
    {
        m_RehostChar = "*";
        CONSOLE_Print( "[UNM] warning - bot_rehostchar is empty or longer than 1 characters in 1251 (ANSI) encoding, value reset to \"*\"" );
    }

    m_DefaultGameNameChar = CFG->GetString( "bot_gamenamechar", string( ) );
    m_GameNameChar = m_DefaultGameNameChar;
    m_TempGameNameChar = m_GameNameChar;

    for( uint32_t i = 0; i <= 10000; i++ )
    {
        if( CFG->GetString( "bot_hostmapconfig" + UTIL_ToString( i + 1 ), string( ) ).empty( ) || CFG->GetString( "bot_hostmapcode" + UTIL_ToString( i + 1 ), string( ) ).empty( ) )
            break;

        m_HostMapConfig[i] = CFG->GetString( "bot_hostmapconfig" + UTIL_ToString( i + 1 ), string( ) );
        m_HostMapCode[i] = CFG->GetString( "bot_hostmapcode" + UTIL_ToString( i + 1 ), string( ) );
    }

    m_VirtualHostName = CFG->GetString( "bot_virtualhostname", string( ) );

    if( m_VirtualHostName.size( ) > 15 )
        CONSOLE_Print( "[UNM] warning - bot_virtualhostname is longer than 15 characters, part of the name is truncated" );

    m_VirtualHostName = UTIL_SubStrFix( m_VirtualHostName, 0, 15 );

    if( m_VirtualHostName.empty( ) )
    {
        CONSOLE_Print( "[UNM] warning - bad bot_virtualhostname detected, default value will be used" );
        m_VirtualHostName = "|cFF00FF00unm";
    }

    m_BroadcastHostNameCustom = CFG->GetString( "bot_broadcasthncustom", m_VirtualHostName );
    ReadAnnounce( );
    ReadWelcome( );
    ReadMars( );
    ReadBut( );
    ReadSlap( );
    ReadFP( );
    m_ObsPlayerName = CFG->GetString( "bot_obsplayername", "|cFF00FF00Obs" );
    m_AllowLaggingTime = m_AllowLaggingTime * 1000;
    gBotID = CFG->GetUInt( "db_mysql_botid", 1 );

    emit gToLog->setCommandsTips( first );

    if( gUNM != nullptr && gUNM )
        emit gToLog->UpdateValuesInGUISignal( { } );
}

void CUNM::ReadAnnounce( )
{
    string file;
    ifstream in;

#ifdef WIN32
    file = "text_files\\announce.txt";
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    file = "text_files/announce.txt";
    in.open( file );
#endif

    m_Announce.clear( );

    if( in.fail( ) )
        CONSOLE_Print( "[UNM] warning - unable to read file [" + file + "]" );
    else
    {
        CONSOLE_Print( "[UNM] loading file [" + file + "]" );
        string Line;

        while( !in.eof( ) )
        {
            getline( in, Line );
            Line = UTIL_FixFileLine( Line );

            // ignore blank lines and comments

            if( Line.empty( ) || Line[0] == '#' )
                continue;

            m_Announce.push_back( Line );
        }
    }

    in.close( );
}

void CUNM::ReadWelcome( )
{
    string file;
    ifstream in;

#ifdef WIN32
    file = "text_files\\welcome.txt";
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    file = "text_files/welcome.txt";
    in.open( file );
#endif

    m_Welcome.clear( );

    if( in.fail( ) )
        CONSOLE_Print( "[UNM] warning - unable to read file [" + file + "]" );
    else
    {
        CONSOLE_Print( "[UNM] loading file [" + file + "]" );
        string Line;

        while( !in.eof( ) )
        {
            getline( in, Line );
            Line = UTIL_FixFileLine( Line );

            // ignore blank lines and comments

            if( Line.empty( ) || Line[0] == '#' )
                continue;

            m_Welcome.push_back( Line );
        }
    }

    in.close( );
}

string CUNM::GetMars( )
{
    if( m_Mars.empty( ) )
        return string( );

    bool ok = true;

    // delete the oldest message

    if( ( m_MarsLast.size( ) >= m_Mars.size( ) || m_MarsLast.size( ) > 15 ) && m_MarsLast.size( ) > 0 )
        m_MarsLast.erase( m_MarsLast.begin( ) );

    do
    {
        ok = true;
        std::random_device rand_dev;
        std::mt19937 rng( rand_dev( ) );
        shuffle( m_Mars.begin( ), m_Mars.end( ), rng );

        for( uint32_t i = 0; i < m_MarsLast.size( ); i++ )
        {
            if( m_MarsLast[i] == m_Mars[0] )
            {
                ok = false;
                break;
            }
        }
    }
    while( !ok );

    m_MarsLast.push_back( m_Mars[0] );
    return m_Mars[0];
}

string CUNM::GetBut( )
{
    if( m_But.empty( ) )
        return string( );

    bool ok = true;

    // delete the oldest message

    if( ( m_ButLast.size( ) >= m_But.size( ) || m_ButLast.size( ) > 15 ) && m_ButLast.size( ) > 0 )
        m_ButLast.erase( m_ButLast.begin( ) );

    do
    {
        ok = true;
        std::random_device rand_dev;
        std::mt19937 rng( rand_dev( ) );
        shuffle( m_But.begin( ), m_But.end( ), rng );

        for( uint32_t i = 0; i < m_ButLast.size( ); i++ )
        {
            if( m_ButLast[i] == m_But[0] )
            {
                ok = false;
                break;
            }
        }
    }
    while( !ok );

    m_ButLast.push_back( m_But[0] );
    return m_But[0];
}

string CUNM::GetSlap( )
{
    if( m_Slap.empty( ) )
        return string( );

    bool ok = true;

    // delete the oldest message

    if( ( m_SlapLast.size( ) >= m_Slap.size( ) || m_SlapLast.size( ) > 15 ) && m_SlapLast.size( ) > 0 )
        m_SlapLast.erase( m_SlapLast.begin( ) );

    do
    {
        ok = true;
        std::random_device rand_dev;
        std::mt19937 rng( rand_dev( ) );
        shuffle( m_Slap.begin( ), m_Slap.end( ), rng  );

        for( uint32_t i = 0; i < m_SlapLast.size( ); i++ )
        {
            if( m_SlapLast[i] == m_Slap[0] )
            {
                ok = false;
                break;
            }
        }
    }
    while( !ok );

    m_SlapLast.push_back( m_Slap[0] );
    return m_Slap[0];
}

string CUNM::GetFPName( )
{
    if( m_FPNames.empty( ) )
        return string( );

    std::random_device rand_dev;
    std::mt19937 rng( rand_dev( ) );
    shuffle( m_FPNames.begin( ), m_FPNames.end( ), rng );

    if( m_FPNamesLast.empty( ) )
        return m_FPNames[0];
    else
    {
        for( uint32_t i = 0; i < m_FPNames.size( ); i++ )
        {
            bool bad = false;

            for( uint32_t j = 0; j < m_FPNamesLast.size( ); j++ )
            {
                if( m_FPNames[i] == m_FPNamesLast[j] )
                    bad = true;
            }

            if( !bad )
                return m_FPNames[i];
        }
    }

    return string( );
}

string CUNM::GetRehostChar( )
{
    if( m_RehostChar.size( ) == 1 )
        return m_RehostChar;
    else
        return "*";
}

void CUNM::ReadFP( )
{
    string file;
    ifstream in;

#ifdef WIN32
    file = "text_files\\fpnames.txt";
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    file = "text_files/fpnames.txt";
    in.open( file );
#endif

    m_FPNames.clear( );

    if( in.fail( ) )
        CONSOLE_Print( "[UNM] warning - unable to read file [" + file + "]" );
    else
    {
        CONSOLE_Print( "[UNM] loading file [" + file + "]" );
        string Line;

        while( !in.eof( ) )
        {
            getline( in, Line );
            Line = UTIL_FixFileLine( Line );

            // ignore blank lines and comments

            if( Line.empty( ) || Line[0] == '#' )
                continue;

            m_FPNames.push_back( Line );
        }
    }

    in.close( );
}

void CUNM::ReadMars( )
{
    string file;
    ifstream in;

#ifdef WIN32
    file = "text_files\\mars.txt";
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    file = "text_files/mars.txt";
    in.open( file );
#endif

    m_Mars.clear( );

    if( in.fail( ) )
        CONSOLE_Print( "[UNM] warning - unable to read file [" + file + "]" );
    else
    {
        CONSOLE_Print( "[UNM] loading file [" + file + "]" );
        string Line;

        while( !in.eof( ) )
        {
            getline( in, Line );
            Line = UTIL_FixFileLine( Line );

            // ignore blank lines and comments

            if( Line.empty( ) || Line[0] == '#' )
                continue;

            m_Mars.push_back( Line );
        }
    }

    in.close( );
}

void CUNM::ReadBut( )
{
    string file;
    ifstream in;

#ifdef WIN32
    file = "text_files\\but.txt";
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    file = "text_files/but.txt";
    in.open( file );
#endif

    m_But.clear( );

    if( in.fail( ) )
        CONSOLE_Print( "[UNM] warning - unable to read file [" + file + "]" );
    else
    {
        CONSOLE_Print( "[UNM] loading file [" + file + "]" );
        string Line;

        while( !in.eof( ) )
        {
            getline( in, Line );
            Line = UTIL_FixFileLine( Line );

            // ignore blank lines and comments

            if( Line.empty( ) || Line[0] == '#' )
                continue;

            m_But.push_back( Line );
        }
    }

    in.close( );
}

void CUNM::ReadSlap( )
{
    string file;
    ifstream in;

#ifdef WIN32
    file = "text_files\\slap.txt";
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    file = "text_files/slap.txt";
    in.open( file );
#endif

    m_Slap.clear( );

    if( in.fail( ) )
        CONSOLE_Print( "[UNM] warning - unable to read file [" + file + "]" );
    else
    {
        CONSOLE_Print( "[UNM] loading file [" + file + "]" );
        string Line;

        while( !in.eof( ) )
        {
            getline( in, Line );
            Line = UTIL_FixFileLine( Line );

            // ignore blank lines and comments

            if( Line.empty( ) || Line[0] == '#' )
                continue;

            m_Slap.push_back( Line );
        }
    }

    in.close( );
}

void CUNM::ParseCensoredWords( )
{
    m_CensoredWords.clear( );
    transform( m_CensorWords.begin( ), m_CensorWords.end( ), m_CensorWords.begin( ), static_cast<int(*)(int)>(tolower) );
    stringstream SS;
    string s;
    SS << m_CensorWords;

    while( !SS.eof( ) )
    {
        SS >> s;
        m_CensoredWords.push_back( s );
    }
}

string CUNM::Censor( string msg )
{
    uint32_t idx = 1;
    string Result = msg.substr( 0, idx );

    for( uint32_t i = idx; i <= msg.length( ) - idx; i++ )
    {
        Result += "*";
    }

    return Result;
}

string CUNM::CensorMessage( string msg )
{
    if( msg.length( ) < 1 || m_CensorWords == "***" )
        return msg;

    string Msg = msg;

    for( vector<string> ::iterator i = m_CensoredWords.begin( ); i != m_CensoredWords.end( ); i++ )
    {
        UTIL_Replace2( Msg, ( *i ), Censor( ( *i ) ) );
    }

    return Msg;
}

bool CUNM::CensorCheck( string message )
{
    transform( message.begin( ), message.end( ), message.begin( ), static_cast<int(*)(int)>(tolower) );

    for( vector<string> ::iterator i = m_CensoredWords.begin( ); i != m_CensoredWords.end( ); i++ )
    {
        if( message.find( ( *i ) ) != string::npos )
            return true;
    }

    return false;
}

bool CUNM::IsIccupServer( string servername )
{
    if( servername == "127.0.0.1" || servername == "178.218.214.114" )
        return true;

    servername = UTIL_StringToLower( servername );

    if( servername == "wc3.theabyss.ru" || servername.find( "iccup" ) != string::npos || servername.find( "айкап" ) != string::npos )
        return true;

    return false;
}

string CUNM::HostMapCode( string name )
{
    transform( name.begin( ), name.end( ), name.begin( ), static_cast<int(*)(int)>(tolower) );
    string HostMapCode;
    string HostMapConfigsTest[1000];
    uint32_t HostMapConfigsNubmer = 0;

    for( uint32_t i = 0; i <= 999; i++ )
    {
        HostMapCode = m_HostMapCode[i];

        if( !HostMapCode.empty( ) )
        {
            transform( HostMapCode.begin( ), HostMapCode.end( ), HostMapCode.begin( ), static_cast<int(*)(int)>(tolower) );
            stringstream SS;
            string s;
            SS << HostMapCode;

            while( !SS.eof( ) )
            {
                SS >> s;

                if( name == s )
                {
                    HostMapConfigsTest[HostMapConfigsNubmer] = m_HostMapConfig[i];
                    HostMapConfigsNubmer++;
                }
            }
        }
    }

    if( HostMapConfigsTest[0].empty( ) )
    {
        if( HostMapConfigsNubmer == 0 )
            return "Invalid map code";
        else
            return "Invalid map config";
    }
    else if( HostMapConfigsTest[1].empty( ) )
    {
        if( !HostMapConfigsTest[0].empty( ) )
        {
            string HostMapConfig = HostMapConfigsTest[0];

            if( HostMapConfig.find_first_not_of( " " ) == string::npos )
                return "Invalid map config";

            if( HostMapConfig.size( ) > 4 )
            {
                if( HostMapConfig.substr( HostMapConfig.size( ) - 4 ) != ".cfg" )
                    HostMapConfig = HostMapConfig + ".cfg";

                return HostMapConfig;
            }
            else if( HostMapConfig == ".cfg" )
                return "Invalid map config";
            else
            {
                HostMapConfig = HostMapConfig + ".cfg";
                return HostMapConfig;
            }
        }
        else
            return "Invalid map config";
    }
    else
    {
        string HostMapConfig2 = HostMapConfigsTest[UTIL_GenerateRandomNumberUInt32( 0, HostMapConfigsNubmer - 1 )];

        if( !HostMapConfig2.empty( ) )
        {
            if( HostMapConfig2.find_first_not_of( " " ) == string::npos )
                return "Invalid map config";

            if( HostMapConfig2.size( ) > 4 )
            {
                if( HostMapConfig2.substr( HostMapConfig2.size( ) - 4 ) != ".cfg" )
                    HostMapConfig2 = HostMapConfig2 + ".cfg";
            }
            else if( HostMapConfig2 == ".cfg" )
                return "Invalid map config";
            else
                HostMapConfig2 = HostMapConfig2 + ".cfg";

            HostMapConfig2 = HostMapConfig2 + " random";
            return HostMapConfig2;
        }
        else
            return "Invalid map config";
    }
}

void CUNM::AddSpoofedIP( string name, string ip )
{
    m_CachedSpoofedIPs.push_back( ip );
    m_CachedSpoofedNames.push_back( name );
}


bool CUNM::IsSpoofedIP( string name, string ip )
{
    for( uint32_t i = 0; i != m_CachedSpoofedIPs.size( ); i++ )
    {
        if( m_CachedSpoofedIPs[i] == ip && m_CachedSpoofedNames[i] == name )
            return true;
    }

    return false;
}

string CUNM::IncGameNr( string name, bool spambot, bool needEncrypt, uint32_t occupiedslots, uint32_t allslots )
{
    if( m_MaxHostCounter == 0 )
        return name;

    name = IncGameNrDecryption( name, true );
    string GameName = name;
    string GameNr = string( );
    string s = GetRehostChar( );
    bool found = false;
    uint32_t idx = 0;
    uint32_t id;
    uint32_t Nr = 0;

    if( m_SlotInfoInGameName == 1 )
    {
        if( GameName.find( " [10/10]" ) != string::npos || GameName.find( " [10/11]" ) != string::npos || GameName.find( " [10/12]" ) != string::npos || GameName.find( " [11/10]" ) != string::npos || GameName.find( " [11/11]" ) != string::npos || GameName.find( " [11/12]" ) != string::npos || GameName.find( " [12/10]" ) != string::npos || GameName.find( " [12/11]" ) != string::npos || GameName.find( " [12/12]" ) != string::npos )
            GameName = GameName.substr( 0, GameName.size( ) - 8 );
        else if( GameName.find( " [0/10]" ) != string::npos || GameName.find( " [0/11]" ) != string::npos || GameName.find( " [0/12]" ) != string::npos || GameName.find( " [1/10]" ) != string::npos || GameName.find( " [1/11]" ) != string::npos || GameName.find( " [1/12]" ) != string::npos || GameName.find( " [2/10]" ) != string::npos || GameName.find( " [2/11]" ) != string::npos || GameName.find( " [2/12]" ) != string::npos || GameName.find( " [3/10]" ) != string::npos || GameName.find( " [3/11]" ) != string::npos || GameName.find( " [3/12]" ) != string::npos || GameName.find( " [4/10]" ) != string::npos || GameName.find( " [4/11]" ) != string::npos || GameName.find( " [4/12]" ) != string::npos || GameName.find( " [5/10]" ) != string::npos || GameName.find( " [5/11]" ) != string::npos || GameName.find( " [5/12]" ) != string::npos || GameName.find( " [6/10]" ) != string::npos || GameName.find( " [6/11]" ) != string::npos || GameName.find( " [6/12]" ) != string::npos || GameName.find( " [7/10]" ) != string::npos || GameName.find( " [7/11]" ) != string::npos || GameName.find( " [7/12]" ) != string::npos || GameName.find( " [8/10]" ) != string::npos || GameName.find( " [8/11]" ) != string::npos || GameName.find( " [8/12]" ) != string::npos || GameName.find( " [9/10]" ) != string::npos || GameName.find( " [9/11]" ) != string::npos || GameName.find( " [9/12]" ) != string::npos || GameName.find( " [10/1]" ) != string::npos || GameName.find( " [10/2]" ) != string::npos || GameName.find( " [10/3]" ) != string::npos || GameName.find( " [10/4]" ) != string::npos || GameName.find( " [10/5]" ) != string::npos || GameName.find( " [10/6]" ) != string::npos || GameName.find( " [10/7]" ) != string::npos || GameName.find( " [10/8]" ) != string::npos || GameName.find( " [10/9]" ) != string::npos || GameName.find( " [11/1]" ) != string::npos || GameName.find( " [11/2]" ) != string::npos || GameName.find( " [11/3]" ) != string::npos || GameName.find( " [11/4]" ) != string::npos || GameName.find( " [11/5]" ) != string::npos || GameName.find( " [11/6]" ) != string::npos || GameName.find( " [11/7]" ) != string::npos || GameName.find( " [11/8]" ) != string::npos || GameName.find( " [11/9]" ) != string::npos || GameName.find( " [12/1]" ) != string::npos || GameName.find( " [12/2]" ) != string::npos || GameName.find( " [12/3]" ) != string::npos || GameName.find( " [12/4]" ) != string::npos || GameName.find( " [12/5]" ) != string::npos || GameName.find( " [12/6]" ) != string::npos || GameName.find( " [12/7]" ) != string::npos || GameName.find( " [12/8]" ) != string::npos || GameName.find( " [12/9]" ) != string::npos )
            GameName = GameName.substr( 0, GameName.size( ) - 7 );
        else if( GameName.find( " [0/1]" ) != string::npos || GameName.find( " [0/2]" ) != string::npos || GameName.find( " [0/3]" ) != string::npos || GameName.find( " [0/4]" ) != string::npos || GameName.find( " [0/5]" ) != string::npos || GameName.find( " [0/6]" ) != string::npos || GameName.find( " [0/7]" ) != string::npos || GameName.find( " [0/8]" ) != string::npos || GameName.find( " [0/9]" ) != string::npos || GameName.find( " [1/1]" ) != string::npos || GameName.find( " [1/2]" ) != string::npos || GameName.find( " [1/3]" ) != string::npos || GameName.find( " [1/4]" ) != string::npos || GameName.find( " [1/5]" ) != string::npos || GameName.find( " [1/6]" ) != string::npos || GameName.find( " [1/7]" ) != string::npos || GameName.find( " [1/8]" ) != string::npos || GameName.find( " [1/9]" ) != string::npos || GameName.find( " [2/1]" ) != string::npos || GameName.find( " [2/2]" ) != string::npos || GameName.find( " [2/3]" ) != string::npos || GameName.find( " [2/4]" ) != string::npos || GameName.find( " [2/5]" ) != string::npos || GameName.find( " [2/6]" ) != string::npos || GameName.find( " [2/7]" ) != string::npos || GameName.find( " [2/8]" ) != string::npos || GameName.find( " [2/9]" ) != string::npos || GameName.find( " [3/1]" ) != string::npos || GameName.find( " [3/2]" ) != string::npos || GameName.find( " [3/3]" ) != string::npos || GameName.find( " [3/4]" ) != string::npos || GameName.find( " [3/5]" ) != string::npos || GameName.find( " [3/6]" ) != string::npos || GameName.find( " [3/7]" ) != string::npos || GameName.find( " [3/8]" ) != string::npos || GameName.find( " [3/9]" ) != string::npos || GameName.find( " [4/1]" ) != string::npos || GameName.find( " [4/2]" ) != string::npos || GameName.find( " [4/3]" ) != string::npos || GameName.find( " [4/4]" ) != string::npos || GameName.find( " [4/5]" ) != string::npos || GameName.find( " [4/6]" ) != string::npos || GameName.find( " [4/7]" ) != string::npos || GameName.find( " [4/8]" ) != string::npos || GameName.find( " [4/9]" ) != string::npos || GameName.find( " [5/1]" ) != string::npos || GameName.find( " [5/2]" ) != string::npos || GameName.find( " [5/3]" ) != string::npos || GameName.find( " [5/4]" ) != string::npos || GameName.find( " [5/5]" ) != string::npos || GameName.find( " [5/6]" ) != string::npos || GameName.find( " [5/7]" ) != string::npos || GameName.find( " [5/8]" ) != string::npos || GameName.find( " [5/9]" ) != string::npos || GameName.find( " [6/1]" ) != string::npos || GameName.find( " [6/2]" ) != string::npos || GameName.find( " [6/3]" ) != string::npos || GameName.find( " [6/4]" ) != string::npos || GameName.find( " [6/5]" ) != string::npos || GameName.find( " [6/6]" ) != string::npos || GameName.find( " [6/7]" ) != string::npos || GameName.find( " [6/8]" ) != string::npos || GameName.find( " [6/9]" ) != string::npos || GameName.find( " [7/1]" ) != string::npos || GameName.find( " [7/2]" ) != string::npos || GameName.find( " [7/3]" ) != string::npos || GameName.find( " [7/4]" ) != string::npos || GameName.find( " [7/5]" ) != string::npos || GameName.find( " [7/6]" ) != string::npos || GameName.find( " [7/7]" ) != string::npos || GameName.find( " [7/8]" ) != string::npos || GameName.find( " [7/9]" ) != string::npos || GameName.find( " [8/1]" ) != string::npos || GameName.find( " [8/2]" ) != string::npos || GameName.find( " [8/3]" ) != string::npos || GameName.find( " [8/4]" ) != string::npos || GameName.find( " [8/5]" ) != string::npos || GameName.find( " [8/6]" ) != string::npos || GameName.find( " [8/7]" ) != string::npos || GameName.find( " [8/8]" ) != string::npos || GameName.find( " [8/9]" ) != string::npos || GameName.find( " [9/1]" ) != string::npos || GameName.find( " [9/2]" ) != string::npos || GameName.find( " [9/3]" ) != string::npos || GameName.find( " [9/4]" ) != string::npos || GameName.find( " [9/5]" ) != string::npos || GameName.find( " [9/6]" ) != string::npos || GameName.find( " [9/7]" ) != string::npos || GameName.find( " [9/8]" ) != string::npos || GameName.find( " [9/9]" ) != string::npos )
            GameName = GameName.substr( 0, GameName.size( ) - 6 );
    }

    name = GameName;
    idx = GameName.length( ) - 1;

    for( id = 7; id >= 1; id-- )
    {
        if( idx >= id )
        {
            if( GameName.at( idx - id ) == s[0] )
            {
                idx = idx - id + 1;
                found = true;
                break;
            }
        }
    }

    if( !found )
        idx = 0;

    // idx = 0, no Game Nr found in gamename

    if( idx == 0 )
    {
        if( m_SlotInfoInGameName == 1 )
        {
            if( name.size( ) + UTIL_ToString( occupiedslots ).size( ) + UTIL_ToString( allslots ).size( ) > 24 )
                name = UTIL_SubStrFix( name, 0, 24 - UTIL_ToString( occupiedslots ).size( ) - UTIL_ToString( allslots ).size( ) );

            GameNr = "0";
            GameName = name + " " + s[0];
        }
        else if( m_SlotInfoInGameName == 2 )
        {
            if( name.size( ) + UTIL_ToString( occupiedslots ).size( ) + UTIL_ToString( allslots ).size( ) > 24 )
                name = UTIL_SubStrFix( name, 0, 24 - UTIL_ToString( occupiedslots ).size( ) - UTIL_ToString( allslots ).size( ) );

            GameNr = "0";
            GameName = name + " [" + UTIL_ToString( occupiedslots ) + "/" + UTIL_ToString( allslots ) + "] " + s[0];
        }
        else
        {
            if( name.size( ) > 28 )
                name = UTIL_SubStrFix( name, 0, 28 );

            GameNr = "0";
            GameName = name + " " + s[0];
        }
    }
    else if( m_SlotInfoInGameName == 2 )
    {
        GameNr = GameName.substr( idx, GameName.length( ) - idx );
        GameName = GameName.substr( 0, idx );

        if( GameName.size( ) >= 2 && GameName.substr( GameName.size( ) - 2 ) == " " + s )
            GameName.erase( GameName.end( ) - 2, GameName.end( ) );
        else if( GameName.size( ) >= 1 && GameName.back( ) == s[0] )
            GameName.erase( GameName.end( ) - 1 );

        if( GameName.find( " [10/10]" ) != string::npos || GameName.find( " [10/11]" ) != string::npos || GameName.find( " [10/12]" ) != string::npos || GameName.find( " [11/10]" ) != string::npos || GameName.find( " [11/11]" ) != string::npos || GameName.find( " [11/12]" ) != string::npos || GameName.find( " [12/10]" ) != string::npos || GameName.find( " [12/11]" ) != string::npos || GameName.find( " [12/12]" ) != string::npos )
            GameName = GameName.substr( 0, GameName.length( ) - 8 );
        else if( GameName.find( " [0/10]" ) != string::npos || GameName.find( " [0/11]" ) != string::npos || GameName.find( " [0/12]" ) != string::npos || GameName.find( " [1/10]" ) != string::npos || GameName.find( " [1/11]" ) != string::npos || GameName.find( " [1/12]" ) != string::npos || GameName.find( " [2/10]" ) != string::npos || GameName.find( " [2/11]" ) != string::npos || GameName.find( " [2/12]" ) != string::npos || GameName.find( " [3/10]" ) != string::npos || GameName.find( " [3/11]" ) != string::npos || GameName.find( " [3/12]" ) != string::npos || GameName.find( " [4/10]" ) != string::npos || GameName.find( " [4/11]" ) != string::npos || GameName.find( " [4/12]" ) != string::npos || GameName.find( " [5/10]" ) != string::npos || GameName.find( " [5/11]" ) != string::npos || GameName.find( " [5/12]" ) != string::npos || GameName.find( " [6/10]" ) != string::npos || GameName.find( " [6/11]" ) != string::npos || GameName.find( " [6/12]" ) != string::npos || GameName.find( " [7/10]" ) != string::npos || GameName.find( " [7/11]" ) != string::npos || GameName.find( " [7/12]" ) != string::npos || GameName.find( " [8/10]" ) != string::npos || GameName.find( " [8/11]" ) != string::npos || GameName.find( " [8/12]" ) != string::npos || GameName.find( " [9/10]" ) != string::npos || GameName.find( " [9/11]" ) != string::npos || GameName.find( " [9/12]" ) != string::npos || GameName.find( " [10/1]" ) != string::npos || GameName.find( " [10/2]" ) != string::npos || GameName.find( " [10/3]" ) != string::npos || GameName.find( " [10/4]" ) != string::npos || GameName.find( " [10/5]" ) != string::npos || GameName.find( " [10/6]" ) != string::npos || GameName.find( " [10/7]" ) != string::npos || GameName.find( " [10/8]" ) != string::npos || GameName.find( " [10/9]" ) != string::npos || GameName.find( " [11/1]" ) != string::npos || GameName.find( " [11/2]" ) != string::npos || GameName.find( " [11/3]" ) != string::npos || GameName.find( " [11/4]" ) != string::npos || GameName.find( " [11/5]" ) != string::npos || GameName.find( " [11/6]" ) != string::npos || GameName.find( " [11/7]" ) != string::npos || GameName.find( " [11/8]" ) != string::npos || GameName.find( " [11/9]" ) != string::npos || GameName.find( " [12/1]" ) != string::npos || GameName.find( " [12/2]" ) != string::npos || GameName.find( " [12/3]" ) != string::npos || GameName.find( " [12/4]" ) != string::npos || GameName.find( " [12/5]" ) != string::npos || GameName.find( " [12/6]" ) != string::npos || GameName.find( " [12/7]" ) != string::npos || GameName.find( " [12/8]" ) != string::npos || GameName.find( " [12/9]" ) != string::npos )
            GameName = GameName.substr( 0, GameName.length( ) - 7 );
        else if( GameName.find( " [0/1]" ) != string::npos || GameName.find( " [0/2]" ) != string::npos || GameName.find( " [0/3]" ) != string::npos || GameName.find( " [0/4]" ) != string::npos || GameName.find( " [0/5]" ) != string::npos || GameName.find( " [0/6]" ) != string::npos || GameName.find( " [0/7]" ) != string::npos || GameName.find( " [0/8]" ) != string::npos || GameName.find( " [0/9]" ) != string::npos || GameName.find( " [1/1]" ) != string::npos || GameName.find( " [1/2]" ) != string::npos || GameName.find( " [1/3]" ) != string::npos || GameName.find( " [1/4]" ) != string::npos || GameName.find( " [1/5]" ) != string::npos || GameName.find( " [1/6]" ) != string::npos || GameName.find( " [1/7]" ) != string::npos || GameName.find( " [1/8]" ) != string::npos || GameName.find( " [1/9]" ) != string::npos || GameName.find( " [2/1]" ) != string::npos || GameName.find( " [2/2]" ) != string::npos || GameName.find( " [2/3]" ) != string::npos || GameName.find( " [2/4]" ) != string::npos || GameName.find( " [2/5]" ) != string::npos || GameName.find( " [2/6]" ) != string::npos || GameName.find( " [2/7]" ) != string::npos || GameName.find( " [2/8]" ) != string::npos || GameName.find( " [2/9]" ) != string::npos || GameName.find( " [3/1]" ) != string::npos || GameName.find( " [3/2]" ) != string::npos || GameName.find( " [3/3]" ) != string::npos || GameName.find( " [3/4]" ) != string::npos || GameName.find( " [3/5]" ) != string::npos || GameName.find( " [3/6]" ) != string::npos || GameName.find( " [3/7]" ) != string::npos || GameName.find( " [3/8]" ) != string::npos || GameName.find( " [3/9]" ) != string::npos || GameName.find( " [4/1]" ) != string::npos || GameName.find( " [4/2]" ) != string::npos || GameName.find( " [4/3]" ) != string::npos || GameName.find( " [4/4]" ) != string::npos || GameName.find( " [4/5]" ) != string::npos || GameName.find( " [4/6]" ) != string::npos || GameName.find( " [4/7]" ) != string::npos || GameName.find( " [4/8]" ) != string::npos || GameName.find( " [4/9]" ) != string::npos || GameName.find( " [5/1]" ) != string::npos || GameName.find( " [5/2]" ) != string::npos || GameName.find( " [5/3]" ) != string::npos || GameName.find( " [5/4]" ) != string::npos || GameName.find( " [5/5]" ) != string::npos || GameName.find( " [5/6]" ) != string::npos || GameName.find( " [5/7]" ) != string::npos || GameName.find( " [5/8]" ) != string::npos || GameName.find( " [5/9]" ) != string::npos || GameName.find( " [6/1]" ) != string::npos || GameName.find( " [6/2]" ) != string::npos || GameName.find( " [6/3]" ) != string::npos || GameName.find( " [6/4]" ) != string::npos || GameName.find( " [6/5]" ) != string::npos || GameName.find( " [6/6]" ) != string::npos || GameName.find( " [6/7]" ) != string::npos || GameName.find( " [6/8]" ) != string::npos || GameName.find( " [6/9]" ) != string::npos || GameName.find( " [7/1]" ) != string::npos || GameName.find( " [7/2]" ) != string::npos || GameName.find( " [7/3]" ) != string::npos || GameName.find( " [7/4]" ) != string::npos || GameName.find( " [7/5]" ) != string::npos || GameName.find( " [7/6]" ) != string::npos || GameName.find( " [7/7]" ) != string::npos || GameName.find( " [7/8]" ) != string::npos || GameName.find( " [7/9]" ) != string::npos || GameName.find( " [8/1]" ) != string::npos || GameName.find( " [8/2]" ) != string::npos || GameName.find( " [8/3]" ) != string::npos || GameName.find( " [8/4]" ) != string::npos || GameName.find( " [8/5]" ) != string::npos || GameName.find( " [8/6]" ) != string::npos || GameName.find( " [8/7]" ) != string::npos || GameName.find( " [8/8]" ) != string::npos || GameName.find( " [8/9]" ) != string::npos || GameName.find( " [9/1]" ) != string::npos || GameName.find( " [9/2]" ) != string::npos || GameName.find( " [9/3]" ) != string::npos || GameName.find( " [9/4]" ) != string::npos || GameName.find( " [9/5]" ) != string::npos || GameName.find( " [9/6]" ) != string::npos || GameName.find( " [9/7]" ) != string::npos || GameName.find( " [9/8]" ) != string::npos || GameName.find( " [9/9]" ) != string::npos )
            GameName = GameName.substr( 0, GameName.length( ) - 6 );

        uint32_t GameNrTest = UTIL_ToUInt32( GameNr );

        if( GameNrTest >= m_MaxHostCounter && m_MaxHostCounter > 0 )
            GameNrTest = 1;
        else
            GameNrTest = GameNrTest + 1;

        if( GameName.size( ) + UTIL_ToString( occupiedslots ).size( ) + UTIL_ToString( allslots ).size( ) + UTIL_ToString( GameNrTest ).size( ) > 25 )
            GameName = UTIL_SubStrFix( GameName, 0, 25 - UTIL_ToString( occupiedslots ).size( ) - UTIL_ToString( allslots ).size( ) - UTIL_ToString( GameNrTest ).size( ) );

        GameName = GameName + " [" + UTIL_ToString( occupiedslots ) + "/" + UTIL_ToString( allslots ) + "]";
        GameName = GameName + " " + s[0];
    }
    else if( m_SlotInfoInGameName == 1 )
    {
        GameNr = GameName.substr( idx, GameName.length( ) - idx );
        GameName = GameName.substr( 0, idx );

        uint32_t GameNrTest = UTIL_ToUInt32( GameNr );

        if( GameNrTest >= m_MaxHostCounter && m_MaxHostCounter > 0 )
            GameNrTest = 1;
        else
            GameNrTest = GameNrTest + 1;

        if( GameName.size( ) + UTIL_ToString( GameNrTest ).size( ) + UTIL_ToString( occupiedslots ).size( ) + UTIL_ToString( allslots ).size( ) > 27 )
        {
            if( GameName.size( ) >= 2 && GameName.substr( GameName.size( ) - 2 ) == " " + s )
                GameName.erase( GameName.end( ) - 2, GameName.end( ) );
            else if( GameName.size( ) >= 1 && GameName.back( ) == s[0] )
                GameName.erase( GameName.end( ) - 1 );

            GameName = UTIL_SubStrFix( GameName, 0, 25 - UTIL_ToString( occupiedslots ).size( ) - UTIL_ToString( allslots ).size( ) - UTIL_ToString( GameNrTest ).size( ) );
            GameName = GameName + " " + s[0];
        }
    }
    else
    {
        GameNr = GameName.substr( idx, GameName.length( ) - idx );
        GameName = GameName.substr( 0, idx );

        uint32_t GameNrTest = UTIL_ToUInt32( GameNr );

        if( GameNrTest >= m_MaxHostCounter && m_MaxHostCounter > 0 )
            GameNrTest = 1;
        else
            GameNrTest = GameNrTest + 1;

        if( GameName.size( ) + UTIL_ToString( GameNrTest ).size( ) > 31 )
        {
            if( GameName.size( ) >= 2 && GameName.substr( GameName.size( ) - 2 ) == " " + s )
                GameName.erase( GameName.end( ) - 2, GameName.end( ) );
            else if( GameName.size( ) >= 1 && GameName.back( ) == s[0] )
                GameName.erase( GameName.end( ) - 1 );

            GameName = UTIL_SubStrFix( GameName, 0, 29 - UTIL_ToString( GameNrTest ).size( ) );
            GameName = GameName + " " + s[0];
        }
    }

    stringstream SS;
    SS << GameNr;
    SS >> Nr;
    Nr++;

    if( m_MaxHostCounter != 0 && Nr > m_MaxHostCounter )
        Nr = 1;

    GameNr = UTIL_ToString( Nr );

    if( m_SlotInfoInGameName == 1 )
        GameName = GameName + GameNr + " [" + UTIL_ToString( occupiedslots ) + "/" + UTIL_ToString( allslots ) + "]";
    else
        GameName = GameName + GameNr;

    if( GameName.size( ) > 31 )
        GameName = UTIL_SubStrFix( GameName, GameName.size( ) - 31, 0 );

    if( m_GameNameCharHide == 1 || ( m_GameNameCharHide == 2 && spambot ) || ( m_GameNameCharHide == 3 && needEncrypt ) )
        GameName = IncGameNrEncryption( GameName );

    return GameName;
}

string CUNM::IncGameNrEncryption( string name )
{
    string::size_type FindRehostChar = name.find( " " + GetRehostChar( ) );

    if( FindRehostChar != string::npos )
    {
        string RehostChar = name.substr( FindRehostChar );
        UTIL_Replace( RehostChar, " " + GetRehostChar( ), m_s );
        string::size_type FindSpace = RehostChar.find( " " );

        if( FindSpace != string::npos )
        {
            RehostChar = RehostChar.substr( 0, FindSpace );
            UTIL_Replace( RehostChar, "1", m_s1 );
            UTIL_Replace( RehostChar, "2", m_s2 );
            UTIL_Replace( RehostChar, "3", m_s3 );
            UTIL_Replace( RehostChar, "4", m_s4 );
            UTIL_Replace( RehostChar, "5", m_s5 );
            UTIL_Replace( RehostChar, "6", m_s6 );
            UTIL_Replace( RehostChar, "7", m_s7 );
            UTIL_Replace( RehostChar, "8", m_s8 );
            UTIL_Replace( RehostChar, "9", m_s9 );
            UTIL_Replace( RehostChar, "0", m_s0 );
            name = name.substr( 0, FindRehostChar ) + RehostChar + name.substr( FindRehostChar + FindSpace );
        }
        else
        {
            UTIL_Replace( RehostChar, "1", m_s1 );
            UTIL_Replace( RehostChar, "2", m_s2 );
            UTIL_Replace( RehostChar, "3", m_s3 );
            UTIL_Replace( RehostChar, "4", m_s4 );
            UTIL_Replace( RehostChar, "5", m_s5 );
            UTIL_Replace( RehostChar, "6", m_s6 );
            UTIL_Replace( RehostChar, "7", m_s7 );
            UTIL_Replace( RehostChar, "8", m_s8 );
            UTIL_Replace( RehostChar, "9", m_s9 );
            UTIL_Replace( RehostChar, "0", m_s0 );
            name = name.substr( 0, FindRehostChar ) + RehostChar;
        }
    }

    return name;
}

string CUNM::IncGameNrDecryption( string name, bool force )
{
    if( !force )
    {
        if( m_CurrentGame == nullptr || !m_CurrentGame || m_CurrentGame->GetGameName( ) != name )
            return name;
        else if( !m_CurrentGameName.empty( ) )
            return m_CurrentGameName;
    }

    string::size_type FindRehostChar = name.find( m_s );

    if( FindRehostChar != string::npos )
    {
        string RehostChar = name.substr( FindRehostChar );
        string::size_type FindSpace = RehostChar.find( " " );

        if( FindSpace != string::npos )
        {
            RehostChar = RehostChar.substr( 0, FindSpace );
            UTIL_Replace( RehostChar, m_s, " " + GetRehostChar( ) );
            UTIL_Replace( RehostChar, m_s1, "1" );
            UTIL_Replace( RehostChar, m_s2, "2" );
            UTIL_Replace( RehostChar, m_s3, "3" );
            UTIL_Replace( RehostChar, m_s4, "4" );
            UTIL_Replace( RehostChar, m_s5, "5" );
            UTIL_Replace( RehostChar, m_s6, "6" );
            UTIL_Replace( RehostChar, m_s7, "7" );
            UTIL_Replace( RehostChar, m_s8, "8" );
            UTIL_Replace( RehostChar, m_s9, "9" );
            UTIL_Replace( RehostChar, m_s0, "0" );
            name = name.substr( 0, FindRehostChar ) + RehostChar + name.substr( FindRehostChar + FindSpace );
        }
        else
        {
            UTIL_Replace( RehostChar, m_s, " " + GetRehostChar( ) );
            UTIL_Replace( RehostChar, m_s1, "1" );
            UTIL_Replace( RehostChar, m_s2, "2" );
            UTIL_Replace( RehostChar, m_s3, "3" );
            UTIL_Replace( RehostChar, m_s4, "4" );
            UTIL_Replace( RehostChar, m_s5, "5" );
            UTIL_Replace( RehostChar, m_s6, "6" );
            UTIL_Replace( RehostChar, m_s7, "7" );
            UTIL_Replace( RehostChar, m_s8, "8" );
            UTIL_Replace( RehostChar, m_s9, "9" );
            UTIL_Replace( RehostChar, m_s0, "0" );
            name = name.substr( 0, FindRehostChar ) + RehostChar;
        }
    }

    m_CurrentGameName = name;
    return name;
}

bool CUNM::IsCorrectNumInSegm( string const & s, int min, int max )
{
    bool bool_res{ };

    try
    {
        auto num = std::stoi( s );
        bool_res = num >= min && num <= max;
    }
    catch( ... )
    {
        bool_res = false;
    }

    return bool_res;
}

bool CUNM::IPAdrIsCorrect( string const & s )
{
    const char POINT_SYMB{ '.' };
    const int NUMBERS_TOTAL{ 4 };

    bool bool_res = s.back( ) != POINT_SYMB && std::find_if( s.begin( ), s.end( ), [POINT_SYMB]( auto symb )
    {
        return !std::isdigit( symb ) && symb != POINT_SYMB;
    } ) == s.end( );

    if( bool_res )
    {
        std::istringstream ssin( s );
        int counter{ };
        string num_str;

        while( getline( ssin, num_str, POINT_SYMB ) )
        {
            ++counter;
            bool_res = counter == NUMBERS_TOTAL ? IsCorrectNumInSegm( num_str, 1, 255 ) : IsCorrectNumInSegm( num_str, 0, 255 );

            if( !bool_res )
                break;
        }

        bool_res = bool_res && counter == NUMBERS_TOTAL;
    }

    return bool_res;
}

bool CUNM::IPIsValid( string ip )
{
    regex e( "((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)\\.){3}(25[0-5]|2[0-4]\\d|[1]?\\d\\d?)" );
    smatch m;

    if( regex_match( ip, m, e ) && IPAdrIsCorrect( ip ) )
        return true;
    else
        return false;
}

string CUNM::CheckMapConfig( string map, bool force )
{
    if( !m_OverrideMapDataFromCFG && !force )
        return string( );

    try
    {
        std::filesystem::path MapCFGPath( m_MapCFGPath );
        string Pattern = UTIL_GetFileNameFromPath( map );

        if( Pattern.find_first_not_of( " " ) == string::npos )
            return string( );

        Pattern = UTIL_StringToLower( Pattern );

        if( !std::filesystem::exists( MapCFGPath ) )
            CONSOLE_Print( "[UNM] error listing map configs - map config path doesn't exist" );
        else
        {
            using convert_typeX = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_typeX, wchar_t> converterX;
            std::filesystem::recursive_directory_iterator EndIterator;
            string FileName;
            string Stem;

            for( std::filesystem::recursive_directory_iterator i( MapCFGPath ); i != EndIterator; i++ )
            {
                if( !is_directory( i->status( ) ) && i->path( ).extension( ) == ".cfg" )
                {
                    FileName = converterX.to_bytes( i->path( ).filename( ) );
                    FileName = UTIL_StringToLower( FileName );

                    if( FileName.find( Pattern ) != string::npos )
                    {
                        Stem = converterX.to_bytes( i->path( ).stem( ) );
                        Stem = UTIL_StringToLower( Stem );

                        // if the pattern matches the filename exactly, with or without extension, stop any further matching

                        if( FileName == Pattern || Stem == Pattern )
                        {
                            string FilePath = converterX.to_bytes( i->path( ) );
                            return FilePath;
                        }
                    }
                }
            }
        }
    }
    catch( const exception &ex )
    {
        CONSOLE_Print( "[UNM] error listing map configs - caught exception [" + (string)ex.what( ) + "]" );
    }

    return string( );
}

string CUNM::FindMap( string map, string mapdefault )
{
    try
    {
        std::filesystem::path MapPath( m_MapPath );
        string Pattern = map;
        Pattern = UTIL_StringToLower( Pattern );

        if( !std::filesystem::exists( MapPath ) )
            CONSOLE_Print( "[UNM] error listing maps - map path doesn't exist" );
        else
        {
            using convert_typeX = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_typeX, wchar_t> converterX;
            std::filesystem::recursive_directory_iterator EndIterator;
            string FilePath;
            string FileName;
            string Stem;

            for( std::filesystem::recursive_directory_iterator i( MapPath ); i != EndIterator; i++ )
            {
                if( !is_directory( i->status( ) ) && ( i->path( ).extension( ) == ".w3x" || i->path( ).extension( ) == ".w3m" ) )
                {
                    FileName = converterX.to_bytes( i->path( ).filename( ) );
                    FileName = UTIL_StringToLower( FileName );

                    if( FileName.find( Pattern ) != string::npos )
                    {
                        Stem = converterX.to_bytes( i->path( ).stem( ) );
                        Stem = UTIL_StringToLower( Stem );
                        FilePath = converterX.to_bytes( i->path( ) );

                        // if the pattern matches the filename exactly, with or without extension, stop any further matching

                        if( FileName == Pattern || ( Stem == Pattern && ( i->path( ).extension( ) == m_MapType || !UTIL_FileExists( FilePath.substr( 0, FilePath.size( ) - 4 ) + m_MapType ) ) ) )
                            return FilePath;
                    }
                }
            }
        }
    }
    catch( const exception &ex )
    {
        CONSOLE_Print( "[UNM] error listing maps - caught exception [" + string( ex.what( ) ) + "]" );
    }

    return mapdefault;
}

void CUNM::MapsScan( bool ConfigsScan, string Pattern, string &FilePath, string &File, string &Stem, string &FoundMaps, uint32_t &Matches )
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;
    std::filesystem::path Path( ConfigsScan ? m_MapCFGPath : m_MapPath );
    std::filesystem::recursive_directory_iterator EndIterator;
    string FileName;
    string FileNameTest;
    string PatternTest;
    string PatternTemp;
    string FoundMapsSpecial;
    string FileTest;
    string PatternUpper = Pattern;
    transform( PatternUpper.begin( ), PatternUpper.end( ), PatternUpper.begin( ), static_cast<int(*)(int)>(toupper) );
    string::size_type CharPos;
    string::size_type CharPosSpecial;
    string::size_type CharPosSpecial2;
    string::size_type CharPosSpecial3;
    string::size_type CharPosSpecial4;
    uint32_t NumDiv = 0;
    uint32_t NumDivTemp = 0;
    uint32_t MaxLength = 0;
    uint32_t MaxLengthTemp = 0;
    uint32_t SpecialScanTest = 0;
    bool PatternFound = false;
    bool SpaceFound = false;
    bool LineFound = false;
    bool UnderLineFound = false;
    bool DotFound = false;
    bool CommaFound = false;
    bool Bracket1Found = false;
    bool Bracket2Found = false;
    bool Bracket3Found = false;
    bool Bracket4Found = false;
    bool SpecialScan = false;
    bool firsttime = true;
    string teststring = string( );

    if( Pattern.size( ) <= 7 && Pattern.find_first_not_of( "qwertyuiopasdfghjklzxcvbnm" ) == string::npos )
        SpecialScan = true;

    if( Pattern.find( " " ) != string::npos )
        SpaceFound = true;

    if( Pattern.find( "-" ) != string::npos )
        LineFound = true;

    if( Pattern.find( "_" ) != string::npos )
        UnderLineFound = true;

    if( Pattern.find( "." ) != string::npos )
        DotFound = true;

    if( Pattern.find( "," ) != string::npos )
        CommaFound = true;

    if( Pattern.find( "(" ) != string::npos )
        Bracket1Found = true;

    if( Pattern.find( ")" ) != string::npos )
        Bracket2Found = true;

    if( Pattern.find( "[" ) != string::npos )
        Bracket3Found = true;

    if( Pattern.find( "]" ) != string::npos )
        Bracket4Found = true;

    for( std::filesystem::recursive_directory_iterator i( Path ); i != EndIterator; i++ )
    {
        if( !is_directory( i->status( ) ) && ( ( !ConfigsScan && ( i->path( ).extension( ) == ".w3x" || i->path( ).extension( ) == ".w3m" ) ) || ( ConfigsScan && i->path( ).extension( ) == ".cfg" ) ) )
        {
            FileName = converterX.to_bytes( i->path( ).filename( ) );
            FileTest = FileName;
            FileName = UTIL_StringToLower( FileName );
            FileNameTest = FileName;
            PatternTest = Pattern;
            PatternTemp = Pattern;
            PatternFound = true;
            SpecialScanTest = 0;
            NumDivTemp = 0;
            MaxLengthTemp = 0;

            if( !SpaceFound )
            {
                teststring = string( );
                UTIL_Replace( FileNameTest, " ", teststring );
            }
            else
                teststring = " ";

            if( !LineFound )
                UTIL_Replace( FileNameTest, "-", teststring );

            if( !UnderLineFound )
                UTIL_Replace( FileNameTest, "_", teststring );

            if( !DotFound )
                UTIL_Replace( FileNameTest, ".", teststring );

            if( !CommaFound )
                UTIL_Replace( FileNameTest, ",", teststring );

            if( !Bracket1Found )
                UTIL_Replace( FileNameTest, "(", teststring );

            if( !Bracket2Found )
                UTIL_Replace( FileNameTest, ")", teststring );

            if( !Bracket3Found )
                UTIL_Replace( FileNameTest, "[", teststring );

            if( !Bracket4Found )
                UTIL_Replace( FileNameTest, "]", teststring );

            while( !PatternTest.empty( ) )
            {
                CharPos = FileNameTest.find( PatternTemp ) ;

                if( CharPos != string::npos )
                {
                    if( PatternTemp.size( ) > MaxLengthTemp )
                        MaxLengthTemp = PatternTemp.size( );

                    FileNameTest.erase( CharPos, PatternTemp.size( ) );
                    PatternTest = PatternTest.substr( PatternTemp.size( ) );
                    PatternTemp = PatternTest;
                    NumDivTemp++;

                    if( NumDiv != 0 && ( NumDivTemp > NumDiv || ( NumDivTemp == NumDiv && MaxLengthTemp < MaxLength ) ) )
                    {
                        if( SpecialScan )
                        {
                            FileNameTest = FileName;
                            PatternTemp = Pattern;

                            if( FileNameTest.size( ) > 4 )
                                FileNameTest = FileNameTest.substr( 0, FileNameTest.size( ) - 4 );

                            firsttime = true;

                            while( !PatternTemp.empty( ) )
                            {
                                if( firsttime && FileNameTest[0] == PatternTemp[0] )
                                    CharPosSpecial = 301;
                                else
                                {
                                    CharPosSpecial = 300;
                                    CharPosSpecial2 = FileNameTest.find( " " + PatternTemp.substr(0, 1) );
                                    CharPosSpecial3 = FileNameTest.find( "_" + PatternTemp.substr(0, 1) );
                                    CharPosSpecial4 = FileNameTest.find( "-" + PatternTemp.substr(0, 1) );

                                    if( CharPosSpecial2 == string::npos )
                                        CharPosSpecial2 = 300;

                                    if( CharPosSpecial3 == string::npos )
                                        CharPosSpecial3 = 300;

                                    if( CharPosSpecial4 == string::npos )
                                        CharPosSpecial4 = 300;

                                    if( CharPosSpecial2 > CharPosSpecial3 )
                                        CharPosSpecial2 = CharPosSpecial3;

                                    if( CharPosSpecial2 > CharPosSpecial4 )
                                        CharPosSpecial = CharPosSpecial4;
                                    else
                                        CharPosSpecial = CharPosSpecial2;
                                }

                                firsttime = false;

                                if( CharPosSpecial != 300 )
                                {
                                    if( PatternTemp.size( ) <= 1 )
                                    {
                                        SpecialScanTest = 1;
                                        PatternFound = true;
                                        break;
                                    }
                                    else if( CharPosSpecial == 301 )
                                    {
                                        FileNameTest = FileNameTest.substr( 1 );
                                        PatternTemp = PatternTemp.substr( 1 );
                                    }
                                    else
                                    {
                                        FileNameTest = FileNameTest.substr( CharPosSpecial + 2 );
                                        PatternTemp = PatternTemp.substr( 1 );
                                    }
                                }
                                else
                                {
                                    PatternFound = false;
                                    break;
                                }
                            }

                            if( SpecialScanTest == 0 )
                            {
                                FileNameTest = FileTest;
                                PatternTemp = PatternUpper;

                                if( FileNameTest.size( ) > 4 )
                                    FileNameTest = FileNameTest.substr( 0, FileNameTest.size( ) - 4 );

                                if( FileNameTest.find_first_of( "qwertyuiopasdfghjklzxcvbnm" ) != string::npos )
                                {
                                    while( !PatternTemp.empty( ) )
                                    {
                                        CharPosSpecial = FileNameTest.find( PatternTemp.substr(0, 1) );

                                        if( CharPosSpecial != string::npos )
                                        {
                                            if( PatternTemp.size( ) > 1 )
                                            {
                                                FileNameTest = FileNameTest.substr( CharPosSpecial + 1 );
                                                PatternTemp = PatternTemp.substr( 1 );
                                            }
                                            else
                                            {
                                                SpecialScanTest = 2;
                                                PatternFound = true;
                                                break;
                                            }
                                        }
                                        else
                                        {
                                            PatternFound = false;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        else
                            PatternFound = false;

                        break;
                    }
                    else if( PatternTest.empty( ) )
                    {
                        if( NumDivTemp < NumDiv || NumDiv == 0 || MaxLengthTemp > MaxLength )
                        {
                            MaxLength = MaxLengthTemp;
                            NumDiv = NumDivTemp;
                            FoundMaps.clear( );
                            Matches = 0;
                        }

                        if( SpecialScan )
                        {
                            FileNameTest = FileName;
                            PatternTemp = Pattern;

                            if( FileNameTest.size( ) > 4 )
                                FileNameTest = FileNameTest.substr( 0, FileNameTest.size( ) - 4 );

                            firsttime = true;

                            while( !PatternTemp.empty( ) )
                            {
                                if( firsttime && FileNameTest[0] == PatternTemp[0] )
                                    CharPosSpecial = 301;
                                else
                                {
                                    CharPosSpecial = 300;
                                    CharPosSpecial2 = FileNameTest.find( " " + PatternTemp.substr(0, 1) );
                                    CharPosSpecial3 = FileNameTest.find( "_" + PatternTemp.substr(0, 1) );
                                    CharPosSpecial4 = FileNameTest.find( "-" + PatternTemp.substr(0, 1) );

                                    if( CharPosSpecial2 == string::npos )
                                        CharPosSpecial2 = 300;

                                    if( CharPosSpecial3 == string::npos )
                                        CharPosSpecial3 = 300;

                                    if( CharPosSpecial4 == string::npos )
                                        CharPosSpecial4 = 300;

                                    if( CharPosSpecial2 > CharPosSpecial3 )
                                        CharPosSpecial2 = CharPosSpecial3;

                                    if( CharPosSpecial2 > CharPosSpecial4 )
                                        CharPosSpecial = CharPosSpecial4;
                                    else
                                        CharPosSpecial = CharPosSpecial2;
                                }

                                firsttime = false;

                                if( CharPosSpecial != 300 )
                                {
                                    if( PatternTemp.size( ) <= 1 )
                                    {
                                        SpecialScanTest = 1;
                                        PatternFound = true;
                                        break;
                                    }
                                    else if( CharPosSpecial == 301 )
                                    {
                                        FileNameTest = FileNameTest.substr( 1 );
                                        PatternTemp = PatternTemp.substr( 1 );
                                    }
                                    else
                                    {
                                        FileNameTest = FileNameTest.substr( CharPosSpecial + 2 );
                                        PatternTemp = PatternTemp.substr( 1 );
                                    }
                                }
                                else
                                    break;
                            }

                            if( SpecialScanTest == 0 )
                            {
                                FileNameTest = FileTest;
                                PatternTemp = PatternUpper;

                                if( FileNameTest.size( ) > 4 )
                                    FileNameTest = FileNameTest.substr( 0, FileNameTest.size( ) - 4 );

                                if( FileNameTest.find_first_of( "qwertyuiopasdfghjklzxcvbnm" ) != string::npos )
                                {
                                    while( !PatternTemp.empty( ) )
                                    {
                                        CharPosSpecial = FileNameTest.find( PatternTemp.substr(0, 1) );

                                        if( CharPosSpecial != string::npos )
                                        {
                                            if( PatternTemp.size( ) > 1 )
                                            {
                                                FileNameTest = FileNameTest.substr( CharPosSpecial + 1 );
                                                PatternTemp = PatternTemp.substr( 1 );
                                            }
                                            else
                                            {
                                                SpecialScanTest = 2;
                                                PatternFound = true;
                                                break;
                                            }
                                        }
                                        else
                                            break;
                                    }
                                }
                            }
                        }

                        break;
                    }
                }
                else
                {
                    if( PatternTemp.size( ) > 1 )
                        PatternTemp.erase( PatternTemp.end( ) - 1 );
                    else
                    {
                        PatternFound = false;
                        break;
                    }
                }
            }

            if( PatternFound )
            {
                Matches++;
                Stem = converterX.to_bytes( i->path( ).stem( ) );
                File = FileTest;
                FilePath = converterX.to_bytes( i->path( ) );
                Stem = UTIL_StringToLower( Stem );

                if( SpecialScanTest > 0 )
                {
                    if( FoundMapsSpecial.empty( ) )
                        FoundMapsSpecial = File;
                    else if( SpecialScanTest == 1 )
                        FoundMapsSpecial = File + ", " + FoundMapsSpecial;
                    else
                        FoundMapsSpecial += ", " + File;
                }
                else if( FoundMaps.empty( ) )
                    FoundMaps = File;
                else
                    FoundMaps += ", " + File;

                // if the pattern matches the filename exactly, with or without extension, stop any further matching

                if( FileName == Pattern || ( ConfigsScan && Stem == Pattern ) || ( !ConfigsScan && ( Stem == Pattern && ( i->path( ).extension( ) == m_MapType || !UTIL_FileExists( FilePath.substr( 0, FilePath.size( ) - 4 ) + m_MapType ) ) ) ) )
                {
                    Matches = 1;
                    break;
                }
            }
        }
    }

    if( !FoundMapsSpecial.empty( ) )
        FoundMaps = FoundMapsSpecial + ", " + FoundMaps;
}

void CUNM::MapLoad( string FilePath, string File, string Stem, string ServerAliasWithUserName, bool loadcfg, bool melee )
{
    if( loadcfg )
    {
        CConfig MapCFG;
        MapCFG.Read( FilePath );

        if( melee )
            MapCFG.Set( "map_gametype", "2" );

        m_Map->Load( &MapCFG, File, FindMap( Stem ) );
    }
    else
    {
        if( m_AllowAutoRenameMaps )
            MapAutoRename( FilePath, File, ServerAliasWithUserName + " " );

        string FileCFG = CheckMapConfig( Stem + ".cfg" );

        // hackhack: create a config file in memory with the required information to load the map

        CConfig MapCFG;

        if( !FileCFG.empty( ) && UTIL_FileExists( FileCFG ) )
        {
            MapCFG.Read( FileCFG );

            if( !m_OverrideMapPathFromCFG )
            {
                MapCFG.Set( "map_path", "Maps\\Download\\" + File );
                MapCFG.Set( "map_localpath", File );
            }

            if( melee )
                MapCFG.Set( "map_gametype", "2" );

            m_Map->Load( &MapCFG, File, FilePath );
        }
        else
        {
            MapCFG.Set( "map_path", "Maps\\Download\\" + File );
            MapCFG.Set( "map_localpath", File );

            if( melee )
                MapCFG.Set( "map_gametype", "2" );

            m_Map->Load( &MapCFG, File, FilePath );
        }
    }
}

void CUNM::MapAutoRename( string &FilePath, string &File, string toLog )
{
    QString QFile = QString::fromUtf8( File.c_str( ) );

    if( QFile.size( ) <= 39 )
        return;

    QString QFilePath = QString::fromUtf8( FilePath.c_str( ) );

    if( QFilePath.size( ) < QFile.size( ) )
        return;

    int32_t FileNameEnd = 0;
    int32_t QFileSizeTest = QFile.size( ) - 1;

    for( int32_t i = QFileSizeTest; i < QFile.size( ); i-- )
    {
        if( QFile[i] == "." )
        {
            FileNameEnd = i;
            break;
        }
        else if ( i == 0 )
            break;
    }

    if( FileNameEnd < 10 || ( QFileSizeTest - FileNameEnd ) > 24 )
        FileNameEnd = QFileSizeTest;

    QString QFileExt = QString( );

    if( FileNameEnd < QFileSizeTest )
        QFileExt = QFile.mid( FileNameEnd );

    uint32_t QFileExtSize = QFileExt.size( );
    QString QFileNewName = QFile.mid( 0, 39 - QFileExtSize ) + QFileExt;
    QString QFileNewNameTest = QFileNewName.mid( 0, 35 - QFileExtSize );
    string FileNewName = QFileNewName.toStdString( );
    string is = string( );
    QString iqs = QString( );
    uint32_t i = 1;

    while( UTIL_FileExists( FilePath.substr( 0, FilePath.size( ) - File.size( ) ) + FileNewName ) && i <= 99 )
    {
        if( i == 10 )
            QFileNewNameTest = QFileNewNameTest.mid( 0, QFileNewNameTest.size( ) - 1 );

        is = " (" + UTIL_ToString( i ) + ")";
        iqs = QString::fromUtf8( is.c_str( ) );
        QFileNewName = QFileNewNameTest + iqs + QFileExt;
        FileNewName = QFileNewName.toStdString( );
        i++;
    }


    if( i >= 100 )
        CONSOLE_Print( toLog + "AutoRenameMap: ошибка при переименовании файла [" + FilePath + "]" );
    else if( UTIL_FileRename( FilePath, FilePath.substr( 0, FilePath.size( ) - File.size( ) ) + FileNewName ) )
    {
        CONSOLE_Print( toLog + "AutoRenameMap: файл [" + File + "] успешно переименован в [" + FileNewName + "]" );
        FilePath = FilePath.substr( 0, FilePath.size( ) - File.size( ) ) + FileNewName;
        File = FileNewName;
    }
    else
        CONSOLE_Print( toLog + "AutoRenameMap: ошибка при переименовании файла [" + FilePath + "]" );
}

uint32_t CUNM::TimeDifference( uint32_t sub, uint32_t minimum )
{
    if( GetTime( ) <= sub )
        return minimum;
    else
        return GetTime( ) - sub;
}

uint32_t CUNM::TicksDifference( uint32_t sub, uint32_t minimum )
{
    if( GetTicks( ) <= sub )
        return minimum;
    else
        return GetTicks( ) - sub;
}

bool CUNM::IsBnetServer( string server )
{
    if( server.empty( ) || server == "LAN" || server.substr( 0, 6 ) == "Garena" )
        return false;

    return true;
}

void CUNM::ExtractLocalPackets( )
{
    if( !m_LocalSocket )
        return;

    string *RecvBuffer = m_LocalSocket->GetBytes( );
    BYTEARRAY Bytes = UTIL_CreateByteArray( (unsigned char *)RecvBuffer->c_str( ), RecvBuffer->size( ) );

    // a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

    while( Bytes.size( ) >= 4 )
    {
        // byte 0 is always 247

        if( Bytes[0] == W3GS_HEADER_CONSTANT )
        {
            // bytes 2 and 3 contain the length of the packet

            uint16_t Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

            if( Length >= 4 )
            {
                if( Bytes.size( ) >= Length )
                {
                    // we have to do a little bit of packet processing here
                    // this is because we don't want to forward any chat messages that start with a "/" as these may be forwarded to battle.net instead
                    // in fact we want to pretend they were never even received from the proxy's perspective

                    bool Forward = true;
                    BYTEARRAY Data = BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length );

                    if( Bytes[1] == CGameProtocol::W3GS_CHAT_TO_HOST )
                    {
                        if( Data.size( ) >= 5 )
                        {
                            unsigned int i = 5;
                            unsigned char Total = Data[4];

                            if( Total > 0 && Data.size( ) >= i + Total )
                            {
                                i += Total;
                                unsigned char Flag = Data[i + 1];
                                i += 2;

                                string MessageString;

                                if( Flag == 16 && Data.size( ) >= i + 1 )
                                {
                                    BYTEARRAY Message = UTIL_ExtractCString( Data, i );
                                    MessageString = string( Message.begin( ), Message.end( ) );
                                }
                                else if( Flag == 32 && Data.size( ) >= i + 5 )
                                {
                                    BYTEARRAY Message = UTIL_ExtractCString( Data, i + 4 );
                                    MessageString = string( Message.begin( ), Message.end( ) );
                                }

                                string Command = MessageString;
                                transform( Command.begin( ), Command.end( ), Command.begin( ), (int(*)(int))tolower );

                                if( Command.size( ) >= 1 && Command.substr( 0, 1 ) == "/" )
                                {
                                    Forward = false;

                                    if( m_UsedBnetID == 0 )
                                        SendLocalChat( "Ошибка! Battle.net соединение не определенно." );
                                    else if( Command.size( ) >= 4 && Command.substr( 0, 3 ) == "/r " )
                                    {
                                        if( m_BNETs[m_UsedBnetID-1]->GetLoggedIn( ) )
                                        {
                                            if( !m_BNETs[m_UsedBnetID-1]->m_ReplyTarget.empty( ) )
                                            {
                                                m_BNETs[m_UsedBnetID-1]->QueueChatCommand( MessageString.substr( 3 ), m_BNETs[m_UsedBnetID-1]->m_ReplyTarget, true );
                                                SendLocalChat( "Whispered to " + m_BNETs[m_UsedBnetID-1]->m_ReplyTarget + ": " + MessageString.substr( 3 ) );
                                            }
                                            else
                                                SendLocalChat( "Nobody has whispered you yet." );
                                        }
                                        else
                                            SendLocalChat( "You are not connected to battle.net." );
                                    }
                                    else if( Command.size( ) >= 5 && Command.substr( 0, 4 ) == "/re " )
                                    {
                                        if( m_BNETs[m_UsedBnetID-1]->GetLoggedIn( ) )
                                        {
                                            if( !m_BNETs[m_UsedBnetID-1]->m_ReplyTarget.empty( ) )
                                            {
                                                m_BNETs[m_UsedBnetID-1]->QueueChatCommand( MessageString.substr( 4 ), m_BNETs[m_UsedBnetID-1]->m_ReplyTarget, true );
                                                SendLocalChat( "Whispered to " + m_BNETs[m_UsedBnetID-1]->m_ReplyTarget + ": " + MessageString.substr( 4 ) );
                                            }
                                            else
                                                SendLocalChat( "Nobody has whispered you yet." );
                                        }
                                        else
                                            SendLocalChat( "You are not connected to battle.net." );
                                    }
                                    else if( Command == "/sc" || Command == "/spoof" || Command == "/spoofcheck" || Command == "/spoof check" )
                                    {
                                        if( m_BNETs[m_UsedBnetID-1]->GetLoggedIn( ) )
                                        {
                                            if( !m_GameStarted )
                                            {
                                                m_BNETs[m_UsedBnetID-1]->QueueChatCommand( "spoofcheck", m_HostName, true );
                                                SendLocalChat( "Whispered to " + m_HostName + ": spoofcheck" );
                                            }
                                            else
                                                SendLocalChat( "The game has already started." );
                                        }
                                        else
                                            SendLocalChat( "You are not connected to battle.net." );
                                    }
                                    else if( Command == "/status" )
                                    {
                                        if( m_LocalSocket )
                                        {
                                            if( m_GameIsReliable && m_GPPort > 0 )
                                                SendLocalChat( "GProxy++ disconnect protection: Enabled" );
                                            else
                                                SendLocalChat( "GProxy++ disconnect protection: Disabled" );

                                            if( m_BNETs[m_UsedBnetID-1]->GetLoggedIn( ) )
                                                SendLocalChat( "battle.net: Connected" );
                                            else
                                                SendLocalChat( "battle.net: Disconnected" );
                                        }
                                    }
                                    else if( Command.size( ) >= 4 && Command.substr( 0, 3 ) == "/w " )
                                    {
                                        if( m_BNETs[m_UsedBnetID-1]->GetLoggedIn( ) )
                                        {
                                            m_BNETs[m_UsedBnetID-1]->QueueChatCommand( MessageString );
                                            string CommandTest = Command.substr( 3 );
                                            string::size_type MsgTestStart = CommandTest.find( " " );

                                            if( MsgTestStart != string::npos )
                                            {
                                                string UserNameTest = CommandTest.substr( 0, MsgTestStart );
                                                string MsgTest = CommandTest.substr( MsgTestStart + 1 );
                                                MsgTestStart = MsgTest.find_first_not_of( " " );

                                                if( !UserNameTest.empty( ) && MsgTestStart != string::npos )
                                                    SendLocalChat( "Whispered to " + UserNameTest + ": " + MsgTest.substr( MsgTestStart ) );
                                                else
                                                    SendLocalChat( "Сообщение не отправлено, правильное применение команды: /w <username> <message>" );
                                            }
                                            else
                                                SendLocalChat( "Сообщение не отправлено, правильное применение команды: /w <username> <message>" );
                                        }
                                        else
                                            SendLocalChat( "You are not connected to battle.net." );
                                    }
                                }
                            }
                        }
                    }

                    if( Forward )
                    {
                        m_LocalPackets.push( new CCommandPacket( W3GS_HEADER_CONSTANT, Bytes[1], Data ) );
                        m_PacketBuffer.push( new CCommandPacket( W3GS_HEADER_CONSTANT, Bytes[1], Data ) );
                        m_TotalPacketsReceivedFromLocal++;
                    }

                    *RecvBuffer = RecvBuffer->substr( Length );
                    Bytes = BYTEARRAY( Bytes.begin( ) + Length, Bytes.end( ) );
                }
                else
                    return;
            }
            else
            {
                CONSOLE_Print( "[GPROXY] received invalid packet from local player (bad length)" );
                m_Exiting = true;
                return;
            }
        }
        else
        {
            CONSOLE_Print( "[GPROXY] received invalid packet from local player (bad header constant)" );
            m_Exiting = true;
            return;
        }
    }
}

void CUNM::ProcessLocalPackets( )
{
    if( !m_LocalSocket )
        return;

    while( !m_LocalPackets.empty( ) )
    {
        CCommandPacket *Packet = m_LocalPackets.front( );
        m_LocalPackets.pop( );
        BYTEARRAY Data = Packet->GetData( );

        if( Packet->GetPacketType( ) == W3GS_HEADER_CONSTANT )
        {
            if( Packet->GetID( ) == CGameProtocol::W3GS_REQJOIN )
            {
                if( Data.size( ) >= 20 )
                {
                    // parse

//                  uint32_t HostCounter = UTIL_ByteArrayToUInt32( Data, false, 4 );
                    uint32_t EntryKey = UTIL_ByteArrayToUInt32( Data, false, 8 );
                    unsigned char Unknown = Data[12];
                    uint16_t ListenPort = UTIL_ByteArrayToUInt16( Data, false, 13 );
                    uint32_t PeerKey = UTIL_ByteArrayToUInt32( Data, false, 15 );
                    BYTEARRAY Name = UTIL_ExtractCString( Data, 19 );
                    string NameString = string( Name.begin( ), Name.end( ) );
                    BYTEARRAY Remainder = BYTEARRAY( Data.begin( ) + Name.size( ) + 20, Data.end( ) );

                    if( ( Remainder.size( ) == 18 && m_LANWar3Version <= 28 ) || ( Remainder.size( ) == 19 && m_LANWar3Version >= 29 ) )
                    {
                        // lookup the game in the main list

                        bool GameFound = false;

                        for( vector<CBNET *> ::iterator j = m_BNETs.begin( ); j != m_BNETs.end( ); j++ )
                        {
                            if( !( *j )->m_SpamSubBot )
                            {
                                for( vector<CIncomingGameHost *>::iterator i = (*j)->m_GPGames.begin( ); i != (*j)->m_GPGames.end( ); i++ )
                                {
                                    if( ( *i )->GetUniqueGameID( ) == EntryKey )
                                    {
                                        m_UsedBnetID = ( *j )->GetHostCounterID( );
                                        CONSOLE_Print( "[GPROXY] local player requested game name [" + ( *i )->GetGameName( ) + "]" );

                                        if( NameString != m_BNETs[m_UsedBnetID-1]->GetName( ) )
                                        {
                                            CONSOLE_Print( "[GPROXY] using battle.net name [" + m_BNETs[m_UsedBnetID-1]->GetName( ) + "] instead of requested name [" + NameString + "]" );

                                            // создаем фейковое лобби, в котором оповещаем игрока что нельзя спруфить нейм
                                            // send slot info to the banned player

                                            vector<CGameSlot> Slots = m_Map->GetSlots( );
                                            m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_SLOTINFOJOIN( 2, m_LocalSocket->GetPort( ), m_LocalSocket->GetIP( ), Slots, PeerKey, m_Map->GetMapGameType( ) == GAMETYPE_CUSTOM ? 3 : 0, static_cast<unsigned char>( m_Map->GetMapNumPlayers( ) ), "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                            BYTEARRAY IP;
                                            IP.push_back( 0 );
                                            IP.push_back( 0 );
                                            IP.push_back( 0 );
                                            IP.push_back( 0 );
                                            m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_PLAYERINFO( 1, m_VirtualHostName, IP, IP, "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );

                                            // send a map check packet to the new player

                                            m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_MAPCHECK( ( *i )->GetMapPath( ), m_Map->GetMapSize( ), m_Map->GetMapInfo( ), m_Map->GetMapCRC( ), m_Map->GetMapSHA1( ), "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                            m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Не удалось зайти в игру [" + ( *i )->GetGameName( ) + "] - обнаружена подмена ника!", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                            m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Указанное вами имя в Warcraft 3 должно совпадать с тем, которое вы испольузете для входа на сервер, откуда была получена игра.", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                            string namelowerused = NameString;
                                            string namelowerrequired = m_BNETs[m_UsedBnetID-1]->GetName( );
                                            transform( namelowerused.begin( ), namelowerused.end( ), namelowerused.begin( ), static_cast<int(*)(int)>(tolower) );
                                            transform( namelowerrequired.begin( ), namelowerrequired.end( ), namelowerrequired.begin( ), static_cast<int(*)(int)>(tolower) );

                                            if( namelowerused == namelowerrequired )
                                                m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Вы указали [" + NameString + "], а необходимо [" + m_BNETs[m_UsedBnetID-1]->GetName( ) + "] (ник следует указывать с учетом регистра)", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                            else
                                                m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Вы указали [" + NameString + "], а необходимо [" + m_BNETs[m_UsedBnetID-1]->GetName( ) + "]", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );

                                            m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Это необходимо для того, чтобы бот, к игре которого вы подключаетесь, смог провести проверку на подмену ника, а также чтобы избежать десинхронизации из-за того что ваш используемый ник (который видите вы) не соответствует ожидаемому (тот который видят другие игроки)", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                            m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Данноe лобби будет автоматически закрыто через минуту.", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                            m_FakeLobbyTime = GetTime( );
                                            m_FakeLobby = true;
                                            GameFound = true;
                                            m_CurrentGameID = EntryKey;
                                            break;
                                        }

                                        CONSOLE_Print( "[GPROXY] connecting to remote server [" + ( *i )->GetIPString( ) + "] on port " + UTIL_ToString( ( *i )->GetPort( ) ) );
                                        m_RemoteServerIP = ( *i )->GetIPString( );
                                        m_RemoteServerPort = ( *i )->GetPort( );
                                        m_RemoteSocket->Reset( );
                                        m_RemoteSocket->SetNoDelay( true );
                                        m_RemoteSocket->Connect( string( ), m_RemoteServerIP, m_RemoteServerPort, "[GPROXY: " + ( *i )->GetGameName( ) + "]" );
                                        m_LastConnectionAttemptTime = GetTime( );
                                        m_GameIsReliable = ( *i )->GetGProxy( );
                                        m_GameStarted = false;

                                        // rewrite packet

                                        BYTEARRAY DataRewritten;
                                        DataRewritten.push_back( W3GS_HEADER_CONSTANT );
                                        DataRewritten.push_back( Packet->GetID( ) );
                                        DataRewritten.push_back( 0 );
                                        DataRewritten.push_back( 0 );
                                        UTIL_AppendByteArray( DataRewritten, ( *i )->GetHostCounter( ), false );
                                        UTIL_AppendByteArray( DataRewritten, (uint32_t)0, false );
                                        DataRewritten.push_back( Unknown );
                                        UTIL_AppendByteArray( DataRewritten, ListenPort, false );
                                        UTIL_AppendByteArray( DataRewritten, PeerKey, false );
                                        UTIL_AppendByteArray( DataRewritten, m_BNETs[m_UsedBnetID-1]->GetName( ) );
                                        UTIL_AppendByteArrayFast( DataRewritten, Remainder );
                                        BYTEARRAY LengthBytes;
                                        LengthBytes = UTIL_CreateByteArray( (uint16_t)DataRewritten.size( ), false );
                                        DataRewritten[2] = LengthBytes[0];
                                        DataRewritten[3] = LengthBytes[1];
                                        Data = DataRewritten;

                                        // tell battle.net we're joining a game (for automatic spoof checking)

                                        m_BNETs[m_UsedBnetID-1]->QueueJoinGame( ( *i )->GetGameName( ) );

                                        // save the hostname for later (for manual spoof checking)

                                        m_JoinedName = NameString;
                                        m_HostName = ( *i )->GetHostName( );
                                        GameFound = true;
                                        m_CurrentGameID = EntryKey;
                                        break;
                                    }
                                }
                            }
                        }

                        if( !GameFound )
                        {
                            for( vector<CIrinaGame *>::iterator i = m_IrinaServer->m_GPGames.begin( ); i != m_IrinaServer->m_GPGames.end( ); i++ )
                            {
                                if( ( *i )->GetUniqueGameID( ) == EntryKey )
                                {
                                    m_UsedBnetID = 0;
                                    CONSOLE_Print( "[GPROXY] local player requested game name [" + ( *i )->GetGameName( ) + "]" );

                                    if( NameString != m_IrinaServer->m_User->m_ConnectorName )
                                    {
                                        CONSOLE_Print( "[GPROXY] using battle.net name [" + m_IrinaServer->m_User->m_ConnectorName + "] instead of requested name [" + NameString + "]" );

                                        // создаем фейковое лобби, в котором оповещаем игрока что нельзя спруфить нейм
                                        // send slot info to the banned player

                                        vector<CGameSlot> Slots = m_Map->GetSlots( );
                                        m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_SLOTINFOJOIN( 2, m_LocalSocket->GetPort( ), m_LocalSocket->GetIP( ), Slots, PeerKey, m_Map->GetMapGameType( ) == GAMETYPE_CUSTOM ? 3 : 0, static_cast<unsigned char>( m_Map->GetMapNumPlayers( ) ), "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                        BYTEARRAY IP;
                                        IP.push_back( 0 );
                                        IP.push_back( 0 );
                                        IP.push_back( 0 );
                                        IP.push_back( 0 );
                                        m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_PLAYERINFO( 1, m_VirtualHostName, IP, IP, "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );

                                        // send a map check packet to the new player

                                        m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_MAPCHECK( ( *i )->m_GameData->GetMapPath( ), m_Map->GetMapSize( ), m_Map->GetMapInfo( ), m_Map->GetMapCRC( ), m_Map->GetMapSHA1( ), "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                        m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Не удалось зайти в игру [" + ( *i )->GetGameName( ) + "] - обнаружена подмена ника!", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                        m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Указанное вами имя в Warcraft 3 должно совпадать с тем, которое вы испольузете для входа на сервер, откуда была получена игра.", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                        string namelowerused = NameString;
                                        string namelowerrequired = m_IrinaServer->m_User->m_ConnectorName;
                                        transform( namelowerused.begin( ), namelowerused.end( ), namelowerused.begin( ), static_cast<int(*)(int)>(tolower) );
                                        transform( namelowerrequired.begin( ), namelowerrequired.end( ), namelowerrequired.begin( ), static_cast<int(*)(int)>(tolower) );

                                        if( namelowerused == namelowerrequired )
                                            m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Вы указали [" + NameString + "], а необходимо [" + m_IrinaServer->m_User->m_ConnectorName + "] (ник следует указывать с учетом регистра)", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                        else
                                            m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Вы указали [" + NameString + "], а необходимо [" + m_IrinaServer->m_User->m_ConnectorName + "]", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );

                                        m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Это необходимо для того, чтобы бот, к игре которого вы подключаетесь, смог провести проверку на подмену ника, а также чтобы избежать десинхронизации из-за того что ваш используемый ник (который видите вы) не соответствует ожидаемому (тот который видят другие игроки)", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                        m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( 1, UTIL_CreateByteArray( 2 ), 16, BYTEARRAY( ), "Данноe лобби будет автоматически закрыто через минуту.", "[GAME: " + ( *i )->GetGameName( ) + "]", 0 ) );
                                        m_FakeLobbyTime = GetTime( );
                                        m_FakeLobby = true;
                                        GameFound = true;
                                        m_CurrentGameID = EntryKey;
                                        break;
                                    }

                                    CONSOLE_Print( "[GPROXY] connecting to remote server [" + ( *i )->m_GameData->m_GameIP + "] on port " + UTIL_ToString( ( *i )->m_GameData->m_GamePort ) );
                                    m_RemoteServerIP = ( *i )->m_GameData->m_GameIP;
                                    m_RemoteServerPort = ( *i )->m_GameData->m_GamePort;
                                    m_RemoteSocket->Reset( );
                                    m_RemoteSocket->SetNoDelay( true );
                                    m_RemoteSocket->Connect( string( ), m_RemoteServerIP, m_RemoteServerPort, "[GPROXY: " + ( *i )->GetGameName( ) + "]" );
                                    m_LastConnectionAttemptTime = GetTime( );
                                    m_GameIsReliable = true;
                                    m_GameStarted = false;

                                    // rewrite packet

                                    BYTEARRAY DataRewritten;
                                    DataRewritten.push_back( W3GS_HEADER_CONSTANT );
                                    DataRewritten.push_back( Packet->GetID( ) );
                                    DataRewritten.push_back( 0 );
                                    DataRewritten.push_back( 0 );
                                    UTIL_AppendByteArray( DataRewritten, ( *i )->GetIrinaID( ), false );
                                    UTIL_AppendByteArray( DataRewritten, (uint32_t)0, false );
                                    DataRewritten.push_back( Unknown );
                                    UTIL_AppendByteArray( DataRewritten, ListenPort, false );
                                    UTIL_AppendByteArray( DataRewritten, PeerKey, false );
                                    UTIL_AppendByteArray( DataRewritten, m_IrinaServer->m_User->m_ConnectorName );
                                    UTIL_AppendByteArrayFast( DataRewritten, Remainder );
                                    BYTEARRAY LengthBytes;
                                    LengthBytes = UTIL_CreateByteArray( (uint16_t)DataRewritten.size( ), false );
                                    DataRewritten[2] = LengthBytes[0];
                                    DataRewritten[3] = LengthBytes[1];
                                    m_CurrentGameID = EntryKey;

                                    // проходим спуфчек на иринаботе

                                    Data = m_IrinaServer->SEND_GPSC_SPOOFCHECK_IRINA( );
                                    UTIL_AppendByteArrayFast( Data, DataRewritten );

                                    // save the hostname for later (for manual spoof checking)

                                    m_JoinedName = NameString;
                                    m_HostName = "irinabot.ru";
                                    GameFound = true;
                                    break;
                                }
                            }

                            if( !GameFound )
                            {
                                CONSOLE_Print( "[GPROXY] local player requested unknown game (expired?)" );
                                m_LocalSocket->Disconnect( );
                            }
                        }
                    }
                    else
                        CONSOLE_Print( "[GPROXY] received invalid join request from local player (invalid remainder)" );
                }
                else
                    CONSOLE_Print( "[GPROXY] received invalid join request from local player (too short)" );
            }
            else if( Packet->GetID( ) == CGameProtocol::W3GS_LEAVEGAME )
            {
                m_FakeLobby = false;
                m_LeaveGameSent = true;
                m_LocalSocket->Disconnect( );
                m_UsedBnetID = 0;
            }
            else if( Packet->GetID( ) == CGameProtocol::W3GS_CHAT_TO_HOST )
            {
                // handled in ExtractLocalPackets (yes, it's ugly)
            }
        }

        // warning: do not forward any data if we are not synchronized (e.g. we are reconnecting and resynchronizing)
        // any data not forwarded here will be cached in the packet buffer and sent later so all is well

        if( m_RemoteSocket && m_Synchronized )
            m_RemoteSocket->PutBytes( Data );

        delete Packet;
    }
}

void CUNM::ExtractRemotePackets( )
{
    string *RecvBuffer = m_RemoteSocket->GetBytes( );
    BYTEARRAY Bytes = UTIL_CreateByteArray( (unsigned char *)RecvBuffer->c_str( ), RecvBuffer->size( ) );

    // a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

    while( Bytes.size( ) >= 4 )
    {
        if( Bytes[0] == W3GS_HEADER_CONSTANT || Bytes[0] == GPS_HEADER_CONSTANT )
        {
            // bytes 2 and 3 contain the length of the packet

            uint16_t Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

            if( Length >= 4 )
            {
                if( Bytes.size( ) >= Length )
                {
                    m_RemotePackets.push( new CCommandPacket( Bytes[0], Bytes[1], BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) );

                    if( Bytes[0] == W3GS_HEADER_CONSTANT )
                        m_TotalPacketsReceivedFromRemote++;

                    *RecvBuffer = RecvBuffer->substr( Length );
                    Bytes = BYTEARRAY( Bytes.begin( ) + Length, Bytes.end( ) );
                }
                else
                    return;
            }
            else
            {
                CONSOLE_Print( "[GPROXY] received invalid packet from remote server (bad length)" );
                m_Exiting = true;
                return;
            }
        }
        else
        {
            CONSOLE_Print( "[GPROXY] received invalid packet from remote server (bad header constant)" );
            m_Exiting = true;
            return;
        }
    }
}

void CUNM::ProcessRemotePackets( )
{
    if( !m_LocalSocket || !m_RemoteSocket )
        return;

    while( !m_RemotePackets.empty( ) )
    {
        CCommandPacket *Packet = m_RemotePackets.front( );
        m_RemotePackets.pop( );

        if( Packet->GetPacketType( ) == W3GS_HEADER_CONSTANT )
        {
            if( Packet->GetID( ) == CGameProtocol::W3GS_SLOTINFOJOIN )
            {
                BYTEARRAY Data = Packet->GetData( );

                if( Data.size( ) >= 6 )
                {
                    uint16_t SlotInfoSize = UTIL_ByteArrayToUInt16( Data, false, 4 );

                    if( Data.size( ) >= 7 + SlotInfoSize )
                        m_ChatPID = Data[6 + SlotInfoSize];
                }

                // send a GPS_INIT packet
                // if the server doesn't recognize it (e.g. it isn't GHost++) we should be kicked

                CONSOLE_Print( "[GPROXY] join request accepted by remote server" );

                if( m_GameIsReliable )
                {
                    CONSOLE_Print( "[GPROXY] detected reliable game, starting GPS handshake" );
                    m_RemoteSocket->PutBytes( m_GPSProtocol->SEND_GPSC_INIT( 1 ) );
                }
                else
                    CONSOLE_Print( "[GPROXY] detected standard game, disconnect protection disabled" );
            }
            else if( Packet->GetID( ) == CGameProtocol::W3GS_COUNTDOWN_END )
            {
                if( m_GameIsReliable && m_GPPort > 0 )
                    CONSOLE_Print( "[GPROXY] game started, disconnect protection enabled" );
                else
                {
                    if( m_GameIsReliable )
                        CONSOLE_Print( "[GPROXY] game started but GPS handshake not complete, disconnect protection disabled" );
                    else
                        CONSOLE_Print( "[GPROXY] game started" );
                }

                m_GameStarted = true;
            }
            else if( Packet->GetID( ) == CGameProtocol::W3GS_INCOMING_ACTION )
            {
                if( m_GameIsReliable )
                {
                    // we received a game update which means we can reset the number of empty actions we have to work with
                    // we also must send any remaining empty actions now
                    // note: the lag screen can't be up right now otherwise the server made a big mistake, so we don't need to check for it

                    BYTEARRAY EmptyAction;
                    EmptyAction.push_back( 0xF7 );
                    EmptyAction.push_back( 0x0C );
                    EmptyAction.push_back( 0x06 );
                    EmptyAction.push_back( 0x00 );
                    EmptyAction.push_back( 0x00 );
                    EmptyAction.push_back( 0x00 );

                    for( unsigned char i = m_NumEmptyActionsUsed; i < m_NumEmptyActions; i++ )
                        m_LocalSocket->PutBytes( EmptyAction );

                    m_NumEmptyActionsUsed = 0;
                }

                m_ActionReceived = true;
                m_LastActionTime = GetTime( );
            }
            else if( Packet->GetID( ) == CGameProtocol::W3GS_START_LAG )
            {
                if( m_GameIsReliable )
                {
                    BYTEARRAY Data = Packet->GetData( );

                    if( Data.size( ) >= 5 )
                    {
                        unsigned char NumLaggers = Data[4];

                        if( Data.size( ) == 5 + NumLaggers * 5 )
                        {
                            for( unsigned char i = 0; i < NumLaggers; i++ )
                            {
                                bool LaggerFound = false;

                                for( vector<unsigned char>::iterator j = m_Laggers.begin( ); j != m_Laggers.end( ); j++ )
                                {
                                    if( *j == Data[5 + i * 5] )
                                        LaggerFound = true;
                                }

                                if( LaggerFound )
                                    CONSOLE_Print( "[GPROXY] warning - received start_lag on known lagger" );
                                else
                                    m_Laggers.push_back( Data[5 + i * 5] );
                            }
                        }
                        else
                            CONSOLE_Print( "[GPROXY] warning - unhandled start_lag (2)" );
                    }
                    else
                        CONSOLE_Print( "[GPROXY] warning - unhandled start_lag (1)" );
                }
            }
            else if( Packet->GetID( ) == CGameProtocol::W3GS_STOP_LAG )
            {
                if( m_GameIsReliable )
                {
                    BYTEARRAY Data = Packet->GetData( );

                    if( Data.size( ) == 9 )
                    {
                        bool LaggerFound = false;

                        for( vector<unsigned char>::iterator i = m_Laggers.begin( ); i != m_Laggers.end( ); )
                        {
                            if( *i == Data[4] )
                            {
                                i = m_Laggers.erase( i );
                                LaggerFound = true;
                            }
                            else
                                i++;
                        }

                        if( !LaggerFound )
                            CONSOLE_Print( "[GPROXY] warning - received stop_lag on unknown lagger" );
                    }
                    else
                        CONSOLE_Print( "[GPROXY] warning - unhandled stop_lag" );
                }
            }
            else if( Packet->GetID( ) == CGameProtocol::W3GS_INCOMING_ACTION2 )
            {
                if( m_GameIsReliable )
                {
                    // we received a fractured game update which means we cannot use any empty actions until we receive the subsequent game update
                    // we also must send any remaining empty actions now
                    // note: this means if we get disconnected right now we can't use any of our buffer time, which would be very unlucky
                    // it still gives us 60 seconds total to reconnect though
                    // note: the lag screen can't be up right now otherwise the server made a big mistake, so we don't need to check for it

                    BYTEARRAY EmptyAction;
                    EmptyAction.push_back( 0xF7 );
                    EmptyAction.push_back( 0x0C );
                    EmptyAction.push_back( 0x06 );
                    EmptyAction.push_back( 0x00 );
                    EmptyAction.push_back( 0x00 );
                    EmptyAction.push_back( 0x00 );

                    for( unsigned char i = m_NumEmptyActionsUsed; i < m_NumEmptyActions; i++ )
                        m_LocalSocket->PutBytes( EmptyAction );

                    m_NumEmptyActionsUsed = m_NumEmptyActions;
                }
            }

            // forward the data

            m_LocalSocket->PutBytes( Packet->GetData( ) );

            // we have to wait until now to send the status message since otherwise the slotinfojoin itself wouldn't have been forwarded

            if( Packet->GetID( ) == CGameProtocol::W3GS_SLOTINFOJOIN )
            {
                if( m_UsedBnetID == 0 )
                    SendLocalChat( "Ошибка! Battle.net соединение не определенно." );
                else if( m_JoinedName != m_BNETs[m_UsedBnetID-1]->GetName( ) )
                    SendLocalChat( "Using battle.net name \"" + m_BNETs[m_UsedBnetID-1]->GetName( ) + "\" instead of LAN name \"" + m_JoinedName + "\"." );

                if( m_GameIsReliable )
                    SendLocalChat( "This is a reliable game. Requesting GProxy++ disconnect protection from server..." );
                else
                    SendLocalChat( "This is an unreliable game. GProxy++ disconnect protection is disabled." );
            }
        }
        else if( Packet->GetPacketType( ) == GPS_HEADER_CONSTANT )
        {
            if( m_GameIsReliable )
            {
                BYTEARRAY Data = Packet->GetData( );

                if( Packet->GetID( ) == CGPSProtocol::GPS_INIT && Data.size( ) == 12 )
                {
                    m_GPPort = UTIL_ByteArrayToUInt16( Data, false, 4 );
                    m_PID = Data[6];
                    m_ReconnectKey = UTIL_ByteArrayToUInt32( Data, false, 7 );
                    m_NumEmptyActions = Data[11];
                    SendLocalChat( "GProxy++ disconnect protection is ready (" + UTIL_ToString( ( m_NumEmptyActions + 1 ) * 60 ) + " second buffer)." );
                    CONSOLE_Print( "[GPROXY] handshake complete, disconnect protection ready (" + UTIL_ToString( ( m_NumEmptyActions + 1 ) * 60 ) + " second buffer)" );
                }
                else if( Packet->GetID( ) == CGPSProtocol::GPS_RECONNECT && Data.size( ) == 8 )
                {
                    uint32_t LastPacket = UTIL_ByteArrayToUInt32( Data, false, 4 );
                    uint32_t PacketsAlreadyUnqueued = m_TotalPacketsReceivedFromLocal - m_PacketBuffer.size( );

                    if( LastPacket > PacketsAlreadyUnqueued )
                    {
                        uint32_t PacketsToUnqueue = LastPacket - PacketsAlreadyUnqueued;

                        if( PacketsToUnqueue > m_PacketBuffer.size( ) )
                        {
                            CONSOLE_Print( "[GPROXY] received GPS_RECONNECT with last packet > total packets sent" );
                            PacketsToUnqueue = m_PacketBuffer.size( );
                        }

                        while( PacketsToUnqueue > 0 )
                        {
                            delete m_PacketBuffer.front( );
                            m_PacketBuffer.pop( );
                            PacketsToUnqueue--;
                        }
                    }

                    // send remaining packets from buffer, preserve buffer
                    // note: any packets in m_LocalPackets are still sitting at the end of this buffer because they haven't been processed yet
                    // therefore we must check for duplicates otherwise we might (will) cause a desync

                    queue<CCommandPacket *> TempBuffer;

                    while( !m_PacketBuffer.empty( ) )
                    {
                        if( m_PacketBuffer.size( ) > m_LocalPackets.size( ) )
                            m_RemoteSocket->PutBytes( m_PacketBuffer.front( )->GetData( ) );

                        TempBuffer.push( m_PacketBuffer.front( ) );
                        m_PacketBuffer.pop( );
                    }

                    m_PacketBuffer = TempBuffer;

                    // we can resume forwarding local packets again
                    // doing so prior to this point could result in an out-of-order stream which would probably cause a desync

                    m_Synchronized = true;
                }
                else if( Packet->GetID( ) == CGPSProtocol::GPS_ACK && Data.size( ) == 8 )
                {
                    uint32_t LastPacket = UTIL_ByteArrayToUInt32( Data, false, 4 );
                    uint32_t PacketsAlreadyUnqueued = m_TotalPacketsReceivedFromLocal - m_PacketBuffer.size( );

                    if( LastPacket > PacketsAlreadyUnqueued )
                    {
                        uint32_t PacketsToUnqueue = LastPacket - PacketsAlreadyUnqueued;

                        if( PacketsToUnqueue > m_PacketBuffer.size( ) )
                        {
                            CONSOLE_Print( "[GPROXY] received GPS_ACK with last packet > total packets sent" );
                            PacketsToUnqueue = m_PacketBuffer.size( );
                        }

                        while( PacketsToUnqueue > 0 )
                        {
                            delete m_PacketBuffer.front( );
                            m_PacketBuffer.pop( );
                            PacketsToUnqueue--;
                        }
                    }
                }
                else if( Packet->GetID( ) == CGPSProtocol::GPS_REJECT && Data.size( ) == 8 )
                {
                    uint32_t Reason = UTIL_ByteArrayToUInt32( Data, false, 4 );

                    if( Reason == REJECTGPS_INVALID )
                        CONSOLE_Print( "[GPROXY] rejected by remote server: invalid data" );
                    else if( Reason == REJECTGPS_NOTFOUND )
                        CONSOLE_Print( "[GPROXY] rejected by remote server: player not found in any running games" );

                    m_LocalSocket->Disconnect( );
                }
            }
        }

        delete Packet;
    }
}

void CUNM::SendLocalChat( string message )
{
    if( m_LocalSocket )
    {
        if( m_GameStarted )
        {
            if( message.size( ) > 127 )
                message = message.substr( 0, 127 );

            m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( m_ChatPID, UTIL_CreateByteArray( m_ChatPID ), 32, UTIL_CreateByteArray( (uint32_t)0, false ), message, "[GPROXY]", 0 ) );
        }
        else
        {
            if( message.size( ) > 254 )
                message = message.substr( 0, 254 );

            m_LocalSocket->PutBytes( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( m_ChatPID, UTIL_CreateByteArray( m_ChatPID ), 16, BYTEARRAY( ), message, "[GPROXY]", 0 ) );
        }
    }
}

void CUNM::SendEmptyAction( )
{
    // we can't send any empty actions while the lag screen is up
    // so we keep track of who the lag screen is currently showing (if anyone) and we tear it down, send the empty action, and put it back up

    for( vector<unsigned char>::iterator i = m_Laggers.begin( ); i != m_Laggers.end( ); i++ )
    {
        BYTEARRAY StopLag;
        StopLag.push_back( 0xF7 );
        StopLag.push_back( 0x11 );
        StopLag.push_back( 0x09 );
        StopLag.push_back( 0 );
        StopLag.push_back( *i );
        UTIL_AppendByteArray( StopLag, (uint32_t)60000, false );
        m_LocalSocket->PutBytes( StopLag );
    }

    BYTEARRAY EmptyAction;
    EmptyAction.push_back( 0xF7 );
    EmptyAction.push_back( 0x0C );
    EmptyAction.push_back( 0x06 );
    EmptyAction.push_back( 0x00 );
    EmptyAction.push_back( 0x00 );
    EmptyAction.push_back( 0x00 );
    m_LocalSocket->PutBytes( EmptyAction );

    if( !m_Laggers.empty( ) )
    {
        BYTEARRAY StartLag;
        StartLag.push_back( 0xF7 );
        StartLag.push_back( 0x10 );
        StartLag.push_back( 0 );
        StartLag.push_back( 0 );
        StartLag.push_back( m_Laggers.size( ) );

        for( vector<unsigned char>::iterator i = m_Laggers.begin( ); i != m_Laggers.end( ); i++ )
        {
            // using a lag time of 60000 ms means the counter will start at zero
            // hopefully warcraft 3 doesn't care about wild variations in the lag time in subsequent packets

            StartLag.push_back( *i );
            UTIL_AppendByteArray( StartLag, (uint32_t)60000, false );
        }

        BYTEARRAY LengthBytes;
        LengthBytes = UTIL_CreateByteArray( (uint16_t)StartLag.size( ), false );
        StartLag[2] = LengthBytes[0];
        StartLag[3] = LengthBytes[1];
        m_LocalSocket->PutBytes( StartLag );
    }
}

void Widget::QListFillingProcess( )
{
    LabelsList.push_back( ui->designeGUILabel );
    WidgetsList.push_back( ui->designeGUICombo );
    LabelsList.push_back( ui->ShowPesettingWindowLabel );
    WidgetsList.push_back( ui->ShowPesettingWindowCombo );
    LabelsList.push_back( ui->LanguageFileLabel );
    WidgetsList.push_back( ui->LanguageFileLine );
    LabelsList.push_back( ui->Warcraft3PathLabel );
    WidgetsList.push_back( ui->Warcraft3PathLine );
    LabelsList.push_back( ui->MapPathLabel );
    WidgetsList.push_back( ui->MapPathLine );
    LabelsList.push_back( ui->patch23Label );
    WidgetsList.push_back( ui->patch23Combo );
    LabelsList.push_back( ui->LANWar3VersionLabel );
    WidgetsList.push_back( ui->LANWar3VersionSpin );
    LabelsList.push_back( ui->BroadcastPortLabel );
    WidgetsList.push_back( ui->BroadcastPortSpin );
    LabelsList.push_back( ui->EnableLoggingToGUILabel );
    WidgetsList.push_back( ui->EnableLoggingToGUICombo );
    LabelsList.push_back( ui->LogReductionLabel );
    WidgetsList.push_back( ui->LogReductionCombo );
    LabelsList.push_back( ui->MaxLogSizeLabel );
    WidgetsList.push_back( ui->MaxLogSizeSpin );
    LabelsList.push_back( ui->MaxLogTimeLabel );
    WidgetsList.push_back( ui->MaxLogTimeSpin );
    LabelsList.push_back( ui->MinLengthGNLabel );
    WidgetsList.push_back( ui->MinLengthGNSpin );
    LabelsList.push_back( ui->SlotInfoInGameNameLabel );
    WidgetsList.push_back( ui->SlotInfoInGameNameSpin );
    LabelsList.push_back( ui->patch21Label );
    WidgetsList.push_back( ui->patch21Combo );
    LabelsList.push_back( ui->broadcastinlanLabel );
    WidgetsList.push_back( ui->broadcastinlanCombo );
    LabelsList.push_back( ui->ShowRealSlotCountLabel );
    WidgetsList.push_back( ui->ShowRealSlotCountCombo );
    LabelsList.push_back( ui->BroadcastHostNameLabel );
    WidgetsList.push_back( ui->BroadcastHostNameCombo );
    LabelsList.push_back( ui->BroadcastHostNameCustomLabel );
    WidgetsList.push_back( ui->BroadcastHostNameCustomLine );
    LabelsList.push_back( ui->HostPortLabel );
    WidgetsList.push_back( ui->HostPortSpin );
    LabelsList.push_back( ui->DefaultMapLabel );
    WidgetsList.push_back( ui->DefaultMapLine );
    LabelsList.push_back( ui->BindAddressLabel );
    WidgetsList.push_back( ui->BindAddressLine );
    LabelsList.push_back( ui->HideIPAddressesLabel );
    WidgetsList.push_back( ui->HideIPAddressesCombo );
    LabelsList.push_back( ui->MapCFGPathLabel );
    WidgetsList.push_back( ui->MapCFGPathLine );
    LabelsList.push_back( ui->SaveGamePathLabel );
    WidgetsList.push_back( ui->SaveGamePathLine );
    LabelsList.push_back( ui->ReplayPathLabel );
    WidgetsList.push_back( ui->ReplayPathLine );
    LabelsList.push_back( ui->SaveReplaysLabel );
    WidgetsList.push_back( ui->SaveReplaysCombo );
    LabelsList.push_back( ui->ReplayBuildNumberLabel );
    WidgetsList.push_back( ui->ReplayBuildNumberSpin );
    LabelsList.push_back( ui->ReplayWar3VersionLabel );
    WidgetsList.push_back( ui->ReplayWar3VersionSpin );
    LabelsList.push_back( ui->ReplaysByNameLabel );
    WidgetsList.push_back( ui->ReplaysByNameCombo );
    LabelsList.push_back( ui->InvalidTriggersGameLabel );
    WidgetsList.push_back( ui->InvalidTriggersGameLine );
    LabelsList.push_back( ui->InvalidReplayCharsLabel );
    WidgetsList.push_back( ui->InvalidReplayCharsLine );
    LabelsList.push_back( ui->MaxHostCounterLabel );
    WidgetsList.push_back( ui->MaxHostCounterSpin );
    LabelsList.push_back( ui->RehostCharLabel );
    WidgetsList.push_back( ui->RehostCharLine );
    LabelsList.push_back( ui->GameNameCharHideLabel );
    WidgetsList.push_back( ui->GameNameCharHideCombo );
    LabelsList.push_back( ui->DefaultGameNameCharLabel );
    WidgetsList.push_back( ui->DefaultGameNameCharLine );
    LabelsList.push_back( ui->ResetGameNameCharLabel );
    WidgetsList.push_back( ui->ResetGameNameCharCombo );
    LabelsList.push_back( ui->AppleIconLabel );
    WidgetsList.push_back( ui->AppleIconCombo );
    LabelsList.push_back( ui->FixGameNameForIccupLabel );
    WidgetsList.push_back( ui->FixGameNameForIccupCombo );
    LabelsList.push_back( ui->GameStartedMessagePrintingLabel );
    WidgetsList.push_back( ui->GameStartedMessagePrintingCombo );
    LabelsList.push_back( ui->PrefixNameLabel );
    WidgetsList.push_back( ui->PrefixNameCombo );
    LabelsList.push_back( ui->MaxGamesLabel );
    WidgetsList.push_back( ui->MaxGamesSpin );
    LabelsList.push_back( ui->IPBlackListFileLabel );
    WidgetsList.push_back( ui->IPBlackListFileLine );
    LabelsList.push_back( ui->gamestateinhouseLabel );
    WidgetsList.push_back( ui->gamestateinhouseSpin );
    LabelsList.push_back( ui->autohclfromgamenameLabel );
    WidgetsList.push_back( ui->autohclfromgamenameCombo );
    LabelsList.push_back( ui->forceautohclindotaLabel );
    WidgetsList.push_back( ui->forceautohclindotaCombo );
    LabelsList.push_back( ui->SpoofCheckIfGameNameIsIndefiniteLabel );
    WidgetsList.push_back( ui->SpoofCheckIfGameNameIsIndefiniteCombo );
    LabelsList.push_back( ui->HoldPlayersForRMKLabel );
    WidgetsList.push_back( ui->HoldPlayersForRMKCombo );
    LabelsList.push_back( ui->ForceLoadInGameLabel );
    WidgetsList.push_back( ui->ForceLoadInGameCombo );
    LabelsList.push_back( ui->MsgAutoCorrectModeLabel );
    WidgetsList.push_back( ui->MsgAutoCorrectModeCombo );
    LabelsList.push_back( ui->SpoofChecksLabel );
    WidgetsList.push_back( ui->SpoofChecksCombo );
    LabelsList.push_back( ui->CheckMultipleIPUsageLabel );
    WidgetsList.push_back( ui->CheckMultipleIPUsageCombo );
    LabelsList.push_back( ui->MaxLatencyLabel );
    WidgetsList.push_back( ui->MaxLatencySpin );
    LabelsList.push_back( ui->MinLatencyLabel );
    WidgetsList.push_back( ui->MinLatencySpin );
    LabelsList.push_back( ui->LatencyLabel );
    WidgetsList.push_back( ui->LatencySpin );
    LabelsList.push_back( ui->UseDynamicLatencyLabel );
    WidgetsList.push_back( ui->UseDynamicLatencyCombo );
    LabelsList.push_back( ui->DynamicLatencyHighestAllowedLabel );
    WidgetsList.push_back( ui->DynamicLatencyHighestAllowedSpin );
    LabelsList.push_back( ui->DynamicLatencyLowestAllowedLabel );
    WidgetsList.push_back( ui->DynamicLatencyLowestAllowedSpin );
    LabelsList.push_back( ui->DynamicLatencyRefreshRateLabel );
    WidgetsList.push_back( ui->DynamicLatencyRefreshRateSpin );
    LabelsList.push_back( ui->DynamicLatencyConsolePrintLabel );
    WidgetsList.push_back( ui->DynamicLatencyConsolePrintSpin );
    LabelsList.push_back( ui->DynamicLatencyPercentPingMaxLabel );
    WidgetsList.push_back( ui->DynamicLatencyPercentPingMaxSpin );
    LabelsList.push_back( ui->DynamicLatencyDifferencePingMaxLabel );
    WidgetsList.push_back( ui->DynamicLatencyDifferencePingMaxSpin );
    LabelsList.push_back( ui->DynamicLatencyMaxSyncLabel );
    WidgetsList.push_back( ui->DynamicLatencyMaxSyncSpin );
    LabelsList.push_back( ui->DynamicLatencyAddIfMaxSyncLabel );
    WidgetsList.push_back( ui->DynamicLatencyAddIfMaxSyncSpin );
    LabelsList.push_back( ui->DynamicLatencyAddIfLagLabel );
    WidgetsList.push_back( ui->DynamicLatencyAddIfLagSpin );
    LabelsList.push_back( ui->DynamicLatencyAddIfBotLagLabel );
    WidgetsList.push_back( ui->DynamicLatencyAddIfBotLagSpin );
    LabelsList.push_back( ui->MaxSyncLimitLabel );
    WidgetsList.push_back( ui->MaxSyncLimitSpin );
    LabelsList.push_back( ui->MinSyncLimitLabel );
    WidgetsList.push_back( ui->MinSyncLimitSpin );
    LabelsList.push_back( ui->SyncLimitLabel );
    WidgetsList.push_back( ui->SyncLimitSpin );
    LabelsList.push_back( ui->AutoSaveLabel );
    WidgetsList.push_back( ui->AutoSaveCombo );
    LabelsList.push_back( ui->AutoKickPingLabel );
    WidgetsList.push_back( ui->AutoKickPingSpin );
    LabelsList.push_back( ui->LCPingsLabel );
    WidgetsList.push_back( ui->LCPingsCombo );
    LabelsList.push_back( ui->PingDuringDownloadsLabel );
    WidgetsList.push_back( ui->PingDuringDownloadsCombo );
    LabelsList.push_back( ui->AllowDownloadsLabel );
    WidgetsList.push_back( ui->AllowDownloadsCombo );
    LabelsList.push_back( ui->maxdownloadersLabel );
    WidgetsList.push_back( ui->maxdownloadersCombo );
    LabelsList.push_back( ui->totaldownloadspeedLabel );
    WidgetsList.push_back( ui->totaldownloadspeedSpin );
    LabelsList.push_back( ui->clientdownloadspeedLabel );
    WidgetsList.push_back( ui->clientdownloadspeedSpin );
    LabelsList.push_back( ui->PlaceAdminsHigherLabel );
    WidgetsList.push_back( ui->PlaceAdminsHigherCombo );
    LabelsList.push_back( ui->ShuffleSlotsOnStartLabel );
    WidgetsList.push_back( ui->ShuffleSlotsOnStartCombo );
    LabelsList.push_back( ui->FakePlayersLobbyLabel );
    WidgetsList.push_back( ui->FakePlayersLobbyCombo );
    LabelsList.push_back( ui->MoreFPsLobbyLabel );
    WidgetsList.push_back( ui->MoreFPsLobbyCombo );
    LabelsList.push_back( ui->NormalCountdownLabel );
    WidgetsList.push_back( ui->NormalCountdownCombo );
    LabelsList.push_back( ui->CountDownMethodLabel );
    WidgetsList.push_back( ui->CountDownMethodCombo );
    LabelsList.push_back( ui->CountDownStartCounterLabel );
    WidgetsList.push_back( ui->CountDownStartCounterSpin );
    LabelsList.push_back( ui->CountDownStartForceCounterLabel );
    WidgetsList.push_back( ui->CountDownStartForceCounterSpin );
    LabelsList.push_back( ui->VoteStartAllowedLabel );
    WidgetsList.push_back( ui->VoteStartAllowedCombo );
    LabelsList.push_back( ui->VoteStartPercentalVotingLabel );
    WidgetsList.push_back( ui->VoteStartPercentalVotingCombo );
    LabelsList.push_back( ui->CancelVoteStartIfLeaveLabel );
    WidgetsList.push_back( ui->CancelVoteStartIfLeaveCombo );
    LabelsList.push_back( ui->StartAfterLeaveIfEnoughVotesLabel );
    WidgetsList.push_back( ui->StartAfterLeaveIfEnoughVotesCombo );
    LabelsList.push_back( ui->VoteStartMinPlayersLabel );
    WidgetsList.push_back( ui->VoteStartMinPlayersCombo );
    LabelsList.push_back( ui->VoteStartPercentLabel );
    WidgetsList.push_back( ui->VoteStartPercentSpin );
    LabelsList.push_back( ui->VoteStartDurationLabel );
    WidgetsList.push_back( ui->VoteStartDurationSpin );
    LabelsList.push_back( ui->VoteKickAllowedLabel );
    WidgetsList.push_back( ui->VoteKickAllowedCombo );
    LabelsList.push_back( ui->VoteKickPercentageLabel );
    WidgetsList.push_back( ui->VoteKickPercentageSpin );
    LabelsList.push_back( ui->OptionOwnerLabel );
    WidgetsList.push_back( ui->OptionOwnerCombo );
    LabelsList.push_back( ui->AFKkickLabel );
    WidgetsList.push_back( ui->AFKkickCombo );
    LabelsList.push_back( ui->AFKtimerLabel );
    WidgetsList.push_back( ui->AFKtimerSpin );
    LabelsList.push_back( ui->ReconnectWaitTimeLabel );
    WidgetsList.push_back( ui->ReconnectWaitTimeSpin );
    LabelsList.push_back( ui->DropVotePercentLabel );
    WidgetsList.push_back( ui->DropVotePercentSpin );
    LabelsList.push_back( ui->DropVoteTimeLabel );
    WidgetsList.push_back( ui->DropVoteTimeSpin );
    LabelsList.push_back( ui->DropWaitTimeLabel );
    WidgetsList.push_back( ui->DropWaitTimeSpin );
    LabelsList.push_back( ui->DropWaitTimeSumLabel );
    WidgetsList.push_back( ui->DropWaitTimeSumSpin );
    LabelsList.push_back( ui->DropWaitTimeGameLabel );
    WidgetsList.push_back( ui->DropWaitTimeGameSpin );
    LabelsList.push_back( ui->DropWaitTimeLoadLabel );
    WidgetsList.push_back( ui->DropWaitTimeLoadSpin );
    LabelsList.push_back( ui->DropWaitTimeLobbyLabel );
    WidgetsList.push_back( ui->DropWaitTimeLobbySpin );
    LabelsList.push_back( ui->AllowLaggingTimeLabel );
    WidgetsList.push_back( ui->AllowLaggingTimeSpin );
    LabelsList.push_back( ui->AllowLaggingTimeIntervalLabel );
    WidgetsList.push_back( ui->AllowLaggingTimeIntervalSpin );
    LabelsList.push_back( ui->UserCanDropAdminLabel );
    WidgetsList.push_back( ui->UserCanDropAdminCombo );
    LabelsList.push_back( ui->DropIfDesyncLabel );
    WidgetsList.push_back( ui->DropIfDesyncCombo );
    LabelsList.push_back( ui->gameoverminpercentLabel );
    WidgetsList.push_back( ui->gameoverminpercentSpin );
    LabelsList.push_back( ui->gameoverminplayersLabel );
    WidgetsList.push_back( ui->gameoverminplayersSpin );
    LabelsList.push_back( ui->gameovermaxteamdifferenceLabel );
    WidgetsList.push_back( ui->gameovermaxteamdifferenceSpin );
    LabelsList.push_back( ui->LobbyTimeLimitLabel );
    WidgetsList.push_back( ui->LobbyTimeLimitSpin );
    LabelsList.push_back( ui->LobbyTimeLimitMaxLabel );
    WidgetsList.push_back( ui->LobbyTimeLimitMaxSpin );
    LabelsList.push_back( ui->PubGameDelayLabel );
    WidgetsList.push_back( ui->PubGameDelaySpin );
    LabelsList.push_back( ui->BotCommandTriggerLabel );
    WidgetsList.push_back( ui->BotCommandTriggerLine );
    LabelsList.push_back( ui->VirtualHostNameLabel );
    WidgetsList.push_back( ui->VirtualHostNameLine );
    LabelsList.push_back( ui->ObsPlayerNameLabel );
    WidgetsList.push_back( ui->ObsPlayerNameLine );
    LabelsList.push_back( ui->DontDeleteVirtualHostLabel );
    WidgetsList.push_back( ui->DontDeleteVirtualHostCombo );
    LabelsList.push_back( ui->DontDeletePenultimatePlayerLabel );
    WidgetsList.push_back( ui->DontDeletePenultimatePlayerCombo );
    LabelsList.push_back( ui->NumRequiredPingsPlayersLabel );
    WidgetsList.push_back( ui->NumRequiredPingsPlayersSpin );
    LabelsList.push_back( ui->NumRequiredPingsPlayersASLabel );
    WidgetsList.push_back( ui->NumRequiredPingsPlayersASSpin );
    LabelsList.push_back( ui->AntiFloodMuteLabel );
    WidgetsList.push_back( ui->AntiFloodMuteCombo );
    LabelsList.push_back( ui->AdminCanUnAutoMuteLabel );
    WidgetsList.push_back( ui->AdminCanUnAutoMuteCombo );
    LabelsList.push_back( ui->AntiFloodMuteSecondsLabel );
    WidgetsList.push_back( ui->AntiFloodMuteSecondsSpin );
    LabelsList.push_back( ui->AntiFloodRigidityTimeLabel );
    WidgetsList.push_back( ui->AntiFloodRigidityTimeSpin );
    LabelsList.push_back( ui->AntiFloodRigidityNumberLabel );
    WidgetsList.push_back( ui->AntiFloodRigidityNumberSpin );
    LabelsList.push_back( ui->AdminCanUnCensorMuteLabel );
    WidgetsList.push_back( ui->AdminCanUnCensorMuteCombo );
    LabelsList.push_back( ui->CensorMuteLabel );
    WidgetsList.push_back( ui->CensorMuteCombo );
    LabelsList.push_back( ui->CensorMuteAdminsLabel );
    WidgetsList.push_back( ui->CensorMuteAdminsCombo );
    LabelsList.push_back( ui->CensorLabel );
    WidgetsList.push_back( ui->CensorCombo );
    LabelsList.push_back( ui->CensorDictionaryLabel );
    WidgetsList.push_back( ui->CensorDictionaryCombo );
    LabelsList.push_back( ui->CensorMuteFirstSecondsLabel );
    WidgetsList.push_back( ui->CensorMuteFirstSecondsSpin );
    LabelsList.push_back( ui->CensorMuteSecondSecondsLabel );
    WidgetsList.push_back( ui->CensorMuteSecondSecondsSpin );
    LabelsList.push_back( ui->CensorMuteExcessiveSecondsLabel );
    WidgetsList.push_back( ui->CensorMuteExcessiveSecondsSpin );
    LabelsList.push_back( ui->NewCensorCookiesLabel );
    WidgetsList.push_back( ui->NewCensorCookiesCombo );
    LabelsList.push_back( ui->NewCensorCookiesNumberLabel );
    WidgetsList.push_back( ui->NewCensorCookiesNumberSpin );
    LabelsList.push_back( ui->EatGameLabel );
    WidgetsList.push_back( ui->EatGameCombo );
    LabelsList.push_back( ui->MaximumNumberCookiesLabel );
    WidgetsList.push_back( ui->MaximumNumberCookiesSpin );
    LabelsList.push_back( ui->RejectColoredNameLabel );
    WidgetsList.push_back( ui->RejectColoredNameCombo );
    LabelsList.push_back( ui->DenyMinDownloadRateLabel );
    WidgetsList.push_back( ui->DenyMinDownloadRateSpin );
    LabelsList.push_back( ui->DenyMaxDownloadTimeLabel );
    WidgetsList.push_back( ui->DenyMaxDownloadTimeSpin );
    LabelsList.push_back( ui->DenyMaxMapsizeTimeLabel );
    WidgetsList.push_back( ui->DenyMaxMapsizeTimeSpin );
    LabelsList.push_back( ui->DenyMaxReqjoinTimeLabel );
    WidgetsList.push_back( ui->DenyMaxReqjoinTimeSpin );
    LabelsList.push_back( ui->DenyMaxLoadTimeLabel );
    WidgetsList.push_back( ui->DenyMaxLoadTimeSpin );
    LabelsList.push_back( ui->NonAdminCommandsGameLabel );
    WidgetsList.push_back( ui->NonAdminCommandsGameCombo );
    LabelsList.push_back( ui->PlayerServerPrintJoinLabel );
    WidgetsList.push_back( ui->PlayerServerPrintJoinCombo );
    LabelsList.push_back( ui->FunCommandsGameLabel );
    WidgetsList.push_back( ui->FunCommandsGameCombo );
    LabelsList.push_back( ui->WarningPacketValidationFailedLabel );
    WidgetsList.push_back( ui->WarningPacketValidationFailedCombo );
    LabelsList.push_back( ui->ResourceTradePrintLabel );
    WidgetsList.push_back( ui->ResourceTradePrintCombo );
    LabelsList.push_back( ui->PlayerBeforeStartPrintLabel );
    WidgetsList.push_back( ui->PlayerBeforeStartPrintCombo );
    LabelsList.push_back( ui->PlayerBeforeStartPrintJoinLabel );
    WidgetsList.push_back( ui->PlayerBeforeStartPrintJoinCombo );
    LabelsList.push_back( ui->PlayerBeforeStartPrivatePrintJoinLabel );
    WidgetsList.push_back( ui->PlayerBeforeStartPrivatePrintJoinCombo );
    LabelsList.push_back( ui->PlayerBeforeStartPrivatePrintJoinSDLabel );
    WidgetsList.push_back( ui->PlayerBeforeStartPrivatePrintJoinSDCombo );
    LabelsList.push_back( ui->PlayerBeforeStartPrintLeaveLabel );
    WidgetsList.push_back( ui->PlayerBeforeStartPrintLeaveCombo );
    LabelsList.push_back( ui->BotChannelPrivatePrintJoinLabel );
    WidgetsList.push_back( ui->BotChannelPrivatePrintJoinCombo );
    LabelsList.push_back( ui->BotNamePrivatePrintJoinLabel );
    WidgetsList.push_back( ui->BotNamePrivatePrintJoinCombo );
    LabelsList.push_back( ui->InfoGamesPrivatePrintJoinLabel );
    WidgetsList.push_back( ui->InfoGamesPrivatePrintJoinCombo );
    LabelsList.push_back( ui->GameNamePrivatePrintJoinLabel );
    WidgetsList.push_back( ui->GameNamePrivatePrintJoinCombo );
    LabelsList.push_back( ui->GameModePrivatePrintJoinLabel );
    WidgetsList.push_back( ui->GameModePrivatePrintJoinCombo );
    LabelsList.push_back( ui->RehostPrintingLabel );
    WidgetsList.push_back( ui->RehostPrintingCombo );
    LabelsList.push_back( ui->RehostFailedPrintingLabel );
    WidgetsList.push_back( ui->RehostFailedPrintingCombo );
    LabelsList.push_back( ui->PlayerFinishedLoadingPrintingLabel );
    WidgetsList.push_back( ui->PlayerFinishedLoadingPrintingCombo );
    LabelsList.push_back( ui->LobbyAnnounceUnoccupiedLabel );
    WidgetsList.push_back( ui->LobbyAnnounceUnoccupiedCombo );
    LabelsList.push_back( ui->RelayChatCommandsLabel );
    WidgetsList.push_back( ui->RelayChatCommandsCombo );
    LabelsList.push_back( ui->DetourAllMessagesToAdminsLabel );
    WidgetsList.push_back( ui->DetourAllMessagesToAdminsCombo );
    LabelsList.push_back( ui->AdminMessagesLabel );
    WidgetsList.push_back( ui->AdminMessagesCombo );
    LabelsList.push_back( ui->LocalAdminMessagesLabel );
    WidgetsList.push_back( ui->LocalAdminMessagesCombo );
    LabelsList.push_back( ui->WarnLatencyLabel );
    WidgetsList.push_back( ui->WarnLatencyCombo );
    LabelsList.push_back( ui->RefreshMessagesLabel );
    WidgetsList.push_back( ui->RefreshMessagesCombo );
    LabelsList.push_back( ui->GameAnnounceLabel );
    WidgetsList.push_back( ui->GameAnnounceCombo );
    LabelsList.push_back( ui->ShowDownloadsInfoLabel );
    WidgetsList.push_back( ui->ShowDownloadsInfoCombo );
    LabelsList.push_back( ui->ShowFinishDownloadingInfoLabel );
    WidgetsList.push_back( ui->ShowFinishDownloadingInfoCombo );
    LabelsList.push_back( ui->ShowDownloadsInfoTimeLabel );
    WidgetsList.push_back( ui->ShowDownloadsInfoTimeSpin );
    LabelsList.push_back( ui->autoinsultlobbyLabel );
    WidgetsList.push_back( ui->autoinsultlobbyCombo );
    LabelsList.push_back( ui->autoinsultlobbysafeadminsLabel );
    WidgetsList.push_back( ui->autoinsultlobbysafeadminsCombo );
    LabelsList.push_back( ui->GameOwnerPrivatePrintJoinLabel );
    WidgetsList.push_back( ui->GameOwnerPrivatePrintJoinCombo );
    LabelsList.push_back( ui->AutoUnMutePrintingLabel );
    WidgetsList.push_back( ui->AutoUnMutePrintingCombo );
    LabelsList.push_back( ui->ManualUnMutePrintingLabel );
    WidgetsList.push_back( ui->ManualUnMutePrintingCombo );
    LabelsList.push_back( ui->GameIsOverMessagePrintingLabel );
    WidgetsList.push_back( ui->GameIsOverMessagePrintingCombo );
    LabelsList.push_back( ui->manualhclmessageLabel );
    WidgetsList.push_back( ui->manualhclmessageCombo );
    LabelsList.push_back( ui->GameAnnounceIntervalLabel );
    WidgetsList.push_back( ui->GameAnnounceIntervalSpin );
    LabelsList.push_back( ui->NoMapPrintDelayLabel );
    WidgetsList.push_back( ui->NoMapPrintDelaySpin );
    LabelsList.push_back( ui->GameLoadedPrintoutLabel );
    WidgetsList.push_back( ui->GameLoadedPrintoutSpin );
    LabelsList.push_back( ui->StillDownloadingPrintLabel );
    WidgetsList.push_back( ui->StillDownloadingPrintCombo );
    LabelsList.push_back( ui->NotSpoofCheckedPrintLabel );
    WidgetsList.push_back( ui->NotSpoofCheckedPrintCombo );
    LabelsList.push_back( ui->NotPingedPrintLabel );
    WidgetsList.push_back( ui->NotPingedPrintCombo );
    LabelsList.push_back( ui->StillDownloadingASPrintLabel );
    WidgetsList.push_back( ui->StillDownloadingASPrintCombo );
    LabelsList.push_back( ui->NotSpoofCheckedASPrintLabel );
    WidgetsList.push_back( ui->NotSpoofCheckedASPrintCombo );
    LabelsList.push_back( ui->NotPingedASPrintLabel );
    WidgetsList.push_back( ui->NotPingedASPrintCombo );
    LabelsList.push_back( ui->StillDownloadingASPrintDelayLabel );
    WidgetsList.push_back( ui->StillDownloadingASPrintDelaySpin );
    LabelsList.push_back( ui->NotSpoofCheckedASPrintDelayLabel );
    WidgetsList.push_back( ui->NotSpoofCheckedASPrintDelaySpin );
    LabelsList.push_back( ui->NotPingedASPrintDelayLabel );
    WidgetsList.push_back( ui->NotPingedASPrintDelaySpin );
    LabelsList.push_back( ui->PlayerBeforeStartPrintDelayLabel );
    WidgetsList.push_back( ui->PlayerBeforeStartPrintDelaySpin );
    LabelsList.push_back( ui->RehostPrintingDelayLabel );
    WidgetsList.push_back( ui->RehostPrintingDelaySpin );
    LabelsList.push_back( ui->ButGameDelayLabel );
    WidgetsList.push_back( ui->ButGameDelaySpin );
    LabelsList.push_back( ui->MarsGameDelayLabel );
    WidgetsList.push_back( ui->MarsGameDelaySpin );
    LabelsList.push_back( ui->SlapGameDelayLabel );
    WidgetsList.push_back( ui->SlapGameDelaySpin );
    LabelsList.push_back( ui->RollGameDelayLabel );
    WidgetsList.push_back( ui->RollGameDelaySpin );
    LabelsList.push_back( ui->PictureGameDelayLabel );
    WidgetsList.push_back( ui->PictureGameDelaySpin );
    LabelsList.push_back( ui->EatGameDelayLabel );
    WidgetsList.push_back( ui->EatGameDelaySpin );
    LabelsList.push_back( ui->BotChannelCustomLabel );
    WidgetsList.push_back( ui->BotChannelCustomLine );
    LabelsList.push_back( ui->BotNameCustomLabel );
    WidgetsList.push_back( ui->BotNameCustomLine );
    LabelsList.push_back( ui->AllowAutoRenameMapsLabel );
    WidgetsList.push_back( ui->AllowAutoRenameMapsCombo );
    LabelsList.push_back( ui->PlayersCanChangeRaceLabel );
    WidgetsList.push_back( ui->PlayersCanChangeRaceCombo );
    LabelsList.push_back( ui->OverrideMapDataFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapDataFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapPathFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapPathFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapSizeFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapSizeFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapInfoFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapInfoFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapCrcFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapCrcFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapSha1FromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapSha1FromCFGCombo );
    LabelsList.push_back( ui->OverrideMapSpeedFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapSpeedFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapVisibilityFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapVisibilityFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapFlagsFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapFlagsFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapFilterTypeFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapFilterTypeFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapGameTypeFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapGameTypeFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapWidthFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapWidthFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapHeightFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapHeightFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapTypeFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapTypeFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapDefaultHCLFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapDefaultHCLFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapLoadInGameFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapLoadInGameFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapNumPlayersFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapNumPlayersFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapNumTeamsFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapNumTeamsFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapSlotsFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapSlotsFromCFGCombo );
    LabelsList.push_back( ui->OverrideMapObserversFromCFGLabel );
    WidgetsList.push_back( ui->OverrideMapObserversFromCFGCombo );
}

void Widget::UpdateValuesInGUI( QList<QString> cfgvars )
{
    if( cfgvars.empty( ) )
    {
        UpdateAllValuesInGUI( );
        return;
    }

    for( int32_t n = 0; n < cfgvars.size( ); n++ )
    {
        int32_t ValueStart = cfgvars[n].indexOf( "@" );
        QString CFGVar = cfgvars[n].mid( 0, ValueStart );
        QString CFGValue = cfgvars[n].mid( ValueStart + 1 );

        for( int32_t i = 0; i < WidgetsList.size( ); i++ )
        {
            if( CFGVar == LabelsList[i]->whatsThis( ) )
            {
                if( WidgetsList[i]->whatsThis( ) == "bool" )
                    qobject_cast<QComboBox *>( WidgetsList[i] )->setCurrentIndex( CFGValue == "1" ? 0 : 1 );
                else if( WidgetsList[i]->whatsThis( ) == "intlimit" )
                    qobject_cast<QComboBox *>( WidgetsList[i] )->setCurrentIndex( UTIL_QSToInt32( CFGValue ) );
                else if( WidgetsList[i]->whatsThis( ) == "intlimitfix1" )
                    qobject_cast<QComboBox *>( WidgetsList[i] )->setCurrentIndex( UTIL_QSToInt32( CFGValue ) - 1 );
                else if( WidgetsList[i]->whatsThis( ) == "intunlimit" )
                    qobject_cast<QSpinBox *>( WidgetsList[i] )->setValue( UTIL_QSToInt32( CFGValue ) );
                else if( WidgetsList[i]->whatsThis( ) == "string" )
                    qobject_cast<QLineEdit *>( WidgetsList[i] )->setText( CFGValue );

                break;
            }
        }
    }
}

void Widget::UpdateAllValuesInGUI( )
{
    QList<QString> AllCFGVars;
    int32_t ValueStart;
    QString CFGVar;
    QString CFGValue;
    AllCFGVars.push_back( "designeGUILabel@" + UTIL_ToQString( ui->designeGUICombo->currentIndex( ) ) );
    AllCFGVars.push_back( "gui_showpesettingwindow@" + UTIL_ToQString( gUNM->m_ShowPesettingWindow ) );
    AllCFGVars.push_back( "bot_language@" + QString::fromUtf8( gLanguageFile.c_str( ) ) );
    AllCFGVars.push_back( "bot_war3path@" + QString::fromUtf8( gUNM->m_Warcraft3Path.c_str( ) ) );
    AllCFGVars.push_back( "bot_mappath@" + QString::fromUtf8( gUNM->m_MapPath.c_str( ) ) );
    AllCFGVars.push_back( gUNM->m_patch23 ? "bot_patch23ornewer@1" : "bot_patch23ornewer@0" );
    AllCFGVars.push_back( "lan_war3version@" + UTIL_ToQString( gUNM->m_LANWar3Version ) );
    AllCFGVars.push_back( "bot_broadcastport@" + UTIL_ToQString( gUNM->m_BroadcastPort ) );
    AllCFGVars.push_back( "bot_enableloggingtogui@" + UTIL_ToQString( gEnableLoggingToGUI ) );
    AllCFGVars.push_back( gUNM->m_LogReduction ? "bot_logreduction@1" : "bot_logreduction@0" );
    AllCFGVars.push_back( "bot_maxlogsize@" + UTIL_ToQString( gUNM->m_MaxLogSize ) );
    AllCFGVars.push_back( "bot_maxlogtime@" + UTIL_ToQString( gUNM->m_MaxLogTime ) );
    AllCFGVars.push_back( "bot_minlengthgn@" + UTIL_ToQString( gUNM->m_MinLengthGN ) );
    AllCFGVars.push_back( "bot_slotinfoingamename@" + UTIL_ToQString( gUNM->m_SlotInfoInGameName ) );
    AllCFGVars.push_back( gUNM->m_patch21 ? "bot_patch21@1" : "bot_patch21@0" );
    AllCFGVars.push_back( gUNM->m_broadcastinlan ? "bot_broadcastlan@1" : "bot_broadcastlan@0" );
    AllCFGVars.push_back( gUNM->m_ShowRealSlotCount ? "bot_showrealslotcountonlan@1" : "bot_showrealslotcountonlan@0" );
    AllCFGVars.push_back( "bot_broadcasthostname@" + UTIL_ToQString( gUNM->m_BroadcastHostName ) );
    AllCFGVars.push_back( "bot_broadcasthncustom@" + QString::fromUtf8( gUNM->m_BroadcastHostNameCustom.c_str( ) ) );
    AllCFGVars.push_back( "bot_hostport@" + UTIL_ToQString( gUNM->m_HostPort ) );
    AllCFGVars.push_back( "bot_defaultmap@" + QString::fromUtf8( gUNM->m_DefaultMap.c_str( ) ) );
    AllCFGVars.push_back( "bot_bindaddress@" + QString::fromUtf8( gUNM->m_BindAddress.c_str( ) ) );
    AllCFGVars.push_back( gUNM->m_HideIPAddresses ? "bot_hideipaddresses@1" : "bot_hideipaddresses@0" );
    AllCFGVars.push_back( "bot_mapcfgpath@" + QString::fromUtf8( gUNM->m_MapCFGPath.c_str( ) ) );
    AllCFGVars.push_back( "bot_savegamepath@" + QString::fromUtf8( gUNM->m_SaveGamePath.c_str( ) ) );
    AllCFGVars.push_back( "bot_replaypath@" + QString::fromUtf8( gUNM->m_ReplayPath.c_str( ) ) );
    AllCFGVars.push_back( gUNM->m_SaveReplays ? "bot_savereplays@1" : "bot_savereplays@0" );
    AllCFGVars.push_back( "replay_buildnumber@" + UTIL_ToQString( gUNM->m_ReplayBuildNumber ) );
    AllCFGVars.push_back( "replay_war3version@" + UTIL_ToQString( gUNM->m_ReplayWar3Version ) );
    AllCFGVars.push_back( gUNM->m_ReplaysByName ? "bot_replayssavedbyname@1" : "bot_replayssavedbyname@0" );
    AllCFGVars.push_back( "bot_invalidtriggersgame@" + QString::fromUtf8( gUNM->m_InvalidTriggersGame.c_str( ) ) );
    AllCFGVars.push_back( "bot_invalidreplaychars@" + QString::fromUtf8( gUNM->m_InvalidReplayChars.c_str( ) ) );
    AllCFGVars.push_back( "bot_maxhostcounter@" + UTIL_ToQString( gUNM->m_MaxHostCounter ) );
    AllCFGVars.push_back( "bot_rehostchar@" + QString::fromUtf8( gUNM->m_RehostChar.c_str( ) ) );
    AllCFGVars.push_back( "bot_gamenamecharhide@" + UTIL_ToQString( gUNM->m_GameNameCharHide ) );
    AllCFGVars.push_back( "bot_gamenamechar@" + QString::fromUtf8( gUNM->m_DefaultGameNameChar.c_str( ) ) );
    AllCFGVars.push_back( gUNM->m_ResetGameNameChar ? "bot_resetgamenamechar@1" : "bot_resetgamenamechar@0" );
    AllCFGVars.push_back( gUNM->m_AppleIcon ? "bot_appleicon@1" : "bot_appleicon@0" );
    AllCFGVars.push_back( gUNM->m_FixGameNameForIccup ? "bot_fixgamenameforiccup@1" : "bot_fixgamenameforiccup@0" );
    AllCFGVars.push_back( gUNM->m_GameStartedMessagePrinting ? "bot_gamestartedmessageprinting@1" : "bot_gamestartedmessageprinting@0" );
    AllCFGVars.push_back( gUNM->m_PrefixName ? "bot_realmprefixname@1" : "bot_realmprefixname@0" );
    AllCFGVars.push_back( "bot_maxgames@" + UTIL_ToQString( gUNM->m_MaxGames ) );
    AllCFGVars.push_back( "bot_ipblacklistfile@" + QString::fromUtf8( gUNM->m_IPBlackListFile.c_str( ) ) );
    AllCFGVars.push_back( "bot_gamestateinhouse@" + UTIL_ToQString( gUNM->m_gamestateinhouse ) );
    AllCFGVars.push_back( gUNM->m_autohclfromgamename ? "bot_autohclfromgamename@1" : "bot_autohclfromgamename@0" );
    AllCFGVars.push_back( gUNM->m_forceautohclindota ? "bot_forceautohclindota@1" : "bot_forceautohclindota@0" );
    AllCFGVars.push_back( gUNM->m_SpoofCheckIfGameNameIsIndefinite ? "bot_spoofcheckifgnisindefinite@1" : "bot_spoofcheckifgnisindefinite@0" );
    AllCFGVars.push_back( gUNM->m_HoldPlayersForRMK ? "bot_holdplayersforrmk@1" : "bot_holdplayersforrmk@0" );
    AllCFGVars.push_back( gUNM->m_ForceLoadInGame ? "bot_forceloadingame@1" : "bot_forceloadingame@0" );
    AllCFGVars.push_back( "bot_msgautocorrectmode@" + UTIL_ToQString( gUNM->m_MsgAutoCorrectMode ) );
    AllCFGVars.push_back( "bot_spoofchecks@" + UTIL_ToQString( gUNM->m_SpoofChecks ) );
    AllCFGVars.push_back( gUNM->m_CheckMultipleIPUsage ? "bot_checkmultipleipusage@1" : "bot_checkmultipleipusage@0" );
    AllCFGVars.push_back( "bot_maxlatency@" + UTIL_ToQString( gUNM->m_MaxLatency ) );
    AllCFGVars.push_back( "bot_minlatency@" + UTIL_ToQString( gUNM->m_MinLatency ) );
    AllCFGVars.push_back( "bot_latency@" + UTIL_ToQString( gUNM->m_Latency ) );
    AllCFGVars.push_back( gUNM->m_UseDynamicLatency ? "bot_usedynamiclatency@1" : "bot_usedynamiclatency@0" );
    AllCFGVars.push_back( "bot_dynamiclatencyhighestallowed@" + UTIL_ToQString( gUNM->m_DynamicLatencyHighestAllowed ) );
    AllCFGVars.push_back( "bot_dynamiclatencylowestallowed@" + UTIL_ToQString( gUNM->m_DynamicLatencyLowestAllowed ) );
    AllCFGVars.push_back( "bot_dynamiclatencyrefreshrate@" + UTIL_ToQString( gUNM->m_DynamicLatencyRefreshRate ) );
    AllCFGVars.push_back( "bot_dynamiclatencyconsoleprint@" + UTIL_ToQString( gUNM->m_DynamicLatencyConsolePrint ) );
    AllCFGVars.push_back( "bot_dynamiclatencypercentpm@" + UTIL_ToQString( gUNM->m_DynamicLatencyPercentPingMax ) );
    AllCFGVars.push_back( "bot_dynamiclatencydiffpingmax@" + UTIL_ToQString( gUNM->m_DynamicLatencyDifferencePingMax ) );
    AllCFGVars.push_back( "bot_dynamiclatencymaxsync@" + UTIL_ToQString( gUNM->m_DynamicLatencyMaxSync ) );
    AllCFGVars.push_back( "bot_dynamiclatencyaddifmaxsync@" + UTIL_ToQString( gUNM->m_DynamicLatencyAddIfMaxSync ) );
    AllCFGVars.push_back( "bot_dynamiclatencyaddiflag@" + UTIL_ToQString( gUNM->m_DynamicLatencyAddIfLag ) );
    AllCFGVars.push_back( "bot_dynamiclatencyaddifbotlag@" + UTIL_ToQString( gUNM->m_DynamicLatencyAddIfBotLag ) );
    AllCFGVars.push_back( "bot_maxsynclimit@" + UTIL_ToQString( gUNM->m_MaxSyncLimit ) );
    AllCFGVars.push_back( "bot_minsynclimit@" + UTIL_ToQString( gUNM->m_MinSyncLimit ) );
    AllCFGVars.push_back( "bot_synclimit@" + UTIL_ToQString( gUNM->m_SyncLimit ) );
    AllCFGVars.push_back( gUNM->m_AutoSave ? "bot_autosave@1" : "bot_autosave@0" );
    AllCFGVars.push_back( "bot_autokickping@" + UTIL_ToQString( gUNM->m_AutoKickPing ) );
    AllCFGVars.push_back( gUNM->m_LCPings ? "bot_lcpings@1" : "bot_lcpings@0" );
    AllCFGVars.push_back( gUNM->m_PingDuringDownloads ? "bot_pingduringdownloads@1" : "bot_pingduringdownloads@0" );
    AllCFGVars.push_back( "bot_allowdownloads@" + UTIL_ToQString( gUNM->m_AllowDownloads ) );
    AllCFGVars.push_back( "bot_maxdownloaders@" + UTIL_ToQString( gUNM->m_maxdownloaders ) );
    AllCFGVars.push_back( "bot_totaldownloadspeed@" + UTIL_ToQString( gUNM->m_totaldownloadspeed ) );
    AllCFGVars.push_back( "bot_clientdownloadspeed@" + UTIL_ToQString( gUNM->m_clientdownloadspeed ) );
    AllCFGVars.push_back( "bot_placeadminshigher@" + UTIL_ToQString( gUNM->m_PlaceAdminsHigher ) );
    AllCFGVars.push_back( gUNM->m_ShuffleSlotsOnStart ? "bot_shuffleslotsonstart@1" : "bot_shuffleslotsonstart@0" );
    AllCFGVars.push_back( gUNM->m_FakePlayersLobby ? "bot_fakeplayersinlobby@1" : "bot_fakeplayersinlobby@0" );
    AllCFGVars.push_back( "bot_morefakeplayersinlobby@" + UTIL_ToQString( gUNM->m_MoreFPsLobby ) );
    AllCFGVars.push_back( gUNM->m_NormalCountdown ? "bot_normalcountdown@1" : "bot_normalcountdown@0" );
    AllCFGVars.push_back( "bot_countdownmethod@" + UTIL_ToQString( gUNM->m_CountDownMethod ) );
    AllCFGVars.push_back( "bot_countdownstartcounter@" + UTIL_ToQString( gUNM->m_CountDownStartCounter ) );
    AllCFGVars.push_back( "bot_countdownstartforcecounter@" + UTIL_ToQString( gUNM->m_CountDownStartForceCounter ) );
    AllCFGVars.push_back( gUNM->m_VoteStartAllowed ? "bot_votestartallowed@1" : "bot_votestartallowed@0" );
    AllCFGVars.push_back( gUNM->m_VoteStartPercentalVoting ? "bot_votestartpercentalvoting@1" : "bot_votestartpercentalvoting@0" );
    AllCFGVars.push_back( gUNM->m_CancelVoteStartIfLeave ? "bot_cancelvotestartifleave@1" : "bot_cancelvotestartifleave@0" );
    AllCFGVars.push_back( gUNM->m_StartAfterLeaveIfEnoughVotes ? "bot_startafterleaveifenoughvotes@1" : "bot_startafterleaveifenoughvotes@0" );
    AllCFGVars.push_back( "bot_votestartminplayers@" + UTIL_ToQString( gUNM->m_VoteStartMinPlayers ) );
    AllCFGVars.push_back( "bot_votestartpercent@" + UTIL_ToQString( gUNM->m_VoteStartPercent ) );
    AllCFGVars.push_back( "bot_votestartduration@" + UTIL_ToQString( gUNM->m_VoteStartDuration ) );
    AllCFGVars.push_back( gUNM->m_VoteKickAllowed ? "bot_votekickallowed@1" : "bot_votekickallowed@0" );
    AllCFGVars.push_back( "bot_votekickpercentage@" + UTIL_ToQString( gUNM->m_VoteKickPercentage ) );
    AllCFGVars.push_back( "bot_optionowner@" + UTIL_ToQString( gUNM->m_OptionOwner ) );
    AllCFGVars.push_back( "bot_afkkick@" + UTIL_ToQString( gUNM->m_AFKkick ) );
    AllCFGVars.push_back( "bot_afktimer@" + UTIL_ToQString( gUNM->m_AFKtimer ) );
    AllCFGVars.push_back( "bot_reconnectwaittime@" + UTIL_ToQString( gUNM->m_ReconnectWaitTime ) );
    AllCFGVars.push_back( "bot_dropvotepercent@" + UTIL_ToQString( gUNM->m_DropVotePercent ) );
    AllCFGVars.push_back( "bot_dropvotetime@" + UTIL_ToQString( gUNM->m_DropVoteTime ) );
    AllCFGVars.push_back( "bot_dropwaittime@" + UTIL_ToQString( gUNM->m_DropWaitTime ) );
    AllCFGVars.push_back( "bot_dropwaittimesum@" + UTIL_ToQString( gUNM->m_DropWaitTimeSum ) );
    AllCFGVars.push_back( "bot_dropwaittimegame@" + UTIL_ToQString( gUNM->m_DropWaitTimeGame ) );
    AllCFGVars.push_back( "bot_dropwaittimeload@" + UTIL_ToQString( gUNM->m_DropWaitTimeLoad ) );
    AllCFGVars.push_back( "bot_dropwaittimelobby@" + UTIL_ToQString( gUNM->m_DropWaitTimeLobby ) );
    AllCFGVars.push_back( "bot_allowlaggingtime@" + UTIL_ToQString( gUNM->m_AllowLaggingTime ) );
    AllCFGVars.push_back( "bot_allowlaggingtimeinterval@" + UTIL_ToQString( gUNM->m_AllowLaggingTimeInterval ) );
    AllCFGVars.push_back( gUNM->m_UserCanDropAdmin ? "bot_usercandropadmin@1" : "bot_usercandropadmin@0" );
    AllCFGVars.push_back( gUNM->m_DropIfDesync ? "bot_dropifdesync@1" : "bot_dropifdesync@0" );
    AllCFGVars.push_back( "bot_gameoverminpercent@" + UTIL_ToQString( gUNM->m_gameoverminpercent ) );
    AllCFGVars.push_back( "bot_gameoverminplayers@" + UTIL_ToQString( gUNM->m_gameoverminplayers ) );
    AllCFGVars.push_back( "bot_gameovermaxteamdifference@" + UTIL_ToQString( gUNM->m_gameovermaxteamdifference ) );
    AllCFGVars.push_back( "bot_lobbytimelimit@" + UTIL_ToQString( gUNM->m_LobbyTimeLimit ) );
    AllCFGVars.push_back( "bot_lobbytimelimitmax@" + UTIL_ToQString( gUNM->m_LobbyTimeLimitMax ) );
    AllCFGVars.push_back( "bot_pubgamedelay@" + UTIL_ToQString( gUNM->m_PubGameDelay ) );
    AllCFGVars.push_back( "bot_commandtrigger@" + QString::fromUtf8( gUNM->BotCommandTrigger.c_str( ) ) );
    AllCFGVars.push_back( "bot_virtualhostname@" + QString::fromUtf8( gUNM->m_VirtualHostName.c_str( ) ) );
    AllCFGVars.push_back( "bot_obsplayername@" + QString::fromUtf8( gUNM->m_ObsPlayerName.c_str( ) ) );
    AllCFGVars.push_back( gUNM->m_DontDeleteVirtualHost ? "bot_dontdeletevirtualhost@1" : "bot_dontdeletevirtualhost@0" );
    AllCFGVars.push_back( gUNM->m_DontDeletePenultimatePlayer ? "bot_dontdeletepenultimateplayer@1" : "bot_dontdeletepenultimateplayer@0" );
    AllCFGVars.push_back( "bot_numrequiredpingsplayers@" + UTIL_ToQString( gUNM->m_NumRequiredPingsPlayers ) );
    AllCFGVars.push_back( "bot_numrequiredpingsplayersas@" + UTIL_ToQString( gUNM->m_NumRequiredPingsPlayersAS ) );
    AllCFGVars.push_back( gUNM->m_AntiFloodMute ? "bot_antifloodmute@1" : "bot_antifloodmute@0" );
    AllCFGVars.push_back( gUNM->m_AdminCanUnAutoMute ? "bot_admincanunautomute@1" : "bot_admincanunautomute@0" );
    AllCFGVars.push_back( "bot_antifloodmuteseconds@" + UTIL_ToQString( gUNM->m_AntiFloodMuteSeconds ) );
    AllCFGVars.push_back( "bot_antifloodrigiditytime@" + UTIL_ToQString( gUNM->m_AntiFloodRigidityTime ) );
    AllCFGVars.push_back( "bot_antifloodrigiditynumber@" + UTIL_ToQString( gUNM->m_AntiFloodRigidityNumber ) );
    AllCFGVars.push_back( gUNM->m_AdminCanUnCensorMute ? "bot_admincanuncensormute@1" : "bot_admincanuncensormute@0" );
    AllCFGVars.push_back( gUNM->m_CensorMute ? "bot_censormute@1" : "bot_censormute@0" );
    AllCFGVars.push_back( gUNM->m_CensorMuteAdmins ? "bot_censormuteadmins@1" : "bot_censormuteadmins@0" );
    AllCFGVars.push_back( gUNM->m_Censor ? "bot_censor@1" : "bot_censor@0" );
    AllCFGVars.push_back( "bot_censordictionary@" + UTIL_ToQString( gUNM->m_CensorDictionary ) );
    AllCFGVars.push_back( "bot_censormutefirstseconds@" + UTIL_ToQString( gUNM->m_CensorMuteFirstSeconds ) );
    AllCFGVars.push_back( "bot_censormutesecondseconds@" + UTIL_ToQString( gUNM->m_CensorMuteSecondSeconds ) );
    AllCFGVars.push_back( "bot_censormuteexcessiveseconds@" + UTIL_ToQString( gUNM->m_CensorMuteExcessiveSeconds ) );
    AllCFGVars.push_back( gUNM->m_NewCensorCookies ? "bot_newcensorcookies@1" : "bot_newcensorcookies@0" );
    AllCFGVars.push_back( "bot_newcensorcookiesnumber@" + UTIL_ToQString( gUNM->m_NewCensorCookiesNumber ) );
    AllCFGVars.push_back( gUNM->m_EatGame ? "bot_eatgame@1" : "bot_eatgame@0" );
    AllCFGVars.push_back( "bot_maximumnumbercookies@" + UTIL_ToQString( gUNM->m_MaximumNumberCookies ) );
    AllCFGVars.push_back( gUNM->m_RejectColoredName ? "bot_rejectcolorname@1" : "bot_rejectcolorname@0" );
    AllCFGVars.push_back( "bot_mindownloadrate@" + UTIL_ToQString( gUNM->m_DenyMinDownloadRate ) );
    AllCFGVars.push_back( "bot_maxdownloadtime@" + UTIL_ToQString( gUNM->m_DenyMaxDownloadTime ) );
    AllCFGVars.push_back( "bot_maxmapsizetime@" + UTIL_ToQString( gUNM->m_DenyMaxMapsizeTime ) );
    AllCFGVars.push_back( "bot_maxreqjointime@" + UTIL_ToQString( gUNM->m_DenyMaxReqjoinTime ) );
    AllCFGVars.push_back( "bot_maxloadtime@" + UTIL_ToQString( gUNM->m_DenyMaxLoadTime ) );
    AllCFGVars.push_back( gUNM->m_NonAdminCommandsGame ? "bot_nonadmincommandsgame@1" : "bot_nonadmincommandsgame@0" );
    AllCFGVars.push_back( gUNM->m_PlayerServerPrintJoin ? "bot_playerserverprintjoin@1" : "bot_playerserverprintjoin@0" );
    AllCFGVars.push_back( gUNM->m_FunCommandsGame ? "bot_funcommandsgame@1" : "bot_funcommandsgame@0" );
    AllCFGVars.push_back( gUNM->m_WarningPacketValidationFailed ? "bot_warningpacketvalidationfailed@1" : "bot_warningpacketvalidationfailed@0" );
    AllCFGVars.push_back( gUNM->m_ResourceTradePrint ? "bot_resourcetradeprint@1" : "bot_resourcetradeprint@0" );
    AllCFGVars.push_back( gUNM->m_PlayerBeforeStartPrint ? "bot_playerbeforestartprint@1" : "bot_playerbeforestartprint@0" );
    AllCFGVars.push_back( gUNM->m_PlayerBeforeStartPrintJoin ? "bot_playerbeforestartprintjoin@1" : "bot_playerbeforestartprintjoin@0" );
    AllCFGVars.push_back( gUNM->m_PlayerBeforeStartPrivatePrintJoin ? "bot_playerbeforestartprivateprintjoin@1" : "bot_playerbeforestartprivateprintjoin@0" );
    AllCFGVars.push_back( gUNM->m_PlayerBeforeStartPrivatePrintJoinSD ? "bot_playerbeforestartprivateprintjoinsd@1" : "bot_playerbeforestartprivateprintjoinsd@0" );
    AllCFGVars.push_back( gUNM->m_PlayerBeforeStartPrintLeave ? "bot_playerbeforestartprintleave@1" : "bot_playerbeforestartprintleave@0" );
    AllCFGVars.push_back( gUNM->m_BotChannelPrivatePrintJoin ? "bot_botchannelprivateprintjoin@1" : "bot_botchannelprivateprintjoin@0" );
    AllCFGVars.push_back( gUNM->m_BotNamePrivatePrintJoin ? "bot_botnameprivateprintjoin@1" : "bot_botnameprivateprintjoin@0" );
    AllCFGVars.push_back( gUNM->m_InfoGamesPrivatePrintJoin ? "bot_infogamesprivateprintjoin@1" : "bot_infogamesprivateprintjoin@0" );
    AllCFGVars.push_back( gUNM->m_GameNamePrivatePrintJoin ? "bot_gamenameprivateprintjoin@1" : "bot_gamenameprivateprintjoin@0" );
    AllCFGVars.push_back( gUNM->m_GameModePrivatePrintJoin ? "bot_gamemodeprivateprintjoin@1" : "bot_gamemodeprivateprintjoin@0" );
    AllCFGVars.push_back( gUNM->m_RehostPrinting ? "bot_rehostprinting@1" : "bot_rehostprinting@0" );
    AllCFGVars.push_back( gUNM->m_RehostFailedPrinting ? "bot_rehostfailedprinting@1" : "bot_rehostfailedprinting@0" );
    AllCFGVars.push_back( gUNM->m_PlayerFinishedLoadingPrinting ? "bot_playerfinishedloadingprinting@1" : "bot_playerfinishedloadingprinting@0" );
    AllCFGVars.push_back( gUNM->m_LobbyAnnounceUnoccupied ? "bot_lobbyannounceunoccupied@1" : "bot_lobbyannounceunoccupied@0" );
    AllCFGVars.push_back( gUNM->m_RelayChatCommands ? "bot_relaychatcommands@1" : "bot_relaychatcommands@0" );
    AllCFGVars.push_back( gUNM->m_DetourAllMessagesToAdmins ? "bot_detourallmessagestoadmins@1" : "bot_detourallmessagestoadmins@0" );
    AllCFGVars.push_back( gUNM->m_AdminMessages ? "bot_adminmessages@1" : "bot_adminmessages@0" );
    AllCFGVars.push_back( gUNM->m_LocalAdminMessages ? "bot_localadminmessages@1" : "bot_localadminmessages@0" );
    AllCFGVars.push_back( gUNM->m_WarnLatency ? "bot_warnLatency@1" : "bot_warnLatency@0" );
    AllCFGVars.push_back( gUNM->m_RefreshMessages ? "bot_refreshmessages@1" : "bot_refreshmessages@0" );
    AllCFGVars.push_back( "bot_gameannounce@" + UTIL_ToQString( gUNM->m_GameAnnounce ) );
    AllCFGVars.push_back( gUNM->m_ShowDownloadsInfo ? "bot_showdownloadsinfo@1" : "bot_showdownloadsinfo@0" );
    AllCFGVars.push_back( gUNM->m_ShowFinishDownloadingInfo ? "bot_showfinishdownloadinginfo@1" : "bot_showfinishdownloadinginfo@0" );
    AllCFGVars.push_back( "bot_showdownloadsinfotime@" + UTIL_ToQString( gUNM->m_ShowDownloadsInfoTime ) );
    AllCFGVars.push_back( gUNM->m_autoinsultlobby ? "bot_autoinsultlobby@1" : "bot_autoinsultlobby@0" );
    AllCFGVars.push_back( gUNM->m_autoinsultlobbysafeadmins ? "bot_autoinsultlobbysafeadmins@1" : "bot_autoinsultlobbysafeadmins@0" );
    AllCFGVars.push_back( "bot_gameownerprivateprintjoin@" + UTIL_ToQString( gUNM->m_GameOwnerPrivatePrintJoin ) );
    AllCFGVars.push_back( "bot_autounmuteprinting@" + UTIL_ToQString( gUNM->m_AutoUnMutePrinting ) );
    AllCFGVars.push_back( "bot_manualunmuteprinting@" + UTIL_ToQString( gUNM->m_ManualUnMutePrinting ) );
    AllCFGVars.push_back( "bot_gameisovermessageprinting@" + UTIL_ToQString( gUNM->m_GameIsOverMessagePrinting ) );
    AllCFGVars.push_back( "bot_manualhclmessage@" + UTIL_ToQString( gUNM->m_manualhclmessage ) );
    AllCFGVars.push_back( "bot_gameannounceinterval@" + UTIL_ToQString( gUNM->m_GameAnnounceInterval ) );
    AllCFGVars.push_back( "bot_nomapprintdelay@" + UTIL_ToQString( gUNM->m_NoMapPrintDelay ) );
    AllCFGVars.push_back( "bot_gameloadedprintout@" + UTIL_ToQString( gUNM->m_GameLoadedPrintout ) );
    AllCFGVars.push_back( gUNM->m_StillDownloadingPrint ? "bot_stilldownloadingprint@1" : "bot_stilldownloadingprint@0" );
    AllCFGVars.push_back( gUNM->m_NotSpoofCheckedPrint ? "bot_notspoofcheckedprint@1" : "bot_notspoofcheckedprint@0" );
    AllCFGVars.push_back( gUNM->m_NotPingedPrint ? "bot_notpingedprint@1" : "bot_notpingedprint@0" );
    AllCFGVars.push_back( gUNM->m_StillDownloadingASPrint ? "bot_stilldownloadingasprint@1" : "bot_stilldownloadingasprint@0" );
    AllCFGVars.push_back( gUNM->m_NotSpoofCheckedASPrint ? "bot_notspoofcheckedasprint@1" : "bot_notspoofcheckedasprint@0" );
    AllCFGVars.push_back( gUNM->m_NotPingedASPrint ? "bot_notpingedasprint@1" : "bot_notpingedasprint@0" );
    AllCFGVars.push_back( "bot_stilldownloadingasprintdelay@" + UTIL_ToQString( gUNM->m_StillDownloadingASPrintDelay ) );
    AllCFGVars.push_back( "bot_notspoofcheckedasprintdelay@" + UTIL_ToQString( gUNM->m_NotSpoofCheckedASPrintDelay ) );
    AllCFGVars.push_back( "bot_notpingedasprintdelay@" + UTIL_ToQString( gUNM->m_NotPingedASPrintDelay ) );
    AllCFGVars.push_back( "bot_playerbeforestartprintdelay@" + UTIL_ToQString( gUNM->m_PlayerBeforeStartPrintDelay ) );
    AllCFGVars.push_back( "bot_rehostprintingdelay@" + UTIL_ToQString( gUNM->m_RehostPrintingDelay ) );
    AllCFGVars.push_back( "bot_butgamedelay@" + UTIL_ToQString( gUNM->m_ButGameDelay ) );
    AllCFGVars.push_back( "bot_marsgamedelay@" + UTIL_ToQString( gUNM->m_MarsGameDelay ) );
    AllCFGVars.push_back( "bot_slapgamedelay@" + UTIL_ToQString( gUNM->m_SlapGameDelay ) );
    AllCFGVars.push_back( "bot_rollgamedelay@" + UTIL_ToQString( gUNM->m_RollGameDelay ) );
    AllCFGVars.push_back( "bot_picturegamedelay@" + UTIL_ToQString( gUNM->m_PictureGameDelay ) );
    AllCFGVars.push_back( "bot_eatgamedelay@" + UTIL_ToQString( gUNM->m_EatGameDelay ) );
    AllCFGVars.push_back( "bot_botchannelcustom@" + QString::fromUtf8( gUNM->m_BotChannelCustom.c_str( ) ) );
    AllCFGVars.push_back( "bot_botnamecustom@" + QString::fromUtf8( gUNM->m_BotNameCustom.c_str( ) ) );
    AllCFGVars.push_back( gUNM->m_AllowAutoRenameMaps ? "bot_allowautorenamemaps@1" : "bot_allowautorenamemaps@0" );
    AllCFGVars.push_back( gUNM->m_PlayersCanChangeRace ? "bot_playerscanchangerace@1" : "bot_playerscanchangerace@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapDataFromCFG ? "bot_overridemapdatafromcfg@1" : "bot_overridemapdatafromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapPathFromCFG ? "bot_overridemappathfromcfg@1" : "bot_overridemappathfromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapSizeFromCFG ? "bot_overridemapsizefromcfg@1" : "bot_overridemapsizefromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapInfoFromCFG ? "bot_overridemapinfofromcfg@1" : "bot_overridemapinfofromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapCrcFromCFG ? "bot_overridemapcrcfromcfg@1" : "bot_overridemapcrcfromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapSha1FromCFG ? "bot_overridemapsha1fromcfg@1" : "bot_overridemapsha1fromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapSpeedFromCFG ? "bot_overridemapspeedfromcfg@1" : "bot_overridemapspeedfromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapVisibilityFromCFG ? "bot_overridemapvisibilityfromcfg@1" : "bot_overridemapvisibilityfromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapFlagsFromCFG ? "bot_overridemapflagsfromcfg@1" : "bot_overridemapflagsfromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapFilterTypeFromCFG ? "bot_overridemapfiltertypefromcfg@1" : "bot_overridemapfiltertypefromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapGameTypeFromCFG ? "bot_overridemapgametypefromcfg@1" : "bot_overridemapgametypefromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapWidthFromCFG ? "bot_overridemapwidthfromcfg@1" : "bot_overridemapwidthfromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapHeightFromCFG ? "bot_overridemapheightfromcfg@1" : "bot_overridemapheightfromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapTypeFromCFG ? "bot_overridemaptypefromcfg@1" : "bot_overridemaptypefromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapDefaultHCLFromCFG ? "bot_overridemapdefaulthclfromcfg@1" : "bot_overridemapdefaulthclfromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapLoadInGameFromCFG ? "bot_overridemaploadingamefromcfg@1" : "bot_overridemaploadingamefromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapNumPlayersFromCFG ? "bot_overridemapnumplayersfromcfg@1" : "bot_overridemapnumplayersfromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapNumTeamsFromCFG ? "bot_overridemapnumteamsfromcfg@1" : "bot_overridemapnumteamsfromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapSlotsFromCFG ? "bot_overridemapslotsfromcfg@1" : "bot_overridemapslotsfromcfg@0" );
    AllCFGVars.push_back( gUNM->m_OverrideMapObserversFromCFG ? "bot_overridemapobserversfromcfg@1" : "bot_overridemapobserversfromcfg@0" );

    for( int32_t i = 0; i < WidgetsList.size( ); i++ )
    {
        ValueStart = AllCFGVars[i].indexOf( "@" );
        CFGVar = AllCFGVars[i].mid( 0, ValueStart );
        CFGValue = AllCFGVars[i].mid( ValueStart + 1 );

        if( CFGVar == LabelsList[i]->whatsThis( ) )
        {
            if( WidgetsList[i]->whatsThis( ) == "bool" )
                qobject_cast<QComboBox *>( WidgetsList[i] )->setCurrentIndex( CFGValue == "1" ? 0 : 1 );
            else if( WidgetsList[i]->whatsThis( ) == "intlimit" )
                qobject_cast<QComboBox *>( WidgetsList[i] )->setCurrentIndex( UTIL_QSToInt32( CFGValue ) );
            else if( WidgetsList[i]->whatsThis( ) == "intlimitfix1" )
                qobject_cast<QComboBox *>( WidgetsList[i] )->setCurrentIndex( UTIL_QSToInt32( CFGValue ) - 1 );
            else if( WidgetsList[i]->whatsThis( ) == "intunlimit" )
                qobject_cast<QSpinBox *>( WidgetsList[i] )->setValue( UTIL_QSToInt32( CFGValue ) );
            else if( WidgetsList[i]->whatsThis( ) == "string" )
                qobject_cast<QLineEdit *>( WidgetsList[i] )->setText( CFGValue );
        }
    }
}

void Widget::on_ShowPesettingWindowApply_clicked( )
{
    gUNM->m_ShowPesettingWindow = ui->ShowPesettingWindowCombo->currentIndex( );
}

void Widget::on_ShowPesettingWindowSave_clicked( )
{
    gUNM->m_ShowPesettingWindow = ui->ShowPesettingWindowCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "gui_showpesettingwindow", UTIL_ToString( gUNM->m_ShowPesettingWindow ) );
}

void Widget::on_ShowPesettingWindowRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ShowPesettingWindow = CFG.GetUInt( "gui_showpesettingwindow", 4, 5 );
    ui->ShowPesettingWindowCombo->setCurrentIndex( gUNM->m_ShowPesettingWindow );
}

void Widget::on_ShowPesettingWindowReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "gui_showpesettingwindow" );
    gUNM->m_ShowPesettingWindow = CFG.GetUInt( "gui_showpesettingwindow", 4, 5 );
    ui->ShowPesettingWindowCombo->setCurrentIndex( gUNM->m_ShowPesettingWindow );
}

void Widget::on_PrefixNameApply_clicked( )
{
    ui->PrefixNameCombo->currentIndex( ) == 0 ? gUNM->m_PrefixName = true : gUNM->m_PrefixName = false;
}

void Widget::on_PrefixNameSave_clicked( )
{
    if( ui->PrefixNameCombo->currentIndex( ) == 0 )
    {
        gUNM->m_PrefixName = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_realmprefixname", "1" );
    }
    else
    {
        gUNM->m_PrefixName = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_realmprefixname", "0" );
    }
}

void Widget::on_PrefixNameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PrefixName = CFG.GetInt( "bot_realmprefixname", 0 ) != 0;

    if( gUNM->m_PrefixName )
        ui->PrefixNameCombo->setCurrentIndex( 0 );
    else
        ui->PrefixNameCombo->setCurrentIndex( 1 );
}

void Widget::on_PrefixNameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_realmprefixname" );
    gUNM->m_PrefixName = CFG.GetInt( "bot_realmprefixname", 0 ) != 0;

    if( gUNM->m_PrefixName )
        ui->PrefixNameCombo->setCurrentIndex( 0 );
    else
        ui->PrefixNameCombo->setCurrentIndex( 1 );
}

void Widget::on_EnableLoggingToGUIApply_clicked( )
{
    gEnableLoggingToGUI = ui->EnableLoggingToGUICombo->currentIndex( );
}

void Widget::on_EnableLoggingToGUISave_clicked( )
{
    gEnableLoggingToGUI = ui->EnableLoggingToGUICombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_enableloggingtogui", UTIL_ToString( gEnableLoggingToGUI ) );
}

void Widget::on_EnableLoggingToGUIRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gEnableLoggingToGUI = CFG.GetUInt( "bot_enableloggingtogui", 1, 2 );
    ui->EnableLoggingToGUICombo->setCurrentIndex( gEnableLoggingToGUI );
}

void Widget::on_EnableLoggingToGUIReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_enableloggingtogui" );
    gEnableLoggingToGUI = CFG.GetUInt( "bot_enableloggingtogui", 1, 2 );
    ui->EnableLoggingToGUICombo->setCurrentIndex( gEnableLoggingToGUI );
}

void Widget::on_LogReductionApply_clicked( )
{
    ui->LogReductionCombo->currentIndex( ) == 0 ? gUNM->m_LogReduction = true : gUNM->m_LogReduction = false;
}

void Widget::on_LogReductionSave_clicked( )
{
    if( ui->LogReductionCombo->currentIndex( ) == 0 )
    {
        gUNM->m_LogReduction = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_logreduction", "1" );
    }
    else
    {
        gUNM->m_LogReduction = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_logreduction", "0" );
    }
}

void Widget::on_LogReductionRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_LogReduction = CFG.GetInt( "bot_logreduction", 0 ) != 0;

    if( gUNM->m_LogReduction )
        ui->LogReductionCombo->setCurrentIndex( 0 );
    else
        ui->LogReductionCombo->setCurrentIndex( 1 );
}

void Widget::on_LogReductionReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_logreduction" );
    gUNM->m_LogReduction = CFG.GetInt( "bot_logreduction", 0 ) != 0;

    if( gUNM->m_LogReduction )
        ui->LogReductionCombo->setCurrentIndex( 0 );
    else
        ui->LogReductionCombo->setCurrentIndex( 1 );
}

void Widget::on_MaxLogSizeApply_clicked( )
{
    gUNM->m_MaxLogSize = ui->MaxLogSizeSpin->value( );
}

void Widget::on_MaxLogSizeSave_clicked( )
{
    gUNM->m_MaxLogSize = ui->MaxLogSizeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maxlogsize", UTIL_ToString( gUNM->m_MaxLogSize ) );
}

void Widget::on_MaxLogSizeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MaxLogSize = CFG.GetInt64( "bot_maxlogsize", 5000000 );
    ui->MaxLogSizeSpin->setValue( gUNM->m_MaxLogSize );
}

void Widget::on_MaxLogSizeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maxlogsize" );
    gUNM->m_MaxLogSize = CFG.GetInt64( "bot_maxlogsize", 5000000 );
    ui->MaxLogSizeSpin->setValue( gUNM->m_MaxLogSize );
}

void Widget::on_MaxLogTimeApply_clicked( )
{
    gUNM->m_MaxLogTime = ui->MaxLogTimeSpin->value( );
}

void Widget::on_MaxLogTimeSave_clicked( )
{
    gUNM->m_MaxLogTime = ui->MaxLogTimeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maxlogtime", UTIL_ToString( gUNM->m_MaxLogTime ) );
}

void Widget::on_MaxLogTimeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MaxLogTime = CFG.GetUInt( "bot_maxlogtime", 0 );
    ui->MaxLogTimeSpin->setValue( gUNM->m_MaxLogTime );
}

void Widget::on_MaxLogTimeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maxlogtime" );
    gUNM->m_MaxLogTime = CFG.GetUInt( "bot_maxlogtime", 0 );
    ui->MaxLogTimeSpin->setValue( gUNM->m_MaxLogTime );
}

void Widget::on_MinLengthGNApply_clicked( )
{
    gUNM->m_MinLengthGN = ui->MinLengthGNSpin->value( );
}

void Widget::on_MinLengthGNSave_clicked( )
{
    gUNM->m_MinLengthGN = ui->MinLengthGNSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_minlengthgn", UTIL_ToString( gUNM->m_MinLengthGN ) );
}

void Widget::on_MinLengthGNRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MinLengthGN = CFG.GetUInt( "bot_minlengthgn", 1 );
    ui->MinLengthGNSpin->setValue( gUNM->m_MinLengthGN );
}

void Widget::on_MinLengthGNReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_minlengthgn" );
    gUNM->m_MinLengthGN = CFG.GetUInt( "bot_minlengthgn", 1 );
    ui->MinLengthGNSpin->setValue( gUNM->m_MinLengthGN );
}

void Widget::on_SlotInfoInGameNameApply_clicked( )
{
    gUNM->m_SlotInfoInGameName = ui->SlotInfoInGameNameSpin->value( );
}

void Widget::on_SlotInfoInGameNameSave_clicked( )
{
    gUNM->m_SlotInfoInGameName = ui->SlotInfoInGameNameSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_slotinfoingamename", UTIL_ToString( gUNM->m_SlotInfoInGameName ) );
}

void Widget::on_SlotInfoInGameNameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_SlotInfoInGameName = CFG.GetUInt( "bot_slotinfoingamename", 1 );
    ui->SlotInfoInGameNameSpin->setValue( gUNM->m_SlotInfoInGameName );
}

void Widget::on_SlotInfoInGameNameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_slotinfoingamename" );
    gUNM->m_SlotInfoInGameName = CFG.GetUInt( "bot_slotinfoingamename", 1 );
    ui->SlotInfoInGameNameSpin->setValue( gUNM->m_SlotInfoInGameName );
}

void Widget::on_MaxGamesApply_clicked( )
{
    gUNM->m_MaxGames = ui->MaxGamesSpin->value( );
}

void Widget::on_MaxGamesSave_clicked( )
{
    gUNM->m_MaxGames = ui->MaxGamesSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maxgames", UTIL_ToString( gUNM->m_MaxGames ) );
}

void Widget::on_MaxGamesRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MaxGames = CFG.GetUInt( "bot_maxgames", 15 );
    ui->MaxGamesSpin->setValue( gUNM->m_MaxGames );
}

void Widget::on_MaxGamesReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maxgames" );
    gUNM->m_MaxGames = CFG.GetUInt( "bot_maxgames", 15 );
    ui->MaxGamesSpin->setValue( gUNM->m_MaxGames );
}

void Widget::on_patch23Apply_clicked( )
{
    ui->patch23Combo->currentIndex( ) == 0 ? gUNM->m_patch23 = true : gUNM->m_patch23 = false;
}

void Widget::on_patch23Save_clicked( )
{
    if( ui->patch23Combo->currentIndex( ) == 0 )
    {
        gUNM->m_patch23 = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_patch23ornewer", "1" );
    }
    else
    {
        gUNM->m_patch23 = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_patch23ornewer", "0" );
    }
}

void Widget::on_patch23Refresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_patch23 = CFG.GetInt( "bot_patch23ornewer", 1 ) != 0;

    if( gUNM->m_patch23 )
        ui->patch23Combo->setCurrentIndex( 0 );
    else
        ui->patch23Combo->setCurrentIndex( 1 );
}

void Widget::on_patch23Reset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_patch23ornewer" );
    gUNM->m_patch23 = CFG.GetInt( "bot_patch23ornewer", 1 ) != 0;

    if( gUNM->m_patch23 )
        ui->patch23Combo->setCurrentIndex( 0 );
    else
        ui->patch23Combo->setCurrentIndex( 1 );
}

void Widget::on_patch21Apply_clicked( )
{
    ui->patch21Combo->currentIndex( ) == 0 ? gUNM->m_patch21 = true : gUNM->m_patch21 = false;
}

void Widget::on_patch21Save_clicked( )
{
    if( ui->patch21Combo->currentIndex( ) == 0 )
    {
        gUNM->m_patch21 = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_patch21", "1" );
    }
    else
    {
        gUNM->m_patch21 = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_patch21", "0" );
    }
}

void Widget::on_patch21Refresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_patch21 = CFG.GetInt( "bot_patch21", 0 ) != 0;

    if( gUNM->m_patch21 )
        ui->patch21Combo->setCurrentIndex( 0 );
    else
        ui->patch21Combo->setCurrentIndex( 1 );
}

void Widget::on_patch21Reset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_patch21" );
    gUNM->m_patch21 = CFG.GetInt( "bot_patch21", 0 ) != 0;

    if( gUNM->m_patch21 )
        ui->patch21Combo->setCurrentIndex( 0 );
    else
        ui->patch21Combo->setCurrentIndex( 1 );
}

void Widget::on_broadcastinlanApply_clicked( )
{
    ui->broadcastinlanCombo->currentIndex( ) == 0 ? gUNM->m_broadcastinlan = true : gUNM->m_broadcastinlan = false;
}

void Widget::on_broadcastinlanSave_clicked( )
{
    if( ui->broadcastinlanCombo->currentIndex( ) == 0 )
    {
        gUNM->m_broadcastinlan = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_broadcastlan", "1" );
    }
    else
    {
        gUNM->m_broadcastinlan = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_broadcastlan", "0" );
    }
}

void Widget::on_broadcastinlanRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_broadcastinlan = CFG.GetInt( "bot_broadcastlan", 1 ) != 0;

    if( gUNM->m_broadcastinlan )
        ui->broadcastinlanCombo->setCurrentIndex( 0 );
    else
        ui->broadcastinlanCombo->setCurrentIndex( 1 );
}

void Widget::on_broadcastinlanReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_broadcastlan" );
    gUNM->m_broadcastinlan = CFG.GetInt( "bot_broadcastlan", 1 ) != 0;

    if( gUNM->m_broadcastinlan )
        ui->broadcastinlanCombo->setCurrentIndex( 0 );
    else
        ui->broadcastinlanCombo->setCurrentIndex( 1 );
}

void Widget::on_LANWar3VersionApply_clicked( )
{
    gUNM->m_LANWar3Version = static_cast<unsigned char>( ui->LANWar3VersionSpin->value( ) );
}

void Widget::on_LANWar3VersionSave_clicked( )
{
    gUNM->m_LANWar3Version = static_cast<unsigned char>( ui->LANWar3VersionSpin->value( ) );
    UTIL_SetVarInFile( "unm.cfg", "lan_war3version", UTIL_ToString( gUNM->m_LANWar3Version ) );
}

void Widget::on_LANWar3VersionRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_LANWar3Version = static_cast<unsigned char>( CFG.GetUInt( "lan_war3version", 26 ) );
    ui->LANWar3VersionSpin->setValue( gUNM->m_LANWar3Version );
}

void Widget::on_LANWar3VersionReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "lan_war3version" );
    gUNM->m_LANWar3Version = static_cast<unsigned char>( CFG.GetUInt( "lan_war3version", 26 ) );
    ui->LANWar3VersionSpin->setValue( gUNM->m_LANWar3Version );
}

void Widget::on_ShowRealSlotCountApply_clicked( )
{
    ui->ShowRealSlotCountCombo->currentIndex( ) == 0 ? gUNM->m_ShowRealSlotCount = true : gUNM->m_ShowRealSlotCount = false;
}

void Widget::on_ShowRealSlotCountSave_clicked( )
{
    if( ui->ShowRealSlotCountCombo->currentIndex( ) == 0 )
    {
        gUNM->m_ShowRealSlotCount = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_showrealslotcountonlan", "1" );
    }
    else
    {
        gUNM->m_ShowRealSlotCount = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_showrealslotcountonlan", "0" );
    }
}

void Widget::on_ShowRealSlotCountRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ShowRealSlotCount = CFG.GetInt( "bot_showrealslotcountonlan", 1 ) != 0;

    if( gUNM->m_ShowRealSlotCount )
        ui->ShowRealSlotCountCombo->setCurrentIndex( 0 );
    else
        ui->ShowRealSlotCountCombo->setCurrentIndex( 1 );
}

void Widget::on_ShowRealSlotCountReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_showrealslotcountonlan" );
    gUNM->m_ShowRealSlotCount = CFG.GetInt( "bot_showrealslotcountonlan", 1 ) != 0;

    if( gUNM->m_ShowRealSlotCount )
        ui->ShowRealSlotCountCombo->setCurrentIndex( 0 );
    else
        ui->ShowRealSlotCountCombo->setCurrentIndex( 1 );
}

void Widget::on_BroadcastHostNameApply_clicked( )
{
    gUNM->m_BroadcastHostName = ui->BroadcastHostNameCombo->currentIndex( );
}

void Widget::on_BroadcastHostNameSave_clicked( )
{
    gUNM->m_BroadcastHostName = ui->BroadcastHostNameCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_broadcasthostname", UTIL_ToString( gUNM->m_BroadcastHostName ) );
}

void Widget::on_BroadcastHostNameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_BroadcastHostName = CFG.GetUInt( "bot_broadcasthostname", 0, 3 );
    ui->BroadcastHostNameCombo->setCurrentIndex( gUNM->m_BroadcastHostName );
}

void Widget::on_BroadcastHostNameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_broadcasthostname" );
    gUNM->m_BroadcastHostName = CFG.GetUInt( "bot_broadcasthostname", 0, 3 );
    ui->BroadcastHostNameCombo->setCurrentIndex( gUNM->m_BroadcastHostName );
}

void Widget::on_BroadcastHostNameCustomLine_returnPressed( )
{
    on_BroadcastHostNameCustomApply_clicked( );
}

void Widget::on_BroadcastHostNameCustomApply_clicked( )
{
    gUNM->m_BroadcastHostNameCustom = ui->BroadcastHostNameCustomLine->text( ).toStdString( );
}

void Widget::on_BroadcastHostNameCustomSave_clicked( )
{
    gUNM->m_BroadcastHostNameCustom = ui->BroadcastHostNameCustomLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_broadcasthncustom", gUNM->m_BroadcastHostNameCustom );
}

void Widget::on_BroadcastHostNameCustomRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_BroadcastHostNameCustom = CFG.GetString( "bot_broadcasthncustom", gUNM->m_VirtualHostName );
    ui->BroadcastHostNameCustomLine->setText( QString::fromUtf8( gUNM->m_BroadcastHostNameCustom.c_str( ) ) );
}

void Widget::on_BroadcastHostNameCustomReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_broadcasthncustom" );
    gUNM->m_BroadcastHostNameCustom = CFG.GetString( "bot_broadcasthncustom", gUNM->m_VirtualHostName );
    ui->BroadcastHostNameCustomLine->setText( QString::fromUtf8( gUNM->m_BroadcastHostNameCustom.c_str( ) ) );
}

void Widget::on_BroadcastPortApply_clicked( )
{
    gUNM->m_BroadcastPort = ui->BroadcastPortSpin->value( );
}

void Widget::on_BroadcastPortSave_clicked( )
{
    gUNM->m_BroadcastPort = ui->BroadcastPortSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_broadcastport", UTIL_ToString( gUNM->m_BroadcastPort ) );
}

void Widget::on_BroadcastPortRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_BroadcastPort = CFG.GetUInt16( "bot_broadcastport", 6112 );
    ui->BroadcastPortSpin->setValue( gUNM->m_BroadcastPort );
}

void Widget::on_BroadcastPortReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_broadcastport" );
    gUNM->m_BroadcastPort = CFG.GetUInt16( "bot_broadcastport", 6112 );
    ui->BroadcastPortSpin->setValue( gUNM->m_BroadcastPort );
}

void Widget::on_HostPortApply_clicked( )
{
    gUNM->m_HostPort = ui->HostPortSpin->value( );
}

void Widget::on_HostPortSave_clicked( )
{
    gUNM->m_HostPort = ui->HostPortSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_hostport", UTIL_ToString( gUNM->m_HostPort ) );
}

void Widget::on_HostPortRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_HostPort = CFG.GetUInt16( "bot_hostport", 6113 );
    ui->HostPortSpin->setValue( gUNM->m_HostPort );
}

void Widget::on_HostPortReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_hostport" );
    gUNM->m_HostPort = CFG.GetUInt16( "bot_hostport", 6113 );
    ui->HostPortSpin->setValue( gUNM->m_HostPort );
}

void Widget::on_DefaultMapLine_returnPressed( )
{
    on_DefaultMapApply_clicked( );
}

void Widget::on_DefaultMapApply_clicked( )
{
    gUNM->m_DefaultMap = ui->DefaultMapLine->text( ).toStdString( );
}

void Widget::on_DefaultMapSave_clicked( )
{
    gUNM->m_DefaultMap = ui->DefaultMapLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_defaultmap", gUNM->m_DefaultMap );
}

void Widget::on_DefaultMapRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DefaultMap = CFG.GetString( "bot_defaultmap", "IER 1.66a.cfg" );
    ui->DefaultMapLine->setText( QString::fromUtf8( gUNM->m_DefaultMap.c_str( ) ) );
}

void Widget::on_DefaultMapReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_defaultmap" );
    gUNM->m_DefaultMap = CFG.GetString( "bot_defaultmap", "IER 1.66a.cfg" );
    ui->DefaultMapLine->setText( QString::fromUtf8( gUNM->m_DefaultMap.c_str( ) ) );
}

void Widget::on_BindAddressLine_returnPressed( )
{
    on_BindAddressApply_clicked( );
}

void Widget::on_BindAddressApply_clicked( )
{
    gUNM->m_BindAddress = ui->BindAddressLine->text( ).toStdString( );
}

void Widget::on_BindAddressSave_clicked( )
{
    gUNM->m_BindAddress = ui->BindAddressLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_bindaddress", gUNM->m_BindAddress );
}

void Widget::on_BindAddressRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_BindAddress = CFG.GetString( "bot_bindaddress", string( ) );
    ui->BindAddressLine->setText( QString::fromUtf8( gUNM->m_BindAddress.c_str( ) ) );
}

void Widget::on_BindAddressReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_bindaddress" );
    gUNM->m_BindAddress = CFG.GetString( "bot_bindaddress", string( ) );
    ui->BindAddressLine->setText( QString::fromUtf8( gUNM->m_BindAddress.c_str( ) ) );
}

void Widget::on_HideIPAddressesApply_clicked( )
{
    ui->HideIPAddressesCombo->currentIndex( ) == 0 ? gUNM->m_HideIPAddresses = true : gUNM->m_HideIPAddresses = false;
}

void Widget::on_HideIPAddressesSave_clicked( )
{
    if( ui->HideIPAddressesCombo->currentIndex( ) == 0 )
    {
        gUNM->m_HideIPAddresses = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_hideipaddresses", "1" );
    }
    else
    {
        gUNM->m_HideIPAddresses = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_hideipaddresses", "0" );
    }
}

void Widget::on_HideIPAddressesRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_HideIPAddresses = CFG.GetInt( "bot_hideipaddresses", 0 ) != 0;

    if( gUNM->m_HideIPAddresses )
        ui->HideIPAddressesCombo->setCurrentIndex( 0 );
    else
        ui->HideIPAddressesCombo->setCurrentIndex( 1 );
}

void Widget::on_HideIPAddressesReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_hideipaddresses" );
    gUNM->m_HideIPAddresses = CFG.GetInt( "bot_hideipaddresses", 0 ) != 0;

    if( gUNM->m_HideIPAddresses )
        ui->HideIPAddressesCombo->setCurrentIndex( 0 );
    else
        ui->HideIPAddressesCombo->setCurrentIndex( 1 );
}

void Widget::on_LanguageFileLine_returnPressed( )
{
    on_LanguageFileApply_clicked( );
}

void Widget::on_LanguageFileApply_clicked( )
{
    gLanguageFile = UTIL_FixFilePath( ui->LanguageFileLine->text( ).toStdString( ) );

    if( gLanguage )
        delete gLanguage;

    gLanguage = new CLanguage( gLanguageFile );
}

void Widget::on_LanguageFileSave_clicked( )
{
    gLanguageFile = UTIL_FixFilePath( ui->LanguageFileLine->text( ).toStdString( ) );

    if( gLanguage )
        delete gLanguage;

    gLanguage = new CLanguage( gLanguageFile );
    UTIL_SetVarInFile( "unm.cfg", "bot_language", gLanguageFile );
}

void Widget::on_LanguageFileRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gLanguageFile = UTIL_FixFilePath( CFG.GetString( "bot_language", "text_files\\Languages\\Russian.cfg" ) );

    if( gLanguage )
        delete gLanguage;

    gLanguage = new CLanguage( gLanguageFile );
    ui->LanguageFileLine->setText( QString::fromUtf8( gLanguageFile.c_str( ) ) );
}

void Widget::on_LanguageFileReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_language" );
    gLanguageFile = UTIL_FixFilePath( CFG.GetString( "bot_language", "text_files\\Languages\\Russian.cfg" ) );

    if( gLanguage )
        delete gLanguage;

    gLanguage = new CLanguage( gLanguageFile );
    ui->LanguageFileLine->setText( QString::fromUtf8( gLanguageFile.c_str( ) ) );
}

void Widget::on_Warcraft3PathLine_returnPressed( )
{
    on_Warcraft3PathApply_clicked( );
}

void Widget::on_Warcraft3PathApply_clicked( )
{
    gUNM->m_Warcraft3Path = UTIL_FixPath( ui->Warcraft3PathLine->text( ).toStdString( ) );
}

void Widget::on_Warcraft3PathSave_clicked( )
{
    gUNM->m_Warcraft3Path = UTIL_FixPath( ui->Warcraft3PathLine->text( ).toStdString( ) );
    UTIL_SetVarInFile( "unm.cfg", "bot_war3path", gUNM->m_Warcraft3Path );
}

void Widget::on_Warcraft3PathRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_Warcraft3Path = UTIL_FixPath( CFG.GetString( "bot_war3path", "war3\\" ) );
    ui->Warcraft3PathLine->setText( QString::fromUtf8( gUNM->m_Warcraft3Path.c_str( ) ) );
}

void Widget::on_Warcraft3PathReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_war3path" );
    gUNM->m_Warcraft3Path = UTIL_FixPath( CFG.GetString( "bot_war3path", "war3\\" ) );
    ui->Warcraft3PathLine->setText( QString::fromUtf8( gUNM->m_Warcraft3Path.c_str( ) ) );
}

void Widget::on_MapCFGPathLine_returnPressed( )
{
    on_MapCFGPathApply_clicked( );
}

void Widget::on_MapCFGPathApply_clicked( )
{
    gUNM->m_MapCFGPath = UTIL_FixPath( ui->MapCFGPathLine->text( ).toStdString( ) );
}

void Widget::on_MapCFGPathSave_clicked( )
{
    gUNM->m_MapCFGPath = UTIL_FixPath( ui->MapCFGPathLine->text( ).toStdString( ) );
    UTIL_SetVarInFile( "unm.cfg", "bot_mapcfgpath", gUNM->m_MapCFGPath );
}

void Widget::on_MapCFGPathRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MapCFGPath = UTIL_FixPath( CFG.GetString( "bot_mapcfgpath", "mapcfgs\\" ) );
    ui->MapCFGPathLine->setText( QString::fromUtf8( gUNM->m_MapCFGPath.c_str( ) ) );
}

void Widget::on_MapCFGPathReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_mapcfgpath" );
    gUNM->m_MapCFGPath = UTIL_FixPath( CFG.GetString( "bot_mapcfgpath", "mapcfgs\\" ) );
    ui->MapCFGPathLine->setText( QString::fromUtf8( gUNM->m_MapCFGPath.c_str( ) ) );
}

void Widget::on_SaveGamePathLine_returnPressed( )
{
    on_SaveGamePathApply_clicked( );
}

void Widget::on_SaveGamePathApply_clicked( )
{
    gUNM->m_SaveGamePath = UTIL_FixPath( ui->SaveGamePathLine->text( ).toStdString( ) );
}

void Widget::on_SaveGamePathSave_clicked( )
{
    gUNM->m_SaveGamePath = UTIL_FixPath( ui->SaveGamePathLine->text( ).toStdString( ) );
    UTIL_SetVarInFile( "unm.cfg", "bot_savegamepath", gUNM->m_SaveGamePath );
}

void Widget::on_SaveGamePathRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_SaveGamePath = UTIL_FixPath( CFG.GetString( "bot_savegamepath", "savegames\\" ) );
    ui->SaveGamePathLine->setText( QString::fromUtf8( gUNM->m_SaveGamePath.c_str( ) ) );
}

void Widget::on_SaveGamePathReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_savegamepath" );
    gUNM->m_SaveGamePath = UTIL_FixPath( CFG.GetString( "bot_savegamepath", "savegames\\" ) );
    ui->SaveGamePathLine->setText( QString::fromUtf8( gUNM->m_SaveGamePath.c_str( ) ) );
}

void Widget::on_MapPathLine_returnPressed( )
{
    on_MapPathApply_clicked( );
}

void Widget::on_MapPathApply_clicked( )
{
    gUNM->m_MapPath = UTIL_FixPath( ui->MapPathLine->text( ).toStdString( ) );
}

void Widget::on_MapPathSave_clicked( )
{
    gUNM->m_MapPath = UTIL_FixPath( ui->MapPathLine->text( ).toStdString( ) );
    UTIL_SetVarInFile( "unm.cfg", "bot_mappath", gUNM->m_MapPath );
}

void Widget::on_MapPathRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MapPath = UTIL_FixPath( CFG.GetString( "bot_mappath", "maps\\" ) );
    ui->MapPathLine->setText( QString::fromUtf8( gUNM->m_MapPath.c_str( ) ) );
}

void Widget::on_MapPathReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_mappath" );
    gUNM->m_MapPath = UTIL_FixPath( CFG.GetString( "bot_mappath", "maps\\" ) );
    ui->MapPathLine->setText( QString::fromUtf8( gUNM->m_MapPath.c_str( ) ) );
}

void Widget::on_ReplayPathLine_returnPressed( )
{
    on_ReplayPathApply_clicked( );
}

void Widget::on_ReplayPathApply_clicked( )
{
    gUNM->m_ReplayPath = UTIL_FixPath( ui->ReplayPathLine->text( ).toStdString( ) );
}

void Widget::on_ReplayPathSave_clicked( )
{
    gUNM->m_ReplayPath = UTIL_FixPath( ui->ReplayPathLine->text( ).toStdString( ) );
    UTIL_SetVarInFile( "unm.cfg", "bot_replaypath", gUNM->m_ReplayPath );
}

void Widget::on_ReplayPathRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ReplayPath = UTIL_FixPath( CFG.GetString( "bot_replaypath", "replays\\" ) );
    ui->ReplayPathLine->setText( QString::fromUtf8( gUNM->m_ReplayPath.c_str( ) ) );
}

void Widget::on_ReplayPathReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_replaypath" );
    gUNM->m_ReplayPath = UTIL_FixPath( CFG.GetString( "bot_replaypath", "replays\\" ) );
    ui->ReplayPathLine->setText( QString::fromUtf8( gUNM->m_ReplayPath.c_str( ) ) );
}

void Widget::on_SaveReplaysApply_clicked( )
{
    ui->SaveReplaysCombo->currentIndex( ) == 0 ? gUNM->m_SaveReplays = true : gUNM->m_SaveReplays = false;
}

void Widget::on_SaveReplaysSave_clicked( )
{
    if( ui->SaveReplaysCombo->currentIndex( ) == 0 )
    {
        gUNM->m_SaveReplays = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_savereplays", "1" );
    }
    else
    {
        gUNM->m_SaveReplays = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_savereplays", "0" );
    }
}

void Widget::on_SaveReplaysRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_SaveReplays = CFG.GetInt( "bot_savereplays", 0 ) != 0;

    if( gUNM->m_SaveReplays )
        ui->SaveReplaysCombo->setCurrentIndex( 0 );
    else
        ui->SaveReplaysCombo->setCurrentIndex( 1 );
}

void Widget::on_SaveReplaysReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_savereplays" );
    gUNM->m_SaveReplays = CFG.GetInt( "bot_savereplays", 0 ) != 0;

    if( gUNM->m_SaveReplays )
        ui->SaveReplaysCombo->setCurrentIndex( 0 );
    else
        ui->SaveReplaysCombo->setCurrentIndex( 1 );
}

void Widget::on_ReplayBuildNumberApply_clicked( )
{
    gUNM->m_ReplayBuildNumber = ui->ReplayBuildNumberSpin->value( );
}

void Widget::on_ReplayBuildNumberSave_clicked( )
{
    gUNM->m_ReplayBuildNumber = ui->ReplayBuildNumberSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "replay_buildnumber", UTIL_ToString( gUNM->m_ReplayBuildNumber ) );
}

void Widget::on_ReplayBuildNumberRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ReplayBuildNumber = CFG.GetUInt16( "replay_buildnumber", 6059 );
    ui->ReplayBuildNumberSpin->setValue( gUNM->m_ReplayBuildNumber );
}

void Widget::on_ReplayBuildNumberReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "replay_buildnumber" );
    gUNM->m_ReplayBuildNumber = CFG.GetUInt16( "replay_buildnumber", 6059 );
    ui->ReplayBuildNumberSpin->setValue( gUNM->m_ReplayBuildNumber );
}

void Widget::on_ReplayWar3VersionApply_clicked( )
{
    gUNM->m_ReplayWar3Version = ui->ReplayWar3VersionSpin->value( );
}

void Widget::on_ReplayWar3VersionSave_clicked( )
{
    gUNM->m_ReplayWar3Version = ui->ReplayWar3VersionSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "replay_war3version", UTIL_ToString( gUNM->m_ReplayWar3Version ) );
}

void Widget::on_ReplayWar3VersionRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ReplayWar3Version = CFG.GetUInt( "replay_war3version", 26 );
    ui->ReplayWar3VersionSpin->setValue( gUNM->m_ReplayWar3Version );
}

void Widget::on_ReplayWar3VersionReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "replay_war3version" );
    gUNM->m_ReplayWar3Version = CFG.GetUInt( "replay_war3version", 26 );
    ui->ReplayWar3VersionSpin->setValue( gUNM->m_ReplayWar3Version );
}

void Widget::on_ReplaysByNameApply_clicked( )
{
    ui->ReplaysByNameCombo->currentIndex( ) == 0 ? gUNM->m_ReplaysByName = true : gUNM->m_ReplaysByName = false;
}

void Widget::on_ReplaysByNameSave_clicked( )
{
    if( ui->ReplaysByNameCombo->currentIndex( ) == 0 )
    {
        gUNM->m_ReplaysByName = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_replayssavedbyname", "1" );
    }
    else
    {
        gUNM->m_ReplaysByName = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_replayssavedbyname", "0" );
    }
}

void Widget::on_ReplaysByNameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ReplaysByName = CFG.GetInt( "bot_replayssavedbyname", 1 ) != 0;

    if( gUNM->m_ReplaysByName )
        ui->ReplaysByNameCombo->setCurrentIndex( 0 );
    else
        ui->ReplaysByNameCombo->setCurrentIndex( 1 );
}

void Widget::on_ReplaysByNameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_replayssavedbyname" );
    gUNM->m_ReplaysByName = CFG.GetInt( "bot_replayssavedbyname", 1 ) != 0;

    if( gUNM->m_ReplaysByName )
        ui->ReplaysByNameCombo->setCurrentIndex( 0 );
    else
        ui->ReplaysByNameCombo->setCurrentIndex( 1 );
}

void Widget::on_IPBlackListFileLine_returnPressed( )
{
    on_IPBlackListFileApply_clicked( );
}

void Widget::on_IPBlackListFileApply_clicked( )
{
    gUNM->m_IPBlackListFile = UTIL_FixFilePath( ui->IPBlackListFileLine->text( ).toStdString( ) );
}

void Widget::on_IPBlackListFileSave_clicked( )
{
    gUNM->m_IPBlackListFile = UTIL_FixFilePath( ui->IPBlackListFileLine->text( ).toStdString( ) );
    UTIL_SetVarInFile( "unm.cfg", "bot_ipblacklistfile", gUNM->m_IPBlackListFile );
}

void Widget::on_IPBlackListFileRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_IPBlackListFile = UTIL_FixFilePath( CFG.GetString( "bot_ipblacklistfile", "text_files\\ipblacklist.txt" ) );
    ui->IPBlackListFileLine->setText( QString::fromUtf8( gUNM->m_IPBlackListFile.c_str( ) ) );
}

void Widget::on_IPBlackListFileReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_ipblacklistfile" );
    gUNM->m_IPBlackListFile = UTIL_FixFilePath( CFG.GetString( "bot_ipblacklistfile", "text_files\\ipblacklist.txt" ) );
    ui->IPBlackListFileLine->setText( QString::fromUtf8( gUNM->m_IPBlackListFile.c_str( ) ) );
}

void Widget::on_gamestateinhouseApply_clicked( )
{
    gUNM->m_gamestateinhouse = ui->gamestateinhouseSpin->value( );
}

void Widget::on_gamestateinhouseSave_clicked( )
{
    gUNM->m_gamestateinhouse = ui->gamestateinhouseSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_gamestateinhouse", UTIL_ToString( gUNM->m_gamestateinhouse ) );
}

void Widget::on_gamestateinhouseRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_gamestateinhouse = CFG.GetUInt( "bot_gamestateinhouse", 999 );
    ui->gamestateinhouseSpin->setValue( gUNM->m_gamestateinhouse );
}

void Widget::on_gamestateinhouseReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gamestateinhouse" );
    gUNM->m_gamestateinhouse = CFG.GetUInt( "bot_gamestateinhouse", 999 );
    ui->gamestateinhouseSpin->setValue( gUNM->m_gamestateinhouse );
}

void Widget::on_InvalidTriggersGameLine_returnPressed( )
{
    on_InvalidTriggersGameApply_clicked( );
}

void Widget::on_InvalidTriggersGameApply_clicked( )
{
    gUNM->m_InvalidTriggersGame = ui->InvalidTriggersGameLine->text( ).toStdString( );
}

void Widget::on_InvalidTriggersGameSave_clicked( )
{
    gUNM->m_InvalidTriggersGame = ui->InvalidTriggersGameLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_invalidtriggersgame", gUNM->m_InvalidTriggersGame );
}

void Widget::on_InvalidTriggersGameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_InvalidTriggersGame = CFG.GetString( "bot_invalidtriggersgame", string( ) );
    ui->InvalidTriggersGameLine->setText( QString::fromUtf8( gUNM->m_InvalidTriggersGame.c_str( ) ) );
}

void Widget::on_InvalidTriggersGameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_invalidtriggersgame" );
    gUNM->m_InvalidTriggersGame = CFG.GetString( "bot_invalidtriggersgame", string( ) );
    ui->InvalidTriggersGameLine->setText( QString::fromUtf8( gUNM->m_InvalidTriggersGame.c_str( ) ) );
}

void Widget::on_InvalidReplayCharsLine_returnPressed( )
{
    on_InvalidReplayCharsApply_clicked( );
}

void Widget::on_InvalidReplayCharsApply_clicked( )
{
    gUNM->m_InvalidReplayChars = ui->InvalidReplayCharsLine->text( ).toStdString( );
}

void Widget::on_InvalidReplayCharsSave_clicked( )
{
    gUNM->m_InvalidReplayChars = ui->InvalidReplayCharsLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_invalidreplaychars", gUNM->m_InvalidReplayChars );
}

void Widget::on_InvalidReplayCharsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_InvalidReplayChars = CFG.GetString( "bot_invalidreplaychars", "/|\\:*?\"<>" );
    ui->InvalidReplayCharsLine->setText( QString::fromUtf8( gUNM->m_InvalidReplayChars.c_str( ) ) );
}

void Widget::on_InvalidReplayCharsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_invalidreplaychars" );
    gUNM->m_InvalidReplayChars = CFG.GetString( "bot_invalidreplaychars", "/|\\:*?\"<>" );
    ui->InvalidReplayCharsLine->setText( QString::fromUtf8( gUNM->m_InvalidReplayChars.c_str( ) ) );
}

void Widget::on_MaxHostCounterApply_clicked( )
{
    gUNM->m_MaxHostCounter = ui->MaxHostCounterSpin->value( );
}

void Widget::on_MaxHostCounterSave_clicked( )
{
    gUNM->m_MaxHostCounter = ui->MaxHostCounterSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maxhostcounter", UTIL_ToString( gUNM->m_MaxHostCounter ) );
}

void Widget::on_MaxHostCounterRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MaxHostCounter = CFG.GetUInt( "bot_maxhostcounter", 999 );

    if( gUNM->m_MaxHostCounter > 16777215 )
        gUNM->m_MaxHostCounter = 16777215;

    ui->MaxHostCounterSpin->setValue( gUNM->m_MaxHostCounter );
}

void Widget::on_MaxHostCounterReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maxhostcounter" );
    gUNM->m_MaxHostCounter = CFG.GetUInt( "bot_maxhostcounter", 999 );

    if( gUNM->m_MaxHostCounter > 16777215 )
        gUNM->m_MaxHostCounter = 16777215;

    ui->MaxHostCounterSpin->setValue( gUNM->m_MaxHostCounter );
}

void Widget::on_RehostCharLine_returnPressed( )
{
    on_RehostCharApply_clicked( );
}

void Widget::on_RehostCharApply_clicked( )
{
    gUNM->m_RehostChar = ui->RehostCharLine->text( ).toStdString( );
}

void Widget::on_RehostCharSave_clicked( )
{
    gUNM->m_RehostChar = ui->RehostCharLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_rehostchar", gUNM->m_RehostChar );
}

void Widget::on_RehostCharRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_RehostChar = CFG.GetString( "bot_rehostchar", "*" );
    ui->RehostCharLine->setText( QString::fromUtf8( gUNM->m_RehostChar.c_str( ) ) );
}

void Widget::on_RehostCharReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_rehostchar" );
    gUNM->m_RehostChar = CFG.GetString( "bot_rehostchar", "*" );
    ui->RehostCharLine->setText( QString::fromUtf8( gUNM->m_RehostChar.c_str( ) ) );
}

void Widget::on_GameNameCharHideApply_clicked( )
{
    gUNM->m_GameNameCharHide = ui->GameNameCharHideCombo->currentIndex( );
}

void Widget::on_GameNameCharHideSave_clicked( )
{
    gUNM->m_GameNameCharHide = ui->GameNameCharHideCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_gamenamecharhide", UTIL_ToString( gUNM->m_GameNameCharHide ) );
}

void Widget::on_GameNameCharHideRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_GameNameCharHide = CFG.GetUInt( "bot_gamenamecharhide", 3, 3 );
    ui->GameNameCharHideCombo->setCurrentIndex( gUNM->m_GameNameCharHide );
}

void Widget::on_GameNameCharHideReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gamenamecharhide" );
    gUNM->m_GameNameCharHide = CFG.GetUInt( "bot_gamenamecharhide", 3, 3 );
    ui->GameNameCharHideCombo->setCurrentIndex( gUNM->m_GameNameCharHide );
}

void Widget::on_DefaultGameNameCharLine_returnPressed( )
{
    on_DefaultGameNameCharApply_clicked( );
}

void Widget::on_DefaultGameNameCharApply_clicked( )
{
    gUNM->m_DefaultGameNameChar = ui->DefaultGameNameCharLine->text( ).toStdString( );
}

void Widget::on_DefaultGameNameCharSave_clicked( )
{
    gUNM->m_DefaultGameNameChar = ui->DefaultGameNameCharLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_gamenamechar", gUNM->m_DefaultGameNameChar );
}

void Widget::on_DefaultGameNameCharRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DefaultGameNameChar = CFG.GetString( "bot_gamenamechar", string( ) );
    ui->DefaultGameNameCharLine->setText( QString::fromUtf8( gUNM->m_DefaultGameNameChar.c_str( ) ) );
}

void Widget::on_DefaultGameNameCharReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gamenamechar" );
    gUNM->m_DefaultGameNameChar = CFG.GetString( "bot_gamenamechar", string( ) );
    ui->DefaultGameNameCharLine->setText( QString::fromUtf8( gUNM->m_DefaultGameNameChar.c_str( ) ) );
}

void Widget::on_ResetGameNameCharApply_clicked( )
{
    ui->ResetGameNameCharCombo->currentIndex( ) == 0 ? gUNM->m_ResetGameNameChar = true : gUNM->m_ResetGameNameChar = false;
}

void Widget::on_ResetGameNameCharSave_clicked( )
{
    if( ui->ResetGameNameCharCombo->currentIndex( ) == 0 )
    {
        gUNM->m_ResetGameNameChar = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_resetgamenamechar", "1" );
    }
    else
    {
        gUNM->m_ResetGameNameChar = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_resetgamenamechar", "0" );
    }
}

void Widget::on_ResetGameNameCharRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ResetGameNameChar = CFG.GetInt( "bot_resetgamenamechar", 1 ) != 0;

    if( gUNM->m_ResetGameNameChar )
        ui->ResetGameNameCharCombo->setCurrentIndex( 0 );
    else
        ui->ResetGameNameCharCombo->setCurrentIndex( 1 );
}

void Widget::on_ResetGameNameCharReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_resetgamenamechar" );
    gUNM->m_ResetGameNameChar = CFG.GetInt( "bot_resetgamenamechar", 1 ) != 0;

    if( gUNM->m_ResetGameNameChar )
        ui->ResetGameNameCharCombo->setCurrentIndex( 0 );
    else
        ui->ResetGameNameCharCombo->setCurrentIndex( 1 );
}

void Widget::on_AppleIconApply_clicked( )
{
    ui->AppleIconCombo->currentIndex( ) == 0 ? gUNM->m_AppleIcon = true : gUNM->m_AppleIcon = false;
}

void Widget::on_AppleIconSave_clicked( )
{
    if( ui->AppleIconCombo->currentIndex( ) == 0 )
    {
        gUNM->m_AppleIcon = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_appleicon", "1" );
    }
    else
    {
        gUNM->m_AppleIcon = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_appleicon", "0" );
    }
}

void Widget::on_AppleIconRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AppleIcon = CFG.GetInt( "bot_appleicon", 1 ) != 0;

    if( gUNM->m_AppleIcon )
        ui->AppleIconCombo->setCurrentIndex( 0 );
    else
        ui->AppleIconCombo->setCurrentIndex( 1 );
}

void Widget::on_AppleIconReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_appleicon" );
    gUNM->m_AppleIcon = CFG.GetInt( "bot_appleicon", 1 ) != 0;

    if( gUNM->m_AppleIcon )
        ui->AppleIconCombo->setCurrentIndex( 0 );
    else
        ui->AppleIconCombo->setCurrentIndex( 1 );
}

void Widget::on_FixGameNameForIccupApply_clicked( )
{
    ui->FixGameNameForIccupCombo->currentIndex( ) == 0 ? gUNM->m_FixGameNameForIccup = true : gUNM->m_FixGameNameForIccup = false;
}

void Widget::on_FixGameNameForIccupSave_clicked( )
{
    if( ui->FixGameNameForIccupCombo->currentIndex( ) == 0 )
    {
        gUNM->m_FixGameNameForIccup = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_fixgamenameforiccup", "1" );
    }
    else
    {
        gUNM->m_FixGameNameForIccup = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_fixgamenameforiccup", "0" );
    }
}

void Widget::on_FixGameNameForIccupRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_FixGameNameForIccup = CFG.GetInt( "bot_fixgamenameforiccup", 1 ) != 0;

    if( gUNM->m_FixGameNameForIccup )
        ui->FixGameNameForIccupCombo->setCurrentIndex( 0 );
    else
        ui->FixGameNameForIccupCombo->setCurrentIndex( 1 );
}

void Widget::on_FixGameNameForIccupReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_fixgamenameforiccup" );
    gUNM->m_FixGameNameForIccup = CFG.GetInt( "bot_fixgamenameforiccup", 1 ) != 0;

    if( gUNM->m_FixGameNameForIccup )
        ui->FixGameNameForIccupCombo->setCurrentIndex( 0 );
    else
        ui->FixGameNameForIccupCombo->setCurrentIndex( 1 );
}

void Widget::on_AllowAutoRenameMapsApply_clicked( )
{
    ui->AllowAutoRenameMapsCombo->currentIndex( ) == 0 ? gUNM->m_AllowAutoRenameMaps = true : gUNM->m_AllowAutoRenameMaps = false;
}

void Widget::on_AllowAutoRenameMapsSave_clicked( )
{
    if( ui->AllowAutoRenameMapsCombo->currentIndex( ) == 0 )
    {
        gUNM->m_AllowAutoRenameMaps = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_allowautorenamemaps", "1" );
    }
    else
    {
        gUNM->m_AllowAutoRenameMaps = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_allowautorenamemaps", "0" );
    }
}

void Widget::on_AllowAutoRenameMapsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AllowAutoRenameMaps = CFG.GetInt( "bot_allowautorenamemaps", 1 ) != 0;

    if( gUNM->m_AllowAutoRenameMaps )
        ui->AllowAutoRenameMapsCombo->setCurrentIndex( 0 );
    else
        ui->AllowAutoRenameMapsCombo->setCurrentIndex( 1 );
}

void Widget::on_AllowAutoRenameMapsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_allowautorenamemaps" );
    gUNM->m_AllowAutoRenameMaps = CFG.GetInt( "bot_allowautorenamemaps", 1 ) != 0;

    if( gUNM->m_AllowAutoRenameMaps )
        ui->AllowAutoRenameMapsCombo->setCurrentIndex( 0 );
    else
        ui->AllowAutoRenameMapsCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapDataFromCFGApply_clicked( )
{
    ui->OverrideMapDataFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapDataFromCFG = true : gUNM->m_OverrideMapDataFromCFG = false;
}

void Widget::on_OverrideMapDataFromCFGSave_clicked( )
{
    if( ui->OverrideMapDataFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapDataFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapdatafromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapDataFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapdatafromcfg", "0" );
    }
}

void Widget::on_OverrideMapDataFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapDataFromCFG = CFG.GetInt( "bot_overridemapdatafromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapDataFromCFG )
        ui->OverrideMapDataFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapDataFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapDataFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapdatafromcfg" );
    gUNM->m_OverrideMapDataFromCFG = CFG.GetInt( "bot_overridemapdatafromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapDataFromCFG )
        ui->OverrideMapDataFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapDataFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapPathFromCFGApply_clicked( )
{
    ui->OverrideMapPathFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapPathFromCFG = true : gUNM->m_OverrideMapPathFromCFG = false;
}

void Widget::on_OverrideMapPathFromCFGSave_clicked( )
{
    if( ui->OverrideMapPathFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapPathFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemappathfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapPathFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemappathfromcfg", "0" );
    }
}

void Widget::on_OverrideMapPathFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapPathFromCFG = CFG.GetInt( "bot_overridemappathfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapPathFromCFG )
        ui->OverrideMapPathFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapPathFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapPathFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemappathfromcfg" );
    gUNM->m_OverrideMapPathFromCFG = CFG.GetInt( "bot_overridemappathfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapPathFromCFG )
        ui->OverrideMapPathFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapPathFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapSizeFromCFGApply_clicked( )
{
    ui->OverrideMapSizeFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapSizeFromCFG = true : gUNM->m_OverrideMapSizeFromCFG = false;
}

void Widget::on_OverrideMapSizeFromCFGSave_clicked( )
{
    if( ui->OverrideMapSizeFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapSizeFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapsizefromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapSizeFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapsizefromcfg", "0" );
    }
}

void Widget::on_OverrideMapSizeFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapSizeFromCFG = CFG.GetInt( "bot_overridemapsizefromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapSizeFromCFG )
        ui->OverrideMapSizeFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapSizeFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapSizeFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapsizefromcfg" );
    gUNM->m_OverrideMapSizeFromCFG = CFG.GetInt( "bot_overridemapsizefromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapSizeFromCFG )
        ui->OverrideMapSizeFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapSizeFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapInfoFromCFGApply_clicked( )
{
    ui->OverrideMapInfoFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapInfoFromCFG = true : gUNM->m_OverrideMapInfoFromCFG = false;
}

void Widget::on_OverrideMapInfoFromCFGSave_clicked( )
{
    if( ui->OverrideMapInfoFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapInfoFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapinfofromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapInfoFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapinfofromcfg", "0" );
    }
}

void Widget::on_OverrideMapInfoFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapInfoFromCFG = CFG.GetInt( "bot_overridemapinfofromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapInfoFromCFG )
        ui->OverrideMapInfoFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapInfoFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapInfoFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapinfofromcfg" );
    gUNM->m_OverrideMapInfoFromCFG = CFG.GetInt( "bot_overridemapinfofromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapInfoFromCFG )
        ui->OverrideMapInfoFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapInfoFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapCrcFromCFGApply_clicked( )
{
    ui->OverrideMapCrcFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapCrcFromCFG = true : gUNM->m_OverrideMapCrcFromCFG = false;
}

void Widget::on_OverrideMapCrcFromCFGSave_clicked( )
{
    if( ui->OverrideMapCrcFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapCrcFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapcrcfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapCrcFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapcrcfromcfg", "0" );
    }
}

void Widget::on_OverrideMapCrcFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapCrcFromCFG = CFG.GetInt( "bot_overridemapcrcfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapCrcFromCFG )
        ui->OverrideMapCrcFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapCrcFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapCrcFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapcrcfromcfg" );
    gUNM->m_OverrideMapCrcFromCFG = CFG.GetInt( "bot_overridemapcrcfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapCrcFromCFG )
        ui->OverrideMapCrcFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapCrcFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapSha1FromCFGApply_clicked( )
{
    ui->OverrideMapSha1FromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapSha1FromCFG = true : gUNM->m_OverrideMapSha1FromCFG = false;
}

void Widget::on_OverrideMapSha1FromCFGSave_clicked( )
{
    if( ui->OverrideMapSha1FromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapSha1FromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapsha1fromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapSha1FromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapsha1fromcfg", "0" );
    }
}

void Widget::on_OverrideMapSha1FromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapSha1FromCFG = CFG.GetInt( "bot_overridemapsha1fromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapSha1FromCFG )
        ui->OverrideMapSha1FromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapSha1FromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapSha1FromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapsha1fromcfg" );
    gUNM->m_OverrideMapSha1FromCFG = CFG.GetInt( "bot_overridemapsha1fromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapSha1FromCFG )
        ui->OverrideMapSha1FromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapSha1FromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapSpeedFromCFGApply_clicked( )
{
    ui->OverrideMapSpeedFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapSpeedFromCFG = true : gUNM->m_OverrideMapSpeedFromCFG = false;
}

void Widget::on_OverrideMapSpeedFromCFGSave_clicked( )
{
    if( ui->OverrideMapSpeedFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapSpeedFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapspeedfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapSpeedFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapspeedfromcfg", "0" );
    }
}

void Widget::on_OverrideMapSpeedFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapSpeedFromCFG = CFG.GetInt( "bot_overridemapspeedfromcfg", 0 ) != 0;

    if( gUNM->m_OverrideMapSpeedFromCFG )
        ui->OverrideMapSpeedFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapSpeedFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapSpeedFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapspeedfromcfg" );
    gUNM->m_OverrideMapSpeedFromCFG = CFG.GetInt( "bot_overridemapspeedfromcfg", 0 ) != 0;

    if( gUNM->m_OverrideMapSpeedFromCFG )
        ui->OverrideMapSpeedFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapSpeedFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapVisibilityFromCFGApply_clicked( )
{
    ui->OverrideMapVisibilityFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapVisibilityFromCFG = true : gUNM->m_OverrideMapVisibilityFromCFG = false;
}

void Widget::on_OverrideMapVisibilityFromCFGSave_clicked( )
{
    if( ui->OverrideMapVisibilityFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapVisibilityFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapvisibilityfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapVisibilityFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapvisibilityfromcfg", "0" );
    }
}

void Widget::on_OverrideMapVisibilityFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapVisibilityFromCFG = CFG.GetInt( "bot_overridemapvisibilityfromcfg", 0 ) != 0;

    if( gUNM->m_OverrideMapVisibilityFromCFG )
        ui->OverrideMapVisibilityFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapVisibilityFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapVisibilityFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapvisibilityfromcfg" );
    gUNM->m_OverrideMapVisibilityFromCFG = CFG.GetInt( "bot_overridemapvisibilityfromcfg", 0 ) != 0;

    if( gUNM->m_OverrideMapVisibilityFromCFG )
        ui->OverrideMapVisibilityFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapVisibilityFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapFlagsFromCFGApply_clicked( )
{
    ui->OverrideMapFlagsFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapFlagsFromCFG = true : gUNM->m_OverrideMapFlagsFromCFG = false;
}

void Widget::on_OverrideMapFlagsFromCFGSave_clicked( )
{
    if( ui->OverrideMapFlagsFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapFlagsFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapflagsfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapFlagsFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapflagsfromcfg", "0" );
    }
}

void Widget::on_OverrideMapFlagsFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapFlagsFromCFG = CFG.GetInt( "bot_overridemapflagsfromcfg", 0 ) != 0;

    if( gUNM->m_OverrideMapFlagsFromCFG )
        ui->OverrideMapFlagsFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapFlagsFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapFlagsFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapflagsfromcfg" );
    gUNM->m_OverrideMapFlagsFromCFG = CFG.GetInt( "bot_overridemapflagsfromcfg", 0 ) != 0;

    if( gUNM->m_OverrideMapFlagsFromCFG )
        ui->OverrideMapFlagsFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapFlagsFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapFilterTypeFromCFGApply_clicked( )
{
    ui->OverrideMapFilterTypeFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapFilterTypeFromCFG = true : gUNM->m_OverrideMapFilterTypeFromCFG = false;
}

void Widget::on_OverrideMapFilterTypeFromCFGSave_clicked( )
{
    if( ui->OverrideMapFilterTypeFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapFilterTypeFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapfiltertypefromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapFilterTypeFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapfiltertypefromcfg", "0" );
    }
}

void Widget::on_OverrideMapFilterTypeFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapFilterTypeFromCFG = CFG.GetInt( "bot_overridemapfiltertypefromcfg", 0 ) != 0;

    if( gUNM->m_OverrideMapFilterTypeFromCFG )
        ui->OverrideMapFilterTypeFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapFilterTypeFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapFilterTypeFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapfiltertypefromcfg" );
    gUNM->m_OverrideMapFilterTypeFromCFG = CFG.GetInt( "bot_overridemapfiltertypefromcfg", 0 ) != 0;

    if( gUNM->m_OverrideMapFilterTypeFromCFG )
        ui->OverrideMapFilterTypeFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapFilterTypeFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapGameTypeFromCFGApply_clicked( )
{
    ui->OverrideMapGameTypeFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapGameTypeFromCFG = true : gUNM->m_OverrideMapGameTypeFromCFG = false;
}

void Widget::on_OverrideMapGameTypeFromCFGSave_clicked( )
{
    if( ui->OverrideMapGameTypeFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapGameTypeFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapgametypefromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapGameTypeFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapgametypefromcfg", "0" );
    }
}

void Widget::on_OverrideMapGameTypeFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapGameTypeFromCFG = CFG.GetInt( "bot_overridemapgametypefromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapGameTypeFromCFG )
        ui->OverrideMapGameTypeFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapGameTypeFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapGameTypeFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapgametypefromcfg" );
    gUNM->m_OverrideMapGameTypeFromCFG = CFG.GetInt( "bot_overridemapgametypefromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapGameTypeFromCFG )
        ui->OverrideMapGameTypeFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapGameTypeFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapWidthFromCFGApply_clicked( )
{
    ui->OverrideMapWidthFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapWidthFromCFG = true : gUNM->m_OverrideMapWidthFromCFG = false;
}

void Widget::on_OverrideMapWidthFromCFGSave_clicked( )
{
    if( ui->OverrideMapWidthFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapWidthFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapwidthfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapWidthFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapwidthfromcfg", "0" );
    }
}

void Widget::on_OverrideMapWidthFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapWidthFromCFG = CFG.GetInt( "bot_overridemapwidthfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapWidthFromCFG )
        ui->OverrideMapWidthFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapWidthFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapWidthFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapwidthfromcfg" );
    gUNM->m_OverrideMapWidthFromCFG = CFG.GetInt( "bot_overridemapwidthfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapWidthFromCFG )
        ui->OverrideMapWidthFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapWidthFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapHeightFromCFGApply_clicked( )
{
    ui->OverrideMapHeightFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapHeightFromCFG = true : gUNM->m_OverrideMapHeightFromCFG = false;
}

void Widget::on_OverrideMapHeightFromCFGSave_clicked( )
{
    if( ui->OverrideMapHeightFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapHeightFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapheightfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapHeightFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapheightfromcfg", "0" );
    }
}

void Widget::on_OverrideMapHeightFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapHeightFromCFG = CFG.GetInt( "bot_overridemapheightfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapHeightFromCFG )
        ui->OverrideMapHeightFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapHeightFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapHeightFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapheightfromcfg" );
    gUNM->m_OverrideMapHeightFromCFG = CFG.GetInt( "bot_overridemapheightfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapHeightFromCFG )
        ui->OverrideMapHeightFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapHeightFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapTypeFromCFGApply_clicked( )
{
    ui->OverrideMapTypeFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapTypeFromCFG = true : gUNM->m_OverrideMapTypeFromCFG = false;
}

void Widget::on_OverrideMapTypeFromCFGSave_clicked( )
{
    if( ui->OverrideMapTypeFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapTypeFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemaptypefromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapTypeFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemaptypefromcfg", "0" );
    }
}

void Widget::on_OverrideMapTypeFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapTypeFromCFG = CFG.GetInt( "bot_overridemaptypefromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapTypeFromCFG )
        ui->OverrideMapTypeFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapTypeFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapTypeFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemaptypefromcfg" );
    gUNM->m_OverrideMapTypeFromCFG = CFG.GetInt( "bot_overridemaptypefromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapTypeFromCFG )
        ui->OverrideMapTypeFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapTypeFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapDefaultHCLFromCFGApply_clicked( )
{
    ui->OverrideMapDefaultHCLFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapDefaultHCLFromCFG = true : gUNM->m_OverrideMapDefaultHCLFromCFG = false;
}

void Widget::on_OverrideMapDefaultHCLFromCFGSave_clicked( )
{
    if( ui->OverrideMapDefaultHCLFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapDefaultHCLFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapdefaulthclfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapDefaultHCLFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapdefaulthclfromcfg", "0" );
    }
}

void Widget::on_OverrideMapDefaultHCLFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapDefaultHCLFromCFG = CFG.GetInt( "bot_overridemapdefaulthclfromcfg", 0 ) != 0;

    if( gUNM->m_OverrideMapDefaultHCLFromCFG )
        ui->OverrideMapDefaultHCLFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapDefaultHCLFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapDefaultHCLFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapdefaulthclfromcfg" );
    gUNM->m_OverrideMapDefaultHCLFromCFG = CFG.GetInt( "bot_overridemapdefaulthclfromcfg", 0 ) != 0;

    if( gUNM->m_OverrideMapDefaultHCLFromCFG )
        ui->OverrideMapDefaultHCLFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapDefaultHCLFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapLoadInGameFromCFGApply_clicked( )
{
    ui->OverrideMapLoadInGameFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapLoadInGameFromCFG = true : gUNM->m_OverrideMapLoadInGameFromCFG = false;
}

void Widget::on_OverrideMapLoadInGameFromCFGSave_clicked( )
{
    if( ui->OverrideMapLoadInGameFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapLoadInGameFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemaploadingamefromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapLoadInGameFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemaploadingamefromcfg", "0" );
    }
}

void Widget::on_OverrideMapLoadInGameFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapLoadInGameFromCFG = CFG.GetInt( "bot_overridemaploadingamefromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapLoadInGameFromCFG )
        ui->OverrideMapLoadInGameFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapLoadInGameFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapLoadInGameFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemaploadingamefromcfg" );
    gUNM->m_OverrideMapLoadInGameFromCFG = CFG.GetInt( "bot_overridemaploadingamefromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapLoadInGameFromCFG )
        ui->OverrideMapLoadInGameFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapLoadInGameFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapNumPlayersFromCFGApply_clicked( )
{
    ui->OverrideMapNumPlayersFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapNumPlayersFromCFG = true : gUNM->m_OverrideMapNumPlayersFromCFG = false;
}

void Widget::on_OverrideMapNumPlayersFromCFGSave_clicked( )
{
    if( ui->OverrideMapNumPlayersFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapNumPlayersFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapnumplayersfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapNumPlayersFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapnumplayersfromcfg", "0" );
    }
}

void Widget::on_OverrideMapNumPlayersFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapNumPlayersFromCFG = CFG.GetInt( "bot_overridemapnumplayersfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapNumPlayersFromCFG )
        ui->OverrideMapNumPlayersFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapNumPlayersFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapNumPlayersFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapnumplayersfromcfg" );
    gUNM->m_OverrideMapNumPlayersFromCFG = CFG.GetInt( "bot_overridemapnumplayersfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapNumPlayersFromCFG )
        ui->OverrideMapNumPlayersFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapNumPlayersFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapNumTeamsFromCFGApply_clicked( )
{
    ui->OverrideMapNumTeamsFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapNumTeamsFromCFG = true : gUNM->m_OverrideMapNumTeamsFromCFG = false;
}

void Widget::on_OverrideMapNumTeamsFromCFGSave_clicked( )
{
    if( ui->OverrideMapNumTeamsFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapNumTeamsFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapnumteamsfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapNumTeamsFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapnumteamsfromcfg", "0" );
    }
}

void Widget::on_OverrideMapNumTeamsFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapNumTeamsFromCFG = CFG.GetInt( "bot_overridemapnumteamsfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapNumTeamsFromCFG )
        ui->OverrideMapNumTeamsFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapNumTeamsFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapNumTeamsFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapnumteamsfromcfg" );
    gUNM->m_OverrideMapNumTeamsFromCFG = CFG.GetInt( "bot_overridemapnumteamsfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapNumTeamsFromCFG )
        ui->OverrideMapNumTeamsFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapNumTeamsFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapSlotsFromCFGApply_clicked( )
{
    ui->OverrideMapSlotsFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapSlotsFromCFG = true : gUNM->m_OverrideMapSlotsFromCFG = false;
}

void Widget::on_OverrideMapSlotsFromCFGSave_clicked( )
{
    if( ui->OverrideMapSlotsFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapSlotsFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapslotsfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapSlotsFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapslotsfromcfg", "0" );
    }
}

void Widget::on_OverrideMapSlotsFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapSlotsFromCFG = CFG.GetInt( "bot_overridemapslotsfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapSlotsFromCFG )
        ui->OverrideMapSlotsFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapSlotsFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapSlotsFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapslotsfromcfg" );
    gUNM->m_OverrideMapSlotsFromCFG = CFG.GetInt( "bot_overridemapslotsfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapSlotsFromCFG )
        ui->OverrideMapSlotsFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapSlotsFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapObserversFromCFGApply_clicked( )
{
    ui->OverrideMapObserversFromCFGCombo->currentIndex( ) == 0 ? gUNM->m_OverrideMapObserversFromCFG = true : gUNM->m_OverrideMapObserversFromCFG = false;
}

void Widget::on_OverrideMapObserversFromCFGSave_clicked( )
{
    if( ui->OverrideMapObserversFromCFGCombo->currentIndex( ) == 0 )
    {
        gUNM->m_OverrideMapObserversFromCFG = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapobserversfromcfg", "1" );
    }
    else
    {
        gUNM->m_OverrideMapObserversFromCFG = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_overridemapobserversfromcfg", "0" );
    }
}

void Widget::on_OverrideMapObserversFromCFGRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OverrideMapObserversFromCFG = CFG.GetInt( "bot_overridemapobserversfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapObserversFromCFG )
        ui->OverrideMapObserversFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapObserversFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_OverrideMapObserversFromCFGReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_overridemapobserversfromcfg" );
    gUNM->m_OverrideMapObserversFromCFG = CFG.GetInt( "bot_overridemapobserversfromcfg", 1 ) != 0;

    if( gUNM->m_OverrideMapObserversFromCFG )
        ui->OverrideMapObserversFromCFGCombo->setCurrentIndex( 0 );
    else
        ui->OverrideMapObserversFromCFGCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayersCanChangeRaceApply_clicked( )
{
    ui->PlayersCanChangeRaceCombo->currentIndex( ) == 0 ? gUNM->m_PlayersCanChangeRace = true : gUNM->m_PlayersCanChangeRace = false;
}

void Widget::on_PlayersCanChangeRaceSave_clicked( )
{
    if( ui->PlayersCanChangeRaceCombo->currentIndex( ) == 0 )
    {
        gUNM->m_PlayersCanChangeRace = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerscanchangerace", "1" );
    }
    else
    {
        gUNM->m_PlayersCanChangeRace = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerscanchangerace", "0" );
    }
}

void Widget::on_PlayersCanChangeRaceRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PlayersCanChangeRace = CFG.GetInt( "bot_playerscanchangerace", 1 ) != 0;

    if( gUNM->m_PlayersCanChangeRace )
        ui->PlayersCanChangeRaceCombo->setCurrentIndex( 0 );
    else
        ui->PlayersCanChangeRaceCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayersCanChangeRaceReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_playerscanchangerace" );
    gUNM->m_PlayersCanChangeRace = CFG.GetInt( "bot_playerscanchangerace", 1 ) != 0;

    if( gUNM->m_PlayersCanChangeRace )
        ui->PlayersCanChangeRaceCombo->setCurrentIndex( 0 );
    else
        ui->PlayersCanChangeRaceCombo->setCurrentIndex( 1 );
}

void Widget::on_HoldPlayersForRMKApply_clicked( )
{
    ui->HoldPlayersForRMKCombo->currentIndex( ) == 0 ? gUNM->m_HoldPlayersForRMK = true : gUNM->m_HoldPlayersForRMK = false;
}

void Widget::on_HoldPlayersForRMKSave_clicked( )
{
    if( ui->HoldPlayersForRMKCombo->currentIndex( ) == 0 )
    {
        gUNM->m_HoldPlayersForRMK = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_holdplayersforrmk", "1" );
    }
    else
    {
        gUNM->m_HoldPlayersForRMK = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_holdplayersforrmk", "0" );
    }
}

void Widget::on_HoldPlayersForRMKRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_HoldPlayersForRMK = CFG.GetInt( "bot_holdplayersforrmk", 1 ) != 0;

    if( gUNM->m_HoldPlayersForRMK )
        ui->HoldPlayersForRMKCombo->setCurrentIndex( 0 );
    else
        ui->HoldPlayersForRMKCombo->setCurrentIndex( 1 );
}

void Widget::on_HoldPlayersForRMKReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_holdplayersforrmk" );
    gUNM->m_HoldPlayersForRMK = CFG.GetInt( "bot_holdplayersforrmk", 1 ) != 0;

    if( gUNM->m_HoldPlayersForRMK )
        ui->HoldPlayersForRMKCombo->setCurrentIndex( 0 );
    else
        ui->HoldPlayersForRMKCombo->setCurrentIndex( 1 );
}

void Widget::on_autohclfromgamenameApply_clicked( )
{
    ui->autohclfromgamenameCombo->currentIndex( ) == 0 ? gUNM->m_autohclfromgamename = true : gUNM->m_autohclfromgamename = false;
}

void Widget::on_autohclfromgamenameSave_clicked( )
{
    if( ui->autohclfromgamenameCombo->currentIndex( ) == 0 )
    {
        gUNM->m_autohclfromgamename = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_autohclfromgamename", "1" );
    }
    else
    {
        gUNM->m_autohclfromgamename = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_autohclfromgamename", "0" );
    }
}

void Widget::on_autohclfromgamenameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_autohclfromgamename = CFG.GetInt( "bot_autohclfromgamename", 0 ) != 0;

    if( gUNM->m_autohclfromgamename )
        ui->autohclfromgamenameCombo->setCurrentIndex( 0 );
    else
        ui->autohclfromgamenameCombo->setCurrentIndex( 1 );
}

void Widget::on_autohclfromgamenameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_autohclfromgamename" );
    gUNM->m_autohclfromgamename = CFG.GetInt( "bot_autohclfromgamename", 0 ) != 0;

    if( gUNM->m_autohclfromgamename )
        ui->autohclfromgamenameCombo->setCurrentIndex( 0 );
    else
        ui->autohclfromgamenameCombo->setCurrentIndex( 1 );
}

void Widget::on_forceautohclindotaApply_clicked( )
{
    ui->forceautohclindotaCombo->currentIndex( ) == 0 ? gUNM->m_forceautohclindota = true : gUNM->m_forceautohclindota = false;
}

void Widget::on_forceautohclindotaSave_clicked( )
{
    if( ui->forceautohclindotaCombo->currentIndex( ) == 0 )
    {
        gUNM->m_forceautohclindota = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_forceautohclindota", "1" );
    }
    else
    {
        gUNM->m_forceautohclindota = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_forceautohclindota", "0" );
    }
}

void Widget::on_forceautohclindotaRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_forceautohclindota = CFG.GetInt( "bot_forceautohclindota", 0 ) != 0;

    if( gUNM->m_forceautohclindota )
        ui->forceautohclindotaCombo->setCurrentIndex( 0 );
    else
        ui->forceautohclindotaCombo->setCurrentIndex( 1 );
}

void Widget::on_forceautohclindotaReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_forceautohclindota" );
    gUNM->m_forceautohclindota = CFG.GetInt( "bot_forceautohclindota", 0 ) != 0;

    if( gUNM->m_forceautohclindota )
        ui->forceautohclindotaCombo->setCurrentIndex( 0 );
    else
        ui->forceautohclindotaCombo->setCurrentIndex( 1 );
}

void Widget::on_ForceLoadInGameApply_clicked( )
{
    ui->ForceLoadInGameCombo->currentIndex( ) == 0 ? gUNM->m_ForceLoadInGame = true : gUNM->m_ForceLoadInGame = false;
}

void Widget::on_ForceLoadInGameSave_clicked( )
{
    if( ui->ForceLoadInGameCombo->currentIndex( ) == 0 )
    {
        gUNM->m_ForceLoadInGame = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_forceloadingame", "1" );
    }
    else
    {
        gUNM->m_ForceLoadInGame = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_forceloadingame", "0" );
    }
}

void Widget::on_ForceLoadInGameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ForceLoadInGame = CFG.GetInt( "bot_forceloadingame", 1 ) != 0;

    if( gUNM->m_ForceLoadInGame )
        ui->ForceLoadInGameCombo->setCurrentIndex( 0 );
    else
        ui->ForceLoadInGameCombo->setCurrentIndex( 1 );
}

void Widget::on_ForceLoadInGameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_forceloadingame" );
    gUNM->m_ForceLoadInGame = CFG.GetInt( "bot_forceloadingame", 1 ) != 0;

    if( gUNM->m_ForceLoadInGame )
        ui->ForceLoadInGameCombo->setCurrentIndex( 0 );
    else
        ui->ForceLoadInGameCombo->setCurrentIndex( 1 );
}

void Widget::on_MsgAutoCorrectModeApply_clicked( )
{
    gUNM->m_MsgAutoCorrectMode = ui->MsgAutoCorrectModeCombo->currentIndex( );
}

void Widget::on_MsgAutoCorrectModeSave_clicked( )
{
    gUNM->m_MsgAutoCorrectMode = ui->MsgAutoCorrectModeCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_msgautocorrectmode", UTIL_ToString( gUNM->m_MsgAutoCorrectMode ) );
}

void Widget::on_MsgAutoCorrectModeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MsgAutoCorrectMode = CFG.GetUInt( "bot_msgautocorrectmode", 2, 3 );
    ui->MsgAutoCorrectModeCombo->setCurrentIndex( gUNM->m_MsgAutoCorrectMode );
}

void Widget::on_MsgAutoCorrectModeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_msgautocorrectmode" );
    gUNM->m_MsgAutoCorrectMode = CFG.GetUInt( "bot_msgautocorrectmode", 2, 3 );
    ui->MsgAutoCorrectModeCombo->setCurrentIndex( gUNM->m_MsgAutoCorrectMode );
}

void Widget::on_SpoofChecksApply_clicked( )
{
    gUNM->m_SpoofChecks = ui->SpoofChecksCombo->currentIndex( );
}

void Widget::on_SpoofChecksSave_clicked( )
{
    gUNM->m_SpoofChecks = ui->SpoofChecksCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_spoofchecks", UTIL_ToString( gUNM->m_SpoofChecks ) );
}

void Widget::on_SpoofChecksRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_SpoofChecks = CFG.GetUInt( "bot_spoofchecks", 2, 2 );
    ui->SpoofChecksCombo->setCurrentIndex( gUNM->m_SpoofChecks );
}

void Widget::on_SpoofChecksReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_spoofchecks" );
    gUNM->m_SpoofChecks = CFG.GetUInt( "bot_spoofchecks", 2, 2 );
    ui->SpoofChecksCombo->setCurrentIndex( gUNM->m_SpoofChecks );
}

void Widget::on_CheckMultipleIPUsageApply_clicked( )
{
    ui->CheckMultipleIPUsageCombo->currentIndex( ) == 0 ? gUNM->m_CheckMultipleIPUsage = true : gUNM->m_CheckMultipleIPUsage = false;
}

void Widget::on_CheckMultipleIPUsageSave_clicked( )
{
    if( ui->CheckMultipleIPUsageCombo->currentIndex( ) == 0 )
    {
        gUNM->m_CheckMultipleIPUsage = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_checkmultipleipusage", "1" );
    }
    else
    {
        gUNM->m_CheckMultipleIPUsage = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_checkmultipleipusage", "0" );
    }
}

void Widget::on_CheckMultipleIPUsageRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_CheckMultipleIPUsage = CFG.GetInt( "bot_checkmultipleipusage", 1 ) != 0;

    if( gUNM->m_CheckMultipleIPUsage )
        ui->CheckMultipleIPUsageCombo->setCurrentIndex( 0 );
    else
        ui->CheckMultipleIPUsageCombo->setCurrentIndex( 1 );
}

void Widget::on_CheckMultipleIPUsageReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_checkmultipleipusage" );
    gUNM->m_CheckMultipleIPUsage = CFG.GetInt( "bot_checkmultipleipusage", 1 ) != 0;

    if( gUNM->m_CheckMultipleIPUsage )
        ui->CheckMultipleIPUsageCombo->setCurrentIndex( 0 );
    else
        ui->CheckMultipleIPUsageCombo->setCurrentIndex( 1 );
}

void Widget::on_SpoofCheckIfGameNameIsIndefiniteApply_clicked( )
{
    ui->SpoofCheckIfGameNameIsIndefiniteCombo->currentIndex( ) == 0 ? gUNM->m_SpoofCheckIfGameNameIsIndefinite = true : gUNM->m_SpoofCheckIfGameNameIsIndefinite = false;
}

void Widget::on_SpoofCheckIfGameNameIsIndefiniteSave_clicked( )
{
    if( ui->SpoofCheckIfGameNameIsIndefiniteCombo->currentIndex( ) == 0 )
    {
        gUNM->m_SpoofCheckIfGameNameIsIndefinite = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_spoofcheckifgnisindefinite", "1" );
    }
    else
    {
        gUNM->m_SpoofCheckIfGameNameIsIndefinite = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_spoofcheckifgnisindefinite", "0" );
    }
}

void Widget::on_SpoofCheckIfGameNameIsIndefiniteRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_SpoofCheckIfGameNameIsIndefinite = CFG.GetInt( "bot_spoofcheckifgnisindefinite", 0 ) != 0;

    if( gUNM->m_SpoofCheckIfGameNameIsIndefinite )
        ui->SpoofCheckIfGameNameIsIndefiniteCombo->setCurrentIndex( 0 );
    else
        ui->SpoofCheckIfGameNameIsIndefiniteCombo->setCurrentIndex( 1 );
}

void Widget::on_SpoofCheckIfGameNameIsIndefiniteReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_spoofcheckifgnisindefinite" );
    gUNM->m_SpoofCheckIfGameNameIsIndefinite = CFG.GetInt( "bot_spoofcheckifgnisindefinite", 0 ) != 0;

    if( gUNM->m_SpoofCheckIfGameNameIsIndefinite )
        ui->SpoofCheckIfGameNameIsIndefiniteCombo->setCurrentIndex( 0 );
    else
        ui->SpoofCheckIfGameNameIsIndefiniteCombo->setCurrentIndex( 1 );
}

void Widget::on_MaxLatencyApply_clicked( )
{
    gUNM->m_MaxLatency = ui->MaxLatencySpin->value( );
}

void Widget::on_MaxLatencySave_clicked( )
{
    gUNM->m_MaxLatency = ui->MaxLatencySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maxlatency", UTIL_ToString( gUNM->m_MaxLatency ) );
}

void Widget::on_MaxLatencyRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MaxLatency = CFG.GetUInt( "bot_maxlatency", 100, 0, 10000 );
    ui->MaxLatencySpin->setValue( gUNM->m_MaxLatency );
}

void Widget::on_MaxLatencyReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maxlatency" );
    gUNM->m_MaxLatency = CFG.GetUInt( "bot_maxlatency", 100, 0, 10000 );
    ui->MaxLatencySpin->setValue( gUNM->m_MaxLatency );
}

void Widget::on_MinLatencyApply_clicked( )
{
    gUNM->m_MinLatency = ui->MinLatencySpin->value( );

    if( gUNM->m_MinLatency > gUNM->m_MaxLatency )
    {
        gUNM->m_MinLatency = gUNM->m_MaxLatency;
        ui->MinLatencySpin->setValue( gUNM->m_MinLatency );
    }
}

void Widget::on_MinLatencySave_clicked( )
{
    gUNM->m_MinLatency = ui->MinLatencySpin->value( );

    if( gUNM->m_MinLatency > gUNM->m_MaxLatency )
    {
        gUNM->m_MinLatency = gUNM->m_MaxLatency;
        ui->MinLatencySpin->setValue( gUNM->m_MinLatency );
    }

    UTIL_SetVarInFile( "unm.cfg", "bot_minlatency", UTIL_ToString( gUNM->m_MinLatency ) );
}

void Widget::on_MinLatencyRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MinLatency = CFG.GetUInt( "bot_minlatency", 0, 0, gUNM->m_MaxLatency );
    ui->MinLatencySpin->setValue( gUNM->m_MinLatency );
}

void Widget::on_MinLatencyReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_minlatency" );
    gUNM->m_MinLatency = CFG.GetUInt( "bot_minlatency", 0, 0, gUNM->m_MaxLatency );
    ui->MinLatencySpin->setValue( gUNM->m_MinLatency );
}

void Widget::on_LatencyApply_clicked( )
{
    gUNM->m_Latency = ui->LatencySpin->value( );

    if( gUNM->m_Latency < gUNM->m_MinLatency )
    {
        gUNM->m_Latency = gUNM->m_MinLatency;
        ui->LatencySpin->setValue( gUNM->m_Latency );
    }
    else if( gUNM->m_Latency > gUNM->m_MaxLatency )
    {
        gUNM->m_Latency = gUNM->m_MaxLatency;
        ui->LatencySpin->setValue( gUNM->m_Latency );
    }
}

void Widget::on_LatencySave_clicked( )
{
    gUNM->m_Latency = ui->LatencySpin->value( );

    if( gUNM->m_Latency < gUNM->m_MinLatency )
    {
        gUNM->m_Latency = gUNM->m_MinLatency;
        ui->LatencySpin->setValue( gUNM->m_Latency );
    }
    else if( gUNM->m_Latency > gUNM->m_MaxLatency )
    {
        gUNM->m_Latency = gUNM->m_MaxLatency;
        ui->LatencySpin->setValue( gUNM->m_Latency );
    }

    UTIL_SetVarInFile( "unm.cfg", "bot_latency", UTIL_ToString( gUNM->m_Latency ) );
}

void Widget::on_LatencyRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_Latency = CFG.GetUInt( "bot_latency", 35, gUNM->m_MinLatency, gUNM->m_MaxLatency );
    ui->LatencySpin->setValue( gUNM->m_Latency );
}

void Widget::on_LatencyReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_latency" );
    gUNM->m_Latency = CFG.GetUInt( "bot_latency", 35, gUNM->m_MinLatency, gUNM->m_MaxLatency );
    ui->LatencySpin->setValue( gUNM->m_Latency );
}

void Widget::on_UseDynamicLatencyApply_clicked( )
{
    ui->UseDynamicLatencyCombo->currentIndex( ) == 0 ? gUNM->m_UseDynamicLatency = true : gUNM->m_UseDynamicLatency = false;
}

void Widget::on_UseDynamicLatencySave_clicked( )
{
    if( ui->UseDynamicLatencyCombo->currentIndex( ) == 0 )
    {
        gUNM->m_UseDynamicLatency = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_usedynamiclatency", "1" );
    }
    else
    {
        gUNM->m_UseDynamicLatency = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_usedynamiclatency", "0" );
    }
}

void Widget::on_UseDynamicLatencyRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_UseDynamicLatency = CFG.GetInt( "bot_usedynamiclatency", 0 ) != 0;

    if( gUNM->m_UseDynamicLatency )
        ui->UseDynamicLatencyCombo->setCurrentIndex( 0 );
    else
        ui->UseDynamicLatencyCombo->setCurrentIndex( 1 );
}

void Widget::on_UseDynamicLatencyReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_usedynamiclatency" );
    gUNM->m_UseDynamicLatency = CFG.GetInt( "bot_usedynamiclatency", 0 ) != 0;

    if( gUNM->m_UseDynamicLatency )
        ui->UseDynamicLatencyCombo->setCurrentIndex( 0 );
    else
        ui->UseDynamicLatencyCombo->setCurrentIndex( 1 );
}

void Widget::on_DynamicLatencyHighestAllowedApply_clicked( )
{
    gUNM->m_DynamicLatencyHighestAllowed = ui->DynamicLatencyHighestAllowedSpin->value( );
}

void Widget::on_DynamicLatencyHighestAllowedSave_clicked( )
{
    gUNM->m_DynamicLatencyHighestAllowed = ui->DynamicLatencyHighestAllowedSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dynamiclatencyhighestallowed", UTIL_ToString( gUNM->m_DynamicLatencyHighestAllowed ) );
}

void Widget::on_DynamicLatencyHighestAllowedRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DynamicLatencyHighestAllowed = CFG.GetUInt( "bot_dynamiclatencyhighestallowed", 50, 0, 10000 );
    ui->DynamicLatencyHighestAllowedSpin->setValue( gUNM->m_DynamicLatencyHighestAllowed );
}

void Widget::on_DynamicLatencyHighestAllowedReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dynamiclatencyhighestallowed" );
    gUNM->m_DynamicLatencyHighestAllowed = CFG.GetUInt( "bot_dynamiclatencyhighestallowed", 50, 0, 10000 );
    ui->DynamicLatencyHighestAllowedSpin->setValue( gUNM->m_DynamicLatencyHighestAllowed );
}

void Widget::on_DynamicLatencyLowestAllowedApply_clicked( )
{
    gUNM->m_DynamicLatencyLowestAllowed = ui->DynamicLatencyLowestAllowedSpin->value( );

    if( gUNM->m_DynamicLatencyLowestAllowed > gUNM->m_DynamicLatencyHighestAllowed )
    {
        gUNM->m_DynamicLatencyLowestAllowed = gUNM->m_DynamicLatencyHighestAllowed;
        ui->DynamicLatencyLowestAllowedSpin->setValue( gUNM->m_DynamicLatencyLowestAllowed );
    }
}

void Widget::on_DynamicLatencyLowestAllowedSave_clicked( )
{
    gUNM->m_DynamicLatencyLowestAllowed = ui->DynamicLatencyLowestAllowedSpin->value( );

    if( gUNM->m_DynamicLatencyLowestAllowed > gUNM->m_DynamicLatencyHighestAllowed )
    {
        gUNM->m_DynamicLatencyLowestAllowed = gUNM->m_DynamicLatencyHighestAllowed;
        ui->DynamicLatencyLowestAllowedSpin->setValue( gUNM->m_DynamicLatencyLowestAllowed );
    }

    UTIL_SetVarInFile( "unm.cfg", "bot_dynamiclatencylowestallowed", UTIL_ToString( gUNM->m_DynamicLatencyLowestAllowed ) );
}

void Widget::on_DynamicLatencyLowestAllowedRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DynamicLatencyLowestAllowed = CFG.GetUInt( "bot_dynamiclatencylowestallowed", 15, 0, gUNM->m_DynamicLatencyHighestAllowed );
    ui->DynamicLatencyLowestAllowedSpin->setValue( gUNM->m_DynamicLatencyLowestAllowed );
}

void Widget::on_DynamicLatencyLowestAllowedReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dynamiclatencylowestallowed" );
    gUNM->m_DynamicLatencyLowestAllowed = CFG.GetUInt( "bot_dynamiclatencylowestallowed", 15, 0, gUNM->m_DynamicLatencyHighestAllowed );
    ui->DynamicLatencyLowestAllowedSpin->setValue( gUNM->m_DynamicLatencyLowestAllowed );
}

void Widget::on_DynamicLatencyRefreshRateApply_clicked( )
{
    gUNM->m_DynamicLatencyRefreshRate = ui->DynamicLatencyRefreshRateSpin->value( );
}

void Widget::on_DynamicLatencyRefreshRateSave_clicked( )
{
    gUNM->m_DynamicLatencyRefreshRate = ui->DynamicLatencyRefreshRateSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dynamiclatencyrefreshrate", UTIL_ToString( gUNM->m_DynamicLatencyRefreshRate ) );
}

void Widget::on_DynamicLatencyRefreshRateRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DynamicLatencyRefreshRate = CFG.GetUInt( "bot_dynamiclatencyrefreshrate", 5 );
    ui->DynamicLatencyRefreshRateSpin->setValue( gUNM->m_DynamicLatencyRefreshRate );
}

void Widget::on_DynamicLatencyRefreshRateReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dynamiclatencyrefreshrate" );
    gUNM->m_DynamicLatencyRefreshRate = CFG.GetUInt( "bot_dynamiclatencyrefreshrate", 5 );
    ui->DynamicLatencyRefreshRateSpin->setValue( gUNM->m_DynamicLatencyRefreshRate );
}

void Widget::on_DynamicLatencyConsolePrintApply_clicked( )
{
    gUNM->m_DynamicLatencyConsolePrint = ui->DynamicLatencyConsolePrintSpin->value( );
}

void Widget::on_DynamicLatencyConsolePrintSave_clicked( )
{
    gUNM->m_DynamicLatencyConsolePrint = ui->DynamicLatencyConsolePrintSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dynamiclatencyconsoleprint", UTIL_ToString( gUNM->m_DynamicLatencyConsolePrint ) );
}

void Widget::on_DynamicLatencyConsolePrintRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DynamicLatencyConsolePrint = CFG.GetUInt( "bot_dynamiclatencyconsoleprint", 600 );
    ui->DynamicLatencyConsolePrintSpin->setValue( gUNM->m_DynamicLatencyConsolePrint );
}

void Widget::on_DynamicLatencyConsolePrintReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dynamiclatencyconsoleprint" );
    gUNM->m_DynamicLatencyConsolePrint = CFG.GetUInt( "bot_dynamiclatencyconsoleprint", 600 );
    ui->DynamicLatencyConsolePrintSpin->setValue( gUNM->m_DynamicLatencyConsolePrint );
}

void Widget::on_DynamicLatencyPercentPingMaxApply_clicked( )
{
    gUNM->m_DynamicLatencyPercentPingMax = ui->DynamicLatencyPercentPingMaxSpin->value( );
}

void Widget::on_DynamicLatencyPercentPingMaxSave_clicked( )
{
    gUNM->m_DynamicLatencyPercentPingMax = ui->DynamicLatencyPercentPingMaxSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dynamiclatencypercentpm", UTIL_ToString( gUNM->m_DynamicLatencyPercentPingMax ) );
}

void Widget::on_DynamicLatencyPercentPingMaxRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DynamicLatencyPercentPingMax = CFG.GetUInt( "bot_dynamiclatencypercentpm", 60 );
    ui->DynamicLatencyPercentPingMaxSpin->setValue( gUNM->m_DynamicLatencyPercentPingMax );
}

void Widget::on_DynamicLatencyPercentPingMaxReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dynamiclatencypercentpm" );
    gUNM->m_DynamicLatencyPercentPingMax = CFG.GetUInt( "bot_dynamiclatencypercentpm", 60 );
    ui->DynamicLatencyPercentPingMaxSpin->setValue( gUNM->m_DynamicLatencyPercentPingMax );
}

void Widget::on_DynamicLatencyDifferencePingMaxApply_clicked( )
{
    gUNM->m_DynamicLatencyDifferencePingMax = ui->DynamicLatencyDifferencePingMaxSpin->value( );
}

void Widget::on_DynamicLatencyDifferencePingMaxSave_clicked( )
{
    gUNM->m_DynamicLatencyDifferencePingMax = ui->DynamicLatencyDifferencePingMaxSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dynamiclatencydiffpingmax", UTIL_ToString( gUNM->m_DynamicLatencyDifferencePingMax ) );
}

void Widget::on_DynamicLatencyDifferencePingMaxRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DynamicLatencyDifferencePingMax = CFG.GetUInt( "bot_dynamiclatencydiffpingmax", 0 );
    ui->DynamicLatencyDifferencePingMaxSpin->setValue( gUNM->m_DynamicLatencyDifferencePingMax );
}

void Widget::on_DynamicLatencyDifferencePingMaxReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dynamiclatencydiffpingmax" );
    gUNM->m_DynamicLatencyDifferencePingMax = CFG.GetUInt( "bot_dynamiclatencydiffpingmax", 0 );
    ui->DynamicLatencyDifferencePingMaxSpin->setValue( gUNM->m_DynamicLatencyDifferencePingMax );
}

void Widget::on_DynamicLatencyMaxSyncApply_clicked( )
{
    gUNM->m_DynamicLatencyMaxSync = ui->DynamicLatencyMaxSyncSpin->value( );
}

void Widget::on_DynamicLatencyMaxSyncSave_clicked( )
{
    gUNM->m_DynamicLatencyMaxSync = ui->DynamicLatencyMaxSyncSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dynamiclatencymaxsync", UTIL_ToString( gUNM->m_DynamicLatencyMaxSync ) );
}

void Widget::on_DynamicLatencyMaxSyncRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DynamicLatencyMaxSync = CFG.GetUInt( "bot_dynamiclatencymaxsync", 0 );
    ui->DynamicLatencyMaxSyncSpin->setValue( gUNM->m_DynamicLatencyMaxSync );
}

void Widget::on_DynamicLatencyMaxSyncReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dynamiclatencymaxsync" );
    gUNM->m_DynamicLatencyMaxSync = CFG.GetUInt( "bot_dynamiclatencymaxsync", 0 );
    ui->DynamicLatencyMaxSyncSpin->setValue( gUNM->m_DynamicLatencyMaxSync );
}

void Widget::on_DynamicLatencyAddIfMaxSyncApply_clicked( )
{
    gUNM->m_DynamicLatencyAddIfMaxSync = ui->DynamicLatencyAddIfMaxSyncSpin->value( );
}

void Widget::on_DynamicLatencyAddIfMaxSyncSave_clicked( )
{
    gUNM->m_DynamicLatencyAddIfMaxSync = ui->DynamicLatencyAddIfMaxSyncSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dynamiclatencyaddifmaxsync", UTIL_ToString( gUNM->m_DynamicLatencyAddIfMaxSync ) );
}

void Widget::on_DynamicLatencyAddIfMaxSyncRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DynamicLatencyAddIfMaxSync = CFG.GetUInt( "bot_dynamiclatencyaddifmaxsync", 0 );
    ui->DynamicLatencyAddIfMaxSyncSpin->setValue( gUNM->m_DynamicLatencyAddIfMaxSync );
}

void Widget::on_DynamicLatencyAddIfMaxSyncReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dynamiclatencyaddifmaxsync" );
    gUNM->m_DynamicLatencyAddIfMaxSync = CFG.GetUInt( "bot_dynamiclatencyaddifmaxsync", 0 );
    ui->DynamicLatencyAddIfMaxSyncSpin->setValue( gUNM->m_DynamicLatencyAddIfMaxSync );
}

void Widget::on_DynamicLatencyAddIfLagApply_clicked( )
{
    gUNM->m_DynamicLatencyAddIfLag = ui->DynamicLatencyAddIfLagSpin->value( );
}

void Widget::on_DynamicLatencyAddIfLagSave_clicked( )
{
    gUNM->m_DynamicLatencyAddIfLag = ui->DynamicLatencyAddIfLagSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dynamiclatencyaddiflag", UTIL_ToString( gUNM->m_DynamicLatencyAddIfLag ) );
}

void Widget::on_DynamicLatencyAddIfLagRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DynamicLatencyAddIfLag = CFG.GetUInt( "bot_dynamiclatencyaddiflag", 0 );
    ui->DynamicLatencyAddIfLagSpin->setValue( gUNM->m_DynamicLatencyAddIfLag );
}

void Widget::on_DynamicLatencyAddIfLagReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dynamiclatencyaddiflag" );
    gUNM->m_DynamicLatencyAddIfLag = CFG.GetUInt( "bot_dynamiclatencyaddiflag", 0 );
    ui->DynamicLatencyAddIfLagSpin->setValue( gUNM->m_DynamicLatencyAddIfLag );
}

void Widget::on_DynamicLatencyAddIfBotLagApply_clicked( )
{
    gUNM->m_DynamicLatencyAddIfBotLag = ui->DynamicLatencyAddIfBotLagSpin->value( );
}

void Widget::on_DynamicLatencyAddIfBotLagSave_clicked( )
{
    gUNM->m_DynamicLatencyAddIfBotLag = ui->DynamicLatencyAddIfBotLagSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dynamiclatencyaddifbotlag", UTIL_ToString( gUNM->m_DynamicLatencyAddIfBotLag ) );
}

void Widget::on_DynamicLatencyAddIfBotLagRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DynamicLatencyAddIfBotLag = CFG.GetUInt( "bot_dynamiclatencyaddifbotlag", 0 );
    ui->DynamicLatencyAddIfBotLagSpin->setValue( gUNM->m_DynamicLatencyAddIfBotLag );
}

void Widget::on_DynamicLatencyAddIfBotLagReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dynamiclatencyaddifbotlag" );
    gUNM->m_DynamicLatencyAddIfBotLag = CFG.GetUInt( "bot_dynamiclatencyaddifbotlag", 0 );
    ui->DynamicLatencyAddIfBotLagSpin->setValue( gUNM->m_DynamicLatencyAddIfBotLag );
}

void Widget::on_MaxSyncLimitApply_clicked( )
{
    gUNM->m_MaxSyncLimit = ui->MaxSyncLimitSpin->value( );
}

void Widget::on_MaxSyncLimitSave_clicked( )
{
    gUNM->m_MaxSyncLimit = ui->MaxSyncLimitSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maxsynclimit", UTIL_ToString( gUNM->m_MaxSyncLimit ) );
}

void Widget::on_MaxSyncLimitRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MaxSyncLimit = CFG.GetUInt( "bot_maxsynclimit", 1000000000, 10, 1000000000 );
    ui->MaxSyncLimitSpin->setValue( gUNM->m_MaxSyncLimit );
}

void Widget::on_MaxSyncLimitReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maxsynclimit" );
    gUNM->m_MaxSyncLimit = CFG.GetUInt( "bot_maxsynclimit", 1000000000, 10, 1000000000 );
    ui->MaxSyncLimitSpin->setValue( gUNM->m_MaxSyncLimit );
}

void Widget::on_MinSyncLimitApply_clicked( )
{
    gUNM->m_MinSyncLimit = ui->MinSyncLimitSpin->value( );

    if( gUNM->m_MinSyncLimit > gUNM->m_MaxSyncLimit )
    {
        gUNM->m_MinSyncLimit = gUNM->m_MaxSyncLimit;
        ui->MinSyncLimitSpin->setValue( gUNM->m_MinSyncLimit );
    }
}

void Widget::on_MinSyncLimitSave_clicked( )
{
    gUNM->m_MinSyncLimit = ui->MinSyncLimitSpin->value( );

    if( gUNM->m_MinSyncLimit > gUNM->m_MaxSyncLimit )
    {
        gUNM->m_MinSyncLimit = gUNM->m_MaxSyncLimit;
        ui->MinSyncLimitSpin->setValue( gUNM->m_MinSyncLimit );
    }

    UTIL_SetVarInFile( "unm.cfg", "bot_minsynclimit", UTIL_ToString( gUNM->m_MinSyncLimit ) );
}

void Widget::on_MinSyncLimitRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MinSyncLimit = CFG.GetUInt( "bot_minsynclimit", 10, 10, gUNM->m_MaxSyncLimit );
    ui->MinLatencySpin->setValue( gUNM->m_MinSyncLimit );
}

void Widget::on_MinSyncLimitReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_minsynclimit" );
    gUNM->m_MinSyncLimit = CFG.GetUInt( "bot_minsynclimit", 10, 10, gUNM->m_MaxSyncLimit );
    ui->MinLatencySpin->setValue( gUNM->m_MinSyncLimit );
}

void Widget::on_SyncLimitApply_clicked( )
{
    gUNM->m_SyncLimit = ui->SyncLimitSpin->value( );

    if( gUNM->m_SyncLimit < gUNM->m_MinSyncLimit )
    {
        gUNM->m_SyncLimit = gUNM->m_MinSyncLimit;
        ui->SyncLimitSpin->setValue( gUNM->m_SyncLimit );
    }
    else if( gUNM->m_SyncLimit > gUNM->m_MaxSyncLimit )
    {
        gUNM->m_SyncLimit = gUNM->m_MaxSyncLimit;
        ui->SyncLimitSpin->setValue( gUNM->m_SyncLimit );
    }
}

void Widget::on_SyncLimitSave_clicked( )
{
    gUNM->m_SyncLimit = ui->SyncLimitSpin->value( );

    if( gUNM->m_SyncLimit < gUNM->m_MinSyncLimit )
    {
        gUNM->m_SyncLimit = gUNM->m_MinSyncLimit;
        ui->SyncLimitSpin->setValue( gUNM->m_SyncLimit );
    }
    else if( gUNM->m_SyncLimit > gUNM->m_MaxSyncLimit )
    {
        gUNM->m_SyncLimit = gUNM->m_MaxSyncLimit;
        ui->SyncLimitSpin->setValue( gUNM->m_SyncLimit );
    }

    UTIL_SetVarInFile( "unm.cfg", "bot_synclimit", UTIL_ToString( gUNM->m_SyncLimit ) );
}

void Widget::on_SyncLimitRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_SyncLimit = CFG.GetUInt( "bot_synclimit", 600, gUNM->m_MinSyncLimit, gUNM->m_MaxSyncLimit );
    ui->SyncLimitSpin->setValue( gUNM->m_SyncLimit );
}

void Widget::on_SyncLimitReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_synclimit" );
    gUNM->m_SyncLimit = CFG.GetUInt( "bot_synclimit", 600, gUNM->m_MinSyncLimit, gUNM->m_MaxSyncLimit );
    ui->SyncLimitSpin->setValue( gUNM->m_SyncLimit );
}

void Widget::on_AutoSaveApply_clicked( )
{
    ui->AutoSaveCombo->currentIndex( ) == 0 ? gUNM->m_AutoSave = true : gUNM->m_AutoSave = false;
}

void Widget::on_AutoSaveSave_clicked( )
{
    if( ui->AutoSaveCombo->currentIndex( ) == 0 )
    {
        gUNM->m_AutoSave = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_autosave", "1" );
    }
    else
    {
        gUNM->m_AutoSave = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_autosave", "0" );
    }
}

void Widget::on_AutoSaveRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AutoSave = CFG.GetInt( "bot_autosave", 0 ) != 0;

    if( gUNM->m_AutoSave )
        ui->AutoSaveCombo->setCurrentIndex( 0 );
    else
        ui->AutoSaveCombo->setCurrentIndex( 1 );
}

void Widget::on_AutoSaveReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_autosave" );
    gUNM->m_AutoSave = CFG.GetInt( "bot_autosave", 0 ) != 0;

    if( gUNM->m_AutoSave )
        ui->AutoSaveCombo->setCurrentIndex( 0 );
    else
        ui->AutoSaveCombo->setCurrentIndex( 1 );
}

void Widget::on_AutoKickPingApply_clicked( )
{
    gUNM->m_AutoKickPing = ui->AutoKickPingSpin->value( );
}

void Widget::on_AutoKickPingSave_clicked( )
{
    gUNM->m_AutoKickPing = ui->AutoKickPingSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_autokickping", UTIL_ToString( gUNM->m_AutoKickPing ) );
}

void Widget::on_AutoKickPingRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AutoKickPing = CFG.GetUInt( "bot_autokickping", 300 );
    ui->AutoKickPingSpin->setValue( gUNM->m_AutoKickPing );
}

void Widget::on_AutoKickPingReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_autokickping" );
    gUNM->m_AutoKickPing = CFG.GetUInt( "bot_autokickping", 300 );
    ui->AutoKickPingSpin->setValue( gUNM->m_AutoKickPing );
}

void Widget::on_LCPingsApply_clicked( )
{
    ui->LCPingsCombo->currentIndex( ) == 0 ? gUNM->m_LCPings = true : gUNM->m_LCPings = false;
}

void Widget::on_LCPingsSave_clicked( )
{
    if( ui->LCPingsCombo->currentIndex( ) == 0 )
    {
        gUNM->m_LCPings = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_lcpings", "1" );
    }
    else
    {
        gUNM->m_LCPings = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_lcpings", "0" );
    }
}

void Widget::on_LCPingsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_LCPings = CFG.GetInt( "bot_lcpings", 1 ) != 0;

    if( gUNM->m_LCPings )
        ui->LCPingsCombo->setCurrentIndex( 0 );
    else
        ui->LCPingsCombo->setCurrentIndex( 1 );
}

void Widget::on_LCPingsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_lcpings" );
    gUNM->m_LCPings = CFG.GetInt( "bot_lcpings", 1 ) != 0;

    if( gUNM->m_LCPings )
        ui->LCPingsCombo->setCurrentIndex( 0 );
    else
        ui->LCPingsCombo->setCurrentIndex( 1 );
}

void Widget::on_PingDuringDownloadsApply_clicked( )
{
    ui->PingDuringDownloadsCombo->currentIndex( ) == 0 ? gUNM->m_PingDuringDownloads = true : gUNM->m_PingDuringDownloads = false;
}

void Widget::on_PingDuringDownloadsSave_clicked( )
{
    if( ui->PingDuringDownloadsCombo->currentIndex( ) == 0 )
    {
        gUNM->m_PingDuringDownloads = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_pingduringdownloads", "1" );
    }
    else
    {
        gUNM->m_PingDuringDownloads = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_pingduringdownloads", "0" );
    }
}

void Widget::on_PingDuringDownloadsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PingDuringDownloads = CFG.GetInt( "bot_pingduringdownloads", 1 ) != 0;

    if( gUNM->m_PingDuringDownloads )
        ui->PingDuringDownloadsCombo->setCurrentIndex( 0 );
    else
        ui->PingDuringDownloadsCombo->setCurrentIndex( 1 );
}

void Widget::on_PingDuringDownloadsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_pingduringdownloads" );
    gUNM->m_PingDuringDownloads = CFG.GetInt( "bot_pingduringdownloads", 1 ) != 0;

    if( gUNM->m_PingDuringDownloads )
        ui->PingDuringDownloadsCombo->setCurrentIndex( 0 );
    else
        ui->PingDuringDownloadsCombo->setCurrentIndex( 1 );
}

void Widget::on_AllowDownloadsApply_clicked( )
{
    gUNM->m_AllowDownloads = ui->AllowDownloadsCombo->currentIndex( );
}

void Widget::on_AllowDownloadsSave_clicked( )
{
    gUNM->m_AllowDownloads = ui->AllowDownloadsCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_allowdownloads", UTIL_ToString( gUNM->m_AllowDownloads ) );
}

void Widget::on_AllowDownloadsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AllowDownloads = CFG.GetUInt( "bot_allowdownloads", 1, 3 );
    ui->AllowDownloadsCombo->setCurrentIndex( gUNM->m_AllowDownloads );
}

void Widget::on_AllowDownloadsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_allowdownloads" );
    gUNM->m_AllowDownloads = CFG.GetUInt( "bot_allowdownloads", 1, 3 );
    ui->AllowDownloadsCombo->setCurrentIndex( gUNM->m_AllowDownloads );
}

void Widget::on_maxdownloadersApply_clicked( )
{
    gUNM->m_maxdownloaders = ui->maxdownloadersCombo->currentIndex( );
}

void Widget::on_maxdownloadersSave_clicked( )
{
    gUNM->m_maxdownloaders = ui->maxdownloadersCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maxdownloaders", UTIL_ToString( gUNM->m_maxdownloaders ) );
}

void Widget::on_maxdownloadersRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_maxdownloaders = CFG.GetUInt( "bot_maxdownloaders", 12, 0, 12 );
    ui->maxdownloadersCombo->setCurrentIndex( gUNM->m_maxdownloaders );
}

void Widget::on_maxdownloadersReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maxdownloaders" );
    gUNM->m_maxdownloaders = CFG.GetUInt( "bot_maxdownloaders", 12, 0, 12 );
    ui->maxdownloadersCombo->setCurrentIndex( gUNM->m_maxdownloaders );
}

void Widget::on_totaldownloadspeedApply_clicked( )
{
    gUNM->m_totaldownloadspeed = ui->totaldownloadspeedSpin->value( );
}

void Widget::on_totaldownloadspeedSave_clicked( )
{
    gUNM->m_totaldownloadspeed = ui->totaldownloadspeedSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_totaldownloadspeed", UTIL_ToString( gUNM->m_totaldownloadspeed ) );
}

void Widget::on_totaldownloadspeedRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_totaldownloadspeed = CFG.GetUInt( "bot_totaldownloadspeed", 24576 );
    ui->totaldownloadspeedSpin->setValue( gUNM->m_totaldownloadspeed );
}

void Widget::on_totaldownloadspeedReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_totaldownloadspeed" );
    gUNM->m_totaldownloadspeed = CFG.GetUInt( "bot_totaldownloadspeed", 24576 );
    ui->totaldownloadspeedSpin->setValue( gUNM->m_totaldownloadspeed );
}

void Widget::on_clientdownloadspeedApply_clicked( )
{
    gUNM->m_clientdownloadspeed = ui->clientdownloadspeedSpin->value( );
}

void Widget::on_clientdownloadspeedSave_clicked( )
{
    gUNM->m_clientdownloadspeed = ui->clientdownloadspeedSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_clientdownloadspeed", UTIL_ToString( gUNM->m_clientdownloadspeed ) );
}

void Widget::on_clientdownloadspeedRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_clientdownloadspeed = CFG.GetUInt( "bot_clientdownloadspeed", 2048 );
    ui->clientdownloadspeedSpin->setValue( gUNM->m_clientdownloadspeed );
}

void Widget::on_clientdownloadspeedReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_clientdownloadspeed" );
    gUNM->m_clientdownloadspeed = CFG.GetUInt( "bot_clientdownloadspeed", 2048 );
    ui->clientdownloadspeedSpin->setValue( gUNM->m_clientdownloadspeed );
}

void Widget::on_PlaceAdminsHigherApply_clicked( )
{
    gUNM->m_PlaceAdminsHigher = ui->PlaceAdminsHigherCombo->currentIndex( );
}

void Widget::on_PlaceAdminsHigherSave_clicked( )
{
    gUNM->m_PlaceAdminsHigher = ui->PlaceAdminsHigherCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_placeadminshigher", UTIL_ToString( gUNM->m_PlaceAdminsHigher ) );
}

void Widget::on_PlaceAdminsHigherRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PlaceAdminsHigher = CFG.GetUInt( "bot_placeadminshigher", 0, 3 );
    ui->PlaceAdminsHigherCombo->setCurrentIndex( gUNM->m_PlaceAdminsHigher );
}

void Widget::on_PlaceAdminsHigherReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_placeadminshigher" );
    gUNM->m_PlaceAdminsHigher = CFG.GetUInt( "bot_placeadminshigher", 0, 3 );
    ui->PlaceAdminsHigherCombo->setCurrentIndex( gUNM->m_PlaceAdminsHigher );
}

void Widget::on_ShuffleSlotsOnStartApply_clicked( )
{
    ui->ShuffleSlotsOnStartCombo->currentIndex( ) == 0 ? gUNM->m_ShuffleSlotsOnStart = true : gUNM->m_ShuffleSlotsOnStart = false;
}

void Widget::on_ShuffleSlotsOnStartSave_clicked( )
{
    if( ui->ShuffleSlotsOnStartCombo->currentIndex( ) == 0 )
    {
        gUNM->m_ShuffleSlotsOnStart = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_shuffleslotsonstart", "1" );
    }
    else
    {
        gUNM->m_ShuffleSlotsOnStart = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_shuffleslotsonstart", "0" );
    }
}

void Widget::on_ShuffleSlotsOnStartRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ShuffleSlotsOnStart = CFG.GetInt( "bot_shuffleslotsonstart", 0 ) != 0;

    if( gUNM->m_ShuffleSlotsOnStart )
        ui->ShuffleSlotsOnStartCombo->setCurrentIndex( 0 );
    else
        ui->ShuffleSlotsOnStartCombo->setCurrentIndex( 1 );
}

void Widget::on_ShuffleSlotsOnStartReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_shuffleslotsonstart" );
    gUNM->m_ShuffleSlotsOnStart = CFG.GetInt( "bot_shuffleslotsonstart", 0 ) != 0;

    if( gUNM->m_ShuffleSlotsOnStart )
        ui->ShuffleSlotsOnStartCombo->setCurrentIndex( 0 );
    else
        ui->ShuffleSlotsOnStartCombo->setCurrentIndex( 1 );
}

void Widget::on_FakePlayersLobbyApply_clicked( )
{
    ui->FakePlayersLobbyCombo->currentIndex( ) == 0 ? gUNM->m_FakePlayersLobby = true : gUNM->m_FakePlayersLobby = false;
}

void Widget::on_FakePlayersLobbySave_clicked( )
{
    if( ui->FakePlayersLobbyCombo->currentIndex( ) == 0 )
    {
        gUNM->m_FakePlayersLobby = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_fakeplayersinlobby", "1" );
    }
    else
    {
        gUNM->m_FakePlayersLobby = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_fakeplayersinlobby", "0" );
    }
}

void Widget::on_FakePlayersLobbyRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_FakePlayersLobby = CFG.GetInt( "bot_fakeplayersinlobby", 0 ) != 0;

    if( gUNM->m_FakePlayersLobby )
        ui->FakePlayersLobbyCombo->setCurrentIndex( 0 );
    else
        ui->FakePlayersLobbyCombo->setCurrentIndex( 1 );
}

void Widget::on_FakePlayersLobbyReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_fakeplayersinlobby" );
    gUNM->m_FakePlayersLobby = CFG.GetInt( "bot_fakeplayersinlobby", 0 ) != 0;

    if( gUNM->m_FakePlayersLobby )
        ui->FakePlayersLobbyCombo->setCurrentIndex( 0 );
    else
        ui->FakePlayersLobbyCombo->setCurrentIndex( 1 );
}

void Widget::on_MoreFPsLobbyApply_clicked( )
{
    gUNM->m_MoreFPsLobby = ui->MoreFPsLobbyCombo->currentIndex( );
}

void Widget::on_MoreFPsLobbySave_clicked( )
{
    gUNM->m_MoreFPsLobby = ui->MoreFPsLobbyCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_morefakeplayersinlobby", UTIL_ToString( gUNM->m_MoreFPsLobby ) );
}

void Widget::on_MoreFPsLobbyRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MoreFPsLobby = CFG.GetUInt( "bot_morefakeplayersinlobby", 3, 3 );
    ui->MoreFPsLobbyCombo->setCurrentIndex( gUNM->m_MoreFPsLobby );
}

void Widget::on_MoreFPsLobbyReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_morefakeplayersinlobby" );
    gUNM->m_MoreFPsLobby = CFG.GetUInt( "bot_morefakeplayersinlobby", 3, 3 );
    ui->MoreFPsLobbyCombo->setCurrentIndex( gUNM->m_MoreFPsLobby );
}

void Widget::on_NormalCountdownApply_clicked( )
{
    ui->NormalCountdownCombo->currentIndex( ) == 0 ? gUNM->m_NormalCountdown = true : gUNM->m_NormalCountdown = false;
}

void Widget::on_NormalCountdownSave_clicked( )
{
    if( ui->NormalCountdownCombo->currentIndex( ) == 0 )
    {
        gUNM->m_NormalCountdown = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_normalcountdown", "1" );
    }
    else
    {
        gUNM->m_NormalCountdown = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_normalcountdown", "0" );
    }
}

void Widget::on_NormalCountdownRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NormalCountdown = CFG.GetInt( "bot_normalcountdown", 0 ) != 0;

    if( gUNM->m_NormalCountdown )
        ui->NormalCountdownCombo->setCurrentIndex( 0 );
    else
        ui->NormalCountdownCombo->setCurrentIndex( 1 );
}

void Widget::on_NormalCountdownReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_normalcountdown" );
    gUNM->m_NormalCountdown = CFG.GetInt( "bot_normalcountdown", 0 ) != 0;

    if( gUNM->m_NormalCountdown )
        ui->NormalCountdownCombo->setCurrentIndex( 0 );
    else
        ui->NormalCountdownCombo->setCurrentIndex( 1 );
}

void Widget::on_CountDownMethodApply_clicked( )
{
    gUNM->m_CountDownMethod = ui->CountDownMethodCombo->currentIndex( );
}

void Widget::on_CountDownMethodSave_clicked( )
{
    gUNM->m_CountDownMethod = ui->CountDownMethodCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_countdownmethod", UTIL_ToString( gUNM->m_CountDownMethod ) );
}

void Widget::on_CountDownMethodRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_CountDownMethod = CFG.GetUInt( "bot_countdownmethod", 1, 1 );
    ui->CountDownMethodCombo->setCurrentIndex( gUNM->m_CountDownMethod );
}

void Widget::on_CountDownMethodReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_countdownmethod" );
    gUNM->m_CountDownMethod = CFG.GetUInt( "bot_countdownmethod", 1, 1 );
    ui->CountDownMethodCombo->setCurrentIndex( gUNM->m_CountDownMethod );
}

void Widget::on_CountDownStartCounterApply_clicked( )
{
    gUNM->m_CountDownStartCounter = ui->CountDownStartCounterSpin->value( );
}

void Widget::on_CountDownStartCounterSave_clicked( )
{
    gUNM->m_CountDownStartCounter = ui->CountDownStartCounterSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_countdownstartcounter", UTIL_ToString( gUNM->m_CountDownStartCounter ) );
}

void Widget::on_CountDownStartCounterRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_CountDownStartCounter = CFG.GetUInt( "bot_countdownstartcounter", 5 );
    ui->CountDownStartCounterSpin->setValue( gUNM->m_CountDownStartCounter );
}

void Widget::on_CountDownStartCounterReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_countdownstartcounter" );
    gUNM->m_CountDownStartCounter = CFG.GetUInt( "bot_countdownstartcounter", 5 );
    ui->CountDownStartCounterSpin->setValue( gUNM->m_CountDownStartCounter );
}

void Widget::on_CountDownStartForceCounterApply_clicked( )
{
    gUNM->m_CountDownStartForceCounter = ui->CountDownStartForceCounterSpin->value( );
}

void Widget::on_CountDownStartForceCounterSave_clicked( )
{
    gUNM->m_CountDownStartForceCounter = ui->CountDownStartForceCounterSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_countdownstartforcecounter", UTIL_ToString( gUNM->m_CountDownStartForceCounter ) );
}

void Widget::on_CountDownStartForceCounterRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_CountDownStartForceCounter = CFG.GetUInt( "bot_countdownstartforcecounter", 5 );
    ui->CountDownStartForceCounterSpin->setValue( gUNM->m_CountDownStartForceCounter );
}

void Widget::on_CountDownStartForceCounterReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_countdownstartforcecounter" );
    gUNM->m_CountDownStartForceCounter = CFG.GetUInt( "bot_countdownstartforcecounter", 5 );
    ui->CountDownStartForceCounterSpin->setValue( gUNM->m_CountDownStartForceCounter );
}

void Widget::on_VoteStartAllowedApply_clicked( )
{
    ui->VoteStartAllowedCombo->currentIndex( ) == 0 ? gUNM->m_VoteStartAllowed = true : gUNM->m_VoteStartAllowed = false;
}

void Widget::on_VoteStartAllowedSave_clicked( )
{
    if( ui->VoteStartAllowedCombo->currentIndex( ) == 0 )
    {
        gUNM->m_VoteStartAllowed = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_votestartallowed", "1" );
    }
    else
    {
        gUNM->m_VoteStartAllowed = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_votestartallowed", "0" );
    }
}

void Widget::on_VoteStartAllowedRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_VoteStartAllowed = CFG.GetInt( "bot_votestartallowed", 1 ) != 0;

    if( gUNM->m_VoteStartAllowed )
        ui->VoteStartAllowedCombo->setCurrentIndex( 0 );
    else
        ui->VoteStartAllowedCombo->setCurrentIndex( 1 );
}

void Widget::on_VoteStartAllowedReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_votestartallowed" );
    gUNM->m_VoteStartAllowed = CFG.GetInt( "bot_votestartallowed", 1 ) != 0;

    if( gUNM->m_VoteStartAllowed )
        ui->VoteStartAllowedCombo->setCurrentIndex( 0 );
    else
        ui->VoteStartAllowedCombo->setCurrentIndex( 1 );
}

void Widget::on_VoteStartPercentalVotingApply_clicked( )
{
    ui->VoteStartPercentalVotingCombo->currentIndex( ) == 0 ? gUNM->m_VoteStartPercentalVoting = true : gUNM->m_VoteStartPercentalVoting = false;
}

void Widget::on_VoteStartPercentalVotingSave_clicked( )
{
    if( ui->VoteStartPercentalVotingCombo->currentIndex( ) == 0 )
    {
        gUNM->m_VoteStartPercentalVoting = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_votestartpercentalvoting", "1" );
    }
    else
    {
        gUNM->m_VoteStartPercentalVoting = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_votestartpercentalvoting", "0" );
    }
}

void Widget::on_VoteStartPercentalVotingRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_VoteStartPercentalVoting = CFG.GetInt( "bot_votestartpercentalvoting", 1 ) != 0;

    if( gUNM->m_VoteStartPercentalVoting )
        ui->VoteStartPercentalVotingCombo->setCurrentIndex( 0 );
    else
        ui->VoteStartPercentalVotingCombo->setCurrentIndex( 1 );
}

void Widget::on_VoteStartPercentalVotingReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_votestartpercentalvoting" );
    gUNM->m_VoteStartPercentalVoting = CFG.GetInt( "bot_votestartpercentalvoting", 1 ) != 0;

    if( gUNM->m_VoteStartPercentalVoting )
        ui->VoteStartPercentalVotingCombo->setCurrentIndex( 0 );
    else
        ui->VoteStartPercentalVotingCombo->setCurrentIndex( 1 );
}

void Widget::on_CancelVoteStartIfLeaveApply_clicked( )
{
    ui->CancelVoteStartIfLeaveCombo->currentIndex( ) == 0 ? gUNM->m_CancelVoteStartIfLeave = true : gUNM->m_CancelVoteStartIfLeave = false;
}

void Widget::on_CancelVoteStartIfLeaveSave_clicked( )
{
    if( ui->CancelVoteStartIfLeaveCombo->currentIndex( ) == 0 )
    {
        gUNM->m_CancelVoteStartIfLeave = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_cancelvotestartifleave", "1" );
    }
    else
    {
        gUNM->m_CancelVoteStartIfLeave = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_cancelvotestartifleave", "0" );
    }
}

void Widget::on_CancelVoteStartIfLeaveRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_CancelVoteStartIfLeave = CFG.GetInt( "bot_cancelvotestartifleave", 0 ) != 0;

    if( gUNM->m_CancelVoteStartIfLeave )
        ui->CancelVoteStartIfLeaveCombo->setCurrentIndex( 0 );
    else
        ui->CancelVoteStartIfLeaveCombo->setCurrentIndex( 1 );
}

void Widget::on_CancelVoteStartIfLeaveReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_cancelvotestartifleave" );
    gUNM->m_CancelVoteStartIfLeave = CFG.GetInt( "bot_cancelvotestartifleave", 0 ) != 0;

    if( gUNM->m_CancelVoteStartIfLeave )
        ui->CancelVoteStartIfLeaveCombo->setCurrentIndex( 0 );
    else
        ui->CancelVoteStartIfLeaveCombo->setCurrentIndex( 1 );
}

void Widget::on_StartAfterLeaveIfEnoughVotesApply_clicked( )
{
    ui->StartAfterLeaveIfEnoughVotesCombo->currentIndex( ) == 0 ? gUNM->m_StartAfterLeaveIfEnoughVotes = true : gUNM->m_StartAfterLeaveIfEnoughVotes = false;
}

void Widget::on_StartAfterLeaveIfEnoughVotesSave_clicked( )
{
    if( ui->StartAfterLeaveIfEnoughVotesCombo->currentIndex( ) == 0 )
    {
        gUNM->m_StartAfterLeaveIfEnoughVotes = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_startafterleaveifenoughvotes", "1" );
    }
    else
    {
        gUNM->m_StartAfterLeaveIfEnoughVotes = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_startafterleaveifenoughvotes", "0" );
    }
}

void Widget::on_StartAfterLeaveIfEnoughVotesRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_StartAfterLeaveIfEnoughVotes = CFG.GetInt( "bot_startafterleaveifenoughvotes", 0 ) != 0;

    if( gUNM->m_StartAfterLeaveIfEnoughVotes )
        ui->StartAfterLeaveIfEnoughVotesCombo->setCurrentIndex( 0 );
    else
        ui->StartAfterLeaveIfEnoughVotesCombo->setCurrentIndex( 1 );
}

void Widget::on_StartAfterLeaveIfEnoughVotesReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_startafterleaveifenoughvotes" );
    gUNM->m_StartAfterLeaveIfEnoughVotes = CFG.GetInt( "bot_startafterleaveifenoughvotes", 0 ) != 0;

    if( gUNM->m_StartAfterLeaveIfEnoughVotes )
        ui->StartAfterLeaveIfEnoughVotesCombo->setCurrentIndex( 0 );
    else
        ui->StartAfterLeaveIfEnoughVotesCombo->setCurrentIndex( 1 );
}

void Widget::on_VoteStartMinPlayersApply_clicked( )
{
    gUNM->m_VoteStartMinPlayers = ui->VoteStartMinPlayersCombo->currentIndex( ) + 1;
}

void Widget::on_VoteStartMinPlayersSave_clicked( )
{
    gUNM->m_VoteStartMinPlayers = ui->VoteStartMinPlayersCombo->currentIndex( ) + 1;
    UTIL_SetVarInFile( "unm.cfg", "bot_votestartminplayers", UTIL_ToString( gUNM->m_VoteStartMinPlayers ) );
}

void Widget::on_VoteStartMinPlayersRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_VoteStartMinPlayers = CFG.GetUInt( "bot_votestartminplayers", 2, 1, 12 );
    ui->VoteStartMinPlayersCombo->setCurrentIndex( gUNM->m_VoteStartMinPlayers - 1 );
}

void Widget::on_VoteStartMinPlayersReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_votestartminplayers" );
    gUNM->m_VoteStartMinPlayers = CFG.GetUInt( "bot_votestartminplayers", 2, 1, 12 );
    ui->VoteStartMinPlayersCombo->setCurrentIndex( gUNM->m_VoteStartMinPlayers - 1 );
}

void Widget::on_VoteStartPercentApply_clicked( )
{
    gUNM->m_VoteStartPercent = ui->VoteStartPercentSpin->value( );
}

void Widget::on_VoteStartPercentSave_clicked( )
{
    gUNM->m_VoteStartPercent = ui->VoteStartPercentSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_votestartpercent", UTIL_ToString( gUNM->m_VoteStartPercent ) );
}

void Widget::on_VoteStartPercentRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_VoteStartPercent = CFG.GetUInt( "bot_votestartpercent", 80, 1, 100 );
    ui->VoteStartPercentSpin->setValue( gUNM->m_VoteStartPercent );
}

void Widget::on_VoteStartPercentReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_votestartpercent" );
    gUNM->m_VoteStartPercent = CFG.GetUInt( "bot_votestartpercent", 80, 1, 100 );
    ui->VoteStartPercentSpin->setValue( gUNM->m_VoteStartPercent );
}

void Widget::on_VoteStartDurationApply_clicked( )
{
    gUNM->m_VoteStartDuration = ui->VoteStartDurationSpin->value( );
}

void Widget::on_VoteStartDurationSave_clicked( )
{
    gUNM->m_VoteStartDuration = ui->VoteStartDurationSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_votestartduration", UTIL_ToString( gUNM->m_VoteStartDuration ) );
}

void Widget::on_VoteStartDurationRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_VoteStartDuration = CFG.GetUInt( "bot_votestartduration", 0 );
    ui->VoteStartDurationSpin->setValue( gUNM->m_VoteStartDuration );
}

void Widget::on_VoteStartDurationReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_votestartduration" );
    gUNM->m_VoteStartDuration = CFG.GetUInt( "bot_votestartduration", 0 );
    ui->VoteStartDurationSpin->setValue( gUNM->m_VoteStartDuration );
}

void Widget::on_VoteKickAllowedApply_clicked( )
{
    ui->VoteKickAllowedCombo->currentIndex( ) == 0 ? gUNM->m_VoteKickAllowed = true : gUNM->m_VoteKickAllowed = false;
}

void Widget::on_VoteKickAllowedSave_clicked( )
{
    if( ui->VoteKickAllowedCombo->currentIndex( ) == 0 )
    {
        gUNM->m_VoteKickAllowed = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_votekickallowed", "1" );
    }
    else
    {
        gUNM->m_VoteKickAllowed = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_votekickallowed", "0" );
    }
}

void Widget::on_VoteKickAllowedRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_VoteKickAllowed = CFG.GetInt( "bot_votekickallowed", 1 ) != 0;

    if( gUNM->m_VoteKickAllowed )
        ui->VoteKickAllowedCombo->setCurrentIndex( 0 );
    else
        ui->VoteKickAllowedCombo->setCurrentIndex( 1 );
}

void Widget::on_VoteKickAllowedReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_votekickallowed" );
    gUNM->m_VoteKickAllowed = CFG.GetInt( "bot_votekickallowed", 1 ) != 0;

    if( gUNM->m_VoteKickAllowed )
        ui->VoteKickAllowedCombo->setCurrentIndex( 0 );
    else
        ui->VoteKickAllowedCombo->setCurrentIndex( 1 );
}

void Widget::on_VoteKickPercentageApply_clicked( )
{
    gUNM->m_VoteKickPercentage = ui->VoteKickPercentageSpin->value( );
}

void Widget::on_VoteKickPercentageSave_clicked( )
{
    gUNM->m_VoteKickPercentage = ui->VoteKickPercentageSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_votekickpercentage", UTIL_ToString( gUNM->m_VoteKickPercentage ) );
}

void Widget::on_VoteKickPercentageRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_VoteKickPercentage = CFG.GetUInt( "bot_votekickpercentage", 66, 1, 100 );
    ui->VoteKickPercentageSpin->setValue( gUNM->m_VoteKickPercentage );
}

void Widget::on_VoteKickPercentageReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_votekickpercentage" );
    gUNM->m_VoteKickPercentage = CFG.GetUInt( "bot_votekickpercentage", 66, 1, 100 );
    ui->VoteKickPercentageSpin->setValue( gUNM->m_VoteKickPercentage );
}

void Widget::on_OptionOwnerApply_clicked( )
{
    gUNM->m_OptionOwner = ui->OptionOwnerCombo->currentIndex( );
}

void Widget::on_OptionOwnerSave_clicked( )
{
    gUNM->m_OptionOwner = ui->OptionOwnerCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_optionowner", UTIL_ToString( gUNM->m_OptionOwner ) );
}

void Widget::on_OptionOwnerRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_OptionOwner = CFG.GetUInt( "bot_optionowner", 3, 4 );
    ui->OptionOwnerCombo->setCurrentIndex( gUNM->m_OptionOwner );
}

void Widget::on_OptionOwnerReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_optionowner" );
    gUNM->m_OptionOwner = CFG.GetUInt( "bot_optionowner", 3, 4 );
    ui->OptionOwnerCombo->setCurrentIndex( gUNM->m_OptionOwner );
}

void Widget::on_AFKkickApply_clicked( )
{
    gUNM->m_AFKkick = ui->AFKkickCombo->currentIndex( );
}

void Widget::on_AFKkickSave_clicked( )
{
    gUNM->m_AFKkick = ui->AFKkickCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_afkkick", UTIL_ToString( gUNM->m_AFKkick ) );
}

void Widget::on_AFKkickRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AFKkick = CFG.GetUInt( "bot_afkkick", 1, 2 );
    ui->AFKkickCombo->setCurrentIndex( gUNM->m_AFKkick );
}

void Widget::on_AFKkickReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_afkkick" );
    gUNM->m_AFKkick = CFG.GetUInt( "bot_afkkick", 1, 2 );
    ui->AFKkickCombo->setCurrentIndex( gUNM->m_AFKkick );
}

void Widget::on_AFKtimerApply_clicked( )
{
    gUNM->m_AFKtimer = ui->AFKtimerSpin->value( );
}

void Widget::on_AFKtimerSave_clicked( )
{
    gUNM->m_AFKtimer = ui->AFKtimerSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_afktimer", UTIL_ToString( gUNM->m_AFKtimer ) );
}

void Widget::on_AFKtimerRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AFKtimer = CFG.GetUInt( "bot_afktimer", 300 );
    ui->AFKtimerSpin->setValue( gUNM->m_AFKtimer );
}

void Widget::on_AFKtimerReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_afktimer" );
    gUNM->m_AFKtimer = CFG.GetUInt( "bot_afktimer", 300 );
    ui->AFKtimerSpin->setValue( gUNM->m_AFKtimer );
}

void Widget::on_ReconnectWaitTimeApply_clicked( )
{
    gUNM->m_ReconnectWaitTime = ui->ReconnectWaitTimeSpin->value( );
}

void Widget::on_ReconnectWaitTimeSave_clicked( )
{
    gUNM->m_ReconnectWaitTime = ui->ReconnectWaitTimeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_reconnectwaittime", UTIL_ToString( gUNM->m_ReconnectWaitTime ) );
}

void Widget::on_ReconnectWaitTimeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ReconnectWaitTime = CFG.GetUInt( "bot_reconnectwaittime", 2 );
    ui->ReconnectWaitTimeSpin->setValue( gUNM->m_ReconnectWaitTime );
}

void Widget::on_ReconnectWaitTimeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_reconnectwaittime" );
    gUNM->m_ReconnectWaitTime = CFG.GetUInt( "bot_reconnectwaittime", 2 );
    ui->ReconnectWaitTimeSpin->setValue( gUNM->m_ReconnectWaitTime );
}

void Widget::on_DropVotePercentApply_clicked( )
{
    gUNM->m_DropVotePercent = ui->DropVotePercentSpin->value( );
}

void Widget::on_DropVotePercentSave_clicked( )
{
    gUNM->m_DropVotePercent = ui->DropVotePercentSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dropvotepercent", UTIL_ToString( gUNM->m_DropVotePercent ) );
}

void Widget::on_DropVotePercentRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DropVotePercent = CFG.GetUInt( "bot_dropvotepercent", 66 );
    ui->DropVotePercentSpin->setValue( gUNM->m_DropVotePercent );
}

void Widget::on_DropVotePercentReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dropvotepercent" );
    gUNM->m_DropVotePercent = CFG.GetUInt( "bot_dropvotepercent", 66 );
    ui->DropVotePercentSpin->setValue( gUNM->m_DropVotePercent );
}

void Widget::on_DropVoteTimeApply_clicked( )
{
    gUNM->m_DropVoteTime = ui->DropVoteTimeSpin->value( );
}

void Widget::on_DropVoteTimeSave_clicked( )
{
    gUNM->m_DropVoteTime = ui->DropVoteTimeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dropvotetime", UTIL_ToString( gUNM->m_DropVoteTime ) );
}

void Widget::on_DropVoteTimeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DropVoteTime = CFG.GetUInt( "bot_dropvotetime", 5 );
    ui->DropVoteTimeSpin->setValue( gUNM->m_DropVoteTime );
}

void Widget::on_DropVoteTimeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dropvotetime" );
    gUNM->m_DropVoteTime = CFG.GetUInt( "bot_dropvotetime", 5 );
    ui->DropVoteTimeSpin->setValue( gUNM->m_DropVoteTime );
}

void Widget::on_DropWaitTimeApply_clicked( )
{
    gUNM->m_DropWaitTime = ui->DropWaitTimeSpin->value( );
}

void Widget::on_DropWaitTimeSave_clicked( )
{
    gUNM->m_DropWaitTime = ui->DropWaitTimeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dropwaittime", UTIL_ToString( gUNM->m_DropWaitTime ) );
}

void Widget::on_DropWaitTimeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DropWaitTime = CFG.GetUInt( "bot_dropwaittime", 60 );
    ui->DropWaitTimeSpin->setValue( gUNM->m_DropWaitTime );
}

void Widget::on_DropWaitTimeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dropwaittime" );
    gUNM->m_DropWaitTime = CFG.GetUInt( "bot_dropwaittime", 60 );
    ui->DropWaitTimeSpin->setValue( gUNM->m_DropWaitTime );
}

void Widget::on_DropWaitTimeSumApply_clicked( )
{
    gUNM->m_DropWaitTimeSum = ui->DropWaitTimeSumSpin->value( );
}

void Widget::on_DropWaitTimeSumSave_clicked( )
{
    gUNM->m_DropWaitTimeSum = ui->DropWaitTimeSumSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dropwaittimesum", UTIL_ToString( gUNM->m_DropWaitTimeSum ) );
}

void Widget::on_DropWaitTimeSumRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DropWaitTimeSum = CFG.GetUInt( "bot_dropwaittimesum", 120 );
    ui->DropWaitTimeSumSpin->setValue( gUNM->m_DropWaitTimeSum );
}

void Widget::on_DropWaitTimeSumReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dropwaittimesum" );
    gUNM->m_DropWaitTimeSum = CFG.GetUInt( "bot_dropwaittimesum", 120 );
    ui->DropWaitTimeSumSpin->setValue( gUNM->m_DropWaitTimeSum );
}

void Widget::on_DropWaitTimeGameApply_clicked( )
{
    gUNM->m_DropWaitTimeGame = ui->DropWaitTimeGameSpin->value( );
}

void Widget::on_DropWaitTimeGameSave_clicked( )
{
    gUNM->m_DropWaitTimeGame = ui->DropWaitTimeGameSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dropwaittimegame", UTIL_ToString( gUNM->m_DropWaitTimeGame ) );
}

void Widget::on_DropWaitTimeGameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DropWaitTimeGame = CFG.GetUInt( "bot_dropwaittimegame", 90 );
    ui->DropWaitTimeGameSpin->setValue( gUNM->m_DropWaitTimeGame );
}

void Widget::on_DropWaitTimeGameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dropwaittimegame" );
    gUNM->m_DropWaitTimeGame = CFG.GetUInt( "bot_dropwaittimegame", 90 );
    ui->DropWaitTimeGameSpin->setValue( gUNM->m_DropWaitTimeGame );
}

void Widget::on_DropWaitTimeLoadApply_clicked( )
{
    gUNM->m_DropWaitTimeLoad = ui->DropWaitTimeLoadSpin->value( );
}

void Widget::on_DropWaitTimeLoadSave_clicked( )
{
    gUNM->m_DropWaitTimeLoad = ui->DropWaitTimeLoadSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dropwaittimeload", UTIL_ToString( gUNM->m_DropWaitTimeLoad ) );
}

void Widget::on_DropWaitTimeLoadRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DropWaitTimeLoad = CFG.GetUInt( "bot_dropwaittimeload", 30 );
    ui->DropWaitTimeLoadSpin->setValue( gUNM->m_DropWaitTimeLoad );
}

void Widget::on_DropWaitTimeLoadReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dropwaittimeload" );
    gUNM->m_DropWaitTimeLoad = CFG.GetUInt( "bot_dropwaittimeload", 30 );
    ui->DropWaitTimeLoadSpin->setValue( gUNM->m_DropWaitTimeLoad );
}

void Widget::on_DropWaitTimeLobbyApply_clicked( )
{
    gUNM->m_DropWaitTimeLobby = ui->DropWaitTimeLobbySpin->value( );
}

void Widget::on_DropWaitTimeLobbySave_clicked( )
{
    gUNM->m_DropWaitTimeLobby = ui->DropWaitTimeLobbySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_dropwaittimelobby", UTIL_ToString( gUNM->m_DropWaitTimeLobby ) );
}

void Widget::on_DropWaitTimeLobbyRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DropWaitTimeLobby = CFG.GetUInt( "bot_dropwaittimelobby", 15 );
    ui->DropWaitTimeLobbySpin->setValue( gUNM->m_DropWaitTimeLobby );
}

void Widget::on_DropWaitTimeLobbyReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dropwaittimelobby" );
    gUNM->m_DropWaitTimeLobby = CFG.GetUInt( "bot_dropwaittimelobby", 15 );
    ui->DropWaitTimeLobbySpin->setValue( gUNM->m_DropWaitTimeLobby );
}

void Widget::on_AllowLaggingTimeApply_clicked( )
{
    gUNM->m_AllowLaggingTime = ui->AllowLaggingTimeSpin->value( );
}

void Widget::on_AllowLaggingTimeSave_clicked( )
{
    gUNM->m_AllowLaggingTime = ui->AllowLaggingTimeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_allowlaggingtime", UTIL_ToString( gUNM->m_AllowLaggingTime ) );
}

void Widget::on_AllowLaggingTimeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AllowLaggingTime = CFG.GetUInt( "bot_allowlaggingtime", 1 );
    ui->AllowLaggingTimeSpin->setValue( gUNM->m_AllowLaggingTime );
}

void Widget::on_AllowLaggingTimeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_allowlaggingtime" );
    gUNM->m_AllowLaggingTime = CFG.GetUInt( "bot_allowlaggingtime", 1 );
    ui->AllowLaggingTimeSpin->setValue( gUNM->m_AllowLaggingTime );
}

void Widget::on_AllowLaggingTimeIntervalApply_clicked( )
{
    gUNM->m_AllowLaggingTimeInterval = ui->AllowLaggingTimeIntervalSpin->value( );
}

void Widget::on_AllowLaggingTimeIntervalSave_clicked( )
{
    gUNM->m_AllowLaggingTimeInterval = ui->AllowLaggingTimeIntervalSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_allowlaggingtimeinterval", UTIL_ToString( gUNM->m_AllowLaggingTimeInterval ) );
}

void Widget::on_AllowLaggingTimeIntervalRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AllowLaggingTimeInterval = CFG.GetUInt( "bot_allowlaggingtimeinterval", 1 );

	if( gUNM->m_AllowLaggingTimeInterval > 71582788 )
		gUNM->m_AllowLaggingTimeInterval = 71582788;

    gUNM->m_AllowLaggingTimeInterval = gUNM->m_AllowLaggingTimeInterval * 60;
    ui->AllowLaggingTimeIntervalSpin->setValue( gUNM->m_AllowLaggingTimeInterval );
}

void Widget::on_AllowLaggingTimeIntervalReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_allowlaggingtimeinterval" );
    gUNM->m_AllowLaggingTimeInterval = CFG.GetUInt( "bot_allowlaggingtimeinterval", 1 );

	if( gUNM->m_AllowLaggingTimeInterval > 71582788 )
		gUNM->m_AllowLaggingTimeInterval = 71582788;

    gUNM->m_AllowLaggingTimeInterval = gUNM->m_AllowLaggingTimeInterval * 60;
    ui->AllowLaggingTimeIntervalSpin->setValue( gUNM->m_AllowLaggingTimeInterval );
}

void Widget::on_UserCanDropAdminApply_clicked( )
{
    ui->UserCanDropAdminCombo->currentIndex( ) == 0 ? gUNM->m_UserCanDropAdmin = true : gUNM->m_UserCanDropAdmin = false;
}

void Widget::on_UserCanDropAdminSave_clicked( )
{
    if( ui->UserCanDropAdminCombo->currentIndex( ) == 0 )
    {
        gUNM->m_UserCanDropAdmin = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_usercandropadmin", "1" );
    }
    else
    {
        gUNM->m_UserCanDropAdmin = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_usercandropadmin", "0" );
    }
}

void Widget::on_UserCanDropAdminRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_UserCanDropAdmin = CFG.GetInt( "bot_usercandropadmin", 0 ) != 0;

    if( gUNM->m_UserCanDropAdmin )
        ui->UserCanDropAdminCombo->setCurrentIndex( 0 );
    else
        ui->UserCanDropAdminCombo->setCurrentIndex( 1 );
}

void Widget::on_UserCanDropAdminReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_usercandropadmin" );
    gUNM->m_UserCanDropAdmin = CFG.GetInt( "bot_usercandropadmin", 0 ) != 0;

    if( gUNM->m_UserCanDropAdmin )
        ui->UserCanDropAdminCombo->setCurrentIndex( 0 );
    else
        ui->UserCanDropAdminCombo->setCurrentIndex( 1 );
}

void Widget::on_DropIfDesyncApply_clicked( )
{
    ui->DropIfDesyncCombo->currentIndex( ) == 0 ? gUNM->m_DropIfDesync = true : gUNM->m_DropIfDesync = false;
}

void Widget::on_DropIfDesyncSave_clicked( )
{
    if( ui->DropIfDesyncCombo->currentIndex( ) == 0 )
    {
        gUNM->m_DropIfDesync = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_dropifdesync", "1" );
    }
    else
    {
        gUNM->m_DropIfDesync = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_dropifdesync", "0" );
    }
}

void Widget::on_DropIfDesyncRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DropIfDesync = CFG.GetInt( "bot_dropifdesync", 0 ) != 0;

    if( gUNM->m_DropIfDesync )
        ui->DropIfDesyncCombo->setCurrentIndex( 0 );
    else
        ui->DropIfDesyncCombo->setCurrentIndex( 1 );
}

void Widget::on_DropIfDesyncReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dropifdesync" );
    gUNM->m_DropIfDesync = CFG.GetInt( "bot_dropifdesync", 0 ) != 0;

    if( gUNM->m_DropIfDesync )
        ui->DropIfDesyncCombo->setCurrentIndex( 0 );
    else
        ui->DropIfDesyncCombo->setCurrentIndex( 1 );
}

void Widget::on_gameoverminpercentApply_clicked( )
{
    gUNM->m_gameoverminpercent = ui->gameoverminpercentSpin->value( );
}

void Widget::on_gameoverminpercentSave_clicked( )
{
    gUNM->m_gameoverminpercent = ui->gameoverminpercentSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_gameoverminpercent", UTIL_ToString( gUNM->m_gameoverminpercent ) );
}

void Widget::on_gameoverminpercentRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_gameoverminpercent = CFG.GetUInt( "bot_gameoverminpercent", 0 );
    ui->gameoverminpercentSpin->setValue( gUNM->m_gameoverminpercent );
}

void Widget::on_gameoverminpercentReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gameoverminpercent" );
    gUNM->m_gameoverminpercent = CFG.GetUInt( "bot_gameoverminpercent", 0 );
    ui->gameoverminpercentSpin->setValue( gUNM->m_gameoverminpercent );
}

void Widget::on_gameoverminplayersApply_clicked( )
{
    gUNM->m_gameoverminplayers = ui->gameoverminplayersSpin->value( );
}

void Widget::on_gameoverminplayersSave_clicked( )
{
    gUNM->m_gameoverminplayers = ui->gameoverminplayersSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_gameoverminplayers", UTIL_ToString( gUNM->m_gameoverminplayers ) );
}

void Widget::on_gameoverminplayersRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_gameoverminplayers = CFG.GetUInt( "bot_gameoverminplayers", 0 );
    ui->gameoverminplayersSpin->setValue( gUNM->m_gameoverminplayers );
}

void Widget::on_gameoverminplayersReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gameoverminplayers" );
    gUNM->m_gameoverminplayers = CFG.GetUInt( "bot_gameoverminplayers", 0 );
    ui->gameoverminplayersSpin->setValue( gUNM->m_gameoverminplayers );
}

void Widget::on_gameovermaxteamdifferenceApply_clicked( )
{
    gUNM->m_gameovermaxteamdifference = ui->gameovermaxteamdifferenceSpin->value( );
}

void Widget::on_gameovermaxteamdifferenceSave_clicked( )
{
    gUNM->m_gameovermaxteamdifference = ui->gameovermaxteamdifferenceSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_gameovermaxteamdifference", UTIL_ToString( gUNM->m_gameovermaxteamdifference ) );
}

void Widget::on_gameovermaxteamdifferenceRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_gameovermaxteamdifference = CFG.GetUInt( "bot_gameovermaxteamdifference", 0 );
    ui->gameovermaxteamdifferenceSpin->setValue( gUNM->m_gameovermaxteamdifference );
}

void Widget::on_gameovermaxteamdifferenceReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gameovermaxteamdifference" );
    gUNM->m_gameovermaxteamdifference = CFG.GetUInt( "bot_gameovermaxteamdifference", 0 );
    ui->gameovermaxteamdifferenceSpin->setValue( gUNM->m_gameovermaxteamdifference );
}

void Widget::on_LobbyTimeLimitApply_clicked( )
{
    gUNM->m_LobbyTimeLimit = ui->LobbyTimeLimitSpin->value( );
}

void Widget::on_LobbyTimeLimitSave_clicked( )
{
    gUNM->m_LobbyTimeLimit = ui->LobbyTimeLimitSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_lobbytimelimit", UTIL_ToString( gUNM->m_LobbyTimeLimit ) );
}

void Widget::on_LobbyTimeLimitRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_LobbyTimeLimit = CFG.GetUInt( "bot_lobbytimelimit", 180 );
    ui->LobbyTimeLimitSpin->setValue( gUNM->m_LobbyTimeLimit );
}

void Widget::on_LobbyTimeLimitReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_lobbytimelimit" );
    gUNM->m_LobbyTimeLimit = CFG.GetUInt( "bot_lobbytimelimit", 180 );
    ui->LobbyTimeLimitSpin->setValue( gUNM->m_LobbyTimeLimit );
}

void Widget::on_LobbyTimeLimitMaxApply_clicked( )
{
    gUNM->m_LobbyTimeLimitMax = ui->LobbyTimeLimitMaxSpin->value( );
}

void Widget::on_LobbyTimeLimitMaxSave_clicked( )
{
    gUNM->m_LobbyTimeLimitMax = ui->LobbyTimeLimitMaxSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_lobbytimelimitmax", UTIL_ToString( gUNM->m_LobbyTimeLimitMax ) );
}

void Widget::on_LobbyTimeLimitMaxRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_LobbyTimeLimitMax = CFG.GetUInt( "bot_lobbytimelimitmax", 0 );
    ui->LobbyTimeLimitMaxSpin->setValue( gUNM->m_LobbyTimeLimitMax );
}

void Widget::on_LobbyTimeLimitMaxReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_lobbytimelimitmax" );
    gUNM->m_LobbyTimeLimitMax = CFG.GetUInt( "bot_lobbytimelimitmax", 0 );
    ui->LobbyTimeLimitMaxSpin->setValue( gUNM->m_LobbyTimeLimitMax );
}

void Widget::on_PubGameDelayApply_clicked( )
{
    gUNM->m_PubGameDelay = ui->PubGameDelaySpin->value( );
}

void Widget::on_PubGameDelaySave_clicked( )
{
    gUNM->m_PubGameDelay = ui->PubGameDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_pubgamedelay", UTIL_ToString( gUNM->m_PubGameDelay ) );
}

void Widget::on_PubGameDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PubGameDelay = CFG.GetUInt( "bot_pubgamedelay", 1 );
    ui->PubGameDelaySpin->setValue( gUNM->m_PubGameDelay );
}

void Widget::on_PubGameDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_pubgamedelay" );
    gUNM->m_PubGameDelay = CFG.GetUInt( "bot_pubgamedelay", 1 );
    ui->PubGameDelaySpin->setValue( gUNM->m_PubGameDelay );
}

void Widget::on_BotCommandTriggerLine_returnPressed( )
{
    on_BotCommandTriggerApply_clicked( );
}

void Widget::on_BotCommandTriggerApply_clicked( )
{
    gUNM->BotCommandTrigger = ui->BotCommandTriggerLine->text( ).toStdString( );
}

void Widget::on_BotCommandTriggerSave_clicked( )
{
    gUNM->BotCommandTrigger = ui->BotCommandTriggerLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_commandtrigger", gUNM->BotCommandTrigger );
}

void Widget::on_BotCommandTriggerRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->BotCommandTrigger = CFG.GetString( "bot_commandtrigger", "-!.@" );
    ui->BotCommandTriggerLine->setText( QString::fromUtf8( gUNM->BotCommandTrigger.c_str( ) ) );
}

void Widget::on_BotCommandTriggerReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_commandtrigger" );
    gUNM->BotCommandTrigger = CFG.GetString( "bot_commandtrigger", "-!.@" );
    ui->BotCommandTriggerLine->setText( QString::fromUtf8( gUNM->BotCommandTrigger.c_str( ) ) );
}

void Widget::on_VirtualHostNameLine_returnPressed( )
{
    on_VirtualHostNameApply_clicked( );
}

void Widget::on_VirtualHostNameApply_clicked( )
{
    gUNM->m_VirtualHostName = ui->VirtualHostNameLine->text( ).toStdString( );
}

void Widget::on_VirtualHostNameSave_clicked( )
{
    gUNM->m_VirtualHostName = ui->VirtualHostNameLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_virtualhostname", gUNM->m_VirtualHostName );
}

void Widget::on_VirtualHostNameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_VirtualHostName = CFG.GetString( "bot_virtualhostname", "|cFF00FF00unm" );
    ui->VirtualHostNameLine->setText( QString::fromUtf8( gUNM->m_VirtualHostName.c_str( ) ) );
}

void Widget::on_VirtualHostNameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_virtualhostname" );
    gUNM->m_VirtualHostName = CFG.GetString( "bot_virtualhostname", "|cFF00FF00unm" );
    ui->VirtualHostNameLine->setText( QString::fromUtf8( gUNM->m_VirtualHostName.c_str( ) ) );
}

void Widget::on_ObsPlayerNameLine_returnPressed( )
{
    on_ObsPlayerNameApply_clicked( );
}

void Widget::on_ObsPlayerNameApply_clicked( )
{
    gUNM->m_ObsPlayerName = ui->ObsPlayerNameLine->text( ).toStdString( );
}

void Widget::on_ObsPlayerNameSave_clicked( )
{
    gUNM->m_ObsPlayerName = ui->ObsPlayerNameLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_obsplayername", gUNM->m_ObsPlayerName );
}

void Widget::on_ObsPlayerNameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ObsPlayerName = CFG.GetString( "bot_obsplayername", "|cFF00FF00Obs" );
    ui->ObsPlayerNameLine->setText( QString::fromUtf8( gUNM->m_ObsPlayerName.c_str( ) ) );
}

void Widget::on_ObsPlayerNameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_obsplayername" );
    gUNM->m_ObsPlayerName = CFG.GetString( "bot_obsplayername", "|cFF00FF00Obs" );
    ui->ObsPlayerNameLine->setText( QString::fromUtf8( gUNM->m_ObsPlayerName.c_str( ) ) );
}

void Widget::on_DontDeleteVirtualHostApply_clicked( )
{
    ui->DontDeleteVirtualHostCombo->currentIndex( ) == 0 ? gUNM->m_DontDeleteVirtualHost = true : gUNM->m_DontDeleteVirtualHost = false;
}

void Widget::on_DontDeleteVirtualHostSave_clicked( )
{
    if( ui->DontDeleteVirtualHostCombo->currentIndex( ) == 0 )
    {
        gUNM->m_DontDeleteVirtualHost = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_dontdeletevirtualhost", "1" );
    }
    else
    {
        gUNM->m_DontDeleteVirtualHost = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_dontdeletevirtualhost", "0" );
    }
}

void Widget::on_DontDeleteVirtualHostRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DontDeleteVirtualHost = CFG.GetInt( "bot_dontdeletevirtualhost", 0 ) != 0;

    if( gUNM->m_DontDeleteVirtualHost )
        ui->DontDeleteVirtualHostCombo->setCurrentIndex( 0 );
    else
        ui->DontDeleteVirtualHostCombo->setCurrentIndex( 1 );
}

void Widget::on_DontDeleteVirtualHostReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dontdeletevirtualhost" );
    gUNM->m_DontDeleteVirtualHost = CFG.GetInt( "bot_dontdeletevirtualhost", 0 ) != 0;

    if( gUNM->m_DontDeleteVirtualHost )
        ui->DontDeleteVirtualHostCombo->setCurrentIndex( 0 );
    else
        ui->DontDeleteVirtualHostCombo->setCurrentIndex( 1 );
}

void Widget::on_DontDeletePenultimatePlayerApply_clicked( )
{
    ui->DontDeletePenultimatePlayerCombo->currentIndex( ) == 0 ? gUNM->m_DontDeletePenultimatePlayer = true : gUNM->m_DontDeletePenultimatePlayer = false;
}

void Widget::on_DontDeletePenultimatePlayerSave_clicked( )
{
    if( ui->DontDeletePenultimatePlayerCombo->currentIndex( ) == 0 )
    {
        gUNM->m_DontDeletePenultimatePlayer = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_dontdeletepenultimateplayer", "1" );
    }
    else
    {
        gUNM->m_DontDeletePenultimatePlayer = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_dontdeletepenultimateplayer", "0" );
    }
}

void Widget::on_DontDeletePenultimatePlayerRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DontDeletePenultimatePlayer = CFG.GetInt( "bot_dontdeletepenultimateplayer", 0 ) != 0;

    if( gUNM->m_DontDeletePenultimatePlayer )
        ui->DontDeletePenultimatePlayerCombo->setCurrentIndex( 0 );
    else
        ui->DontDeletePenultimatePlayerCombo->setCurrentIndex( 1 );
}

void Widget::on_DontDeletePenultimatePlayerReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_dontdeletepenultimateplayer" );
    gUNM->m_DontDeletePenultimatePlayer = CFG.GetInt( "bot_dontdeletepenultimateplayer", 0 ) != 0;

    if( gUNM->m_DontDeletePenultimatePlayer )
        ui->DontDeletePenultimatePlayerCombo->setCurrentIndex( 0 );
    else
        ui->DontDeletePenultimatePlayerCombo->setCurrentIndex( 1 );
}

void Widget::on_NumRequiredPingsPlayersApply_clicked( )
{
    gUNM->m_NumRequiredPingsPlayers = ui->NumRequiredPingsPlayersSpin->value( );
}

void Widget::on_NumRequiredPingsPlayersSave_clicked( )
{
    gUNM->m_NumRequiredPingsPlayers = ui->NumRequiredPingsPlayersSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_numrequiredpingsplayers", UTIL_ToString( gUNM->m_NumRequiredPingsPlayers ) );
}

void Widget::on_NumRequiredPingsPlayersRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NumRequiredPingsPlayers = CFG.GetUInt( "bot_numrequiredpingsplayers", 1 );
    ui->NumRequiredPingsPlayersSpin->setValue( gUNM->m_NumRequiredPingsPlayers );
}

void Widget::on_NumRequiredPingsPlayersReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_numrequiredpingsplayers" );
    gUNM->m_NumRequiredPingsPlayers = CFG.GetUInt( "bot_numrequiredpingsplayers", 1 );
    ui->NumRequiredPingsPlayersSpin->setValue( gUNM->m_NumRequiredPingsPlayers );
}

void Widget::on_NumRequiredPingsPlayersASApply_clicked( )
{
    gUNM->m_NumRequiredPingsPlayersAS = ui->NumRequiredPingsPlayersASSpin->value( );
}

void Widget::on_NumRequiredPingsPlayersASSave_clicked( )
{
    gUNM->m_NumRequiredPingsPlayersAS = ui->NumRequiredPingsPlayersASSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_numrequiredpingsplayersas", UTIL_ToString( gUNM->m_NumRequiredPingsPlayersAS ) );
}

void Widget::on_NumRequiredPingsPlayersASRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NumRequiredPingsPlayersAS = CFG.GetUInt( "bot_numrequiredpingsplayersas", 3 );
    ui->NumRequiredPingsPlayersASSpin->setValue( gUNM->m_NumRequiredPingsPlayersAS );
}

void Widget::on_NumRequiredPingsPlayersASReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_numrequiredpingsplayersas" );
    gUNM->m_NumRequiredPingsPlayersAS = CFG.GetUInt( "bot_numrequiredpingsplayersas", 3 );
    ui->NumRequiredPingsPlayersASSpin->setValue( gUNM->m_NumRequiredPingsPlayersAS );
}

void Widget::on_AntiFloodMuteApply_clicked( )
{
    ui->AntiFloodMuteCombo->currentIndex( ) == 0 ? gUNM->m_AntiFloodMute = true : gUNM->m_AntiFloodMute = false;
}

void Widget::on_AntiFloodMuteSave_clicked( )
{
    if( ui->AntiFloodMuteCombo->currentIndex( ) == 0 )
    {
        gUNM->m_AntiFloodMute = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_antifloodmute", "1" );
    }
    else
    {
        gUNM->m_AntiFloodMute = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_antifloodmute", "0" );
    }
}

void Widget::on_AntiFloodMuteRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AntiFloodMute = CFG.GetInt( "bot_antifloodmute", 0 ) != 0;

    if( gUNM->m_AntiFloodMute )
        ui->AntiFloodMuteCombo->setCurrentIndex( 0 );
    else
        ui->AntiFloodMuteCombo->setCurrentIndex( 1 );
}

void Widget::on_AntiFloodMuteReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_antifloodmute" );
    gUNM->m_AntiFloodMute = CFG.GetInt( "bot_antifloodmute", 0 ) != 0;

    if( gUNM->m_AntiFloodMute )
        ui->AntiFloodMuteCombo->setCurrentIndex( 0 );
    else
        ui->AntiFloodMuteCombo->setCurrentIndex( 1 );
}

void Widget::on_AdminCanUnAutoMuteApply_clicked( )
{
    ui->AdminCanUnAutoMuteCombo->currentIndex( ) == 0 ? gUNM->m_AdminCanUnAutoMute = true : gUNM->m_AdminCanUnAutoMute = false;
}

void Widget::on_AdminCanUnAutoMuteSave_clicked( )
{
    if( ui->AdminCanUnAutoMuteCombo->currentIndex( ) == 0 )
    {
        gUNM->m_AdminCanUnAutoMute = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_admincanunautomute", "1" );
    }
    else
    {
        gUNM->m_AdminCanUnAutoMute = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_admincanunautomute", "0" );
    }
}

void Widget::on_AdminCanUnAutoMuteRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AdminCanUnAutoMute = CFG.GetInt( "bot_admincanunautomute", 1 ) != 0;

    if( gUNM->m_AdminCanUnAutoMute )
        ui->AdminCanUnAutoMuteCombo->setCurrentIndex( 0 );
    else
        ui->AdminCanUnAutoMuteCombo->setCurrentIndex( 1 );
}

void Widget::on_AdminCanUnAutoMuteReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_admincanunautomute" );
    gUNM->m_AdminCanUnAutoMute = CFG.GetInt( "bot_admincanunautomute", 1 ) != 0;

    if( gUNM->m_AdminCanUnAutoMute )
        ui->AdminCanUnAutoMuteCombo->setCurrentIndex( 0 );
    else
        ui->AdminCanUnAutoMuteCombo->setCurrentIndex( 1 );
}

void Widget::on_AntiFloodMuteSecondsApply_clicked( )
{
    gUNM->m_AntiFloodMuteSeconds = ui->AntiFloodMuteSecondsSpin->value( );
}

void Widget::on_AntiFloodMuteSecondsSave_clicked( )
{
    gUNM->m_AntiFloodMuteSeconds = ui->AntiFloodMuteSecondsSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_antifloodmuteseconds", UTIL_ToString( gUNM->m_AntiFloodMuteSeconds ) );
}

void Widget::on_AntiFloodMuteSecondsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AntiFloodMuteSeconds = CFG.GetUInt( "bot_antifloodmuteseconds", 15 );
    ui->AntiFloodMuteSecondsSpin->setValue( gUNM->m_AntiFloodMuteSeconds );
}

void Widget::on_AntiFloodMuteSecondsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_antifloodmuteseconds" );
    gUNM->m_AntiFloodMuteSeconds = CFG.GetUInt( "bot_antifloodmuteseconds", 15 );
    ui->AntiFloodMuteSecondsSpin->setValue( gUNM->m_AntiFloodMuteSeconds );
}

void Widget::on_AntiFloodRigidityTimeApply_clicked( )
{
    gUNM->m_AntiFloodRigidityTime = ui->AntiFloodRigidityTimeSpin->value( );
}

void Widget::on_AntiFloodRigidityTimeSave_clicked( )
{
    gUNM->m_AntiFloodRigidityTime = ui->AntiFloodRigidityTimeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_antifloodrigiditytime", UTIL_ToString( gUNM->m_AntiFloodRigidityTime ) );
}

void Widget::on_AntiFloodRigidityTimeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AntiFloodRigidityTime = CFG.GetUInt( "bot_antifloodrigiditytime", 5 );
    ui->AntiFloodRigidityTimeSpin->setValue( gUNM->m_AntiFloodRigidityTime );
}

void Widget::on_AntiFloodRigidityTimeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_antifloodrigiditytime" );
    gUNM->m_AntiFloodRigidityTime = CFG.GetUInt( "bot_antifloodrigiditytime", 5 );
    ui->AntiFloodRigidityTimeSpin->setValue( gUNM->m_AntiFloodRigidityTime );
}

void Widget::on_AntiFloodRigidityNumberApply_clicked( )
{
    gUNM->m_AntiFloodRigidityNumber = ui->AntiFloodRigidityNumberSpin->value( );
}

void Widget::on_AntiFloodRigidityNumberSave_clicked( )
{
    gUNM->m_AntiFloodRigidityNumber = ui->AntiFloodRigidityNumberSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_antifloodrigiditynumber", UTIL_ToString( gUNM->m_AntiFloodRigidityNumber ) );
}

void Widget::on_AntiFloodRigidityNumberRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AntiFloodRigidityNumber = CFG.GetUInt( "bot_antifloodrigiditynumber", 7 );
    ui->AntiFloodRigidityNumberSpin->setValue( gUNM->m_AntiFloodRigidityNumber );
}

void Widget::on_AntiFloodRigidityNumberReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_antifloodrigiditynumber" );
    gUNM->m_AntiFloodRigidityNumber = CFG.GetUInt( "bot_antifloodrigiditynumber", 7 );
    ui->AntiFloodRigidityNumberSpin->setValue( gUNM->m_AntiFloodRigidityNumber );
}

void Widget::on_AdminCanUnCensorMuteApply_clicked( )
{
    ui->AdminCanUnCensorMuteCombo->currentIndex( ) == 0 ? gUNM->m_AdminCanUnCensorMute = true : gUNM->m_AdminCanUnCensorMute = false;
}

void Widget::on_AdminCanUnCensorMuteSave_clicked( )
{
    if( ui->AdminCanUnCensorMuteCombo->currentIndex( ) == 0 )
    {
        gUNM->m_AdminCanUnCensorMute = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_admincanuncensormute", "1" );
    }
    else
    {
        gUNM->m_AdminCanUnCensorMute = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_admincanuncensormute", "0" );
    }
}

void Widget::on_AdminCanUnCensorMuteRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AdminCanUnCensorMute = CFG.GetInt( "bot_admincanuncensormute", 1 ) != 0;

    if( gUNM->m_AdminCanUnCensorMute )
        ui->AdminCanUnCensorMuteCombo->setCurrentIndex( 0 );
    else
        ui->AdminCanUnCensorMuteCombo->setCurrentIndex( 1 );
}

void Widget::on_AdminCanUnCensorMuteReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_admincanuncensormute" );
    gUNM->m_AdminCanUnCensorMute = CFG.GetInt( "bot_admincanuncensormute", 1 ) != 0;

    if( gUNM->m_AdminCanUnCensorMute )
        ui->AdminCanUnCensorMuteCombo->setCurrentIndex( 0 );
    else
        ui->AdminCanUnCensorMuteCombo->setCurrentIndex( 1 );
}

void Widget::on_CensorMuteApply_clicked( )
{
    ui->CensorMuteCombo->currentIndex( ) == 0 ? gUNM->m_CensorMute = true : gUNM->m_CensorMute = false;
}

void Widget::on_CensorMuteSave_clicked( )
{
    if( ui->CensorMuteCombo->currentIndex( ) == 0 )
    {
        gUNM->m_CensorMute = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_censormute", "1" );
    }
    else
    {
        gUNM->m_CensorMute = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_censormute", "0" );
    }
}

void Widget::on_CensorMuteRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_CensorMute = CFG.GetInt( "bot_censormute", 0 ) != 0;

    if( gUNM->m_CensorMute )
        ui->CensorMuteCombo->setCurrentIndex( 0 );
    else
        ui->CensorMuteCombo->setCurrentIndex( 1 );
}

void Widget::on_CensorMuteReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_censormute" );
    gUNM->m_CensorMute = CFG.GetInt( "bot_censormute", 0 ) != 0;

    if( gUNM->m_CensorMute )
        ui->CensorMuteCombo->setCurrentIndex( 0 );
    else
        ui->CensorMuteCombo->setCurrentIndex( 1 );
}

void Widget::on_CensorMuteAdminsApply_clicked( )
{
    ui->CensorMuteAdminsCombo->currentIndex( ) == 0 ? gUNM->m_CensorMuteAdmins = true : gUNM->m_CensorMuteAdmins = false;
}

void Widget::on_CensorMuteAdminsSave_clicked( )
{
    if( ui->CensorMuteAdminsCombo->currentIndex( ) == 0 )
    {
        gUNM->m_CensorMuteAdmins = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_censormuteadmins", "1" );
    }
    else
    {
        gUNM->m_CensorMuteAdmins = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_censormuteadmins", "0" );
    }
}

void Widget::on_CensorMuteAdminsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_CensorMuteAdmins = CFG.GetInt( "bot_censormuteadmins", 1 ) != 0;

    if( gUNM->m_CensorMuteAdmins )
        ui->CensorMuteAdminsCombo->setCurrentIndex( 0 );
    else
        ui->CensorMuteAdminsCombo->setCurrentIndex( 1 );
}

void Widget::on_CensorMuteAdminsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_censormuteadmins" );
    gUNM->m_CensorMuteAdmins = CFG.GetInt( "bot_censormuteadmins", 1 ) != 0;

    if( gUNM->m_CensorMuteAdmins )
        ui->CensorMuteAdminsCombo->setCurrentIndex( 0 );
    else
        ui->CensorMuteAdminsCombo->setCurrentIndex( 1 );
}

void Widget::on_CensorApply_clicked( )
{
    ui->CensorCombo->currentIndex( ) == 0 ? gUNM->m_Censor = true : gUNM->m_Censor = false;
}

void Widget::on_CensorSave_clicked( )
{
    if( ui->CensorCombo->currentIndex( ) == 0 )
    {
        gUNM->m_Censor = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_censor", "1" );
    }
    else
    {
        gUNM->m_Censor = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_censor", "0" );
    }
}

void Widget::on_CensorRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_Censor = CFG.GetInt( "bot_censor", 0 ) != 0;

    if( gUNM->m_Censor )
        ui->CensorCombo->setCurrentIndex( 0 );
    else
        ui->CensorCombo->setCurrentIndex( 1 );
}

void Widget::on_CensorReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_censor" );
    gUNM->m_Censor = CFG.GetInt( "bot_censor", 0 ) != 0;

    if( gUNM->m_Censor )
        ui->CensorCombo->setCurrentIndex( 0 );
    else
        ui->CensorCombo->setCurrentIndex( 1 );
}

void Widget::on_CensorDictionaryApply_clicked( )
{
    gUNM->m_CensorDictionary = ui->CensorDictionaryCombo->currentIndex( );
}

void Widget::on_CensorDictionarySave_clicked( )
{
    gUNM->m_CensorDictionary = ui->CensorDictionaryCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_censordictionary", UTIL_ToString( gUNM->m_CensorDictionary ) );
}

void Widget::on_CensorDictionaryRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_CensorDictionary = CFG.GetUInt( "bot_censordictionary", 3, 3 );
    ui->CensorDictionaryCombo->setCurrentIndex( gUNM->m_CensorDictionary );
}

void Widget::on_CensorDictionaryReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_censordictionary" );
    gUNM->m_CensorDictionary = CFG.GetUInt( "bot_censordictionary", 3, 3 );
    ui->CensorDictionaryCombo->setCurrentIndex( gUNM->m_CensorDictionary );
}

void Widget::on_CensorMuteFirstSecondsApply_clicked( )
{
    gUNM->m_CensorMuteFirstSeconds = ui->CensorMuteFirstSecondsSpin->value( );
}

void Widget::on_CensorMuteFirstSecondsSave_clicked( )
{
    gUNM->m_CensorMuteFirstSeconds = ui->CensorMuteFirstSecondsSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_censormutefirstseconds", UTIL_ToString( gUNM->m_CensorMuteFirstSeconds ) );
}

void Widget::on_CensorMuteFirstSecondsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_CensorMuteFirstSeconds = CFG.GetUInt( "bot_censormutefirstseconds", 5 );
    ui->CensorMuteFirstSecondsSpin->setValue( gUNM->m_CensorMuteFirstSeconds );
}

void Widget::on_CensorMuteFirstSecondsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_censormutefirstseconds" );
    gUNM->m_CensorMuteFirstSeconds = CFG.GetUInt( "bot_censormutefirstseconds", 5 );
    ui->CensorMuteFirstSecondsSpin->setValue( gUNM->m_CensorMuteFirstSeconds );
}

void Widget::on_CensorMuteSecondSecondsApply_clicked( )
{
    gUNM->m_CensorMuteSecondSeconds = ui->CensorMuteSecondSecondsSpin->value( );
}

void Widget::on_CensorMuteSecondSecondsSave_clicked( )
{
    gUNM->m_CensorMuteSecondSeconds = ui->CensorMuteSecondSecondsSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_censormutesecondseconds", UTIL_ToString( gUNM->m_CensorMuteSecondSeconds ) );
}

void Widget::on_CensorMuteSecondSecondsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_CensorMuteSecondSeconds = CFG.GetUInt( "bot_censormutesecondseconds", 10 );
    ui->CensorMuteSecondSecondsSpin->setValue( gUNM->m_CensorMuteSecondSeconds );
}

void Widget::on_CensorMuteSecondSecondsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_censormutesecondseconds" );
    gUNM->m_CensorMuteSecondSeconds = CFG.GetUInt( "bot_censormutesecondseconds", 10 );
    ui->CensorMuteSecondSecondsSpin->setValue( gUNM->m_CensorMuteSecondSeconds );
}

void Widget::on_CensorMuteExcessiveSecondsApply_clicked( )
{
    gUNM->m_CensorMuteExcessiveSeconds = ui->CensorMuteExcessiveSecondsSpin->value( );
}

void Widget::on_CensorMuteExcessiveSecondsSave_clicked( )
{
    gUNM->m_CensorMuteExcessiveSeconds = ui->CensorMuteExcessiveSecondsSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_censormuteexcessiveseconds", UTIL_ToString( gUNM->m_CensorMuteExcessiveSeconds ) );
}

void Widget::on_CensorMuteExcessiveSecondsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_CensorMuteExcessiveSeconds = CFG.GetUInt( "bot_censormuteexcessiveseconds", 15 );
    ui->CensorMuteExcessiveSecondsSpin->setValue( gUNM->m_CensorMuteExcessiveSeconds );
}

void Widget::on_CensorMuteExcessiveSecondsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_censormuteexcessiveseconds" );
    gUNM->m_CensorMuteExcessiveSeconds = CFG.GetUInt( "bot_censormuteexcessiveseconds", 15 );
    ui->CensorMuteExcessiveSecondsSpin->setValue( gUNM->m_CensorMuteExcessiveSeconds );
}

void Widget::on_NewCensorCookiesApply_clicked( )
{
    ui->NewCensorCookiesCombo->currentIndex( ) == 0 ? gUNM->m_NewCensorCookies = true : gUNM->m_NewCensorCookies = false;
}

void Widget::on_NewCensorCookiesSave_clicked( )
{
    if( ui->NewCensorCookiesCombo->currentIndex( ) == 0 )
    {
        gUNM->m_NewCensorCookies = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_newcensorcookies", "1" );
    }
    else
    {
        gUNM->m_NewCensorCookies = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_newcensorcookies", "0" );
    }
}

void Widget::on_NewCensorCookiesRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NewCensorCookies = CFG.GetInt( "bot_newcensorcookies", 1 ) != 0;

    if( gUNM->m_NewCensorCookies )
        ui->NewCensorCookiesCombo->setCurrentIndex( 0 );
    else
        ui->NewCensorCookiesCombo->setCurrentIndex( 1 );
}

void Widget::on_NewCensorCookiesReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_newcensorcookies" );
    gUNM->m_NewCensorCookies = CFG.GetInt( "bot_newcensorcookies", 1 ) != 0;

    if( gUNM->m_NewCensorCookies )
        ui->NewCensorCookiesCombo->setCurrentIndex( 0 );
    else
        ui->NewCensorCookiesCombo->setCurrentIndex( 1 );
}

void Widget::on_NewCensorCookiesNumberApply_clicked( )
{
    gUNM->m_NewCensorCookiesNumber = ui->NewCensorCookiesNumberSpin->value( );
}

void Widget::on_NewCensorCookiesNumberSave_clicked( )
{
    gUNM->m_NewCensorCookiesNumber = ui->NewCensorCookiesNumberSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_newcensorcookiesnumber", UTIL_ToString( gUNM->m_NewCensorCookiesNumber ) );
}

void Widget::on_NewCensorCookiesNumberRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NewCensorCookiesNumber = CFG.GetUInt( "bot_newcensorcookiesnumber", 5 );
    ui->NewCensorCookiesNumberSpin->setValue( gUNM->m_NewCensorCookiesNumber );
}

void Widget::on_NewCensorCookiesNumberReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_newcensorcookiesnumber" );
    gUNM->m_NewCensorCookiesNumber = CFG.GetUInt( "bot_newcensorcookiesnumber", 5 );
    ui->NewCensorCookiesNumberSpin->setValue( gUNM->m_NewCensorCookiesNumber );
}

void Widget::on_EatGameApply_clicked( )
{
    ui->EatGameCombo->currentIndex( ) == 0 ? gUNM->m_EatGame = true : gUNM->m_EatGame = false;
}

void Widget::on_EatGameSave_clicked( )
{
    if( ui->EatGameCombo->currentIndex( ) == 0 )
    {
        gUNM->m_EatGame = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_eatgame", "1" );
    }
    else
    {
        gUNM->m_EatGame = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_eatgame", "0" );
    }
}

void Widget::on_EatGameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_EatGame = CFG.GetInt( "bot_eatgame", 1 ) != 0;

    if( gUNM->m_EatGame )
        ui->EatGameCombo->setCurrentIndex( 0 );
    else
        ui->EatGameCombo->setCurrentIndex( 1 );
}

void Widget::on_EatGameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_eatgame" );
    gUNM->m_EatGame = CFG.GetInt( "bot_eatgame", 1 ) != 0;

    if( gUNM->m_EatGame )
        ui->EatGameCombo->setCurrentIndex( 0 );
    else
        ui->EatGameCombo->setCurrentIndex( 1 );
}

void Widget::on_MaximumNumberCookiesApply_clicked( )
{
    gUNM->m_MaximumNumberCookies = ui->MaximumNumberCookiesSpin->value( );
}

void Widget::on_MaximumNumberCookiesSave_clicked( )
{
    gUNM->m_MaximumNumberCookies = ui->MaximumNumberCookiesSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maximumnumbercookies", UTIL_ToString( gUNM->m_MaximumNumberCookies ) );
}

void Widget::on_MaximumNumberCookiesRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MaximumNumberCookies = CFG.GetUInt( "bot_maximumnumbercookies", 5 );
    ui->MaximumNumberCookiesSpin->setValue( gUNM->m_MaximumNumberCookies );
}

void Widget::on_MaximumNumberCookiesReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maximumnumbercookies" );
    gUNM->m_MaximumNumberCookies = CFG.GetUInt( "bot_maximumnumbercookies", 5 );
    ui->MaximumNumberCookiesSpin->setValue( gUNM->m_MaximumNumberCookies );
}

void Widget::on_NonAdminCommandsGameApply_clicked( )
{
    ui->NonAdminCommandsGameCombo->currentIndex( ) == 0 ? gUNM->m_NonAdminCommandsGame = true : gUNM->m_NonAdminCommandsGame = false;
}

void Widget::on_NonAdminCommandsGameSave_clicked( )
{
    if( ui->NonAdminCommandsGameCombo->currentIndex( ) == 0 )
    {
        gUNM->m_NonAdminCommandsGame = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_nonadmincommandsgame", "1" );
    }
    else
    {
        gUNM->m_NonAdminCommandsGame = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_nonadmincommandsgame", "0" );
    }
}

void Widget::on_NonAdminCommandsGameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NonAdminCommandsGame = CFG.GetInt( "bot_nonadmincommandsgame", 1 ) != 0;

    if( gUNM->m_NonAdminCommandsGame )
        ui->NonAdminCommandsGameCombo->setCurrentIndex( 0 );
    else
        ui->NonAdminCommandsGameCombo->setCurrentIndex( 1 );
}

void Widget::on_NonAdminCommandsGameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_nonadmincommandsgame" );
    gUNM->m_NonAdminCommandsGame = CFG.GetInt( "bot_nonadmincommandsgame", 1 ) != 0;

    if( gUNM->m_NonAdminCommandsGame )
        ui->NonAdminCommandsGameCombo->setCurrentIndex( 0 );
    else
        ui->NonAdminCommandsGameCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerServerPrintJoinApply_clicked( )
{
    ui->PlayerServerPrintJoinCombo->currentIndex( ) == 0 ? gUNM->m_PlayerServerPrintJoin = true : gUNM->m_PlayerServerPrintJoin = false;
}

void Widget::on_PlayerServerPrintJoinSave_clicked( )
{
    if( ui->PlayerServerPrintJoinCombo->currentIndex( ) == 0 )
    {
        gUNM->m_PlayerServerPrintJoin = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerserverprintjoin", "1" );
    }
    else
    {
        gUNM->m_PlayerServerPrintJoin = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerserverprintjoin", "0" );
    }
}

void Widget::on_PlayerServerPrintJoinRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PlayerServerPrintJoin = CFG.GetInt( "bot_playerserverprintjoin", 0 ) != 0;

    if( gUNM->m_PlayerServerPrintJoin )
        ui->PlayerServerPrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->PlayerServerPrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerServerPrintJoinReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_playerserverprintjoin" );
    gUNM->m_PlayerServerPrintJoin = CFG.GetInt( "bot_playerserverprintjoin", 0 ) != 0;

    if( gUNM->m_PlayerServerPrintJoin )
        ui->PlayerServerPrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->PlayerServerPrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_FunCommandsGameApply_clicked( )
{
    ui->FunCommandsGameCombo->currentIndex( ) == 0 ? gUNM->m_FunCommandsGame = true : gUNM->m_FunCommandsGame = false;
}

void Widget::on_FunCommandsGameSave_clicked( )
{
    if( ui->FunCommandsGameCombo->currentIndex( ) == 0 )
    {
        gUNM->m_FunCommandsGame = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_funcommandsgame", "1" );
    }
    else
    {
        gUNM->m_FunCommandsGame = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_funcommandsgame", "0" );
    }
}

void Widget::on_FunCommandsGameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_FunCommandsGame = CFG.GetInt( "bot_funcommandsgame", 1 ) != 0;

    if( gUNM->m_FunCommandsGame )
        ui->FunCommandsGameCombo->setCurrentIndex( 0 );
    else
        ui->FunCommandsGameCombo->setCurrentIndex( 1 );
}

void Widget::on_FunCommandsGameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_funcommandsgame" );
    gUNM->m_FunCommandsGame = CFG.GetInt( "bot_funcommandsgame", 1 ) != 0;

    if( gUNM->m_FunCommandsGame )
        ui->FunCommandsGameCombo->setCurrentIndex( 0 );
    else
        ui->FunCommandsGameCombo->setCurrentIndex( 1 );
}

void Widget::on_WarningPacketValidationFailedApply_clicked( )
{
    ui->WarningPacketValidationFailedCombo->currentIndex( ) == 0 ? gUNM->m_WarningPacketValidationFailed = true : gUNM->m_WarningPacketValidationFailed = false;
}

void Widget::on_WarningPacketValidationFailedSave_clicked( )
{
    if( ui->WarningPacketValidationFailedCombo->currentIndex( ) == 0 )
    {
        gUNM->m_WarningPacketValidationFailed = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_warningpacketvalidationfailed", "1" );
    }
    else
    {
        gUNM->m_WarningPacketValidationFailed = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_warningpacketvalidationfailed", "0" );
    }
}

void Widget::on_WarningPacketValidationFailedRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_WarningPacketValidationFailed = CFG.GetInt( "bot_warningpacketvalidationfailed", 0 ) != 0;

    if( gUNM->m_WarningPacketValidationFailed )
        ui->WarningPacketValidationFailedCombo->setCurrentIndex( 0 );
    else
        ui->WarningPacketValidationFailedCombo->setCurrentIndex( 1 );
}

void Widget::on_WarningPacketValidationFailedReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_warningpacketvalidationfailed" );
    gUNM->m_WarningPacketValidationFailed = CFG.GetInt( "bot_warningpacketvalidationfailed", 0 ) != 0;

    if( gUNM->m_WarningPacketValidationFailed )
        ui->WarningPacketValidationFailedCombo->setCurrentIndex( 0 );
    else
        ui->WarningPacketValidationFailedCombo->setCurrentIndex( 1 );
}

void Widget::on_ResourceTradePrintApply_clicked( )
{
    ui->ResourceTradePrintCombo->currentIndex( ) == 0 ? gUNM->m_ResourceTradePrint = true : gUNM->m_ResourceTradePrint = false;
}

void Widget::on_ResourceTradePrintSave_clicked( )
{
    if( ui->ResourceTradePrintCombo->currentIndex( ) == 0 )
    {
        gUNM->m_ResourceTradePrint = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_resourcetradeprint", "1" );
    }
    else
    {
        gUNM->m_ResourceTradePrint = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_resourcetradeprint", "0" );
    }
}

void Widget::on_ResourceTradePrintRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ResourceTradePrint = CFG.GetInt( "bot_resourcetradeprint", 0 ) != 0;

    if( gUNM->m_ResourceTradePrint )
        ui->ResourceTradePrintCombo->setCurrentIndex( 0 );
    else
        ui->ResourceTradePrintCombo->setCurrentIndex( 1 );
}

void Widget::on_ResourceTradePrintReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_resourcetradeprint" );
    gUNM->m_ResourceTradePrint = CFG.GetInt( "bot_resourcetradeprint", 0 ) != 0;

    if( gUNM->m_ResourceTradePrint )
        ui->ResourceTradePrintCombo->setCurrentIndex( 0 );
    else
        ui->ResourceTradePrintCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerBeforeStartPrintApply_clicked( )
{
    ui->PlayerBeforeStartPrintCombo->currentIndex( ) == 0 ? gUNM->m_PlayerBeforeStartPrint = true : gUNM->m_PlayerBeforeStartPrint = false;
}

void Widget::on_PlayerBeforeStartPrintSave_clicked( )
{
    if( ui->PlayerBeforeStartPrintCombo->currentIndex( ) == 0 )
    {
        gUNM->m_PlayerBeforeStartPrint = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerbeforestartprint", "1" );
    }
    else
    {
        gUNM->m_PlayerBeforeStartPrint = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerbeforestartprint", "0" );
    }
}

void Widget::on_PlayerBeforeStartPrintRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PlayerBeforeStartPrint = CFG.GetInt( "bot_playerbeforestartprint", 1 ) != 0;

    if( gUNM->m_PlayerBeforeStartPrint )
        ui->PlayerBeforeStartPrintCombo->setCurrentIndex( 0 );
    else
        ui->PlayerBeforeStartPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerBeforeStartPrintReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_playerbeforestartprint" );
    gUNM->m_PlayerBeforeStartPrint = CFG.GetInt( "bot_playerbeforestartprint", 1 ) != 0;

    if( gUNM->m_PlayerBeforeStartPrint )
        ui->PlayerBeforeStartPrintCombo->setCurrentIndex( 0 );
    else
        ui->PlayerBeforeStartPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerBeforeStartPrintJoinApply_clicked( )
{
    ui->PlayerBeforeStartPrintJoinCombo->currentIndex( ) == 0 ? gUNM->m_PlayerBeforeStartPrintJoin = true : gUNM->m_PlayerBeforeStartPrintJoin = false;
}

void Widget::on_PlayerBeforeStartPrintJoinSave_clicked( )
{
    if( ui->PlayerBeforeStartPrintJoinCombo->currentIndex( ) == 0 )
    {
        gUNM->m_PlayerBeforeStartPrintJoin = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerbeforestartprintjoin", "1" );
    }
    else
    {
        gUNM->m_PlayerBeforeStartPrintJoin = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerbeforestartprintjoin", "0" );
    }
}

void Widget::on_PlayerBeforeStartPrintJoinRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PlayerBeforeStartPrintJoin = CFG.GetInt( "bot_playerbeforestartprintjoin", 0 ) != 0;

    if( gUNM->m_PlayerBeforeStartPrintJoin )
        ui->PlayerBeforeStartPrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->PlayerBeforeStartPrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerBeforeStartPrintJoinReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_playerbeforestartprintjoin" );
    gUNM->m_PlayerBeforeStartPrintJoin = CFG.GetInt( "bot_playerbeforestartprintjoin", 0 ) != 0;

    if( gUNM->m_PlayerBeforeStartPrintJoin )
        ui->PlayerBeforeStartPrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->PlayerBeforeStartPrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerBeforeStartPrivatePrintJoinApply_clicked( )
{
    ui->PlayerBeforeStartPrivatePrintJoinCombo->currentIndex( ) == 0 ? gUNM->m_PlayerBeforeStartPrivatePrintJoin = true : gUNM->m_PlayerBeforeStartPrivatePrintJoin = false;
}

void Widget::on_PlayerBeforeStartPrivatePrintJoinSave_clicked( )
{
    if( ui->PlayerBeforeStartPrivatePrintJoinCombo->currentIndex( ) == 0 )
    {
        gUNM->m_PlayerBeforeStartPrivatePrintJoin = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerbeforestartprivateprintjoin", "1" );
    }
    else
    {
        gUNM->m_PlayerBeforeStartPrivatePrintJoin = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerbeforestartprivateprintjoin", "0" );
    }
}

void Widget::on_PlayerBeforeStartPrivatePrintJoinRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PlayerBeforeStartPrivatePrintJoin = CFG.GetInt( "bot_playerbeforestartprivateprintjoin", 1 ) != 0;

    if( gUNM->m_PlayerBeforeStartPrivatePrintJoin )
        ui->PlayerBeforeStartPrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->PlayerBeforeStartPrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerBeforeStartPrivatePrintJoinReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_playerbeforestartprivateprintjoin" );
    gUNM->m_PlayerBeforeStartPrivatePrintJoin = CFG.GetInt( "bot_playerbeforestartprivateprintjoin", 1 ) != 0;

    if( gUNM->m_PlayerBeforeStartPrivatePrintJoin )
        ui->PlayerBeforeStartPrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->PlayerBeforeStartPrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerBeforeStartPrivatePrintJoinSDApply_clicked( )
{
    ui->PlayerBeforeStartPrivatePrintJoinSDCombo->currentIndex( ) == 0 ? gUNM->m_PlayerBeforeStartPrivatePrintJoinSD = true : gUNM->m_PlayerBeforeStartPrivatePrintJoinSD = false;
}

void Widget::on_PlayerBeforeStartPrivatePrintJoinSDSave_clicked( )
{
    if( ui->PlayerBeforeStartPrivatePrintJoinSDCombo->currentIndex( ) == 0 )
    {
        gUNM->m_PlayerBeforeStartPrivatePrintJoinSD = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerbeforestartprivateprintjoinsd", "1" );
    }
    else
    {
        gUNM->m_PlayerBeforeStartPrivatePrintJoinSD = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerbeforestartprivateprintjoinsd", "0" );
    }
}

void Widget::on_PlayerBeforeStartPrivatePrintJoinSDRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PlayerBeforeStartPrivatePrintJoinSD = CFG.GetInt( "bot_playerbeforestartprivateprintjoinsd", 1 ) != 0;

    if( gUNM->m_PlayerBeforeStartPrivatePrintJoinSD )
        ui->PlayerBeforeStartPrivatePrintJoinSDCombo->setCurrentIndex( 0 );
    else
        ui->PlayerBeforeStartPrivatePrintJoinSDCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerBeforeStartPrivatePrintJoinSDReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_playerbeforestartprivateprintjoinsd" );
    gUNM->m_PlayerBeforeStartPrivatePrintJoinSD = CFG.GetInt( "bot_playerbeforestartprivateprintjoinsd", 1 ) != 0;

    if( gUNM->m_PlayerBeforeStartPrivatePrintJoinSD )
        ui->PlayerBeforeStartPrivatePrintJoinSDCombo->setCurrentIndex( 0 );
    else
        ui->PlayerBeforeStartPrivatePrintJoinSDCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerBeforeStartPrintLeaveApply_clicked( )
{
    ui->PlayerBeforeStartPrintLeaveCombo->currentIndex( ) == 0 ? gUNM->m_PlayerBeforeStartPrintLeave = true : gUNM->m_PlayerBeforeStartPrintLeave = false;
}

void Widget::on_PlayerBeforeStartPrintLeaveSave_clicked( )
{
    if( ui->PlayerBeforeStartPrintLeaveCombo->currentIndex( ) == 0 )
    {
        gUNM->m_PlayerBeforeStartPrintLeave = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerbeforestartprintleave", "1" );
    }
    else
    {
        gUNM->m_PlayerBeforeStartPrintLeave = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerbeforestartprintleave", "0" );
    }
}

void Widget::on_PlayerBeforeStartPrintLeaveRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PlayerBeforeStartPrintLeave = CFG.GetInt( "bot_playerbeforestartprintleave", 0 ) != 0;

    if( gUNM->m_PlayerBeforeStartPrintLeave )
        ui->PlayerBeforeStartPrintLeaveCombo->setCurrentIndex( 0 );
    else
        ui->PlayerBeforeStartPrintLeaveCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerBeforeStartPrintLeaveReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_playerbeforestartprintleave" );
    gUNM->m_PlayerBeforeStartPrintLeave = CFG.GetInt( "bot_playerbeforestartprintleave", 0 ) != 0;

    if( gUNM->m_PlayerBeforeStartPrintLeave )
        ui->PlayerBeforeStartPrintLeaveCombo->setCurrentIndex( 0 );
    else
        ui->PlayerBeforeStartPrintLeaveCombo->setCurrentIndex( 1 );
}

void Widget::on_BotChannelPrivatePrintJoinApply_clicked( )
{
    ui->BotChannelPrivatePrintJoinCombo->currentIndex( ) == 0 ? gUNM->m_BotChannelPrivatePrintJoin = true : gUNM->m_BotChannelPrivatePrintJoin = false;
}

void Widget::on_BotChannelPrivatePrintJoinSave_clicked( )
{
    if( ui->BotChannelPrivatePrintJoinCombo->currentIndex( ) == 0 )
    {
        gUNM->m_BotChannelPrivatePrintJoin = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_botchannelprivateprintjoin", "1" );
    }
    else
    {
        gUNM->m_BotChannelPrivatePrintJoin = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_botchannelprivateprintjoin", "0" );
    }
}

void Widget::on_BotChannelPrivatePrintJoinRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_BotChannelPrivatePrintJoin = CFG.GetInt( "bot_botchannelprivateprintjoin", 0 ) != 0;

    if( gUNM->m_BotChannelPrivatePrintJoin )
        ui->BotChannelPrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->BotChannelPrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_BotChannelPrivatePrintJoinReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_botchannelprivateprintjoin" );
    gUNM->m_BotChannelPrivatePrintJoin = CFG.GetInt( "bot_botchannelprivateprintjoin", 0 ) != 0;

    if( gUNM->m_BotChannelPrivatePrintJoin )
        ui->BotChannelPrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->BotChannelPrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_BotNamePrivatePrintJoinApply_clicked( )
{
    ui->BotNamePrivatePrintJoinCombo->currentIndex( ) == 0 ? gUNM->m_BotNamePrivatePrintJoin = true : gUNM->m_BotNamePrivatePrintJoin = false;
}

void Widget::on_BotNamePrivatePrintJoinSave_clicked( )
{
    if( ui->BotNamePrivatePrintJoinCombo->currentIndex( ) == 0 )
    {
        gUNM->m_BotNamePrivatePrintJoin = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_botnameprivateprintjoin", "1" );
    }
    else
    {
        gUNM->m_BotNamePrivatePrintJoin = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_botnameprivateprintjoin", "0" );
    }
}

void Widget::on_BotNamePrivatePrintJoinRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_BotNamePrivatePrintJoin = CFG.GetInt( "bot_botnameprivateprintjoin", 0 ) != 0;

    if( gUNM->m_BotNamePrivatePrintJoin )
        ui->BotNamePrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->BotNamePrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_BotNamePrivatePrintJoinReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_botnameprivateprintjoin" );
    gUNM->m_BotNamePrivatePrintJoin = CFG.GetInt( "bot_botnameprivateprintjoin", 0 ) != 0;

    if( gUNM->m_BotNamePrivatePrintJoin )
        ui->BotNamePrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->BotNamePrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_InfoGamesPrivatePrintJoinApply_clicked( )
{
    ui->InfoGamesPrivatePrintJoinCombo->currentIndex( ) == 0 ? gUNM->m_InfoGamesPrivatePrintJoin = true : gUNM->m_InfoGamesPrivatePrintJoin = false;
}

void Widget::on_InfoGamesPrivatePrintJoinSave_clicked( )
{
    if( ui->InfoGamesPrivatePrintJoinCombo->currentIndex( ) == 0 )
    {
        gUNM->m_InfoGamesPrivatePrintJoin = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_infogamesprivateprintjoin", "1" );
    }
    else
    {
        gUNM->m_InfoGamesPrivatePrintJoin = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_infogamesprivateprintjoin", "0" );
    }
}

void Widget::on_InfoGamesPrivatePrintJoinRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_InfoGamesPrivatePrintJoin = CFG.GetInt( "bot_infogamesprivateprintjoin", 0 ) != 0;

    if( gUNM->m_InfoGamesPrivatePrintJoin )
        ui->InfoGamesPrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->InfoGamesPrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_InfoGamesPrivatePrintJoinReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_infogamesprivateprintjoin" );
    gUNM->m_InfoGamesPrivatePrintJoin = CFG.GetInt( "bot_infogamesprivateprintjoin", 0 ) != 0;

    if( gUNM->m_InfoGamesPrivatePrintJoin )
        ui->InfoGamesPrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->InfoGamesPrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_GameNamePrivatePrintJoinApply_clicked( )
{
    ui->GameNamePrivatePrintJoinCombo->currentIndex( ) == 0 ? gUNM->m_GameNamePrivatePrintJoin = true : gUNM->m_GameNamePrivatePrintJoin = false;
}

void Widget::on_GameNamePrivatePrintJoinSave_clicked( )
{
    if( ui->GameNamePrivatePrintJoinCombo->currentIndex( ) == 0 )
    {
        gUNM->m_GameNamePrivatePrintJoin = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_gamenameprivateprintjoin", "1" );
    }
    else
    {
        gUNM->m_GameNamePrivatePrintJoin = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_gamenameprivateprintjoin", "0" );
    }
}

void Widget::on_GameNamePrivatePrintJoinRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_GameNamePrivatePrintJoin = CFG.GetInt( "bot_gamenameprivateprintjoin", 0 ) != 0;

    if( gUNM->m_GameNamePrivatePrintJoin )
        ui->GameNamePrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->GameNamePrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_GameNamePrivatePrintJoinReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gamenameprivateprintjoin" );
    gUNM->m_GameNamePrivatePrintJoin = CFG.GetInt( "bot_gamenameprivateprintjoin", 0 ) != 0;

    if( gUNM->m_GameNamePrivatePrintJoin )
        ui->GameNamePrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->GameNamePrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_GameModePrivatePrintJoinApply_clicked( )
{
    ui->GameModePrivatePrintJoinCombo->currentIndex( ) == 0 ? gUNM->m_GameModePrivatePrintJoin = true : gUNM->m_GameModePrivatePrintJoin = false;
}

void Widget::on_GameModePrivatePrintJoinSave_clicked( )
{
    if( ui->GameModePrivatePrintJoinCombo->currentIndex( ) == 0 )
    {
        gUNM->m_GameModePrivatePrintJoin = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_gamemodeprivateprintjoin", "1" );
    }
    else
    {
        gUNM->m_GameModePrivatePrintJoin = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_gamemodeprivateprintjoin", "0" );
    }
}

void Widget::on_GameModePrivatePrintJoinRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_GameModePrivatePrintJoin = CFG.GetInt( "bot_gamemodeprivateprintjoin", 1 ) != 0;

    if( gUNM->m_GameModePrivatePrintJoin )
        ui->GameModePrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->GameModePrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_GameModePrivatePrintJoinReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gamemodeprivateprintjoin" );
    gUNM->m_GameModePrivatePrintJoin = CFG.GetInt( "bot_gamemodeprivateprintjoin", 1 ) != 0;

    if( gUNM->m_GameModePrivatePrintJoin )
        ui->GameModePrivatePrintJoinCombo->setCurrentIndex( 0 );
    else
        ui->GameModePrivatePrintJoinCombo->setCurrentIndex( 1 );
}

void Widget::on_RehostPrintingApply_clicked( )
{
    ui->RehostPrintingCombo->currentIndex( ) == 0 ? gUNM->m_RehostPrinting = true : gUNM->m_RehostPrinting = false;
}

void Widget::on_RehostPrintingSave_clicked( )
{
    if( ui->RehostPrintingCombo->currentIndex( ) == 0 )
    {
        gUNM->m_RehostPrinting = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_rehostprinting", "1" );
    }
    else
    {
        gUNM->m_RehostPrinting = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_rehostprinting", "0" );
    }
}

void Widget::on_RehostPrintingRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_RehostPrinting = CFG.GetInt( "bot_rehostprinting", 0 ) != 0;

    if( gUNM->m_RehostPrinting )
        ui->RehostPrintingCombo->setCurrentIndex( 0 );
    else
        ui->RehostPrintingCombo->setCurrentIndex( 1 );
}

void Widget::on_RehostPrintingReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_rehostprinting" );
    gUNM->m_RehostPrinting = CFG.GetInt( "bot_rehostprinting", 0 ) != 0;

    if( gUNM->m_RehostPrinting )
        ui->RehostPrintingCombo->setCurrentIndex( 0 );
    else
        ui->RehostPrintingCombo->setCurrentIndex( 1 );
}

void Widget::on_RehostFailedPrintingApply_clicked( )
{
    ui->RehostFailedPrintingCombo->currentIndex( ) == 0 ? gUNM->m_RehostFailedPrinting = true : gUNM->m_RehostFailedPrinting = false;
}

void Widget::on_RehostFailedPrintingSave_clicked( )
{
    if( ui->RehostFailedPrintingCombo->currentIndex( ) == 0 )
    {
        gUNM->m_RehostFailedPrinting = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_rehostfailedprinting", "1" );
    }
    else
    {
        gUNM->m_RehostFailedPrinting = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_rehostfailedprinting", "0" );
    }
}

void Widget::on_RehostFailedPrintingRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_RehostFailedPrinting = CFG.GetInt( "bot_rehostfailedprinting", 0 ) != 0;

    if( gUNM->m_RehostFailedPrinting )
        ui->RehostFailedPrintingCombo->setCurrentIndex( 0 );
    else
        ui->RehostFailedPrintingCombo->setCurrentIndex( 1 );
}

void Widget::on_RehostFailedPrintingReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_rehostfailedprinting" );
    gUNM->m_RehostFailedPrinting = CFG.GetInt( "bot_rehostfailedprinting", 0 ) != 0;

    if( gUNM->m_RehostFailedPrinting )
        ui->RehostFailedPrintingCombo->setCurrentIndex( 0 );
    else
        ui->RehostFailedPrintingCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerFinishedLoadingPrintingApply_clicked( )
{
    ui->PlayerFinishedLoadingPrintingCombo->currentIndex( ) == 0 ? gUNM->m_PlayerFinishedLoadingPrinting = true : gUNM->m_PlayerFinishedLoadingPrinting = false;
}

void Widget::on_PlayerFinishedLoadingPrintingSave_clicked( )
{
    if( ui->PlayerFinishedLoadingPrintingCombo->currentIndex( ) == 0 )
    {
        gUNM->m_PlayerFinishedLoadingPrinting = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerfinishedloadingprinting", "1" );
    }
    else
    {
        gUNM->m_PlayerFinishedLoadingPrinting = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_playerfinishedloadingprinting", "0" );
    }
}

void Widget::on_PlayerFinishedLoadingPrintingRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PlayerFinishedLoadingPrinting = CFG.GetInt( "bot_playerfinishedloadingprinting", 0 ) != 0;

    if( gUNM->m_PlayerFinishedLoadingPrinting )
        ui->PlayerFinishedLoadingPrintingCombo->setCurrentIndex( 0 );
    else
        ui->PlayerFinishedLoadingPrintingCombo->setCurrentIndex( 1 );
}

void Widget::on_PlayerFinishedLoadingPrintingReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_playerfinishedloadingprinting" );
    gUNM->m_PlayerFinishedLoadingPrinting = CFG.GetInt( "bot_playerfinishedloadingprinting", 0 ) != 0;

    if( gUNM->m_PlayerFinishedLoadingPrinting )
        ui->PlayerFinishedLoadingPrintingCombo->setCurrentIndex( 0 );
    else
        ui->PlayerFinishedLoadingPrintingCombo->setCurrentIndex( 1 );
}

void Widget::on_LobbyAnnounceUnoccupiedApply_clicked( )
{
    ui->LobbyAnnounceUnoccupiedCombo->currentIndex( ) == 0 ? gUNM->m_LobbyAnnounceUnoccupied = true : gUNM->m_LobbyAnnounceUnoccupied = false;
}

void Widget::on_LobbyAnnounceUnoccupiedSave_clicked( )
{
    if( ui->LobbyAnnounceUnoccupiedCombo->currentIndex( ) == 0 )
    {
        gUNM->m_LobbyAnnounceUnoccupied = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_lobbyannounceunoccupied", "1" );
    }
    else
    {
        gUNM->m_LobbyAnnounceUnoccupied = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_lobbyannounceunoccupied", "0" );
    }
}

void Widget::on_LobbyAnnounceUnoccupiedRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_LobbyAnnounceUnoccupied = CFG.GetInt( "bot_lobbyannounceunoccupied", 1 ) != 0;

    if( gUNM->m_LobbyAnnounceUnoccupied )
        ui->LobbyAnnounceUnoccupiedCombo->setCurrentIndex( 0 );
    else
        ui->LobbyAnnounceUnoccupiedCombo->setCurrentIndex( 1 );
}

void Widget::on_LobbyAnnounceUnoccupiedReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_lobbyannounceunoccupied" );
    gUNM->m_LobbyAnnounceUnoccupied = CFG.GetInt( "bot_lobbyannounceunoccupied", 1 ) != 0;

    if( gUNM->m_LobbyAnnounceUnoccupied )
        ui->LobbyAnnounceUnoccupiedCombo->setCurrentIndex( 0 );
    else
        ui->LobbyAnnounceUnoccupiedCombo->setCurrentIndex( 1 );
}

void Widget::on_RelayChatCommandsApply_clicked( )
{
    ui->RelayChatCommandsCombo->currentIndex( ) == 0 ? gUNM->m_RelayChatCommands = true : gUNM->m_RelayChatCommands = false;
}

void Widget::on_RelayChatCommandsSave_clicked( )
{
    if( ui->RelayChatCommandsCombo->currentIndex( ) == 0 )
    {
        gUNM->m_RelayChatCommands = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_relaychatcommands", "1" );
    }
    else
    {
        gUNM->m_RelayChatCommands = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_relaychatcommands", "0" );
    }
}

void Widget::on_RelayChatCommandsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_RelayChatCommands = CFG.GetInt( "bot_relaychatcommands", 1 ) != 0;

    if( gUNM->m_RelayChatCommands )
        ui->RelayChatCommandsCombo->setCurrentIndex( 0 );
    else
        ui->RelayChatCommandsCombo->setCurrentIndex( 1 );
}

void Widget::on_RelayChatCommandsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_relaychatcommands" );
    gUNM->m_RelayChatCommands = CFG.GetInt( "bot_relaychatcommands", 1 ) != 0;

    if( gUNM->m_RelayChatCommands )
        ui->RelayChatCommandsCombo->setCurrentIndex( 0 );
    else
        ui->RelayChatCommandsCombo->setCurrentIndex( 1 );
}

void Widget::on_DetourAllMessagesToAdminsApply_clicked( )
{
    ui->DetourAllMessagesToAdminsCombo->currentIndex( ) == 0 ? gUNM->m_DetourAllMessagesToAdmins = true : gUNM->m_DetourAllMessagesToAdmins = false;
}

void Widget::on_DetourAllMessagesToAdminsSave_clicked( )
{
    if( ui->DetourAllMessagesToAdminsCombo->currentIndex( ) == 0 )
    {
        gUNM->m_DetourAllMessagesToAdmins = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_detourallmessagestoadmins", "1" );
    }
    else
    {
        gUNM->m_DetourAllMessagesToAdmins = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_detourallmessagestoadmins", "0" );
    }
}

void Widget::on_DetourAllMessagesToAdminsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DetourAllMessagesToAdmins = CFG.GetInt( "bot_detourallmessagestoadmins", 0 ) != 0;

    if( gUNM->m_DetourAllMessagesToAdmins )
        ui->DetourAllMessagesToAdminsCombo->setCurrentIndex( 0 );
    else
        ui->DetourAllMessagesToAdminsCombo->setCurrentIndex( 1 );
}

void Widget::on_DetourAllMessagesToAdminsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_detourallmessagestoadmins" );
    gUNM->m_DetourAllMessagesToAdmins = CFG.GetInt( "bot_detourallmessagestoadmins", 0 ) != 0;

    if( gUNM->m_DetourAllMessagesToAdmins )
        ui->DetourAllMessagesToAdminsCombo->setCurrentIndex( 0 );
    else
        ui->DetourAllMessagesToAdminsCombo->setCurrentIndex( 1 );
}

void Widget::on_AdminMessagesApply_clicked( )
{
    ui->AdminMessagesCombo->currentIndex( ) == 0 ? gUNM->m_AdminMessages = true : gUNM->m_AdminMessages = false;
}

void Widget::on_AdminMessagesSave_clicked( )
{
    if( ui->AdminMessagesCombo->currentIndex( ) == 0 )
    {
        gUNM->m_AdminMessages = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_adminmessages", "1" );
    }
    else
    {
        gUNM->m_AdminMessages = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_adminmessages", "0" );
    }
}

void Widget::on_AdminMessagesRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AdminMessages = CFG.GetInt( "bot_adminmessages", 0 ) != 0;

    if( gUNM->m_AdminMessages )
        ui->AdminMessagesCombo->setCurrentIndex( 0 );
    else
        ui->AdminMessagesCombo->setCurrentIndex( 1 );
}

void Widget::on_AdminMessagesReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_adminmessages" );
    gUNM->m_AdminMessages = CFG.GetInt( "bot_adminmessages", 0 ) != 0;

    if( gUNM->m_AdminMessages )
        ui->AdminMessagesCombo->setCurrentIndex( 0 );
    else
        ui->AdminMessagesCombo->setCurrentIndex( 1 );
}

void Widget::on_LocalAdminMessagesApply_clicked( )
{
    ui->LocalAdminMessagesCombo->currentIndex( ) == 0 ? gUNM->m_LocalAdminMessages = true : gUNM->m_LocalAdminMessages = false;
}

void Widget::on_LocalAdminMessagesSave_clicked( )
{
    if( ui->LocalAdminMessagesCombo->currentIndex( ) == 0 )
    {
        gUNM->m_LocalAdminMessages = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_localadminmessages", "1" );
    }
    else
    {
        gUNM->m_LocalAdminMessages = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_localadminmessages", "0" );
    }
}

void Widget::on_LocalAdminMessagesRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_LocalAdminMessages = CFG.GetInt( "bot_localadminmessages", 0 ) != 0;

    if( gUNM->m_LocalAdminMessages )
        ui->LocalAdminMessagesCombo->setCurrentIndex( 0 );
    else
        ui->LocalAdminMessagesCombo->setCurrentIndex( 1 );
}

void Widget::on_LocalAdminMessagesReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_localadminmessages" );
    gUNM->m_LocalAdminMessages = CFG.GetInt( "bot_localadminmessages", 0 ) != 0;

    if( gUNM->m_LocalAdminMessages )
        ui->LocalAdminMessagesCombo->setCurrentIndex( 0 );
    else
        ui->LocalAdminMessagesCombo->setCurrentIndex( 1 );
}

void Widget::on_WarnLatencyApply_clicked( )
{
    ui->WarnLatencyCombo->currentIndex( ) == 0 ? gUNM->m_WarnLatency = true : gUNM->m_WarnLatency = false;
}

void Widget::on_WarnLatencySave_clicked( )
{
    if( ui->WarnLatencyCombo->currentIndex( ) == 0 )
    {
        gUNM->m_WarnLatency = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_warnLatency", "1" );
    }
    else
    {
        gUNM->m_WarnLatency = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_warnLatency", "0" );
    }
}

void Widget::on_WarnLatencyRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_WarnLatency = CFG.GetInt( "bot_warnLatency", 0 ) != 0;

    if( gUNM->m_WarnLatency )
        ui->WarnLatencyCombo->setCurrentIndex( 0 );
    else
        ui->WarnLatencyCombo->setCurrentIndex( 1 );
}

void Widget::on_WarnLatencyReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_warnLatency" );
    gUNM->m_WarnLatency = CFG.GetInt( "bot_warnLatency", 0 ) != 0;

    if( gUNM->m_WarnLatency )
        ui->WarnLatencyCombo->setCurrentIndex( 0 );
    else
        ui->WarnLatencyCombo->setCurrentIndex( 1 );
}

void Widget::on_RefreshMessagesApply_clicked( )
{
    ui->RefreshMessagesCombo->currentIndex( ) == 0 ? gUNM->m_RefreshMessages = true : gUNM->m_RefreshMessages = false;
}

void Widget::on_RefreshMessagesSave_clicked( )
{
    if( ui->RefreshMessagesCombo->currentIndex( ) == 0 )
    {
        gUNM->m_RefreshMessages = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_refreshmessages", "1" );
    }
    else
    {
        gUNM->m_RefreshMessages = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_refreshmessages", "0" );
    }
}

void Widget::on_RefreshMessagesRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_RefreshMessages = CFG.GetInt( "bot_refreshmessages", 0 ) != 0;

    if( gUNM->m_RefreshMessages )
        ui->RefreshMessagesCombo->setCurrentIndex( 0 );
    else
        ui->RefreshMessagesCombo->setCurrentIndex( 1 );
}

void Widget::on_RefreshMessagesReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_refreshmessages" );
    gUNM->m_RefreshMessages = CFG.GetInt( "bot_refreshmessages", 0 ) != 0;

    if( gUNM->m_RefreshMessages )
        ui->RefreshMessagesCombo->setCurrentIndex( 0 );
    else
        ui->RefreshMessagesCombo->setCurrentIndex( 1 );
}

void Widget::on_GameAnnounceApply_clicked( )
{
    gUNM->m_GameAnnounce = ui->GameAnnounceCombo->currentIndex( );
}

void Widget::on_GameAnnounceSave_clicked( )
{
    gUNM->m_GameAnnounce = ui->GameAnnounceCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_gameannounce", UTIL_ToString( gUNM->m_GameAnnounce ) );
}

void Widget::on_GameAnnounceRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_GameAnnounce = CFG.GetUInt( "bot_gameannounce", 0, 3 );
    ui->GameAnnounceCombo->setCurrentIndex( gUNM->m_GameAnnounce );
}

void Widget::on_GameAnnounceReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gameannounce" );
    gUNM->m_GameAnnounce = CFG.GetUInt( "bot_gameannounce", 0, 3 );
    ui->GameAnnounceCombo->setCurrentIndex( gUNM->m_GameAnnounce );
}

void Widget::on_ShowDownloadsInfoApply_clicked( )
{
    ui->ShowDownloadsInfoCombo->currentIndex( ) == 0 ? gUNM->m_ShowDownloadsInfo = true : gUNM->m_ShowDownloadsInfo = false;
}

void Widget::on_ShowDownloadsInfoSave_clicked( )
{
    if( ui->ShowDownloadsInfoCombo->currentIndex( ) == 0 )
    {
        gUNM->m_ShowDownloadsInfo = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_showdownloadsinfo", "1" );
    }
    else
    {
        gUNM->m_ShowDownloadsInfo = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_showdownloadsinfo", "0" );
    }
}

void Widget::on_ShowDownloadsInfoRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ShowDownloadsInfo = CFG.GetInt( "bot_showdownloadsinfo", 0 ) != 0;

    if( gUNM->m_ShowDownloadsInfo )
        ui->ShowDownloadsInfoCombo->setCurrentIndex( 0 );
    else
        ui->ShowDownloadsInfoCombo->setCurrentIndex( 1 );
}

void Widget::on_ShowDownloadsInfoReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_showdownloadsinfo" );
    gUNM->m_ShowDownloadsInfo = CFG.GetInt( "bot_showdownloadsinfo", 0 ) != 0;

    if( gUNM->m_ShowDownloadsInfo )
        ui->ShowDownloadsInfoCombo->setCurrentIndex( 0 );
    else
        ui->ShowDownloadsInfoCombo->setCurrentIndex( 1 );
}

void Widget::on_ShowFinishDownloadingInfoApply_clicked( )
{
    ui->ShowFinishDownloadingInfoCombo->currentIndex( ) == 0 ? gUNM->m_ShowFinishDownloadingInfo = true : gUNM->m_ShowFinishDownloadingInfo = false;
}

void Widget::on_ShowFinishDownloadingInfoSave_clicked( )
{
    if( ui->ShowFinishDownloadingInfoCombo->currentIndex( ) == 0 )
    {
        gUNM->m_ShowFinishDownloadingInfo = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_showfinishdownloadinginfo", "1" );
    }
    else
    {
        gUNM->m_ShowFinishDownloadingInfo = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_showfinishdownloadinginfo", "0" );
    }
}

void Widget::on_ShowFinishDownloadingInfoRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ShowFinishDownloadingInfo = CFG.GetInt( "bot_showfinishdownloadinginfo", 0 ) != 0;

    if( gUNM->m_ShowFinishDownloadingInfo )
        ui->ShowFinishDownloadingInfoCombo->setCurrentIndex( 0 );
    else
        ui->ShowFinishDownloadingInfoCombo->setCurrentIndex( 1 );
}

void Widget::on_ShowFinishDownloadingInfoReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_showfinishdownloadinginfo" );
    gUNM->m_ShowFinishDownloadingInfo = CFG.GetInt( "bot_showfinishdownloadinginfo", 0 ) != 0;

    if( gUNM->m_ShowFinishDownloadingInfo )
        ui->ShowFinishDownloadingInfoCombo->setCurrentIndex( 0 );
    else
        ui->ShowFinishDownloadingInfoCombo->setCurrentIndex( 1 );
}

void Widget::on_ShowDownloadsInfoTimeApply_clicked( )
{
    gUNM->m_ShowDownloadsInfoTime = ui->ShowDownloadsInfoTimeSpin->value( );
}

void Widget::on_ShowDownloadsInfoTimeSave_clicked( )
{
    gUNM->m_ShowDownloadsInfoTime = ui->ShowDownloadsInfoTimeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_showdownloadsinfotime", UTIL_ToString( gUNM->m_ShowDownloadsInfoTime ) );
}

void Widget::on_ShowDownloadsInfoTimeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ShowDownloadsInfoTime = CFG.GetUInt( "bot_showdownloadsinfotime", 15 );
    ui->ShowDownloadsInfoTimeSpin->setValue( gUNM->m_ShowDownloadsInfoTime );
}

void Widget::on_ShowDownloadsInfoTimeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_showdownloadsinfotime" );
    gUNM->m_ShowDownloadsInfoTime = CFG.GetUInt( "bot_showdownloadsinfotime", 15 );
    ui->ShowDownloadsInfoTimeSpin->setValue( gUNM->m_ShowDownloadsInfoTime );
}

void Widget::on_autoinsultlobbyApply_clicked( )
{
    ui->autoinsultlobbyCombo->currentIndex( ) == 0 ? gUNM->m_autoinsultlobby = true : gUNM->m_autoinsultlobby = false;
}

void Widget::on_autoinsultlobbySave_clicked( )
{
    if( ui->autoinsultlobbyCombo->currentIndex( ) == 0 )
    {
        gUNM->m_autoinsultlobby = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_autoinsultlobby", "1" );
    }
    else
    {
        gUNM->m_autoinsultlobby = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_autoinsultlobby", "0" );
    }
}

void Widget::on_autoinsultlobbyRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_autoinsultlobby = CFG.GetInt( "bot_autoinsultlobby", 0 ) != 0;

    if( gUNM->m_autoinsultlobby )
        ui->autoinsultlobbyCombo->setCurrentIndex( 0 );
    else
        ui->autoinsultlobbyCombo->setCurrentIndex( 1 );
}

void Widget::on_autoinsultlobbyReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_autoinsultlobby" );
    gUNM->m_autoinsultlobby = CFG.GetInt( "bot_autoinsultlobby", 0 ) != 0;

    if( gUNM->m_autoinsultlobby )
        ui->autoinsultlobbyCombo->setCurrentIndex( 0 );
    else
        ui->autoinsultlobbyCombo->setCurrentIndex( 1 );
}

void Widget::on_autoinsultlobbysafeadminsApply_clicked( )
{
    ui->autoinsultlobbysafeadminsCombo->currentIndex( ) == 0 ? gUNM->m_autoinsultlobbysafeadmins = true : gUNM->m_autoinsultlobbysafeadmins = false;
}

void Widget::on_autoinsultlobbysafeadminsSave_clicked( )
{
    if( ui->autoinsultlobbysafeadminsCombo->currentIndex( ) == 0 )
    {
        gUNM->m_autoinsultlobbysafeadmins = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_autoinsultlobbysafeadmins", "1" );
    }
    else
    {
        gUNM->m_autoinsultlobbysafeadmins = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_autoinsultlobbysafeadmins", "0" );
    }
}

void Widget::on_autoinsultlobbysafeadminsRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_autoinsultlobbysafeadmins = CFG.GetInt( "bot_autoinsultlobbysafeadmins", 0 ) != 0;

    if( gUNM->m_autoinsultlobbysafeadmins )
        ui->autoinsultlobbysafeadminsCombo->setCurrentIndex( 0 );
    else
        ui->autoinsultlobbysafeadminsCombo->setCurrentIndex( 1 );
}

void Widget::on_autoinsultlobbysafeadminsReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_autoinsultlobbysafeadmins" );
    gUNM->m_autoinsultlobbysafeadmins = CFG.GetInt( "bot_autoinsultlobbysafeadmins", 0 ) != 0;

    if( gUNM->m_autoinsultlobbysafeadmins )
        ui->autoinsultlobbysafeadminsCombo->setCurrentIndex( 0 );
    else
        ui->autoinsultlobbysafeadminsCombo->setCurrentIndex( 1 );
}

void Widget::on_GameOwnerPrivatePrintJoinApply_clicked( )
{
    gUNM->m_GameOwnerPrivatePrintJoin = ui->GameOwnerPrivatePrintJoinCombo->currentIndex( );
}

void Widget::on_GameOwnerPrivatePrintJoinSave_clicked( )
{
    gUNM->m_GameOwnerPrivatePrintJoin = ui->GameOwnerPrivatePrintJoinCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_gameownerprivateprintjoin", UTIL_ToString( gUNM->m_GameOwnerPrivatePrintJoin ) );
}

void Widget::on_GameOwnerPrivatePrintJoinRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_GameOwnerPrivatePrintJoin = CFG.GetUInt( "bot_gameownerprivateprintjoin", 1, 2 );
    ui->GameOwnerPrivatePrintJoinCombo->setCurrentIndex( gUNM->m_GameOwnerPrivatePrintJoin );
}

void Widget::on_GameOwnerPrivatePrintJoinReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gameownerprivateprintjoin" );
    gUNM->m_GameOwnerPrivatePrintJoin = CFG.GetUInt( "bot_gameownerprivateprintjoin", 1, 2 );
    ui->GameOwnerPrivatePrintJoinCombo->setCurrentIndex( gUNM->m_GameOwnerPrivatePrintJoin );
}

void Widget::on_AutoUnMutePrintingApply_clicked( )
{
    gUNM->m_AutoUnMutePrinting = ui->AutoUnMutePrintingCombo->currentIndex( );
}

void Widget::on_AutoUnMutePrintingSave_clicked( )
{
    gUNM->m_AutoUnMutePrinting = ui->AutoUnMutePrintingCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_autounmuteprinting", UTIL_ToString( gUNM->m_AutoUnMutePrinting ) );
}

void Widget::on_AutoUnMutePrintingRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_AutoUnMutePrinting = CFG.GetUInt( "bot_autounmuteprinting", 2, 3 );
    ui->AutoUnMutePrintingCombo->setCurrentIndex( gUNM->m_AutoUnMutePrinting );
}

void Widget::on_AutoUnMutePrintingReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_autounmuteprinting" );
    gUNM->m_AutoUnMutePrinting = CFG.GetUInt( "bot_autounmuteprinting", 2, 3 );
    ui->AutoUnMutePrintingCombo->setCurrentIndex( gUNM->m_AutoUnMutePrinting );
}

void Widget::on_ManualUnMutePrintingApply_clicked( )
{
    gUNM->m_ManualUnMutePrinting = ui->ManualUnMutePrintingCombo->currentIndex( );
}

void Widget::on_ManualUnMutePrintingSave_clicked( )
{
    gUNM->m_ManualUnMutePrinting = ui->ManualUnMutePrintingCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_manualunmuteprinting", UTIL_ToString( gUNM->m_ManualUnMutePrinting ) );
}

void Widget::on_ManualUnMutePrintingRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ManualUnMutePrinting = CFG.GetUInt( "bot_manualunmuteprinting", 1, 3 );
    ui->ManualUnMutePrintingCombo->setCurrentIndex( gUNM->m_ManualUnMutePrinting );
}

void Widget::on_ManualUnMutePrintingReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_manualunmuteprinting" );
    gUNM->m_ManualUnMutePrinting = CFG.GetUInt( "bot_manualunmuteprinting", 1, 3 );
    ui->ManualUnMutePrintingCombo->setCurrentIndex( gUNM->m_ManualUnMutePrinting );
}

void Widget::on_GameIsOverMessagePrintingApply_clicked( )
{
    gUNM->m_GameIsOverMessagePrinting = ui->GameIsOverMessagePrintingCombo->currentIndex( );
}

void Widget::on_GameIsOverMessagePrintingSave_clicked( )
{
    gUNM->m_GameIsOverMessagePrinting = ui->GameIsOverMessagePrintingCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_gameisovermessageprinting", UTIL_ToString( gUNM->m_GameIsOverMessagePrinting ) );
}

void Widget::on_GameIsOverMessagePrintingRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_GameIsOverMessagePrinting = CFG.GetUInt( "bot_gameisovermessageprinting", 1, 3 );
    ui->GameIsOverMessagePrintingCombo->setCurrentIndex( gUNM->m_GameIsOverMessagePrinting );
}

void Widget::on_GameIsOverMessagePrintingReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gameisovermessageprinting" );
    gUNM->m_GameIsOverMessagePrinting = CFG.GetUInt( "bot_gameisovermessageprinting", 1, 3 );
    ui->GameIsOverMessagePrintingCombo->setCurrentIndex( gUNM->m_GameIsOverMessagePrinting );
}

void Widget::on_manualhclmessageApply_clicked( )
{
    gUNM->m_manualhclmessage = ui->manualhclmessageCombo->currentIndex( );
}

void Widget::on_manualhclmessageSave_clicked( )
{
    gUNM->m_manualhclmessage = ui->manualhclmessageCombo->currentIndex( );
    UTIL_SetVarInFile( "unm.cfg", "bot_manualhclmessage", UTIL_ToString( gUNM->m_manualhclmessage ) );
}

void Widget::on_manualhclmessageRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_manualhclmessage = CFG.GetUInt( "bot_manualhclmessage", 0, 2 );
    ui->manualhclmessageCombo->setCurrentIndex( gUNM->m_manualhclmessage );
}

void Widget::on_manualhclmessageReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_manualhclmessage" );
    gUNM->m_manualhclmessage = CFG.GetUInt( "bot_manualhclmessage", 0, 2 );
    ui->manualhclmessageCombo->setCurrentIndex( gUNM->m_manualhclmessage );
}

void Widget::on_GameAnnounceIntervalApply_clicked( )
{
    gUNM->m_GameAnnounceInterval = ui->GameAnnounceIntervalSpin->value( );
}

void Widget::on_GameAnnounceIntervalSave_clicked( )
{
    gUNM->m_GameAnnounceInterval = ui->GameAnnounceIntervalSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_gameannounceinterval", UTIL_ToString( gUNM->m_GameAnnounceInterval ) );
}

void Widget::on_GameAnnounceIntervalRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_GameAnnounceInterval = CFG.GetUInt( "bot_gameannounceinterval", 60 );
    ui->GameAnnounceIntervalSpin->setValue( gUNM->m_GameAnnounceInterval );
}

void Widget::on_GameAnnounceIntervalReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gameannounceinterval" );
    gUNM->m_GameAnnounceInterval = CFG.GetUInt( "bot_gameannounceinterval", 60 );
    ui->GameAnnounceIntervalSpin->setValue( gUNM->m_GameAnnounceInterval );
}

void Widget::on_NoMapPrintDelayApply_clicked( )
{
    gUNM->m_NoMapPrintDelay = ui->NoMapPrintDelaySpin->value( );
}

void Widget::on_NoMapPrintDelaySave_clicked( )
{
    gUNM->m_NoMapPrintDelay = ui->NoMapPrintDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_nomapprintdelay", UTIL_ToString( gUNM->m_NoMapPrintDelay ) );
}

void Widget::on_NoMapPrintDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NoMapPrintDelay = CFG.GetUInt( "bot_nomapprintdelay", 120 );
    ui->NoMapPrintDelaySpin->setValue( gUNM->m_NoMapPrintDelay );
}

void Widget::on_NoMapPrintDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_nomapprintdelay" );
    gUNM->m_NoMapPrintDelay = CFG.GetUInt( "bot_nomapprintdelay", 120 );
    ui->NoMapPrintDelaySpin->setValue( gUNM->m_NoMapPrintDelay );
}

void Widget::on_GameLoadedPrintoutApply_clicked( )
{
    gUNM->m_GameLoadedPrintout = ui->GameLoadedPrintoutSpin->value( );
}

void Widget::on_GameLoadedPrintoutSave_clicked( )
{
    gUNM->m_GameLoadedPrintout = ui->GameLoadedPrintoutSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_gameloadedprintout", UTIL_ToString( gUNM->m_GameLoadedPrintout ) );
}

void Widget::on_GameLoadedPrintoutRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_GameLoadedPrintout = CFG.GetUInt( "bot_gameloadedprintout", 10800 );
    ui->GameLoadedPrintoutSpin->setValue( gUNM->m_GameLoadedPrintout );
}

void Widget::on_GameLoadedPrintoutReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gameloadedprintout" );
    gUNM->m_GameLoadedPrintout = CFG.GetUInt( "bot_gameloadedprintout", 10800 );
    ui->GameLoadedPrintoutSpin->setValue( gUNM->m_GameLoadedPrintout );
}

void Widget::on_StillDownloadingPrintApply_clicked( )
{
    ui->StillDownloadingPrintCombo->currentIndex( ) == 0 ? gUNM->m_StillDownloadingPrint = true : gUNM->m_StillDownloadingPrint = false;
}

void Widget::on_StillDownloadingPrintSave_clicked( )
{
    if( ui->StillDownloadingPrintCombo->currentIndex( ) == 0 )
    {
        gUNM->m_StillDownloadingPrint = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_stilldownloadingprint", "1" );
    }
    else
    {
        gUNM->m_StillDownloadingPrint = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_stilldownloadingprint", "0" );
    }
}

void Widget::on_StillDownloadingPrintRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_StillDownloadingPrint = CFG.GetInt( "bot_stilldownloadingprint", 1 ) != 0;

    if( gUNM->m_StillDownloadingPrint )
        ui->StillDownloadingPrintCombo->setCurrentIndex( 0 );
    else
        ui->StillDownloadingPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_StillDownloadingPrintReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_stilldownloadingprint" );
    gUNM->m_StillDownloadingPrint = CFG.GetInt( "bot_stilldownloadingprint", 1 ) != 0;

    if( gUNM->m_StillDownloadingPrint )
        ui->StillDownloadingPrintCombo->setCurrentIndex( 0 );
    else
        ui->StillDownloadingPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_NotSpoofCheckedPrintApply_clicked( )
{
    ui->NotSpoofCheckedPrintCombo->currentIndex( ) == 0 ? gUNM->m_NotSpoofCheckedPrint = true : gUNM->m_NotSpoofCheckedPrint = false;
}

void Widget::on_NotSpoofCheckedPrintSave_clicked( )
{
    if( ui->NotSpoofCheckedPrintCombo->currentIndex( ) == 0 )
    {
        gUNM->m_NotSpoofCheckedPrint = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_notspoofcheckedprint", "1" );
    }
    else
    {
        gUNM->m_NotSpoofCheckedPrint = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_notspoofcheckedprint", "0" );
    }
}

void Widget::on_NotSpoofCheckedPrintRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NotSpoofCheckedPrint = CFG.GetInt( "bot_notspoofcheckedprint", 1 ) != 0;

    if( gUNM->m_NotSpoofCheckedPrint )
        ui->NotSpoofCheckedPrintCombo->setCurrentIndex( 0 );
    else
        ui->NotSpoofCheckedPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_NotSpoofCheckedPrintReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_notspoofcheckedprint" );
    gUNM->m_NotSpoofCheckedPrint = CFG.GetInt( "bot_notspoofcheckedprint", 1 ) != 0;

    if( gUNM->m_NotSpoofCheckedPrint )
        ui->NotSpoofCheckedPrintCombo->setCurrentIndex( 0 );
    else
        ui->NotSpoofCheckedPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_NotPingedPrintApply_clicked( )
{
    ui->NotPingedPrintCombo->currentIndex( ) == 0 ? gUNM->m_NotPingedPrint = true : gUNM->m_NotPingedPrint = false;
}

void Widget::on_NotPingedPrintSave_clicked( )
{
    if( ui->NotPingedPrintCombo->currentIndex( ) == 0 )
    {
        gUNM->m_NotPingedPrint = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_notpingedprint", "1" );
    }
    else
    {
        gUNM->m_NotPingedPrint = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_notpingedprint", "0" );
    }
}

void Widget::on_NotPingedPrintRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NotPingedPrint = CFG.GetInt( "bot_notpingedprint", 1 ) != 0;

    if( gUNM->m_NotPingedPrint )
        ui->NotPingedPrintCombo->setCurrentIndex( 0 );
    else
        ui->NotPingedPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_NotPingedPrintReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_notpingedprint" );
    gUNM->m_NotPingedPrint = CFG.GetInt( "bot_notpingedprint", 1 ) != 0;

    if( gUNM->m_NotPingedPrint )
        ui->NotPingedPrintCombo->setCurrentIndex( 0 );
    else
        ui->NotPingedPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_StillDownloadingASPrintApply_clicked( )
{
    ui->StillDownloadingASPrintCombo->currentIndex( ) == 0 ? gUNM->m_StillDownloadingASPrint = true : gUNM->m_StillDownloadingASPrint = false;
}

void Widget::on_StillDownloadingASPrintSave_clicked( )
{
    if( ui->StillDownloadingASPrintCombo->currentIndex( ) == 0 )
    {
        gUNM->m_StillDownloadingASPrint = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_stilldownloadingasprint", "1" );
    }
    else
    {
        gUNM->m_StillDownloadingASPrint = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_stilldownloadingasprint", "0" );
    }
}

void Widget::on_StillDownloadingASPrintRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_StillDownloadingASPrint = CFG.GetInt( "bot_stilldownloadingasprint", 1 ) != 0;

    if( gUNM->m_StillDownloadingASPrint )
        ui->StillDownloadingASPrintCombo->setCurrentIndex( 0 );
    else
        ui->StillDownloadingASPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_StillDownloadingASPrintReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_stilldownloadingasprint" );
    gUNM->m_StillDownloadingASPrint = CFG.GetInt( "bot_stilldownloadingasprint", 1 ) != 0;

    if( gUNM->m_StillDownloadingASPrint )
        ui->StillDownloadingASPrintCombo->setCurrentIndex( 0 );
    else
        ui->StillDownloadingASPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_NotSpoofCheckedASPrintApply_clicked( )
{
    ui->NotSpoofCheckedASPrintCombo->currentIndex( ) == 0 ? gUNM->m_NotSpoofCheckedASPrint = true : gUNM->m_NotSpoofCheckedASPrint = false;
}

void Widget::on_NotSpoofCheckedASPrintSave_clicked( )
{
    if( ui->NotSpoofCheckedASPrintCombo->currentIndex( ) == 0 )
    {
        gUNM->m_NotSpoofCheckedASPrint = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_notspoofcheckedasprint", "1" );
    }
    else
    {
        gUNM->m_NotSpoofCheckedASPrint = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_notspoofcheckedasprint", "0" );
    }
}

void Widget::on_NotSpoofCheckedASPrintRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NotSpoofCheckedASPrint = CFG.GetInt( "bot_notspoofcheckedasprint", 1 ) != 0;

    if( gUNM->m_NotSpoofCheckedASPrint )
        ui->NotSpoofCheckedASPrintCombo->setCurrentIndex( 0 );
    else
        ui->NotSpoofCheckedASPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_NotSpoofCheckedASPrintReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_notspoofcheckedasprint" );
    gUNM->m_NotSpoofCheckedASPrint = CFG.GetInt( "bot_notspoofcheckedasprint", 1 ) != 0;

    if( gUNM->m_NotSpoofCheckedASPrint )
        ui->NotSpoofCheckedASPrintCombo->setCurrentIndex( 0 );
    else
        ui->NotSpoofCheckedASPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_NotPingedASPrintApply_clicked( )
{
    ui->NotPingedASPrintCombo->currentIndex( ) == 0 ? gUNM->m_NotPingedASPrint = true : gUNM->m_NotPingedASPrint = false;
}

void Widget::on_NotPingedASPrintSave_clicked( )
{
    if( ui->NotPingedASPrintCombo->currentIndex( ) == 0 )
    {
        gUNM->m_NotPingedASPrint = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_notpingedasprint", "1" );
    }
    else
    {
        gUNM->m_NotPingedASPrint = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_notpingedasprint", "0" );
    }
}

void Widget::on_NotPingedASPrintRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NotPingedASPrint = CFG.GetInt( "bot_notpingedasprint", 1 ) != 0;

    if( gUNM->m_NotPingedASPrint )
        ui->NotPingedASPrintCombo->setCurrentIndex( 0 );
    else
        ui->NotPingedASPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_NotPingedASPrintReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_notpingedasprint" );
    gUNM->m_NotPingedASPrint = CFG.GetInt( "bot_notpingedasprint", 1 ) != 0;

    if( gUNM->m_NotPingedASPrint )
        ui->NotPingedASPrintCombo->setCurrentIndex( 0 );
    else
        ui->NotPingedASPrintCombo->setCurrentIndex( 1 );
}

void Widget::on_StillDownloadingASPrintDelayApply_clicked( )
{
    gUNM->m_StillDownloadingASPrintDelay = ui->StillDownloadingASPrintDelaySpin->value( );
}

void Widget::on_StillDownloadingASPrintDelaySave_clicked( )
{
    gUNM->m_StillDownloadingASPrintDelay = ui->StillDownloadingASPrintDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_stilldownloadingasprintdelay", UTIL_ToString( gUNM->m_StillDownloadingASPrintDelay ) );
}

void Widget::on_StillDownloadingASPrintDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_StillDownloadingASPrintDelay = CFG.GetUInt( "bot_stilldownloadingasprintdelay", 5 );
    ui->StillDownloadingASPrintDelaySpin->setValue( gUNM->m_StillDownloadingASPrintDelay );
}

void Widget::on_StillDownloadingASPrintDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_stilldownloadingasprintdelay" );
    gUNM->m_StillDownloadingASPrintDelay = CFG.GetUInt( "bot_stilldownloadingasprintdelay", 5 );
    ui->StillDownloadingASPrintDelaySpin->setValue( gUNM->m_StillDownloadingASPrintDelay );
}

void Widget::on_NotSpoofCheckedASPrintDelayApply_clicked( )
{
    gUNM->m_NotSpoofCheckedASPrintDelay = ui->NotSpoofCheckedASPrintDelaySpin->value( );
}

void Widget::on_NotSpoofCheckedASPrintDelaySave_clicked( )
{
    gUNM->m_NotSpoofCheckedASPrintDelay = ui->NotSpoofCheckedASPrintDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_notspoofcheckedasprintdelay", UTIL_ToString( gUNM->m_NotSpoofCheckedASPrintDelay ) );
}

void Widget::on_NotSpoofCheckedASPrintDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NotSpoofCheckedASPrintDelay = CFG.GetUInt( "bot_notspoofcheckedasprintdelay", 3 );
    ui->NotSpoofCheckedASPrintDelaySpin->setValue( gUNM->m_NotSpoofCheckedASPrintDelay );
}

void Widget::on_NotSpoofCheckedASPrintDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_notspoofcheckedasprintdelay" );
    gUNM->m_NotSpoofCheckedASPrintDelay = CFG.GetUInt( "bot_notspoofcheckedasprintdelay", 3 );
    ui->NotSpoofCheckedASPrintDelaySpin->setValue( gUNM->m_NotSpoofCheckedASPrintDelay );
}

void Widget::on_NotPingedASPrintDelayApply_clicked( )
{
    gUNM->m_NotPingedASPrintDelay = ui->NotPingedASPrintDelaySpin->value( );
}

void Widget::on_NotPingedASPrintDelaySave_clicked( )
{
    gUNM->m_NotPingedASPrintDelay = ui->NotPingedASPrintDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_notpingedasprintdelay", UTIL_ToString( gUNM->m_NotPingedASPrintDelay ) );
}

void Widget::on_NotPingedASPrintDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_NotPingedASPrintDelay = CFG.GetUInt( "bot_notpingedasprintdelay", 3 );
    ui->NotPingedASPrintDelaySpin->setValue( gUNM->m_NotPingedASPrintDelay );
}

void Widget::on_NotPingedASPrintDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_notpingedasprintdelay" );
    gUNM->m_NotPingedASPrintDelay = CFG.GetUInt( "bot_notpingedasprintdelay", 3 );
    ui->NotPingedASPrintDelaySpin->setValue( gUNM->m_NotPingedASPrintDelay );
}

void Widget::on_PlayerBeforeStartPrintDelayApply_clicked( )
{
    gUNM->m_PlayerBeforeStartPrintDelay = ui->PlayerBeforeStartPrintDelaySpin->value( );
}

void Widget::on_PlayerBeforeStartPrintDelaySave_clicked( )
{
    gUNM->m_PlayerBeforeStartPrintDelay = ui->PlayerBeforeStartPrintDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_playerbeforestartprintdelay", UTIL_ToString( gUNM->m_PlayerBeforeStartPrintDelay ) );
}

void Widget::on_PlayerBeforeStartPrintDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PlayerBeforeStartPrintDelay = CFG.GetUInt( "bot_playerbeforestartprintdelay", 60 );
    ui->PlayerBeforeStartPrintDelaySpin->setValue( gUNM->m_PlayerBeforeStartPrintDelay );
}

void Widget::on_PlayerBeforeStartPrintDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_playerbeforestartprintdelay" );
    gUNM->m_PlayerBeforeStartPrintDelay = CFG.GetUInt( "bot_playerbeforestartprintdelay", 60 );
    ui->PlayerBeforeStartPrintDelaySpin->setValue( gUNM->m_PlayerBeforeStartPrintDelay );
}

void Widget::on_RehostPrintingDelayApply_clicked( )
{
    gUNM->m_RehostPrintingDelay = ui->RehostPrintingDelaySpin->value( );
}

void Widget::on_RehostPrintingDelaySave_clicked( )
{
    gUNM->m_RehostPrintingDelay = ui->RehostPrintingDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_rehostprintingdelay", UTIL_ToString( gUNM->m_RehostPrintingDelay ) );
}

void Widget::on_RehostPrintingDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_RehostPrintingDelay = CFG.GetUInt( "bot_rehostprintingdelay", 12 );
    ui->RehostPrintingDelaySpin->setValue( gUNM->m_RehostPrintingDelay );
}

void Widget::on_RehostPrintingDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_rehostprintingdelay" );
    gUNM->m_RehostPrintingDelay = CFG.GetUInt( "bot_rehostprintingdelay", 12 );
    ui->RehostPrintingDelaySpin->setValue( gUNM->m_RehostPrintingDelay );
}

void Widget::on_ButGameDelayApply_clicked( )
{
    gUNM->m_ButGameDelay = ui->ButGameDelaySpin->value( );
}

void Widget::on_ButGameDelaySave_clicked( )
{
    gUNM->m_ButGameDelay = ui->ButGameDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_butgamedelay", UTIL_ToString( gUNM->m_ButGameDelay ) );
}

void Widget::on_ButGameDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_ButGameDelay = CFG.GetUInt( "bot_butgamedelay", 3 );
    ui->ButGameDelaySpin->setValue( gUNM->m_ButGameDelay );
}

void Widget::on_ButGameDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_butgamedelay" );
    gUNM->m_ButGameDelay = CFG.GetUInt( "bot_butgamedelay", 3 );
    ui->ButGameDelaySpin->setValue( gUNM->m_ButGameDelay );
}

void Widget::on_MarsGameDelayApply_clicked( )
{
    gUNM->m_MarsGameDelay = ui->MarsGameDelaySpin->value( );
}

void Widget::on_MarsGameDelaySave_clicked( )
{
    gUNM->m_MarsGameDelay = ui->MarsGameDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_marsgamedelay", UTIL_ToString( gUNM->m_MarsGameDelay ) );
}

void Widget::on_MarsGameDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_MarsGameDelay = CFG.GetUInt( "bot_marsgamedelay", 3 );
    ui->MarsGameDelaySpin->setValue( gUNM->m_MarsGameDelay );
}

void Widget::on_MarsGameDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_marsgamedelay" );
    gUNM->m_MarsGameDelay = CFG.GetUInt( "bot_marsgamedelay", 3 );
    ui->MarsGameDelaySpin->setValue( gUNM->m_MarsGameDelay );
}

void Widget::on_SlapGameDelayApply_clicked( )
{
    gUNM->m_SlapGameDelay = ui->SlapGameDelaySpin->value( );
}

void Widget::on_SlapGameDelaySave_clicked( )
{
    gUNM->m_SlapGameDelay = ui->SlapGameDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_slapgamedelay", UTIL_ToString( gUNM->m_SlapGameDelay ) );
}

void Widget::on_SlapGameDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_SlapGameDelay = CFG.GetUInt( "bot_slapgamedelay", 3 );
    ui->SlapGameDelaySpin->setValue( gUNM->m_SlapGameDelay );
}

void Widget::on_SlapGameDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_slapgamedelay" );
    gUNM->m_SlapGameDelay = CFG.GetUInt( "bot_slapgamedelay", 3 );
    ui->SlapGameDelaySpin->setValue( gUNM->m_SlapGameDelay );
}

void Widget::on_RollGameDelayApply_clicked( )
{
    gUNM->m_RollGameDelay = ui->RollGameDelaySpin->value( );
}

void Widget::on_RollGameDelaySave_clicked( )
{
    gUNM->m_RollGameDelay = ui->RollGameDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_rollgamedelay", UTIL_ToString( gUNM->m_RollGameDelay ) );
}

void Widget::on_RollGameDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_RollGameDelay = CFG.GetUInt( "bot_rollgamedelay", 0 );
    ui->RollGameDelaySpin->setValue( gUNM->m_RollGameDelay );
}

void Widget::on_RollGameDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_rollgamedelay" );
    gUNM->m_RollGameDelay = CFG.GetUInt( "bot_rollgamedelay", 0 );
    ui->RollGameDelaySpin->setValue( gUNM->m_RollGameDelay );
}

void Widget::on_PictureGameDelayApply_clicked( )
{
    gUNM->m_PictureGameDelay = ui->PictureGameDelaySpin->value( );
}

void Widget::on_PictureGameDelaySave_clicked( )
{
    gUNM->m_PictureGameDelay = ui->PictureGameDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_picturegamedelay", UTIL_ToString( gUNM->m_PictureGameDelay ) );
}

void Widget::on_PictureGameDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_PictureGameDelay = CFG.GetUInt( "bot_picturegamedelay", 3 );
    ui->PictureGameDelaySpin->setValue( gUNM->m_PictureGameDelay );
}

void Widget::on_PictureGameDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_picturegamedelay" );
    gUNM->m_PictureGameDelay = CFG.GetUInt( "bot_picturegamedelay", 3 );
    ui->PictureGameDelaySpin->setValue( gUNM->m_PictureGameDelay );
}

void Widget::on_EatGameDelayApply_clicked( )
{
    gUNM->m_EatGameDelay = ui->EatGameDelaySpin->value( );
}

void Widget::on_EatGameDelaySave_clicked( )
{
    gUNM->m_EatGameDelay = ui->EatGameDelaySpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_eatgamedelay", UTIL_ToString( gUNM->m_EatGameDelay ) );
}

void Widget::on_EatGameDelayRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_EatGameDelay = CFG.GetUInt( "bot_eatgamedelay", 3 );
    ui->EatGameDelaySpin->setValue( gUNM->m_EatGameDelay );
}

void Widget::on_EatGameDelayReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_eatgamedelay" );
    gUNM->m_EatGameDelay = CFG.GetUInt( "bot_eatgamedelay", 3 );
    ui->EatGameDelaySpin->setValue( gUNM->m_EatGameDelay );
}

void Widget::on_BotChannelCustomLine_returnPressed( )
{
    on_BotChannelCustomApply_clicked( );
}

void Widget::on_BotChannelCustomApply_clicked( )
{
    gUNM->m_BotChannelCustom = ui->BotChannelCustomLine->text( ).toStdString( );
}

void Widget::on_BotChannelCustomSave_clicked( )
{
    gUNM->m_BotChannelCustom = ui->BotChannelCustomLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_botchannelcustom", gUNM->m_BotChannelCustom );
}

void Widget::on_BotChannelCustomRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_BotChannelCustom = CFG.GetString( "bot_botchannelcustom", string( ) );
    ui->BotChannelCustomLine->setText( QString::fromUtf8( gUNM->m_BotChannelCustom.c_str( ) ) );
}

void Widget::on_BotChannelCustomReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_botchannelcustom" );
    gUNM->m_BotChannelCustom = CFG.GetString( "bot_botchannelcustom", string( ) );
    ui->BotChannelCustomLine->setText( QString::fromUtf8( gUNM->m_BotChannelCustom.c_str( ) ) );
}

void Widget::on_BotNameCustomLine_returnPressed( )
{
    on_BotNameCustomApply_clicked( );
}

void Widget::on_BotNameCustomApply_clicked( )
{
    gUNM->m_BotNameCustom = ui->BotNameCustomLine->text( ).toStdString( );
}

void Widget::on_BotNameCustomSave_clicked( )
{
    gUNM->m_BotNameCustom = ui->BotNameCustomLine->text( ).toStdString( );
    UTIL_SetVarInFile( "unm.cfg", "bot_botnamecustom", gUNM->m_BotNameCustom );
}

void Widget::on_BotNameCustomRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_BotNameCustom = CFG.GetString( "bot_botnamecustom", string( ) );
    ui->BotNameCustomLine->setText( QString::fromUtf8( gUNM->m_BotNameCustom.c_str( ) ) );
}

void Widget::on_BotNameCustomReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_botnamecustom" );
    gUNM->m_BotNameCustom = CFG.GetString( "bot_botnamecustom", string( ) );
    ui->BotNameCustomLine->setText( QString::fromUtf8( gUNM->m_BotNameCustom.c_str( ) ) );
}

void Widget::on_GameStartedMessagePrintingApply_clicked( )
{
    ui->GameStartedMessagePrintingCombo->currentIndex( ) == 0 ? gUNM->m_GameStartedMessagePrinting = true : gUNM->m_GameStartedMessagePrinting = false;
}

void Widget::on_GameStartedMessagePrintingSave_clicked( )
{
    if( ui->GameStartedMessagePrintingCombo->currentIndex( ) == 0 )
    {
        gUNM->m_GameStartedMessagePrinting = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_gamestartedmessageprinting", "1" );
    }
    else
    {
        gUNM->m_GameStartedMessagePrinting = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_gamestartedmessageprinting", "0" );
    }
}

void Widget::on_GameStartedMessagePrintingRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_GameStartedMessagePrinting = CFG.GetInt( "bot_gamestartedmessageprinting", 1 ) != 0;

    if( gUNM->m_GameStartedMessagePrinting )
        ui->GameStartedMessagePrintingCombo->setCurrentIndex( 0 );
    else
        ui->GameStartedMessagePrintingCombo->setCurrentIndex( 1 );
}

void Widget::on_GameStartedMessagePrintingReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_gamestartedmessageprinting" );
    gUNM->m_GameStartedMessagePrinting = CFG.GetInt( "bot_gamestartedmessageprinting", 1 ) != 0;

    if( gUNM->m_GameStartedMessagePrinting )
        ui->GameStartedMessagePrintingCombo->setCurrentIndex( 0 );
    else
        ui->GameStartedMessagePrintingCombo->setCurrentIndex( 1 );
}

void Widget::on_RejectColoredNameApply_clicked( )
{
    ui->RejectColoredNameCombo->currentIndex( ) == 0 ? gUNM->m_RejectColoredName = true : gUNM->m_RejectColoredName = false;
}

void Widget::on_RejectColoredNameSave_clicked( )
{
    if( ui->RejectColoredNameCombo->currentIndex( ) == 0 )
    {
        gUNM->m_RejectColoredName = true;
        UTIL_SetVarInFile( "unm.cfg", "bot_rejectcolorname", "1" );
    }
    else
    {
        gUNM->m_RejectColoredName = false;
        UTIL_SetVarInFile( "unm.cfg", "bot_rejectcolorname", "0" );
    }
}

void Widget::on_RejectColoredNameRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_RejectColoredName = CFG.GetInt( "bot_rejectcolorname", 0 ) != 0;

    if( gUNM->m_RejectColoredName )
        ui->RejectColoredNameCombo->setCurrentIndex( 0 );
    else
        ui->RejectColoredNameCombo->setCurrentIndex( 1 );
}

void Widget::on_RejectColoredNameReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_rejectcolorname" );
    gUNM->m_RejectColoredName = CFG.GetInt( "bot_rejectcolorname", 0 ) != 0;

    if( gUNM->m_RejectColoredName )
        ui->RejectColoredNameCombo->setCurrentIndex( 0 );
    else
        ui->RejectColoredNameCombo->setCurrentIndex( 1 );
}

void Widget::on_DenyMinDownloadRateApply_clicked( )
{
    gUNM->m_DenyMinDownloadRate = ui->DenyMinDownloadRateSpin->value( );
}

void Widget::on_DenyMinDownloadRateSave_clicked( )
{
    gUNM->m_DenyMinDownloadRate = ui->DenyMinDownloadRateSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_mindownloadrate", UTIL_ToString( gUNM->m_DenyMinDownloadRate ) );
}

void Widget::on_DenyMinDownloadRateRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DenyMinDownloadRate = CFG.GetUInt( "bot_mindownloadrate", 0 );
    ui->DenyMinDownloadRateSpin->setValue( gUNM->m_DenyMinDownloadRate );
}

void Widget::on_DenyMinDownloadRateReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_mindownloadrate" );
    gUNM->m_DenyMinDownloadRate = CFG.GetUInt( "bot_mindownloadrate", 0 );
    ui->DenyMinDownloadRateSpin->setValue( gUNM->m_DenyMinDownloadRate );
}

void Widget::on_DenyMaxDownloadTimeApply_clicked( )
{
    gUNM->m_DenyMaxDownloadTime = ui->DenyMaxDownloadTimeSpin->value( );
}

void Widget::on_DenyMaxDownloadTimeSave_clicked( )
{
    gUNM->m_DenyMaxDownloadTime = ui->DenyMaxDownloadTimeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maxdownloadtime", UTIL_ToString( gUNM->m_DenyMaxDownloadTime ) );
}

void Widget::on_DenyMaxDownloadTimeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DenyMaxDownloadTime = CFG.GetUInt( "bot_maxdownloadtime", 0 );
    ui->DenyMaxDownloadTimeSpin->setValue( gUNM->m_DenyMaxDownloadTime );
}

void Widget::on_DenyMaxDownloadTimeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maxdownloadtime" );
    gUNM->m_DenyMaxDownloadTime = CFG.GetUInt( "bot_maxdownloadtime", 0 );
    ui->DenyMaxDownloadTimeSpin->setValue( gUNM->m_DenyMaxDownloadTime );
}

void Widget::on_DenyMaxMapsizeTimeApply_clicked( )
{
    gUNM->m_DenyMaxMapsizeTime = ui->DenyMaxMapsizeTimeSpin->value( );
}

void Widget::on_DenyMaxMapsizeTimeSave_clicked( )
{
    gUNM->m_DenyMaxMapsizeTime = ui->DenyMaxMapsizeTimeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maxmapsizetime", UTIL_ToString( gUNM->m_DenyMaxMapsizeTime ) );
}

void Widget::on_DenyMaxMapsizeTimeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DenyMaxMapsizeTime = CFG.GetUInt( "bot_maxmapsizetime", 0 );
    ui->DenyMaxMapsizeTimeSpin->setValue( gUNM->m_DenyMaxMapsizeTime );
}

void Widget::on_DenyMaxMapsizeTimeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maxmapsizetime" );
    gUNM->m_DenyMaxMapsizeTime = CFG.GetUInt( "bot_maxmapsizetime", 0 );
    ui->DenyMaxMapsizeTimeSpin->setValue( gUNM->m_DenyMaxMapsizeTime );
}

void Widget::on_DenyMaxReqjoinTimeApply_clicked( )
{
    gUNM->m_DenyMaxReqjoinTime = ui->DenyMaxReqjoinTimeSpin->value( );
}

void Widget::on_DenyMaxReqjoinTimeSave_clicked( )
{
    gUNM->m_DenyMaxReqjoinTime = ui->DenyMaxReqjoinTimeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maxreqjointime", UTIL_ToString( gUNM->m_DenyMaxReqjoinTime ) );
}

void Widget::on_DenyMaxReqjoinTimeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DenyMaxReqjoinTime = CFG.GetUInt( "bot_maxreqjointime", 10 );
    ui->DenyMaxReqjoinTimeSpin->setValue( gUNM->m_DenyMaxReqjoinTime );
}

void Widget::on_DenyMaxReqjoinTimeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maxreqjointime" );
    gUNM->m_DenyMaxReqjoinTime = CFG.GetUInt( "bot_maxreqjointime", 10 );
    ui->DenyMaxReqjoinTimeSpin->setValue( gUNM->m_DenyMaxReqjoinTime );
}

void Widget::on_DenyMaxLoadTimeApply_clicked( )
{
    gUNM->m_DenyMaxLoadTime = ui->DenyMaxLoadTimeSpin->value( );
}

void Widget::on_DenyMaxLoadTimeSave_clicked( )
{
    gUNM->m_DenyMaxLoadTime = ui->DenyMaxLoadTimeSpin->value( );
    UTIL_SetVarInFile( "unm.cfg", "bot_maxloadtime", UTIL_ToString( gUNM->m_DenyMaxLoadTime ) );
}

void Widget::on_DenyMaxLoadTimeRefresh_clicked( )
{
    CConfig CFG;
    CFG.Read( "unm.cfg" );
    gUNM->m_DenyMaxLoadTime = CFG.GetUInt( "bot_maxloadtime", 0 );
    ui->DenyMaxLoadTimeSpin->setValue( gUNM->m_DenyMaxLoadTime );
}

void Widget::on_DenyMaxLoadTimeReset_clicked( )
{
    CConfig CFG;

#ifdef WIN32
    CFG.Read( "text_files\\default.cfg" );
#else
    CFG.Read( "text_files/default.cfg" );
#endif

    UTIL_DelVarInFile( "unm.cfg", "bot_maxloadtime" );
    gUNM->m_DenyMaxLoadTime = CFG.GetUInt( "bot_maxloadtime", 0 );
    ui->DenyMaxLoadTimeSpin->setValue( gUNM->m_DenyMaxLoadTime );
}

void Widget::on_designeGUISave_clicked( )
{
    string stylecfgfile;

#ifdef WIN32
    stylecfgfile = "styles\\config.cfg";
#else
    stylecfgfile = "styles/config.cfg";
#endif

    UTIL_SetVarInFile( stylecfgfile, "currentstyle", ui->designeGUICombo->currentText( ).toStdString( ) );
}

void Widget::on_designeGUISettings_clicked( )
{
    StyleSettingWindow styleWindow;
    styleWindow.setModal( true );
    styleWindow.exec( );
}

void Widget::on_designeGUIRefresh_clicked( )
{
    RefreshStylesList( ui->designeGUICombo->currentText( ) );
}

void Widget::on_designeGUIReset_clicked( )
{
    ui->designeGUICombo->setCurrentIndex( 0 );
    string stylecfgfile;

#ifdef WIN32
    stylecfgfile = "styles\\config.cfg";
#else
    stylecfgfile = "styles/config.cfg";
#endif

    UTIL_SetVarInFile( stylecfgfile, "currentstyle", "default" );
}

void Widget::resizeEvent( QResizeEvent * )
{
    TabsResizeFix( 6 );

    if( height( ) > ( m_SimplifiedStyle ? 440 : 473 ) )
    {
        if( !m_CornerLabelHaveImage )
        {
            m_CornerLabelHaveImage = true;
            m_CornerLabel->setText( "<img src=\":/mainwindow/unm.ico\" width=\"48\" height=\"48\"><br><font size=\"3\" face=\"Calibri\">unm client<br>v1.2.10</font>" );
        }
    }
    else if( m_CornerLabelHaveImage )
    {
        m_CornerLabelHaveImage = false;
        m_CornerLabel->setText( "<font size=\"3\" face=\"Calibri\">unm client<br>v1.2.10</font>" );
    }
}

void Widget::on_MainTab_currentChanged( int index )
{
    if( index == 0 || index == 3 || index == 4 || index == 5 )
        TabsResizeFix( index );

    if( index == 0 && ui->ChatTab->count( ) > 0 && ui->ChatTab->currentIndex( ) > 0 && (dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->CurrentTabIndex( ) == 0 ) )
        dynamic_cast<ChatWidget*>( ui->ChatTab->widget( ui->ChatTab->currentIndex( ) ) )->GetEntryField( )->setFocus( );
}

void Widget::on_ChatTab_currentChanged( int index )
{
    TabsResizeFix( 0 );

    if( index > 0 && ui->MainTab->currentIndex( ) == 0 && (dynamic_cast<ChatWidget*>( ui->ChatTab->widget( index ) )->CurrentTabIndex( ) == 0 ) )
        dynamic_cast<ChatWidget*>( ui->ChatTab->widget( index ) )->GetEntryField( )->setFocus( );
}

void Widget::TabsResizeFix( int type )
{
    if( type == 0 )
    {
        ui->ChatTab->tabBar( )->setMinimumWidth( ui->ChatTab->width( ) );

        if( ui->ChatTab->count( ) > 0 )
            dynamic_cast<ChatWidgetIrina*>( ui->ChatTab->widget( 0 ) )->TabsResizeFix( );

        for( int32_t n = 1; n < ui->ChatTab->count( ); ++n )
            dynamic_cast<ChatWidget*>( ui->ChatTab->widget( n ) )->TabsResizeFix( );
    }
    else if( type == 3 )
        ui->ToolsTab->tabBar( )->setMinimumWidth( ui->ToolsTab->width( ) );
    else if( type == 4 )
        ui->SettingsTab->tabBar( )->setMinimumWidth( ui->SettingsTab->width( ) );
    else if( type == 5 )
        ui->HelpTab->tabBar( )->setMinimumWidth( ui->HelpTab->width( ) );
    else if( ui->MainTab->currentIndex( ) == 0 )
    {
        ui->ChatTab->tabBar( )->setMinimumWidth( ui->ChatTab->width( ) );

        if( ui->ChatTab->count( ) > 0 )
            dynamic_cast<ChatWidgetIrina*>( ui->ChatTab->widget( 0 ) )->TabsResizeFix( );

        for( int32_t n = 1; n < ui->ChatTab->count( ); ++n )
            dynamic_cast<ChatWidget*>( ui->ChatTab->widget( n ) )->TabsResizeFix( );
    }
    else if( ui->MainTab->currentIndex( ) == 3 )
        ui->ToolsTab->tabBar( )->setMinimumWidth( ui->ToolsTab->width( ) );
    else if( ui->MainTab->currentIndex( ) == 4 )
        ui->SettingsTab->tabBar( )->setMinimumWidth( ui->SettingsTab->width( ) );
    else if( ui->MainTab->currentIndex( ) == 5 )
        ui->HelpTab->tabBar( )->setMinimumWidth( ui->HelpTab->width( ) );
}

void Widget::on_designeGUICombo_currentIndexChanged( int )
{
    if( !m_IgnoreChangeStyle )
        ChangeStyleGUI( );
}

void Widget::ChangeStyleGUI( QString styleName )
{
    QFile styleFile( styleName.isEmpty( ) ? "styles/" + ui->designeGUICombo->currentText( ) + ".qss" : "styles/" + styleName + ".qss" );

    if( !styleFile.exists( ) )
        gCurrentStyleName = "default";
    else if( styleName.isEmpty( ) )
        gCurrentStyleName = ui->designeGUICombo->currentText( ).toStdString( );
    else
        gCurrentStyleName = styleName.toStdString( );

    CConfig CFG;

#ifdef WIN32
    CFG.Read( "styles\\config.cfg" );
#else
    CFG.Read( "styles/config.cfg" );
#endif

    gNormalMessagesColor = QString::fromUtf8( CFG.GetString( gCurrentStyleName + "_normal", "#000000" ).c_str( ) );
    gPrivateMessagesColor = QString::fromUtf8( CFG.GetString( gCurrentStyleName + "_private", "#00ff00" ).c_str( ) );
    gNotifyMessagesColor = QString::fromUtf8( CFG.GetString( gCurrentStyleName + "_notify", "#0000ff" ).c_str( ) );
    gWarningMessagesColor = QString::fromUtf8( CFG.GetString( gCurrentStyleName + "_warning", "#ff0000" ).c_str( ) );
    gPreSentMessagesColor = QString::fromUtf8( CFG.GetString( gCurrentStyleName + "_present", "#808080" ).c_str( ) );
    gDefaultScrollBarForce = CFG.GetInt( gCurrentStyleName + "_scrollbardefault", 0 ) != 0;
    qApp->setStyleSheet( m_StyleDefault );

    if( gCurrentStyleName == "default" )
        qApp->setStyleSheet( gDefaultScrollBarForce ? ( "QWidget#CentralWidget { background-color:#ffffff; } " + m_ScrollBarStyleDefault ) : "QWidget#CentralWidget { background-color:#ffffff; }" );
    else
    {
        styleFile.open( QFile::ReadOnly );
        QString style( styleFile.readAll( ) );
        qApp->setStyleSheet( gDefaultScrollBarForce ? ( m_ScrollBarStyleDefault + style + m_ScrollAreaStyleDefault + " QTableCornerButton::section{ background-color: rgba(0,0,0,0); }" ) : ( style + m_ScrollAreaStyleDefault + " QTableCornerButton::section{ background-color: rgba(0,0,0,0); }" ) );
        styleFile.close( );
    }

    if( styleName.isEmpty( ) )
    {
        for( int32_t n = 0; n < ui->GameTab->count( ); ++n )
            dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->logfieldSetColor( );

        ui->outputfield->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
        ui->CommandList->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
        ui->ChangelogText->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
        ui->HelpText->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
        TabsResizeFix( 5 );
    }
}

void Widget::RefreshStylesList( QString currentstyle, bool first )
{
    m_IgnoreChangeStyle = true;
    ui->designeGUICombo->clear( );
    QDir dir( "styles" );
    QStringList styles = dir.entryList( QStringList( "*.qss" ), QDir::Files );
    ui->designeGUICombo->addItem( "default" );
    bool currentstylefound = false;

    if( currentstyle == "default" )
        currentstylefound = true;

    foreach( QString file, styles )
    {
        if( file.left( file.length( ) - 4 ) != "default" )
            ui->designeGUICombo->addItem( file.left( file.length( ) - 4 ) );

        if( !currentstylefound && file.left( file.length( ) - 4 ) == currentstyle )
            currentstylefound = true;
    }

    if( !currentstylefound )
        currentstyle = "default";

    int index = ui->designeGUICombo->findText( currentstyle );

    if( index == -1 )
        ui->designeGUICombo->setCurrentIndex( 0 );
    else
        ui->designeGUICombo->setCurrentIndex( index );

    m_IgnoreChangeStyle = false;
    
    if( first )
    {
        for( int32_t n = 0; n < ui->GameTab->count( ); ++n )
            dynamic_cast<GameWidget*>( ui->GameTab->widget( n ) )->logfieldSetColor( );
    
        ui->outputfield->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
        ui->CommandList->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
        ui->ChangelogText->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
        ui->HelpText->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
        TabsResizeFix( 5 );
    }
    else
        ChangeStyleGUI( );
}

void Widget::on_logBackup_clicked( )
{
    ui->logBackup->setEnabled( false );

    if( !gUNM || gUNM == nullptr )
        ui->logfield->appendHtml( "<font color = " + gWarningMessagesColor + ">Что-то пошло не так, требуется перезапуск...</font>" );
    else if( gUNM->m_LogBackup || gUNM->m_LogBackupForce )
        ui->logfield->appendHtml( "<font color = " + gWarningMessagesColor + ">В данный момент уже производиться бэкап лог-файла.</font>" );
    else
        gUNM->m_LogBackupForce = true;

    ui->logBackup->setEnabled( true );
}

void Widget::on_logClear_clicked( )
{
    ui->logClear->setEnabled( false );
    ui->logfield->clear( );
    ui->logClear->setEnabled( true );
}

void Widget::on_logEnable_clicked( )
{
    ui->logEnable->setEnabled( false );

    if( !gUNM || gUNM == nullptr )
        ui->logfield->appendHtml( "<font color = " + gWarningMessagesColor + ">Что-то пошло не так, требуется перезапуск...</font>" );
    else if( ui->logEnable->text( ) == "Отключить логирование в этом окне" )
    {
        if( gEnableLoggingToGUI == 2 )
            ui->logfield->appendHtml( "<font color = " + gNotifyMessagesColor + ">Логирование в данном окне </font><font color = " + gWarningMessagesColor + ">отключено</font><font color = " + gNotifyMessagesColor + ">!</font>" );

        gEnableLoggingToGUI = m_LoggingMode;
        ui->logEnable->setText( "Включить логирование в этом окне" );
    }
    else
    {
        if( gEnableLoggingToGUI != 2 )
            ui->logfield->appendHtml( "<font color = " + gNotifyMessagesColor + ">Логирование в данном окне </font><font color = " + gPrivateMessagesColor + ">включено</font><font color = " + gNotifyMessagesColor + ">!</font>" );

        gEnableLoggingToGUI = 2;
        ui->logEnable->setText( "Отключить логирование в этом окне" );
    }

    ui->logEnable->setEnabled( true );
}

void Widget::on_quickSetupWizard_clicked( )
{
    PresettingWindow pWindow;
    pWindow.setModal( true );
    pWindow.exec( );
}

void Widget::on_About_UNM_clicked( )
{
    QMessageBox::information( this, "UNM v1.2.10 (32 bit) by motherfuunnick", "UNMClient это pvpgn-клиент с GUI на основе GProxy с встроенным ботом для создания игр, irina-коннектором и лаунчером для Warcraft III, основан на UNMBot\nсобрано в Qt Creator 4.14.2 на Microsoft Visual C++ Compiler 16.9.31129.286 (MSVC2019) с использованием Qt 5.15.2. включает в себя библиотеки BNCSUtil v1.4.1, mpir v3.0.0+, StormLib v9.22+, curl v7.59.0\nUNMCLient также использует следующие программы/библиотеки:\niCCupLoader for bots (by Abso!)\nWFE v2.23 Update 4 (by Unryze)" );
}

void Widget::on_About_Qt_clicked( )
{
    QMessageBox::aboutQt( this, "Информация о Qt 5.15.2" );
}

void Widget::on_logSettings_clicked( )
{
    LogSettingsWindow lWindow;
    lWindow.setModal( true );
    lWindow.exec( );
}

void Widget::on_restartUNM_clicked( )
{
    // restart

    if( gExitProcessStarted )
    {
        QMessageBox messageBox( QMessageBox::Question, tr( "Перезапуск клиента" ), tr( "Уже начался процесс перезапуска программы.\nКлиент перезапустится по завершению всех процессов." ), QMessageBox::Yes | QMessageBox::No, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "Перезапустить сейчас" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "Подождать" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::Yes )
        {
            gExitProcessStarted = true;
            ui->restartUNM->setIcon( QIcon( ":/mainwindow/tools/restart_red.png" ) );
            gRestartAfterExiting = true;
            qApp->quit( );
            QProcess::startDetached( qApp->arguments( )[0], qApp->arguments( ) );
        }
    }
    else if( gUNM == nullptr )
    {
        QMessageBox messageBox( QMessageBox::Question, tr( "Перезапуск клиента" ), tr( "Выполнить перезапуск?" ), QMessageBox::Yes | QMessageBox::No, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "Перезапустить" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "Передумать" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::Yes )
        {
            gExitProcessStarted = true;
            ui->restartUNM->setIcon( QIcon( ":/mainwindow/tools/restart_red.png" ) );
            gRestartAfterExiting = true;
            qApp->quit( );
            QProcess::startDetached( qApp->arguments( )[0], qApp->arguments( ) );
        }
    }
    else if( !gUNM->m_Games.empty( ) )
    {
        QMessageBox messageBox( QMessageBox::Question, tr( "Перезапуск клиента" ), tr( "Имеются начатые игры в процессе!\nПезапустить клиент немедленно?" ), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "Перезапустить сейчас" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "Подождать и перезапустить" ) );
        messageBox.setButtonText( QMessageBox::Cancel, tr( "Передумать" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::Yes )
        {
            gExitProcessStarted = true;
            ui->restartUNM->setIcon( QIcon( ":/mainwindow/tools/restart_red.png" ) );
            gRestartAfterExiting = true;
            gUNM->m_Exiting = true;
        }
        else if( resBtn == QMessageBox::No )
        {
            gExitProcessStarted = true;
            ui->restartUNM->setIcon( QIcon( ":/mainwindow/tools/restart_red.png" ) );
            gRestartAfterExiting = true;
            gUNM->m_ExitingNice = true;
        }
    }
    else if( gUNM->m_CurrentGame )
    {
        QMessageBox messageBox( QMessageBox::Question, tr( "Перезапуск клиента" ), tr( "Имеется неначатая игра процессе!\nПезапустить клиент немедленно?" ), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "Перезапустить" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "Перезапустить безопасно" ) );
        messageBox.setButtonText( QMessageBox::Cancel, tr( "Передумать" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::Yes )
        {
            gExitProcessStarted = true;
            ui->restartUNM->setIcon( QIcon( ":/mainwindow/tools/restart_red.png" ) );
            gRestartAfterExiting = true;
            gUNM->m_Exiting = true;
        }
        else if( resBtn == QMessageBox::No )
        {
            gExitProcessStarted = true;
            ui->restartUNM->setIcon( QIcon( ":/mainwindow/tools/restart_red.png" ) );
            gRestartAfterExiting = true;
            gUNM->m_ExitingNice = true;
        }
    }
    else
    {
        QMessageBox messageBox( QMessageBox::Question, tr( "Перезапуск клиента" ), tr( "Выполнить перезапуск?" ), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this );
        messageBox.setButtonText( QMessageBox::Yes, tr( "Перезапустить" ) );
        messageBox.setButtonText( QMessageBox::No, tr( "Перезапустить безопасно" ) );
        messageBox.setButtonText( QMessageBox::Cancel, tr( "Передумать" ) );
        int resBtn = messageBox.exec( );

        if( resBtn == QMessageBox::Yes )
        {
            gExitProcessStarted = true;
            ui->restartUNM->setIcon( QIcon( ":/mainwindow/tools/restart_red.png" ) );
            gRestartAfterExiting = true;
            gUNM->m_Exiting = true;
        }
        else if( resBtn == QMessageBox::No )
        {
            gExitProcessStarted = true;
            ui->restartUNM->setIcon( QIcon( ":/mainwindow/tools/restart_red.png" ) );
            gRestartAfterExiting = true;
            gUNM->m_ExitingNice = true;
        }
    }
}

void Widget::on_shutdownUNM_clicked( )
{
    close( );
}

ChatWidget::ChatWidget( Widget *parent, unsigned int BnetID, QString BnetCommandTrigger, QString ServerName, QString UserName, unsigned int TimeWaitingToReconnect ) :
    m_MainWidget( parent ),
    m_ServerName( ServerName ),
    m_UserName( UserName ),
    ui( new Ui::ChatWidget ),
    m_BnetID( BnetID ),
    m_BnetCommandTrigger( BnetCommandTrigger ),
    m_TimeWaitingToReconnect( TimeWaitingToReconnect ),
    m_ReplyTarget( QString( ) ),
    m_Icons( { QIcon( ":/mainwindow/irina/gamelist/game_unm.png" ), QIcon( ":/mainwindow/irina/gamelist/game_irina_blue.png" ) } ),
    m_CurrentBotMap( QString( ) ),
    m_CurrentBotWebSite( QString( ) ),
    m_CurrentBotVK( QString( ) ),
    m_CurrentBotStatus( 0 )
{
    /*
    m_Icons:
    0 = w/o
    1 = unm
    2 = IirinaBlue
    */

    ui->setupUi( this );
    QList<int> sizes;
    sizes << ui->splitter->width( ) - 129 << 124;
    ui->splitter->setSizes( sizes );
    CustomStyledItemDelegate *delegate = new CustomStyledItemDelegate( ui->GameList, ui->GameList );
    ui->GameList->setItemDelegate( delegate );
    connect( ui->GameList, &QTableWidget::cellClicked, this, &ChatWidget::cellClickedCustom );
    connect( ui->GameList, &QTableWidget::cellDoubleClicked, this, &ChatWidget::cellDoubleClickedCustom );
    ui->GameList->setIconSize( QSize( 32, 16 ) );
    ui->GameList->setColumnWidth( 0, 174 );
    ui->GameList->setColumnWidth( 1, 183 );
    ui->GameList->setColumnWidth( 2, 61 );
    ui->scrollArea->hide( );
    QList<int> sizes2;
    sizes2 << 397 << 256;
    ui->creteGameSplitter->setSizes( sizes2 );
}

void ChatWidget::game_list_table_pos_fix_direct( )
{
    QApplication::sendEvent( ui->GameList->viewport( ), new QMouseEvent( QEvent::MouseMove, ui->GameList->viewport( )->mapFromGlobal( QCursor::pos( ) ), Qt::NoButton,  Qt::NoButton, Qt::NoModifier ) );
    ui->GameList->setSortingEnabled( true );
}

ChatWidget::~ChatWidget( )
{
    ui->GameList->clear( );
    delete ui;
}

void ChatWidget::cellClickedCustom( int iRow, int )
{
    if( ui->scrollArea->isHidden( ) )
        ui->scrollArea->show( );

    ui->mapName->setText( "Секундочку..." );
    ui->mapPlayers->clear( );
    ui->mapAuthor->clear( );
    ui->mapDescription->clear( );
    ui->mapImage->clear( );

    if( !gUNM )
        return;

    if( ui->GameList->item( iRow, 0 ) )
    {
        CIncomingGameHost *game = gUNM->m_BNETs[m_BnetID]->GetGame( ui->GameList->item( iRow, 0 )->data( gGameRoleID ).toUInt( ) );

        if( game == nullptr )
        {
            if( ui->GameList->item( iRow, 1 ) )
                ui->mapName->setText( ui->GameList->item( iRow, 1 )->text( ) );
            else
                ui->mapName->setText( "unknown" );

            ui->mapPlayers->setText( QString( ) );
            ui->mapAuthor->setText( QString( ) );
            ui->mapDescription->setText( "Описание карты: N/A" );
            ui->mapImage->setPixmap( QPixmap( ":/wc3map/map_image_unknown.png" ) );
        }
        else
        {
            ui->mapName->setText( QString::fromUtf8( game->GetMapName( ).c_str( ) ) );
            ui->mapPlayers->setText( "Кол-во игроков: " + QString::fromUtf8( game->GetMapPlayers( ).c_str( ) ) );
            ui->mapAuthor->setText( "Автор карты: " + QString::fromUtf8( game->GetMapAuthor( ).c_str( ) ) );
            ui->mapDescription->setText( "Описание карты: " + QString::fromUtf8( game->GetMapDescription( ).c_str( ) ) );

            if( !game->GetMapTGA( ).empty( ) )
            {
                bool success = false;
                QImage image = UTIL_LoadTga( game->GetMapTGA( ), success );

                if( success )
                    ui->mapImage->setPixmap( QPixmap::fromImage( image ) );
                else
                    ui->mapImage->setPixmap( QPixmap( ":/wc3map/map_image_unknown.png" ) );
            }
            else
                ui->mapImage->setPixmap( QPixmap( ":/wc3map/map_image_unknown.png" ) );
        }
    }
}

void ChatWidget::cellDoubleClickedCustom( int iRow, int )
{
    if( !gUNM )
        return;

    if( ui->GameList->item( iRow, 0 ) )
    {
        gUNM->m_BNETs[m_BnetID]->m_GamelistRequestQueue.push_back( ui->GameList->item( iRow, 0 )->data( gGameRoleID ).toUInt( ) );
        bool tabfound = false;
        uint32_t tabindex = m_MainWidget->GetTabIndexByGameNumber( ui->GameList->item( iRow, 0 )->data( gGameRoleID ).toUInt( ), tabfound );

        if( !tabfound )
        {
            // general info

            QStringList GameInfoInit;
            GameInfoInit.append( ui->GameList->item( iRow, 0 )->text( ) );
            GameInfoInit.append( ui->GameList->item( iRow, 1 )->text( ) );
            GameInfoInit.append( "Публичная игра" );
            GameInfoInit.append( "Неначатая игра (лобби)" );
            GameInfoInit.append( "Открытая (админ-команды доступны всем админам)" );
            GameInfoInit.append( ui->GameList->item( iRow, 2 )->text( ) );
            GameInfoInit.append( ui->GameList->item( iRow, 2 )->text( ) );
            GameInfoInit.append( ui->GameList->item( iRow, 2 )->text( ) );
            GameInfoInit.append( m_ServerName );
            struct tm * timeinfo;
            char buffer[150];
            string sDate;
            time_t Now = time( nullptr );
            timeinfo = localtime( &Now );
            strftime( buffer, 150, "%d.%m.%y %H:%M:%S", timeinfo );
            sDate = buffer;
            GameInfoInit.append( QString::fromUtf8( sDate.c_str( ) ) );
            GameInfoInit.append( "Статистика отключена" );

            // slots

            QStringList names;
            QStringList races;
            QStringList clans;
            QStringList colors;
            QStringList handicaps;
            QStringList statuses;
            QStringList pings;
            QStringList servers;
            QStringList accesses;
            QStringList levels;
            QStringList ips;
            QStringList countries;
/*          string name = string( );
            string race = string( );
            string clan = string( );
            string colour = string( );
            string handicap = string( );
            string status = string( );
            string ping = string( );
            string server = string( );
            string access = string( );
            string level = string( );
            string ip = string( );
            string country = string( );

            for( unsigned char i = 0; i < m_Slots.size( ); ++i )
            {
                name = string( );
                race = string( );
                clan = "Клан " + UTIL_ToString( m_Slots[i].GetTeam( ) + 1 );
                colour = string( );
                handicap = UTIL_ToString( m_Slots[i].GetHandicap( ) ) + "%";
                status = string( );
                ping = string( );
                server = string( );
                access = string( );
                level = string( );
                ip = string( );
                country = string( );

                switch( m_Slots[i].GetRace( ) )
                {
                    case SLOTRACE_RANDOM:
                    race = "Случайная раса";
                    break;
                    case SLOTRACE_UNDEAD:
                    race = "Нежить";
                    break;
                    case SLOTRACE_HUMAN:
                    race = "Альянс";
                    break;
                    case SLOTRACE_ORC:
                    race = "Орда";
                    break;
                    case SLOTRACE_NIGHTELF:
                    race = "Ночные эльфы";
                    break;
                    case 96:
                    race = "Случайная раса";
                    break;
                    case 72:
                    race = "Нежить";
                    break;
                    case 65:
                    race = "Альянс";
                    break;
                    case 66:
                    race = "Орда";
                    break;
                    case 68:
                    race = "Ночные эльфы";
                    break;
                    case 64:
                    race = "Случайная раса";
                    break;
                    default:
                    race = "Случайная раса";
                }

                switch( m_Slots[i].GetColour( ) )
                {
                    case 0:
                    colour = "red";
                    break;
                    case 1:
                    colour = "blue";
                    break;
                    case 2:
                    colour = "teal";
                    break;
                    case 3:
                    colour = "purple";
                    break;
                    case 4:
                    colour = "yellow";
                    break;
                    case 5:
                    colour = "orange";
                    break;
                    case 6:
                    colour = "green";
                    break;
                    case 7:
                    colour = "pink";
                    break;
                    case 8:
                    colour = "gray";
                    break;
                    case 9:
                    colour = "lightblue";
                    break;
                    case 10:
                    colour = "darkgreen";
                    break;
                    case 11:
                    colour = "brown";
                    break;
                    default:
                    colour = "black";
                }

                if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetComputer( ) == 0 )
                {
                    CGamePlayer *player = GetPlayerFromSID2( i );

                    if( player )
                    {
                        name = player->GetName( );
                        country = "unknown";
                        level = "1 lvl";
                        ping = UTIL_ToString( player->GetPing( m_UNM->m_LCPings ) ) + " ms";
                        ip = player->GetExternalIPString( );

                        if( !m_UNM->IsBnetServer( player->GetJoinedRealm( ) ) )
                            server = "LAN";
                        else
                            server = player->GetJoinedRealm( );

                        if( IsOwner( player->GetName( ) ) )
                            access = "owner";
                        else
                            access = "user";

                        if( !m_GameLoaded && !m_GameLoading )
                        {
                            if( m_Slots[i].GetDownloadStatus( ) == 255 || m_Slots[i].GetDownloadStatus( ) == 100 )
                                status = "ожидает старта игры";
                            else
                                status = "качает карту: " + UTIL_ToString( m_Slots[i].GetDownloadStatus( ) ) + "%";
                        }
                        else if( m_GameLoading )
                        {
                            if( !player->GetFinishedLoading( ) )
                                status = "грузит карту";
                            else
                                status = "ждет начала игры";
                        }
                        else if( player->GetLeftTime( ) > 0 )
                            status = "ливнул на " + UTIL_ToString( player->GetLeftTime( ) ) + " секунде, причина: " + player->GetLeftReason( );
                        else
                            status = "играет";
                    }
                    else
                        name = "Fakeplayer";
                }
                else if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED )
                {
                    if( m_Slots[i].GetComputerType( ) == SLOTCOMP_EASY )
                        name = "Комьютер (слабый)";
                    else if( m_Slots[i].GetComputerType( ) == SLOTCOMP_NORMAL )
                        name = "Комьютер (средний)";
                    else if( m_Slots[i].GetComputerType( ) == SLOTCOMP_HARD )
                        name = "Комьютер (сильный)";
                    else
                        name = "Комьютер";
                }
                else if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN )
                    name = "Открыто";
                else if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_CLOSED )
                    name = "Закрыто";

                names.append( QString::fromUtf8( name.c_str( ) ) );
                races.append( QString::fromUtf8( race.c_str( ) ) );
                clans.append( QString::fromUtf8( clan.c_str( ) ) );
                colors.append( QString::fromUtf8( colour.c_str( ) ) );
                handicaps.append( QString::fromUtf8( handicap.c_str( ) ) );
                statuses.append( QString::fromUtf8( status.c_str( ) ) );
                pings.append( QString::fromUtf8( ping.c_str( ) ) );
                servers.append( QString::fromUtf8( server.c_str( ) ) );
                accesses.append( QString::fromUtf8( access.c_str( ) ) );
                levels.append( QString::fromUtf8( level.c_str( ) ) );
                ips.append( QString::fromUtf8( ip.c_str( ) ) );
                countries.append( QString::fromUtf8( country.c_str( ) ) );
            }
*/
            m_MainWidget->add_game( ui->GameList->item( iRow, 0 )->data( gGameRoleID ).toUInt( ), GameInfoInit, { names, races, clans, colors, handicaps, statuses, pings, accesses, levels, servers, ips, countries } );
        }

        m_MainWidget->ChangeCurrentGameTab( tabindex );
        m_MainWidget->ChangeCurrentMainTab( 1 );
    }
}

QString ChatWidget::GetBnetCommandTrigger( )
{
    return m_BnetCommandTrigger;
}

QLineEdit *ChatWidget::GetEntryField( )
{
    return ui->entryfield;
}

QListWidget *ChatWidget::GetChatMembers( )
{
    return ui->chatmembers;
}

unsigned int ChatWidget::CurrentTabIndex( )
{
    return ui->tabWidget->currentIndex( );
}

void ChatWidget::TabsResizeFix( )
{
    ui->tabWidget->tabBar( )->setMinimumWidth( ui->tabWidget->width( ) );
}

void ChatWidget::on_tabWidget_currentChanged( int index )
{
    if( index == 0 )
        ui->entryfield->setFocus( );
}

void ChatWidget::on_chatmembers_customContextMenuRequested( const QPoint &pos )
{
    if( gUNM && gUNM != nullptr && gUNM->m_BNETs.size( ) > m_BnetID )
    {
        QListWidgetItem *selectedItem = ui->chatmembers->itemAt( pos );

        if( selectedItem )
        {
            QPoint item = ui->chatmembers->mapToGlobal( pos );
            QMenu submenu;
            string itemText = selectedItem->text( ).toStdString( );
            string itemTextLower = itemText;
            string UserNameLower = gUNM->m_BNETs[m_BnetID]->GetName( );
            transform( itemTextLower.begin( ), itemTextLower.end( ), itemTextLower.begin( ), static_cast<int(*)(int)>(tolower) );
            transform( UserNameLower.begin( ), UserNameLower.end( ), UserNameLower.begin( ), static_cast<int(*)(int)>(tolower) );

            if( itemTextLower != UserNameLower )
            {
                if( gUNM->m_BNETs[m_BnetID]->GetLoggedIn( ) )
                {
                    submenu.addAction( "Написать в лс" );
                    submenu.addAction( "Добавить в друзья" );
                    submenu.addAction( "Удалить из друзей" );
                }
            }

            if( gUNM->m_BNETs[m_BnetID]->GetLoggedIn( ) )
                submenu.addAction( "AH check" );

            if( itemTextLower != UserNameLower || gUNM->m_BNETs[m_BnetID]->GetLoggedIn( ) )
            {
                QAction *selectedLine = submenu.exec( item );

                if( selectedLine )
                {
                    QString selectedLineQString = selectedLine->text( );

                    if( selectedLineQString == "Написать в лс" )
                    {
                        QString saytouser2 = "/w " + selectedItem->text( );
                        QString saytouser = saytouser2 + " ";
                        QString currenttext = ui->entryfield->text( );

                        if( currenttext.length( ) >= saytouser )
                        {
                            if( currenttext.left( saytouser.length( ) ) == saytouser )
                                return;
                            else if( currenttext.left( saytouser2.length( ) ) == saytouser2 )
                                currenttext = saytouser + currenttext.mid( saytouser2.length( ) );
                            else if( currenttext.left( 3 ) == "/w " )
                                currenttext = saytouser + currenttext.mid( 3 );
                            else if( currenttext.left( 2 ) == "/w" )
                                currenttext = saytouser + currenttext.mid( 2 );
                            else
                                currenttext = saytouser + currenttext.mid( 2 );
                        }
                        else if( currenttext.length( ) == saytouser2 )
                        {
                            if( currenttext.left( saytouser2.length( ) ) == saytouser2 )
                                currenttext = saytouser + currenttext.mid( saytouser2.length( ) );
                            else if( currenttext.left( 3 ) == "/w " )
                                currenttext = saytouser + currenttext.mid( 3 );
                            else if( currenttext.left( 2 ) == "/w" )
                                currenttext = saytouser + currenttext.mid( 2 );
                            else
                                currenttext = saytouser + currenttext.mid( 2 );
                        }
                        else
                            currenttext = saytouser + currenttext.mid( 2 );

                        if( currenttext.length( ) > ui->entryfield->maxLength( ) )
                            currenttext = currenttext.left( ui->entryfield->maxLength( ) );

                        ui->entryfield->setText( currenttext );
                        ui->entryfield->setFocus( );
                    }
                    else if( selectedLineQString == "Добавить в друзья" )
                    {
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandWhisperQueue.push_back( true );
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandUserQueue.push_back( string( ) );
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandQueue.push_back( "/f add " + itemText );
                    }
                    else if( selectedLineQString == "Удалить из друзей" )
                    {
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandWhisperQueue.push_back( true );
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandUserQueue.push_back( string( ) );
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandQueue.push_back( "/f remove " + itemText );
                    }
                    else if( selectedLineQString == "AH check" )
                    {
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandWhisperQueue.push_back( true );
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandUserQueue.push_back( string( ) );
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandQueue.push_back( "/ah " + itemText );
                    }
                }
            }
        }
    }
}

void ChatWidget::on_entryfield_textChanged( const QString &arg1 )
{
    if( arg1.size( ) > 1 && arg1.left( 1 ) == m_BnetCommandTrigger )
    {
        for( int i = 0; i < m_MainWidget->m_StringList.size( ); ++i )
        {
            if( arg1.mid( 1 ) == m_MainWidget->m_StringList[i] )
            {
                ui->entryfield->setText( arg1.left( arg1.indexOf(' ') ) );
                break;
            }
        }
    }
}

void ChatWidget::on_entryfield_textEdited( const QString &arg1 )
{
    if( gUNM && gUNM != nullptr && gUNM->m_BNETs.size( ) > m_BnetID && !gUNM->m_BNETs[m_BnetID]->m_ReplyTarget.empty( ) )
    {
        if( arg1 == "/r" && ui->entryfield->cursorPosition( ) == 2 )
        {
            string ReplyTarget = "/w " + gUNM->m_BNETs[m_BnetID]->m_ReplyTarget + " ";
            m_ReplyTarget = QString::fromUtf8( ReplyTarget.c_str( ) );
            ui->entryfield->setText( m_ReplyTarget );
        }
        else if( arg1 == "/reply" && ui->entryfield->cursorPosition( ) == 6 )
        {
            string ReplyTarget = "/w " + gUNM->m_BNETs[m_BnetID]->m_ReplyTarget + " ";
            m_ReplyTarget = QString::fromUtf8( ReplyTarget.c_str( ) );
            ui->entryfield->setText( m_ReplyTarget );
        }
        else if( !m_ReplyTarget.isEmpty( ) && arg1 == m_ReplyTarget + " " )
        {
            ui->entryfield->setText( m_ReplyTarget );
            m_ReplyTarget.clear( );
        }
        else
            m_ReplyTarget.clear( );
    }
    else
        m_ReplyTarget.clear( );
}

void ChatWidget::on_send_clicked( )
{
    sending_messages_to_battlenet( );
}

void ChatWidget::on_entryfield_returnPressed( )
{
    sending_messages_to_battlenet( );
}

void ChatWidget::bnet_disconnect_direct( )
{
    ui->chatmembers->clear( );
    write_to_chat_direct( "Соединение с сервером [" + m_ServerName + "] потеряно...4", QString( ) );
    write_to_chat_direct( "Ожидайте автоматического переподключения (каждые " + UTIL_ToQString( m_TimeWaitingToReconnect ) + " сек.)3", QString( ) );
}

void ChatWidget::bnet_reconnect_direct( )
{
    ui->chatmembers->clear( );
    write_to_chat_direct( "Соединение с сервером [" + m_ServerName + "] восстановлено!2", QString( ) );
}

void ChatWidget::write_to_chat_direct( QString message, QString msg )
{
    message.replace( "<", "&lt;" );
    message.replace( ">", "&gt;" );
    message.replace( "  ", " &nbsp;" );
    message.replace( "  ", "&nbsp; " );
    QString formattedTime;

    if( message.mid( message.size( ) - 1 ) != "1" )
    {
        time_t Now = time( nullptr );
        string Time = asctime( localtime( &Now ) );
        Time = Time.substr( Time.size( ) - 14, 5 );
        formattedTime = "<font face=\"Arial\" color = " + gPreSentMessagesColor + ">" + QString::fromUtf8( Time.c_str( ) ) + " </font>";
    }
    else
        formattedTime = "<img src=\":/mainwindow/chat/chat_loading6.png\"> ";

    if( message.mid( message.size( ) - 1 ) == "0" )
        ui->chatfield->append( formattedTime + "<font color = " + gNormalMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
    else if( message.mid( message.size( ) - 1 ) == "1" )
    {
        ui->chatfield->append( formattedTime + "<font color = " + gPreSentMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
        m_ChatLinesToDelete.push_back( ui->chatfield->document( )->blockCount( ) - 1 );
        m_ChatLinesToDeleteTime.push_back( GetTime( ) );
        m_ChatLinesToDeleteText.push_back( msg );
    }
    else if( message.mid( message.size( ) - 1 ) == "2" )
        ui->chatfield->append( formattedTime + "<font color = " + gPrivateMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
    else if( message.mid( message.size( ) - 1 ) == "7" )
        ui->chatfield->append( "<font color = " + gPrivateMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
    else if( message.mid( message.size( ) - 1 ) == "8" )
        ui->chatfield->append( formattedTime + "<font color = " + gPreSentMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
    else if( message.mid( message.size( ) - 1 ) == "3" )
        ui->chatfield->append( formattedTime + "<font color = " + gNotifyMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
    else if( message.mid( message.size( ) - 1 ) == "4" )
        ui->chatfield->append( formattedTime + "<font color = " + gWarningMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
    else
    {
        bool removed = false;

        for( uint32_t i = 0; i != m_ChatLinesToDelete.size( ); )
        {
            if( removed )
            {
                m_ChatLinesToDelete[i]--;
                i++;
            }
            else if( m_ChatLinesToDeleteText[i] == msg )
            {
                QTextCursor cursor( ui->chatfield->document( )->findBlockByNumber( m_ChatLinesToDelete[i] ) );
                cursor.select( QTextCursor::BlockUnderCursor );
                cursor.removeSelectedText( );
                m_ChatLinesToDelete.erase( m_ChatLinesToDelete.begin( ) + i );
                m_ChatLinesToDeleteTime.erase( m_ChatLinesToDeleteTime.begin( ) + i );
                m_ChatLinesToDeleteText.erase( m_ChatLinesToDeleteText.begin( ) + i );
                removed = true;
            }
            else
                i++;
        }

        if( message.mid( message.size( ) - 1 ) == "5" )
            ui->chatfield->append( formattedTime + "<font color = " + gNormalMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
        else
            ui->chatfield->append( formattedTime + "<font color = " + gPrivateMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
    }
}

void ChatWidget::deletingOldMessages( )
{
    while( !m_ChatLinesToDeleteTime.empty( ) && GetTime( ) >= m_ChatLinesToDeleteTime[0] + 300 )
    {
        m_ChatLinesToDelete.erase( m_ChatLinesToDelete.begin( ) );
        m_ChatLinesToDeleteTime.erase( m_ChatLinesToDeleteTime.begin( ) );
        m_ChatLinesToDeleteText.erase( m_ChatLinesToDeleteText.begin( ) );
    }
}

void ChatWidget::sending_messages_to_battlenet( )
{
    if( gUNM && gUNM != nullptr && gUNM->m_BNETs.size( ) > m_BnetID )
    {
        string Payload = ui->entryfield->text( ).toStdString( );

        if( Payload.empty( ) )
        {
            ui->entryfield->clear( );
            return;
        }

        string PayloadLower = Payload;
        transform( PayloadLower.begin( ), PayloadLower.end( ), PayloadLower.begin( ), static_cast<int(*)(int)>(tolower) );

        if( gUNM->m_BNETs[m_BnetID]->GetLoggedIn( ) )
        {
            if( ( PayloadLower.size( ) >= 7 && PayloadLower.substr( 0, 7 ) == "/where " ) || PayloadLower == "/where" )
                Payload = "/whois" + Payload.substr( 6 );
            else if( ( PayloadLower.size( ) >= 9 && PayloadLower.substr( 0, 9 ) == "/whereis " ) || PayloadLower == "/whereis" )
                Payload = "/whois" + Payload.substr( 8 );

            if( ui->entryfield->text( ).at( 0 ) != m_BnetCommandTrigger )
            {
                if( gUNM->IsIccupServer( gUNM->m_BNETs[m_BnetID]->GetLowerServer( ) ) )
                {
                    if( PayloadLower == "/flw" || PayloadLower == "/follow" )
                    {
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandWhisperQueue.push_back( true );
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandUserQueue.push_back( string( ) );
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandQueue.push_back( "/watch" );
                    }
                    else if( PayloadLower.size( ) >= 5 && PayloadLower.substr( 0, 5 ) == "/flw " )
                    {
                        string::size_type NameStart = string::npos;

                        if( PayloadLower.size( ) > 5 )
                            NameStart = PayloadLower.substr( 5 ).find_first_not_of( " " );

                        if( NameStart != string::npos )
                            gUNM->m_BNETs[m_BnetID]->m_QueueSearchBotPre.push_back( Payload.substr( 5 + NameStart ) );
                        else
                        {
                            gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandWhisperQueue.push_back( true );
                            gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandUserQueue.push_back( string( ) );
                            gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandQueue.push_back( "/watch " + Payload.substr( 5 ) );
                        }
                    }
                    else if( ( PayloadLower.size( ) >= 8 && PayloadLower.substr( 0, 8 ) == "/follow " ) )
                    {
                        string::size_type NameStart = string::npos;

                        if( PayloadLower.size( ) > 8 )
                            NameStart = PayloadLower.substr( 8 ).find_first_not_of( " " );

                        if( NameStart != string::npos )
                            gUNM->m_BNETs[m_BnetID]->m_QueueSearchBotPre.push_back( Payload.substr( 8 + NameStart ) );
                        else
                        {
                            gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandWhisperQueue.push_back( true );
                            gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandUserQueue.push_back( string( ) );
                            gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandQueue.push_back( "/watch " + Payload.substr( 8 ) );
                        }
                    }
                    else
                    {
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandWhisperQueue.push_back( true );
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandUserQueue.push_back( string( ) );
                        gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandQueue.push_back( Payload );
                    }
                }
                else
                {
                    gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandWhisperQueue.push_back( true );
                    gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandUserQueue.push_back( string( ) );
                    gUNM->m_BNETs[m_BnetID]->m_QueueChatCommandQueue.push_back( Payload );
                }
            }
        }

        ui->entryfield->clear( );

        if( PayloadLower.size( ) > gUNM->m_BNETs[m_BnetID]->GetLowerName( ).size( ) + 5 && PayloadLower.substr( 0, 4 + gUNM->m_BNETs[m_BnetID]->GetLowerName( ).size( ) ) == "/w " + gUNM->m_BNETs[m_BnetID]->GetLowerName( ) + " " && PayloadLower.substr( 4 + gUNM->m_BNETs[m_BnetID]->GetLowerName( ).size( ) ).find_first_not_of( " " ) != string::npos )
            gUNM->m_BNETs[m_BnetID]->m_UNMChatCommandQueue.push_back( Payload.substr( 4 + gUNM->m_BNETs[m_BnetID]->GetLowerName( ).size( ) ) );
        else
            gUNM->m_BNETs[m_BnetID]->m_UNMChatCommandQueue.push_back( Payload );
    }
    else
        ui->chatfield->append( "<font color = " + gWarningMessagesColor + ">Что-то пошло не так, требуется перезапуск." );
}

void ChatWidget::on_mapChooseButton_clicked( )
{
    if( gUNM && gUNM != nullptr && gUNM->m_BNETs.size( ) > m_BnetID )
    {
        ui->mapChooseButton->setEnabled( false );
        ui->mapChooseFindButton->setEnabled( false );
        ui->mapUploadButton->setEnabled( false );
        ui->createGameButton->setEnabled( false );
        ui->createGameInfoLabel->setText( "Ждем ответ от бота..." );
        m_CurrentBotStatus = 3;
        gUNM->m_BNETs[m_BnetID]->AddCommandOnBnetBot( ui->bnetBotsCombo->currentText( ).toStdString( ), "map " + ui->mapChooseLine->text( ).toStdString( ), 2 );
    }
    else
    {
        ui->createGameInfoLabel->setText( "Что-то пошло не так..." );
        ui->mapChooseButton->setEnabled( false );
        ui->mapChooseFindButton->setEnabled( false );
        ui->mapUploadButton->setEnabled( false );
        ui->createGameButton->setEnabled( false );
    }
}

void ChatWidget::on_mapChooseLine_returnPressed( )
{
    if( m_CurrentBotStatus == 4 )
        on_mapChooseButton_clicked( );
}

void ChatWidget::on_mapUploadButton_clicked( )
{
    if( gUNM && gUNM != nullptr && gUNM->m_BNETs.size( ) > m_BnetID )
    {
        string url = UTIL_UrlFix( ui->mapChooseLine->text( ).toStdString( ) );

        if( url.find( "." ) == string::npos || url.find( ".." ) != string::npos || url.size( ) <= 3 || url.back( ) == '.' || url.front( ) == '.' )
            ui->createGameInfoLabel->setText( "Ссылка некорректна!" );
        else
        {
            transform( url.begin( ), url.end( ), url.begin( ), static_cast<int(*)(int)>(tolower) );
            ui->mapChooseButton->setEnabled( false );
            ui->mapChooseFindButton->setEnabled( false );
            ui->mapUploadButton->setEnabled( false );
            ui->createGameButton->setEnabled( false );
            ui->createGameInfoLabel->setText( "Ждем ответ от бота..." );
            m_CurrentBotStatus = 3;
            gUNM->m_BNETs[m_BnetID]->AddCommandOnBnetBot( ui->bnetBotsCombo->currentText( ).toStdString( ), "dm " + url, 4 );
        }
    }
    else
    {
        ui->createGameInfoLabel->setText( "Что-то пошло не так..." );
        ui->mapChooseButton->setEnabled( false );
        ui->mapChooseFindButton->setEnabled( false );
        ui->mapUploadButton->setEnabled( false );
        ui->createGameButton->setEnabled( false );
    }
}

void ChatWidget::on_mapUploadLine_returnPressed( )
{
    if( m_CurrentBotStatus == 4 )
        on_mapUploadButton_clicked( );
}

void ChatWidget::on_botWebSiteButton_clicked( )
{
    QDesktopServices::openUrl( QUrl( m_CurrentBotWebSite ) );
}

void ChatWidget::on_botVkGroupButton_clicked( )
{
    QDesktopServices::openUrl( QUrl( m_CurrentBotVK ) );
}

void ChatWidget::on_createGameButton_clicked( )
{
    if( m_CurrentBotMap.isEmpty( ) )
    {
        ui->createGameInfoLabel->setText( "Сначала выберите карту!" );
        ui->createGameButton->setEnabled( false );
    }
    else if( gUNM && gUNM != nullptr && gUNM->m_BNETs.size( ) > m_BnetID )
    {
        if( ui->GameNameLine->text( ).isEmpty( ) )
            ui->createGameInfoLabel->setText( "Укажите название игры!" );
        else if( ui->GameNameLine->text( ).size( ) < 2 )
            ui->createGameInfoLabel->setText( "Название игры должно быть не короче двух знаков!" );
        else
        {
            string command;

            if( ui->gameTypeCombo->currentText( ) == "Private" )
                command = "priv";
            else if( ui->gameTypeCombo->currentText( ) == "Inhouse" )
                command = "pri";
            else
                command = "pub";

            if( ui->pubByCheckBox->isChecked( ) )
                command += "by";

            command += " " + ui->GameNameLine->text( ).toStdString( );
            ui->mapChooseButton->setEnabled( false );
            ui->mapChooseFindButton->setEnabled( false );
            ui->mapUploadButton->setEnabled( false );
            ui->createGameButton->setEnabled( false );
            ui->createGameInfoLabel->setText( "Ждем ответ от бота..." );
            m_CurrentBotStatus = 3;
            gUNM->m_BNETs[m_BnetID]->AddCommandOnBnetBot( ui->bnetBotsCombo->currentText( ).toStdString( ), command, 3 );
        }
    }
    else
    {
        ui->createGameInfoLabel->setText( "Что-то пошло не так..." );
        ui->mapChooseButton->setEnabled( false );
        ui->mapChooseFindButton->setEnabled( false );
        ui->mapUploadButton->setEnabled( false );
        ui->createGameButton->setEnabled( false );
    }
}

void ChatWidget::on_mapListWidget_itemClicked( QListWidgetItem *item )
{
    if( item != nullptr )
        gUNM->m_BNETs[m_BnetID]->AddCommandOnBnetBot( ui->bnetBotsCombo->currentText( ).toStdString( ), "map " + item->text( ).toStdString( ), 1 );

    m_CurrentBotMap = item->text( );
}

void ChatWidget::on_bnetBotsCombo_currentTextChanged( const QString &arg1 )
{
    qDebug( ) << "VOT";
    uint32_t botStatus = 0;
    bool canMap = false;
    bool supMap = false;
    bool canPub = false;
    bool canPrivate = false;
    bool canInhouse = false;
    bool canPubBy = false;
    bool canDM = false;
    bool supDM = false;
    string statusString = string( );
    string description = string( );
    string accessName = string( );
    string site = string( );
    string vk = string( );
    string map = string( );
    string botLogin = string( );
    vector<string> maps;

    if( gUNM && gUNM != nullptr && gUNM->m_BNETs.size( ) > m_BnetID )
        gUNM->m_BNETs[m_BnetID]->GetBnetBot( arg1.toStdString( ), botStatus, canMap, supMap, canPub, canPrivate, canInhouse, canPubBy, canDM, supDM, statusString, description, accessName, site, vk, map, botLogin, maps );

    if( botStatus == 0 && ui->bnetBotsCombo->count( ) != 0 )
    {
        ui->bnetBotsCombo->removeItem( ui->bnetBotsCombo->currentIndex( ) );
        return;
    }

    m_CurrentBotStatus = botStatus;

    // pub by other user

    ui->pubByLine->clear( );
    ui->pubByCheckBox->setChecked( false );

    if( canPubBy )
    {
        ui->pubByCheckBox->setEnabled( true );
        ui->pubByLine->setEnabled( true );
    }
    else
    {
        ui->pubByCheckBox->setEnabled( false );
        ui->pubByLine->setEnabled( false );
    }

    // gametype

    ui->gameTypeCombo->clear( );

    if( canPub || canPrivate || canInhouse )
    {
        ui->gameOptionsBox->setEnabled( true );
        ui->gameTypeLabel->setEnabled( true );
        ui->gameTypeCombo->setEnabled( true );

        if( canPub )
            ui->gameTypeCombo->addItem( "Public" );

        if( canPrivate )
            ui->gameTypeCombo->addItem( "Private" );

        if( canInhouse )
            ui->gameTypeCombo->addItem( "Inhouse" );
    }
    else
    {
        ui->gameTypeLabel->setEnabled( false );
        ui->gameTypeCombo->setEnabled( false );

        if( !canPubBy )
            ui->gameOptionsBox->setEnabled( false );
        else
            ui->gameOptionsBox->setEnabled( true );
    }

    // upload map

    ui->mapUploadLine->clear( );

    if( canDM )
    {
        ui->mapUploadLabel->setEnabled( true );
        ui->mapUploadLine->setEnabled( true );
        ui->mapUploadButton->setEnabled( true );
    }
    else
    {
        ui->mapUploadLabel->setEnabled( false );
        ui->mapUploadLine->setEnabled( false );
        ui->mapUploadButton->setEnabled( false );
    }

    // help

    QString helpString = "<html><head/><body><p>";

    if( canMap )
        helpString += "Используйте <a href=\"1\"><span style=\" text-decoration: underline; color:#007af4;\">поиск</span></a> <img src=\":/mainwindow/up_arrow_orange.png\" width=\"9\" height=\"16\"/> чтобы найти нужную вам карту. В ответ на запрос вы получите <a href=\"2\"><span style=\" text-decoration: underline; color:#007af4;\">список релевантных карт</span></a> <img src=\":/mainwindow/right_arrow_orange.png\" width=\"16\" height=\"9\"/><br>Затем выберите из списка карту, которую хотите создать.<br>";
    else if( supMap )
    {
        if( maps.size( ) > 1 )
            helpString += "У вас недостаточно прав для поиска карт на этом боте.<br>";
        else
            helpString += "У вас недостаточно прав для выбора карт на этом боте.<br>";
    }
    else if( maps.size( ) > 1 )
        helpString += "Этот бот не поддерживает поиск карт.<br>";
    else
        helpString += "Этот бот не поддерживает выбор карт.<br>";

    if( canDM )
        helpString += "Если нужная вам карта отсутствует, вы можете <a href=\"3\"><span style=\" text-decoration: underline; color:#007af4;\">загрузить</span></a> <img src=\":/mainwindow/down_arrow_orange.png\" width=\"9\" height=\"16\"/> её на бот по прямой ссылке, используя, например, сайт <a href=\"https://www.epicwar.com/maps/\"><span style=\" text-decoration: underline; color:#007af4;\">epicwar.com</span></a>";
    else if( supDM )
        helpString += "У вас недостаточно прав для загрузки карт на этот бот.";
    else
        helpString += "Этот бот не поддерживает загрузку карт.";

    if( ui->gameTypeCombo->count( ) > 1 )
    {
        if( canPubBy )
            helpString += "<br>Также есть возможность выбрать тип игры или в качестве владельца игры указать ник друга.</p></body></html>";
        else
            helpString += "<br>Также есть возможность выбрать тип игры.</p></body></html>";
    }
    else if( canPubBy )
        helpString += "<br>Также есть возможность в качестве владельца игры указать ник друга.</p></body></html>";
    else
        helpString += "</p></body></html>";

    ui->createGameHelpLabel->setText( helpString );

    // description

    ui->botDescriptionTextEdit->clear( );

    if( !statusString.empty( ) )
        ui->botDescriptionTextEdit->appendPlainText( QString::fromUtf8( statusString.c_str( ) ) );

    if( !description.empty( ) )
        ui->botDescriptionTextEdit->appendPlainText( QString::fromUtf8( description.c_str( ) ) );

    if( !accessName.empty( ) )
        ui->botDescriptionTextEdit->appendPlainText( QString::fromUtf8( accessName.c_str( ) ) );

    if( !site.empty( ) )
    {
        m_CurrentBotWebSite = QString::fromUtf8( site.c_str( ) );

        if( vk.empty( ) )
            ui->botDescriptionTextEdit->appendPlainText( "Сайт бота: " + m_CurrentBotWebSite );

        ui->botVkGroupButton->show( );
    }
    else
    {
        ui->botWebSiteButton->hide( );
        m_CurrentBotWebSite.clear( );
    }

    if( !vk.empty( ) )
    {
        m_CurrentBotVK = QString::fromUtf8( vk.c_str( ) );

        if( !site.empty( ) )
            ui->botDescriptionTextEdit->appendPlainText( "Сайт бота: " + m_CurrentBotWebSite + " Вк бота: " + m_CurrentBotVK );
        else
            ui->botDescriptionTextEdit->appendPlainText( "Вк бота: " + m_CurrentBotVK );

        ui->botVkGroupButton->show( );
    }
    else
    {
        ui->botVkGroupButton->hide( );
        m_CurrentBotVK.clear( );
    }

    // map

    if( canMap )
    {
        ui->mapChooseLabel->setEnabled( true );
        ui->mapChooseLine->setEnabled( true );
        ui->mapChooseButton->setEnabled( true );
        ui->mapChooseFindButton->setEnabled( true );
    }
    else
    {
        ui->mapChooseLabel->setEnabled( false );
        ui->mapChooseLine->setEnabled( false );
        ui->mapChooseButton->setEnabled( false );
        ui->mapChooseFindButton->setEnabled( false );
    }

    ui->mapChooseLine->clear( );
    ui->mapListWidget->clear( );
    m_CurrentBotMap = QString::fromUtf8( map.c_str( ) );

    for( uint32_t i = 0; i < maps.size( ); i++ )
        ui->mapListWidget->addItem( QString::fromUtf8( maps[i].c_str( ) ) );

    if( m_CurrentBotStatus != 4 )
    {
        ui->mapChooseButton->setEnabled( false );
        ui->mapChooseFindButton->setEnabled( false );
        ui->mapUploadButton->setEnabled( false );
        ui->createGameButton->setEnabled( false );

        if( m_CurrentBotStatus == 1 )
            ui->createGameInfoLabel->setText( "Бот не инициализирован!" );
        else if( m_CurrentBotStatus == 2 )
            ui->createGameInfoLabel->setText( "Бот офлайн!" );
        else if( m_CurrentBotStatus == 3 )
            ui->createGameInfoLabel->setText( "Ждем ответ от бота..." );
    }
    else if( !canPub && !canPrivate && !canInhouse )
    {
        ui->createGameButton->setEnabled( false );
        ui->createGameInfoLabel->setText( "Недостаточно прав для создания игр на этом боте!" );
    }
    else
    {
        ui->createGameButton->setEnabled( true );

        if( m_CurrentBotMap.isEmpty( ) )
            ui->createGameInfoLabel->setText( "Карта не выбрана!" );
        else
            ui->createGameInfoLabel->setText( "Выбранная карта: " + m_CurrentBotMap );
    }
}

void ChatWidget::on_mapChooseFindButton_clicked( )
{
    QString selfilter = tr( "War3 карты (*.w3m *.w3x)" );
    QString directory = QString( );

    if( gUNM && gUNM != nullptr )
    {
        if( !gUNM->m_MapPath.empty( ) && std::filesystem::exists( gUNM->m_MapPath ) )
            directory = QString::fromUtf8( gUNM->m_MapPath.c_str( ) );
        else if( !gUNM->m_Warcraft3Path.empty( ) && std::filesystem::exists( gUNM->m_Warcraft3Path ) )
            directory = QString::fromUtf8( gUNM->m_Warcraft3Path.c_str( ) );
        else
            directory = QDir::currentPath( );
    }
    else
        directory = QDir::currentPath( );

    QString fileName = QFileDialog::getOpenFileName( this, tr( "Выберите карту" ), directory, tr( "Все файлы (*.*);;War3 карты (*.w3m *.w3x);;War3RoC карты (*.w3m);;War3TFT карты (*.w3x)" ), &selfilter );

    if( !fileName.isEmpty( ) )
    {
        ui->mapChooseLine->setText( UTIL_GetFileNameFromPath( fileName ) );
        on_mapChooseButton_clicked( );
    }
}

void ChatWidget::on_createGameHelpLabel_linkActivated( const QString &link )
{
    if( link == "1" )
        QTimer::singleShot( 0, ui->mapChooseLine, SLOT( setFocus( ) ) );
    else if( link == "2" )
        QTimer::singleShot( 0, ui->mapListWidget, SLOT( setFocus( ) ) );
    else if( link == "3" )
        QTimer::singleShot( 0, ui->mapUploadLine, SLOT( setFocus( ) ) );
    else
        QDesktopServices::openUrl( QUrl( link ) );
}

void ChatWidget::add_bnet_bot_direct( QString botName )
{
    ui->bnetBotsCombo->addItem( botName );

    if( ui->bnetBotsCombo->count( ) == 2 && ui->bnetBotsCombo->itemText( 0 ) == "А ботов нет" )
        ui->bnetBotsCombo->removeItem( 0 );

    ui->bnetBotsCombo->setCurrentIndex( ui->bnetBotsCombo->count( ) - 1 );
}

void ChatWidget::update_bnet_bot_direct( QString botName, unsigned int type )
{
    // type:
    // 0 = all
    // 1 = set current map
    // 2 = add maps
    // 3 = map not found
    // 4 = dm success
    // 5 = dm failure
    // 6 = host success
    // 7 = host failure
    // 7 = ответ от бота не пришел

    if( ui->bnetBotsCombo->currentText( ) == botName )
    {
        if( type == 0 || type > 7 )
            on_bnetBotsCombo_currentTextChanged( botName );
        else if( type == 7 )
        {
            on_bnetBotsCombo_currentTextChanged( botName );
            ui->createGameInfoLabel->setText( "Ответ от бота не пришел!" );
        }
        else
        {
            on_bnetBotsCombo_currentTextChanged( botName );
//            ui->mapChooseButton->setEnabled( ui->mapChooseLabel->isEnabled( ) );
//            ui->mapChooseFindButton->setEnabled( ui->mapChooseLabel->isEnabled( ) );
//            ui->mapUploadButton->setEnabled( ui->mapUploadLabel->isEnabled( ) );
//            ui->createGameButton->setEnabled( ( ui->gameTypeCombo->count( ) > 0 ) );
        }

    }
}

void ChatWidget::delete_bnet_bot_direct( QString botName )
{
    int index = ui->bnetBotsCombo->findData( botName );

    if ( index != -1 )
        ui->bnetBotsCombo->removeItem( index );
}

void ChatWidget::add_game_to_list_direct( unsigned int gameID, bool gproxy, unsigned int gameIcon, QStringList gameInfo )
{
    ui->GameList->setSortingEnabled( false );

    if( !gproxy )
    {
        ui->GameList->insertRow( ui->GameList->rowCount( ) );

        for( int i = 0; i < ui->GameList->columnCount( ); i++ )
        {
            if( !ui->GameList->item( ui->GameList->rowCount( ) - 1, i ) )
                ui->GameList->setItem( ui->GameList->rowCount( ) - 1, i, new CustomTableWidgetItem( false ) );

            ui->GameList->item( ui->GameList->rowCount( ) - 1, i )->setText( gameInfo[i] );
            ui->GameList->item( ui->GameList->rowCount( ) - 1, i )->setData( gGameRoleGProxy, gproxy );
            ui->GameList->item( ui->GameList->rowCount( ) - 1, i )->setData( gGameRoleID, gameID );
        }

        ui->GameList->item( ui->GameList->rowCount( ) - 1, 2 )->setData( gGameRoleHost, gameInfo[3] );
        ui->GameList->item( ui->GameList->rowCount( ) - 1, 2 )->setData( gGameRoleIcon, gameIcon );

        if( gameIcon > 0 )
            ui->GameList->item( ui->GameList->rowCount( ) - 1, 2 )->setIcon( m_Icons[gameIcon-1] );
    }
    else
    {
        ui->GameList->insertRow( 0 );

        for( int i = 0; i < ui->GameList->columnCount( ); i++ )
        {
            if( !ui->GameList->item( 0, i ) )
                ui->GameList->setItem( 0, i, new CustomTableWidgetItem( false ) );

            ui->GameList->item( 0, i )->setText( gameInfo[i] );
            ui->GameList->item( 0, i )->setData( gGameRoleGProxy, gproxy );
            ui->GameList->item( 0, i )->setData( gGameRoleID, gameID );
        }

        ui->GameList->item( 0, 2 )->setData( gGameRoleHost, gameInfo[3] );
        ui->GameList->item( 0, 2 )->setData( gGameRoleIcon, gameIcon );

        if( gameIcon > 0 )
            ui->GameList->item( 0, 2 )->setIcon( m_Icons[gameIcon-1] );
    }
}

void ChatWidget::update_game_to_list_direct( unsigned int gameID, QStringList gameInfo )
{
    ui->GameList->setSortingEnabled( false );

    // изменяем старую строку

    for( int i = 0; i < ui->GameList->rowCount( ); i++ )
    {
        if( ui->GameList->item( i, 0 )->data( gGameRoleID ).toUInt( ) == gameID )
        {
            for( int j = 0; j < ui->GameList->columnCount( ); j++ )
                ui->GameList->item( i, j )->setText( gameInfo[j] );

            ui->GameList->item( i, 2 )->setData( gGameRoleHost, gameInfo[3] );
            break;
        }
    }
}

void ChatWidget::delete_game_from_list_direct( unsigned int gameID )
{
    ui->GameList->setSortingEnabled( false );

    // удаляем старую строку

    for( int i = 0; i < ui->GameList->rowCount( ); i++ )
    {
        if( ui->GameList->item( i, 0 )->data( gGameRoleID ).toUInt( ) == gameID )
        {
            ui->GameList->removeRow( i );
            break;
        }
    }
}

ChatWidgetIrina::ChatWidgetIrina( Widget *parent ) :
    m_WebSocket( new QWebSocket( ) ),
    m_MainWidget( parent ),
    ui( new Ui::ChatWidgetIrina ),
    m_ServerName( "IrinaBot.ru" ),
    m_ConnectionStatus( 0 ),
    irinaOAuth2( nullptr ),
    irinaReplyHandler( nullptr ),
    m_CurrentGameID( 0 ),
    m_Icons( { QIcon( ":/mainwindow/irina/gamelist/game_irina_red.png" ), QIcon( ":/mainwindow/irina/gamelist/game_irina_blue.png" ), QIcon( ":/mainwindow/irina/gamelist/game_irina_ah_red.png" ), QIcon( ":/mainwindow/irina/gamelist/game_irina_ah_blue.png" ), QIcon( ":/mainwindow/irina/gamelist/game_irina_red_iccup.png" ), QIcon( ":/mainwindow/irina/gamelist/game_irina_blue_iccup.png" ), QIcon( ":/mainwindow/irina/gamelist/game_irina_ah_red_iccup.png" ), QIcon( ":/mainwindow/irina/gamelist/game_irina_ah_blue_iccup.png" ), QIcon( ":/mainwindow/irina/gamelist/game_other_bot.png" ) } )
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

    ui->setupUi( this );
    QList<int> sizes;
    sizes << ui->splitter->width( ) << 0;
    ui->splitter->setSizes( sizes );
    CustomStyledItemDelegateIrina *delegate = new CustomStyledItemDelegateIrina( ui->GameList, ui->GameList );
    ui->GameList->setItemDelegate( delegate );
    connect( ui->GameList, &QTableWidget::cellClicked, this, &ChatWidgetIrina::cellClickedCustom );
    connect( ui->GameList, &QTableWidget::cellDoubleClicked, this, &ChatWidgetIrina::cellDoubleClickedCustom );
    connect( m_WebSocket, &QWebSocket::connected, this, &ChatWidgetIrina::onConnected );
    connect( m_WebSocket, &QWebSocket::disconnected, this, &ChatWidgetIrina::onDisconnected );
    connect( m_WebSocket, &QWebSocket::binaryMessageReceived, this, &ChatWidgetIrina::onBytes );
    connect( m_WebSocket, &QWebSocket::textMessageReceived, this, &ChatWidgetIrina::onMessage );
    connect( gToLog, &ExampleObject::openConnect, this, &ChatWidgetIrina::open_connect );
    connect( gToLog, &ExampleObject::closeConnect, this, &ChatWidgetIrina::close_connect );
    connect( gToLog, &ExampleObject::sendLogin, this, &ChatWidgetIrina::send_login );
    connect( gToLog, &ExampleObject::badLogon, this, &ChatWidgetIrina::bad_logon );
    connect( gToLog, &ExampleObject::gameListTablePosFixIrina, this, &ChatWidgetIrina::game_list_table_pos_fix_irina );
    connect( gToLog, &ExampleObject::successLogon, this, &ChatWidgetIrina::success_logon );
    connect( gToLog, &ExampleObject::sendPing, this, &ChatWidgetIrina::send_ping );
    connect( gToLog, &ExampleObject::sendGamelist, this, &ChatWidgetIrina::send_gamelist );
    connect( gToLog, &ExampleObject::successInit, this, &ChatWidgetIrina::success_init );
    connect( gToLog, &ExampleObject::addGameToListIrina, this, &ChatWidgetIrina::add_game_to_list_irina );
    connect( gToLog, &ExampleObject::updateGameToListIrina, this, &ChatWidgetIrina::update_game_to_list_irina );
    connect( gToLog, &ExampleObject::deleteGameFromListIrina, this, &ChatWidgetIrina::delete_game_from_list_irina );
    connect( gToLog, &ExampleObject::sendGetgame, this, &ChatWidgetIrina::send_getgame );
    connect( gToLog, &ExampleObject::setSelectGameData, this, &ChatWidgetIrina::set_select_game_data );
    connect( gToLog, &ExampleObject::setSelectGameDataBadTga, this, &ChatWidgetIrina::set_select_game_data_bad_tga );
    connect( gToLog, &ExampleObject::authButtonReset, this, &ChatWidgetIrina::auth_button_reset );
    ui->GameList->setColumnWidth( 0, 165 );
    ui->GameList->setColumnWidth( 1, 45 );
    ui->scrollArea->hide( );
    ui->GameList->setIconSize( QSize( 32, 16 ) );

    if( !gOAuthDiscordToken.isEmpty( ) )
    {
        ui->authButton->setText( "Авторизация..." );
        ui->authButton->setEnabled( false );
    }
    else
        ui->authButton->setText( "Авторизироваться (Discord)" );

    /*
     0 = Ждём...
     1 = Подключение...
     2 = Подключено, инициализация...
     3 = Инициализация завершена/Ожидание авторизации...
     4 = Авторизация...
     5 = Ошибка авторизации
     6 = Дисконнект
     7 = Дисконнект, ожидание переподключения...
     8 = Авторизация пройдена
    */
}

void ChatWidgetIrina::game_list_table_pos_fix_irina( )
{
    QApplication::sendEvent( ui->GameList->viewport( ), new QMouseEvent( QEvent::MouseMove, ui->GameList->viewport( )->mapFromGlobal( QCursor::pos( ) ), Qt::NoButton,  Qt::NoButton, Qt::NoModifier ) );
    ui->GameList->setSortingEnabled( true );
}

void ChatWidgetIrina::add_game_to_list_irina( unsigned int gameID, bool gameStarted, unsigned int gameIcon, QStringList gameInfo )
{
    ui->GameList->setSortingEnabled( false );

    if( gameStarted )
    {
        ui->GameList->insertRow( ui->GameList->rowCount( ) );

        for( int i = 0; i < ui->GameList->columnCount( ); i++ )
        {
            if( !ui->GameList->item( ui->GameList->rowCount( ) - 1, i ) )
                ui->GameList->setItem( ui->GameList->rowCount( ) - 1, i, new CustomTableWidgetItem( true ) );

            ui->GameList->item( ui->GameList->rowCount( ) - 1, i )->setText( gameInfo[i] );
            ui->GameList->item( ui->GameList->rowCount( ) - 1, i )->setData( gGameRoleStarted, gameStarted );
            ui->GameList->item( ui->GameList->rowCount( ) - 1, i )->setData( gGameRoleID, gameID );
        }

        ui->GameList->item( ui->GameList->rowCount( ) - 1, 2 )->setToolTip( gameInfo[3] );
        ui->GameList->item( ui->GameList->rowCount( ) - 1, 2 )->setData( gGameRolePlayers, gameInfo[4] );
        ui->GameList->item( ui->GameList->rowCount( ) - 1, 2 )->setData( gGameRoleIcon, gameIcon );
        ui->GameList->item( ui->GameList->rowCount( ) - 1, 2 )->setIcon( m_Icons[gameIcon] );
    }
    else
    {
        ui->GameList->insertRow( 0 );

        for( int i = 0; i < ui->GameList->columnCount( ); i++ )
        {
            if( !ui->GameList->item( 0, i ) )
                ui->GameList->setItem( 0, i, new CustomTableWidgetItem( true ) );

            ui->GameList->item( 0, i )->setText( gameInfo[i] );
            ui->GameList->item( 0, i )->setData( gGameRoleStarted, gameStarted );
            ui->GameList->item( 0, i )->setData( gGameRoleID, gameID );
        }

        ui->GameList->item( 0, 2 )->setToolTip( gameInfo[3] );
        ui->GameList->item( 0, 2 )->setData( gGameRolePlayers, gameInfo[4] );
        ui->GameList->item( 0, 2 )->setData( gGameRoleIcon, gameIcon );
        ui->GameList->item( 0, 2 )->setIcon( m_Icons[gameIcon] );
    }
}

void ChatWidgetIrina::update_game_to_list_irina( unsigned int gameID, bool gameStarted, QStringList gameInfo, bool deleteFromCurrentGames )
{
    ui->GameList->setSortingEnabled( false );

    // изменяем старую строку

    for( int i = 0; i < ui->GameList->rowCount( ); i++ )
    {
        if( ui->GameList->item( i, 0 )->data( gGameRoleID ).toUInt( ) == gameID )
        {
            if( !ui->GameList->item( i, 0 )->data( gGameRoleStarted ).toBool( ) && gameStarted )
            {
                // если игра была в списке как неначатая, а теперь началась - значит перемещаем эту игру на самый низ (добавляем новую строку и удалям прошлую запись)

                ui->GameList->insertRow( ui->GameList->rowCount( ) );

                for( int j = 0; j < ui->GameList->columnCount( ); j++ )
                {
                    if( !ui->GameList->item( ui->GameList->rowCount( ) - 1, j ) )
                        ui->GameList->setItem( ui->GameList->rowCount( ) - 1, j, new CustomTableWidgetItem( true ) );

                    ui->GameList->item( ui->GameList->rowCount( ) - 1, j )->setText( gameInfo[j] );
                    ui->GameList->item( ui->GameList->rowCount( ) - 1, j )->setData( gGameRoleStarted, 1 );
                    ui->GameList->item( ui->GameList->rowCount( ) - 1, j )->setData( gGameRoleID, gameID );
                }

                ui->GameList->item( ui->GameList->rowCount( ) - 1, 2 )->setToolTip( gameInfo[3] );
                ui->GameList->item( ui->GameList->rowCount( ) - 1, 2 )->setData( gGameRolePlayers, gameInfo[4] );
                ui->GameList->item( ui->GameList->rowCount( ) - 1, 2 )->setData( gGameRoleIcon, ui->GameList->item( i, 2 )->data( gGameRoleIcon ).toUInt( ) );
                ui->GameList->item( ui->GameList->rowCount( ) - 1, 2 )->setIcon( m_Icons[ui->GameList->item( i, 2 )->data( gGameRoleIcon ).toUInt( )] );
                ui->GameList->removeRow( i );
            }
            else
            {
                for( int j = 0; j < ui->GameList->columnCount( ); j++ )
                {
                    ui->GameList->item( i, j )->setText( gameInfo[j] );
                    ui->GameList->item( i, j )->setData( gGameRoleStarted, gameStarted );
                }

                ui->GameList->item( i, 2 )->setToolTip( gameInfo[3] );
                ui->GameList->item( i, 2 )->setData( gGameRolePlayers, gameInfo[4] );
            }

            break;
        }
    }

    if( deleteFromCurrentGames )
        m_MainWidget->delete_game( gameID );
}

void ChatWidgetIrina::delete_game_from_list_irina( unsigned int gameID, unsigned int type )
{
    ui->GameList->setSortingEnabled( false );

    // type 0 = delete from gamelist and from currentgamelist
    // type 1 = delete only from gamelist
    // type 2 = delete only from currentgamelist

    if( type != 2 )
    {
        // удаляем старую строку

        for( int i = 0; i < ui->GameList->rowCount( ); i++ )
        {
            if( ui->GameList->item( i, 0 )->data( gGameRoleID ).toUInt( ) == gameID )
            {
                ui->GameList->removeRow( i );
                break;
            }
        }

    }

    if( type != 1 )
        m_MainWidget->delete_game( gameID );
}

void ChatWidgetIrina::cellClickedCustom( int iRow, int )
{
    if( ui->scrollArea->isHidden( ) )
        ui->scrollArea->show( );

    ui->mapName->setText( "Секундочку..." );
    ui->mapPlayers->clear( );
    ui->mapAuthor->clear( );
    ui->mapDescription->clear( );
    ui->mapImage->clear( );

    if( !gUNM )
        return;

    if( ui->GameList->item( iRow, 0 ) )
    {
        m_CurrentGameID = ui->GameList->item( iRow, 0 )->data( gGameRoleID ).toUInt( );
        BYTEARRAY request;
        request.push_back( 2 );
        request.push_back( 4 );
        UTIL_AppendByteArray( request, m_CurrentGameID, false );
        gUNM->m_IrinaServer->m_RequestQueue.push_back( request );
    }
}

void ChatWidgetIrina::cellDoubleClickedCustom( int iRow, int )
{
    if( m_ConnectionStatus != 8 )
    {
        ui->connectionStatus->setText( "Авторизация не пройдена, невозможно зайти в игру!" );
        return;
    }

    if( !gUNM )
        return;

    if( ui->GameList->item( iRow, 0 ) )
    {
        m_CurrentGameID = ui->GameList->item( iRow, 0 )->data( gGameRoleID ).toUInt( );
        BYTEARRAY request;
        request.push_back( 2 );
        request.push_back( 1 );
        UTIL_AppendByteArray( request, m_CurrentGameID, false );
        gUNM->m_IrinaServer->m_RequestQueue.push_back( request );
        bool tabfound = false;
        uint32_t tabindex = m_MainWidget->GetTabIndexByGameNumber( m_CurrentGameID, tabfound );

        if( !tabfound )
        {
            // general info

            QStringList GameInfoInit;
            GameInfoInit.append( ui->GameList->item( iRow, 0 )->text( ) );
            GameInfoInit.append( ui->GameList->item( iRow, 1 )->text( ) );
            GameInfoInit.append( "Публичная игра" );
            GameInfoInit.append( "Неначатая игра (лобби)" );
            GameInfoInit.append( "Открытая (админ-команды доступны всем админам)" );
            GameInfoInit.append( ui->GameList->item( iRow, 2 )->text( ) );
            GameInfoInit.append( ui->GameList->item( iRow, 2 )->text( ) );
            GameInfoInit.append( ui->GameList->item( iRow, 2 )->text( ) );
            GameInfoInit.append( m_ServerName );
            struct tm * timeinfo;
            char buffer[150];
            string sDate;
            time_t Now = time( nullptr );
            timeinfo = localtime( &Now );
            strftime( buffer, 150, "%d.%m.%y %H:%M:%S", timeinfo );
            sDate = buffer;
            GameInfoInit.append( QString::fromUtf8( sDate.c_str( ) ) );
            GameInfoInit.append( "Статистика отключена" );

            // slots

            QStringList names;
            QStringList races;
            QStringList clans;
            QStringList colors;
            QStringList handicaps;
            QStringList statuses;
            QStringList pings;
            QStringList servers;
            QStringList accesses;
            QStringList levels;
            QStringList ips;
            QStringList countries;
/*          string name = string( );
            string race = string( );
            string clan = string( );
            string colour = string( );
            string handicap = string( );
            string status = string( );
            string ping = string( );
            string server = string( );
            string access = string( );
            string level = string( );
            string ip = string( );
            string country = string( );

            for( unsigned char i = 0; i < m_Slots.size( ); ++i )
            {
                name = string( );
                race = string( );
                clan = "Клан " + UTIL_ToString( m_Slots[i].GetTeam( ) + 1 );
                colour = string( );
                handicap = UTIL_ToString( m_Slots[i].GetHandicap( ) ) + "%";
                status = string( );
                ping = string( );
                server = string( );
                access = string( );
                level = string( );
                ip = string( );
                country = string( );

                switch( m_Slots[i].GetRace( ) )
                {
                    case SLOTRACE_RANDOM:
                    race = "Случайная раса";
                    break;
                    case SLOTRACE_UNDEAD:
                    race = "Нежить";
                    break;
                    case SLOTRACE_HUMAN:
                    race = "Альянс";
                    break;
                    case SLOTRACE_ORC:
                    race = "Орда";
                    break;
                    case SLOTRACE_NIGHTELF:
                    race = "Ночные эльфы";
                    break;
                    case 96:
                    race = "Случайная раса";
                    break;
                    case 72:
                    race = "Нежить";
                    break;
                    case 65:
                    race = "Альянс";
                    break;
                    case 66:
                    race = "Орда";
                    break;
                    case 68:
                    race = "Ночные эльфы";
                    break;
                    case 64:
                    race = "Случайная раса";
                    break;
                    default:
                    race = "Случайная раса";
                }

                switch( m_Slots[i].GetColour( ) )
                {
                    case 0:
                    colour = "red";
                    break;
                    case 1:
                    colour = "blue";
                    break;
                    case 2:
                    colour = "teal";
                    break;
                    case 3:
                    colour = "purple";
                    break;
                    case 4:
                    colour = "yellow";
                    break;
                    case 5:
                    colour = "orange";
                    break;
                    case 6:
                    colour = "green";
                    break;
                    case 7:
                    colour = "pink";
                    break;
                    case 8:
                    colour = "gray";
                    break;
                    case 9:
                    colour = "lightblue";
                    break;
                    case 10:
                    colour = "darkgreen";
                    break;
                    case 11:
                    colour = "brown";
                    break;
                    default:
                    colour = "black";
                }

                if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetComputer( ) == 0 )
                {
                    CGamePlayer *player = GetPlayerFromSID2( i );

                    if( player )
                    {
                        name = player->GetName( );
                        country = "unknown";
                        level = "1 lvl";
                        ping = UTIL_ToString( player->GetPing( m_UNM->m_LCPings ) ) + " ms";
                        ip = player->GetExternalIPString( );

                        if( !m_UNM->IsBnetServer( player->GetJoinedRealm( ) ) )
                            server = "LAN";
                        else
                            server = player->GetJoinedRealm( );

                        if( IsOwner( player->GetName( ) ) )
                            access = "owner";
                        else
                            access = "user";

                        if( !m_GameLoaded && !m_GameLoading )
                        {
                            if( m_Slots[i].GetDownloadStatus( ) == 255 || m_Slots[i].GetDownloadStatus( ) == 100 )
                                status = "ожидает старта игры";
                            else
                                status = "качает карту: " + UTIL_ToString( m_Slots[i].GetDownloadStatus( ) ) + "%";
                        }
                        else if( m_GameLoading )
                        {
                            if( !player->GetFinishedLoading( ) )
                                status = "грузит карту";
                            else
                                status = "ждет начала игры";
                        }
                        else if( player->GetLeftTime( ) > 0 )
                            status = "ливнул на " + UTIL_ToString( player->GetLeftTime( ) ) + " секунде, причина: " + player->GetLeftReason( );
                        else
                            status = "играет";
                    }
                    else
                        name = "Fakeplayer";
                }
                else if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED )
                {
                    if( m_Slots[i].GetComputerType( ) == SLOTCOMP_EASY )
                        name = "Комьютер (слабый)";
                    else if( m_Slots[i].GetComputerType( ) == SLOTCOMP_NORMAL )
                        name = "Комьютер (средний)";
                    else if( m_Slots[i].GetComputerType( ) == SLOTCOMP_HARD )
                        name = "Комьютер (сильный)";
                    else
                        name = "Комьютер";
                }
                else if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN )
                    name = "Открыто";
                else if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_CLOSED )
                    name = "Закрыто";

                names.append( QString::fromUtf8( name.c_str( ) ) );
                races.append( QString::fromUtf8( race.c_str( ) ) );
                clans.append( QString::fromUtf8( clan.c_str( ) ) );
                colors.append( QString::fromUtf8( colour.c_str( ) ) );
                handicaps.append( QString::fromUtf8( handicap.c_str( ) ) );
                statuses.append( QString::fromUtf8( status.c_str( ) ) );
                pings.append( QString::fromUtf8( ping.c_str( ) ) );
                servers.append( QString::fromUtf8( server.c_str( ) ) );
                accesses.append( QString::fromUtf8( access.c_str( ) ) );
                levels.append( QString::fromUtf8( level.c_str( ) ) );
                ips.append( QString::fromUtf8( ip.c_str( ) ) );
                countries.append( QString::fromUtf8( country.c_str( ) ) );
            }
*/
            m_MainWidget->add_game( m_CurrentGameID, GameInfoInit, { names, races, clans, colors, handicaps, statuses, pings, accesses, levels, servers, ips, countries } );
        }

        m_MainWidget->ChangeCurrentGameTab( tabindex );
        m_MainWidget->ChangeCurrentMainTab( 1 );
    }
}


ChatWidgetIrina::~ChatWidgetIrina( )
{
    ui->GameList->clear( );

    if( m_WebSocket != nullptr )
    {
        delete m_WebSocket;
        m_WebSocket = nullptr;
    }

    if( irinaOAuth2 != nullptr )
    {
        delete irinaOAuth2;
        irinaOAuth2 = nullptr;
    }

    if( irinaReplyHandler != nullptr )
    {
        delete irinaReplyHandler;
        irinaReplyHandler = nullptr;
    }

    delete ui;
}

void ChatWidgetIrina::set_select_game_data( unsigned int gameID, QString mapName, QString mapPlayers, QString mapAuthor, QString mapDescription, QPixmap mapImage )
{
    if( m_CurrentGameID == gameID )
    {
        ui->mapName->setText( mapName );
        ui->mapPlayers->setText( "Кол-во игроков: " + mapPlayers );
        ui->mapAuthor->setText( "Автор карты: " + mapAuthor );
        ui->mapDescription->setText( "Описание карты: " + mapDescription );
        ui->mapImage->setPixmap( mapImage );
    }
}

void ChatWidgetIrina::set_select_game_data_bad_tga( unsigned int gameID, QString mapName, QString mapPlayers, QString mapAuthor, QString mapDescription )
{
    if( m_CurrentGameID == gameID || gameID == 0 )
    {
        ui->mapName->setText( mapName );
        ui->mapPlayers->setText( "Кол-во игроков: " + mapPlayers );
        ui->mapAuthor->setText( "Автор карты: " + mapAuthor );
        ui->mapDescription->setText( "Описание карты: " + mapDescription );
        ui->mapImage->setPixmap( QPixmap( ":/wc3map/map_image_unknown.png" ) );
    }
}

void ChatWidgetIrina::on_authButton_clicked( )
{
    ui->authButton->setEnabled( false );
    ui->authButton->setText( "Ожидание..." );
    QMessageBox messageBox( QMessageBox::Question, tr( "Авторизация через дискорд" ), tr( "Сейчас мы откроем браузер и вы будете перенаправлены на страницу авторизации (discord.com)" ), QMessageBox::Yes | QMessageBox::No, this );
    messageBox.setButtonText( QMessageBox::Yes, tr( "хорошо" ) );
    messageBox.setButtonText( QMessageBox::No, tr( "отмена, я передумал!" ) );
    int resBtn = messageBox.exec( );

    if( resBtn == QMessageBox::Yes )
    {
        if( irinaOAuth2 != nullptr )
        {
            delete irinaOAuth2;
            irinaOAuth2 = nullptr;
        }

        if( irinaReplyHandler != nullptr )
        {
            delete irinaReplyHandler;
            irinaReplyHandler = nullptr;
        }

        irinaOAuth2 = new QOAuth2AuthorizationCodeFlow( this );
        irinaOAuth2->setScope( "identify+guilds.join" );
        connect( irinaOAuth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl );
        connect( irinaOAuth2, &QOAuth2AuthorizationCodeFlow::error, this, &ChatWidgetIrina::irinaOAuth2_error );
        connect( irinaOAuth2, &QOAuth2AuthorizationCodeFlow::granted, this, &ChatWidgetIrina::irinaOAuth2_granted );
        irinaOAuth2->setAuthorizationUrl( QUrl ("https://discord.com/api/oauth2/authorize" ) );
        irinaOAuth2->setClientIdentifier( DISCORD_IDENTIFIER );
        irinaOAuth2->setAccessTokenUrl( QUrl( "https://discord.com/api/oauth2/token" ) );
        irinaOAuth2->setClientIdentifierSharedKey( DISCORD_SHARED_KEY );
        irinaReplyHandler = new QOAuthHttpServerReplyHandler( 27016, this );
        irinaOAuth2->setReplyHandler( irinaReplyHandler );
        irinaOAuth2->grant( );

        if( gUNM )
            gUNM->m_IrinaServer->m_CommandQueue.push_back( 0 );
    }
    else
    {
        ui->authButton->setText( "Авторизироваться (Discord)" );
        ui->authButton->setEnabled( true );
    }
}

void ChatWidgetIrina::auth_button_reset( )
{
    ui->authButton->setText( "Авторизироваться (Discord)" );
    ui->authButton->setEnabled( true );
}

void ChatWidgetIrina::irinaOAuth2_granted( )
{
    if( irinaOAuth2 != nullptr )
    {
        ui->authButton->setText( "Авторизация..." );
        ui->authButton->setEnabled( false );
        ui->connectionStatus->setText( "Ожидание авторизации..." );
        gOAuthDiscordToken = irinaOAuth2->token( );

        if( gUNM )
            gUNM->m_IrinaServer->m_CommandQueue.push_back( 1 );

        delete irinaOAuth2;
        irinaOAuth2 = nullptr;

        if( irinaReplyHandler != nullptr )
        {
            delete irinaReplyHandler;
            irinaReplyHandler = nullptr;
        }
    }
}

void ChatWidgetIrina::irinaOAuth2_error( const QString &, const QString &, const QUrl & )
{
    if( irinaOAuth2 != nullptr )
    {
        delete irinaOAuth2;
        irinaOAuth2 = nullptr;
    }

    if( irinaReplyHandler != nullptr )
    {
        delete irinaReplyHandler;
        irinaReplyHandler = nullptr;
    }

    if( gUNM )
        gUNM->m_IrinaServer->m_CommandQueue.push_back( 2 );
}

void ChatWidgetIrina::success_init( )
{
    m_ConnectionStatus = 3;

    if( !gOAuthDiscordToken.isEmpty( ) )
        ui->connectionStatus->setText( "Инициализация завершена" );
    else
        ui->connectionStatus->setText( "Ожидание авторизации..." );
}

void ChatWidgetIrina::TabsResizeFix( )
{
    ui->tabWidget->tabBar( )->setMinimumWidth( ui->tabWidget->width( ) );
}

void ChatWidgetIrina::close_connect( )
{
    m_ConnectionStatus = 7;
    ui->connectionStatus->setText( "Дисконнект, ожидание переподключения..." );
    m_WebSocket->abort( );
}

void ChatWidgetIrina::open_connect( )
{
    m_ConnectionStatus = 1;
    ui->connectionStatus->setText( "Подключение..." );
    m_WebSocket->open( QUrl( "wss://irinabot.ru/ghost/" ) );
}

void ChatWidgetIrina::send_login( )
{
    m_ConnectionStatus = 4;
    ui->connectionStatus->setText( "Авторизация..." );
//    QByteArray ping3;
//    ping3.append( 0x03 ); ping3.append( 0xe9 );
//    m_WebSocket->sendBinaryMessage( ping3 );


    QByteArray init;
    QByteArray init2;
    init2.append( char( 0x00 ) ); init2.append( 0x03 ); init2.append( 0x01 ); init2.append( char( 0x01 ) ); init2.append( 0x4f ); init2.append( 0x65 ); init2.append( 0x71 ); init2.append( 0x4f ); init2.append( 0x42 ); init2.append( 0x58 ); init2.append( 0x34 ); init2.append( 0x35 ); init2.append( 0x59 ); init2.append( 0x31 ); init2.append( 0x72 ); init2.append( 0x7a ); init2.append( 0x78 ); init2.append( 0x33 ); init2.append( 0x46 ); init2.append( 0x58 ); init2.append( 0x78 ); init2.append( 0x55 ); init2.append( 0x79 ); init2.append( 0x51 ); init2.append( 0x65 ); init2.append( 0x72 ); init2.append( 0x6d ); init2.append( 0x73 ); init2.append( 0x6f ); init2.append( 0x49 ); init2.append( 0x4b ); init2.append( 0x4d ); init2.append( 0x33 ); init2.append( 0x30 ); init2.append( char( 0x00 ) );
    init.append( char( 0x00 ) ); init.append( 0x03 ); init.append( 0x01 ); init.append( char( 0x01 ) );
    init.append( gOAuthDiscordToken.toUtf8( ) );
    init.append( char( 0x00 ) );
    m_WebSocket->sendBinaryMessage( init );

//    QByteArray ping2;
//    ping2.append( char( 0x00 ) ); ping2.append( 0x06 ); ping2.append( 0x56 ); ping2.append( 0x65 ); ping2.append( 0x67 ); ping2.append( 0x6c ); ping2.append( 0x69 ); ping2.append( char( 0x00 ) );
//    m_WebSocket->sendBinaryMessage( ping2 );
}

void ChatWidgetIrina::bad_logon( )
{
    m_ConnectionStatus = 5;
    ui->connectionStatus->setText( "Ошибка авторизации" );
    UTIL_SetVarInFile( "unm.cfg", "discord_token", string( ) );
    ui->authButton->setText( "Авторизироваться (Discord)" );
    ui->authButton->setEnabled( true );
}

void ChatWidgetIrina::success_logon( )
{
    m_ConnectionStatus = 8;
    ui->connectionStatus->setText( "Авторизация пройдена" );
    UTIL_SetVarInFile( "unm.cfg", "discord_token", gOAuthDiscordToken.toStdString( ) );

    if( gOAuthDiscordName.isEmpty( ) )
        ui->authButton->setText( "Выйти из аккаунта discord" );
    else
        ui->authButton->setText( "Выйти из аккаунта [" + gOAuthDiscordName + "]" );

    ui->authButton->setEnabled( true );
}

void ChatWidgetIrina::onConnected( )
{
    m_ConnectionStatus = 2;
    ui->connectionStatus->setText( "Подключено, инициализация..." );
//    QByteArray ping;
//    ping.append( char( 0x00 ) ); ping.append( 0x04 ); ping.append( char( 0x00 ) ); ping.append( char( 0x00 ) ); ping.append( char( 0x00 ) );
//    m_WebSocket->sendBinaryMessage( ping );
//    QByteArray ping2;
//    ping2.append( 0xff ); ping2.append( char( 0x00 ) ); ping2.append( char( 0x00 ) ); ping2.append( char( 0x00 ) ); ping2.append( char( 0x00 ) );
//    m_WebSocket->sendBinaryMessage( ping2 );

    if( gUNM )
    {
        BYTEARRAY request;
        request.push_back( 2 );
        request.push_back( 2 );
        gUNM->m_IrinaServer->m_RequestQueue.push_back( request );
    }
}

void ChatWidgetIrina::onDisconnected( )
{
    m_ConnectionStatus = 6;
    ui->connectionStatus->setText( "Дисконнект" );

    if( gUNM )
    {
        BYTEARRAY request;
        request.push_back( 2 );
        request.push_back( 3 );
        gUNM->m_IrinaServer->m_RequestQueue.push_back( request );
    }
}

void ChatWidgetIrina::onBytes( const QByteArray &message )
{
    if( gUNM )
    {
        BYTEARRAY request = BYTEARRAY( message.begin( ), message.end( ) );
        gUNM->m_IrinaServer->m_RequestQueue.push_back( request );
    }
}

void ChatWidgetIrina::onMessage( const QString &message )
{
    if( gUNM )
    {
        string requeststr = message.toStdString( );
        BYTEARRAY request = BYTEARRAY( requeststr.begin( ), requeststr.end( ) );
        request.insert( request.begin( ), (unsigned char)3 );
        gUNM->m_IrinaServer->m_RequestQueue.push_back( request );
    }
}

void ChatWidgetIrina::send_ping( )
{
    QByteArray ping;
    ping.append( 0x01 ); ping.append( 0x05 );
    m_WebSocket->sendBinaryMessage( ping );
//    QByteArray ping2;
//    ping2.append( 0x01 ); ping2.append( 0x02 ); ping2.append( 0x03 ); ping2.append( 0x04 );
//    m_WebSocket->sendBinaryMessage( ping2 );
}

void ChatWidgetIrina::send_gamelist( )
{
    QByteArray gamelist;
    gamelist.append( 0x01 ); gamelist.append( 0x01 ); gamelist.append( 0xff ); gamelist.append( 0xff ); gamelist.append( 0xff ); gamelist.append( 0xff );
    m_WebSocket->sendBinaryMessage( gamelist );
}

void ChatWidgetIrina::send_getgame( unsigned int gameID, unsigned char packetType )
{
    // с помощью функций cellClickedCustom( ) и cellDoubleClickedCustom( ) пользователь делает запрос на получение информации об игре (gamedataex или gamedata)
    // этот запрос мы обрабатываем в основном потоке и если нужной инфы нет в кеше - вызывается данная фунция, в которой мы запрашиваем нужную инфа с ирина-сервера
    // 0 = нужно отправить gamedataex
    // 1 = нужно отправить gamedata
    // 2 = нужно отправить gamedataex и gamedata

    BYTEARRAY getgameB( 4 );
    *(unsigned int*)&getgameB[0] = gameID;

    if( packetType != 0 )
    {
        QByteArray getgame;
        getgame.append( 0x01 ); getgame.append( 0x03 ); getgame.append( char( 0x00) );
        getgame.append( getgameB[0] ); getgame.append( getgameB[1] ); getgame.append( getgameB[2] ); getgame.append( getgameB[3] );
        getgame.append( 0x75 ); getgame.append( 0x6e ); getgame.append( 0x64 ); getgame.append( 0x65 ); getgame.append( 0x66 ); getgame.append( 0x69 ); getgame.append( 0x6e ); getgame.append( 0x65 ); getgame.append( 0x64 ); getgame.append( nullptr );
        m_WebSocket->sendBinaryMessage( getgame );
    }

    if( packetType != 1 )
    {
        QByteArray getgameex;
        getgameex.append( 0x01 ); getgameex.append( 0x0d );
        getgameex.append( getgameB[0] ); getgameex.append( getgameB[1] ); getgameex.append( getgameB[2] ); getgameex.append( getgameB[3] );
        getgameex.append( nullptr );
        m_WebSocket->sendBinaryMessage( getgameex );
    }

//    QByteArray ping2;
//    ping2.append( char( 0x00 ) ); ping2.append( 0x06 ); ping2.append( 0x56 ); ping2.append( 0x65 ); ping2.append( 0x67 ); ping2.append( 0x6c ); ping2.append( 0x69 ); ping2.append( char( 0x00 ) );
//    m_WebSocket->sendBinaryMessage( ping2 );
}

void ChatWidgetIrina::on_send_clicked( )
{
    on_entryfield_returnPressed( );
}

void ChatWidgetIrina::on_entryfield_returnPressed( )
{
    ui->entryfield->clear( );
    ui->chatfield->appendHtml( "<font color = " + gNormalMessagesColor + ">" + ui->entryfield->text( ) );
}

void ChatWidgetIrina::write_to_chat_direct( QString message )
{
    message.replace( "<", "&lt;" );
    message.replace( ">", "&gt;" );
    message.replace( "  ", " &nbsp;" );
    message.replace( "  ", "&nbsp; " );

    if( message.mid( message.size( ) - 1 ) == "0" )
        ui->chatfield->appendHtml( "<font color = " + gNormalMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
    else if( message.mid( message.size( ) - 1 ) == "2" )
        ui->chatfield->appendHtml( "<font color = " + gPrivateMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
    else if( message.mid( message.size( ) - 1 ) == "3" )
        ui->chatfield->appendHtml( "<font color = " + gNotifyMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
    else if( message.mid( message.size( ) - 1 ) == "4" )
        ui->chatfield->appendHtml( "<font color = " + gWarningMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
    else
        ui->chatfield->appendHtml( "<font color = " + gNormalMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
}

GameWidget::GameWidget( Widget *parent, unsigned int gameID, QStringList gameInfoInit, QList<QStringList> playersInfo ) :
    m_MainWidget( parent ),
    m_GameID( gameID ),
    m_GameStarted( false ),
    ui( new Ui::GameWidget )
{
    ui->setupUi( this );
    ui->logfield->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
    ui->playersList->setColumnCount( playersInfo.size( ) );
    ui->playersList->setRowCount( playersInfo[0].size( ) );

    if( gUNM )
        ui->war3Path->setText( QString::fromUtf8( gUNM->m_Warcraft3Path.c_str( ) ) );

    for( int i = 0; i < ui->playersList->columnCount( ); i++ )
    {
        for( int j = 0; j < ui->playersList->rowCount( ); j++ )
        {
            if( !ui->playersList->item( j, i ) )
            {
                ui->playersList->setItem( j, i, new QTableWidgetItem( ) );
                ui->playersList->item( j, i )->setTextAlignment( Qt::AlignCenter );
            }

            if( i != 3 )
                ui->playersList->item( j, i )->setText( playersInfo[i][j] );
            else if( playersInfo[i][j] == "red" )
                ui->playersList->item( j, i )->setBackground( QColor( "#ff0000" ) );
            else if( playersInfo[i][j] == "blue" )
                ui->playersList->item( j, i )->setBackground( QColor( "#0000ff" ) );
            else if( playersInfo[i][j] == "teal" )
                ui->playersList->item( j, i )->setBackground( QColor( "#00ffff" ) );
            else if( playersInfo[i][j] == "purple" )
                ui->playersList->item( j, i )->setBackground( QColor( "#800080" ) );
            else if( playersInfo[i][j] == "yellow" )
                ui->playersList->item( j, i )->setBackground( QColor( "#ffff00" ) );
            else if( playersInfo[i][j] == "orange" )
                ui->playersList->item( j, i )->setBackground( QColor( "#ffa500" ) );
            else if( playersInfo[i][j] == "green" )
                ui->playersList->item( j, i )->setBackground( QColor( "#00ff00" ) );
            else if( playersInfo[i][j] == "pink" )
                ui->playersList->item( j, i )->setBackground( QColor( "#ff00ff" ) );
            else if( playersInfo[i][j] == "gray" )
                ui->playersList->item( j, i )->setBackground( QColor( "#858585" ) );
            else if( playersInfo[i][j] == "lightblue" )
                ui->playersList->item( j, i )->setBackground( QColor( "#add8e6" ) );
            else if( playersInfo[i][j] == "darkgreen" )
                ui->playersList->item( j, i )->setBackground( QColor( "#008000" ) );
            else if( playersInfo[i][j] == "brown" )
                ui->playersList->item( j, i )->setBackground( QColor( "#964B00" ) );
            else
                ui->playersList->item( j, i )->setBackground( QColor( "#000000" ) );
        }
    }

    for( int i = 0; i < ui->gameInfo->rowCount( ); i++ )
    {
        if( !ui->gameInfo->item( i, 0 ) )
            ui->gameInfo->setItem( i, 0, new QTableWidgetItem( ) );

        ui->gameInfo->item( i, 0 )->setText( gameInfoInit[i] );
    }
}

void GameWidget::update_game_direct( QStringList gameInfo, QList<QStringList> playersInfo )
{
    for( int i = 0; i < ui->playersList->columnCount( ); i++ )
    {
        for( int j = 0; j < ui->playersList->rowCount( ); j++ )
        {
            if( i != 3 )
                ui->playersList->item( j, i )->setText( playersInfo[i][j] );
            else if( playersInfo[i][j] == "red" )
                ui->playersList->item( j, i )->setBackground( QColor( "#ff0000" ) );
            else if( playersInfo[i][j] == "blue" )
                ui->playersList->item( j, i )->setBackground( QColor( "#0000ff" ) );
            else if( playersInfo[i][j] == "teal" )
                ui->playersList->item( j, i )->setBackground( QColor( "#00ffff" ) );
            else if( playersInfo[i][j] == "purple" )
                ui->playersList->item( j, i )->setBackground( QColor( "#800080" ) );
            else if( playersInfo[i][j] == "yellow" )
                ui->playersList->item( j, i )->setBackground( QColor( "#ffff00" ) );
            else if( playersInfo[i][j] == "orange" )
                ui->playersList->item( j, i )->setBackground( QColor( "#ffa500" ) );
            else if( playersInfo[i][j] == "green" )
                ui->playersList->item( j, i )->setBackground( QColor( "#00ff00" ) );
            else if( playersInfo[i][j] == "pink" )
                ui->playersList->item( j, i )->setBackground( QColor( "#ff00ff" ) );
            else if( playersInfo[i][j] == "gray" )
                ui->playersList->item( j, i )->setBackground( QColor( "#858585" ) );
            else if( playersInfo[i][j] == "lightblue" )
                ui->playersList->item( j, i )->setBackground( QColor( "#add8e6" ) );
            else if( playersInfo[i][j] == "darkgreen" )
                ui->playersList->item( j, i )->setBackground( QColor( "#008000" ) );
            else if( playersInfo[i][j] == "brown" )
                ui->playersList->item( j, i )->setBackground( QColor( "#964B00" ) );
            else
                ui->playersList->item( j, i )->setBackground( QColor( "#000000" ) );
        }
    }

    ui->gameInfo->item( 0, 0 )->setText( gameInfo[0] );
    ui->gameInfo->item( 2, 0 )->setText( gameInfo[1] );
    ui->gameInfo->item( 3, 0 )->setText( gameInfo[2] );
    ui->gameInfo->item( 4, 0 )->setText( gameInfo[3] );
    ui->gameInfo->item( 6, 0 )->setText( gameInfo[4] );

    if( !m_GameStarted && gameInfo[2] != "Неначатая игра (лобби)" )
        m_GameStarted = true;
}

void GameWidget::update_game_map_info_direct( QString mapPath )
{
    ui->gameInfo->item( 1, 0 )->setText( mapPath );
}

void GameWidget::logfieldSetColor( )
{
    ui->logfield->setStyleSheet( "QPlainTextEdit { color: " + gNormalMessagesColor + "; }" );
}

void GameWidget::write_to_log_direct( QString message )
{
    ui->logfield->appendPlainText( message );
}

void GameWidget::write_to_game_chat_direct( QString message, QString extramessage, QString user, unsigned int type )
{
    message.replace( "<", "&lt;" );
    message.replace( ">", "&gt;" );
    message.replace( "  ", " &nbsp;" );
    message.replace( "  ", "&nbsp; " );

    // bot messages

    if( type == 0 )
    {
        if( !extramessage.isEmpty( ) )
        {
            if( user == extramessage || user.isEmpty( ) )
                ui->chatfield->appendHtml( "<font color = " + gNotifyMessagesColor + ">[BOT - Private chat] To </font>" + extramessage + "<font color = " + gNotifyMessagesColor + ">: " + message + "</font>" );
            else
                ui->chatfield->appendHtml( "<font color = " + gNotifyMessagesColor + ">[BOT - Private chat] From </font>" + user + "<font color = " + gNotifyMessagesColor + "> To </font>" + extramessage + "<font color = " + gNotifyMessagesColor + ">: " + message + "</font>" );
        }
    }
    else if( type == 1 )
    {
        if( !user.isEmpty( ) )
            ui->chatfield->appendHtml( "<font color = " + gNotifyMessagesColor + ">[BOT - All chat] </font>" + user + "<font color = " + gNotifyMessagesColor + ">: " +  message + "</font>" );
        else
            ui->chatfield->appendHtml( "<font color = " + gNotifyMessagesColor + ">[BOT - All chat] " + message + "</font>" );
    }
    else if( type == 2 )
    {
        if( !user.isEmpty( ) )
            ui->chatfield->appendHtml( "<font color = " + gNotifyMessagesColor + ">[BOT - Ally Chat " + extramessage + "] </font>" + user + "<font color = " + gNotifyMessagesColor + ">: " +  message + "</font>" );
        else
            ui->chatfield->appendHtml( "<font color = " + gNotifyMessagesColor + ">[BOT - Ally Chat " + extramessage + "] " + message + "</font>" );
    }
    else if( type == 3 )
    {
        if( !user.isEmpty( ) )
            ui->chatfield->appendHtml( "<font color = " + gNotifyMessagesColor + ">[BOT - Admin chat] </font>" + user + "<font color = " + gNotifyMessagesColor + ">: " +  message + "</font>" );
        else
            ui->chatfield->appendHtml( "<font color = " + gNotifyMessagesColor + ">[BOT - Admin chat] " + message + "</font>" );
    }

    // player messages

    else if( type == 4 )
    {
        if( !user.isEmpty( ) )
            ui->chatfield->appendHtml( "<font color = " + gNormalMessagesColor + ">[All] </font>" + user + "<font color = " + gNormalMessagesColor + ">: " +  message + "</font>" );
    }
    else if( type == 5 )
    {
        if( !user.isEmpty( ) )
            ui->chatfield->appendHtml( "<font color = " + gNormalMessagesColor + ">[Allies " + extramessage + "] </font>" + user + "<font color = " + gNormalMessagesColor + ">: " +  message + "</font>" );
    }
    else if( type == 6 )
    {
        if( !user.isEmpty( ) )
            ui->chatfield->appendHtml( "<font color = " + gPreSentMessagesColor + ">[" + extramessage + "] </font><font color = " + gNormalMessagesColor + ">" + user + "</font><font color = " + gPreSentMessagesColor + ">: " +  message + "</font>" );
    }
    else if( type == 7 )
    {
        if( !extramessage.isEmpty( ) && !user.isEmpty( ) )
            ui->chatfield->appendHtml( "<font color = " + gPrivateMessagesColor + ">[Private] From </font>" + user + "<font color = " + gPrivateMessagesColor + "> To </font>" + extramessage + "<font color = " + gPrivateMessagesColor + ">: " + message + "</font>" );
    }
    else if( type == 8 )
    {
        if( !extramessage.isEmpty( ) && !user.isEmpty( ) )
            ui->chatfield->appendHtml( "<font color = " + gPrivateMessagesColor + ">[Private] From </font>" + user + "<font color = " + gPrivateMessagesColor + "> To " + extramessage + ": " + message + "</font>" );
    }

    // notification messages

    else if( type == 9 )
    {
        if( !user.isEmpty( ) )
            ui->chatfield->appendHtml( "<font color = " + gPrivateMessagesColor + ">Игрок [</font>" + user + "<font color = " + gPrivateMessagesColor + ">] присоединился к игре</font>" );
    }
    else if( type == 10 )
    {
        if( !user.isEmpty( ) )
            ui->chatfield->appendHtml( "<font color = " + gPreSentMessagesColor + ">Игрок [</font>" + user + "<font color = " + gPreSentMessagesColor + ">] вышел из лобби</font>" );
    }
    else if( type == 11 )
    {
        if( !user.isEmpty( ) )
            ui->chatfield->appendHtml( "<font color = " + gWarningMessagesColor + ">Игрок [</font>" + user + "<font color = " + gWarningMessagesColor + ">] покинул игру</font>" );
    }
    else if( type == 3 )
        ui->chatfield->appendHtml( "<font color = " + gWarningMessagesColor + ">" + message.mid( 0, message.size( ) - 1 ) + "</font>" );
}

GameWidget::~GameWidget( )
{
    ui->playersList->clear( );
    ui->gameInfo->clear( );
    delete ui;
}

uint32_t CreateRemoteThreadUNM( LPCSTR DllPath, HANDLE hProcess )
{
    uint32_t result = 0;
    LPVOID LoadLibAddr = (LPVOID)GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "LoadLibraryA" );

    if( !LoadLibAddr )
        return result;

    LPVOID pDllPath = VirtualAllocEx( hProcess, nullptr, strlen(DllPath), MEM_COMMIT, PAGE_READWRITE );

    if( !pDllPath )
        return result;

    BOOL Written = WriteProcessMemory( hProcess, pDllPath, (LPVOID)DllPath, strlen(DllPath), NULL );

    if( !Written )
        return result;

    HANDLE hThread = CreateRemoteThread( hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibAddr, pDllPath, 0, NULL );

    if( !hThread )
        return result;

    result++;
    WaitForSingleObject( hThread, INFINITE );

    if( !VirtualFreeEx( hProcess, pDllPath, 0, MEM_RELEASE ) )
        result++;

    CloseHandle( hThread );
    return result;
}

void GameWidget::on_war3Run_clicked( )
{
    if( gUNM )
    {
        if( gUNM->m_Warcraft3Path == "war3\\" || gUNM->m_Warcraft3Path == "war3/" || gUNM->m_Warcraft3Path == "war3" )
            QMessageBox::critical( this, "Ошибка, не найдена директория игры", "Для запуска игры необходимо указать путь к папке с игрой, где лежит файл [war3.exe]" );
        else if( gUNM->m_LastWar3RunTicks < GetTicks( ) && ( GetTicks( ) - gUNM->m_LastWar3RunTicks > 1500 || gUNM->m_LastWar3RunTicks == 0 ) )
        {
            gUNM->m_LastWar3RunTicks = GetTicks( );
            QString war3path = QString::fromUtf8( gUNM->m_Warcraft3Path.c_str( ) );

            if( UTIL_FixPath( ui->war3Path->text( ).toStdString( ) ) != gUNM->m_Warcraft3Path )
            {
                QMessageBox messageBox( QMessageBox::Question, tr( "Секундочку!" ), tr( "Похоже, путь к каталогу игры был изменён. Примениь новый путь или использовать прежний?" ), QMessageBox::Yes | QMessageBox::No, this );
                messageBox.setButtonText( QMessageBox::Yes, tr( "Применить новый!" ) );
                messageBox.setButtonText( QMessageBox::No, tr( "Использовать старый!" ) );
                messageBox.setButtonText( QMessageBox::Cancel, tr( "Отмена, я запутался!" ) );
                int resBtn = messageBox.exec( );

                if( resBtn == QMessageBox::Yes )
                {
                    gUNM->m_Warcraft3Path = UTIL_FixPath( ui->war3Path->text( ).toStdString( ) );
                    war3path = QString::fromUtf8( gUNM->m_Warcraft3Path.c_str( ) );
                }
                else if( resBtn == QMessageBox::No )
                    ui->war3Path->setText( war3path );
                else
                    return;
            }
            else
                ui->war3Path->setText( war3path );

            QDir pathDir( war3path );

            if( !pathDir.exists( ) )
                QMessageBox::critical( this, "Ошибка, не найдена директория игры", "Путь к каталогу игры указан неверно!" );
            else if( !QFile::exists( war3path + "war3.exe" ) )
                QMessageBox::critical( this, "Ошибка, не найден исполняемый файл игры", "В указанном вами каталоге отсутствует файл \"war3.exe\". А он нам нужен и никакой варкрафт мы без него запускать не будем, вот так!" );
            else
            {
                string FileName = gUNM->m_Warcraft3Path + "war3.exe";
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory( &si, sizeof( si ) );
                si.cb = sizeof( si );
                ZeroMemory( &pi, sizeof( pi ) );

                if( !CreateProcessA( FileName.c_str( ), const_cast<char *>( FileName.c_str( ) ), NULL, NULL, FALSE, 0, NULL, gUNM->m_Warcraft3Path.c_str( ), (LPSTARTUPINFOA)&si, &pi ) )
                {
                    CONSOLE_Print( "[UNM] Не удалось запустить Warcraft III" );
                    QMessageBox::critical( this, "У нас ошибка", "По непонятным причинам запуск не удался. И мы не знаем почему." );
                }
                else
                {
                    CONSOLE_Print( "[UNM] Произведен успешный запуск Warcraft III" );
                    QString dllfile = qApp->applicationDirPath( ) + "\\wfe\\WFEDll.dll";

                    if( !QFile::exists( dllfile ) )
                        QMessageBox::critical( this, "У нас ошибка", "Не найден файл [" + dllfile + "] - невозможно пропатчить игру. Христа ради, верните файл на место!" );
                    else
                    {
                        QThread::msleep( 500 );
                        uint32_t result = CreateRemoteThreadUNM( dllfile.toUtf8( ).constData( ), pi.hProcess );

                        if( result == 0 )
                            QMessageBox::critical( this, "У нас ошибка", "По непонятным причинам не удалось пропатчить игру. Возможно кто-то или что-то не позволяет нам это сделать." );
                        else
                        {
                            CONSOLE_Print( "[UNM] Процесс [war3.exe] был успешно пропатчен!" );

                            if( result == 2 )
                                CONSOLE_Print( "[UNM] Не удалось освободить память в процессе [war3.exe] после успешного патчинга." );
                        }
                    }

                    CloseHandle( pi.hProcess );
                    CloseHandle( pi.hThread );
                }
            }
        }
    }
    else
        QMessageBox::critical( this, "Так точно не должно быть!", "Увы, что-то пошло не так, требуется перезапуск программы." );
}
