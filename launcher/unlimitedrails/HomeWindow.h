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

#pragma once

#include <QMainWindow>
#include <QPixmap>
#include <QVector>

#include "unlimitedrails/LauncherUpdate.h"
#include "unlimitedrails/PackSource.h"

class QComboBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QTextBrowser;
class QTimer;
class QToolButton;
class MinecraftInstance;

namespace UnlimitedRails {

/**
 * The entire launcher UI: one pack, one button.
 *
 * Replaces Prism's instance-list MainWindow. Everything the user can do lives
 * here — pick an account, see what's new, and play.
 */
class HomeWindow : public QMainWindow {
    Q_OBJECT
   public:
    explicit HomeWindow(QWidget* parent = nullptr);
    ~HomeWindow() override = default;

   signals:
    /** Application counts open windows with this and exits when none are left. */
    void isClosing();

   protected:
    void closeEvent(QCloseEvent* event) override;

   private slots:
    void onPackRefreshed();
    void onPackRefreshFailed(const QString& reason);
    void onPrimaryClicked();
    void onAddAccount();
    void onRemoveAccount();
    void showNextScreenshot();

    /** Opens the given instance page (mods, resourcepacks, console, settings…). */
    void openInstancePage(const QString& pageId);

    /** Re-reads total playtime from the instance and updates the label. */
    void refreshPlaytime();

    void onLauncherUpdateAvailable(const LauncherRelease& release);

   private:
    void buildUi();
    void refreshAccounts();
    void updatePrimaryButton();
    void loadScreenshots();
    void showScreenshot(int index);
    void setBusy(bool busy, const QString& what = {});

    /** Paints the current screenshot behind the controls, dimmed for legibility. */
    class Backdrop;
    Backdrop* m_backdrop = nullptr;

    /** The single instance this launcher manages, or nullptr if not installed. */
    MinecraftInstance* packInstance() const;

    /** Replaces the pack's own icon with the launcher mark. */
    void applyLauncherIcon(MinecraftInstance* instance);
    QString installedVersion() const;

    bool installOrUpdatePack();
    void launchPack();

    PackSource* m_packs = nullptr;
    LauncherUpdate* m_launcherUpdate = nullptr;
    LauncherRelease m_pendingRelease;

    QWidget* m_updateBar = nullptr;
    QLabel* m_updateText = nullptr;
    QLabel* m_playtime = nullptr;
    QLabel* m_headerVersion = nullptr;
    QLabel* m_shotCaption = nullptr;
    QTextBrowser* m_changelog = nullptr;
    QComboBox* m_accounts = nullptr;
    QPushButton* m_addAccount = nullptr;
    QToolButton* m_gear = nullptr;
    QPushButton* m_primary = nullptr;
    QLabel* m_status = nullptr;
    QProgressBar* m_progress = nullptr;
    QTimer* m_shotTimer = nullptr;

    QVector<QPixmap> m_shotPixmaps;
    QVector<QString> m_shotCaptions;
    int m_shotIndex = -1;
    bool m_busy = false;
};

}  // namespace UnlimitedRails
