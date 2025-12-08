#ifndef PATH_MEMORY_H
#define PATH_MEMORY_H

#include "config.h"

// Path memory states
typedef enum
{
    PATH_IDLE,
    PATH_RECORDING,
    PATH_PLAYING
} PathState;

/**
 * Initialize path memory system
 */
void pathMemoryInit();

/**
 * Start recording path
 */
void pathStartRecording();

/**
 * Stop recording path
 */
void pathStopRecording();

/**
 * Start playing back recorded path
 */
void pathStartPlayback();

/**
 * Stop playback
 */
void pathStopPlayback();

/**
 * Clear recorded path
 */
void pathClear();

/**
 * Get current path state
 */
PathState pathGetState();

/**
 * Get number of recorded points
 */
int pathGetPointCount();

/**
 * Record a command to the path
 */
void pathRecordCommand(RobotCommand cmd);

/**
 * Get next playback command
 */
bool pathGetNextCommand(RobotCommand *cmd, uint32_t *duration_ms);

/**
 * Get path data as JSON for Flask interface
 */
String pathGetJSON();

/**
 * Load path from JSON (from Flask interface)
 */
bool pathLoadFromJSON(const String &json);

/**
 * HTTP handlers for path memory (called from wifi_control task)
 */
void pathHandleHTTP();

#endif // PATH_MEMORY_H
