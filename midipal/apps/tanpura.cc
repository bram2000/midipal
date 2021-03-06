// Copyright 2012 Olivier Gillet.
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
// Tanpura app.

#include "midipal/apps/tanpura.h"

#include "midi/midi_constants.h"
#include "avrlib/op.h"
#include "avrlib/string.h"

#include "midipal/clock.h"
#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal {
namespace apps{

using namespace avrlib;

const uint8_t Tanpura::factory_data[Parameter::COUNT] PROGMEM = {
  0, 0, 120, 8, 0, 60, 0, 5
};

/* static */
uint8_t Tanpura::settings[Parameter::COUNT];

/* <static> */
uint8_t Tanpura::midi_clock_prescaler_;
uint8_t Tanpura::tick_;
uint8_t Tanpura::step_;
/* </static> */

/* static */
const AppInfo Tanpura::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  &OnNoteOn, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnNoteAftertouch)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnAftertouch)(uint8_t, uint8_t);
  nullptr, // void (*OnControlChange)(uint8_t, uint8_t, uint8_t);
  nullptr, // void (*OnProgramChange)(uint8_t, uint8_t);
  nullptr, // void (*OnPitchBend)(uint8_t, uint16_t);
  nullptr, // void (*OnSysExByte)(uint8_t);
  &OnClock, // void (*OnClock)();
  &OnStart, // void (*OnStart)();
  &OnContinue, // void (*OnContinue)();
  &OnStop, // void (*OnStop)();
  nullptr, // bool *(CheckChannel)(uint8_t);
  nullptr, // void (*OnRawByte)(uint8_t);
  &OnRawMidiData, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t);
  nullptr, // uint8_t (*OnIncrement)(int8_t);
  nullptr, // uint8_t (*OnClick)();
  nullptr, // uint8_t (*OnPot)(uint8_t, uint8_t);
  nullptr, // uint8_t (*OnRedraw)();
  &SetParameter, // void (*SetParameter)(uint8_t, uint8_t);
  nullptr, // uint8_t (*GetParameter)(uint8_t);
  nullptr, // uint8_t (*CheckPageStatus)(uint8_t);
  Parameter::COUNT, // settings_size
  SETTINGS_TANPURA, // settings_offset
  settings, // settings_data
  factory_data, // factory_data
  STR_RES_TANPURA, // app_name
  true
};

/* static */
void Tanpura::OnInit() {
  Ui::AddPage(STR_RES_RUN, STR_RES_OFF, 0, 1);
  Ui::AddPage(STR_RES_CLK, STR_RES_INT, 0, 2);
  Ui::AddPage(STR_RES_BPM, UNIT_INTEGER, 40, 240);
  Ui::AddPage(STR_RES_DIV, STR_RES_2_1, 0, 16);
  Ui::AddPage(STR_RES_CHN, UNIT_INDEX, 0, 15);
  Ui::AddPage(STR_RES_SA, UNIT_NOTE, 36, 84);
  Ui::AddPage(STR_RES_MOD, STR_RES_PA, 0, 3);
  Ui::AddPage(STR_RES_CYC, UNIT_INTEGER, 0, 7);
  Clock::Update(bpm(), 0, 0); // TODO is this necessary, given SetParameter()?
  SetParameter(bpm_, bpm());
  Clock::Start();
  running() = 0;
}

/* static */
void Tanpura::OnRawMidiData(uint8_t status, uint8_t* data, uint8_t data_size) {
  App::Send(status, data, data_size);
}

/* static */
void Tanpura::SetParameter(uint8_t key, uint8_t value) {
  auto param = static_cast<Parameter>(key);
  if (param == running_) {
    if (value) {
      Start();
    } else {
      Stop();
    }
  }
  ParameterValue(param) = value;
  if (param == bpm_) {
    Clock::Update(bpm(), 0, 0);
  }
  midi_clock_prescaler_ = ResourcesManager::Lookup<uint8_t, uint8_t>(
      midi_clock_tick_per_step, clock_division());
}

/* static */
void Tanpura::OnStart() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    Start();
  }
}

/* static */
void Tanpura::OnStop() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    Stop();
  }
}

/* static */
void Tanpura::OnContinue() {
  if (clk_mode() != CLOCK_MODE_INTERNAL) {
    running() = 1;
  }
}

/* static */
void Tanpura::OnClock(uint8_t clock_source) {
  if (clk_mode() == clock_source && running()) {
    if (clock_source == CLOCK_MODE_INTERNAL) {
      App::SendNow(0xf8);
    }
    Tick();
  }
}

/* static */
void Tanpura::OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (channel == Tanpura::channel()) {
    if (Ui::editing() && Ui::page() == 5) {
      while (note < 36) {
        note += 12;
      }
      while (note > 84) {
        note -= 12;
      }
      root() = note;
    }
  }
}

/* static */
void Tanpura::Stop() {
  if (running()) {
    // Flush the note off messages in the queue.
    App::FlushQueue(channel());
    // To be on the safe side, send an all notes off message.
    App::Send3(controlChangeFor(channel()), 123, 0);
    if (clk_mode() == CLOCK_MODE_INTERNAL) {
      App::SendNow(MIDI_SYS_CLK_STOP);
    }
    running() = 0;
  }
}

/* static */
void Tanpura::Start() {
  if (!running()) {
    if (clk_mode() == CLOCK_MODE_INTERNAL) {
      Clock::Start();
      App::SendNow(MIDI_SYS_CLK_START);
    }
    tick_ = midi_clock_prescaler_ - 1_u8;
    running() = 1;
    step_ = 0;
  }
}

const int8_t shifts[] PROGMEM = { -5, -7, -1, 0 };
const uint8_t durations[] PROGMEM = { 5, 0, 0, 0, 0, 1, 1, 2 };

/* static */
void Tanpura::Tick() {
  ++tick_;
  if (tick_ >= midi_clock_prescaler_) {
    App::SendScheduledNotes(channel());
    tick_ = 0;
    uint8_t note = 0;
    uint8_t actual_step = byteAnd(step_ + shift(), 0x07);
    uint8_t duration = pgm_read_byte(durations + actual_step);
    if (actual_step == 0) {
      note = root() - 12_u8;
    } else if (actual_step == 5) {
      int8_t shift = pgm_read_byte(shifts + pattern());
      if (shift != 0) {
        note = root() + shift;
      }
    } else if (actual_step >= 6) {
      note = root();
    }
    if (note) {
      App::Send3(noteOnFor(channel()), note, actual_step == 0 ? 127_u8 : 80_u8);
      App::SendLater(note, 0, duration - 1_u8);
    }
    step_ = byteAnd(step_ + 1, 0x7);
  }
}

} // namespace apps
} // namespace midipal
