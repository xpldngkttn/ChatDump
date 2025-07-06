/*
 * Copyright (c) 2017-2025 The Forge Interactive Inc.
 *
 * This file is part of The-Forge
 * (see https://github.com/ConfettiFX/The-Forge).
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

// buttonSet0
#define CONTROLLER_DPAD_LEFT_BIT  (1u << 0)
#define CONTROLLER_DPAD_RIGHT_BIT (1u << 1)
#define CONTROLLER_DPAD_UP_BIT    (1u << 2)
#define CONTROLLER_DPAD_DOWN_BIT  (1u << 3)
#define CONTROLLER_A_BIT          (1u << 4)
#define CONTROLLER_B_BIT          (1u << 5)
#define CONTROLLER_X_BIT          (1u << 6)
#define CONTROLLER_Y_BIT          (1u << 7)
#define CONTROLLER_L1_BIT         (1u << 8)
#define CONTROLLER_R1_BIT         (1u << 9)
#define CONTROLLER_L3_BIT         (1u << 10)
#define CONTROLLER_R3_BIT         (1u << 11)
#define CONTROLLER_START_BIT      (1u << 12)
#define CONTROLLER_SELECT_BIT     (1u << 13)
#define CONTROLLER_HOME_BIT       (1u << 14)

// buttonSet0
#define ESCAPE_BIT                (1u << 0)
#define F1_BIT                    (1u << 1)
#define F2_BIT                    (1u << 2)
#define F3_BIT                    (1u << 3)
#define F4_BIT                    (1u << 4)
#define F5_BIT                    (1u << 5)
#define F6_BIT                    (1u << 6)
#define F7_BIT                    (1u << 7)
#define F8_BIT                    (1u << 8)
#define F9_BIT                    (1u << 9)
#define F10_BIT                   (1u << 10)
#define F11_BIT                   (1u << 11)
#define F12_BIT                   (1u << 12)
#define BREAK_BIT                 (1u << 13)
#define INSERT_BIT                (1u << 14)
#define DEL_BIT                   (1u << 15)
#define NUM_LOCK_BIT              (1u << 16)
#define KP_MULTIPLY_BIT           (1u << 17)
#define ACUTE_BIT                 (1u << 18)
#define NUM_1_BIT                 (1u << 19)
#define NUM_2_BIT                 (1u << 20)
#define NUM_3_BIT                 (1u << 21)
#define NUM_4_BIT                 (1u << 22)
#define NUM_5_BIT                 (1u << 23)
#define NUM_6_BIT                 (1u << 24)
#define NUM_7_BIT                 (1u << 25)
#define NUM_8_BIT                 (1u << 26)
#define NUM_9_BIT                 (1u << 27)
#define NUM_0_BIT                 (1u << 28)
#define MINUS_BIT                 (1u << 29)
#define EQUAL_BIT                 (1u << 30)
#define BACK_SPACE_BIT            (1u << 31)
// buttonSet1
#define KP_ADD_BIT                (1u << 0)
#define KP_SUBTRACT_BIT           (1u << 1)
#define TAB_BIT                   (1u << 2)
#define Q_BIT                     (1u << 3)
#define W_BIT                     (1u << 4)
#define E_BIT                     (1u << 5)
#define R_BIT                     (1u << 6)
#define T_BIT                     (1u << 7)
#define Y_BIT                     (1u << 8)
#define U_BIT                     (1u << 9)
#define I_BIT                     (1u << 10)
#define O_BIT                     (1u << 11)
#define P_BIT                     (1u << 12)
#define BRACKET_LEFT_BIT          (1u << 13)
#define BRACKET_RIGHT_BIT         (1u << 14)
#define BACK_SLASH_BIT            (1u << 15)
#define KP_7_HOME_BIT             (1u << 16)
#define KP_8_UP_BIT               (1u << 17)
#define KP_9_PAGE_UP_BIT          (1u << 18)
#define CAPS_LOCK_BIT             (1u << 19)
#define A_BIT                     (1u << 20)
#define S_BIT                     (1u << 21)
#define D_BIT                     (1u << 22)
#define F_BIT                     (1u << 23)
#define G_BIT                     (1u << 24)
#define H_BIT                     (1u << 25)
#define J_BIT                     (1u << 26)
#define K_BIT                     (1u << 27)
#define L_BIT                     (1u << 28)
#define SEMICOLON_BIT             (1u << 29)
#define APOSTROPHE_BIT            (1u << 30)
#define ENTER_BIT                 (1u << 31)
// buttonSet2
#define KP_4_LEFT_BIT             (1u << 0)
#define KP_5_BEGIN_BIT            (1u << 1)
#define KP_6_RIGHT_BIT            (1u << 2)
#define SHIFT_L_BIT               (1u << 3)
#define Z_BIT                     (1u << 4)
#define X_BIT                     (1u << 5)
#define C_BIT                     (1u << 6)
#define V_BIT                     (1u << 7)
#define B_BIT                     (1u << 8)
#define N_BIT                     (1u << 9)
#define M_BIT                     (1u << 10)
#define COMMA_BIT                 (1u << 11)
#define PERIOD_BIT                (1u << 12)
#define FWRD_SLASH_BIT            (1u << 13)
#define SHIFT_R_BIT               (1u << 14)
#define KP_1_END_BIT              (1u << 15)
#define KP_2_DOWN_BIT             (1u << 16)
#define KP_3_PAGE_DOWN_BIT        (1u << 17)
#define CTRL_L_BIT                (1u << 18)
#define ALT_L_BIT                 (1u << 19)
#define SPACE_BIT                 (1u << 20)
#define ALT_R_BIT                 (1u << 21)
#define CTRL_R_BIT                (1u << 22)
#define LEFT_BIT                  (1u << 23)
#define RIGHT_BIT                 (1u << 24)
#define UP_BIT                    (1u << 25)
#define DOWN_BIT                  (1u << 26)
#define KP_0_INSERT_BIT           (1u << 27)
#define LEFT_CLICK_BIT            (1u << 28)
#define MID_CLICK_BIT             (1u << 29)
#define RIGHT_CLICK_BIT           (1u << 30)
#define SCROLL_UP_BIT             (1u << 31)

// buttonSet3
#define SCROLL_DOWN_BIT           (1u << 0)

#define INPUT_DEVICE_GAMEPAD      0
#define INPUT_DEVICE_KBM          1
#define INPUT_DEVICE_TOUCH        2
