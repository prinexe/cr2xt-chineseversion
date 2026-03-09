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

#ifndef XTEXPORTDLG_H
#define XTEXPORTDLG_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif

#include <QSet>
#include <QStringList>

#include <lvbasedrawbuf.h>
#include <xtcexport.h>

class QTimer;
class XtExportProfileManager;
class XtExportProfile;
class PreviewWidget;
class LVDocView;
class XtExportDlg;

namespace Ui
{
    class XtExportDlg;
}

/**
 * @brief Qt-compatible export callback for progress updates and cancellation
 *
 * This callback class bridges the XtcExporter's callback interface with Qt's
 * event loop. It calls processEvents() to keep the UI responsive during export
 * and provides cancellation support via a flag that the dialog can set.
 */
class QtExportCallback : public XtcExportCallback
{
public:
    explicit QtExportCallback(XtExportDlg* dialog);

    void onProgress(int percent) override;
    bool isCancelled() override;

    void setCancelled(bool cancelled) { m_cancelled = cancelled; }
    bool wasCancelled() const { return m_cancelled; }

private:
    XtExportDlg* m_dialog;
    bool m_cancelled;
};

/**
 * @brief Export dialog for XT* formats (XTC/XTCH)
 *
 * Full-featured export dialog with live preview, profile management,
 * and integrated progress tracking.
 */
class XtExportDlg : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(XtExportDlg)

    friend class QtExportCallback;

public:
    explicit XtExportDlg(QWidget* parent, LVDocView* docView);
    ~XtExportDlg();

public slots:
    void reject() override;

protected:
    void changeEvent(QEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

private slots:
    // Profile
    void onProfileChanged(int index);

    // Format settings
    void onFormatChanged(int index);
    void onWidthChanged(int value);
    void onHeightChanged(int value);

    // Dithering settings
    void onGrayPolicyChanged(int index);
    void onDitherModeChanged(int index);
    void onThresholdSliderChanged(int value);
    void onThresholdSpinBoxChanged(double value);
    void onErrorDiffusionSliderChanged(int value);
    void onErrorDiffusionSpinBoxChanged(double value);
    void onGammaSliderChanged(int value);
    void onGammaSpinBoxChanged(double value);
    void onSerpentineChanged(Qt::CheckState state);
    void onResetDithering();

    // Page range
    void onFromPageChanged(int value);
    void onToPageChanged(int value);

    // Chapters
    void onChaptersEnabledChanged(Qt::CheckState state);
    void onChapterDepthChanged(int index);

    // Invert mode
    void onInvertChanged(Qt::CheckState state);

    // Preview navigation
    void onFirstPage();
    void onPrevPage();
    void onNextPage();
    void onLastPage();
    void onPreviewPageChanged(int value);

    // Zoom controls
    void onZoomSliderChanged(int value);
    void onZoomSpinBoxChanged(int value);
    void onResetZoom();
    void onSet200Zoom();

    // Preview widget wheel events
    void onPreviewPageChangeRequested(int delta);
    void onPreviewZoomChangeRequested(int delta);
    void onPreviewPreferredSizeRequested();

    // Output path
    void onBrowseOutput();

    // Batch mode
    void onBrowseSource();

    // Actions
    void onExport();
    void onCancel();

    // Export progress (called by QtExportCallback)
    void updateExportProgress(int percent);

private:
    /**
     * @brief Setup navigation button icons with Unicode fallback
     *
     * Checks if theme icons were loaded successfully.
     * If icons are missing (common on Linux AppImage), falls back to Unicode symbols.
     */
    void setupNavigationIcons();

    /**
     * @brief Initialize profile dropdown from ProfileManager
     */
    void initProfiles();

    /**
     * @brief Load settings from profile to UI controls
     */
    void loadProfileToUi(XtExportProfile* profile);

    /**
     * @brief Update dithering controls enable state based on dither mode
     */
    void updateDitheringControlsState();

    /**
     * @brief Update gray policy enable state based on format
     */
    void updateGrayPolicyState();

    /**
     * @brief Update format/width/height controls enable state based on profile lock
     * @param profile Current profile (nullptr defaults to enabled)
     */
    void updateFormatControlsLockState(XtExportProfile* profile);

    /**
     * @brief Update preview widget and container sizes based on current dimensions
     */
    void updatePreviewContainerSizes();

    /**
     * @brief Setup bidirectional sync between slider and spinbox
     */
    void setupSliderSpinBoxSync();

    /**
     * @brief Compute default output path (directory + filename)
     */
    QString computeDefaultOutputPath();

    /**
     * @brief Compute the base filename for export (without directory)
     * @return Filename with extension based on current profile
     */
    QString computeExportFilename();

    /**
     * @brief Resolve output path for export
     *
     * If the output path is a directory, appends the computed filename.
     * Otherwise returns the path as-is.
     *
     * @return Full path to export file
     */
    QString resolveExportPath();

    /**
     * @brief Resolve effective dither mode from UI selection
     *
     * For Floyd-Steinberg mode, automatically selects 1-bit or 2-bit
     * based on the current format selection (XTC=1-bit, XTCH=2-bit).
     *
     * @return The actual ImageDitherMode to use for export/preview
     */
    ImageDitherMode resolveEffectiveDitherMode() const;

    /**
     * @brief Get file extension from current profile
     * @return Extension string (e.g., "xtc", "xtch"), defaults to "xtc"
     */
    QString currentProfileExtension() const;

    /**
     * @brief Extract base filename, stripping archive extensions
     *
     * Handles double-extension archives like .fb2.zip, .epub.gz, etc.
     * For "book.fb2.zip" returns "book", for "book.fb2" returns "book".
     *
     * @param filePath Full path or filename to process
     * @return Base name without archive extensions
     */
    static QString extractBaseName(const QString& filePath);

    void resizeMainWindow();

    /**
     * @brief Schedule a debounced preview update
     */
    void schedulePreviewUpdate();

    /**
     * @brief Render preview page using XtcExporter
     */
    void renderPreview();

    /**
     * @brief Configure XtcExporter from current UI settings
     */
    void configureExporter(XtcExporter& exporter);

    /**
     * @brief Update page count display after resolution change
     */
    void updatePageCount(int totalPages);

    /**
     * @brief Load zoom setting from crui.ini
     */
    void loadZoomSetting();

    /**
     * @brief Save zoom setting to crui.ini
     */
    void saveZoomSetting();

    /**
     * @brief Update zoom button label based on current zoom level
     */
    void updateZoomButtonLabel();

    /**
     * @brief Load last profile setting from crui.ini
     */
    void loadLastProfileSetting();

    /**
     * @brief Save last profile setting to crui.ini
     */
    void saveLastProfileSetting();

    /**
     * @brief Load last export directory from crui.ini
     */
    void loadLastDirectorySetting();

    /**
     * @brief Save last export directory to crui.ini
     */
    void saveLastDirectorySetting();

    /**
     * @brief Load window geometry from crui.ini
     */
    void loadWindowGeometry();

    /**
     * @brief Save window geometry to crui.ini
     */
    void saveWindowGeometry();

    /**
     * @brief Clamp dialog size to fit within available screen bounds
     *
     * Called after loadWindowGeometry() to handle first-time launch
     * when no saved geometry exists and the dialog's default size
     * might exceed the screen dimensions.
     */
    void clampToScreenBounds();

    /**
     * @brief Save current UI settings back to the current profile
     */
    void saveCurrentProfileSettings();

    /**
     * @brief Update profile object from current UI state
     * @param profile Profile to update
     */
    void updateProfileFromUi(XtExportProfile* profile);

    /**
     * @brief Load document metadata (title, author) into UI fields
     *
     * Called during dialog initialization to populate metadata fields
     * from the current document's properties.
     */
    void loadDocumentMetadata();

    /**
     * @brief Set dialog UI state for exporting mode
     *
     * Disables settings controls, shows progress bar, and changes
     * button states during export operation.
     *
     * @param exporting true when export is in progress
     */
    void setExporting(bool exporting);

    /**
     * @brief Set the dialog state for async file scanning
     *
     * Shows progress UI (indeterminate progress bar), disables controls,
     * and enables cancellation.
     *
     * @param scanning true when scanning is in progress
     */
    void setScanning(bool scanning);

    /**
     * @brief Validate export settings before starting
     * @return true if settings are valid, false otherwise (shows error message)
     */
    bool validateExportSettings();

    /**
     * @brief Perform the actual export operation
     *
     * This method is called after validation. It runs the export synchronously
     * with progress updates via QtExportCallback.
     */
    void performExport();

    // Batch mode
    /**
     * @brief Set the export mode (single file or batch)
     * @param batchMode true for batch export, false for single file
     */
    void setMode(bool batchMode);

    /**
     * @brief Handle mode change - update UI visibility and button text
     */
    void onModeChanged();

    // Batch mode settings persistence
    /**
     * @brief Load batch mode settings from crui.ini
     */
    void loadBatchSettings();

    /**
     * @brief Save batch mode settings to crui.ini
     */
    void saveBatchSettings();

    // Invert mode settings and color management

    /**
     * @brief Load inverse setting from crui.ini
     */
    void loadInvertSetting();

    /**
     * @brief Save inverse setting to crui.ini
     */
    void saveInvertSetting();

    /**
     * @brief Apply inverse colors to document if inverse mode is enabled
     *
     * Swaps text and background colors in the document view.
     * Stores original colors for restoration.
     */
    void applyInvertColors();

    /**
     * @brief Restore original document colors
     *
     * Called when dialog closes or inverse mode is disabled.
     */
    void restoreOriginalColors();

    /**
     * @brief Compute default paths for batch mode from current document
     *
     * If source directory is empty, uses current document's directory.
     * If output directory is empty in batch mode, uses source directory.
     */
    void computeDefaultBatchPaths();

    // Directory scanning for batch mode

    /**
     * @brief Parse file mask string into list of wildcard patterns
     * @return List of trimmed wildcard patterns (e.g., "*.epub", "*.fb2")
     */
    QStringList parseFileMask() const;

    /**
     * @brief Check if filename matches any pattern in the list
     * @param fileName Filename to check (not full path)
     * @param patterns List of wildcard patterns
     * @return true if filename matches at least one pattern
     */
    bool matchesFileMask(const QString& fileName, const QStringList& patterns) const;

    /**
     * @brief Find all files matching the file mask in a directory
     *
     * This method processes the directory incrementally, calling processEvents()
     * periodically to keep the UI responsive and check for cancellation.
     *
     * @param directory Directory to scan recursively
     * @return true if scan completed, false if cancelled
     */
    bool findMatchingFiles(const QString& directory);

    /**
     * @brief Scan source directory and populate m_batchFiles
     *
     * Shows warning messages if directory is empty/missing or no files found.
     * @return true if scan completed successfully, false if cancelled or no files found
     */
    bool scanSourceDirectory();

    /**
     * @brief Compute output path for a batch file
     * @param sourceFile Full path to source file
     * @return Full path to output file (with subdirs created if needed)
     */
    QString computeBatchOutputPath(const QString& sourceFile);

    /**
     * @brief Resolve filename collision in flat mode
     *
     * If the base path already exists or was used in this batch,
     * appends _1, _2, etc. until a unique path is found.
     *
     * @param basePath Desired output path
     * @return Unique output path (may be same as input if no collision)
     */
    QString resolveFilenameCollision(const QString& basePath);

    // Batch export execution (Phase 5)

    /**
     * @brief Validate batch export settings before starting
     * @return true if settings are valid, false otherwise (shows error message)
     */
    bool validateBatchSettings();

    /**
     * @brief Perform batch export of all matching files
     *
     * Scans source directory, loads each document, exports it,
     * and shows summary dialog when complete.
     */
    void performBatchExport();

    /**
     * @brief Check if a file should be overwritten based on overwrite mode
     * @param outputPath Path to the output file
     * @return true if file should be overwritten/created, false to skip
     */
    bool checkOverwriteFile(const QString& outputPath);

    /**
     * @brief Show overwrite confirmation dialog with multiple options
     * @param outputPath Path to the existing file
     * @return true to overwrite, false to skip
     */
    bool showOverwriteDialog(const QString& outputPath);

    /**
     * @brief Load a document for batch export
     * @param filePath Path to the document file
     * @return true if document was loaded successfully
     */
    bool loadDocumentForBatch(const QString& filePath);

    /**
     * @brief Export a single file (used by batch export)
     * @param outputPath Path to the output file
     * @return true if export was successful
     */
    bool exportSingleFile(const QString& outputPath);

    /**
     * @brief Reload the original document after batch export
     */
    void reloadOriginalDocument();

    /**
     * @brief Show batch export summary dialog
     */
    void showBatchSummary();

    Ui::XtExportDlg* m_ui;
    LVDocView* m_docView;
    XtExportProfileManager* m_profileManager;
    PreviewWidget* m_previewWidget;

    int m_currentPreviewPage;       ///< Current preview page (0-based)
    int m_totalPageCount;           ///< Total page count at export resolution
    int m_zoomPercent;              ///< Current zoom level (50-200)
    bool m_updatingControls;        ///< Flag to prevent signal loops
    QString m_lastDirectory;        ///< Last export directory

    // Debounce timer for preview updates (Phase 3)
    QTimer* m_previewDebounceTimer;

    // Export state (Phase 5)
    bool m_exporting;                   ///< true during export operation
    QtExportCallback* m_exportCallback; ///< Callback for progress and cancellation
    QString m_exportFilePath;           ///< Full path to export file (for cleanup on cancel)

    // Batch mode state
    bool m_batchMode;                   ///< true when in batch export mode
    QStringList m_batchFiles;           ///< List of files to export in batch mode
    QSet<QString> m_usedOutputPaths;    ///< Track used output paths for collision resolution

    // Scanning state (for async file collection)
    bool m_scanning;                    ///< true during async file scanning
    bool m_scanCancelled;               ///< true if scan was cancelled by user

    // Batch export execution state (Phase 5)
    int m_batchCurrentIndex;            ///< Index of currently exporting file
    int m_batchSuccessCount;            ///< Number of successful exports
    int m_batchSkipCount;               ///< Number of skipped files
    int m_batchErrorCount;              ///< Number of failed exports
    bool m_batchOverwriteAll;           ///< Runtime flag: overwrite all remaining files
    bool m_batchSkipAll;                ///< Runtime flag: skip all remaining existing files
    QString m_originalWindowTitle;      ///< Store original window title during batch
    QString m_originalDocumentPath;     ///< Store original document path for reload after batch
    int m_originalPreviewPage;          ///< Store original preview page for restore after batch

    // Invert mode state
    bool m_inverseEnabled;              ///< true when inverse mode checkbox is checked
    bool m_colorsInverted;              ///< true when colors are currently inverted
    lUInt32 m_originalTextColor;        ///< Original text color before inversion
    lUInt32 m_originalBackgroundColor;  ///< Original background color before inversion
};

#endif // XTEXPORTDLG_H
