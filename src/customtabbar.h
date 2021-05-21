#ifndef CUSTOMTABBAR_H
#define CUSTOMTABBAR_H

#include <QTabBar>
#include <QStylePainter>
#include <QStyleOptionTabV4>

class CustomTabBar : public QTabBar
{
public:
    explicit CustomTabBar( QWidget *parent = nullptr ) : QTabBar( parent )
    {
    }

protected:
    void paintEvent( QPaintEvent * )
    {
        for( int index = 0; index < count( ); index++ )
        {
            QStylePainter p( this );
            QStyleOptionTabV4 tab;
            initStyleOption( &tab, index );
            QIcon tempIcon = tab.icon;
            QString tempText = tab.text;
            QRect tabrect = tab.rect;
            tab.icon = QIcon( );
            tab.text = QString( );
            p.drawControl( QStyle::CE_TabBarTab, tab );
            tabrect.adjust( 19, 0, 0, 0 );

            if( index == currentIndex( ) )
                p.setPen( Qt::black );

            p.drawText( tabrect, Qt::AlignVCenter | Qt::AlignHCenter, tempText );
            QFontMetrics fm( p.font( ) );
            int len_px = fm.width( tempText );
            tabrect.adjust( -24 - len_px, 0, 0, 0 );
            QRect tabrecticon = QRect( tabrect.x( ), tabrect.y( ), 16, 16 );
            tabrecticon.moveCenter( tabrect.center( ) );
            tempIcon.paint( &p, tabrecticon, Qt::AlignVCenter | Qt::AlignHCenter );
        }
    }
};

#endif
