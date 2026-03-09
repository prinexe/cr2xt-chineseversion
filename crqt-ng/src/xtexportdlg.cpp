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

#include "xtexportdlg.h"
#include "ui_xtexportdlg.h"
#include "previewwidget.h"
#include "xtexportprofile.h"
#include "mainwindow.h"
#include "cr3widget.h"
#include "crqtutil.h"

#include <lvdocview.h>
#include <xtcexport.h>

#include <QTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QVBoxLayout>
#include <QStandardPaths>
#include <QMessageBox>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QPushButton>
#include <QScreen>
#include <QGuiApplication>
#include <QUrl>

// Default file mask for batch mode - common e-book formats
static const QString DEFAULT_FILE_MASK =
    "*.epub;*.fb2;*.fb2.zip";

// =============================================================================
// QtExportCallback implementation
// =============================================================================

QtExportCallback::QtExportCallback(XtExportDlg* dialog)
    : m_dialog(dialog)
    , m_cancelled(false)
{
}

void QtExportCallback::onProgress(int percent)
{
    if (m_dialog) {
        m_dialog->updateExportProgress(percent);
    }
    // Process events to keep UI responsive
    QCoreApplication::processEvents();
}

bool QtExportCallback::isCancelled()
{
    // Process events to check for cancel button clicks
    QCoreApplication::processEvents();
    return m_cancelled;
}

// =============================================================================
// XtExportDlg implementation
// =============================================================================

XtExportDlg::XtExportDlg(QWidget* parent, LVDocView* docView)
    : QDialog(parent)
    , m_ui(new Ui::XtExportDlg)
    , m_docView(docView)
    , m_profileManager(new XtExportProfileManager())
    , m_previewWidget(nullptr)
    , m_currentPreviewPage(0)
    , m_totalPageCount(1)
    , m_zoomPercent(100)
    , m_updatingControls(false)
    , m_lastDirectory()
    , m_previewDebounceTimer(nullptr)
    , m_exporting(false)
    , m_exportCallback(nullptr)
    , m_exportFilePath()
    , m_batchMode(false)
    , m_scanning(false)
    , m_scanCancelled(false)
    , m_batchCurrentIndex(0)
    , m_batchSuccessCount(0)
    , m_batchSkipCount(0)
    , m_batchErrorCount(0)
    , m_batchOverwriteAll(false)
    , m_batchSkipAll(false)
    , m_originalWindowTitle()
    , m_originalDocumentPath()
    , m_originalPreviewPage(0)
    , m_inverseEnabled(false)
    , m_colorsInverted(false)
    , m_originalTextColor(0)
    , m_originalBackgroundColor(0xFFFFFF)
{
    setAttribute(Qt::WA_DeleteOnClose);  // Ensure destructor runs when dialog closes
    m_ui->setupUi(this);

    // Setup navigation button icons with fallback to Unicode symbols
    // Theme icons may not be available on Linux AppImage without bundled icon themes
    setupNavigationIcons();

    // Set zoom controls max from constant (overrides .ui file values)
    m_ui->sliderZoom->setMaximum(PreviewWidget::ZOOM_LEVEL_MAX);
    m_ui->sbZoom->setMaximum(PreviewWidget::ZOOM_LEVEL_MAX);

    // Initialize profile manager
    m_profileManager->initialize();

    // Create and insert preview widget into placeholder
    m_previewWidget = new PreviewWidget(this);
    auto* previewContainerLayout = new QVBoxLayout(m_ui->previewPlaceholder);
    previewContainerLayout->setContentsMargins(0, 0, 0, 0);
    previewContainerLayout->addWidget(m_previewWidget);

    // Set vertical stretch on the previewWidgetLayout item (index 2) in previewLayout
    // so it expands to fill available vertical space
    // Layout items: 0=zoomLabelLayout, 1=zoomLayout, 2=previewWidgetLayout, 3=navigationLayout
    m_ui->previewLayout->setStretch(2, 1);

    // Preview container sizes will be set by updatePreviewContainerSizes()
    // after profiles are loaded and dimensions are known

    // Setup debounce timer for preview updates
    m_previewDebounceTimer = new QTimer(this);
    m_previewDebounceTimer->setSingleShot(true);
    m_previewDebounceTimer->setInterval(50);
    connect(m_previewDebounceTimer, &QTimer::timeout, this, &XtExportDlg::renderPreview);

    // Initialize profiles dropdown
    initProfiles();

    // Setup slider/spinbox synchronization
    setupSliderSpinBoxSync();

    // Connect profile change
    connect(m_ui->cbProfile, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &XtExportDlg::onProfileChanged);

    // Connect format settings
    connect(m_ui->cbFormat, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &XtExportDlg::onFormatChanged);
    connect(m_ui->sbWidth, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &XtExportDlg::onWidthChanged);
    connect(m_ui->sbHeight, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &XtExportDlg::onHeightChanged);

    // Connect dithering settings
    connect(m_ui->cbGrayPolicy, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &XtExportDlg::onGrayPolicyChanged);
    connect(m_ui->cbDitherMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &XtExportDlg::onDitherModeChanged);
    connect(m_ui->cbSerpentine, &QCheckBox::checkStateChanged,
            this, &XtExportDlg::onSerpentineChanged);
    connect(m_ui->btnResetDithering, &QPushButton::clicked,
            this, &XtExportDlg::onResetDithering);

    // Connect page range
    connect(m_ui->sbFromPage, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &XtExportDlg::onFromPageChanged);
    connect(m_ui->sbToPage, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &XtExportDlg::onToPageChanged);

    // Connect chapters
    connect(m_ui->cbChaptersEnabled, &QCheckBox::checkStateChanged,
            this, &XtExportDlg::onChaptersEnabledChanged);
    connect(m_ui->cbChapterDepth, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &XtExportDlg::onChapterDepthChanged);

    // Connect inverse mode
    connect(m_ui->cbInvert, &QCheckBox::checkStateChanged,
            this, &XtExportDlg::onInvertChanged);

    // Connect preview navigation
    connect(m_ui->btnFirstPage, &QPushButton::clicked,
            this, &XtExportDlg::onFirstPage);
    connect(m_ui->btnPrevPage, &QPushButton::clicked,
            this, &XtExportDlg::onPrevPage);
    connect(m_ui->btnNextPage, &QPushButton::clicked,
            this, &XtExportDlg::onNextPage);
    connect(m_ui->btnLastPage, &QPushButton::clicked,
            this, &XtExportDlg::onLastPage);
    connect(m_ui->sbPreviewPage, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &XtExportDlg::onPreviewPageChanged);

    // Connect zoom controls
    connect(m_ui->sliderZoom, &QSlider::valueChanged,
            this, &XtExportDlg::onZoomSliderChanged);
    connect(m_ui->sbZoom, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &XtExportDlg::onZoomSpinBoxChanged);
    connect(m_ui->btnResetZoom, &QPushButton::clicked,
            this, &XtExportDlg::onResetZoom);
    connect(m_ui->btn200Zoom, &QPushButton::clicked,
            this, &XtExportDlg::onSet200Zoom);

    // Connect preview widget mouse/keyboard events
    connect(m_previewWidget, &PreviewWidget::pageChangeRequested,
            this, &XtExportDlg::onPreviewPageChangeRequested);
    connect(m_previewWidget, &PreviewWidget::firstPageRequested,
            this, &XtExportDlg::onFirstPage);
    connect(m_previewWidget, &PreviewWidget::lastPageRequested,
            this, &XtExportDlg::onLastPage);
    connect(m_previewWidget, &PreviewWidget::zoomChangeRequested,
            this, &XtExportDlg::onPreviewZoomChangeRequested);
    connect(m_previewWidget, &PreviewWidget::zoomResetRequested,
            this, &XtExportDlg::onResetZoom);
    connect(m_previewWidget, &PreviewWidget::preferredSizeRequested,
            this, &XtExportDlg::onPreviewPreferredSizeRequested);

    // Connect output path
    connect(m_ui->btnBrowse, &QPushButton::clicked,
            this, &XtExportDlg::onBrowseOutput);

    // Connect mode radio buttons
    connect(m_ui->rbSingleFile, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) setMode(false);
    });
    connect(m_ui->rbBatchExport, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) setMode(true);
    });

    // Connect batch mode controls
    connect(m_ui->btnBrowseSource, &QPushButton::clicked,
            this, &XtExportDlg::onBrowseSource);

    // Connect actions
    connect(m_ui->btnExport, &QPushButton::clicked,
            this, &XtExportDlg::onExport);
    // Cancel is already connected via UI file to reject()

    // Load last export directory setting (before computing default output path)
    loadLastDirectorySetting();

    // Load last used profile (defaults to first profile if not saved)
    loadLastProfileSetting();

    // Populate title and author from document metadata
    loadDocumentMetadata();

    // Store original document path for restoring after batch export
    if (m_docView) {
        m_originalDocumentPath = cr2qt(m_docView->getFileName());
    }

    // Load batch mode settings (must be before computeDefaultOutputPath
    // as batch mode affects output path interpretation)
    loadBatchSettings();

    // Load inverse setting
    loadInvertSetting();

    // Compute default batch paths if needed
    computeDefaultBatchPaths();

    // Set default output path (only in single file mode, batch mode already handled)
    if (!m_batchMode) {
        m_ui->leOutputPath->setText(computeDefaultOutputPath());
    }

    // Load window geometry
    loadWindowGeometry();

    // Clamp to screen bounds (handles first-time launch when dialog's
    // default size might exceed available screen dimensions)
    clampToScreenBounds();

    // Update control states
    updateDitheringControlsState();
    updateGrayPolicyState();

    // Load saved zoom setting
    loadZoomSetting();

    // Set initial preview page to current document page
    if (m_docView) {
        int currentPage = m_docView->getCurPage();  // 0-based page number
        m_currentPreviewPage = currentPage;
        m_ui->sbPreviewPage->setValue(currentPage + 1);  // SpinBox is 1-based
    }

    // Trigger initial preview after a short delay (allow dialog to be fully shown)
    QTimer::singleShot(100, this, &XtExportDlg::renderPreview);
}

XtExportDlg::~XtExportDlg()
{
    // Restore original document colors if inverted
    restoreOriginalColors();

    // Save all settings to crui.ini
    saveZoomSetting();
    saveLastProfileSetting();
    saveLastDirectorySetting();
    saveWindowGeometry();
    saveBatchSettings();
    saveInvertSetting();

    // Save current profile settings to its INI file
    saveCurrentProfileSettings();

    delete m_profileManager;
    delete m_ui;
}

void XtExportDlg::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        m_ui->retranslateUi(this);
    }
}

void XtExportDlg::closeEvent(QCloseEvent* e)
{
    if (m_exporting) {
        // Trigger cancellation instead of closing
        if (m_exportCallback) {
            m_exportCallback->setCancelled(true);
        }
        e->ignore();
        return;
    }
    QDialog::closeEvent(e);
}

void XtExportDlg::reject()
{
    if (m_exporting) {
        // Trigger cancellation instead of closing
        if (m_exportCallback) {
            m_exportCallback->setCancelled(true);
        }
        return;
    }
    QDialog::reject();
}

void XtExportDlg::setupNavigationIcons()
{
    // Check if theme icons loaded (they may be missing on Linux AppImage)
    // Fall back to Unicode symbols if icons are null/empty
    struct ButtonFallback {
        QPushButton* button;
        const char* unicodeFallback;
    };

    ButtonFallback buttons[] = {
        {m_ui->btnFirstPage, "\u23EE"},  // Black left-pointing double triangle with vertical bar
        {m_ui->btnPrevPage, "\u25C0"},   // Black left-pointing triangle
        {m_ui->btnNextPage, "\u25B6"},   // Black right-pointing triangle
        {m_ui->btnLastPage, "\u23ED"}    // Black right-pointing double triangle with vertical bar
    };

    for (const auto& b : buttons) {
        if (b.button->icon().isNull()) {
            b.button->setText(QString::fromUtf8(b.unicodeFallback));
        }
    }
}

void XtExportDlg::initProfiles()
{
    m_ui->cbProfile->clear();
    QStringList names = m_profileManager->profileNames();
    m_ui->cbProfile->addItems(names);
}

void XtExportDlg::loadProfileToUi(XtExportProfile* profile)
{
    if (!profile)
        return;

    m_updatingControls = true;

    // Format settings
    m_ui->cbFormat->setCurrentIndex(profile->format == XTC_FORMAT_XTC ? 0 : 1);
    m_ui->sbWidth->setValue(profile->width);
    m_ui->sbHeight->setValue(profile->height);

    // Dithering settings
    // Gray policy: 0=Balanced, 1=Preserve Light, 2=High Contrast
    int grayPolicyIndex = 0;
    switch (profile->grayPolicy) {
        case GRAY_SPLIT_LIGHT_DARK: grayPolicyIndex = 0; break;
        case GRAY_ALL_TO_WHITE: grayPolicyIndex = 1; break;
        case GRAY_ALL_TO_BLACK: grayPolicyIndex = 2; break;
    }
    m_ui->cbGrayPolicy->setCurrentIndex(grayPolicyIndex);

    // Dither mode: 0=None, 1=Ordered, 2=Floyd-Steinberg (auto-selects 1-bit or 2-bit based on format)
    int ditherModeIndex = 0;
    switch (profile->ditherMode) {
        case IMAGE_DITHER_NONE: ditherModeIndex = 0; break;
        case IMAGE_DITHER_ORDERED: ditherModeIndex = 1; break;
        case IMAGE_DITHER_FS_1BIT:
        case IMAGE_DITHER_FS_2BIT: ditherModeIndex = 2; break;
    }
    m_ui->cbDitherMode->setCurrentIndex(ditherModeIndex);

    // FS parameters
    m_ui->sbThreshold->setValue(profile->ditheringOpts.threshold);
    m_ui->sliderThreshold->setValue(static_cast<int>(profile->ditheringOpts.threshold * 100));
    m_ui->sbErrorDiffusion->setValue(profile->ditheringOpts.errorDiffusion);
    m_ui->sliderErrorDiffusion->setValue(static_cast<int>(profile->ditheringOpts.errorDiffusion * 100));
    m_ui->sbGamma->setValue(profile->ditheringOpts.gamma);
    m_ui->sliderGamma->setValue(static_cast<int>(profile->ditheringOpts.gamma * 100));
    m_ui->cbSerpentine->setChecked(profile->ditheringOpts.serpentine);

    // Chapters
    m_ui->cbChaptersEnabled->setChecked(profile->chaptersEnabled);
    m_ui->cbChapterDepth->setCurrentIndex(profile->chapterDepth - 1);

    m_updatingControls = false;

    // Update control states
    updateDitheringControlsState();
    updateGrayPolicyState();
    updateFormatControlsLockState(profile);

    // Update preview container sizes based on new dimensions
    updatePreviewContainerSizes();
}

void XtExportDlg::updateDitheringControlsState()
{
    // FS parameters are enabled only for Floyd-Steinberg dithering mode (index 2)
    int modeIndex = m_ui->cbDitherMode->currentIndex();
    bool enableFs = (modeIndex == 2);

    m_ui->sliderThreshold->setEnabled(enableFs);
    m_ui->sbThreshold->setEnabled(enableFs);
    m_ui->sliderErrorDiffusion->setEnabled(enableFs);
    m_ui->sbErrorDiffusion->setEnabled(enableFs);
    m_ui->sliderGamma->setEnabled(enableFs);
    m_ui->sbGamma->setEnabled(enableFs);
    m_ui->cbSerpentine->setEnabled(enableFs);
}

void XtExportDlg::updateGrayPolicyState()
{
    // Gray policy is disabled for XTCH format (2-bit doesn't need gray-to-mono)
    int formatIndex = m_ui->cbFormat->currentIndex();
    bool enableGrayPolicy = (formatIndex == 0);  // XTC only
    m_ui->cbGrayPolicy->setEnabled(enableGrayPolicy);
}

void XtExportDlg::updateFormatControlsLockState(XtExportProfile* profile)
{
    // When profile is locked, format/width/height controls are disabled
    bool enabled = !profile || !profile->locked;
    m_ui->cbFormat->setEnabled(enabled);
    m_ui->sbWidth->setEnabled(enabled);
    m_ui->sbHeight->setEnabled(enabled);
}

void XtExportDlg::updatePreviewContainerSizes()
{
    // Update preview widget with current document dimensions
    int width = m_ui->sbWidth->value();
    int height = m_ui->sbHeight->value();
    m_previewWidget->setDocumentSize(width, height);

    // Use small fixed minimum sizes to allow dialog to fit on small screens (e.g., 1366x768)
    // The preview widget supports pan/zoom for navigating content larger than display area
    // sizeHint() still returns the ideal DPI-aware size for layout calculations
    m_previewWidget->setMinimumSize(PreviewWidget::MIN_WIDTH, PreviewWidget::MIN_HEIGHT);
    m_ui->previewPlaceholder->setMinimumSize(PreviewWidget::MIN_WIDTH, PreviewWidget::MIN_HEIGHT);

    // Frame adds 2px for border (1px each side)
    m_ui->previewFrame->setMinimumSize(PreviewWidget::MIN_WIDTH + 2, PreviewWidget::MIN_HEIGHT + 2);

    // Set previewGroup minimum width: must fit navigation controls
    // Navigation needs ~300px (4 buttons + spinbox + label + spacing)
    constexpr int minControlsWidth = 300;
    m_ui->previewGroup->setMinimumWidth(minControlsWidth);
}

void XtExportDlg::setupSliderSpinBoxSync()
{
    // Threshold: slider 0-100 maps to spinbox 0.00-1.00
    connect(m_ui->sliderThreshold, &QSlider::valueChanged,
            this, &XtExportDlg::onThresholdSliderChanged);
    connect(m_ui->sbThreshold, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &XtExportDlg::onThresholdSpinBoxChanged);

    // Error Diffusion: slider 0-100 maps to spinbox 0.00-1.00
    connect(m_ui->sliderErrorDiffusion, &QSlider::valueChanged,
            this, &XtExportDlg::onErrorDiffusionSliderChanged);
    connect(m_ui->sbErrorDiffusion, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &XtExportDlg::onErrorDiffusionSpinBoxChanged);

    // Gamma: slider 50-400 maps to spinbox 0.50-4.00
    connect(m_ui->sliderGamma, &QSlider::valueChanged,
            this, &XtExportDlg::onGammaSliderChanged);
    connect(m_ui->sbGamma, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &XtExportDlg::onGammaSpinBoxChanged);
}

QString XtExportDlg::computeExportFilename()
{
    QString baseName;

    // Get source document info
    if (m_docView) {
        QString fileName = cr2qt(m_docView->getFileName());
        if (!fileName.isEmpty()) {
            baseName = extractBaseName(fileName);
        }
    }

    // Fallback name (extractBaseName already handles empty case)
    if (baseName.isEmpty()) {
        baseName = "export";
    }

    return baseName + '.' + currentProfileExtension();
}

QString XtExportDlg::extractBaseName(const QString& filePath)
{
    QString path = filePath;

    // Handle archive paths like "archive.fb2.zip@/internal.fb2"
    // Extract just the archive path (before @)
    int atPos = path.indexOf('@');
    if (atPos > 0) {
        path = path.left(atPos);
    }

    QFileInfo fi(path);
    QString baseName = fi.completeBaseName();

    // For double-extension archives like .fb2.zip, strip inner extension too
    QString suffix = fi.suffix().toLower();
    if (suffix == "zip" || suffix == "gz" || suffix == "bz2") {
        QFileInfo innerInfo(baseName);
        if (!innerInfo.suffix().isEmpty()) {
            baseName = innerInfo.completeBaseName();
        }
    }

    return baseName.isEmpty() ? "export" : baseName;
}

QString XtExportDlg::computeDefaultOutputPath()
{
    QString directory;

    // Get source document directory
    if (m_docView) {
        QString fileName = cr2qt(m_docView->getFileName());
        if (!fileName.isEmpty()) {
            // Handle archive paths like "archive.fb2.zip@/internal.fb2"
            int atPos = fileName.indexOf('@');
            if (atPos > 0) {
                fileName = fileName.left(atPos);
            }

            QFileInfo docInfo(fileName);

            // Use last directory if set, otherwise source document directory
            if (!m_lastDirectory.isEmpty()) {
                directory = m_lastDirectory;
            } else {
                directory = docInfo.absolutePath();
            }
        }
    }

    // Fallback to Documents folder if no document
    if (directory.isEmpty()) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    // Build full path
    QString path = directory;
    if (!path.endsWith('/') && !path.endsWith('\\')) {
        path += '/';
    }
    path += computeExportFilename();

    return QDir::toNativeSeparators(path);
}

QString XtExportDlg::resolveExportPath()
{
    QString outputPath = m_ui->leOutputPath->text();

    if (outputPath.isEmpty()) {
        return computeDefaultOutputPath();
    }

    QFileInfo fi(outputPath);

    // If it's a directory, append the filename
    if (fi.isDir()) {
        QString path = outputPath;
        if (!path.endsWith('/') && !path.endsWith('\\')) {
            path += '/';
        }
        path += computeExportFilename();
        return QDir::toNativeSeparators(path);
    }

    // Otherwise return as-is (it's already a full file path)
    return QDir::toNativeSeparators(outputPath);
}

ImageDitherMode XtExportDlg::resolveEffectiveDitherMode() const
{
    int modeIndex = m_ui->cbDitherMode->currentIndex();
    switch (modeIndex) {
        case 0: return IMAGE_DITHER_NONE;
        case 1: return IMAGE_DITHER_ORDERED;
        case 2: {
            // Floyd-Steinberg: select 1-bit or 2-bit based on format
            // XTC (index 0) = 1-bit, XTCH (index 1) = 2-bit
            int formatIndex = m_ui->cbFormat->currentIndex();
            return (formatIndex == 0) ? IMAGE_DITHER_FS_1BIT : IMAGE_DITHER_FS_2BIT;
        }
        default: return IMAGE_DITHER_NONE;
    }
}

QString XtExportDlg::currentProfileExtension() const
{
    XtExportProfile* profile = m_profileManager->profileByIndex(m_ui->cbProfile->currentIndex());
    if (profile && !profile->extension.isEmpty()) {
        return profile->extension;
    }
    return QStringLiteral("xtc");
}

void XtExportDlg::resizeMainWindow()
{
    // Resize main window's document view to match profile resolution
    // This prevents slow preview rendering when resolutions differ
    if (auto* mainWin = qobject_cast<MainWindow*>(parent())) {
        mainWin->resizeDocViewToSize(m_ui->sbWidth->value(), m_ui->sbHeight->value());
    }
}

// Slot implementations

void XtExportDlg::onProfileChanged(int index)
{
    if (m_updatingControls)
        return;

    XtExportProfile* profile = m_profileManager->profileByIndex(index);
    if (profile) {
        loadProfileToUi(profile);
        resizeMainWindow();
    }
    schedulePreviewUpdate();
}

void XtExportDlg::onFormatChanged(int /*index*/)
{
    if (m_updatingControls)
        return;
    onResetDithering();
    updateDitheringControlsState();
    updateGrayPolicyState();
    schedulePreviewUpdate();
}

void XtExportDlg::onWidthChanged(int /*value*/)
{
    if (m_updatingControls)
        return;
    updatePreviewContainerSizes();
    resizeMainWindow();
    schedulePreviewUpdate();
}

void XtExportDlg::onHeightChanged(int /*value*/)
{
    if (m_updatingControls)
        return;
    updatePreviewContainerSizes();
    resizeMainWindow();
    schedulePreviewUpdate();
}

void XtExportDlg::onGrayPolicyChanged(int /*index*/)
{
    if (m_updatingControls)
        return;
    schedulePreviewUpdate();
}

void XtExportDlg::onDitherModeChanged(int /*index*/)
{
    if (m_updatingControls)
        return;
    updateDitheringControlsState();
    schedulePreviewUpdate();
}

void XtExportDlg::onThresholdSliderChanged(int value)
{
    if (m_updatingControls)
        return;
    m_updatingControls = true;
    m_ui->sbThreshold->setValue(value / 100.0);
    m_updatingControls = false;
    schedulePreviewUpdate();
}

void XtExportDlg::onThresholdSpinBoxChanged(double value)
{
    if (m_updatingControls)
        return;
    m_updatingControls = true;
    m_ui->sliderThreshold->setValue(static_cast<int>(value * 100));
    m_updatingControls = false;
    schedulePreviewUpdate();
}

void XtExportDlg::onErrorDiffusionSliderChanged(int value)
{
    if (m_updatingControls)
        return;
    m_updatingControls = true;
    m_ui->sbErrorDiffusion->setValue(value / 100.0);
    m_updatingControls = false;
    schedulePreviewUpdate();
}

void XtExportDlg::onErrorDiffusionSpinBoxChanged(double value)
{
    if (m_updatingControls)
        return;
    m_updatingControls = true;
    m_ui->sliderErrorDiffusion->setValue(static_cast<int>(value * 100));
    m_updatingControls = false;
    schedulePreviewUpdate();
}

void XtExportDlg::onGammaSliderChanged(int value)
{
    if (m_updatingControls)
        return;
    m_updatingControls = true;
    m_ui->sbGamma->setValue(value / 100.0);
    m_updatingControls = false;
    schedulePreviewUpdate();
}

void XtExportDlg::onGammaSpinBoxChanged(double value)
{
    if (m_updatingControls)
        return;
    m_updatingControls = true;
    m_ui->sliderGamma->setValue(static_cast<int>(value * 100));
    m_updatingControls = false;
    schedulePreviewUpdate();
}

void XtExportDlg::onSerpentineChanged(Qt::CheckState /*state*/)
{
    if (m_updatingControls)
        return;
    schedulePreviewUpdate();
}

void XtExportDlg::onResetDithering()
{
    // Get default profile based on current format (XTC=0, XTCH=1)
    int formatIndex = m_ui->cbFormat->currentIndex();
    XtExportProfile defaultProfile = (formatIndex == 0)
        ? XtExportProfile::defaultXtcProfile()
        : XtExportProfile::defaultXtchProfile();

    m_updatingControls = true;

    // Reset gray policy
    int grayPolicyIndex = 0;
    switch (defaultProfile.grayPolicy) {
        case GRAY_SPLIT_LIGHT_DARK: grayPolicyIndex = 0; break;
        case GRAY_ALL_TO_WHITE: grayPolicyIndex = 1; break;
        case GRAY_ALL_TO_BLACK: grayPolicyIndex = 2; break;
    }
    m_ui->cbGrayPolicy->setCurrentIndex(grayPolicyIndex);

    // Reset dither mode
    int ditherModeIndex = 0;
    switch (defaultProfile.ditherMode) {
        case IMAGE_DITHER_NONE: ditherModeIndex = 0; break;
        case IMAGE_DITHER_ORDERED: ditherModeIndex = 1; break;
        case IMAGE_DITHER_FS_1BIT:
        case IMAGE_DITHER_FS_2BIT: ditherModeIndex = 2; break;
    }
    m_ui->cbDitherMode->setCurrentIndex(ditherModeIndex);

    // Reset FS parameters
    m_ui->sbThreshold->setValue(defaultProfile.ditheringOpts.threshold);
    m_ui->sliderThreshold->setValue(static_cast<int>(defaultProfile.ditheringOpts.threshold * 100));
    m_ui->sbErrorDiffusion->setValue(defaultProfile.ditheringOpts.errorDiffusion);
    m_ui->sliderErrorDiffusion->setValue(static_cast<int>(defaultProfile.ditheringOpts.errorDiffusion * 100));
    m_ui->sbGamma->setValue(defaultProfile.ditheringOpts.gamma);
    m_ui->sliderGamma->setValue(static_cast<int>(defaultProfile.ditheringOpts.gamma * 100));
    m_ui->cbSerpentine->setChecked(defaultProfile.ditheringOpts.serpentine);

    m_updatingControls = false;

    // Update control states and trigger preview
    updateDitheringControlsState();
    schedulePreviewUpdate();
}

void XtExportDlg::onFromPageChanged(int value)
{
    if (m_updatingControls)
        return;

    // Swap if From > To
    if (value > m_ui->sbToPage->value()) {
        m_updatingControls = true;
        int to = m_ui->sbToPage->value();
        m_ui->sbToPage->setValue(value);
        m_ui->sbFromPage->setValue(to);
        m_updatingControls = false;
    }
}

void XtExportDlg::onToPageChanged(int value)
{
    if (m_updatingControls)
        return;

    // Swap if To < From
    if (value < m_ui->sbFromPage->value()) {
        m_updatingControls = true;
        int from = m_ui->sbFromPage->value();
        m_ui->sbFromPage->setValue(value);
        m_ui->sbToPage->setValue(from);
        m_updatingControls = false;
    }
}

void XtExportDlg::onChaptersEnabledChanged(Qt::CheckState /*state*/)
{
    if (m_updatingControls)
        return;
    // Chapter depth combo could be enabled/disabled based on this
}

void XtExportDlg::onChapterDepthChanged(int /*index*/)
{
    if (m_updatingControls)
        return;
}

void XtExportDlg::onFirstPage()
{
    m_ui->sbPreviewPage->setValue(1);
}

void XtExportDlg::onPrevPage()
{
    int page = m_ui->sbPreviewPage->value();
    if (page > 1) {
        m_ui->sbPreviewPage->setValue(page - 1);
    }
}

void XtExportDlg::onNextPage()
{
    int page = m_ui->sbPreviewPage->value();
    int maxPage = m_ui->sbPreviewPage->maximum();
    if (page < maxPage) {
        m_ui->sbPreviewPage->setValue(page + 1);
    }
}

void XtExportDlg::onLastPage()
{
    m_ui->sbPreviewPage->setValue(m_ui->sbPreviewPage->maximum());
}

void XtExportDlg::onPreviewPageChanged(int value)
{
    if (m_updatingControls)
        return;
    m_currentPreviewPage = value - 1;  // Convert to 0-based
    // Reset pan and render immediately (no debounce for page navigation)
    renderPreview();
}

void XtExportDlg::onZoomSliderChanged(int value)
{
    if (m_updatingControls)
        return;
    m_updatingControls = true;
    m_zoomPercent = value;
    m_previewWidget->setZoom(value);
    updateZoomButtonLabel();
    m_ui->sbZoom->setValue(value);
    m_updatingControls = false;
}

void XtExportDlg::onZoomSpinBoxChanged(int value)
{
    if (m_updatingControls)
        return;
    m_updatingControls = true;
    m_zoomPercent = value;
    m_previewWidget->setZoom(value);
    updateZoomButtonLabel();
    m_ui->sliderZoom->setValue(value);
    m_updatingControls = false;
}

void XtExportDlg::onResetZoom()
{
    m_ui->sliderZoom->setValue(100);
}

void XtExportDlg::onSet200Zoom()
{
    // Cycle through zoom levels: find next level higher than current, or wrap to first
    // The button label update is handled by onZoomSliderChanged() -> updateZoomButtonLabel()
    for (int i : PreviewWidget::ZOOM_LEVELS) {
        if (m_zoomPercent < i) {
            m_ui->sliderZoom->setValue(i);
            return;
        }
    }
    // At or above max level, wrap to first
    m_ui->sliderZoom->setValue(PreviewWidget::ZOOM_LEVELS[0]);
}

void XtExportDlg::onPreviewPageChangeRequested(int delta)
{
    int newPage = m_ui->sbPreviewPage->value() + delta;
    newPage = qBound(1, newPage, m_ui->sbPreviewPage->maximum());
    m_ui->sbPreviewPage->setValue(newPage);
}

void XtExportDlg::onPreviewZoomChangeRequested(int delta)
{
    int newZoom = m_ui->sliderZoom->value() + delta;
    newZoom = qBound(m_ui->sliderZoom->minimum(), newZoom, m_ui->sliderZoom->maximum());
    m_ui->sliderZoom->setValue(newZoom);
}

void XtExportDlg::onPreviewPreferredSizeRequested()
{
    // Calculate the size difference between current preview size and preferred size
    QSize preferredSize = m_previewWidget->sizeHint();
    QSize currentSize = m_previewWidget->size();
    QSize sizeDelta = preferredSize - currentSize;

    // Resize the dialog by the delta to accommodate the preferred preview size
    if (sizeDelta.width() != 0 || sizeDelta.height() != 0) {
        resize(size() + sizeDelta);
        clampToScreenBounds();
    }
}

void XtExportDlg::onBrowseOutput()
{
    QString currentPath = m_ui->leOutputPath->text();

    if (m_batchMode) {
        // Batch mode: directory selection
        QString startPath;

        if (!currentPath.isEmpty() && QDir(currentPath).exists()) {
            startPath = currentPath;
        } else if (!m_ui->leSourcePath->text().isEmpty()) {
            startPath = m_ui->leSourcePath->text();
        } else if (!m_lastDirectory.isEmpty()) {
            startPath = m_lastDirectory;
        } else {
            startPath = QDir::homePath();
        }

        QString directory = QFileDialog::getExistingDirectory(
            this,
            tr("选择输出目录"),
            startPath,
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        if (!directory.isEmpty()) {
            m_ui->leOutputPath->setText(QDir::toNativeSeparators(directory));
        }
    } else {
        // Single file mode: file selection
        QString startPath;

        // Determine starting path
        if (!currentPath.isEmpty()) {
            QFileInfo fi(currentPath);
            if (fi.isDir()) {
                // If current path is a directory, start there with computed filename
                startPath = currentPath;
                if (!startPath.endsWith('/') && !startPath.endsWith('\\')) {
                    startPath += '/';
                }
                startPath += computeExportFilename();
            } else {
                // Use existing full path
                startPath = currentPath;
            }
        } else {
            // No current path, use default
            startPath = computeDefaultOutputPath();
        }

        QString filter = tr("XT* 文件 (*.%1);;所有文件 (*)").arg(currentProfileExtension());
        QString filename = QFileDialog::getSaveFileName(this, tr("导出到..."), startPath, filter);

        if (!filename.isEmpty()) {
            m_ui->leOutputPath->setText(QDir::toNativeSeparators(filename));
        }
    }
}

void XtExportDlg::onBrowseSource()
{
    QString currentPath = m_ui->leSourcePath->text();
    QString startPath;

    if (!currentPath.isEmpty() && QDir(currentPath).exists()) {
        startPath = currentPath;
    } else if (!m_lastDirectory.isEmpty()) {
        startPath = m_lastDirectory;
    } else {
        startPath = QDir::homePath();
    }

    QString directory = QFileDialog::getExistingDirectory(
        this,
        tr("选择源目录"),
        startPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!directory.isEmpty()) {
        m_ui->leSourcePath->setText(QDir::toNativeSeparators(directory));
    }
}

void XtExportDlg::onExport()
{
    if (m_exporting)
        return;

    if (m_batchMode) {
        // Batch mode: validate and perform batch export
        performBatchExport();
    } else {
        // Single file mode: existing behavior
        if (!validateExportSettings())
            return;
        performExport();
    }
}

void XtExportDlg::onCancel()
{
    if (m_exporting) {
        // Set cancellation flag - the callback will pick this up
        if (m_exportCallback) {
            m_exportCallback->setCancelled(true);
        }
    } else {
        reject();
    }
}

void XtExportDlg::schedulePreviewUpdate()
{
    // Restart the debounce timer (restarts if already running)
    m_previewDebounceTimer->start();
}

void XtExportDlg::renderPreview()
{
    if (!m_docView) {
        m_previewWidget->showMessage(tr("未加载文档"));
        return;
    }

    m_previewWidget->showMessage(tr("渲染中..."));

    // Create and configure exporter
    XtcExporter exporter;
    configureExporter(exporter);

    // Set preview mode for current page
    exporter.setPreviewPage(m_currentPreviewPage);

    // Execute preview render (no file output in preview mode)
    bool success = exporter.exportDocument(m_docView, static_cast<LVStreamRef>(nullptr));

    if (success) {
        // Get rendered BMP data
        const LVArray<lUInt8>& bmpData = exporter.getPreviewBmp();

        if (bmpData.length() > 0) {
            // Load BMP from memory into QImage
            QImage image;
            if (image.loadFromData(bmpData.ptr(), bmpData.length(), "BMP")) {
                m_previewWidget->setImage(image, m_zoomPercent);
            } else {
                m_previewWidget->showMessage(tr("预览解码失败"));
            }
        } else {
            m_previewWidget->showMessage(tr("预览渲染失败"));
        }

        // Update page count from exporter
        int totalPages = exporter.getLastTotalPageCount();
        if (totalPages > 0) {
            updatePageCount(totalPages);
        }
    } else {
        m_previewWidget->showMessage(tr("预览渲染失败"));
    }
}

void XtExportDlg::configureExporter(XtcExporter& exporter)
{
    // Format
    XtcExportFormat format = (m_ui->cbFormat->currentIndex() == 0)
                             ? XTC_FORMAT_XTC : XTC_FORMAT_XTCH;
    exporter.setFormat(format);

    // Dimensions
    exporter.setDimensions(static_cast<uint16_t>(m_ui->sbWidth->value()),
                           static_cast<uint16_t>(m_ui->sbHeight->value()));

    // Gray policy (only affects XTC/1-bit)
    GrayToMonoPolicy grayPolicy;
    switch (m_ui->cbGrayPolicy->currentIndex()) {
        case 0: grayPolicy = GRAY_SPLIT_LIGHT_DARK; break;
        case 1: grayPolicy = GRAY_ALL_TO_WHITE; break;
        case 2: grayPolicy = GRAY_ALL_TO_BLACK; break;
        default: grayPolicy = GRAY_SPLIT_LIGHT_DARK; break;
    }
    exporter.setGrayPolicy(grayPolicy);

    // Dither mode
    ImageDitherMode ditherMode = resolveEffectiveDitherMode();
    exporter.setImageDitherMode(ditherMode);

    // Dithering options (for Floyd-Steinberg modes)
    if (ditherMode == IMAGE_DITHER_FS_1BIT || ditherMode == IMAGE_DITHER_FS_2BIT) {
        DitheringOptions opts;
        opts.threshold = static_cast<float>(m_ui->sbThreshold->value());
        opts.errorDiffusion = static_cast<float>(m_ui->sbErrorDiffusion->value());
        opts.gamma = static_cast<float>(m_ui->sbGamma->value());
        opts.serpentine = m_ui->cbSerpentine->isChecked();
        exporter.setDitheringOptions(opts);
    }

    // Chapters (not used in preview, but configure anyway)
    exporter.enableChapters(m_ui->cbChaptersEnabled->isChecked());
    exporter.setChapterDepth(m_ui->cbChapterDepth->currentIndex() + 1);

    // Metadata from UI fields
    lString8 title = UnicodeToUtf8(qt2cr(m_ui->leTitle->text()));
    lString8 author = UnicodeToUtf8(qt2cr(m_ui->leAuthor->text()));
    exporter.setMetadata(title, author);
}

void XtExportDlg::updatePageCount(int totalPages)
{
    if (totalPages == m_totalPageCount)
        return;

    m_totalPageCount = totalPages;
    m_updatingControls = true;

    // Update page count label
    m_ui->lblPageCount->setText(QString("/ %1").arg(totalPages));

    // Update spinbox maximums
    m_ui->sbPreviewPage->setMaximum(totalPages);
    m_ui->sbFromPage->setMaximum(totalPages);
    m_ui->sbToPage->setMaximum(totalPages);
    m_ui->sbToPage->setValue(totalPages);


    // Clamp current preview page if needed
    if (m_currentPreviewPage >= totalPages) {
        m_currentPreviewPage = totalPages - 1;
        m_ui->sbPreviewPage->setValue(m_currentPreviewPage + 1);
    }

    // Clamp From/To page values if needed
    if (m_ui->sbFromPage->value() > totalPages) {
        m_ui->sbFromPage->setValue(totalPages);
    }
    if (m_ui->sbToPage->value() > totalPages) {
        m_ui->sbToPage->setValue(totalPages);
    }

    // Make sure To >= From
    if (m_ui->sbToPage->value() < m_ui->sbFromPage->value()) {
        int from = m_ui->sbFromPage->value();
        int to = m_ui->sbToPage->value();
        m_ui->sbFromPage->setValue(to);
        m_ui->sbToPage->setValue(from);
    }

    m_updatingControls = false;
}

void XtExportDlg::loadZoomSetting()
{
    int zoom = 100;  // default
    if (auto* mainWin = qobject_cast<MainWindow*>(parent())) {
        CRPropRef props = mainWin->getSettings();
        zoom = props->getIntDef("xtexport.preview.zoom", 100);
    }
    zoom = qBound(50, zoom, PreviewWidget::ZOOM_LEVEL_MAX);

    m_zoomPercent = zoom;
    m_ui->sliderZoom->setValue(zoom);
    updateZoomButtonLabel();
}

void XtExportDlg::updateZoomButtonLabel()
{
    // Button shows the zoom level it will jump TO when clicked
    // Find next level higher than current, or wrap to first
    for (int i : PreviewWidget::ZOOM_LEVELS) {
        if (m_zoomPercent < i) {
            m_ui->btn200Zoom->setText(QString("%1%").arg(i));
            return;
        }
    }
    // At or above max level, show first level (wraps)
    m_ui->btn200Zoom->setText(QString("%1%").arg(PreviewWidget::ZOOM_LEVELS[0]));
}

void XtExportDlg::saveZoomSetting()
{
    if (auto* mainWin = qobject_cast<MainWindow*>(parent())) {
        CRPropRef props = mainWin->getSettings();
        props->setInt("xtexport.preview.zoom", m_ui->sliderZoom->value());
    }
}

void XtExportDlg::loadLastProfileSetting()
{
    QString lastProfile;
    if (auto* mainWin = qobject_cast<MainWindow*>(parent())) {
        CRPropRef props = mainWin->getSettings();
        lastProfile = cr2qt(props->getStringDef("xtexport.last.profile", ""));
    }

    // Find the saved profile index
    int profileIndex = 0;  // default to first
    if (!lastProfile.isEmpty()) {
        int idx = m_profileManager->indexOfProfile(lastProfile);
        if (idx >= 0) {
            profileIndex = idx;
        }
    }

    // Select the profile in dropdown and load it
    if (m_profileManager->profileCount() > 0) {
        m_ui->cbProfile->setCurrentIndex(profileIndex);
        loadProfileToUi(m_profileManager->profileByIndex(profileIndex));
    }
}

void XtExportDlg::saveLastProfileSetting()
{
    if (auto* mainWin = qobject_cast<MainWindow*>(parent())) {
        CRPropRef props = mainWin->getSettings();
        XtExportProfile* profile = m_profileManager->profileByIndex(m_ui->cbProfile->currentIndex());
        if (profile) {
            props->setString("xtexport.last.profile", qt2cr(profile->filename).c_str());
        }
    }
}

void XtExportDlg::loadLastDirectorySetting()
{
    if (auto* mainWin = qobject_cast<MainWindow*>(parent())) {
        CRPropRef props = mainWin->getSettings();
        m_lastDirectory = cr2qt(props->getStringDef("xtexport.last.directory", ""));

        // Sanitize: discard if it contains @ (invalid archive path artifact)
        if (m_lastDirectory.contains('@')) {
            m_lastDirectory.clear();
        }
    }
}

void XtExportDlg::saveLastDirectorySetting()
{
    if (auto* mainWin = qobject_cast<MainWindow*>(parent())) {
        CRPropRef props = mainWin->getSettings();
        // Extract directory from current output path
        QString outputPath = m_ui->leOutputPath->text();
        if (!outputPath.isEmpty()) {
            QFileInfo fi(outputPath);
            QString dir;

            // If it's already a directory, use it directly; otherwise extract parent
            if (fi.isDir()) {
                dir = outputPath;
            } else {
                dir = fi.absolutePath();
            }

            // Don't save if it contains @ (invalid archive path artifact)
            if (!dir.contains('@')) {
                props->setString("xtexport.last.directory", qt2cr(dir).c_str());
            }
        }
    }
}

void XtExportDlg::loadWindowGeometry()
{
    if (auto* mainWin = qobject_cast<MainWindow*>(parent())) {
        CRPropRef props = mainWin->getSettings();
        restoreWindowPosition(this, props, "xtexport.");
    }
}

void XtExportDlg::saveWindowGeometry()
{
    if (auto* mainWin = qobject_cast<MainWindow*>(parent())) {
        CRPropRef props = mainWin->getSettings();
        saveWindowPosition(this, props, "xtexport.");
    }
}

void XtExportDlg::clampToScreenBounds()
{
    QScreen* screen = this->screen();
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    QRect availableGeom = screen->availableGeometry();
    QSize currentSize = size();

    // Check if dialog exceeds screen bounds
    if (currentSize.width() > availableGeom.width() ||
        currentSize.height() > availableGeom.height()) {
        // Clamp to available screen size
        int newWidth = qMin(currentSize.width(), availableGeom.width());
        int newHeight = qMin(currentSize.height(), availableGeom.height());
        resize(newWidth, newHeight);
    }
}

void XtExportDlg::saveCurrentProfileSettings()
{
    XtExportProfile* profile = m_profileManager->profileByIndex(m_ui->cbProfile->currentIndex());
    if (profile) {
        updateProfileFromUi(profile);
        m_profileManager->saveProfile(profile);
    }
}

void XtExportDlg::updateProfileFromUi(XtExportProfile* profile)
{
    if (!profile)
        return;

    // Format settings
    profile->format = (m_ui->cbFormat->currentIndex() == 0) ? XTC_FORMAT_XTC : XTC_FORMAT_XTCH;
    profile->width = m_ui->sbWidth->value();
    profile->height = m_ui->sbHeight->value();
    profile->bpp = (profile->format == XTC_FORMAT_XTC) ? 1 : 2;

    // Gray policy
    switch (m_ui->cbGrayPolicy->currentIndex()) {
        case 0: profile->grayPolicy = GRAY_SPLIT_LIGHT_DARK; break;
        case 1: profile->grayPolicy = GRAY_ALL_TO_WHITE; break;
        case 2: profile->grayPolicy = GRAY_ALL_TO_BLACK; break;
        default: profile->grayPolicy = GRAY_SPLIT_LIGHT_DARK; break;
    }

    // Dither mode - store format-appropriate FS mode
    switch (m_ui->cbDitherMode->currentIndex()) {
        case 0: profile->ditherMode = IMAGE_DITHER_NONE; break;
        case 1: profile->ditherMode = IMAGE_DITHER_ORDERED; break;
        case 2:
            // Floyd-Steinberg: store format-appropriate mode
            profile->ditherMode = (profile->format == XTC_FORMAT_XTC)
                ? IMAGE_DITHER_FS_1BIT : IMAGE_DITHER_FS_2BIT;
            break;
        default: profile->ditherMode = IMAGE_DITHER_ORDERED; break;
    }

    // Dithering options
    profile->ditheringOpts.threshold = static_cast<float>(m_ui->sbThreshold->value());
    profile->ditheringOpts.errorDiffusion = static_cast<float>(m_ui->sbErrorDiffusion->value());
    profile->ditheringOpts.gamma = static_cast<float>(m_ui->sbGamma->value());
    profile->ditheringOpts.serpentine = m_ui->cbSerpentine->isChecked();

    // Chapter settings
    profile->chaptersEnabled = m_ui->cbChaptersEnabled->isChecked();
    profile->chapterDepth = m_ui->cbChapterDepth->currentIndex() + 1;
}

void XtExportDlg::loadDocumentMetadata()
{
    if (!m_docView)
        return;

    // Get title and author from document properties
    QString title = cr2qt(m_docView->getTitle());
    QString author = cr2qt(m_docView->getAuthors());

    // Populate the UI fields
    m_ui->leTitle->setText(title);
    m_ui->leAuthor->setText(author);
}

void XtExportDlg::updateExportProgress(int percent)
{
    m_ui->progressBar->setValue(percent);
    m_ui->lblCurrentPercent->setText(QString("%1%").arg(percent));
}

void XtExportDlg::setExporting(bool exporting)
{
    m_exporting = exporting;

    // Disable/enable settings groups
    m_ui->settingsGroup->setEnabled(!exporting);

    // Preview group stays enabled but navigation is disabled
    m_ui->btnFirstPage->setEnabled(!exporting);
    m_ui->btnPrevPage->setEnabled(!exporting);
    m_ui->btnNextPage->setEnabled(!exporting);
    m_ui->btnLastPage->setEnabled(!exporting);
    m_ui->sbPreviewPage->setEnabled(!exporting);
    m_previewWidget->setPageNavigationEnabled(!exporting);

    // Mode toggle disabled during export
    m_ui->rbSingleFile->setEnabled(!exporting);
    m_ui->rbBatchExport->setEnabled(!exporting);

    // Batch options and page range disabled during export
    m_ui->batchOptionsGroup->setEnabled(!exporting);
    m_ui->pageRangeGroup->setEnabled(!exporting);

    // Progress bar visibility - replaces path controls during export
    // Single file mode: output row -> single progress bar (Current)
    // Batch mode: source row -> Files progress, output row -> Current progress
    if (exporting) {
        // Hide path controls
        m_ui->leOutputPath->setVisible(false);
        m_ui->btnBrowse->setVisible(false);

        if (m_batchMode) {
            // Hide source row (already hidden in single mode)
            m_ui->leSourcePath->setVisible(false);
            m_ui->btnBrowseSource->setVisible(false);

            // Update group title for batch progress display
            m_ui->pathsGroup->setTitle(tr("文件 / 当前"));

            // Show both progress rows for batch mode
            m_ui->progressBarFiles->setVisible(true);
            m_ui->progressBarFiles->setValue(0);
            m_ui->lblFilesCount->setVisible(true);
            m_ui->lblFilesCount->setText("0/0");

            m_ui->progressBar->setVisible(true);
            m_ui->progressBar->setValue(0);
            m_ui->lblCurrentPercent->setVisible(true);
            m_ui->lblCurrentPercent->setText("0%");
        } else {
            // Single file mode: show only current progress bar (replacing output row)
            m_ui->progressBar->setVisible(true);
            m_ui->progressBar->setValue(0);
            m_ui->lblCurrentPercent->setVisible(true);
            m_ui->lblCurrentPercent->setText("0%");
        }
    } else {
        // Hide all progress bars
        m_ui->progressBarFiles->setVisible(false);
        m_ui->lblFilesCount->setVisible(false);
        m_ui->progressBar->setVisible(false);
        m_ui->lblCurrentPercent->setVisible(false);

        // Restore path controls based on mode
        m_ui->leOutputPath->setVisible(true);
        m_ui->btnBrowse->setVisible(true);

        // Restore group title based on mode
        m_ui->pathsGroup->setTitle(m_batchMode ? tr("源目录 / 输出") : tr("输出"));

        if (m_batchMode) {
            // Show source row in batch mode
            m_ui->leSourcePath->setVisible(true);
            m_ui->btnBrowseSource->setVisible(true);
        }
    }

    // Button states
    m_ui->btnExport->setEnabled(!exporting);
    m_ui->btnClose->setText(exporting ? tr("取消") : tr("关闭"));

    // Disconnect/reconnect close button based on state
    // During export, clicking Close should cancel; otherwise reject dialog
    disconnect(m_ui->btnClose, &QPushButton::clicked, nullptr, nullptr);
    if (exporting) {
        connect(m_ui->btnClose, &QPushButton::clicked, this, &XtExportDlg::onCancel);
    } else {
        connect(m_ui->btnClose, &QPushButton::clicked, this, &QDialog::reject);
    }
}

void XtExportDlg::setScanning(bool scanning)
{
    m_scanning = scanning;

    // Only manage path/progress UI visibility - other controls handled by setExporting()
    if (scanning) {
        // Hide all path controls in the group
        m_ui->leSourcePath->setVisible(false);
        m_ui->btnBrowseSource->setVisible(false);
        m_ui->leOutputPath->setVisible(false);
        m_ui->btnBrowse->setVisible(false);

        // Show files progress bar with indeterminate state (0 max = busy indicator)
        m_ui->progressBarFiles->setVisible(true);
        m_ui->progressBarFiles->setMinimum(0);
        m_ui->progressBarFiles->setMaximum(0);  // Indeterminate/busy mode
        m_ui->lblFilesCount->setVisible(true);
        m_ui->lblFilesCount->setMinimumWidth(60);  // Wider to fit large scan counts
        m_ui->lblFilesCount->setText("0");

        // Update group title to indicate scanning
        m_ui->pathsGroup->setTitle(tr("扫描中..."));

        // Change Close button to Cancel for scan cancellation
        m_ui->btnExport->setEnabled(false);
        m_ui->btnClose->setText(tr("取消"));
        disconnect(m_ui->btnClose, &QPushButton::clicked, nullptr, nullptr);
        connect(m_ui->btnClose, &QPushButton::clicked, this, [this]() {
            m_scanCancelled = true;
        });
    } else {
        // Hide progress bar
        m_ui->progressBarFiles->setVisible(false);
        m_ui->progressBarFiles->setMaximum(100);  // Reset to determinate mode
        m_ui->lblFilesCount->setVisible(false);
        m_ui->lblFilesCount->setMinimumWidth(36);  // Restore original width for export alignment

        // Restore path controls for batch mode
        m_ui->leSourcePath->setVisible(true);
        m_ui->btnBrowseSource->setVisible(true);
        m_ui->leOutputPath->setVisible(true);
        m_ui->btnBrowse->setVisible(true);

        // Restore group title
        m_ui->pathsGroup->setTitle(tr("源目录 / 输出"));

        // Restore button states
        m_ui->btnExport->setEnabled(true);
        m_ui->btnClose->setText(tr("关闭"));
        disconnect(m_ui->btnClose, &QPushButton::clicked, nullptr, nullptr);
        connect(m_ui->btnClose, &QPushButton::clicked, this, &QDialog::reject);
    }
}

bool XtExportDlg::validateExportSettings()
{
    // Check document
    if (!m_docView) {
        QMessageBox::warning(this, tr("导出错误"),
                             tr("未加载文档。"));
        return false;
    }

    // Check output path
    QString outputPath = resolveExportPath();
    if (outputPath.isEmpty()) {
        QMessageBox::warning(this, tr("导出错误"),
                             tr("请指定输出文件路径。"));
        m_ui->leOutputPath->setFocus();
        return false;
    }

    // Check if output directory exists
    QFileInfo fi(outputPath);
    QDir outputDir = fi.absoluteDir();
    if (!outputDir.exists()) {
        QMessageBox::warning(this, tr("导出错误"),
                             tr("输出目录不存在：\n%1").arg(outputDir.absolutePath()));
        m_ui->leOutputPath->setFocus();
        return false;
    }

    // Check page range
    int fromPage = m_ui->sbFromPage->value();
    int toPage = m_ui->sbToPage->value();
    if (fromPage > toPage) {
        QMessageBox::warning(this, tr("导出错误"),
                             tr("页面范围无效：起始页不能大于结束页。"));
        m_ui->sbFromPage->setFocus();
        return false;
    }

    // Check if file already exists
    if (fi.exists()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("确认覆盖"),
            tr("文件已存在：\n%1\n\n是否覆盖？").arg(outputPath),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return false;
        }
    }

    return true;
}

void XtExportDlg::performExport()
{
    // Get the resolved output path
    m_exportFilePath = resolveExportPath();

    // Update the output path field with the resolved path
    m_ui->leOutputPath->setText(m_exportFilePath);

    // Create the export callback
    m_exportCallback = new QtExportCallback(this);

    // Enter exporting state
    setExporting(true);

    // Create and configure exporter
    XtcExporter exporter;
    configureExporter(exporter);

    // Disable preview mode (ensure we're in full export mode)
    exporter.setPreviewPage(-1);

    // Set page range (convert from 1-based UI to 0-based exporter)
    int fromPage = m_ui->sbFromPage->value() - 1;
    int toPage = m_ui->sbToPage->value() - 1;
    exporter.setPageRange(fromPage, toPage);

    // Set the progress callback
    exporter.setProgressCallback(m_exportCallback);

    // Perform the export
    lString32 outputPath = qt2cr(m_exportFilePath);
    bool success = exporter.exportDocument(m_docView, outputPath.c_str());

    // Check if cancelled
    bool wasCancelled = m_exportCallback->wasCancelled();

    // Clean up callback
    delete m_exportCallback;
    m_exportCallback = nullptr;

    // Exit exporting state
    setExporting(false);

    // Handle result
    if (wasCancelled) {
        // Delete partial file on cancellation
        QFile partialFile(m_exportFilePath);
        if (partialFile.exists()) {
            partialFile.remove();
        }
        // No message needed - user knows they cancelled
    } else if (success) {
        // Save the directory for next time
        saveLastDirectorySetting();

        // Show success message
        QMessageBox::information(this, tr("导出完成"),
                                 tr("文档已成功导出到：\n%1").arg(m_exportFilePath));
    } else {
        // Show error message
        QMessageBox::critical(this, tr("导出失败"),
                              tr("导出到以下路径失败：\n%1\n\n请检查路径后重试。").arg(m_exportFilePath));
    }

    // Clear the export file path
    m_exportFilePath.clear();
}

void XtExportDlg::setMode(bool batchMode)
{
    if (m_batchMode == batchMode)
        return;

    m_batchMode = batchMode;
    onModeChanged();
}

void XtExportDlg::onModeChanged()
{
    // Update Export button text
    m_ui->btnExport->setText(m_batchMode ? tr("全部导出") : tr("导出"));

    // Single file mode: show page range, metadata, preview
    // Batch mode: show batch options, source directory, hide preview navigation details

    // Page Range - single file only
    m_ui->pageRangeGroup->setVisible(!m_batchMode);

    // Metadata - single file only (in left column, hidden in batch mode to avoid confusion)
    m_ui->metadataGroup->setVisible(!m_batchMode);

    // Batch Options - batch mode only
    m_ui->batchOptionsGroup->setVisible(m_batchMode);

    // Source Directory - batch mode only (individual widgets)
    m_ui->leSourcePath->setVisible(m_batchMode);
    m_ui->btnBrowseSource->setVisible(m_batchMode);

    // Update pathsGroup title based on mode
    m_ui->pathsGroup->setTitle(m_batchMode ? tr("源目录 / 输出") : tr("输出"));

    // Output path placeholder
    m_ui->leOutputPath->setPlaceholderText(
        m_batchMode ? tr("输出目录路径...") : tr("输出文件路径..."));

    // Convert output path between file and directory based on mode
    QString currentOutput = m_ui->leOutputPath->text();
    if (!currentOutput.isEmpty()) {
        QFileInfo fi(currentOutput);
        if (m_batchMode) {
            // Switching to batch mode: convert file path to directory
            if (!fi.isDir()) {
                // Extract directory from file path
                QString dir = fi.absolutePath();
                m_ui->leOutputPath->setText(QDir::toNativeSeparators(dir));
            }
        } else {
            // Switching to single file mode: convert directory to file path
            if (fi.isDir()) {
                // Append computed filename to directory
                QString path = currentOutput;
                if (!path.endsWith('/') && !path.endsWith('\\')) {
                    path += '/';
                }
                path += computeExportFilename();
                m_ui->leOutputPath->setText(QDir::toNativeSeparators(path));
            }
        }
    }
}

void XtExportDlg::loadBatchSettings()
{
    auto* mainWin = qobject_cast<MainWindow*>(parent());
    if (!mainWin)
        return;

    CRPropRef props = mainWin->getSettings();

    // Load batch mode flag
    m_batchMode = props->getBoolDef("xtexport.batch.mode", false);

    // Load source and output directories
    QString sourceDir = cr2qt(props->getStringDef("xtexport.batch.source.directory", ""));
    QString outputDir = cr2qt(props->getStringDef("xtexport.batch.output.directory", ""));

    // Load file mask (use default if empty)
    QString fileMask = cr2qt(props->getStringDef("xtexport.batch.file.mask", ""));
    if (fileMask.isEmpty()) {
        fileMask = DEFAULT_FILE_MASK;
    }

    // Load other batch options
    bool preserveStructure = props->getBoolDef("xtexport.batch.preserve.structure", true);
    int overwriteMode = props->getIntDef("xtexport.batch.overwrite.mode", 0);

    // Apply to UI controls
    m_ui->leFileMask->setText(fileMask);
    m_ui->cbPreserveStructure->setChecked(preserveStructure);
    m_ui->cbOverwriteMode->setCurrentIndex(overwriteMode);

    // Apply source directory
    if (!sourceDir.isEmpty()) {
        m_ui->leSourcePath->setText(sourceDir);
    }

    // Apply output directory for batch mode (don't override single file output)
    // The output path will be computed based on mode in computeDefaultBatchPaths()
    if (m_batchMode && !outputDir.isEmpty()) {
        m_ui->leOutputPath->setText(outputDir);
    }

    // Apply mode to radio buttons
    m_ui->rbBatchExport->setChecked(m_batchMode);
    m_ui->rbSingleFile->setChecked(!m_batchMode);

    // Trigger UI update for mode
    onModeChanged();
}

void XtExportDlg::saveBatchSettings()
{
    auto* mainWin = qobject_cast<MainWindow*>(parent());
    if (!mainWin)
        return;

    CRPropRef props = mainWin->getSettings();

    // Save batch mode flag
    props->setBool("xtexport.batch.mode", m_batchMode);

    // Save source directory
    props->setString("xtexport.batch.source.directory",
                     qt2cr(m_ui->leSourcePath->text()).c_str());

    // Save output directory (only in batch mode)
    if (m_batchMode) {
        props->setString("xtexport.batch.output.directory",
                         qt2cr(m_ui->leOutputPath->text()).c_str());
    }

    // Save file mask
    props->setString("xtexport.batch.file.mask",
                     qt2cr(m_ui->leFileMask->text()).c_str());

    // Save other batch options
    props->setBool("xtexport.batch.preserve.structure",
                   m_ui->cbPreserveStructure->isChecked());
    props->setInt("xtexport.batch.overwrite.mode",
                  m_ui->cbOverwriteMode->currentIndex());
}

// =============================================================================
// Invert mode - color inversion
// =============================================================================

void XtExportDlg::onInvertChanged(Qt::CheckState state)
{
    if (m_updatingControls)
        return;

    bool enabled = (state == Qt::Checked);
    if (enabled == m_inverseEnabled)
        return;

    m_inverseEnabled = enabled;

    if (enabled) {
        applyInvertColors();
    } else {
        restoreOriginalColors();
    }

    // Update preview to reflect color change
    schedulePreviewUpdate();
}

void XtExportDlg::loadInvertSetting()
{
    auto* mainWin = qobject_cast<MainWindow*>(parent());
    if (!mainWin)
        return;

    CRPropRef props = mainWin->getSettings();
    m_inverseEnabled = props->getBoolDef("xtexport.invert", false);

    // Apply to UI
    m_updatingControls = true;
    m_ui->cbInvert->setChecked(m_inverseEnabled);
    m_updatingControls = false;

    // Apply colors if enabled
    if (m_inverseEnabled) {
        applyInvertColors();
    }
}

void XtExportDlg::saveInvertSetting()
{
    auto* mainWin = qobject_cast<MainWindow*>(parent());
    if (!mainWin)
        return;

    CRPropRef props = mainWin->getSettings();
    props->setBool("xtexport.invert", m_inverseEnabled);
}

void XtExportDlg::applyInvertColors()
{
    if (!m_docView || m_colorsInverted)
        return;

    // Store original colors
    m_originalTextColor = m_docView->getTextColor();
    m_originalBackgroundColor = m_docView->getBackgroundColor();

    // Swap colors
    m_docView->setTextColor(m_originalBackgroundColor);
    m_docView->setBackgroundColor(m_originalTextColor);

    m_colorsInverted = true;
}

void XtExportDlg::restoreOriginalColors()
{
    if (!m_docView || !m_colorsInverted)
        return;

    // Restore original colors
    m_docView->setTextColor(m_originalTextColor);
    m_docView->setBackgroundColor(m_originalBackgroundColor);

    m_colorsInverted = false;
}

void XtExportDlg::computeDefaultBatchPaths()
{
    // If source directory is empty, use current document's directory
    if (m_ui->leSourcePath->text().isEmpty() && m_docView) {
        QString docPath = cr2qt(m_docView->getFileName());
        if (!docPath.isEmpty()) {
            // Handle archive paths like "archive.fb2.zip@/internal.fb2"
            int atPos = docPath.indexOf('@');
            if (atPos > 0) {
                docPath = docPath.left(atPos);
            }
            QFileInfo fi(docPath);
            m_ui->leSourcePath->setText(QDir::toNativeSeparators(fi.absolutePath()));
        }
    }

    // If output directory is empty in batch mode, use source directory
    if (m_batchMode && m_ui->leOutputPath->text().isEmpty()) {
        QString sourceDir = m_ui->leSourcePath->text();
        if (!sourceDir.isEmpty()) {
            m_ui->leOutputPath->setText(sourceDir);
        }
    }
}

// =============================================================================
// Directory scanning for batch mode
// =============================================================================

QStringList XtExportDlg::parseFileMask() const
{
    QString mask = m_ui->leFileMask->text().trimmed();
    if (mask.isEmpty()) {
        mask = DEFAULT_FILE_MASK;
    }

    // Split by semicolon, trim whitespace from each pattern
    QStringList patterns = mask.split(';', Qt::SkipEmptyParts);
    for (QString& pattern : patterns) {
        pattern = pattern.trimmed();
    }

    return patterns;
}

bool XtExportDlg::matchesFileMask(const QString& fileName, const QStringList& patterns) const
{
    for (const QString& pattern : patterns) {
        QRegularExpression regex(
            QRegularExpression::wildcardToRegularExpression(pattern),
            QRegularExpression::CaseInsensitiveOption);
        if (regex.match(fileName).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool XtExportDlg::findMatchingFiles(const QString& directory)
{
    m_batchFiles.clear();
    QStringList patterns = parseFileMask();

    // Use breadth-first traversal with frequent UI updates
    QStringList pendingDirs;
    pendingDirs.append(directory);

    int entriesProcessed = 0;
    const int UPDATE_INTERVAL = 100;  // Update UI every N entries

    while (!pendingDirs.isEmpty()) {
        QString currentDir = pendingDirs.takeFirst();

        // Use QDirIterator for lazy enumeration (doesn't load all entries at once)
        QDirIterator it(currentDir, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        while (it.hasNext()) {
            it.next();
            QFileInfo fi = it.fileInfo();

            if (fi.isDir()) {
                // Queue subdirectory for later processing
                pendingDirs.append(fi.absoluteFilePath());
            } else if (fi.isFile() && matchesFileMask(fi.fileName(), patterns)) {
                m_batchFiles.append(fi.absoluteFilePath());
            }

            entriesProcessed++;

            // Update UI frequently to stay responsive
            if (entriesProcessed % UPDATE_INTERVAL == 0) {
                // Show entries scanned (always increases, gives feedback even before finding matches)
                m_ui->lblFilesCount->setText(QString::number(entriesProcessed));
                QCoreApplication::processEvents();

                if (m_scanCancelled) {
                    m_batchFiles.clear();
                    return false;
                }
            }
        }
    }

    // Final update: show number of matching files found
    m_ui->lblFilesCount->setText(QString::number(m_batchFiles.size()));

    // Sort for consistent ordering
    m_batchFiles.sort(Qt::CaseInsensitive);
    return true;
}

bool XtExportDlg::scanSourceDirectory()
{
    QString sourceDir = m_ui->leSourcePath->text();

    if (sourceDir.isEmpty()) {
        QMessageBox::warning(this, tr("批量导出"),
            tr("请选择源目录。"));
        return false;
    }

    QDir dir(sourceDir);
    if (!dir.exists()) {
        QMessageBox::warning(this, tr("批量导出"),
            tr("源目录不存在：\n%1").arg(sourceDir));
        return false;
    }

    // Enter scanning state
    m_scanCancelled = false;
    setScanning(true);

    // Perform the scan (populates m_batchFiles)
    bool completed = findMatchingFiles(sourceDir);

    // Exit scanning state
    setScanning(false);

    if (!completed) {
        // Scan was cancelled
        return false;
    }

    if (m_batchFiles.isEmpty()) {
        QMessageBox::warning(this, tr("批量导出"),
            tr("在以下目录未找到匹配的文档：\n%1\n\n文件匹配：%2")
                .arg(sourceDir)
                .arg(m_ui->leFileMask->text()));
        return false;
    }

    return true;
}

QString XtExportDlg::computeBatchOutputPath(const QString& sourceFile)
{
    QString sourceDir = m_ui->leSourcePath->text();
    QString outputDir = m_ui->leOutputPath->text();

    QFileInfo fi(sourceFile);
    QString baseName = extractBaseName(sourceFile);  // Handles archives like .fb2.zip
    QString extension = currentProfileExtension();

    QString outputPath;

    if (m_ui->cbPreserveStructure->isChecked()) {
        // Preserve relative path structure
        QString relativePath = QDir(sourceDir).relativeFilePath(fi.absolutePath());
        QDir outDir(outputDir);
        outputPath = outDir.filePath(relativePath);
        QDir().mkpath(outputPath);  // Create subdirs as needed
        outputPath = QDir(outputPath).filePath(baseName + "." + extension);
    } else {
        // Flat mode - check for collisions
        outputPath = QDir(outputDir).filePath(baseName + "." + extension);
        outputPath = resolveFilenameCollision(outputPath);
    }

    return outputPath;
}

QString XtExportDlg::resolveFilenameCollision(const QString& basePath)
{
    if (!QFile::exists(basePath) && !m_usedOutputPaths.contains(basePath)) {
        m_usedOutputPaths.insert(basePath);
        return basePath;
    }

    QFileInfo fi(basePath);
    QString dir = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString ext = fi.suffix();

    int counter = 1;
    QString newPath;
    do {
        newPath = QDir(dir).filePath(QString("%1_%2.%3").arg(name).arg(counter).arg(ext));
        counter++;
    } while (QFile::exists(newPath) || m_usedOutputPaths.contains(newPath));

    m_usedOutputPaths.insert(newPath);
    return newPath;
}

// =============================================================================
// Batch export execution
// =============================================================================

bool XtExportDlg::validateBatchSettings()
{
    // Check source directory
    QString sourceDir = m_ui->leSourcePath->text();
    if (sourceDir.isEmpty()) {
        QMessageBox::warning(this, tr("批量导出"),
            tr("请选择源目录。"));
        m_ui->leSourcePath->setFocus();
        return false;
    }

    if (!QDir(sourceDir).exists()) {
        QMessageBox::warning(this, tr("批量导出"),
            tr("源目录不存在：\n%1").arg(sourceDir));
        m_ui->leSourcePath->setFocus();
        return false;
    }

    // Check output directory
    QString outputDir = m_ui->leOutputPath->text();
    if (outputDir.isEmpty()) {
        QMessageBox::warning(this, tr("批量导出"),
            tr("请选择输出目录。"));
        m_ui->leOutputPath->setFocus();
        return false;
    }

    return true;
}

bool XtExportDlg::checkOverwriteFile(const QString& outputPath)
{
    if (!QFile::exists(outputPath)) {
        return true;  // No conflict
    }

    // Check runtime flags first (from "All" buttons in dialog)
    if (m_batchOverwriteAll)
        return true;
    if (m_batchSkipAll)
        return false;

    int overwriteMode = m_ui->cbOverwriteMode->currentIndex();

    switch (overwriteMode) {
        case 0:  // Overwrite all
            return true;
        case 2:  // Skip existing
            return false;
        case 1:  // Ask for each file
            return showOverwriteDialog(outputPath);
        default:
            return true;
    }
}

bool XtExportDlg::showOverwriteDialog(const QString& outputPath)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("文件已存在"));
    msgBox.setText(tr("文件已存在：\n%1\n\n要执行什么操作？").arg(outputPath));

    QPushButton* btnOverwrite = msgBox.addButton(tr("覆盖"), QMessageBox::AcceptRole);
    QPushButton* btnSkip = msgBox.addButton(tr("跳过"), QMessageBox::RejectRole);
    QPushButton* btnOverwriteAll = msgBox.addButton(tr("全部覆盖"), QMessageBox::AcceptRole);
    QPushButton* btnSkipAll = msgBox.addButton(tr("全部跳过"), QMessageBox::RejectRole);
    QPushButton* btnCancel = msgBox.addButton(tr("取消"), QMessageBox::DestructiveRole);

    msgBox.setDefaultButton(btnSkip);
    msgBox.exec();

    QAbstractButton* clicked = msgBox.clickedButton();

    if (clicked == btnOverwrite) {
        return true;
    } else if (clicked == btnSkip) {
        return false;
    } else if (clicked == btnOverwriteAll) {
        m_batchOverwriteAll = true;
        return true;
    } else if (clicked == btnSkipAll) {
        m_batchSkipAll = true;
        return false;
    } else if (clicked == btnCancel) {
        // Set cancellation flag
        if (m_exportCallback) {
            m_exportCallback->setCancelled(true);
        }
        return false;
    }

    return false;
}

bool XtExportDlg::loadDocumentForBatch(const QString& filePath)
{
    // Access MainWindow to get the CR3View
    auto* mainWin = qobject_cast<MainWindow*>(parent());
    if (!mainWin)
        return false;

    CR3View* crView = mainWin->currentCRView();
    if (!crView)
        return false;

    // Disable history tracking for batch mode to avoid polluting history
    bool wasHistoryEnabled = crView->isHistoryEnabled();
    crView->setHistoryEnabled(false);

    // Load document
    bool success = crView->loadDocument(filePath);

    // Restore history tracking state
    crView->setHistoryEnabled(wasHistoryEnabled);

    if (success) {
        // Update our m_docView pointer to the new document's view
        m_docView = crView->getDocView();
    }

    // Process events to ensure UI updates
    QCoreApplication::processEvents();

    return success;
}

bool XtExportDlg::exportSingleFile(const QString& outputPath)
{
    if (!m_docView)
        return false;

    // Create and configure exporter
    XtcExporter exporter;
    configureExporter(exporter);

    // Full document export (no preview mode)
    exporter.setPreviewPage(-1);

    // In batch mode, export all pages (0 to max)
    exporter.setPageRange(0, INT_MAX);

    // Set callback for progress updates
    exporter.setProgressCallback(m_exportCallback);

    // Export
    lString32 output = qt2cr(outputPath);
    return exporter.exportDocument(m_docView, output.c_str());
}

void XtExportDlg::reloadOriginalDocument()
{
    if (m_originalDocumentPath.isEmpty())
        return;

    auto* mainWin = qobject_cast<MainWindow*>(parent());
    if (!mainWin)
        return;

    CR3View* crView = mainWin->currentCRView();
    if (!crView)
        return;

    crView->loadDocument(m_originalDocumentPath);

    // Update our m_docView pointer
    m_docView = crView->getDocView();
}

void XtExportDlg::showBatchSummary()
{
    QString outputDir = m_ui->leOutputPath->text();
    bool wasCancelled = m_exportCallback && m_exportCallback->wasCancelled();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("批量导出完成"));

    QString summary;
    if (wasCancelled) {
        summary = tr("导出已取消。\n\n");
    }

    summary += tr("已处理：%1 个文件\n"
                  "成功：%2\n"
                  "跳过：%3\n"
                  "失败：%4\n\n"
                  "输出目录：\n%5")
        .arg(m_batchCurrentIndex + 1)  // Files processed (including current partial)
        .arg(m_batchSuccessCount)
        .arg(m_batchSkipCount)
        .arg(m_batchErrorCount)
        .arg(outputDir);

    msgBox.setText(summary);

    QPushButton* btnOpenFolder = msgBox.addButton(tr("打开文件夹"), QMessageBox::ActionRole);
    QPushButton* btnOk = msgBox.addButton(QMessageBox::Ok);

    msgBox.setDefaultButton(btnOk);
    msgBox.exec();

    if (msgBox.clickedButton() == btnOpenFolder) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(outputDir));
    }
}

void XtExportDlg::performBatchExport()
{
    // Validate settings first
    if (!validateBatchSettings())
        return;

    // Scan source directory (includes cancellation check)
    if (!scanSourceDirectory()) {
        return;
    }

    // Validate/create output directory
    QString outputDir = m_ui->leOutputPath->text();
    QDir dir(outputDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            QMessageBox::critical(this, tr("批量导出"),
                tr("无法创建输出目录：\n%1").arg(outputDir));
            return;
        }
    }

    // Initialize state
    m_batchCurrentIndex = 0;
    m_batchSuccessCount = 0;
    m_batchSkipCount = 0;
    m_batchErrorCount = 0;
    m_batchOverwriteAll = false;
    m_batchSkipAll = false;
    m_usedOutputPaths.clear();

    // Store original title and preview page
    m_originalWindowTitle = windowTitle();
    m_originalPreviewPage = m_currentPreviewPage;

    // Create callback
    m_exportCallback = new QtExportCallback(this);

    // Enter exporting state
    setExporting(true);

    // Initialize files progress bar
    m_ui->progressBarFiles->setMaximum(m_batchFiles.size());
    m_ui->progressBarFiles->setValue(0);
    m_ui->lblFilesCount->setText(QString("0/%1").arg(m_batchFiles.size()));

    // Export loop
    for (int i = 0; i < m_batchFiles.size(); ++i) {
        if (m_exportCallback->wasCancelled()) {
            break;
        }

        m_batchCurrentIndex = i;
        QString sourceFile = m_batchFiles[i];
        QFileInfo fi(sourceFile);

        // Update window title
        setWindowTitle(QString("%1 - %2 (%3/%4)")
            .arg(m_originalWindowTitle)
            .arg(fi.fileName())
            .arg(i + 1)
            .arg(m_batchFiles.size()));

        // Update files progress
        m_ui->progressBarFiles->setValue(i);
        m_ui->lblFilesCount->setText(QString("%1/%2").arg(i).arg(m_batchFiles.size()));

        // Compute output path
        QString outputPath = computeBatchOutputPath(sourceFile);

        // Check overwrite
        if (!checkOverwriteFile(outputPath)) {
            if (m_exportCallback->wasCancelled()) {
                break;  // Cancel was clicked in overwrite dialog
            }
            m_batchSkipCount++;
            continue;
        }

        // Reset current progress
        m_ui->progressBar->setValue(0);

        // Load document
        if (!loadDocumentForBatch(sourceFile)) {
            m_batchErrorCount++;
            continue;
        }

        // Update preview with new document (always show first page in batch mode)
        m_currentPreviewPage = 0;
        m_updatingControls = true;
        m_ui->sbPreviewPage->setValue(1);  // SpinBox is 1-based
        m_updatingControls = false;
        renderPreview();

        // Export the document
        bool success = exportSingleFile(outputPath);

        if (m_exportCallback->wasCancelled()) {
            // Delete partial file
            QFile::remove(outputPath);
            break;
        }

        if (success) {
            m_batchSuccessCount++;
        } else {
            m_batchErrorCount++;
            // Delete partial file on error
            QFile::remove(outputPath);
        }
    }

    // Finalize files progress
    m_ui->progressBarFiles->setValue(m_batchFiles.size());
    m_ui->lblFilesCount->setText(QString("%1/%1").arg(m_batchFiles.size()));

    // Restore original title
    setWindowTitle(m_originalWindowTitle);

    // Show summary before cleanup (callback is still available)
    showBatchSummary();

    // Clean up
    delete m_exportCallback;
    m_exportCallback = nullptr;

    // Exit exporting state
    setExporting(false);

    // Reload original document and restore preview state
    reloadOriginalDocument();

    // Restore original preview page
    m_currentPreviewPage = m_originalPreviewPage;
    m_updatingControls = true;
    m_ui->sbPreviewPage->setValue(m_currentPreviewPage + 1);  // SpinBox is 1-based
    m_updatingControls = false;
    renderPreview();
}
