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
    MSG_0006, // Enable/Disable Logging for the Network Time Module

    // Messages for Desk Control
    MSG_1000, // Move Desk to up, down, p1, p2, p3, p4, wake, memory
    MSG_1001, // Toggle Desk Position
    MSG_1002, // Get Desk Height (query current height)

    // Messages for the Presence Detector
    MSG_2001, // Presence Detected
    MSG_2002, // No Presence Detected
    MSG_2003, // Set Presence Threshold (number of close devices)
    MSG_2004, // Get Presence Threshold (query current threshold)

    // Messages for the Countdown Timer
    MSG_3001, // Start Countdown with Time Stamp
    MSG_3002, // Stop Countdown
    MSG_3003, // Countdown finished

    // Application Control Configuration Messages
    MSG_4001, // Set Timer Interval (in minutes)
    MSG_4002, // Get Timer Interval (query current interval)
    MSG_4003, // Get Elapsed Timer Time (query how long timer has been running)

    // Network Time Module Messages
    MSG_5001, // Set WiFi Credentials
    MSG_5002, // Get WiFi Credentials
    MSG_5003, // Get WiFi Status
    MSG_5004, // Get Time Info

    E_TOPIC_LAST_TOPIC // Last Topic - DO NOT USE (Only for boundary checks)
} msg_id_e;

#endif /* MESSAGEIDS_H_ */