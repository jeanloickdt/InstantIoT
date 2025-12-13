#pragma once

/*************************************************************
 * ⚡ InstantIoT Library v2.0
 * 
 * WidgetIncludes.hpp - Inclusion conditionnelle des widgets
 * 
 * Les widgets sont inclus en fonction des #define:
 *   #define INSTANTIOT_WIDGETS_LED
 *   #define INSTANTIOT_WIDGETS_GAUGE
 *   etc.
 * 
 * Si aucun #define, tous les widgets sont inclus.
 * 
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include "../InstantIoTConfig.h"

// ============================================================
// 📊 DISPLAYS (Arduino → App)
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
// 🎮 CONTROLLERS (App → Arduino)
// Note: Controllers n'ont pas de classe, juste des events
// Ils sont gérés par Registry.hpp
// ============================================================

// Les macros DSL sont dans InstantIoTMacros.hpp
#include "../utils/InstantIoTMacros.hpp"
