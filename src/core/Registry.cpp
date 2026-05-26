/*************************************************************
 * ⚡ InstantIoT Library v1.2.1
 * 
 * Registry.cpp - Default callback implementations
 *
 * These functions are weak and will be replaced if the user
 * defines them in their sketch.
 * 
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include "Registry.hpp"

// Default implementations (empty)
// The user can override them by defining their own version

void onSimpleButtonEvent(const SimpleButtonEvent& e) { (void)e; }
void onAdvancedButtonEvent(const AdvancedButtonEvent& e) { (void)e; }
void onEmergencyButtonEvent(const EmergencyButtonEvent& e) { (void)e; }
void onHorizontalSliderEvent(const HorizontalSliderEvent& e) { (void)e; }
void onVerticalSliderEvent(const VerticalSliderEvent& e) { (void)e; }
void onSwitchEvent(const SwitchEvent& e) { (void)e; }
void onJoystickEvent(const JoystickEvent& e) { (void)e; }
void onDirectionPadEvent(const DirectionPadEvent& e) { (void)e; }
void onSegmentedSwitchEvent(const SegmentedSwitchEvent& e) { (void)e; }
