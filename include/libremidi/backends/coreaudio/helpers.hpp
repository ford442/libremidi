#pragma once
#include <libremidi/detail/midi_api.hpp>
#include <libremidi/libremidi.hpp>

#include <CoreMIDI/CoreMIDI.h>
#include <CoreServices/CoreServices.h>

#include <cmath>

#if TARGET_OS_IPHONE
  #include <CoreAudio/CoreAudioTypes.h>
  #include <mach/mach_time.h>
  #define AudioGetCurrentHostTime mach_absolute_time
#else
  #include <CoreAudio/HostTime.h>
#endif

namespace libremidi
{
using CFString_handle = unique_handle<const __CFString, CFRelease>;
using CFStringMutable_handle = unique_handle<__CFString, CFRelease>;
namespace
{
#if TARGET_OS_IPHONE
inline uint64_t AudioConvertHostTimeToNanos(uint64_t hostTime)
{
  static const struct mach_timebase_info timebase = [] {
    struct mach_timebase_info theTimeBaseInfo;
    mach_timebase_info(&theTimeBaseInfo);
    return theTimeBaseInfo;
  }();
  const auto numer = timebase.numer;
  const auto denom = timebase.denom;

  __uint128_t res = hostTime;
  if (numer != denom)
  {
    res *= numer;
    res /= denom;
  }
  return static_cast<uint64_t>(res);
}
#endif
// This function was submitted by Douglas Casey Tucker and apparently
// derived largely from PortMidi.
inline CFStringRef EndpointName(MIDIEndpointRef endpoint, bool isExternal)
{
  CFMutableStringRef result = CFStringCreateMutable(nullptr, 0);

  static constexpr auto getProp = [] (MIDIObjectRef prop) {
    CFStringRef str = nullptr;
    MIDIObjectGetStringProperty(prop, kMIDIPropertyName, &str);
    return CFString_handle{str};
  };

  // Begin with the endpoint's name.
  if(auto endpoint_name = getProp(endpoint)) {
    CFStringAppend(result, endpoint_name.get());
  }

  // some MIDI devices have a leading space in endpoint name. trim
  CFStringTrim(result, CFSTR(" "));

  MIDIEntityRef entity = 0;
  MIDIDeviceRef device = 0;
  MIDIEndpointGetEntity(endpoint, &entity);
  if (entity == 0)
    goto finish;

  if (CFStringGetLength(result) == 0)
  {
    // endpoint name has zero length -- try the entity
    if(auto entity_name = getProp(entity)) {
      CFStringAppend(result, entity_name.get());
    }
  }

  // now consider the device's name
  MIDIEntityGetDevice(entity, &device);
  if (device == 0)
    goto finish;

  if (auto dev_name = getProp(device))
  {
    const auto dev_strlen = CFStringGetLength(dev_name.get());

    // if an external device has only one entity, throw away
    // the endpoint name and just use the device name
    if (CFStringGetLength(result) == 0 || (isExternal && MIDIDeviceGetNumberOfEntities(device) < 2))
    {
      CFStringAppend(result, dev_name.get());
      goto finish;
    }

    // does the entity name already start with the device name?
    // (some drivers do this though they shouldn't)
    // if so, do not prepend
    if (CFStringCompareWithOptions(
            result, dev_name.get(), CFRangeMake(0, dev_strlen), 0)
        != kCFCompareEqualTo)
    {
      // prepend the device name to the entity name
      if (CFStringGetLength(result) > 0)
        CFStringInsert(result, 0, CFSTR(" "));
      CFStringInsert(result, 0, dev_name.get());
    }
  }

  finish:
  if(CFStringGetLength(result) == 0)
    return CFSTR("No name");
  else
    return result;
}

// This function was submitted by Douglas Casey Tucker and apparently
// derived largely from PortMidi.
inline CFStringRef ConnectedEndpointName(MIDIEndpointRef endpoint)
{
  CFMutableStringRef result = CFStringCreateMutable(nullptr, 0);
  CFStringRef str{};

  // Does the endpoint have connections?
  CFDataRef connections = nullptr;
  std::size_t nConnected = 0;
  bool anyStrings = false;
  MIDIObjectGetDataProperty(endpoint, kMIDIPropertyConnectionUniqueID, &connections);
  if (connections != nullptr)
  {
    // It has connections, follow them
    // Concatenate the names of all connected devices
    nConnected = CFDataGetLength(connections) / sizeof(MIDIUniqueID);
    if (nConnected)
    {
      const SInt32* pid = (const SInt32*)(CFDataGetBytePtr(connections));
      for (std::size_t i = 0; i < nConnected; ++i, ++pid)
      {
        MIDIUniqueID id = CFSwapInt32BigToHost(*pid);
        MIDIObjectRef connObject;
        MIDIObjectType connObjectType;
        auto err = MIDIObjectFindByUniqueID(id, &connObject, &connObjectType);
        if (err == noErr)
        {
          if (connObjectType == kMIDIObjectType_ExternalSource
              || connObjectType == kMIDIObjectType_ExternalDestination)
          {
            // Connected to an external device's endpoint (10.3 and later).
            str = EndpointName((MIDIEndpointRef)(connObject), true);
          }
          else
          {
            // Connected to an external device (10.2) (or something else,
            // catch-
            str = nullptr;
            MIDIObjectGetStringProperty(connObject, kMIDIPropertyName, &str);
          }
          if (str != nullptr)
          {
            if (anyStrings)
              CFStringAppend(result, CFSTR(", "));
            else
              anyStrings = true;
            CFStringAppend(result, str);
            CFRelease(str);
          }
        }
      }
    }
    CFRelease(connections);
  }
  if (anyStrings)
    return result;

  CFRelease(result);

  // Here, either the endpoint had no connections, or we failed to obtain names
  return EndpointName(endpoint, false);
}

}

// A structure to hold variables related to the CoreMIDI API
// implementation.
struct coremidi_data
{
  MIDIClientRef client{};
  MIDIPortRef port{};
  MIDIEndpointRef endpoint{};

  static inline CFString_handle toCFString(std::string_view str) noexcept
  {
    return CFString_handle{CFStringCreateWithCString(nullptr, str.data(), kCFStringEncodingASCII) };
  }
};

}
