#ifndef CUSTOMASPECTRATIOPIXMAPLABEL_H
#define CUSTOMASPECTRATIOPIXMAPLABEL_H

#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>

class CustomAspectRatioPixmapLabel : public QLabel
{
    Q_OBJECT

public:
    explicit CustomAspectRatioPixmapLabel( QWidget *parent = nullptr );
    virtual int heightForWidth( int width ) const;
    virtual QSize sizeHint( ) const;
    QPixmap scaledPixmap( ) const;

public slots:
    void setPixmap( const QPixmap & );
    void resizeEvent( QResizeEvent * );

private:
    QPixmap pix;
};

#endif // CUSTOMASPECTRATIOPIXMAPLABEL_H