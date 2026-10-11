// Retain the engine's real Exception constructor without executing its startup.
#include <SDL.h>

#undef main
#define main runUnusedEngineEntryPoint
#include "../../main.cpp"
#undef main
