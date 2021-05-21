#ifndef CUSTOMMAINTABBAR_H
#define CUSTOMMAINTABBAR_H

#include <QTabBar>
#include <QStylePainter>
#include <QStyleOptionTabV4>

class CustomMainTabBar : public QTabBar
{
public:
    explicit CustomMainTabBar( QWidget *parent = nullptr ) : QTabBar( parent )
    {
        setIconSize( QSize( 60, 60 ) );
    }

protected:
    QSize tabSizeHint(int) const
    {
        return QSize( 70, 60 );
    }
    void paintEvent( QPaintEvent * )
    {
        for( int index = 0; index < count( ); index++ )
        {
            QStylePainter p( this );
            QStyleOptionTabV4 tab;
            p.setFont( QFont( "Calibri", 8 ) );
            initStyleOption( &tab, index );
            QIcon tempIcon = tab.icon;
            QString tempText = tab.text;
            QRect tabrect = tab.rect;
            tab.icon = QIcon( );
            tab.text = QString( );
            p.drawControl( QStyle::CE_TabBarTab, tab );
            tabrect.adjust( 0, 0, 0, -5 );

            if( index == currentIndex( ) )
                p.setPen( Qt::black );

            p.drawText( tabrect, Qt::AlignBottom | Qt::AlignHCenter, tempText );
            tabrect.adjust( 0, 5, 0, -15 );
            tempIcon.paint( &p, tabrect, Qt::AlignTop | Qt::AlignHCenter );
        }
    }
};

#endif
