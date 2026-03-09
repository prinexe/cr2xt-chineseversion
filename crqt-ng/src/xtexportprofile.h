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

#ifndef XTEXPORTPROFILE_H
#define XTEXPORTPROFILE_H

#include <QString>
#include <QStringList>
#include <QMap>

#include <xtcexport.h>
#include <lvbasedrawbuf.h>

/**
 * @brief XT* export profile containing all export settings
 *
 * Profiles are stored in xtp_*.ini files in the same directory as crui.ini.
 * Each profile defines format, dimensions, dithering settings, and chapter options.
 */
class XtExportProfile
{
public:
    XtExportProfile();

    // Profile identification
    QString name;           ///< Display name shown in UI dropdown
    QString filename;       ///< INI filename (e.g., "xtp_xtc.ini")
    int order;              ///< Sort order in UI (lower values appear first)

    // Profile behavior
    bool locked;            ///< If true, format/width/height cannot be changed in UI

    // Format settings
    XtcExportFormat format; ///< XTC_FORMAT_XTC or XTC_FORMAT_XTCH
    QString extension;      ///< File extension for export (e.g., "xtc")
    int width;              ///< Output width in pixels
    int height;             ///< Output height in pixels
    int bpp;                ///< Bits per pixel (1 or 2)

    // Dithering settings
    ImageDitherMode ditherMode;     ///< Dithering algorithm
    GrayToMonoPolicy grayPolicy;    ///< Gray to mono conversion policy
    DitheringOptions ditheringOpts; ///< Floyd-Steinberg parameters

    // Chapter settings
    bool chaptersEnabled;   ///< Include chapter markers
    int chapterDepth;       ///< Maximum TOC depth (1-3)

    /**
     * @brief Load profile from INI file
     * @param filepath Full path to INI file
     * @return true on success, false on error
     */
    bool load(const QString& filepath);

    /**
     * @brief Save profile to INI file
     * @param filepath Full path to INI file
     * @return true on success, false on error
     */
    bool save(const QString& filepath) const;

    /**
     * @brief Check if profile is valid
     * @return true if profile has valid settings
     */
    bool isValid() const;

    /**
     * @brief Get default XTC (1-bit) profile
     * @return Profile with default 1-bit settings
     */
    static XtExportProfile defaultXtcProfile();

    /**
     * @brief Get default XTCH (2-bit) profile
     * @return Profile with default 2-bit settings
     */
    static XtExportProfile defaultXtchProfile();

    /**
     * @brief Get default custom profile (unlocked, based on X4 XTC)
     * @return Profile with default settings and locked=false
     */
    static XtExportProfile defaultCustomProfile();

    // Default profile filenames
    static const char* const DEFAULT_XTC_FILENAME;
    static const char* const DEFAULT_XTCH_FILENAME;
    static const char* const DEFAULT_X3_XTC_FILENAME;
    static const char* const DEFAULT_X3_XTCH_FILENAME;
    static const char* const DEFAULT_CUSTOM_FILENAME;

private:
    // INI key names
    static const char* const KEY_NAME;
    static const char* const KEY_FORMAT;
    static const char* const KEY_EXTENSION;
    static const char* const KEY_WIDTH;
    static const char* const KEY_HEIGHT;
    static const char* const KEY_BPP;
    static const char* const KEY_DITHER_MODE;
    static const char* const KEY_GRAY_POLICY;
    static const char* const KEY_THRESHOLD;
    static const char* const KEY_ERROR_DIFFUSION;
    static const char* const KEY_GAMMA;
    static const char* const KEY_SERPENTINE;
    static const char* const KEY_CHAPTERS_ENABLED;
    static const char* const KEY_CHAPTER_DEPTH;
    static const char* const KEY_ORDER;
    static const char* const KEY_LOCKED;

    // String <-> enum conversion helpers
    static QString formatToString(XtcExportFormat format);
    static XtcExportFormat stringToFormat(const QString& str);
    static QString ditherModeToString(ImageDitherMode mode);
    static ImageDitherMode stringToDitherMode(const QString& str);
    static QString grayPolicyToString(GrayToMonoPolicy policy);
    static GrayToMonoPolicy stringToGrayPolicy(const QString& str);
};


/**
 * @brief Manager for XT* export profiles
 *
 * Discovers and manages profile files (xtp_*.ini) in the config directory.
 * Creates default profiles if none exist.
 */
class XtExportProfileManager
{
public:
    XtExportProfileManager();
    ~XtExportProfileManager();

    /**
     * @brief Initialize manager and discover profiles
     *
     * Scans config directory for xtp_*.ini files.
     * Creates default profiles if none found.
     */
    void initialize();

    /**
     * @brief Get list of profile display names for dropdown
     * @return List of profile names
     */
    QStringList profileNames() const;

    /**
     * @brief Get list of profile filenames
     * @return List of filenames (e.g., "xtp_xtc.ini")
     */
    QStringList profileFilenames() const;

    /**
     * @brief Get profile by filename
     * @param filename Profile filename (e.g., "xtp_xtc.ini")
     * @return Pointer to profile or nullptr if not found
     */
    XtExportProfile* profileByFilename(const QString& filename);

    /**
     * @brief Get profile by index
     * @param index Profile index
     * @return Pointer to profile or nullptr if out of range
     */
    XtExportProfile* profileByIndex(int index);

    /**
     * @brief Get number of loaded profiles
     * @return Profile count
     */
    int profileCount() const;

    /**
     * @brief Get index of profile by filename
     * @param filename Profile filename
     * @return Index or -1 if not found
     */
    int indexOfProfile(const QString& filename) const;

    /**
     * @brief Save profile to its INI file
     * @param profile Profile to save
     * @return true on success
     */
    bool saveProfile(XtExportProfile* profile);

    /**
     * @brief Get config directory path
     * @return Path to directory containing profile files
     */
    QString configDir() const;

private:
    /**
     * @brief Discover profile files in config directory
     */
    void discoverProfiles();

    /**
     * @brief Create default profiles if none exist
     */
    void createDefaultProfiles();

    /**
     * @brief Save a default profile to file
     * @param profile Profile to save
     * @param filename Filename to use
     * @return true on success
     */
    bool saveDefaultProfile(const XtExportProfile& profile, const QString& filename);

    QString m_configDir;                        ///< Config directory path
    QList<XtExportProfile*> m_profiles;         ///< Loaded profiles
    QMap<QString, int> m_filenameToIndex;       ///< Filename to index map
};

#endif // XTEXPORTPROFILE_H
