#ifndef MESSAGEIDS_H_
#define MESSAGEIDS_H_

/**
 * Message Topics
 */
typedef enum
{
    E_TOPIC_FIRST_TOPIC = 0U, // First Topic - DO NOT USE (Only for boundary checks)

    // Test Messages
    MSG_0001, // Chaos Elephant
    MSG_0002, // Tickly Giraffe

    // Messages for Desk Control
    MSG_1001, // Desk Move Up
    MSG_1002, // Desk Move Down
    MSG_1003, // Desk move to P1 Preset
    MSG_1004, // Desk move to P2 Preset
    MSG_1005, // Desk Wake/Enable
    MSG_1006, // Desk Memory/Store Position
    MSG_1007, // Desk move to P3 Preset
    MSG_1008, // Desk move to P4 Preset

    // Messages for the Presence Detector
    MSG_2001, // Presence Detected
    MSG_2002, // No Presence Detected
    MSG_2003, // Enable Presence Detector Logging
    MSG_2004, // Disable Presence Detector Logging
    MSG_2005, // Start Presence Detection
    MSG_2006, // Stop Presence Detection

    // Messages for the Countdown Timer
    MSG_3001, // Countdown Time Stamp
    MSG_3002, // Start Countdown
    MSG_3003, // Countdown finished

    E_TOPIC_LAST_TOPIC // Last Topic - DO NOT USE (Only for boundary checks)
} msg_id_e;

#endif /* MESSAGEIDS_H_ */