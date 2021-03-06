//-----------------------------------------------------------------------------
//
//	ThermostatFanState.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_FAN_STATE
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "command_classes/CommandClasses.h"
#include "command_classes/ThermostatFanState.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueString.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum ThermostatFanStateCmd
			{
				ThermostatFanStateCmd_Get = 0x02,
				ThermostatFanStateCmd_Report = 0x03
			};

			static char const* c_stateName[] =
			{ "Idle", "Running", "Running High", "Running Medium", "Circulation Mode", "Humidity Circulation Mode", "Right - Left Circulation Mode", "Up - Down Circulation Mode", "Quiet Circulation Mode" };

//-----------------------------------------------------------------------------
// <ThermostatFanState::RequestState>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
			bool ThermostatFanState::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (_requestFlags & RequestFlag_Dynamic)
				{
					// Request the current state
					return RequestValue(_requestFlags, 0, _instance, _queue);
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <ThermostatFanState::RequestValue>
// Get the thermostat fan state details from the device
//-----------------------------------------------------------------------------
			bool ThermostatFanState::RequestValue(uint32 const _requestFlags, uint16 const _dummy1,	// = 0 (not used)
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					// Request the current state
					Msg* msg = new Msg("ThermostatFanStateCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->SetInstance(this, _instance);
					msg->Append(GetNodeId());
					msg->Append(2);
					msg->Append(GetCommandClassId());
					msg->Append(ThermostatFanStateCmd_Get);
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					return true;
				}
				else
				{
					Log::Write(LogLevel_Info, GetNodeId(), "ThermostatFanStateCmd_Get Not Supported on this node");
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <ThermostatFanState::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool ThermostatFanState::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (ThermostatFanStateCmd_Report == (ThermostatFanStateCmd) _data[0])
				{
					// We have received the thermostat fan state from the Z-Wave device
					if (Internal::VC::ValueString* valueString = static_cast<Internal::VC::ValueString*>(GetValue(_instance, ValueID_Index_ThermostatFanState::FanState)))
					{
						uint8 state = (_data[1] & 0x0f);

						std::string statename;

						if (state < (sizeof(c_stateName) / sizeof(*c_stateName)))
						{
							statename = c_stateName[state];
						}
						else
						{
							statename = "Unknown " + std::to_string(state);
						}
						valueString->OnValueRefreshed(statename);
						valueString->Release();
						Log::Write(LogLevel_Info, GetNodeId(), "Received thermostat fan state: %s", statename.c_str());
					}
					return true;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <ThermostatFanState::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void ThermostatFanState::CreateVars(uint8 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					node->CreateValueString(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_ThermostatFanState::FanState, "Fan State", "", true, false, c_stateName[0], 0);
				}
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave

