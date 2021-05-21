#include "customaspectratiopixmaplabel.h"

CustomAspectRatioPixmapLabel::CustomAspectRatioPixmapLabel( QWidget *parent ) : QLabel( parent )
{
    this->setMinimumSize( 1,1 );
    setScaledContents( false );
}

void CustomAspectRatioPixmapLabel::setPixmap( const QPixmap &p )
{
    pix = p;
    QLabel::setPixmap( scaledPixmap( ) );
}

int CustomAspectRatioPixmapLabel::heightForWidth( int width ) const
{
    return ( pix.isNull( ) ? this->height( ) : ( ( (qreal)pix.height( ) * width ) / pix.width( ) ) );
}

QSize CustomAspectRatioPixmapLabel::sizeHint( ) const
{
    int w = this->width( );
    return QSize( w, heightForWidth( w ) );
}

QPixmap CustomAspectRatioPixmapLabel::scaledPixmap( ) const
{
    return pix.scaled( this->size( ), Qt::KeepAspectRatio, Qt::SmoothTransformation );
}

void CustomAspectRatioPixmapLabel::resizeEvent( QResizeEvent * e )
{
    if( !pix.isNull( ) )
        QLabel::setPixmap( scaledPixmap( ) );
}