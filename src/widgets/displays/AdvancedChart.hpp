#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../InstantIoTConfig.h"

namespace InstantIoT {

class AdvancedChartWidget : public DisplayWidget {
private:
    int _pointIndex = 0;  // Auto-increment X

public:
    AdvancedChartWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender), _pointIndex(0) {}
    
    const char* getType() const override { return "advancedchart"; }
    
    // Ajouter un point avec seriesId (X auto-incrémenté)
    AdvancedChartWidget& addPoint(const char* seriesId, float y) {
        char payload[96];
        snprintf(payload, sizeof(payload), 
            "{\"seriesId\":\"%s\",\"x\":%d,\"y\":%.2f}", 
            seriesId, _pointIndex++, y);
        sendMessage("adddatapoint", payload);
        return *this;
    }
    
    // Ajouter un point avec seriesId et X explicite
    AdvancedChartWidget& addPoint(const char* seriesId, float x, float y) {
        char payload[96];
        snprintf(payload, sizeof(payload), 
            "{\"seriesId\":\"%s\",\"x\":%.2f,\"y\":%.2f}", 
            seriesId, x, y);
        sendMessage("adddatapoint", payload);
        return *this;
    }
    
    // Ajouter un point sur la série "default"
    AdvancedChartWidget& addPoint(float y) {
        return addPoint("default", y);
    }
    
    // Clear une série
    AdvancedChartWidget& clearSeries(const char* seriesId) {
        char payload[48];
        snprintf(payload, sizeof(payload), "{\"seriesId\":\"%s\"}", seriesId);
        sendMessage("clearseriesdata", payload);
        return *this;
    }
    
    // Clear toutes les séries
    AdvancedChartWidget& clear() {
        _pointIndex = 0;
        sendMessage("clearalldata");
        return *this;
    }
    
    // Reset l'index X
    AdvancedChartWidget& resetIndex() {
        _pointIndex = 0;
        return *this;
    }
    
    // Ajouter une série
    AdvancedChartWidget& addSeries(const char* seriesId, const char* label, const char* color = nullptr) {
        char payload[128];
        if (color) {
            snprintf(payload, sizeof(payload), 
                "{\"seriesId\":\"%s\",\"label\":\"%s\",\"lineColor\":\"%s\"}", 
                seriesId, label, color);
        } else {
            snprintf(payload, sizeof(payload), 
                "{\"seriesId\":\"%s\",\"label\":\"%s\"}", 
                seriesId, label);
        }
        sendMessage("addseries", payload);
        return *this;
    }
    
    // Configurer le style d'une série
    AdvancedChartWidget& setSeriesStyle(const char* seriesId, const char* lineColor, bool showPoints = false, bool showFill = false) {
        char payload[128];
        snprintf(payload, sizeof(payload), 
            "{\"seriesId\":\"%s\",\"lineColor\":\"%s\",\"showPoints\":%s,\"showFill\":%s}", 
            seriesId, lineColor, 
            showPoints ? "true" : "false",
            showFill ? "true" : "false");
        sendMessage("updateseriesstyle", payload);
        return *this;
    }
    
    // Configurer le nombre max de points (buffer oscilloscope)
    AdvancedChartWidget& setMaxPoints(int maxPoints) {
        char payload[32];
        snprintf(payload, sizeof(payload), "{\"maxDataPoints\":%d}", maxPoints);
        sendMessage("setmaxdatapoints", payload);
        return *this;
    }
    
    // Configurer les labels
    AdvancedChartWidget& setLabels(const char* title, const char* xLabel, const char* yLabel) {
        char payload[128];
        snprintf(payload, sizeof(payload), 
            "{\"chartTitle\":\"%s\",\"xAxisLabel\":\"%s\",\"yAxisLabel\":\"%s\"}", 
            title, xLabel, yLabel);
        sendMessage("updateaxislabels", payload);
        return *this;
    }
    
    // Configurer le range Y
    AdvancedChartWidget& setYRange(float yMin, float yMax) {
        char payload[64];
        snprintf(payload, sizeof(payload), "{\"yMin\":%.2f,\"yMax\":%.2f,\"autoScale\":false}", yMin, yMax);
        sendMessage("updaterange", payload);
        return *this;
    }
    
    // Auto-scale
    AdvancedChartWidget& setAutoScale(bool enabled) {
        char payload[32];
        snprintf(payload, sizeof(payload), "{\"autoScale\":%s}", enabled ? "true" : "false");
        sendMessage("updaterange", payload);
        return *this;
    }
};

} // namespace InstantIoT
