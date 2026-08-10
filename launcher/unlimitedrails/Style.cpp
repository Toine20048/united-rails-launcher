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

#include "unlimitedrails/Style.h"

#include <QApplication>
#include <QIcon>
#include <QPalette>
#include <QStyleFactory>

namespace UnlimitedRails {

QIcon launcherIcon()
{
    static const QIcon icon = []() {
        QIcon built;
        for (int size : { 16, 24, 32, 48, 64, 128, 256 }) {
            built.addFile(QStringLiteral(":/icon-%1.png").arg(size), QSize(size, size));
        }
        return built;
    }();
    return icon;
}

void applyStyle(QApplication& app)
{
    // Fusion draws every control itself, so the same look holds regardless of
    // the Windows version underneath.
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Every window and dialog gets our mark. Without this they inherit
    // Prism's logo from the icon theme.
    app.setWindowIcon(launcherIcon());

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(Colors::kInk));
    palette.setColor(QPalette::WindowText, QColor(Colors::kText));
    palette.setColor(QPalette::Base, QColor(Colors::kSurface));
    palette.setColor(QPalette::AlternateBase, QColor(Colors::kSurfaceRaised));
    palette.setColor(QPalette::Text, QColor(Colors::kText));
    palette.setColor(QPalette::Button, QColor(Colors::kSurfaceRaised));
    palette.setColor(QPalette::ButtonText, QColor(Colors::kText));
    palette.setColor(QPalette::Highlight, QColor(Colors::kGoldDim));
    palette.setColor(QPalette::HighlightedText, QColor(Colors::kText));
    palette.setColor(QPalette::ToolTipBase, QColor(Colors::kSurfaceRaised));
    palette.setColor(QPalette::ToolTipText, QColor(Colors::kText));
    palette.setColor(QPalette::Link, QColor(Colors::kGold));
    palette.setColor(QPalette::PlaceholderText, QColor(Colors::kMuted));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(Colors::kMuted));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(Colors::kMuted));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(Colors::kMuted));
    app.setPalette(palette);

    app.setStyleSheet(QStringLiteral(R"(
        QWidget { background: #0a0a0b; color: #e8e6e0; }
        QDialog, QMainWindow { background: #0a0a0b; }

        QLineEdit, QPlainTextEdit, QTextEdit, QTextBrowser, QSpinBox, QAbstractSpinBox {
            background: #111113;
            border: 1px solid #26262b;
            border-radius: 6px;
            padding: 5px 8px;
            selection-background-color: #a06e00;
        }
        QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QAbstractSpinBox:focus {
            border-color: #f0a500;
        }

        QPushButton {
            background: #1a1a1e;
            border: 1px solid #3a3a42;
            border-radius: 6px;
            padding: 6px 16px;
            min-height: 20px;
        }
        QPushButton:hover { border-color: #f0a500; }
        QPushButton:pressed { background: #26262b; }
        QPushButton:default { border-color: #a06e00; }
        QPushButton:disabled { color: #6a6a6a; background: #141418; border-color: #26262b; }

        QComboBox {
            background: #1a1a1e;
            border: 1px solid #3a3a42;
            border-radius: 6px;
            padding: 5px 10px;
        }
        QComboBox:hover { border-color: #f0a500; }
        QComboBox QAbstractItemView {
            background: #1a1a1e;
            border: 1px solid #26262b;
            selection-background-color: #a06e00;
        }

        QProgressBar {
            background: #1a1a1e;
            border: 1px solid #3a3a42;
            border-radius: 6px;
            text-align: center;
            min-height: 18px;
        }
        QProgressBar::chunk { background: #f0a500; border-radius: 5px; }

        QMenu, QMenuBar { background: #111113; }
        QMenu::item:selected, QMenuBar::item:selected { background: #a06e00; }

        QTabWidget::pane { border: 1px solid #26262b; border-radius: 6px; }
        QTabBar::tab { background: #111113; padding: 7px 14px; border: 1px solid #26262b; }
        QTabBar::tab:selected { background: #1a1a1e; color: #f0a500; border-bottom-color: #f0a500; }

        QScrollBar:vertical { background: #0a0a0b; width: 11px; margin: 0; }
        QScrollBar:horizontal { background: #0a0a0b; height: 11px; margin: 0; }
        QScrollBar::handle { background: #3a3a42; border-radius: 5px; min-height: 28px; }
        QScrollBar::handle:hover { background: #a06e00; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
        QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

        QToolTip {
            background: #1a1a1e;
            color: #e8e6e0;
            border: 1px solid #f0a500;
            padding: 4px;
        }

        QGroupBox {
            border: 1px solid #26262b;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 8px;
        }
        QGroupBox::title { color: #f0a500; subcontrol-origin: margin; left: 10px; }

        QHeaderView::section {
            background: #111113;
            border: none;
            border-bottom: 1px solid #26262b;
            padding: 5px;
        }
    )"));
}

}  // namespace UnlimitedRails
