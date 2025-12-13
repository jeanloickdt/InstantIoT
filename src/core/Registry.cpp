/*************************************************************
 * ⚡ InstantIoT Library v2.0
 * 
 * Registry.cpp - Implémentations par défaut des callbacks
 * 
 * Ces fonctions sont weak et seront remplacées si l'utilisateur
 * les définit dans son sketch.
 * 
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include "Registry.hpp"

// Implémentations par défaut (vides)
// L'utilisateur peut les overrider en définissant sa propre version

void onSimpleButtonEvent(const SimpleButtonEvent& e) { (void)e; }
void onAdvancedButtonEvent(const AdvancedButtonEvent& e) { (void)e; }
void onEmergencyButtonEvent(const EmergencyButtonEvent& e) { (void)e; }
void onHorizontalSliderEvent(const HorizontalSliderEvent& e) { (void)e; }
void onVerticalSliderEvent(const VerticalSliderEvent& e) { (void)e; }
void onSwitchEvent(const SwitchEvent& e) { (void)e; }
void onJoystickEvent(const JoystickEvent& e) { (void)e; }
void onDirectionPadEvent(const DirectionPadEvent& e) { (void)e; }
void onSegmentedSwitchEvent(const SegmentedSwitchEvent& e) { (void)e; }
void onWidgetRequest(const WidgetRequest& e) { (void)e; }
