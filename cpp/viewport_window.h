#pragma once

/**
 * @file Renders the color-mapped simulation result as an image
 */

// C++ standard libraries
#include <algorithm>
#include <chrono>

// third-party includes
#include "opengl.h"
#include <imgui.h>

// TIRA includes
#include <tira/grid/color_image.h>

// loadbalance includes
#include "imgui_window.h"
#include "run_simulation.h"

/**
 * Displays the Mie cross-section. The simulation is re-run on every frame, so
 * the frame rate of this window IS the throughput of tira::optics::Mie::Sum --
 * which is the number the load balancing work is meant to move.
 */
class ViewportWindow : public ImguiWindow {};
