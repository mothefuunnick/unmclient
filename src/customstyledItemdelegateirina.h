#ifndef CUSTOMSTYLEDITEMDELEGATEIRINA_H
#define CUSTOMSTYLEDITEMDELEGATEIRINA_H

#include <QStyledItemDelegate>
#include <QTableWidget>

class CustomStyledItemDelegateIrina : public QStyledItemDelegate
{
    Q_OBJECT
public:
    CustomStyledItemDelegateIrina( QTableWidget *tableWidget, QObject *parent = nullptr );

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
