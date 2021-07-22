#pragma once

// The CPU features setup and available after the call to
// setup_arch().
struct cpu_features {

  /// The CPU has support for the NX bit. If this is true, the error
  /// code of page faults will also indicate whether the exception was
  /// because of an instruction fetch or not.
  bool has_nx = false;
};
