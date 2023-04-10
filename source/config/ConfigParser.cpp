/****************************************************************************
 * Copyright (C) 2016,2017 Maschell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include "./ConfigParser.hpp"
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/StringTools.h>

ConfigParser::ConfigParser(std::string configData) {
    this->content      = configData;
    this->contentLines = StringTools::stringSplit(content, "");

    if (contentLines.empty()) {
        return;
    }

    //remove the comments and make everything uppercase
    for (uint32_t i = 0; i < contentLines.size(); i++) {
        std::vector<std::string> comments = StringTools::stringSplit(contentLines[i], "//");
        if (!comments.empty()) {
            contentLines[i] = comments[0];
        }
        //we want everything uppercase
        std::transform(contentLines[i].begin(), contentLines[i].end(), contentLines[i].begin(), ::toupper);
    }

    //remove empty lines
    std::vector<std::string> contentline2;
    for (uint32_t i = 0; i < contentLines.size(); i++) {
        if (strlen(contentLines[i].c_str()) > 0) {
            contentline2.push_back(contentLines[i]);
        }
    }
    contentLines = contentline2;
    Init();
}

ConfigParser::~ConfigParser() {
}

PARSE_TYPE ConfigParser::getType() {
    return type_b;
}

void ConfigParser::setType(PARSE_TYPE newType) {
    this->type_b = newType;
}

uint16_t ConfigParser::getSlot() {
    return this->slot_b;
}

void ConfigParser::setSlot(uint16_t newSlot) {
    this->slot_b = newSlot;
}

BOOL ConfigParser::Init() {
    if (contentLines.size() == 0) {
        DEBUG_FUNCTION_LINE("File seems to be empty. Make sure to have a proper header");
        return false;
    }
    const char *line = contentLines[0].c_str();
    int32_t len      = strlen(line);
    if (len <= 4) {
        DEBUG_FUNCTION_LINE("Header is too short.");
        return false;
    }
    std::string identify;

    if (line[0] == '[' && line[len - 1] == ']') {
        identify = contentLines[0].substr(1, len - 2);
    } else {
        DEBUG_FUNCTION_LINE("Not a proper config file!");
        return false;
    }

    if (identify.compare("GAMEPAD") == 0) {
        DEBUG_FUNCTION_LINE("Its a gamepad config file!");
        setSlot(gGamePadSlot);
        setType(PARSE_GAMEPAD);
    } else if (identify.compare("MOUSE") == 0) {
        DEBUG_FUNCTION_LINE("Its a mouse config file!");
        setSlot(gMouseSlot);
        setType(PARSE_MOUSE);
        this->vid = HID_MOUSE_VID;
        this->pid = HID_MOUSE_PID;
    } else if (identify.compare("KEYBOARD") == 0) {
        DEBUG_FUNCTION_LINE("Its a keyboard config file!");
        setSlot(gHID_SLOT_KEYBOARD);
        setType(PARSE_KEYBOARD);
        this->vid = HID_KEYBOARD_VID;
        this->pid = HID_KEYBOARD_PID;
    } else {
        DEBUG_FUNCTION_LINE("Its a controller config file!");
        setSlot(getSlotController(identify));
        setType(PARSE_CONTROLLER);
    }

    if (getSlot() == HID_INVALID_SLOT) {
        return false;
    }

    ConfigReader::increaseNumberOfLoadedFiles();

    return true;
}

void ConfigParser::parseSingleLine(std::string line) {
    if (line.empty()) {
        DEBUG_FUNCTION_LINE("Can't parse line. it's empty");
        return;
    }
    std::vector<std::string> cur_values = StringTools::stringSplit(line, "=");
    if (cur_values.size() != 2) {
        if (HID_DEBUG || cur_values.size() > 2) {
            DEBUG_FUNCTION_LINE("Not a valid key=pair line %s", line.c_str());
        }
        return;
    } else {
        uint16_t hid_slot = getSlot();

        if (HID_DEBUG) {
            DEBUG_FUNCTION_LINE("leftpart = \"%s\" ", cur_values[0].c_str());
        }
        if (HID_DEBUG) {
            DEBUG_FUNCTION_LINE("rightpart = \"%s\" ", cur_values[1].c_str());
        }
        int32_t keyslot = -1;

        if (HID_DEBUG) {
            DEBUG_FUNCTION_LINE("Checking single value");
        }
        if (getType() == PARSE_GAMEPAD || getType() == PARSE_KEYBOARD) {
            keyslot = ConfigValues::getKeySlotGamePad(cur_values[0]);
        } else if (getType() == PARSE_MOUSE) {
            keyslot = ConfigValues::getKeySlotMouse(cur_values[0]);
        } else {
            keyslot = ConfigValues::getKeySlotDefaultSingleValue(cur_values[0]);
        }
        if (keyslot != -1) {
            if (HID_DEBUG) {
                DEBUG_FUNCTION_LINE("Its a single value");
            }
            long rightValue = -1;
            BOOL valueSet   = false;
            if (cur_values[0].compare("DPAD_MODE") == 0) {
                const uint8_t *values_ = NULL;
                if ((values_ = ConfigValues::getValuesStickPreset(cur_values[1])) != NULL) {
                    if (values_[STICK_CONF_MAGIC_VERSION] != STICK_CONF_MAGIC_VALUE)
                        if (HID_DEBUG) {
                            DEBUG_FUNCTION_LINE("Settings preset DPAD MODE and Mask");
                        }
                    config_controller[hid_slot][CONTRPS_DPAD_MODE][0] = CONTROLLER_PATCHER_VALUE_SET;
                    config_controller[hid_slot][CONTRPS_DPAD_MODE][1] = values_[CONTRDPAD_MODE];
                    if (values_[CONTRDPAD_MASK] != 0x00) {
                        config_controller[hid_slot][CONTRPS_DPAD_MASK][0] = CONTROLLER_PATCHER_VALUE_SET;
                        config_controller[hid_slot][CONTRPS_DPAD_MASK][1] = values_[CONTRDPAD_MASK];
                    }
                    valueSet = true;
                }
            }

            if (!valueSet) {
                if (getType() == PARSE_KEYBOARD) {
                    if ((rightValue = ConfigValues::getPresetValuesKeyboard(cur_values[1])) != -1) {
                        if (HID_DEBUG) {
                            DEBUG_FUNCTION_LINE("Used pre-defined Keyboard! \"%s\" is %d", cur_values[1].c_str(), rightValue);
                        }
                    } else {
                        if (HID_DEBUG) {
                            DEBUG_FUNCTION_LINE("I need to parse %s", cur_values[1].c_str());
                        }
                        char *ptr;
                        rightValue = strtol(cur_values[1].c_str(), &ptr, 16);
                    }
                } else {
                    rightValue = ConfigValues::getPresetValue(cur_values[1]);

                    if (getType() == PARSE_MOUSE) { //No parsing for the mouse
                        if (rightValue == -1) {
                            if (HID_DEBUG) {
                                DEBUG_FUNCTION_LINE("Invalid mouse value, lets skip it %s", cur_values[1].c_str());
                            }
                            return;
                        }
                    } else {
                        if (rightValue == -1) {
                            if (HID_DEBUG) {
                                DEBUG_FUNCTION_LINE("I need to parse %s", cur_values[1].c_str());
                            }
                            char *ptr;
                            rightValue = strtol(cur_values[1].c_str(), &ptr, 16);
                        }
                    }
                }
                if (HID_DEBUG) {
                    DEBUG_FUNCTION_LINE("Setting value to %d", rightValue);
                }

                config_controller[hid_slot][keyslot][0] = CONTROLLER_PATCHER_VALUE_SET;
                config_controller[hid_slot][keyslot][1] = rightValue;
            }
        } else {
            if (HID_DEBUG) {
                DEBUG_FUNCTION_LINE("Check pair value");
            }
            keyslot = ConfigValues::getKeySlotDefaultPairedValue(cur_values[0]);
            if (keyslot != -1) {
                if (HID_DEBUG) {
                    DEBUG_FUNCTION_LINE("Its a  pair value");
                }

                if (!ConfigValues::getInstance()->setIfValueIsAControllerPreset(cur_values[1], getSlot(), keyslot)) {
                    if (HID_DEBUG) {
                        DEBUG_FUNCTION_LINE("And its no preset");
                    }
                    std::vector<std::string> rightvalues = StringTools::stringSplit(cur_values[1], ",");

                    if (rightvalues.size() != 2) {
                        DEBUG_FUNCTION_LINE("%d instead of 2 key=values pairs in line", rightvalues.size());
                        return;
                    }

                    char *ptr;
                    long firstValue                         = strtol(rightvalues[0].c_str(), &ptr, 16);
                    long secondValue                        = strtol(rightvalues[1].c_str(), &ptr, 16);
                    config_controller[hid_slot][keyslot][0] = firstValue;
                    config_controller[hid_slot][keyslot][1] = secondValue;

                    if (HID_DEBUG) {
                        DEBUG_FUNCTION_LINE("Set %02X,%02X", firstValue, secondValue);
                    }
                } else {
                    if (HID_DEBUG) {
                        DEBUG_FUNCTION_LINE("Found preset value!!");
                    }
                }
            } else {
                DEBUG_FUNCTION_LINE("The setting \"%s\" is unknown!", cur_values[0].c_str());
            }
        }
    }
}

BOOL ConfigParser::resetConfig() {
    int32_t slot = getSlot();
    if ((slot == HID_INVALID_SLOT) || (slot >= gHIDMaxDevices)) return false;
    for (int32_t j = (CONTRPS_PID + 1); j < CONTRPS_MAX_VALUE; j++) {
        config_controller[slot][j][0] = CONTROLLER_PATCHER_INVALIDVALUE;
        config_controller[slot][j][1] = CONTROLLER_PATCHER_INVALIDVALUE;
    }
    return true;
}

int32_t ConfigParser::getSlotController(std::string identify) {
    if (HID_DEBUG) {
        DEBUG_FUNCTION_LINE("Getting Controller Slot");
    }

    std::vector<std::string> values = StringTools::stringSplit(identify, ",");

    if (values.size() != 2) {
        DEBUG_FUNCTION_LINE("You need to provide a VID and PID. e.g. \"[vid=0x451,pid=0x152]\". (%s)", identify.c_str());
        return HID_INVALID_SLOT;
    }

    int32_t vid = getValueFromKeyValue(values[0], "VID", "=");
    if (vid < 0) {
        return HID_INVALID_SLOT;
    }
    int32_t pid = getValueFromKeyValue(values[1], "PID", "=");
    if (pid < 0) {
        return HID_INVALID_SLOT;
    }
    DEBUG_FUNCTION_LINE("VID: %04x PID: %04x", vid, pid);

    this->vid = vid;
    this->pid = pid;
    DeviceInfo deviceinfo;
    memset(&deviceinfo, 0, sizeof(deviceinfo));
    deviceinfo.vidpid.vid = vid;
    deviceinfo.vidpid.pid = pid;
    int32_t result        = ControllerPatcherUtils::getDeviceInfoFromVidPid(&deviceinfo);
    int32_t slot          = deviceinfo.slotdata.deviceslot;
    int32_t hid           = 0;
    if (result < 0) {
        if (HID_DEBUG) {
            DEBUG_FUNCTION_LINE("Its a new controller, lets save it");
        }

        HIDSlotData slotdata;
        ControllerPatcherUtils::getNextSlotData(&slotdata);

        slot = slotdata.deviceslot;
        hid  = slotdata.hidmask;

        if (slot >= gHIDMaxDevices) {
            DEBUG_FUNCTION_LINE("We don't a space for a new controller, please delete .inis");
            return HID_INVALID_SLOT;
        }
        if (HID_DEBUG) {
            DEBUG_FUNCTION_LINE("Got new slot! slot: %d hid %s .. Lets registrate it!", slot, StringTools::byte_to_binary(hid));
        }
        config_controller[slot][CONTRPS_VID][0] = (vid & 0xFF00) >> 8;
        config_controller[slot][CONTRPS_VID][1] = (vid & 0x00FF);
        config_controller[slot][CONTRPS_PID][0] = (pid & 0xFF00) >> 8;
        config_controller[slot][CONTRPS_PID][1] = (pid & 0x00FF);

        if (HID_DEBUG) {
            DEBUG_FUNCTION_LINE("Saved vid: %04X pid: %04X",
                                config_controller[slot][CONTRPS_VID][0] * 0x100 + config_controller[slot][CONTRPS_VID][1],
                                config_controller[slot][CONTRPS_PID][0] * 0x100 + config_controller[slot][CONTRPS_PID][1]);
        }

        config_controller_hidmask[slot] = hid;
        if (HID_DEBUG) {
            DEBUG_FUNCTION_LINE("Saved the hid");
        }

    } else {
        if (slot < gHIDMaxDevices) {
            hid = config_controller_hidmask[slot];
            if (HID_DEBUG) {
                DEBUG_FUNCTION_LINE(">>>>>> found slot %d (hid:%s). Modifing existing data <<<<<<<<", slot, StringTools::byte_to_binary(hid));
            }
            DEBUG_FUNCTION_LINE("We already have data of this controller, lets modify it");
        } else {
            DEBUG_FUNCTION_LINE("Something really odd happend to the slots. %d is bigger then max (%d)", slot, gHIDMaxDevices);
            return HID_INVALID_SLOT;
        }
    }

    DEBUG_FUNCTION_LINE("using slot: %d hid %08X", slot, hid);
    return slot;
}

BOOL ConfigParser::parseIni() {
    if (getSlot() == HID_INVALID_SLOT) {
        DEBUG_FUNCTION_LINE("Couldn't parse file. Not a valid slot. Probably broken config. Or you tried to have more than %d devices", getType(), gHIDMaxDevices);
        return false;
    }


    if (HID_DEBUG) {
        DEBUG_FUNCTION_LINE("Parsing content, type %d", getType());
    }

    int32_t start = 1;
    if (contentLines.size() <= 1) {
        DEBUG_FUNCTION_LINE("File only contains a header.");
        return false;
    }
    if (contentLines[1].compare("[IGNOREDEFAULT]") == 0) {
        resetConfig();
        DEBUG_FUNCTION_LINE("Ignoring existing settings of this device");
        start++;
    }

    for (uint32_t i = start; i < contentLines.size(); i++) {
        if (HID_DEBUG) {
            DEBUG_FUNCTION_LINE("line %d: \"%s\" ", (i + 1), contentLines[i].c_str());
        }
        parseSingleLine(contentLines[i]);
    }

    if (HID_DEBUG) {
        DEBUG_FUNCTION_LINE("Parsing of the file is done.");
    }
    return true;
}

int32_t ConfigParser::getValueFromKeyValue(std::string value_pair, std::string expectedKey, std::string delimiter) {
    std::vector<std::string> string_value = StringTools::stringSplit(value_pair, delimiter);
    if (string_value.size() != 2) {
        if (HID_DEBUG || string_value.size() > 2) {
            DEBUG_FUNCTION_LINE("Not a valid key=pair line %s", value_pair.c_str());
        }
        return -1;
    }
    if (string_value[0].compare(expectedKey) != 0) {
        DEBUG_FUNCTION_LINE("Key part not %s, its %s", expectedKey.c_str(), string_value[0].c_str());
        return -1;
    }
    char *ptr;
    int32_t value = strtol(string_value[1].c_str(), &ptr, 16);

    return value;
}
