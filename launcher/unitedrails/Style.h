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

class QApplication;
class QIcon;

namespace UnitedRails {

/** Site palette, so widget code does not repeat hex values. */
namespace Colors {
constexpr const char* kGold = "#f0a500";
constexpr const char* kGoldHover = "#ffb81a";
constexpr const char* kGoldDim = "#a06e00";
constexpr const char* kInk = "#0a0a0b";
constexpr const char* kSurface = "#111113";
constexpr const char* kSurfaceRaised = "#1a1a1e";
constexpr const char* kLine = "#26262b";
constexpr const char* kText = "#e8e6e0";
constexpr const char* kMuted = "#888680";
}  // namespace Colors

/**
 * Applies the United Rails look to the whole application — every dialog,
 * message box and progress window, not just the home screen.
 */
void applyStyle(QApplication& app);

/**
 * The launcher icon, carrying every rendered size.
 *
 * Built from per-size PNGs rather than the SVG: the artwork inside that SVG is
 * a raster, so letting Qt scale it down to 16px for a title bar turns it to
 * mush. With all sizes present Qt picks the one that needs no scaling.
 */
QIcon launcherIcon();

}  // namespace UnitedRails
