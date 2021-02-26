//-----------------------------------------------------------------------------
//
//	ValueList.cpp
//
//	Represents a list of items
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

#include "tinyxml.h"
#include "value_classes/ValueList.h"
#include "Msg.h"
#include "platform/Log.h"
#include "Manager.h"
#include "Localization.h"
#include <ctime>

namespace OpenZWave
{
	namespace Internal
	{
		namespace VC
		{
			//-----------------------------------------------------------------------------
			// <ValueList::ValueList>
			// Constructor
			//-----------------------------------------------------------------------------
			ValueList::ValueList(uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _index, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, vector<Item> const& _items, int32 const _value, uint8 const _pollIntensity, uint8 const _size) :
				Value(_homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_Int, _label, _units, _readOnly, _writeOnly, false, _pollIntensity), m_value(_value), m_valueCheck(0), m_targetValue(0), m_items(_items), m_size(_size)
			{
				m_min = INT_MIN;
				m_max = INT_MAX;
			}

			//-----------------------------------------------------------------------------
			// <ValueList::ValueList>
			// Constructor (from XML)
			//-----------------------------------------------------------------------------
			ValueList::ValueList() :
				Value(), m_value(0), m_valueCheck(0), m_size(0)

			{
				m_min = INT_MIN;
				m_max = INT_MAX;
			}

			std::string const ValueList::GetAsString() const
			{
				stringstream ss;
				ss << GetValue();
				return ss.str();
			}

			bool ValueList::SetFromString(string const& _value)
			{
				int32 val = atoi(_value.c_str());
				return Set(val);
			}

			//-----------------------------------------------------------------------------
			// <ValueList::ReadXML>
			// Apply settings from XML
			//-----------------------------------------------------------------------------
			void ValueList::ReadXML(uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement)
			{
				Value::ReadXML(_homeId, _nodeId, _commandClassId, _valueElement);

				// Get value
				int intVal;
				if (TIXML_SUCCESS != _valueElement->QueryIntAttribute("value", &intVal))
				{
					Log::Write(LogLevel_Info, "Missing default integer value from xml configuration: node %d, class 0x%02x, instance %d, index %d", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex());
					intVal = 0;
				}
				m_value = (int32)intVal;

				// Get size of values
				int intSize;
				if (TIXML_SUCCESS != _valueElement->QueryIntAttribute("size", &intSize))
				{
					Log::Write(LogLevel_Warning, "Value list size is not set, assuming 4 bytes for node %d, class 0x%02x, instance %d, index %d - %s", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex(), GetID().GetAsString().c_str());
					intSize = 4;
				}

				if (intSize != 1 && intSize != 2 && intSize != 4)
				{
					Log::Write(LogLevel_Warning, "Value size is invalid (%d). Only 1, 2 & 4 supported for node %d, class 0x%02x, instance %d, index %d - %s", intSize, _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex(), GetID().GetAsString().c_str());
					intSize = 4;
				}
				m_size = intSize;


				TiXmlElement const* itemElement = _valueElement->FirstChildElement();

				bool shouldclearlist = true;
				while (itemElement)
				{
					char const* str = itemElement->Value();
					if (str && !strcmp(str, "Item"))
					{
						/* clear the existing list, if we have Item entries. (static list entries are created in the constructor
						 * here, we load up any localized labels
						 */
						if (shouldclearlist)
						{
							m_items.clear();
							shouldclearlist = false;
						}

						bool AddItem = true;
						char const* labelStr = itemElement->Attribute("label");
						char const* lang = "";
						if (itemElement->Attribute("lang"))
						{
							lang = itemElement->Attribute("lang");
							AddItem = false;
						}
						else
						{
							AddItem = true;
						}
						int value = 0;
						if (itemElement->QueryIntAttribute("value", &value) != TIXML_SUCCESS)
						{
							Log::Write(LogLevel_Warning, "Item value %s is wrong type or does not exist in xml configuration for node %d, class 0x%02x, instance %d, index %d - %s", labelStr, _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex(), GetID().GetAsString().c_str());
						}
						else if ((m_size == 1 && value > 255) || (m_size == 2 && value > 65535))
						{
							Log::Write(LogLevel_Warning, "Item value %s is incorrect size in xml configuration for node %d, class 0x%02x, instance %d, index %d - %s", labelStr, _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex(), GetID().GetAsString().c_str());
						}
						else
						{
							Localization::Get()->SetValueItemLabel(m_id.GetNodeId(), m_id.GetCommandClassId(), m_id.GetIndex(), -1, value, labelStr, lang);
							if (AddItem)
							{
								Item item;
								item.m_label = labelStr;
								item.m_value = value;
								m_items.push_back(item);
							}
						}
					}

					itemElement = itemElement->NextSiblingElement();
				}
				/* setup any Localization now as we should have read all available languages already */
				for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
				{
					it->m_label = Localization::Get()->GetValueItemLabel(m_id.GetNodeId(), m_id.GetCommandClassId(), m_id.GetIndex(), -1, it->m_value);
				}
			}

			//-----------------------------------------------------------------------------
			// <ValueList::WriteXML>
			// Write ourselves to an XML document
			//-----------------------------------------------------------------------------
			void ValueList::WriteXML(TiXmlElement* _valueElement)
			{
				Value::WriteXML(_valueElement);

				char str[16];
				snprintf(str, sizeof(str), "%d", m_value);
				_valueElement->SetAttribute("value", str);

				snprintf(str, sizeof(str), "%d", m_size);
				_valueElement->SetAttribute("size", str);

				for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
				{
					TiXmlElement* pItemElement = new TiXmlElement("Item");
					pItemElement->SetAttribute("label", (*it).m_label.c_str());

					snprintf(str, sizeof(str), "%d", (*it).m_value);
					pItemElement->SetAttribute("value", str);

					_valueElement->LinkEndChild(pItemElement);
				}
			}

			//-----------------------------------------------------------------------------
			// <ValueList::Set>
			// Set a new value in the device
			//-----------------------------------------------------------------------------
			bool ValueList::Set(int32 const _value)
			{
				// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
				ValueList* tempValue = new ValueList(*this);
				tempValue->m_value = _value;

				// Set the value in the device.
				bool ret = ((Value*)tempValue)->Set();

				// clean up the temporary value
				delete tempValue;

				return ret;
			}

			//-----------------------------------------------------------------------------
			// <ValueList::SetTargetValue>
			// Set the Value Target (Used for Automatic Refresh)
			//-----------------------------------------------------------------------------
			void ValueList::SetTargetValue(int32 const _target, uint32 _duration)
			{
				m_targetValueSet = true;
				m_targetValue = _target;
				m_duration = _duration;
			}

			//-----------------------------------------------------------------------------
			// <ValueList::OnValueRefreshed>
			// A value in a device has been refreshed
			//-----------------------------------------------------------------------------
			void ValueList::OnValueRefreshed(int32 const _value)
			{
				switch (VerifyRefreshedValue((void*)&m_value, (void*)&m_valueCheck, (void*)&_value, (void*)&m_targetValue, ValueID::ValueType_List))
				{
				case 0:		// value hasn't changed, nothing to do
					break;
				case 1:		// value has changed (not confirmed yet), save _value in m_valueCheck
					m_valueCheck = _value;
					break;
				case 2:		// value has changed (confirmed), save _value in m_value
					m_value = _value;
					break;
				case 3:		// all three values are different, so wait for next refresh to try again
					break;
				}
			}


///////////////////////////////////////////////////////////////////////////////




////-----------------------------------------------------------------------------
//// <ValueList::ValueList>
//// Constructor
////-----------------------------------------------------------------------------
//			ValueList::ValueList(uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _index, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, vector<Item> const& _items, int32 const _valueIdx, uint8 const _pollIntensity, uint8 const _size	// = 4
//					) :
//					Value(_homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_List, _label, _units, _readOnly, _writeOnly, false, _pollIntensity), m_items(_items), m_valueIdx(_valueIdx), m_valueIdxCheck(0), m_size(_size), m_targetValue(0)
//			{
//				for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
//				{
//					/* first what is currently in m_label is the default text for a Item, so set it */
//					Localization::Get()->SetValueItemLabel(m_id.GetNodeId(), _commandClassId, _index, -1, it->m_value, it->m_label, "");
//					/* now set to the Localized Value */
//					it->m_label = Localization::Get()->GetValueItemLabel(m_id.GetNodeId(), _commandClassId, _index, -1, it->m_value);
//				}
//			}
//
////-----------------------------------------------------------------------------
//// <ValueList::ValueList>
//// Constructor
////-----------------------------------------------------------------------------
//			ValueList::ValueList() :
//					Value(), m_items(), m_valueIdx(), m_valueIdxCheck(0), m_size(0)
//			{
//
//			}
//
////-----------------------------------------------------------------------------
//// <ValueList::ReadXML>
//// Apply settings from XML
////-----------------------------------------------------------------------------
//			void ValueList::ReadXML(uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement)
//			{
//				Value::ReadXML(_homeId, _nodeId, _commandClassId, _valueElement);
//
//				// Get size of values
//				int intSize;
//				if (TIXML_SUCCESS == _valueElement->QueryIntAttribute("size", &intSize))
//				{
//					if (intSize == 1 || intSize == 2 || intSize == 4)
//					{
//						m_size = intSize;
//					}
//					else
//					{
//						Log::Write(LogLevel_Warning, "Value size is invalid (%d). Only 1, 2 & 4 supported for node %d, class 0x%02x, instance %d, index %d - %s", intSize, _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex(), GetID().GetAsString().c_str());
//					}
//				}
//				else
//				{
//					Log::Write(LogLevel_Warning, "Value list size is not set, assuming 4 bytes for node %d, class 0x%02x, instance %d, index %d - %s", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex(), GetID().GetAsString().c_str());
//				}
//
//				TiXmlElement const* itemElement = _valueElement->FirstChildElement();
//
//				bool shouldclearlist = true;
//				while (itemElement)
//				{
//					char const* str = itemElement->Value();
//					if (str && !strcmp(str, "Item"))
//					{
//						/* clear the existing list, if we have Item entries. (static list entries are created in the constructor
//						 * here, we load up any localized labels
//						 */
//						if (shouldclearlist)
//						{
//							m_items.clear();
//							shouldclearlist = false;
//						}
//
//						bool AddItem = true;
//						char const* labelStr = itemElement->Attribute("label");
//						char const* lang = "";
//						if (itemElement->Attribute("lang"))
//						{
//							lang = itemElement->Attribute("lang");
//							AddItem = false;
//						}
//						else
//						{
//							AddItem = true;
//						}
//						int value = 0;
//						if (itemElement->QueryIntAttribute("value", &value) != TIXML_SUCCESS)
//						{
//							Log::Write(LogLevel_Warning, "Item value %s is wrong type or does not exist in xml configuration for node %d, class 0x%02x, instance %d, index %d - %s", labelStr, _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex(), GetID().GetAsString().c_str());
//						}
//						else if ((m_size == 1 && value > 255) || (m_size == 2 && value > 65535))
//						{
//							Log::Write(LogLevel_Warning, "Item value %s is incorrect size in xml configuration for node %d, class 0x%02x, instance %d, index %d - %s", labelStr, _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex(), GetID().GetAsString().c_str());
//						}
//						else
//						{
//							Localization::Get()->SetValueItemLabel(m_id.GetNodeId(), m_id.GetCommandClassId(), m_id.GetIndex(), -1, value, labelStr, lang);
//							if (AddItem)
//							{
//								Item item;
//								item.m_label = labelStr;
//								item.m_value = value;
//								m_items.push_back(item);
//							}
//						}
//					}
//
//					itemElement = itemElement->NextSiblingElement();
//				}
//				/* setup any Localization now as we should have read all available languages already */
//				for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
//				{
//					it->m_label = Localization::Get()->GetValueItemLabel(m_id.GetNodeId(), m_id.GetCommandClassId(), m_id.GetIndex(), -1, it->m_value);
//				}
//
//				// Set the value
//				bool valSet = false;
//				int intVal;
//				m_valueIdx = 0;
//				if (TIXML_SUCCESS == _valueElement->QueryIntAttribute("value", &intVal))
//				{
//					valSet = true;
//					intVal = GetItemIdxByValue(intVal);
//					if (intVal != -1)
//					{
//						m_valueIdx = (int32) intVal;
//					}
//					else
//					{
//						Log::Write(LogLevel_Warning, "Value is not found in xml configuration for node %d, class 0x%02x, instance %d, index %d - %s", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex(), GetID().GetAsString().c_str());
//					}
//				}
//
//				// Set the index
//				bool indSet = false;
//				int intInd = 0;
//				if (TIXML_SUCCESS == _valueElement->QueryIntAttribute("vindex", &intInd))
//				{
//					indSet = true;
//					if (intInd >= 0 && intInd < (int32) m_items.size())
//					{
//						m_valueIdx = (int32) intInd;
//					}
//					else
//					{
//						Log::Write(LogLevel_Warning, "Vindex is out of range for index in xml configuration for node %d, class 0x%02x, instance %d, index %d - %s", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex(), GetID().GetAsString().c_str());
//					}
//				}
//				if (!valSet && !indSet)
//				{
//					Log::Write(LogLevel_Warning, "Missing default list value or vindex from xml configuration: node %d, class 0x%02x, instance %d, index %d - %s", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex(), GetID().GetAsString().c_str());
//				}
//
//			}
//
////-----------------------------------------------------------------------------
//// <ValueList::WriteXML>
//// Write ourselves to an XML document
////-----------------------------------------------------------------------------
//			void ValueList::WriteXML(TiXmlElement* _valueElement)
//			{
//				Value::WriteXML(_valueElement);
//
//				char str[16];
//				snprintf(str, sizeof(str), "%d", m_valueIdx);
//				_valueElement->SetAttribute("vindex", str);
//
//				snprintf(str, sizeof(str), "%d", m_size);
//				_valueElement->SetAttribute("size", str);
//
//				for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
//				{
//					TiXmlElement* pItemElement = new TiXmlElement("Item");
//					pItemElement->SetAttribute("label", (*it).m_label.c_str());
//
//					snprintf(str, sizeof(str), "%d", (*it).m_value);
//					pItemElement->SetAttribute("value", str);
//
//					_valueElement->LinkEndChild(pItemElement);
//				}
//			}
//
//-----------------------------------------------------------------------------
// <ValueList::SetByValue>
// Set a new value in the device, selected by item index
//-----------------------------------------------------------------------------
			bool ValueList::SetByValue(uint32 const _index)
			{
				bool ret = false;

				// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
				ValueList* tempValue = new ValueList(*this);
				if (_index < m_items.size())
				{
					tempValue->m_value = m_items[_index].m_value;
					// Set the value in the device.
					ret = ((Value*)tempValue)->Set();
				}

				// clean up the temporary value
				delete tempValue;

				return ret;
			}

//-----------------------------------------------------------------------------
// <ValueList::SetByLabel>
// Set a new value in the device, selected by item label
//-----------------------------------------------------------------------------
			bool ValueList::SetByLabel(string const& _label)
			{
				for (uint32 i = 0; i < m_items.size(); ++i)
				{
					if (_label == m_items[i].m_label)
					{
						return Set(m_items[i].m_value);
					}
				}
				Log::Write(LogLevel_Warning, "Attempt to set an invalid Label %s in SetByLabel %s", _label.c_str(), GetID().GetAsString().c_str());
				return false;
			}

////-----------------------------------------------------------------------------
//// <ValueByte::SetTargetValue>
//// Set the Value Target (Used for Automatic Refresh)
////-----------------------------------------------------------------------------
//			void ValueList::SetTargetValue(int32 const _target, uint32 _duration)
//			{
//				m_targetValueSet = true;
//				m_targetValue = _target;
//				m_duration = _duration;
//			}

////-----------------------------------------------------------------------------
//// <ValueList::OnValueRefreshed>
//// A value in a device has been refreshed
////-----------------------------------------------------------------------------
//			void ValueList::OnValueRefreshed(int32 const _value)
//			{
//				switch (VerifyRefreshedValue((void*) &m_value, (void*) &m_valueCheck, (void*) &_value, (void*) &m_targetValue, ValueID::ValueType_List))
//				{
//					case 0:		// value hasn't changed, nothing to do
//						break;
//					case 1:		// value has changed (not confirmed yet), save _value in m_valueCheck
//						m_valueCheck = _value;
//						break;
//					case 2:		// value has changed (confirmed), save _value in m_value
//						m_value = _value;
//						break;
//					case 3:		// all three values are different, so wait for next refresh to try again
//						break;
//				}
//			}

//-----------------------------------------------------------------------------
// <ValueList::GetItemIdxByLabel>
// Get the index of an item from its label
//-----------------------------------------------------------------------------
			uint32 ValueList::GetItemIdxByLabel(string const& _label) const
			{
				for (uint32 i = 0; i < m_items.size(); ++i)
				{
					if (_label == m_items[i].m_label)
					{
						return i;
					}
				}
				Log::Write(LogLevel_Warning, "Attempt to get a Invalid Label %s from ValueList %s", _label.c_str(), GetID().GetAsString().c_str());
				return -1;
			}

//-----------------------------------------------------------------------------
// <ValueList::GetItemIdxByValue>
// Get the index of an item from its value
//-----------------------------------------------------------------------------
			uint32 ValueList::GetItemIdxByValue(int32 const _value) const
			{
				for (uint32 i = 0; i < m_items.size(); ++i)
				{
					if (_value == m_items[i].m_value)
					{
						return i;
					}
				}
				Log::Write(LogLevel_Warning, "Attempt to get a Invalid Index %d on ValueList %s", _value, GetID().GetAsString().c_str());
				return -1;
			}

//-----------------------------------------------------------------------------
// <ValueList::GetItemLabels>
// Fill a vector with the item labels
//-----------------------------------------------------------------------------
			bool ValueList::GetItemLabels(vector<string>* o_items)
			{
				if (o_items)
				{
					for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
					{
						o_items->push_back((*it).m_label);
					}

					return true;
				}
				Log::Write(LogLevel_Error, "o_items passed to ValueList::GetItemLabels is null: %s", GetID().GetAsString().c_str());
				return false;
			}

//-----------------------------------------------------------------------------
// <ValueList::GetItemValues>
// Fill a vector with the item values
//-----------------------------------------------------------------------------
			bool ValueList::GetItemValues(vector<int32>* o_values)
			{
				if (o_values)
				{
					for (vector<Item>::iterator it = m_items.begin(); it != m_items.end(); ++it)
					{
						o_values->push_back((*it).m_value);
					}

					return true;
				}
				Log::Write(LogLevel_Error, "o_values passed to ValueList::GetItemLabels is null: %s", GetID().GetAsString().c_str());
				return false;
			}

//-----------------------------------------------------------------------------
// <ValueList::GetItem>
// Get the Item at the Currently selected Index
//-----------------------------------------------------------------------------
			ValueList::Item const *ValueList::GetItem() const
			{
				for (uint32 i = 0; i < m_items.size(); ++i)
				{
					if (m_value == m_items[i].m_value)
					{
						return &m_items[i];
					}
				}
				Log::Write(LogLevel_Warning, "Invalid m_value Set on ValueList %.", GetID().GetAsString().c_str());
				return NULL;
			}
		} // namespace VC
	} // namespace Internal
} // namespace OpenZWave
