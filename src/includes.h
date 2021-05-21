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

#ifndef INCLUDES_H
#define INCLUDES_H

// standard integer sizes for 64 bit compatibility

#ifdef WIN32
#include "ms_stdint.h"
#else
#include <stdint.h>
#endif

// STL

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>
#include <codecvt>

// Qt

#include <QApplication>
#include <QProcess>
#include <QTextBlock>
#include <QMessageBox>
#include <QTimer>
#include <QTextStream>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDir>
#include <QToolTip>
#include <QCompleter>
#include <QHostInfo>
#include <QGraphicsDropShadowEffect>

typedef std::vector<unsigned char> BYTEARRAY;
typedef std::pair<unsigned char, std::string> PIDPlayer;
typedef std::pair<std::string, std::vector<std::string>> PairedMapList;

// time

uint32_t GetTime( );		// seconds
uint32_t GetTicks( );		// milliseconds

// network

#undef FD_SETSIZE
#define FD_SETSIZE 512

// output

void CONSOLE_Print( std::string message, uint32_t toGUI = 0, uint32_t gameID = 0 );

// filesystem

void ShowInExplorer( QString path, bool selectForlder = false );

// patch 21

bool Patch21( );

// exit

void ApplicationShutdown( );

#endif
