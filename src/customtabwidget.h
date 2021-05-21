#ifndef CUSTOMTABWIDGET_H
#define CUSTOMTABWIDGET_H

#include <QTabWidget>
#include "customtabbar.h"

class CustomTabWidget : public QTabWidget
{
public:
    explicit CustomTabWidget( QWidget *parent = nullptr ) : QTabWidget( parent )
    {
        setTabBar( new CustomTabBar( ) );
    }
};

#endif
