// Chords Firmware for STM32F401 / STM32F411 Black Pill Board
// Use with Chords applications:
// Chords-Web: chords.upsidedownlabs.tech
// Chords-Python: github.com/upsidedownlabs/chords-python
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
// Copyright (c) 2024 - 2025 Upside Down Labs - contact@upsidedownlabs.tech
// Author: Deepak Khatri
//
// At Upside Down Labs, we create open-source DIY neuroscience hardware and software.
// Our mission is to make neuroscience affordable and accessible for everyone.
// By supporting us with your purchase, you help spread innovation and open science.
// Thank you for being part of this journey with us!

#include <Arduino.h>

// Macros Definitions
#define NUM_CHANNELS 8                                    // Number of channels supported
#define HEADER_LEN 3                                      // Header: SYNC_BYTE_1 + SYNC_BYTE_2 + Counter
#define PACKET_LEN (HEADER_LEN + (NUM_CHANNELS * 2) + 1)  // Packet length = Header + Data + END_BYTE
#define SAMP_RATE 500.0                                   // Sampling rate
#define SYNC_BYTE_1 0xC7                                  // Packet first sync byte
#define SYNC_BYTE_2 0x7C                                  // Packet second sync byte
#define END_BYTE 0x01                                     // Packet last check byte
#define BAUD_RATE 230400                                  // Serial connection baud rate

// Hardware Timer for ADC sampling
HardwareTimer *timer = new HardwareTimer(TIM3);

// Define ADC channels (PA0 to PA7, PB0, PB1)
const int adcPins[] = { PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7 };

// Global constants and variables
uint8_t packetBuffer[PACKET_LEN];  // The transmission packet
uint8_t currentChannel;            // Current channel being sampled
uint16_t adcValue = 0;             // ADC current value
bool timerStatus = false;          // Timer satus flag
bool bufferReady = false;          // Buffer ready flag

void timerStart() {
  timerStatus = true;
  timer->resume();
}

void timerStop() {
  timerStatus = false;
  timer->pause();
  bufferReady = false;
}

void timerCallback() {
  if (!timerStatus or Serial.available()) {
    timerStop();
    return;
  }

  // Set buffer ready flag
  bufferReady = true;
}

void setup() {
  // Initialize the serial communication
  Serial.begin(BAUD_RATE);
  while (!Serial) {
    ;  // Wait for serial port to connect
  }

  // Configure ADC pins
  for (int i = 0; i < NUM_CHANNELS; i++) {
    pinMode(adcPins[i], INPUT_ANALOG);
  }

  // Set ADC resolution to 12 bits
  analogReadResolution(12);

  // Initialize packetBuffer
  packetBuffer[0] = SYNC_BYTE_1;            // Sync 0
  packetBuffer[1] = SYNC_BYTE_2;            // Sync 1
  packetBuffer[2] = 0;                      // Packet counter
  packetBuffer[PACKET_LEN - 1] = END_BYTE;  // End Byte

  // Configure HardwareTimer for ADC sampling
  timer->setOverflow(SAMP_RATE, HERTZ_FORMAT);  // Set timer frequency for oversampling
  timer->attachInterrupt(timerCallback);        // Attach the callback function
}

void loop() {
  // Transmit data if buffer is ready
  if (timerStatus && bufferReady) {
    // Read 6ch ADC inputs and store current values in packetBuffer
    for (currentChannel = 0; currentChannel < NUM_CHANNELS; currentChannel++) {
      adcValue = analogRead(adcPins[currentChannel]);                             // Read Analog input
      packetBuffer[((2 * currentChannel) + HEADER_LEN)] = highByte(adcValue);     // Write High Byte
      packetBuffer[((2 * currentChannel) + HEADER_LEN + 1)] = lowByte(adcValue);  // Write Low Byte
    }

    // Increment the packet counter
    packetBuffer[2]++;
    // Transmit the packet
    Serial.write(packetBuffer, PACKET_LEN);
    // Reset the buffer ready flag
    bufferReady = false;
  }

  // Handle commands from the serial interface
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();         // Remove extra spaces or newline characters
    command.toUpperCase();  // Normalize to uppercase for case-insensitivity

    if (command == "WHORU")  // Who are you?
    {
      Serial.println("STM32F4-BLACK-PILL");
    } else if (command == "START")  // Start data acquisition
    {
      timerStart();
    } else if (command == "STOP")  // Stop data acquisition
    {
      timerStop();
    } else if (command == "STATUS")  // Get status
    {
      Serial.println(timerStatus ? "RUNNING" : "STOPPED");
    } else {
      Serial.println("UNKNOWN COMMAND");
    }
  }
}