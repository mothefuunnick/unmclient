#include "customtablewidgetitem.h"

CustomTableWidgetItem::CustomTableWidgetItem( bool irina ) : QTableWidgetItem( )
{
	m_Irina = irina;
}

bool CustomTableWidgetItem::operator < ( const QTableWidgetItem &other ) const
{
    if( !m_Irina )
    {
        if( !other.data( Qt::UserRole ).toBool( ) && data( Qt::UserRole ).toBool( ) )
            return false;
        else if( other.data( Qt::UserRole ).toBool( ) && !data( Qt::UserRole ).toBool( ) )
            return false;
        else
            return QTableWidgetItem::operator < ( other );
    }
    else if( !other.data( Qt::UserRole ).toBool( ) && data( Qt::UserRole ).toBool( ) )
        return false;
    else if( other.data( Qt::UserRole ).toBool( ) && !data( Qt::UserRole ).toBool( ) )
        return false;
    else
        return QTableWidgetItem::operator < ( other );
}
