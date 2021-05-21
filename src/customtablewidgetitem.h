#ifndef CUSTOMTABLEWIDGETITEM_H
#define CUSTOMTABLEWIDGETITEM_H

#include <QTableWidgetItem>

class CustomTableWidgetItem : public QTableWidgetItem
{
public:
    CustomTableWidgetItem( bool irina );
    bool operator < ( const QTableWidgetItem &other ) const;

private:
    bool m_Irina;
};

#endif
