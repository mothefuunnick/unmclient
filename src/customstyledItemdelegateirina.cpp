#include "customstyledItemdelegateirina.h"
#include <QPainter>
#include <QTextDocument>

extern int gGameRoleStarted;
extern int gGameRoleID;
extern int gGameRolePlayers;

CustomStyledItemDelegateIrina::CustomStyledItemDelegateIrina( QTableWidget *tableWidget, QObject *parent )
    : QStyledItemDelegate( parent ), m_tableWdt( tableWidget ), m_hoveredRow( -1 )
{
    // mouse tracking have to be true, otherwise itemEntered won't emit

    m_tableWdt->setMouseTracking( false );
    m_tableWdt->viewport( )->setMouseTracking( true );
    connect( m_tableWdt, SIGNAL( itemEntered( QTableWidgetItem *) ), this, SLOT( onItemEntered( QTableWidgetItem * ) ) );
}

void CustomStyledItemDelegateIrina::onItemEntered( QTableWidgetItem *item )
{
    m_hoveredRow = item->row( );
    m_tableWdt->viewport( )->update( );  // force update
}

void CustomStyledItemDelegateIrina::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    painter->save( );

    if( option.state & QStyle::State_Selected )
        painter->fillRect( option.rect, QColor( "#f4802a" ) );
    else if( index.row( ) == m_hoveredRow )
    {
        if( QAbstractItemView* tableView = qobject_cast<QAbstractItemView*>( this->parent( ) ) )
        {
            int y = tableView->viewport( )->mapFromGlobal( QCursor::pos( ) ).y( );
            int x = tableView->viewport( )->mapFromGlobal( QCursor::pos( ) ).x( );

            if( index.row( ) == m_hoveredRow && y >= tableView->viewport( )->rect( ).y( ) && y <= tableView->viewport( )->rect( ).height( ) && x >= tableView->viewport( )->rect( ).x( ) && x <= tableView->viewport( )->rect( ).width( ) )
                painter->fillRect( option.rect, QColor( "#ffaa00" ) );
            else if( index.data( gGameRoleStarted ).toBool( ) )
                painter->fillRect( option.rect, ( index.row( ) % 2 ) ? QColor( "#c0f8bc" ) : QColor( "#a6ffa3" ) );
            else
                painter->fillRect( option.rect, ( index.row( ) % 2 ) ? QColor( "#d0d0d0" ) : QColor( "#b2b2b2" ) );
        }
        else if( index.data( gGameRoleStarted ).toBool( ) )
            painter->fillRect( option.rect, ( index.row( ) % 2 ) ? QColor( "#c0f8bc" ) : QColor( "#a6ffa3" ) );
        else
            painter->fillRect( option.rect, ( index.row( ) % 2 ) ? QColor( "#d0d0d0" ) : QColor( "#b2b2b2" ) );
    }
    else if( index.data( gGameRoleStarted ).toBool( ) )
        painter->fillRect( option.rect, ( index.row( ) % 2 ) ? QColor( "#c0f8bc" ) : QColor( "#a6ffa3" ) );
    else
        painter->fillRect( option.rect, ( index.row( ) % 2 ) ? QColor( "#d0d0d0" ) : QColor( "#b2b2b2" ) );

    if( index.column( ) != 2 )
    {
        painter->setPen( Qt::black );
        QRect clip( option.rect.x( ) + 3, option.rect.y( ), option.rect.width( ) - 6, option.rect.height( ) );

        if( index.column( ) == 1 )
            painter->drawText( clip, option.displayAlignment | Qt::TextWordWrap | Qt::AlignHCenter, index.data( ).toString( ) );
        else
            painter->drawText( clip, option.displayAlignment | Qt::TextWordWrap, index.data( ).toString( ) );
    }
    else
    {
        QStyleOptionViewItem options = option;
        initStyleOption( &options, index );
        int iconSizeWidth = ( options.icon.actualSize( options.rect.size( ) ).width( ) / 2 ) + 5;
        QTextDocument doc;
        doc.setHtml( options.text );
        doc.setTextWidth( options.rect.width( ) - iconSizeWidth );
        QTextDocument doc2;
        doc2.setHtml( index.data( gGameRolePlayers ).toString( ) );
        doc2.setTextWidth( options.rect.width( ) - iconSizeWidth );
        options.text = QString( );
        options.state = QStyle::State_Enabled;
        options.widget->style( )->drawControl( QStyle::CE_ItemViewItem, &options, painter );
        painter->restore( );
        painter->save( );

        if( doc.size( ).height( ) < 28 )
        {
            painter->translate( options.rect.left( ) + iconSizeWidth + 1, options.rect.top( ) + 4 );
            doc2.drawContents( painter );
            painter->restore( );
            painter->save( );
            painter->translate( options.rect.left( ) + iconSizeWidth, options.rect.top( ) + 5 );
            doc2.drawContents( painter );
            painter->restore( );
            painter->save( );
            painter->translate( options.rect.left( ) + iconSizeWidth, options.rect.top( ) + 4 );
            doc.drawContents( painter );
        }
        else
        {
            painter->translate( options.rect.left( ) + iconSizeWidth + 1, options.rect.top( ) - 2 );
            doc2.drawContents( painter );
            painter->restore( );
            painter->save( );
            painter->translate( options.rect.left( ) + iconSizeWidth, options.rect.top( ) - 1 );
            doc2.drawContents( painter );
            painter->restore( );
            painter->save( );
            painter->translate( options.rect.left( ) + iconSizeWidth, options.rect.top( ) - 2 );
            doc.drawContents( painter );
        }
    }

    painter->restore( );
}

QSize CustomStyledItemDelegateIrina::sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    QStyleOptionViewItem options = option;
    initStyleOption( &options, index );
    QTextDocument doc;
    doc.setHtml( options.text );
    doc.setTextWidth( options.rect.width( ) );
    return QSize( doc.idealWidth( ), doc.size( ).height( ) );
}
