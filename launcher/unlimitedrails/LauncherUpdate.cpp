// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Unlimited Rails Launcher - a Prism Launcher fork for one modpack
 *  Copyright (C) 2026 Unlimited Rails
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

#include "unlimitedrails/LauncherUpdate.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressDialog>
#include <QStandardPaths>

#include "Application.h"
#include "BuildConfig.h"
#include "ui/dialogs/CustomMessageBox.h"

namespace UnlimitedRails {

LauncherUpdate::LauncherUpdate(QObject* parent) : QObject(parent) {}

QString LauncherUpdate::currentVersion()
{
    return QStringLiteral("%1.%2.%3")
        .arg(BuildConfig.VERSION_MAJOR)
        .arg(BuildConfig.VERSION_MINOR)
        .arg(BuildConfig.VERSION_PATCH);
}

bool LauncherUpdate::isNewer(const QString& candidate, const QString& current)
{
    const auto split = [](const QString& v) {
        QList<int> parts;
        for (const auto& piece : v.split(QLatin1Char('.'))) {
            // Tolerate suffixes like "1.0.3-beta" by taking the leading digits.
            parts.append(piece.split(QRegularExpression(QStringLiteral("[^0-9]")))[0].toInt());
        }
        return parts;
    };

    const auto a = split(candidate);
    const auto b = split(current);
    for (int i = 0; i < qMax(a.size(), b.size()); ++i) {
        const int left = i < a.size() ? a[i] : 0;
        const int right = i < b.size() ? b[i] : 0;
        if (left != right) {
            return left > right;
        }
    }
    return false;
}

void LauncherUpdate::check()
{
    if (m_busy) {
        return;
    }
    m_busy = true;

    QNetworkRequest request{ QUrl(QString::fromUtf8(kApiLauncherUrl)) };
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto* reply = APPLICATION->network()->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_busy = false;

        if (reply->error() != QNetworkReply::NoError) {
            return;  // an unreachable site must never block playing
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            return;
        }
        const auto root = doc.object();
        if (!root.value("ok").toBool()) {
            return;
        }

        LauncherRelease release;
        release.version = root.value("version").toString();
        release.notes = root.value("notes").toString();
        release.installer = QUrl(root.value("installer").toString());
        release.page = QUrl(root.value("page").toString());

        if (release.version.isEmpty() || !isNewer(release.version, currentVersion())) {
            return;
        }
        emit updateAvailable(release);
    });
}

void LauncherUpdate::downloadAndRun(const LauncherRelease& release, QWidget* parent)
{
    if (!release.installer.isValid()) {
        // No installer asset: send them to the release page instead of failing.
        if (release.page.isValid()) {
            QDesktopServices::openUrl(release.page);
        }
        return;
    }

    const QString target = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                               .filePath(QStringLiteral("UnlimitedRails-%1-Setup.exe").arg(release.version));

    auto* progress = new QProgressDialog(tr("Downloading launcher %1…").arg(release.version), tr("Cancel"), 0, 100, parent);
    progress->setWindowTitle(tr("Updating launcher"));
    progress->setMinimumDuration(0);
    progress->setAutoClose(false);
    progress->setValue(0);

    QNetworkRequest request{ release.installer };
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto* reply = APPLICATION->network()->get(request);

    connect(reply, &QNetworkReply::downloadProgress, progress, [progress](qint64 got, qint64 total) {
        if (total > 0) {
            progress->setValue(static_cast<int>(got * 100 / total));
        }
    });
    connect(progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this, [this, reply, progress, target, release, parent]() {
        reply->deleteLater();
        progress->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            if (reply->error() != QNetworkReply::OperationCanceledError) {
                CustomMessageBox::selectable(parent, tr("Update failed"),
                                             tr("Could not download the update:\n\n%1").arg(reply->errorString()),
                                             QMessageBox::Warning)
                    ->show();
            }
            return;
        }

        QFile file(target);
        if (!file.open(QIODevice::WriteOnly) || file.write(reply->readAll()) < 0) {
            CustomMessageBox::selectable(parent, tr("Update failed"), tr("Could not save the downloaded installer."),
                                         QMessageBox::Warning)
                ->show();
            return;
        }
        file.close();

        if (!QProcess::startDetached(target, {})) {
            CustomMessageBox::selectable(parent, tr("Update failed"), tr("Could not start the installer."), QMessageBox::Warning)
                ->show();
            return;
        }

        // The installer closes any running copy itself, but quitting first
        // means the user does not see the launcher killed from under them.
        APPLICATION->quit();
    });
}

}  // namespace UnlimitedRails
