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

#include "xtexportprofile.h"
#include "crqtutil.h"

#include <QSettings>
#include <QDir>
#include <QFileInfo>

#include <crlog.h>

#include <algorithm>
#include <cmath>

// =============================================================================
// XtExportProfile - INI key names
// =============================================================================

const char* const XtExportProfile::KEY_NAME = "Profile/name";
const char* const XtExportProfile::KEY_FORMAT = "Profile/format";
const char* const XtExportProfile::KEY_EXTENSION = "Profile/extension";
const char* const XtExportProfile::KEY_WIDTH = "Profile/width";
const char* const XtExportProfile::KEY_HEIGHT = "Profile/height";
const char* const XtExportProfile::KEY_BPP = "Profile/bpp";
const char* const XtExportProfile::KEY_DITHER_MODE = "Dithering/mode";
const char* const XtExportProfile::KEY_GRAY_POLICY = "Dithering/gray_policy";
const char* const XtExportProfile::KEY_THRESHOLD = "Dithering/threshold";
const char* const XtExportProfile::KEY_ERROR_DIFFUSION = "Dithering/error_diffusion";
const char* const XtExportProfile::KEY_GAMMA = "Dithering/gamma";
const char* const XtExportProfile::KEY_SERPENTINE = "Dithering/serpentine";
const char* const XtExportProfile::KEY_CHAPTERS_ENABLED = "Chapters/enabled";
const char* const XtExportProfile::KEY_CHAPTER_DEPTH = "Chapters/depth";
const char* const XtExportProfile::KEY_ORDER = "Profile/order";
const char* const XtExportProfile::KEY_LOCKED = "Profile/locked";

// Default profile filenames
const char* const XtExportProfile::DEFAULT_XTC_FILENAME = "xtp_xtc.ini";
const char* const XtExportProfile::DEFAULT_XTCH_FILENAME = "xtp_xtch.ini";
const char* const XtExportProfile::DEFAULT_X3_XTC_FILENAME = "xtp_xtc3.ini";
const char* const XtExportProfile::DEFAULT_X3_XTCH_FILENAME = "xtp_xtch3.ini";
const char* const XtExportProfile::DEFAULT_CUSTOM_FILENAME = "xtp_custom.ini";

// =============================================================================
// XtExportProfile implementation
// =============================================================================

XtExportProfile::XtExportProfile()
    : order(100)
    , locked(false)
    , format(XTC_FORMAT_XTC)
    , width(480)
    , height(800)
    , bpp(1)
    , ditherMode(IMAGE_DITHER_FS_1BIT)
    , grayPolicy(GRAY_SPLIT_LIGHT_DARK)
    , chaptersEnabled(true)
    , chapterDepth(1) {
    // Get default dithering options from engine
    ditheringOpts = getDefault1BitDitheringOptions();
}

bool XtExportProfile::load(const QString& filepath) {
    QFileInfo fi(filepath);
    if (!fi.exists() || !fi.isReadable()) {
        CRLog::warn("XtExportProfile: Cannot read file: %s", filepath.toUtf8().constData());
        return false;
    }

    QSettings settings(filepath, QSettings::IniFormat);
    if (settings.status() != QSettings::NoError) {
        CRLog::warn("XtExportProfile: Failed to parse INI file: %s", filepath.toUtf8().constData());
        return false;
    }

    // Store filename
    filename = fi.fileName();

    // Profile section
    name = settings.value(KEY_NAME, "Unnamed Profile").toString();
    order = settings.value(KEY_ORDER, 100).toInt();
    locked = settings.value(KEY_LOCKED, false).toBool();
    format = stringToFormat(settings.value(KEY_FORMAT, "xtc").toString());
    extension = settings.value(KEY_EXTENSION, "xtc").toString();
    width = settings.value(KEY_WIDTH, 480).toInt();
    height = settings.value(KEY_HEIGHT, 800).toInt();
    bpp = settings.value(KEY_BPP, 1).toInt();

    // Dithering section
    ditherMode = stringToDitherMode(settings.value(KEY_DITHER_MODE, "fs_1bit").toString());
    grayPolicy = stringToGrayPolicy(settings.value(KEY_GRAY_POLICY, "split_light_dark").toString());

    // Get appropriate defaults based on bit depth
    DitheringOptions defaults = (bpp == 1) ? getDefault1BitDitheringOptions() : getDefault2BitDitheringOptions();

    ditheringOpts.threshold = settings.value(KEY_THRESHOLD, defaults.threshold).toFloat();
    ditheringOpts.errorDiffusion = settings.value(KEY_ERROR_DIFFUSION, defaults.errorDiffusion).toFloat();
    ditheringOpts.gamma = settings.value(KEY_GAMMA, defaults.gamma).toFloat();
    ditheringOpts.serpentine = settings.value(KEY_SERPENTINE, defaults.serpentine).toBool();

    // Chapters section
    chaptersEnabled = settings.value(KEY_CHAPTERS_ENABLED, true).toBool();
    chapterDepth = settings.value(KEY_CHAPTER_DEPTH, 1).toInt();

    CRLog::info("XtExportProfile: Loaded profile '%s' from %s",
                name.toUtf8().constData(), filepath.toUtf8().constData());
    return true;
}

bool XtExportProfile::save(const QString& filepath) const {
    QSettings settings(filepath, QSettings::IniFormat);

    // Profile section
    settings.setValue(KEY_NAME, name);
    settings.setValue(KEY_ORDER, order);
    settings.setValue(KEY_LOCKED, locked);
    settings.setValue(KEY_FORMAT, formatToString(format));
    settings.setValue(KEY_EXTENSION, extension);
    settings.setValue(KEY_WIDTH, width);
    settings.setValue(KEY_HEIGHT, height);
    settings.setValue(KEY_BPP, bpp);

    // Dithering section
    settings.setValue(KEY_DITHER_MODE, ditherModeToString(ditherMode));
    settings.setValue(KEY_GRAY_POLICY, grayPolicyToString(grayPolicy));
    // Round to 2 decimal places to avoid floating-point representation artifacts (e.g., 0.7 → 0.699999988...)
    settings.setValue(KEY_THRESHOLD, std::round(ditheringOpts.threshold * 100.0) / 100.0);
    settings.setValue(KEY_ERROR_DIFFUSION, std::round(ditheringOpts.errorDiffusion * 100.0) / 100.0);
    settings.setValue(KEY_GAMMA, std::round(ditheringOpts.gamma * 100.0) / 100.0);
    settings.setValue(KEY_SERPENTINE, ditheringOpts.serpentine);

    // Chapters section
    settings.setValue(KEY_CHAPTERS_ENABLED, chaptersEnabled);
    settings.setValue(KEY_CHAPTER_DEPTH, chapterDepth);

    settings.sync();

    if (settings.status() != QSettings::NoError) {
        CRLog::error("XtExportProfile: Failed to save profile to %s", filepath.toUtf8().constData());
        return false;
    }

    CRLog::info("XtExportProfile: Saved profile '%s' to %s",
                name.toUtf8().constData(), filepath.toUtf8().constData());
    return true;
}

bool XtExportProfile::isValid() const {
    return !name.isEmpty() && width > 0 && height > 0 && (bpp == 1 || bpp == 2);
}

XtExportProfile XtExportProfile::defaultXtcProfile() {
    XtExportProfile profile;
    profile.name = "X4 (480x800) fast (1-bit)";
    profile.filename = DEFAULT_XTC_FILENAME;
    profile.order = 10;
    profile.locked = true;
    profile.format = XTC_FORMAT_XTC;
    profile.extension = "xtc";
    profile.width = 480;
    profile.height = 800;
    profile.bpp = 1;
    profile.ditherMode = IMAGE_DITHER_FS_1BIT;
    profile.grayPolicy = GRAY_SPLIT_LIGHT_DARK;
    profile.ditheringOpts = getDefault1BitDitheringOptions();
    profile.chaptersEnabled = true;
    profile.chapterDepth = 1;
    return profile;
}

XtExportProfile XtExportProfile::defaultXtchProfile() {
    XtExportProfile profile;
    profile.name = "X4 (480x800) quality (2-bit)";
    profile.filename = DEFAULT_XTCH_FILENAME;
    profile.order = 11;
    profile.locked = true;
    profile.format = XTC_FORMAT_XTCH;
    profile.extension = "xtc";
    profile.width = 480;
    profile.height = 800;
    profile.bpp = 2;
    profile.ditherMode = IMAGE_DITHER_FS_2BIT;
    profile.grayPolicy = GRAY_SPLIT_LIGHT_DARK;
    profile.ditheringOpts = getDefault2BitDitheringOptions();
    profile.chaptersEnabled = true;
    profile.chapterDepth = 1;
    return profile;
}

XtExportProfile XtExportProfile::defaultCustomProfile() {
    XtExportProfile profile;
    profile.name = "Custom";
    profile.filename = DEFAULT_CUSTOM_FILENAME;
    profile.order = 50; // End of list
    profile.locked = false;
    profile.format = XTC_FORMAT_XTC;
    profile.extension = "xtc";
    profile.width = 480;
    profile.height = 800;
    profile.bpp = 1;
    profile.ditherMode = IMAGE_DITHER_FS_1BIT;
    profile.grayPolicy = GRAY_SPLIT_LIGHT_DARK;
    profile.ditheringOpts = getDefault1BitDitheringOptions();
    profile.chaptersEnabled = true;
    profile.chapterDepth = 1;
    return profile;
}

// String <-> enum conversion helpers

QString XtExportProfile::formatToString(XtcExportFormat format) {
    switch (format) {
        case XTC_FORMAT_XTC: return "xtc";
        case XTC_FORMAT_XTCH: return "xtch";
        default: return "xtc";
    }
}

XtcExportFormat XtExportProfile::stringToFormat(const QString& str) {
    if (str.toLower() == "xtch") return XTC_FORMAT_XTCH;
    return XTC_FORMAT_XTC;
}

QString XtExportProfile::ditherModeToString(ImageDitherMode mode) {
    switch (mode) {
        case IMAGE_DITHER_NONE: return "none";
        case IMAGE_DITHER_ORDERED: return "ordered";
        case IMAGE_DITHER_FS_2BIT: return "fs_2bit";
        case IMAGE_DITHER_FS_1BIT: return "fs_1bit";
        default: return "ordered";
    }
}

ImageDitherMode XtExportProfile::stringToDitherMode(const QString& str) {
    QString s = str.toLower();
    if (s == "none") return IMAGE_DITHER_NONE;
    if (s == "ordered") return IMAGE_DITHER_ORDERED;
    if (s == "fs_2bit") return IMAGE_DITHER_FS_2BIT;
    if (s == "fs_1bit") return IMAGE_DITHER_FS_1BIT;
    return IMAGE_DITHER_ORDERED;
}

QString XtExportProfile::grayPolicyToString(GrayToMonoPolicy policy) {
    switch (policy) {
        case GRAY_SPLIT_LIGHT_DARK: return "split_light_dark";
        case GRAY_ALL_TO_WHITE: return "all_to_white";
        case GRAY_ALL_TO_BLACK: return "all_to_black";
        default: return "split_light_dark";
    }
}

GrayToMonoPolicy XtExportProfile::stringToGrayPolicy(const QString& str) {
    QString s = str.toLower();
    if (s == "all_to_white") return GRAY_ALL_TO_WHITE;
    if (s == "all_to_black") return GRAY_ALL_TO_BLACK;
    return GRAY_SPLIT_LIGHT_DARK;
}

// =============================================================================
// XtExportProfileManager implementation
// =============================================================================

XtExportProfileManager::XtExportProfileManager() {
}

XtExportProfileManager::~XtExportProfileManager() {
    qDeleteAll(m_profiles);
    m_profiles.clear();
}

void XtExportProfileManager::initialize() {
    // Get config directory (same as crui.ini location)
    m_configDir = cr2qt(getConfigDir());

    // Ensure directory exists
    QDir dir(m_configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Create any missing default profiles (handles upgrades with new profiles)
    createDefaultProfiles();

    // Discover all profiles
    discoverProfiles();

    CRLog::info("XtExportProfileManager: Initialized with %d profiles", m_profiles.size());
}

void XtExportProfileManager::discoverProfiles() {
    // Clear existing profiles
    qDeleteAll(m_profiles);
    m_profiles.clear();
    m_filenameToIndex.clear();

    QDir dir(m_configDir);
    QStringList filters;
    filters << "xtp_*.ini";
    QStringList files = dir.entryList(filters, QDir::Files | QDir::Readable, QDir::Name);

    for (const QString& file : files) {
        QString filepath = m_configDir + file;
        XtExportProfile* profile = new XtExportProfile();
        if (profile->load(filepath)) {
            m_profiles.append(profile);
        } else {
            CRLog::warn("XtExportProfileManager: Skipping invalid profile: %s", file.toUtf8().constData());
            delete profile;
        }
    }

    // Sort by order (ascending), then by name alphabetically for equal orders
    std::sort(m_profiles.begin(), m_profiles.end(), [](const XtExportProfile* a, const XtExportProfile* b) {
        if (a->order != b->order)
            return a->order < b->order;
        return a->name.compare(b->name, Qt::CaseInsensitive) < 0;
    });

    // Rebuild filename-to-index map after sorting
    for (int i = 0; i < m_profiles.size(); ++i) {
        m_filenameToIndex[m_profiles[i]->filename] = i;
    }
}

void XtExportProfileManager::createDefaultProfiles() {
    // Create Custom profile if it doesn't exist (unlocked, user-modifiable)
    if (!QFileInfo::exists(m_configDir + XtExportProfile::DEFAULT_CUSTOM_FILENAME)) {
        CRLog::info("XtExportProfileManager: Creating default Custom profile");
        XtExportProfile customProfile = XtExportProfile::defaultCustomProfile();
        saveDefaultProfile(customProfile, XtExportProfile::DEFAULT_CUSTOM_FILENAME);
    }

    // Create default XTC profile if it doesn't exist
    if (!QFileInfo::exists(m_configDir + XtExportProfile::DEFAULT_XTC_FILENAME)) {
        CRLog::info("XtExportProfileManager: Creating default XTC profile");
        XtExportProfile xtcProfile = XtExportProfile::defaultXtcProfile();
        saveDefaultProfile(xtcProfile, XtExportProfile::DEFAULT_XTC_FILENAME);
    }

    // Create default XTCH profile if it doesn't exist
    if (!QFileInfo::exists(m_configDir + XtExportProfile::DEFAULT_XTCH_FILENAME)) {
        CRLog::info("XtExportProfileManager: Creating default XTCH profile");
        XtExportProfile xtchProfile = XtExportProfile::defaultXtchProfile();
        saveDefaultProfile(xtchProfile, XtExportProfile::DEFAULT_XTCH_FILENAME);
    }

    // Create X3 XTC profile if it doesn't exist
    if (!QFileInfo::exists(m_configDir + XtExportProfile::DEFAULT_X3_XTC_FILENAME)) {
        CRLog::info("XtExportProfileManager: Creating default XTC X3 profile");
        XtExportProfile xtcProfile = XtExportProfile::defaultXtcProfile();
        xtcProfile.name = "X3 (528x792) fast (1-bit)";
        xtcProfile.filename = XtExportProfile::DEFAULT_X3_XTC_FILENAME;
        xtcProfile.order = 20;
        xtcProfile.locked = true;
        xtcProfile.width = 528;
        xtcProfile.height = 792;
        saveDefaultProfile(xtcProfile, XtExportProfile::DEFAULT_X3_XTC_FILENAME);
    }

    // Create X3 XTCH profile if it doesn't exist
    if (!QFileInfo::exists(m_configDir + XtExportProfile::DEFAULT_X3_XTCH_FILENAME)) {
        CRLog::info("XtExportProfileManager: Creating default XTCH X3 profile");
        XtExportProfile xtchProfile = XtExportProfile::defaultXtchProfile();
        xtchProfile.name = "X3 (528x792) quality (2-bit)";
        xtchProfile.filename = XtExportProfile::DEFAULT_X3_XTCH_FILENAME;
        xtchProfile.order = 21;
        xtchProfile.locked = true;
        xtchProfile.width = 528;
        xtchProfile.height = 792;
        saveDefaultProfile(xtchProfile, XtExportProfile::DEFAULT_X3_XTCH_FILENAME);
    }
}

bool XtExportProfileManager::saveDefaultProfile(const XtExportProfile& profile, const QString& filename) {
    QString filepath = m_configDir + filename;
    return profile.save(filepath);
}

QStringList XtExportProfileManager::profileNames() const {
    QStringList names;
    for (const XtExportProfile* profile : m_profiles) {
        names.append(profile->name);
    }
    return names;
}

QStringList XtExportProfileManager::profileFilenames() const {
    QStringList filenames;
    for (const XtExportProfile* profile : m_profiles) {
        filenames.append(profile->filename);
    }
    return filenames;
}

XtExportProfile* XtExportProfileManager::profileByFilename(const QString& filename) {
    auto it = m_filenameToIndex.find(filename);
    if (it != m_filenameToIndex.end()) {
        return m_profiles.at(it.value());
    }
    return nullptr;
}

XtExportProfile* XtExportProfileManager::profileByIndex(int index) {
    if (index >= 0 && index < m_profiles.size()) {
        return m_profiles.at(index);
    }
    return nullptr;
}

int XtExportProfileManager::profileCount() const {
    return m_profiles.size();
}

int XtExportProfileManager::indexOfProfile(const QString& filename) const {
    auto it = m_filenameToIndex.find(filename);
    if (it != m_filenameToIndex.end()) {
        return it.value();
    }
    return -1;
}

bool XtExportProfileManager::saveProfile(XtExportProfile* profile) {
    if (!profile || profile->filename.isEmpty()) {
        return false;
    }
    QString filepath = m_configDir + profile->filename;
    return profile->save(filepath);
}

QString XtExportProfileManager::configDir() const {
    return m_configDir;
}
