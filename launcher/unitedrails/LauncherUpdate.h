// SPDX-License-Identifier: GPL-3.0-only
/*
 *  United Rails Launcher - a Prism Launcher fork for one modpack
 *  Copyright (C) 2026 United Rails
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

namespace UnitedRails {

/** Where the launcher asks what the newest release is. */
constexpr const char* kApiLauncherUrl = "https://united-rails.net/api/launcher.php";

struct LauncherRelease {
    QString version;
    QString notes;
    QUrl installer;
    QUrl page;
};

/**
 * Checks whether a newer launcher has been released, and installs it.
 *
 * Upstream's updater is disabled in this fork because it points at Prism's own
 * releases. This replaces it with one that points at ours.
 */
class LauncherUpdate : public QObject {
    Q_OBJECT
   public:
    explicit LauncherUpdate(QObject* parent = nullptr);

    /** Asks the site what the newest release is. Silent on failure. */
    void check();

    /**
     * Downloads the installer and runs it, then asks the app to quit so the
     * installer can replace the running executable. The NSIS installer closes
     * any running copy itself, but quitting first keeps that from looking like
     * a crash.
     */
    void downloadAndRun(const LauncherRelease& release, QWidget* parent);

    /** Compares dotted version numbers, e.g. "1.0.10" > "1.0.9". */
    static bool isNewer(const QString& candidate, const QString& current);

    /**
     * This build's version as plain "major.minor.patch".
     * Not printableVersionString(), which appends the git branch and would
     * never compare cleanly against a release tag.
     */
    static QString currentVersion();

   signals:
    void updateAvailable(const LauncherRelease& release);

   private:
    bool m_busy = false;
};

}  // namespace UnitedRails
