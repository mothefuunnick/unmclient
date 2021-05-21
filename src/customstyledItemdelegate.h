#ifndef CUSTOMSTYLEDITEMDELEGATE_H
#define CUSTOMSTYLEDITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QTableWidget>

class CustomStyledItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    CustomStyledItemDelegate( QTableWidget *tableWidget, QObject *parent = nullptr );

protected:
    void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
    QSize sizeHint( const QStyleOptionViewItem & option, const QModelIndex & index ) const;

protected slots:
    void onItemEntered( QTableWidgetItem *item );

private:
    QTableWidget *m_tableWdt;
    int m_hoveredRow;
};

#endif
