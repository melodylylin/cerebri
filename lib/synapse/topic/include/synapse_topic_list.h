/*
 * Copyright (c) 2023 CogniPilot Foundation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SYNAPSE_TOPIC_LIST_H
#define SYNAPSE_TOPIC_LIST_H

#include <zros/zros_topic.h>

#include <synapse_protobuf/actuators.pb.h>
#include <synapse_protobuf/altimeter.pb.h>
#include <synapse_protobuf/battery_state.pb.h>
#include <synapse_protobuf/bezier_trajectory.pb.h>
#include <synapse_protobuf/imu.pb.h>
#include <synapse_protobuf/joy.pb.h>
#include <synapse_protobuf/led_array.pb.h>
#include <synapse_protobuf/magnetic_field.pb.h>
#include <synapse_protobuf/nav_sat_fix.pb.h>
#include <synapse_protobuf/odometry.pb.h>
#include <synapse_protobuf/safety.pb.h>
#include <synapse_protobuf/status.pb.h>
#include <synapse_protobuf/time.pb.h>
#include <synapse_protobuf/twist.pb.h>
#include <synapse_protobuf/vector3.pb.h>
#include <synapse_protobuf/wheel_odometry.pb.h>

/********************************************************************
 * helper
 ********************************************************************/
void stamp_header(synapse_msgs_Header* hdr, int64_t ticks);
const char* mode_str(synapse_msgs_Status_Mode mode);
const char* armed_str(synapse_msgs_Status_Arming arming);
const char* safety_str(synapse_msgs_Safety_Status safety);
const char* fuel_str(synapse_msgs_Status_Fuel fuel);
const char* status_safety_str(synapse_msgs_Status_Safety safety);
const char* status_joy_str(synapse_msgs_Status_Joy joy);

enum {
    JOY_BUTTON_MANUAL = 0,
    JOY_BUTTON_AUTO = 1,
    JOY_BUTTON_CMD_VEL = 2,
    JOY_BUTTON_CALIBRATION = 3,
    JOY_BUTTON_LIGHTS_OFF = 4,
    JOY_BUTTON_LIGHTS_ON = 5,
    JOY_BUTTON_DISARM = 6,
    JOY_BUTTON_ARM = 7,
};

enum {
    JOY_AXES_THRUST = 1,
    JOY_AXES_PITCH = 2,
    JOY_AXES_ROLL = 3,
    JOY_AXES_YAW = 4,
};

/********************************************************************
 * topics
 ********************************************************************/
ZROS_TOPIC_DECLARE(topic_actuators, synapse_msgs_Actuators);
ZROS_TOPIC_DECLARE(topic_actuators_manual, synapse_msgs_Actuators);
ZROS_TOPIC_DECLARE(topic_altimeter, synapse_msgs_Altimeter);
ZROS_TOPIC_DECLARE(topic_attitude_sp, synapse_msgs_Vector3);
ZROS_TOPIC_DECLARE(topic_battery_state, synapse_msgs_BatteryState);
ZROS_TOPIC_DECLARE(topic_bezier_trajectory, synapse_msgs_BezierTrajectory);
ZROS_TOPIC_DECLARE(topic_clock_offset, synapse_msgs_Time);
ZROS_TOPIC_DECLARE(topic_cmd_vel, synapse_msgs_Twist);
ZROS_TOPIC_DECLARE(topic_estimator_odometry, synapse_msgs_Odometry);
ZROS_TOPIC_DECLARE(topic_external_odometry, synapse_msgs_Odometry);
ZROS_TOPIC_DECLARE(topic_imu, synapse_msgs_Imu);
ZROS_TOPIC_DECLARE(topic_joy, synapse_msgs_Joy);
ZROS_TOPIC_DECLARE(topic_led_array, synapse_msgs_LEDArray);
ZROS_TOPIC_DECLARE(topic_magnetic_field, synapse_msgs_MagneticField);
ZROS_TOPIC_DECLARE(topic_nav_sat_fix, synapse_msgs_NavSatFix);
ZROS_TOPIC_DECLARE(topic_angular_velocity_sp, synapse_msgs_Vector3);
ZROS_TOPIC_DECLARE(topic_safety, synapse_msgs_Safety);
ZROS_TOPIC_DECLARE(topic_status, synapse_msgs_Status);
ZROS_TOPIC_DECLARE(topic_wheel_odometry, synapse_msgs_WheelOdometry);
ZROS_TOPIC_DECLARE(topic_force_sp, synapse_msgs_Vector3);
ZROS_TOPIC_DECLARE(topic_moment_sp, synapse_msgs_Vector3);
ZROS_TOPIC_DECLARE(topic_position_sp, synapse_msgs_Vector3);
ZROS_TOPIC_DECLARE(topic_velocity_sp, synapse_msgs_Vector3);
ZROS_TOPIC_DECLARE(topic_orientation_sp, synapse_msgs_Vector3);

#endif // SYNAPSE_TOPIC_LIST_H_
// vi: ts=4 sw=4 et
