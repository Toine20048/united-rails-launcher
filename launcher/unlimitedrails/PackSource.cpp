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

#include "unlimitedrails/PackSource.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "Application.h"

namespace UnlimitedRails {

PackSource::PackSource(QObject* parent) : QObject(parent) {}

void PackSource::refresh()
{
    if (m_busy) {
        return;
    }
    m_busy = true;

    QNetworkRequest request{ QUrl(QString::fromUtf8(kApiVersionUrl)) };
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    auto* reply = APPLICATION->network()->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_busy = false;

        if (reply->error() != QNetworkReply::NoError) {
            emit refreshFailed(reply->errorString());
            return;
        }

        QJsonParseError parseError{};
        const auto doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit refreshFailed(tr("The server sent a malformed response."));
            return;
        }

        const auto root = doc.object();
        if (!root.value("ok").toBool()) {
            emit refreshFailed(tr("No modpack has been published yet."));
            return;
        }

        PackInfo parsed;
        parsed.version = root.value("version").toString();
        parsed.filename = root.value("filename").toString();
        parsed.sha256 = root.value("sha256").toString();
        parsed.changelog = root.value("changelog").toString();
        parsed.url = QUrl(root.value("url").toString());
        parsed.size = static_cast<qint64>(root.value("size").toDouble());

        // A pack with no version or no download is not something we can act on,
        // so treat it as a failure rather than showing a broken Play button.
        if (parsed.version.isEmpty() || !parsed.url.isValid()) {
            emit refreshFailed(tr("The server did not report a usable pack version."));
            return;
        }

        for (const auto& entry : root.value("screenshots").toArray()) {
            const auto shot = entry.toObject();
            const QUrl url(shot.value("url").toString());
            if (url.isValid()) {
                parsed.screenshots.append({ url, shot.value("caption").toString() });
            }
        }

        parsed.valid = true;
        m_info = parsed;
        emit refreshed();
    });
}

}  // namespace UnlimitedRails
