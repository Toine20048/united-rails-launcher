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

#include "unitedrails/HomeWindow.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QEasingCurve>
#include <QFrame>
#include <QMenu>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QNetworkReply>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariantAnimation>
#include <QWidget>

#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "MMCTime.h"
#include "icons/IconList.h"
#include "icons/MMCIcon.h"
#include "InstanceImportTask.h"
#include "LaunchMode.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "ui/dialogs/AboutDialog.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/MSALoginDialog.h"
#include "ui/dialogs/ProgressDialog.h"

namespace UnitedRails {

namespace {
constexpr int kScreenshotIntervalMs = 8000;
constexpr const char* kInstanceName = "Neo Rails";
constexpr const char* kIconKey = "unitedrails";

/**
 * The backdrop is not dimmed as a whole — that washes the screenshot out.
 * Instead only the bands where text sits get a gradient, so the middle of the
 * image stays at full brightness.
 */
constexpr double kTopScrimFraction = 0.18;
constexpr double kBottomScrimFraction = 0.45;
constexpr int kTopScrimAlpha = 170;
constexpr int kBottomScrimAlpha = 225;

/** Cross-fade length when the backdrop changes. */
constexpr int kFadeMs = 900;

/** Header logo height, downscaled from the 128px tiled render. */
constexpr int kHeaderLogoHeight = 46;

/**
 * Makes every newline in the changelog a visible line break.
 *
 * Markdown treats a single newline as a soft wrap, so lines typed separately in
 * the admin panel would run together into one paragraph. Two trailing spaces is
 * Markdown's hard line break, so adding them preserves what was typed while
 * leaving headings, lists and emphasis working as normal.
 */
QString hardWrapLines(const QString& markdown)
{
    QStringList lines = markdown.split(QLatin1Char('\n'));
    for (auto& line : lines) {
        QString trimmedEnd = line;
        while (trimmedEnd.endsWith(QLatin1Char(' ')) || trimmedEnd.endsWith(QLatin1Char('\r'))) {
            trimmedEnd.chop(1);
        }
        // Blank lines already separate paragraphs and must stay blank.
        if (!trimmedEnd.isEmpty()) {
            line = trimmedEnd + QStringLiteral("  ");
        } else {
            line = trimmedEnd;
        }
    }
    return lines.join(QLatin1Char('\n'));
}
}  // namespace

/**
 * Central widget that draws the current screenshot scaled to cover, with a
 * dark scrim on top. Everything else is laid out over this.
 */
class HomeWindow::Backdrop : public QWidget {
   public:
    explicit Backdrop(QWidget* parent) : QWidget(parent)
    {
        m_fade = new QVariantAnimation(this);
        m_fade->setDuration(kFadeMs);
        m_fade->setStartValue(0.0);
        m_fade->setEndValue(1.0);
        m_fade->setEasingCurve(QEasingCurve::InOutQuad);
        connect(m_fade, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
            m_fadeProgress = value.toDouble();
            update();
        });
        connect(m_fade, &QVariantAnimation::finished, this, [this]() {
            // Once faded in, the old image is no longer needed.
            m_outgoing = QPixmap();
        });
    }

    void setImage(const QPixmap& image)
    {
        m_fade->stop();
        // Cross-fade from whatever is currently on screen, including a
        // part-faded frame, so rapid changes never snap.
        m_outgoing = m_image;
        m_image = image;
        m_fadeProgress = m_outgoing.isNull() ? 1.0 : 0.0;
        if (!m_outgoing.isNull()) {
            m_fade->start();
        }
        update();
    }

   protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.fillRect(rect(), QColor(10, 10, 11));

        if (!m_outgoing.isNull()) {
            drawCover(painter, m_outgoing);
        }
        if (!m_image.isNull()) {
            painter.setOpacity(m_fadeProgress);
            drawCover(painter, m_image);
            painter.setOpacity(1.0);
        }

        if (m_image.isNull()) {
            return;  // nothing to protect text against on a plain background
        }

        // Top band: behind the title and version.
        const int topHeight = static_cast<int>(height() * kTopScrimFraction);
        QLinearGradient top(0, 0, 0, topHeight);
        top.setColorAt(0.0, QColor(0, 0, 0, kTopScrimAlpha));
        top.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(QRect(0, 0, width(), topHeight), top);

        // Bottom band: behind the changelog, account row and Play button.
        const int bottomHeight = static_cast<int>(height() * kBottomScrimFraction);
        QLinearGradient bottom(0, height() - bottomHeight, 0, height());
        bottom.setColorAt(0.0, QColor(0, 0, 0, 0));
        bottom.setColorAt(1.0, QColor(0, 0, 0, kBottomScrimAlpha));
        painter.fillRect(QRect(0, height() - bottomHeight, width(), bottomHeight), bottom);
    }

   private:
    /** Scales to fill the window and centres the overflow, whatever the aspect. */
    void drawCover(QPainter& painter, const QPixmap& pixmap) const
    {
        const QSize scaled = pixmap.size().scaled(size(), Qt::KeepAspectRatioByExpanding);
        const QRect target(QPoint((width() - scaled.width()) / 2, (height() - scaled.height()) / 2), scaled);
        painter.drawPixmap(target, pixmap);
    }

    QPixmap m_image;
    QPixmap m_outgoing;
    QVariantAnimation* m_fade = nullptr;
    double m_fadeProgress = 1.0;
};

HomeWindow::HomeWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("United Rails"));
    buildUi();

    m_packs = new PackSource(this);
    connect(m_packs, &PackSource::refreshed, this, &HomeWindow::onPackRefreshed);
    connect(m_packs, &PackSource::refreshFailed, this, &HomeWindow::onPackRefreshFailed);

    refreshAccounts();
    connect(APPLICATION->accounts(), &AccountList::listChanged, this, &HomeWindow::refreshAccounts);

    // Fixes installs made before this, without needing a reinstall.
    applyLauncherIcon(packInstance());

    // Something to look at from the first frame. The server screenshots take a
    // second or two to arrive, and starting on black looks broken.
    m_backdrop->setImage(QPixmap(QStringLiteral(":/default-backdrop.jpg")));

    m_launcherUpdate = new LauncherUpdate(this);
    connect(m_launcherUpdate, &LauncherUpdate::updateAvailable, this, &HomeWindow::onLauncherUpdateAvailable);
    m_launcherUpdate->check();

    refreshPlaytime();

    m_shotTimer = new QTimer(this);
    m_shotTimer->setInterval(kScreenshotIntervalMs);
    connect(m_shotTimer, &QTimer::timeout, this, &HomeWindow::showNextScreenshot);

    m_status->setText(tr("Checking for updates…"));
    m_packs->refresh();
}

void HomeWindow::buildUi()
{
    m_backdrop = new Backdrop(this);

    // The whole window sits on a dark image, so the palette is pinned to a
    // light-on-dark scheme rather than following the system theme.
    // Overrides the app-wide style for this subtree only: everything here sits
    // on the screenshot, so opaque widget backgrounds would hide it.
    m_backdrop->setStyleSheet(QStringLiteral(R"(
        QWidget { background: transparent; }
        QLabel { color: #e8e6e0; background: transparent; }
        QLabel#title { color: #f0a500; }
        QTextBrowser {
            color: #e8e6e0;
            background: rgba(17, 17, 19, 190);
            border: 1px solid rgba(240, 165, 0, 60);
            border-radius: 8px;
            padding: 8px;
        }
        QComboBox {
            color: #e8e6e0;
            background: rgba(26, 26, 30, 220);
            border: 1px solid #3a3a42;
            border-radius: 6px;
            padding: 6px 10px;
        }
        QComboBox QAbstractItemView {
            color: #e8e6e0;
            background: #1a1a1e;
            selection-background-color: #a06e00;
        }
        /* QToolButton is styled alongside QPushButton so the gear matches the
           account box and Add account button exactly. */
        QPushButton, QToolButton {
            color: #e8e6e0;
            background: rgba(26, 26, 30, 220);
            border: 1px solid #3a3a42;
            border-radius: 6px;
            padding: 6px 14px;
            min-height: 20px;
        }
        QPushButton:hover, QToolButton:hover { border-color: #f0a500; }
        /* No dropdown triangle — the gear is the whole affordance. */
        QToolButton::menu-indicator { image: none; width: 0; }
        QPushButton#play {
            color: #1a1200;
            background: #f0a500;
            border: none;
            border-radius: 8px;
        }
        QPushButton#play:hover { background: #ffb81a; }
        QPushButton#play:disabled { color: #6a6a6a; background: #2a2a30; }
        QProgressBar {
            color: #e8e6e0;
            background: rgba(26, 26, 30, 220);
            border: 1px solid #3a3a42;
            border-radius: 6px;
            text-align: center;
        }
        QProgressBar::chunk { background: #f0a500; border-radius: 5px; }
        /* Launcher update banner */
        QWidget#updateBar {
            background: rgba(240, 165, 0, 28);
            border: 1px solid #a06e00;
            border-radius: 8px;
        }
    )"));

    auto* outer = new QVBoxLayout(m_backdrop);
    outer->setContentsMargins(24, 20, 24, 20);
    outer->setSpacing(12);

    // --- header -----------------------------------------------------------
    auto* header = new QHBoxLayout();
    // Tiled mark, downscaled from the 128px render rather than the SVG, so the
    // embedded artwork is resampled once instead of twice.
    auto* icon = new QLabel(m_backdrop);
    icon->setPixmap(QPixmap(QStringLiteral(":/icon-128.png"))
                        .scaledToHeight(kHeaderLogoHeight, Qt::SmoothTransformation));
    header->addWidget(icon);

    auto* title = new QLabel(QStringLiteral("UNITED RAILS"), m_backdrop);
    title->setObjectName(QStringLiteral("title"));
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 10);
    titleFont.setBold(true);
    title->setFont(titleFont);
    header->addWidget(title);

    header->addStretch(1);

    m_playtime = new QLabel(m_backdrop);
    m_playtime->setAlignment(Qt::AlignRight);
    header->addWidget(m_playtime);

    m_headerVersion = new QLabel(m_backdrop);
    m_headerVersion->setAlignment(Qt::AlignRight);
    header->addWidget(m_headerVersion);
    outer->addLayout(header);

    // --- launcher update banner (hidden until there is one) ---------------
    m_updateBar = new QWidget(m_backdrop);
    m_updateBar->setObjectName(QStringLiteral("updateBar"));
    auto* updateLayout = new QHBoxLayout(m_updateBar);
    updateLayout->setContentsMargins(12, 8, 12, 8);
    m_updateText = new QLabel(m_updateBar);
    updateLayout->addWidget(m_updateText, 1);

    auto* updateNow = new QPushButton(tr("Update now"), m_updateBar);
    connect(updateNow, &QPushButton::clicked, this, [this]() {
        m_launcherUpdate->downloadAndRun(m_pendingRelease, this);
    });
    updateLayout->addWidget(updateNow);

    auto* updateLater = new QPushButton(tr("Later"), m_updateBar);
    connect(updateLater, &QPushButton::clicked, this, [this]() { m_updateBar->setVisible(false); });
    updateLayout->addWidget(updateLater);

    m_updateBar->setVisible(false);
    outer->addWidget(m_updateBar);

    // Empty space in the middle is deliberate — it is where the screenshot shows.
    outer->addStretch(1);

    m_shotCaption = new QLabel(m_backdrop);
    m_shotCaption->setAlignment(Qt::AlignRight);
    outer->addWidget(m_shotCaption);

    // --- changelog --------------------------------------------------------
    m_changelog = new QTextBrowser(m_backdrop);
    m_changelog->setOpenExternalLinks(true);
    m_changelog->setMaximumHeight(150);
    m_changelog->setFrameShape(QFrame::NoFrame);
    m_changelog->setVisible(false);  // shown only once a changelog arrives
    outer->addWidget(m_changelog);

    // --- progress ---------------------------------------------------------
    m_progress = new QProgressBar(m_backdrop);
    m_progress->setRange(0, 0);
    m_progress->setVisible(false);
    outer->addWidget(m_progress);

    // --- bottom bar -------------------------------------------------------
    auto* bottom = new QHBoxLayout();
    bottom->setSpacing(10);

    m_accounts = new QComboBox(m_backdrop);
    m_accounts->setMinimumWidth(200);
    bottom->addWidget(m_accounts);

    m_addAccount = new QPushButton(tr("Add account"), m_backdrop);
    connect(m_addAccount, &QPushButton::clicked, this, &HomeWindow::onAddAccount);
    bottom->addWidget(m_addAccount);

    // Everything Prism already does well — mods, packs, logs, memory — reached
    // through one menu instead of a whole instance-management UI.
    m_gear = new QToolButton(m_backdrop);
    m_gear->setIcon(QIcon(QStringLiteral(":/gear.svg")));
    m_gear->setIconSize(QSize(18, 18));
    m_gear->setToolTip(tr("Mods, packs, log and memory"));
    m_gear->setPopupMode(QToolButton::InstantPopup);

    auto* gearMenu = new QMenu(m_gear);
    gearMenu->addAction(tr("Mods…"), this, [this]() { openInstancePage(QStringLiteral("mods")); });
    gearMenu->addAction(tr("Resource packs…"), this, [this]() { openInstancePage(QStringLiteral("resourcepacks")); });
    gearMenu->addAction(tr("Shader packs…"), this, [this]() { openInstancePage(QStringLiteral("shaderpacks")); });
    gearMenu->addSeparator();
    gearMenu->addAction(tr("Memory && Java…"), this, [this]() { openInstancePage(QStringLiteral("settings")); });
    gearMenu->addAction(tr("View log…"), this, [this]() { openInstancePage(QStringLiteral("console")); });
    gearMenu->addSeparator();
    gearMenu->addAction(tr("Sign out of this account"), this, &HomeWindow::onRemoveAccount);
    gearMenu->addAction(tr("Open game folder"), this, [this]() {
        if (auto* instance = packInstance()) {
            DesktopServices::openPath(instance->gameRoot(), true);
        }
    });
    gearMenu->addSeparator();
    // The licence and credits live in this dialog. Upstream reached it from the
    // main window, which this fork does not show, so it needs a way in here.
    gearMenu->addAction(tr("About && licence"), this, [this]() {
        AboutDialog dialog(this);
        dialog.exec();
    });
    m_gear->setMenu(gearMenu);
    bottom->addWidget(m_gear);

    m_status = new QLabel(m_backdrop);
    bottom->addWidget(m_status, 1);

    m_primary = new QPushButton(tr("Play"), m_backdrop);
    m_primary->setObjectName(QStringLiteral("play"));
    m_primary->setMinimumSize(180, 48);
    QFont playFont = m_primary->font();
    playFont.setPointSize(playFont.pointSize() + 4);
    playFont.setBold(true);
    m_primary->setFont(playFont);
    m_primary->setEnabled(false);
    connect(m_primary, &QPushButton::clicked, this, &HomeWindow::onPrimaryClicked);
    bottom->addWidget(m_primary);

    outer->addLayout(bottom);

    setCentralWidget(m_backdrop);
    resize(1000, 640);
}

void HomeWindow::closeEvent(QCloseEvent* event)
{
    emit isClosing();
    QMainWindow::closeEvent(event);
}

/* ------------------------------------------------------------------ state --- */

MinecraftInstance* HomeWindow::packInstance() const
{
    const auto storedId = APPLICATION->settings()->get(kSettingInstanceId).toString();
    if (!storedId.isEmpty()) {
        if (auto* inst = APPLICATION->instances()->getInstanceById(storedId)) {
            return inst;
        }
    }

    // The stored id can go stale if the instance was removed by hand. This
    // launcher only ever manages one, so fall back to whatever is there.
    if (APPLICATION->instances()->count() == 1) {
        return APPLICATION->instances()->at(0);
    }
    return nullptr;
}

QString HomeWindow::installedVersion() const
{
    const auto recorded = APPLICATION->settings()->get(kSettingPackVersion).toString();
    if (!recorded.isEmpty()) {
        return recorded;
    }

    // Nothing recorded: fall back to what the instance itself says it is. This
    // recovers installs made before the setting existed, so they don't get
    // told to re-download a version they already have.
    if (auto* instance = packInstance()) {
        const auto fromInstance = instance->getManagedPackVersionID();
        if (!fromInstance.isEmpty()) {
            APPLICATION->settings()->set(kSettingPackVersion, fromInstance);
            return fromInstance;
        }
    }
    return {};
}

void HomeWindow::refreshAccounts()
{
    const QString previous = m_accounts->currentData().toString();
    m_accounts->clear();

    auto* accounts = APPLICATION->accounts();
    for (int i = 0; i < accounts->count(); ++i) {
        auto account = accounts->at(i);
        if (!account) {
            continue;
        }
        m_accounts->addItem(account->profileName(), account->profileName());
    }

    if (m_accounts->count() == 0) {
        m_accounts->addItem(tr("No account — click Add account"), QString());
    } else if (!previous.isEmpty()) {
        const int index = m_accounts->findData(previous);
        if (index >= 0) {
            m_accounts->setCurrentIndex(index);
        }
    }

    updatePrimaryButton();
}

void HomeWindow::updatePrimaryButton()
{
    if (m_busy) {
        m_primary->setEnabled(false);
        return;
    }

    const auto& info = m_packs ? m_packs->info() : PackInfo{};
    if (!info.valid) {
        m_primary->setText(tr("Play"));
        m_primary->setEnabled(false);
        return;
    }

    const bool haveAccount = !m_accounts->currentData().toString().isEmpty();
    m_primary->setEnabled(haveAccount);
    if (!haveAccount) {
        m_status->setText(tr("Add an account to play."));
    }

    if (!packInstance()) {
        m_primary->setText(tr("Install"));
        m_status->setText(haveAccount ? tr("Neo Rails %1 is ready to install.").arg(info.version) : m_status->text());
    } else if (installedVersion() != info.version) {
        m_primary->setText(tr("Update to %1").arg(info.version));
        m_status->setText(haveAccount ? tr("You have %1 installed.").arg(installedVersion().isEmpty() ? tr("an older version")
                                                                                                      : installedVersion())
                                      : m_status->text());
    } else {
        m_primary->setText(tr("Play"));
        if (haveAccount) {
            m_status->setText(tr("Neo Rails %1 — up to date.").arg(info.version));
        }
    }
}

void HomeWindow::setBusy(bool busy, const QString& what)
{
    m_busy = busy;
    m_progress->setVisible(busy);
    m_accounts->setEnabled(!busy);
    m_addAccount->setEnabled(!busy);
    if (busy && !what.isEmpty()) {
        m_status->setText(what);
    }
    updatePrimaryButton();
}

/* ------------------------------------------------------------------- pack --- */

void HomeWindow::onPackRefreshed()
{
    const auto& info = m_packs->info();
    m_headerVersion->setText(tr("Latest: %1").arg(info.version));

    // An empty changelog panel is just a box covering the screenshot, so it
    // only appears when there is something to read.
    const bool haveChangelog = !info.changelog.trimmed().isEmpty();
    m_changelog->setVisible(haveChangelog);
    if (haveChangelog) {
        m_changelog->setMarkdown(hardWrapLines(info.changelog));
    }

    loadScreenshots();
    updatePrimaryButton();
}

void HomeWindow::onPackRefreshFailed(const QString& reason)
{
    m_headerVersion->setText(tr("Offline"));
    m_changelog->setPlainText(tr("Could not reach united-rails.net.\n\n%1").arg(reason));

    // An installed pack is still playable with no network, so only block the
    // button when there is nothing installed to play.
    if (packInstance() && !m_accounts->currentData().toString().isEmpty()) {
        m_primary->setText(tr("Play"));
        m_primary->setEnabled(true);
        m_status->setText(tr("Offline — playing the installed version."));
    } else {
        m_primary->setEnabled(false);
        m_status->setText(tr("Could not check for updates."));
    }
}

void HomeWindow::loadScreenshots()
{
    m_shotTimer->stop();
    m_shotPixmaps.clear();
    m_shotCaptions.clear();
    m_shotIndex = -1;

    const auto shots = m_packs->info().screenshots;
    if (shots.isEmpty()) {
        // Keep the bundled backdrop rather than dropping to black.
        m_backdrop->setImage(QPixmap(QStringLiteral(":/default-backdrop.jpg")));
        m_shotCaption->clear();
        return;
    }

    for (const auto& shot : shots) {
        QNetworkRequest request{ shot.url };
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        auto* reply = APPLICATION->network()->get(request);
        const QString caption = shot.caption;

        connect(reply, &QNetworkReply::finished, this, [this, reply, caption]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                return;  // a screenshot that will not load is not worth an error dialog
            }
            QPixmap pixmap;
            if (!pixmap.loadFromData(reply->readAll())) {
                return;
            }
            // Kept at full size; the backdrop scales to cover whatever the window is.
            m_shotPixmaps.append(pixmap);
            m_shotCaptions.append(caption);

            if (m_shotIndex < 0) {
                showScreenshot(0);
                if (m_shotPixmaps.size() > 1) {
                    m_shotTimer->start();
                }
            } else if (m_shotPixmaps.size() > 1 && !m_shotTimer->isActive()) {
                m_shotTimer->start();
            }
        });
    }
}

void HomeWindow::showScreenshot(int index)
{
    if (m_shotPixmaps.isEmpty()) {
        return;
    }
    m_shotIndex = index % m_shotPixmaps.size();
    m_backdrop->setImage(m_shotPixmaps.at(m_shotIndex));
    m_shotCaption->setText(m_shotCaptions.value(m_shotIndex));
}

void HomeWindow::showNextScreenshot()
{
    if (m_shotPixmaps.size() > 1) {
        showScreenshot(m_shotIndex + 1);
    }
}

/* --------------------------------------------------------------- actions --- */

void HomeWindow::applyLauncherIcon(MinecraftInstance* instance)
{
    if (!instance) {
        return;
    }
    const QString key = QString::fromUtf8(kIconKey);

    // The pack ships its own icon and the import applies it. Replace it so the
    // console window and everything else show the launcher mark instead.
    if (!APPLICATION->icons()->iconFileExists(key)) {
        APPLICATION->icons()->addIcon(key, QStringLiteral("United Rails"),
                                      QStringLiteral(":/net.unitedrails.Launcher.svg"), IconType::Builtin);
    }
    if (instance->iconKey() != key) {
        instance->setIconKey(key);
    }
}

void HomeWindow::refreshPlaytime()
{
    auto* instance = packInstance();
    if (!instance) {
        m_playtime->clear();
        return;
    }

    const int64_t seconds = instance->totalTimePlayed();
    if (seconds <= 0) {
        m_playtime->clear();
        return;
    }

    m_playtime->setText(tr("%1 played").arg(
        Time::prettifyDuration(seconds, APPLICATION->settings()->get("ShowGameTimeWithoutDays").toBool())));
}

void HomeWindow::onLauncherUpdateAvailable(const LauncherRelease& release)
{
    m_pendingRelease = release;
    m_updateText->setText(tr("Launcher %1 is available — you have %2.")
                              .arg(release.version, LauncherUpdate::currentVersion()));
    m_updateBar->setVisible(true);
}

void HomeWindow::openInstancePage(const QString& pageId)
{
    auto* instance = packInstance();
    if (!instance) {
        CustomMessageBox::selectable(this, tr("Nothing installed yet"),
                                     tr("Install the modpack first — then you can manage mods, packs and settings here."),
                                     QMessageBox::Information)
            ->show();
        return;
    }
    APPLICATION->showInstanceWindow(instance, pageId);
}

void HomeWindow::onAddAccount()
{
    if (auto account = MSALoginDialog::newAccount(this)) {
        APPLICATION->accounts()->addAccount(account);
        refreshAccounts();
        const int index = m_accounts->findData(account->profileName());
        if (index >= 0) {
            m_accounts->setCurrentIndex(index);
        }
        updatePrimaryButton();
    }
}

void HomeWindow::onRemoveAccount()
{
    const QString profile = m_accounts->currentData().toString();
    if (profile.isEmpty()) {
        return;
    }

    auto* accounts = APPLICATION->accounts();
    // Find the row by profile name; the combo index is not the account index
    // once accounts have been added and removed.
    for (int i = 0; i < accounts->count(); ++i) {
        auto account = accounts->at(i);
        if (!account || account->profileName() != profile) {
            continue;
        }

        auto* confirm = CustomMessageBox::selectable(this, tr("Sign out"), tr("Sign out of %1?").arg(profile),
                                                     QMessageBox::Question, QMessageBox::Yes | QMessageBox::No);
        if (confirm->exec() != QMessageBox::Yes) {
            return;
        }

        accounts->removeAccount(accounts->index(i));
        refreshAccounts();
        return;
    }
}

void HomeWindow::onPrimaryClicked()
{
    const auto& info = m_packs->info();
    const bool needsInstall = !packInstance();
    const bool needsUpdate = info.valid && !needsInstall && installedVersion() != info.version;

    if (needsInstall || needsUpdate) {
        if (!installOrUpdatePack()) {
            return;
        }
        // Installing is the action the user asked for; let them press Play
        // themselves rather than launching a 78 MB download straight into the game.
        updatePrimaryButton();
        return;
    }

    launchPack();
}

bool HomeWindow::installOrUpdatePack()
{
    const auto& info = m_packs->info();
    if (!info.valid) {
        return false;
    }

    auto* existing = packInstance();

    QMap<QString, QString> extraInfo;
    extraInfo.insert("pack_id", QStringLiteral("neo-rails"));
    extraInfo.insert("pack_version_id", info.version);
    if (existing) {
        // This is what makes InstanceImportTask update in place instead of
        // creating a second instance — saves and options survive.
        extraInfo.insert("original_instance_id", existing->id());
    }

    auto* importTask = new InstanceImportTask(info.url, true, this, std::move(extraInfo));
    importTask->setName(QString::fromUtf8(kInstanceName));
    importTask->setConfirmUpdate(false);
    if (existing) {
        importTask->setGroup(APPLICATION->instances()->getInstanceGroup(existing->id()));
        importTask->setIcon(existing->iconKey());
    }

    const unique_qobject_ptr<Task> wrapped(APPLICATION->instances()->wrapInstanceTask(importTask));

    setBusy(true, existing ? tr("Updating to %1…").arg(info.version) : tr("Installing Neo Rails %1…").arg(info.version));

    ProgressDialog dialog(this);
    dialog.setSkipButton(true, tr("Abort"));
    dialog.execWithTask(wrapped.get());

    setBusy(false);

    if (!wrapped->wasSuccessful()) {
        CustomMessageBox::selectable(this, tr("Install failed"),
                                     tr("Could not install the modpack:\n\n%1").arg(wrapped->failReason()), QMessageBox::Critical)
            ->show();
        return false;
    }

    // Record what we now have. The instance id is only known after the task
    // committed it, so re-resolve rather than trusting the old value.
    if (auto* installed = packInstance()) {
        APPLICATION->settings()->set(kSettingInstanceId, installed->id());

        applyLauncherIcon(installed);
    }
    APPLICATION->settings()->set(kSettingPackVersion, info.version);

    m_status->setText(tr("Neo Rails %1 installed.").arg(info.version));
    return true;
}

void HomeWindow::launchPack()
{
    auto* instance = packInstance();
    if (!instance) {
        return;
    }

    const QString profile = m_accounts->currentData().toString();
    if (profile.isEmpty()) {
        return;
    }

    auto account = APPLICATION->accounts()->getAccountByProfileName(profile);
    if (!account) {
        CustomMessageBox::selectable(this, tr("Account problem"), tr("That account could not be used. Try adding it again."),
                                     QMessageBox::Warning)
            ->show();
        return;
    }

    // Playtime is written when the session ends, so refresh the label then.
    // UniqueConnection because this runs on every launch.
    connect(instance, &BaseInstance::runningStatusChanged, this, &HomeWindow::refreshPlaytime, Qt::UniqueConnection);

    APPLICATION->launch(instance, LaunchMode::Normal, nullptr, account);
}

}  // namespace UnitedRails
