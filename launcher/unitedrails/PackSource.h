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
#include <QVector>

namespace UnitedRails {

/** Where the launcher looks for the pack and everything shown on the home screen. */
constexpr const char* kApiVersionUrl = "https://united-rails.net/api/version.php";

/** Settings keys owned by this fork. */
constexpr const char* kSettingInstanceId = "URPackInstanceID";
constexpr const char* kSettingPackVersion = "URPackVersion";

struct Screenshot {
    QUrl url;
    QString caption;
};

struct PackInfo {
    bool valid = false;
    QString version;
    QString filename;
    QString sha256;
    QString changelog;
    QUrl url;
    qint64 size = 0;
    QVector<Screenshot> screenshots;
};

/**
 * Polls the site for what the current pack is. One request, no caching beyond
 * the process, because the launcher only asks on startup and on manual refresh.
 */
class PackSource : public QObject {
    Q_OBJECT
   public:
    explicit PackSource(QObject* parent = nullptr);

    void refresh();
    const PackInfo& info() const { return m_info; }
    bool busy() const { return m_busy; }

   signals:
    void refreshed();
    void refreshFailed(const QString& reason);

   private:
    PackInfo m_info;
    bool m_busy = false;
};

}  // namespace UnitedRails
