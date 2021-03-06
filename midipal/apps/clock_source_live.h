// Copyright 2011 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------
//
// MIDI clock generator app.

#ifndef MIDIPAL_APPS_CLOCK_SOURCE_LIVE_H_
#define MIDIPAL_APPS_CLOCK_SOURCE_LIVE_H_

#include "midipal/app.h"

namespace midipal {
namespace apps{

class ClockSourceLive {
 public:
  enum Parameter : uint8_t {
    running_,
    bpm_,
    multiplier_,
    divider_,
    send_start_,
    control_note_,
    tap_bpm_,
    COUNT
  };

  static uint8_t settings[Parameter::COUNT];
  static const uint8_t factory_data[Parameter::COUNT] PROGMEM;
  static const AppInfo app_info_ PROGMEM;

  static void OnInit();
  static void OnStart();
  static void OnStop();
  static void OnContinue();
  static void OnRawByte(uint8_t byte);
  static void OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  static void SetParameter(uint8_t key, uint8_t value);
  static uint8_t OnClick();
  static uint8_t OnIncrement(int8_t increment);

  static void OnClock(uint8_t clock_source);
  

 private:
  static void TapTempo();
  static void Stop();
  static void Start();

  static uint8_t& ParameterValue(Parameter key) {
    return settings[key];
  }
  static inline uint8_t& running() {
    return ParameterValue(running_);
  }
  static inline uint8_t& bpm() {
    return ParameterValue(bpm_);
  }
  static inline uint8_t& multiplier() {
    return ParameterValue(multiplier_);
  }
  static inline uint8_t& divider() {
    return ParameterValue(divider_);
  }
  static inline uint8_t& send_start() {
    return ParameterValue(send_start_);
  }
  static inline uint8_t& control_note() {
    return ParameterValue(control_note_);
  }
  static inline uint8_t& tap_bpm() {
    return ParameterValue(tap_bpm_);
  }

  static uint8_t num_taps_;
  static uint8_t num_ticks_;
  static uint32_t elapsed_time_;
  
  DISALLOW_COPY_AND_ASSIGN(ClockSourceLive);
};

} // namespace apps
} // namespace midipal

#endif // MIDIPAL_APPS_CLOCK_SOURCE_LIVE_H_
