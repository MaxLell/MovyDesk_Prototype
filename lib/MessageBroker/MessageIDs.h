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

    // Logging Control Messages
    MSG_0003, // Enable/Disable Logging for the Application Control Module
    MSG_0004, // Enable/Disable Logging for the Desk Control Module
    MSG_0005, // Enable/Disable Logging for the Presence Detector Module

    // Messages for Desk Control
    MSG_1000, // Move Desk to up, down, p1, p2, p3, p4, wake, memory
    MSG_1001, // Toggle Desk Position

    // Messages for the Presence Detector
    MSG_2001, // Presence Detected
    MSG_2002, // No Presence Detected

    // Messages for the Countdown Timer
    MSG_3001, // Start Countdown with Time Stamp
    MSG_3002, // Stop Countdown
    MSG_3003, // Countdown finished

    // Application Control Configuration Messages
    MSG_4001, // Set Timer Interval (in minutes)
    MSG_4002, // Get Timer Interval (query current interval)

    E_TOPIC_LAST_TOPIC // Last Topic - DO NOT USE (Only for boundary checks)
} msg_id_e;

#endif /* MESSAGEIDS_H_ */