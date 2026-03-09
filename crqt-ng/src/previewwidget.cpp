/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2025 Serge Baranov                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#include "previewwidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QClipboard>
#include <QApplication>

PreviewWidget::PreviewWidget(QWidget* parent)
    : QWidget(parent)
    , m_docSize(540, 960)  // Default document size
    , m_zoomPercent(100)
    , m_isDragging(false)
    , m_pageNavigationEnabled(true)
    , m_pageScrollAccumulator(0)
    , m_zoomScrollAccumulator(0)
{
    // Size will be set by caller via setDocumentSize() after DPR is known
    setMinimumSize(m_docSize.width() / 4, m_docSize.height() / 4);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);  // Focusable via click and Tab key
    m_message = tr("No document loaded");
}

void PreviewWidget::setDocumentSize(int width, int height)
{
    m_docSize = QSize(qMax(1, width), qMax(1, height));
}

void PreviewWidget::setImage(const QImage& image, int zoomPercent)
{
    bool sameSize = (m_lastImageSize == image.size());
    m_sourceImage = image;
    m_lastImageSize = image.size();
    m_zoomPercent = qBound(50, zoomPercent, ZOOM_LEVEL_MAX);
    m_message.clear();
    updateScaledPixmap();
    if (!sameSize) {
        m_panOffset = QPoint(0, 0);
    }
    updateCursor();
    update();
}

void PreviewWidget::setZoom(int zoomPercent)
{
    int oldZoom = m_zoomPercent;
    m_zoomPercent = qBound(50, zoomPercent, ZOOM_LEVEL_MAX);
    // Scale pan offset proportionally to maintain view position
    if (oldZoom > 0 && oldZoom != m_zoomPercent) {
        m_panOffset = m_panOffset * m_zoomPercent / oldZoom;
    }
    updateScaledPixmap();
    clampPanOffset();
    updateCursor();
    update();
}

void PreviewWidget::showMessage(const QString& message)
{
    m_message = message;
    m_sourceImage = QImage();
    m_scaledPixmap = QPixmap();
    m_logicalSize = QSize();
    setCursor(Qt::ArrowCursor);
    update();
}

void PreviewWidget::clear()
{
    showMessage(tr("No document loaded"));
}

void PreviewWidget::setPageNavigationEnabled(bool enabled)
{
    m_pageNavigationEnabled = enabled;
}

QSize PreviewWidget::dpiAwareSize() const
{
    qreal dpr = devicePixelRatioF();
    // Return logical size that will result in m_docSize physical pixels
    return QSize(qRound(m_docSize.width() / dpr), qRound(m_docSize.height() / dpr));
}

QSize PreviewWidget::sizeHint() const
{
    return dpiAwareSize();
}

QSize PreviewWidget::minimumSizeHint() const
{
    // Return a small fixed minimum to allow dialog to fit on small screens
    // The preview is scrollable/pannable so users can navigate larger content
    return QSize(MIN_WIDTH, MIN_HEIGHT);
}

void PreviewWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    // Fill background with light gray
    painter.fillRect(rect(), QColor(200, 200, 200));

    if (!m_message.isEmpty()) {
        // Show centered message
        painter.setPen(Qt::darkGray);
        painter.drawText(rect(), Qt::AlignCenter, m_message);
        return;
    }

    if (m_scaledPixmap.isNull()) {
        return;
    }

    // Draw scaled image centered with pan offset
    QPoint drawPos = calculateCenteredPosition() + m_panOffset;
    painter.drawPixmap(drawPos, m_scaledPixmap);
}

void PreviewWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && isPanningEnabled()) {
        m_isDragging = true;
        m_dragStart = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void PreviewWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        m_panOffset += event->pos() - m_dragStart;
        m_dragStart = event->pos();
        clampPanOffset();
        update();
    }
}

void PreviewWidget::mouseReleaseEvent(QMouseEvent* /*event*/)
{
    if (m_isDragging) {
        m_isDragging = false;
        updateCursor();
    }
}

void PreviewWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // Reset pan offset to center
        m_panOffset = QPoint(0, 0);
        emit zoomResetRequested();
        emit preferredSizeRequested();
        event->accept();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

int PreviewWidget::processScrollEvent(QWheelEvent* event, int& accumulator, int threshold)
{
    int delta = event->angleDelta().y();
    if (delta == 0)
        return 0;

    // Use phase() to distinguish device types:
    // - NoScrollPhase: discrete device (mouse wheel) - one step per event
    // - Other phases: continuous device (trackpad gesture) - accumulate
    if (event->phase() == Qt::NoScrollPhase) {
        // Mouse wheel: always exactly one step per click, ignore delta magnitude
        accumulator = 0;
        return (delta > 0) ? 1 : -1;
    }

    // Trackpad gesture: accumulate small deltas until threshold
    accumulator += delta;
    int steps = 0;

    while (accumulator >= threshold) {
        steps++;
        accumulator -= threshold;
    }
    while (accumulator <= -threshold) {
        steps--;
        accumulator += threshold;
    }

    // Reset on gesture end to prevent carryover between gestures
    if (event->phase() == Qt::ScrollEnd)
        accumulator = 0;

    return steps;
}

void PreviewWidget::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() == 0) {
        event->ignore();
        return;
    }

    constexpr int SCROLL_THRESHOLD = 120;

    // Zoom modifier: Ctrl on Windows/Linux, Option (Alt) on macOS
    // (Cmd+scroll is captured by macOS system for accessibility zoom)
#ifdef Q_OS_MACOS
    bool zoomModifier = event->modifiers() & Qt::AltModifier;
#else
    bool zoomModifier = event->modifiers() & Qt::ControlModifier;
#endif

    if (zoomModifier) {
        // Ctrl/Cmd + scroll: zoom control
        int steps = processScrollEvent(event, m_zoomScrollAccumulator, SCROLL_THRESHOLD);
        if (steps != 0)
            emit zoomChangeRequested(steps * ZOOM_WHEEL_STEP);
    } else if (m_pageNavigationEnabled) {
        // Plain scroll: page navigation
        int steps = processScrollEvent(event, m_pageScrollAccumulator, SCROLL_THRESHOLD);
        if (steps != 0)
            emit pageChangeRequested(-steps);  // Positive scroll = previous page
    }

    event->accept();
}

void PreviewWidget::keyPressEvent(QKeyEvent* event)
{
    // Zoom controls (always available, even when page navigation is disabled)
    // Keyboard uses Cmd on macOS (works fine, unlike Cmd+scroll which is system-captured)
    bool zoomModifier = event->modifiers() & Qt::ControlModifier;

    if (zoomModifier) {
        switch (event->key()) {
            case Qt::Key_Plus:
            case Qt::Key_Equal:  // = key (same key as + without Shift on US keyboards)
                emit zoomChangeRequested(ZOOM_KEY_STEP);
                event->accept();
                return;
            case Qt::Key_Minus:
                emit zoomChangeRequested(-ZOOM_KEY_STEP);
                event->accept();
                return;
            default:
                break;
        }
    }

    // Reset zoom with 0 key (no modifier)
    if (event->key() == Qt::Key_0 && !zoomModifier) {
        emit zoomResetRequested();
        event->accept();
        return;
    }

    // Page navigation (when enabled)
    if (!m_pageNavigationEnabled) {
        QWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
        case Qt::Key_Right:
        case Qt::Key_Down:
        case Qt::Key_PageDown:
            emit pageChangeRequested(1);  // Next page
            event->accept();
            break;
        case Qt::Key_Left:
        case Qt::Key_Up:
        case Qt::Key_PageUp:
            emit pageChangeRequested(-1);  // Previous page
            event->accept();
            break;
        case Qt::Key_Home:
            emit firstPageRequested();
            event->accept();
            break;
        case Qt::Key_End:
            emit lastPageRequested();
            event->accept();
            break;
        default:
            QWidget::keyPressEvent(event);
            break;
    }
}

bool PreviewWidget::isPanningEnabled() const
{
    if (m_logicalSize.isEmpty())
        return false;
    return m_logicalSize.width() > width() || m_logicalSize.height() > height();
}

QPoint PreviewWidget::calculateCenteredPosition() const
{
    int x = (width() - m_logicalSize.width()) / 2;
    int y = (height() - m_logicalSize.height()) / 2;
    return QPoint(x, y);
}

void PreviewWidget::clampPanOffset()
{
    if (m_logicalSize.isEmpty())
        return;

    // Calculate overflow in each direction
    int overflowX = m_logicalSize.width() - width();
    int overflowY = m_logicalSize.height() - height();

    if (overflowX <= 0) {
        // Image fits horizontally - no panning allowed
        m_panOffset.setX(0);
    } else {
        // Calculate how far we can pan in each direction
        // Centered position is negative when image is larger than widget
        // Pan offset moves image: positive = image moves right (shows left side)
        //                         negative = image moves left (shows right side)
        int maxPanLeft = overflowX / 2;          // Can pan left (positive offset)
        int maxPanRight = overflowX - maxPanLeft; // Can pan right (negative offset)
        m_panOffset.setX(qBound(-maxPanRight, m_panOffset.x(), maxPanLeft));
    }

    if (overflowY <= 0) {
        // Image fits vertically - no panning allowed
        m_panOffset.setY(0);
    } else {
        int maxPanUp = overflowY / 2;
        int maxPanDown = overflowY - maxPanUp;
        m_panOffset.setY(qBound(-maxPanDown, m_panOffset.y(), maxPanUp));
    }
}

void PreviewWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // Re-clamp pan offset when widget size changes
    clampPanOffset();
    updateCursor();
}

void PreviewWidget::updateScaledPixmap()
{
    if (m_sourceImage.isNull()) {
        m_scaledPixmap = QPixmap();
        m_logicalSize = QSize();
        return;
    }

    qreal dpr = devicePixelRatioF();
    int targetWidth = m_sourceImage.width() * m_zoomPercent / 100;
    int targetHeight = m_sourceImage.height() * m_zoomPercent / 100;

    if (targetWidth <= 0 || targetHeight <= 0) {
        m_scaledPixmap = QPixmap();
        m_logicalSize = QSize();
        return;
    }

    QImage scaled;

    if (m_zoomPercent >= ZOOM_LEVEL_GRID) {
        // High zoom: create scaled image with integrated grid
        scaled = createScaledImageWithGrid(targetWidth, targetHeight);
    } else if (m_zoomPercent <= 100) {
        scaled = m_sourceImage.scaled(targetWidth, targetHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        scaled = m_sourceImage.scaled(targetWidth, targetHeight, Qt::KeepAspectRatio, Qt::FastTransformation);
    }

    m_scaledPixmap = QPixmap::fromImage(scaled);
    m_scaledPixmap.setDevicePixelRatio(dpr);
    m_logicalSize = QSize(qRound(scaled.width() / dpr), qRound(scaled.height() / dpr));
}

void PreviewWidget::updateCursor()
{
    if (isPanningEnabled() && !m_isDragging) {
        setCursor(Qt::OpenHandCursor);
    } else if (!m_isDragging) {
        setCursor(Qt::ArrowCursor);
    }
}

QImage PreviewWidget::createScaledImageWithGrid(int targetWidth, int targetHeight)
{
    int srcW = m_sourceImage.width();
    int srcH = m_sourceImage.height();

    // Ensure source is grayscale for fast scanLine access
    QImage srcGray = m_sourceImage.format() == QImage::Format_Grayscale8
                         ? m_sourceImage
                         : m_sourceImage.convertToFormat(QImage::Format_Grayscale8);

    QImage result(targetWidth, targetHeight, QImage::Format_Grayscale8);

    // Grid blending: 60% pixel + 40% grid (gridGray=128)
    // blended = (pixel * 156 + 128 * 100) / 256 = (pixel * 156 + 12800) / 256
    constexpr int gridGray = 128;
    constexpr int pixelWeight = 156;
    constexpr int gridWeight = 100;

    // Process by source rows - use memset for runs of identical pixels
    for (int sy = 0; sy < srcH; ++sy) {
        const uchar* srcLine = srcGray.constScanLine(sy);

        // Destination row range for this source row
        int dyStart = sy * targetHeight / srcH;
        int dyEnd = (sy + 1) * targetHeight / srcH;
        int gridRowY = dyEnd - 1;  // Last row is grid row

        for (int dy = dyStart; dy < dyEnd; ++dy) {
            uchar* dstLine = result.scanLine(dy);
            bool isGridRow = (dy == gridRowY);

            // Process by source columns
            for (int sx = 0; sx < srcW; ++sx) {
                uchar pixel = srcLine[sx];
                uchar blended = static_cast<uchar>((pixel * pixelWeight + gridGray * gridWeight) >> 8);

                // Destination column range for this source pixel
                int dxStart = sx * targetWidth / srcW;
                int dxEnd = (sx + 1) * targetWidth / srcW;
                int runLen = dxEnd - dxStart;

                if (isGridRow) {
                    // Entire run is blended
                    memset(dstLine + dxStart, blended, runLen);
                } else {
                    // Fill with pixel color, last column is grid
                    if (runLen > 1) {
                        memset(dstLine + dxStart, pixel, runLen - 1);
                    }
                    dstLine[dxEnd - 1] = blended;
                }
            }
        }
    }

    return result;
}

void PreviewWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu;  // No parent - use default system style
    QAction* copyAction = menu.addAction(tr("Copy"));
    copyAction->setEnabled(!m_sourceImage.isNull());
    connect(copyAction, &QAction::triggered, this, &PreviewWidget::copyImageToClipboard);
    menu.exec(event->globalPos());
}

void PreviewWidget::copyImageToClipboard()
{
    if (!m_sourceImage.isNull()) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setImage(m_sourceImage);
    }
}
