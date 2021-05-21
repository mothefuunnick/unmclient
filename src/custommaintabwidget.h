#ifndef CUSTOMMAINTABWIDGET_H
#define CUSTOMMAINTABWIDGET_H

#include <QTabWidget>
#include "custommaintabbar.h"

class CustomMainTabWidget : public QTabWidget
{
public:
    explicit CustomMainTabWidget( QWidget *parent = nullptr ) : QTabWidget( parent )
    {
        setTabBar( new CustomMainTabBar( ) );
    }
};

#endif
