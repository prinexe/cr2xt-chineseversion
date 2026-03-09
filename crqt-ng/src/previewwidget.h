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

#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QWidget>
#else
#include <QtGui/QWidget>
#endif
#include <QPixmap>
#include <QPoint>
#include <QImage>

/**
 * @brief Preview widget for XT* export dialog
 *
 * Fixed-size widget (540x960) that displays a scaled preview image
 * with mouse panning support when the zoomed image exceeds bounds.
 */
class PreviewWidget : public QWidget
{
    Q_OBJECT

public:
    /// Minimum widget width for small screen compatibility
    static constexpr int MIN_WIDTH = 200;

    /// Minimum widget height for small screen compatibility
    static constexpr int MIN_HEIGHT = 300;

    /// Zoom step for mouse wheel (percentage points)
    static constexpr int ZOOM_WHEEL_STEP = 25;

    /// Zoom step for keyboard shortcuts (percentage points)
    static constexpr int ZOOM_KEY_STEP = 10;

signals:
    /**
     * @brief Emitted when user scrolls without modifier to change page
     * @param delta Page delta: +1 for next page (scroll down), -1 for previous (scroll up)
     */
    void pageChangeRequested(int delta);

    /**
     * @brief Emitted when user presses Home key to go to first page
     */
    void firstPageRequested();

    /**
     * @brief Emitted when user presses End key to go to last page
     */
    void lastPageRequested();

    /**
     * @brief Emitted when user scrolls with Ctrl/Cmd modifier to change zoom
     * @param delta Zoom delta in percentage points (positive = zoom in, negative = zoom out)
     */
    void zoomChangeRequested(int delta);

    /**
     * @brief Emitted when user double-clicks to reset zoom to 100%
     */
    void zoomResetRequested();

    /**
     * @brief Emitted when user double-clicks to request resizing to preferred size
     *
     * This signal is emitted together with zoomResetRequested() when the user
     * double-clicks the preview. The parent dialog should resize to accommodate
     * the preview widget at its preferred size (document size at 100% zoom).
     */
    void preferredSizeRequested();

public:
    /// Zoom levels for toggle button
    static constexpr int ZOOM_LEVELS[] = {200, 500, 1000};
    static constexpr int ZOOM_LEVELS_COUNT = sizeof(ZOOM_LEVELS) / sizeof(int);
    static constexpr int ZOOM_LEVEL_MAX = ZOOM_LEVELS[ZOOM_LEVELS_COUNT - 1];
    static constexpr int ZOOM_LEVEL_GRID = 600;

    explicit PreviewWidget(QWidget* parent = nullptr);

    /**
     * @brief Set document dimensions for size calculation
     * @param width Document width in pixels
     * @param height Document height in pixels
     */
    void setDocumentSize(int width, int height);

    /**
     * @brief Set the image to display with zoom level
     * @param image Image to display
     * @param zoomPercent Zoom percentage (50-200)
     */
    void setImage(const QImage& image, int zoomPercent);

    /**
     * @brief Update zoom level without changing image
     * @param zoomPercent Zoom percentage (50-200)
     * @note Resets pan offset to center
     */
    void setZoom(int zoomPercent);

    /**
     * @brief Show centered message (for loading/error states)
     * @param message Message to display
     */
    void showMessage(const QString& message);

    /**
     * @brief Clear the preview and show default state
     */
    void clear();

    /**
     * @brief Enable or disable page navigation via mouse wheel
     * @param enabled true to enable (default), false to disable
     *
     * When disabled, scroll wheel without modifier does nothing.
     * Zoom via Ctrl+wheel is not affected.
     */
    void setPageNavigationEnabled(bool enabled);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /**
     * @brief Get DPI-aware widget size
     * @return Widget size in logical pixels that corresponds to MAX_DOC physical pixels
     */
    QSize dpiAwareSize() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void copyImageToClipboard();

private:
    /**
     * @brief Check if panning is possible (image exceeds bounds)
     */
    bool isPanningEnabled() const;

    /**
     * @brief Calculate centered position for image
     */
    QPoint calculateCenteredPosition() const;

    /**
     * @brief Clamp pan offset to keep image within bounds
     */
    void clampPanOffset();

    /**
     * @brief Update scaled pixmap from source image
     */
    void updateScaledPixmap();

    /**
     * @brief Update cursor based on panning state
     */
    void updateCursor();

    /**
     * @brief Create scaled image with integrated pixel grid for high zoom levels
     * @param targetWidth Target width
     * @param targetHeight Target height
     * @return Scaled image with grid lines built into it
     */
    QImage createScaledImageWithGrid(int targetWidth, int targetHeight);

    /**
     * @brief Process scroll event with device-aware accumulation
     * @param event Wheel event to process
     * @param accumulator Reference to accumulator for this scroll type
     * @param threshold Accumulation threshold for trackpad gestures
     * @return Number of discrete steps (positive or negative), 0 if threshold not reached
     */
    int processScrollEvent(QWheelEvent* event, int& accumulator, int threshold);

    QImage m_sourceImage;           ///< Original image
    QPixmap m_scaledPixmap;         ///< Image after zoom applied
    QSize m_logicalSize;            ///< Logical size of scaled pixmap (for centering/panning)
    QSize m_lastImageSize;          ///< Last image size for pan preservation
    QSize m_docSize;                ///< Current document size in pixels
    QPoint m_panOffset;             ///< Current pan position in pixels
    QPoint m_dragStart;             ///< For mouse drag tracking
    int m_zoomPercent;              ///< Current zoom level (50-200)
    QString m_message;              ///< Message to display (empty = show image)
    bool m_isDragging;              ///< True when dragging
    bool m_pageNavigationEnabled;   ///< True to allow page change via wheel
    int m_pageScrollAccumulator;    ///< Accumulated scroll delta for page navigation (trackpad)
    int m_zoomScrollAccumulator;    ///< Accumulated scroll delta for zoom (trackpad)
};

#endif // PREVIEWWIDGET_H
