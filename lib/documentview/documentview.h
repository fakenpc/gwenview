// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QGraphicsWidget>

// KDE
#include <kactioncollection.h>

// Local
#include <lib/document/document.h>

class KUrl;

namespace Gwenview
{

class AbstractRasterImageViewTool;
class RasterImageView;
class SlideShow;
class ZoomWidget;

struct DocumentViewPrivate;

/**
 * This widget can display various documents, using an instance of
 * AbstractDocumentViewAdapter
 */
class GWENVIEWLIB_EXPORT DocumentView : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(bool zoomToFit READ zoomToFit WRITE setZoomToFit NOTIFY zoomToFitChanged)
    Q_PROPERTY(QPoint position READ position WRITE setPosition NOTIFY positionChanged)
public:
    enum {
        MaximumZoom = 16
    };

    enum AnimationMethod {
        NoAnimation,
        SoftwareAnimation,
        GLAnimation
    };

    /**
     * Create a new view attached to scene. We need the scene to be able to
     * install scene event filters.
     */
    DocumentView(QGraphicsScene* scene);
    ~DocumentView();

    Document::Ptr document() const;

    KUrl url() const;

    void openUrl(const KUrl&);

    /**
     * Tells the current adapter to load its config. Used when the user changed
     * the config while the view was visible.
     */
    void loadAdapterConfig();

    /**
     * Returns true if an adapter is loaded (note: adapters are also used to
     * display error messages!)
     */
    bool isEmpty() const;

    bool canZoom() const;

    qreal minimumZoom() const;

    qreal zoom() const;

    bool isCurrent() const;

    void setCurrent(bool);

    void setCompareMode(bool);

    bool zoomToFit() const;

    QPoint position() const;

    /**
     * Returns the RasterImageView of the current adapter, if it has one
     */
    RasterImageView* imageView() const;

    AbstractRasterImageViewTool* currentTool() const;

    void moveTo(const QRect&);
    void moveToAnimated(const QRect&);
    void fadeIn();
    void fadeOut();

    void setGeometry(const QRectF& rect); // reimp

public Q_SLOTS:
    void setZoom(qreal);

    void setZoomToFit(bool);

    void setPosition(const QPoint&);

Q_SIGNALS:
    /**
     * Emitted when the part has finished loading
     */
    void completed();

    void previousImageRequested();

    void nextImageRequested();

    void captionUpdateRequested(const QString&);

    void toggleFullScreenRequested();

    void videoFinished();

    void minimumZoomChanged(qreal);

    void zoomChanged(qreal);

    void adapterChanged();

    void focused(DocumentView*);

    void zoomToFitChanged(bool);

    void positionChanged();

    void hudTrashClicked(DocumentView*);
    void hudDeselectClicked(DocumentView*);

    void animationFinished(DocumentView*);

    void contextMenuRequested();

    void currentToolChanged(AbstractRasterImageViewTool*);

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
    void wheelEvent(QGraphicsSceneWheelEvent* event);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);
    bool sceneEventFilter(QGraphicsItem*, QEvent*);

private Q_SLOTS:
    void finishOpenUrl();
    void slotCompleted();
    void slotLoadingFailed();

    void zoomActualSize();

    void zoomIn(const QPointF& center = QPointF(-1, -1));
    void zoomOut(const QPointF& center = QPointF(-1, -1));

    void slotZoomChanged(qreal);

    void slotBusyChanged(const KUrl&, bool);

    void emitHudTrashClicked();
    void emitHudDeselectClicked();
    void emitFocused();

    void slotAnimationFinished();

private:
    friend struct DocumentViewPrivate;
    DocumentViewPrivate* const d;

    void createAdapterForDocument();
};

} // namespace

#endif /* DOCUMENTVIEW_H */
