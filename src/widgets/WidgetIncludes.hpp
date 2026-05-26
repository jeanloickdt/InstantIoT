#pragma once

/*************************************************************
 * ⚡ InstantIoT Library v1.1.0
 *
 * WidgetIncludes.hpp - Conditional widget inclusion
 *
 * Widgets are included based on #defines:
 *   #define INSTANTIOT_WIDGETS_LED
 *   #define INSTANTIOT_WIDGETS_GAUGE
 *   etc.
 *
 * If no #define, all widgets are included.
 *
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include "../InstantIoTConfig.h"

// ============================================================
// 📊 DISPLAYS (Device → App)
// ============================================================

#ifdef INSTANTIOT_WIDGETS_LED
    #include "displays/Led.hpp"
#endif

#ifdef INSTANTIOT_WIDGETS_GAUGE
    #include "displays/Gauge.hpp"
#endif

#ifdef INSTANTIOT_WIDGETS_METRIC
    #include "displays/Metric.hpp"
#endif

#ifdef INSTANTIOT_WIDGETS_HORIZONTALLEVEL
    #include "displays/HorizontalLevel.hpp"
#endif

#ifdef INSTANTIOT_WIDGETS_VERTICALLEVEL
    #include "displays/VerticalLevel.hpp"
#endif

#ifdef INSTANTIOT_WIDGETS_ADVANCEDCHART
    #include "displays/AdvancedChart.hpp"
#endif

#ifdef INSTANTIOT_WIDGETS_BARCHART
    #include "displays/BarChart.hpp"
#endif


#ifdef INSTANTIOT_WIDGETS_TEXT
    #include "displays/Text.hpp"
#endif

// ============================================================
// 🎮 CONTROLS (App → Device)
// No class — handled by Registry.hpp + DSL macros
// ============================================================

#include "../utils/InstantIoTMacros.hpp"
#include "../utils/InstantIoTWhen.hpp"