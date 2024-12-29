#ifndef DISPLAY_EXPORT_H
#define DISPLAY_EXPORT_H

#if defined(COMPONENT_BUILD)

#if defined(WIN32)

#if defined(DISPLAY_IMPLEMENTATION)
#define DISPLAY_EXPORT __declspec(dllexport)
#else
#define DISPLAY_EXPORT __declspec(dllimport)
#endif

#else  // !defined(WIN32)

#if defined(DISPLAY_IMPLEMENTATION)
#define DISPLAY_EXPORT __attribute__((visibility("default")))
#else
#define DISPLAY_EXPORT
#endif

#endif

#else  // !defined(COMPONENT_BUILD)

#define DISPLAY_EXPORT

#endif

#endif  // DISPLAY_EXPORT_H