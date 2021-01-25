//-----------------------------------------------------------------------------
//
//	ThermostatOperatingState.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_OPERATING_STATE
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
#include "command_classes/ThermostatOperatingState.h"
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

			enum ThermostatOperatingStateCmd
			{
				ThermostatOperatingStateCmd_Get = 0x02,
				ThermostatOperatingStateCmd_Report = 0x03
			};

			static char const* c_stateName[] =
			{ "Idle", "Heating", "Cooling", "Fan Only", "Pending Heat", "Pending Cool", "Vent / Economizer", "Aux Heating",
					"2nd Stage Heating", "2nd Stage Cooling", "2nd Stage Aux Heating", "3rd Stage Aux Heating" };

//-----------------------------------------------------------------------------
// <ThermostatOperatingState::RequestState>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
			bool ThermostatOperatingState::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (_requestFlags & RequestFlag_Dynamic)
				{
					return RequestValue(_requestFlags, 0, _instance, _queue);
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <ThermostatOperatingState::RequestValue>
// Get a thermostat mode value from the device
//-----------------------------------------------------------------------------
			bool ThermostatOperatingState::RequestValue(uint32 const _requestFlags, uint16 const _dummy1,	// = 0 (not used)
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				if (m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					Msg* msg = new Msg("ThermostatOperatingStateCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->SetInstance(this, _instance);
					msg->Append(GetNodeId());
					msg->Append(2);
					msg->Append(GetCommandClassId());
					msg->Append(ThermostatOperatingStateCmd_Get);
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					return true;
				}
				else
				{
					Log::Write(LogLevel_Info, GetNodeId(), "ThermostatOperatingStateCmd_Get Not Supported on this node");
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <ThermostatOperatingState::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool ThermostatOperatingState::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (ThermostatOperatingStateCmd_Report == (ThermostatOperatingStateCmd) _data[0])
				{
					// We have received the thermostat operating state from the Z-Wave device
					if (Internal::VC::ValueString* valueString = static_cast<Internal::VC::ValueString*>(GetValue(_instance, ValueID_Index_ThermostatOperatingState::OperatingState)))
					{
						uint8 index = _data[1] & 0x0f;
						std::string statename;
						if (index < (sizeof(c_stateName) / sizeof(*c_stateName)))
						{
							statename = c_stateName[index];
						}
						else
						{
							statename = "Unknown " + std::to_string(index);
						}
						valueString->OnValueRefreshed(statename);
						valueString->Release();
						Log::Write(LogLevel_Info, GetNodeId(), "Received thermostat operating state: %s", statename);
					}
					return true;
				}

				return false;
			}

//-----------------------------------------------------------------------------
// <ThermostatOperatingState::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void ThermostatOperatingState::CreateVars(uint8 const _instance)
			{
				if (Node* node = GetNodeUnsafe())
				{
					node->CreateValueString(ValueID::ValueGenre_User, GetCommandClassId(), _instance, ValueID_Index_ThermostatOperatingState::OperatingState, "Operating State", "", true, false, c_stateName[0], 0);
				}
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave

