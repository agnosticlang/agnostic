#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
set -eu

SRC_DIR="$1"
BUILD_DIR="$2"
shift 2

cmake -S "$SRC_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build "$BUILD_DIR" -j"$(nproc)"
