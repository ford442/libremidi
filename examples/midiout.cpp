//*****************************************//
//  midiout.cpp
//  by Gary Scavone, 2003-2004.
//
//  Simple program to test MIDI output.
//
//*****************************************//

#include "utils.hpp"

#include <libremidi/libremidi.hpp>

using namespace libremidi;
using namespace std::literals;

#include <array>
#include <chrono>
// #include <thread>

int main()

midi_out midiout;
auto ports=observer{{},observer_configuration_for(midiout.get_current_api())}.get_output_ports();
midiout.open_virtual_port();

midiout.open_port(ports[0]);
  
  // Send out a series of MIDI messages.


  // Note On: 144, 64, 90
  midiout.send_message(144, 64, 90);


  // Note Off: 128, 64, 40
  midiout.send_message(128, 64, 40);

  return 0;


}
