#ifndef CUSTOMDYNAMICFONTSIZELABEL_H
#define CUSTOMDYNAMICFONTSIZELABEL_H

#include <QLabel>
#include <QColor>

class CustomDynamicFontSizeLabel : public QLabel
{
    Q_OBJECT

public:
    explicit CustomDynamicFontSizeLabel( QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags( ) );
    static float getWidgetMaximumFontSize( QWidget *widget, QString text );

    /* This method overwrite stylesheet */

    void setText( QString text );
    void clear( );
    bool GetGreenBorder( );
    void SetGreenBorder( bool nGreenBorder );

signals:
    void clicked( bool rightButton );

protected:
    void mousePressEvent( QMouseEvent* event );
	
// QWidget interface

protected:
    void paintEvent( QPaintEvent *event );

public:
    QSize minimumSizeHint( ) const;
    QSize sizeHint( ) const;

private:
    bool m_GreenBorder;
};

#endif // CUSTOMDYNAMICFONTSIZELABEL_H
