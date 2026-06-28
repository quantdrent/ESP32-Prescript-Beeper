#pragma once

#include <Arduino.h>

struct PrescriptSeed {
  const char *text;
  uint32_t durationMs;
};

const PrescriptSeed DEFAULT_PRESCRIPTS[] = {
  { "Larp As the black silence.", 3000 },
  { "Hop on aba.", 3000 },
  { "do a 2 star gregor solo md run.", 3000 }
};

constexpr size_t DEFAULT_PRESCRIPT_COUNT =
  sizeof(DEFAULT_PRESCRIPTS) / sizeof(DEFAULT_PRESCRIPTS[0]);
