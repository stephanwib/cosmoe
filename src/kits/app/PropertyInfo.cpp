//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		PropertyInfo.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Utility class for maintain scripting information.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <PropertyInfo.h>
#include <Message.h>
#include <Errors.h>
#include <ByteOrder.h>
#include <DataIO.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BPropertyInfo::BPropertyInfo(property_info *p, value_info *ci,
							 bool free_on_delete)
	:	fPropInfo(p),
		fValueInfo(ci),
		fPropCount(0),
		fInHeap(free_on_delete),
		fValueCount(0)
{
	if (fPropInfo)
	{
		while (fPropInfo[fPropCount].name)
			fPropCount++;
	}

	if (fValueInfo)
	{
		while (fValueInfo[fValueCount].name)
			fValueCount++;
	}
}
//------------------------------------------------------------------------------
BPropertyInfo::~BPropertyInfo()
{
	FreeMem();
}
//------------------------------------------------------------------------------
int32 BPropertyInfo::FindMatch(BMessage *msg, int32 index, BMessage *spec,
							   int32 form, const char *prop, void *data) const
{
	int32 property_index = 0;

	while ((fPropInfo != NULL) && (fPropInfo[property_index].name)) {
		property_info *propInfo = fPropInfo + property_index;

		if ((strcmp(propInfo->name, prop) == 0) &&
		    (FindCommand(msg->what, index, propInfo)) &&
		    (FindSpecifier(form, propInfo))) {
			if (data)
				*((uint32*)data) = propInfo->extra_data;
			return property_index;
		}
		property_index++;
	}

	return B_ERROR;
}
//------------------------------------------------------------------------------
bool BPropertyInfo::IsFixedSize() const
{
	return false;
}
//------------------------------------------------------------------------------
type_code BPropertyInfo::TypeCode() const
{
	return B_PROPERTY_INFO_TYPE;
}
//------------------------------------------------------------------------------
ssize_t BPropertyInfo::FlattenedSize() const
{
	size_t size = (2 * sizeof(int32)) + 1;

	if (fPropInfo)
	{
		// Main chunks
		for (int32 pi = 0; fPropInfo[pi].name != NULL; pi++)
		{
			size += strlen(fPropInfo[pi].name) + 1;

			if (fPropInfo[pi].usage)
				size += strlen(fPropInfo[pi].usage) + 1;
			else
				size += sizeof(char);

			size += sizeof(int32);

			for (int32 i = 0; i < 10 && fPropInfo[pi].commands[i] != 0; i++)
				size += sizeof(int32);
			size += sizeof(int32);

			for (int32 i = 0; i < 10 && fPropInfo[pi].specifiers[i] != 0; i++)
				size += sizeof(int32);
			size += sizeof(int32);
		}

		// Type chunks
		for (int32 pi = 0; fPropInfo[pi].name != NULL; pi++)
		{
			for (int32 i = 0; i < 10 && fPropInfo[pi].types[i] != 0; i++)
				size += sizeof(int32);
			size += sizeof(int32);

			for (int32 i = 0; i < 3 && fPropInfo[pi].ctypes[i].pairs[0].name != 0; i++)
			{
				for (int32 j = 0; j < 5 && fPropInfo[pi].ctypes[i].pairs[j].name != 0; j++)
				{
					size += strlen(fPropInfo[pi].ctypes[i].pairs[j].name) + 1;
					size += sizeof(int32);
				}
				size += sizeof(int32);
			}
			size += sizeof(int32);
		}
	}

	if (fValueInfo)
	{
		size += sizeof(int16);

		// Chunks
		for (int32 vi = 0; fValueInfo[vi].name != NULL; vi++)
		{
			size += sizeof(int32);
			size += sizeof(int32);

			size += strlen(fValueInfo[vi].name) + 1;

			if (fValueInfo[vi].usage)
				size += strlen(fValueInfo[vi].usage) + 1;
			else
				size += sizeof(char);

			size += sizeof(int32);
		}
	}

	return size;
}
//------------------------------------------------------------------------------
status_t BPropertyInfo::Flatten(void *buffer, ssize_t numBytes) const
{
	if (numBytes < FlattenedSize())
		return B_NO_MEMORY;

	if (buffer == NULL)
		return B_BAD_VALUE;

	BMemoryIO flatData(buffer, numBytes);
	
	char tmpChar = B_HOST_IS_BENDIAN;
	int32 tmpInt;
	
	flatData.Write(&tmpChar, sizeof(tmpChar));
	flatData.Write(&fPropCount, sizeof(fPropCount));
	tmpInt = 0x01 | (fValueInfo ? 0x2 : 0x0);
	flatData.Write(&tmpInt, sizeof(tmpInt));
	
	if (fPropInfo) { // Main chunks
		for (int32 pi = 0; fPropInfo[pi].name != NULL; pi++) {
			flatData.Write(fPropInfo[pi].name, strlen(fPropInfo[pi].name) + 1);
			if (fPropInfo[pi].usage != NULL) {
				flatData.Write(fPropInfo[pi].usage, strlen(fPropInfo[pi].usage) + 1);
			}
			else {
				tmpChar = 0;
				flatData.Write(&tmpChar, sizeof(tmpChar));
			}
			
			flatData.Write(&fPropInfo[pi].extra_data, sizeof(fPropInfo[pi].extra_data));

			for (int32 i = 0; i < 10 && fPropInfo[pi].commands[i] != 0; i++) {
				flatData.Write(&fPropInfo[pi].commands[i], sizeof(fPropInfo[pi].commands[i]));
			}
			tmpInt = 0;
			flatData.Write(&tmpInt, sizeof(tmpInt));
			
			for (int32 i = 0; i < 10 && fPropInfo[pi].specifiers[i] != 0; i++) {
				flatData.Write(&fPropInfo[pi].specifiers[i], sizeof(fPropInfo[pi].specifiers[i]));
			}
			tmpInt = 0;
			flatData.Write(&tmpInt, sizeof(tmpInt));
		}
		
		// Type chunks
		for (int32 pi = 0; fPropInfo[pi].name != NULL; pi++) {
			for (int32 i = 0; i < 10 && fPropInfo[pi].types[i] != 0; i++) {
				flatData.Write(&fPropInfo[pi].types[i], sizeof(fPropInfo[pi].types[i]));
			}
			tmpInt = 0;
			flatData.Write(&tmpInt, sizeof(tmpInt));

			for (int32 i = 0; i < 3 && fPropInfo[pi].ctypes[i].pairs[0].name != 0; i++) {
				for (int32 j = 0; j < 5 && fPropInfo[pi].ctypes[i].pairs[j].name != 0; j++) {
					flatData.Write(fPropInfo[pi].ctypes[i].pairs[j].name,
					               strlen(fPropInfo[pi].ctypes[i].pairs[j].name) + 1);
					flatData.Write(&fPropInfo[pi].ctypes[i].pairs[j].type,
					               sizeof(fPropInfo[pi].ctypes[i].pairs[j].type));
				}
				tmpInt = 0;
				flatData.Write(&tmpInt, sizeof(tmpInt));
			}
			tmpInt = 0;
			flatData.Write(&tmpInt, sizeof(tmpInt));
		}
	}
	
	if (fValueInfo) {
		// Value Chunks
		flatData.Write(&fValueCount, sizeof(fValueCount));
		for (int32 vi = 0; fValueInfo[vi].name != NULL; vi++) {
			flatData.Write(&fValueInfo[vi].kind, sizeof(fValueInfo[vi].kind));
			flatData.Write(&fValueInfo[vi].value, sizeof(fValueInfo[vi].value));
			flatData.Write(fValueInfo[vi].name, strlen(fValueInfo[vi].name) + 1);
			if (fValueInfo[vi].usage) {
				flatData.Write(fValueInfo[vi].usage, strlen(fValueInfo[vi].usage) + 1);
			}
			else {
				tmpChar = 0;
				flatData.Write(&tmpChar, sizeof(tmpChar));
			}
			flatData.Write(&fValueInfo[vi].extra_data, sizeof(fValueInfo[vi].extra_data));
		}
	}
	
	return B_OK;
}
//------------------------------------------------------------------------------
bool BPropertyInfo::AllowsTypeCode(type_code code) const
{
	return (code == B_PROPERTY_INFO_TYPE);
}
//------------------------------------------------------------------------------
status_t BPropertyInfo::Unflatten(type_code code, const void *buffer,
								  ssize_t numBytes)
{
	if (!AllowsTypeCode(code))
		return B_BAD_TYPE;

	if (buffer == NULL)
		return B_BAD_VALUE;

	FreeMem();
	
	BMemoryIO flatData(buffer, numBytes);
	char tmpChar = B_HOST_IS_BENDIAN;
	int32 tmpInt;

	flatData.Read(&tmpChar, sizeof(tmpChar));
	bool swapRequired = (tmpChar != B_HOST_IS_BENDIAN);
	
	flatData.Read(&fPropCount, sizeof(fPropCount));
	
	int32 flags;
	flatData.Read(&flags, sizeof(flags));
	if (swapRequired) {
		fPropCount = B_SWAP_INT32(fPropCount);
		flags = B_SWAP_INT32(flags);
	}
	
	if (flags & 1) {
		fPropInfo = static_cast<property_info *>(malloc(sizeof(property_info) * (fPropCount + 1)));
		memset(fPropInfo, 0, (fPropCount + 1) * sizeof(property_info));

		// Main chunks
		for (int32 pi = 0; pi < fPropCount; pi++) {
			fPropInfo[pi].name = strdup(static_cast<const char*>(buffer) + flatData.Position());
			flatData.Seek(strlen(fPropInfo[pi].name) + 1, SEEK_CUR);
			
			fPropInfo[pi].usage = strdup(static_cast<const char *>(buffer) + flatData.Position());
			flatData.Seek(strlen(fPropInfo[pi].usage) + 1, SEEK_CUR);

			flatData.Read(&fPropInfo[pi].extra_data, sizeof(fPropInfo[pi].extra_data));
			if (swapRequired) {
				fPropInfo[pi].extra_data = B_SWAP_INT32(fPropInfo[pi].extra_data);
			}

			flatData.Read(&tmpInt, sizeof(tmpInt));
			for (int32 i = 0; tmpInt != 0; i++) {
				if (swapRequired) {
					tmpInt = B_SWAP_INT32(tmpInt);
				}
				fPropInfo[pi].commands[i] = tmpInt;
				flatData.Read(&tmpInt, sizeof(tmpInt));
			}
			
			flatData.Read(&tmpInt, sizeof(tmpInt));
			for (int32 i = 0; tmpInt != 0; i++) {
				if (swapRequired) {
					tmpInt = B_SWAP_INT32(tmpInt);
				}
				fPropInfo[pi].specifiers[i] = tmpInt;
				flatData.Read(&tmpInt, sizeof(tmpInt));
			}
		}

		// Type chunks
		for (int32 pi = 0; pi < fPropCount; pi++) {
			flatData.Read(&tmpInt, sizeof(tmpInt));
			for (int32 i = 0; tmpInt != 0; i++) {
				if (swapRequired) {
					tmpInt = B_SWAP_INT32(tmpInt);
				}
				fPropInfo[pi].types[i] = tmpInt;
				flatData.Read(&tmpInt, sizeof(tmpInt));
			}

			flatData.Read(&tmpInt, sizeof(tmpInt));
			for (int32 i = 0; tmpInt != 0; i++) {
				for (int32 j = 0; tmpInt != 0; j++) {
					flatData.Seek(-sizeof(tmpInt), SEEK_CUR);
					fPropInfo[pi].ctypes[i].pairs[j].name =
									strdup(static_cast<const char *>(buffer) + flatData.Position());
					flatData.Seek(strlen(fPropInfo[pi].ctypes[i].pairs[j].name) + 1, SEEK_CUR);
									
					flatData.Read(&fPropInfo[pi].ctypes[i].pairs[j].type,
					              sizeof(fPropInfo[pi].ctypes[i].pairs[j].type));
					if (swapRequired) {
						fPropInfo[pi].ctypes[i].pairs[j].type =
										B_SWAP_INT32(fPropInfo[pi].ctypes[i].pairs[j].type);
					}
					flatData.Read(&tmpInt, sizeof(tmpInt));
				}
				flatData.Read(&tmpInt, sizeof(tmpInt));
			}
		}
	}

	if (flags & 2) {
		flatData.Read(&fValueCount, sizeof(fValueCount));
		if (swapRequired) {
			fValueCount = B_SWAP_INT16(fValueCount);
		}

		fValueInfo = static_cast<value_info *>(malloc(sizeof(value_info) * (fValueCount + 1)));
		memset(fValueInfo, 0, (fValueCount + 1) * sizeof(value_info));

		for (int32 vi = 0; vi < fValueCount; vi++)
		{
			flatData.Read(&fValueInfo[vi].kind, sizeof(fValueInfo[vi].kind));
			flatData.Read(&fValueInfo[vi].value, sizeof(fValueInfo[vi].value));

			fValueInfo[vi].name = strdup(static_cast<const char *>(buffer) + flatData.Position());
			flatData.Seek(strlen(fValueInfo[vi].name) + 1, SEEK_CUR);
			
			fValueInfo[vi].usage = strdup(static_cast<const char *>(buffer) + flatData.Position());
			flatData.Seek(strlen(fValueInfo[vi].usage) + 1, SEEK_CUR);
			
			flatData.Read(&fValueInfo[vi].extra_data, sizeof(fValueInfo[vi].extra_data));
			if (swapRequired) {
				fValueInfo[vi].kind = static_cast<value_kind>(B_SWAP_INT32(fValueInfo[vi].kind));
				fValueInfo[vi].value = B_SWAP_INT32(fValueInfo[vi].value);
				fValueInfo[vi].extra_data = B_SWAP_INT32(fValueInfo[vi].extra_data);
			}
		}
	}

	return B_OK;
}
//------------------------------------------------------------------------------
const property_info *BPropertyInfo::Properties() const
{
	return fPropInfo;
}
//------------------------------------------------------------------------------
const value_info *BPropertyInfo::Values() const
{
	return fValueInfo;
}
//------------------------------------------------------------------------------
int32 BPropertyInfo::CountProperties() const
{
	return fPropCount;
}
//------------------------------------------------------------------------------
int32 BPropertyInfo::CountValues() const
{
	return fValueCount;
}
//------------------------------------------------------------------------------
void BPropertyInfo::PrintToStream() const
{
	printf("      property   commands                       types                specifiers\n");
	printf("--------------------------------------------------------------------------------\n");

	for (int32 pi = 0; fPropInfo[pi].name != 0; pi++) {
		// property
		printf("%14s", fPropInfo[pi].name);
		// commands
		for (int32 i = 0; i < 10 && fPropInfo[pi].commands[i] != 0; i++) {
			uint32 command = fPropInfo[pi].commands[i];

			printf("   %c%c%c%-28c", int(command & 0xFF000000) >> 24,
				int(command & 0xFF0000) >> 16, int(command & 0xFF00) >> 8,
				int(command) & 0xFF);
		}
		// types
		for (int32 i = 0; i < 10 && fPropInfo[pi].types[i] != 0; i++) {
			uint32 type = fPropInfo[pi].types[i];

			printf("%c%c%c%c", int(type & 0xFF000000) >> 24,
				int(type & 0xFF0000) >> 16, int(type & 0xFF00) >> 8, (int)type & 0xFF);
		}
		// specifiers
		for (int32 i = 0; i < 10 && fPropInfo[pi].specifiers[i] != 0; i++) {
			uint32 spec = fPropInfo[pi].specifiers[i];

			printf("%lu", spec);
		}
		printf("\n");
	}
}
//------------------------------------------------------------------------------
bool BPropertyInfo::FindCommand(uint32 what, int32 index, property_info *p)
{
	bool result = false;
	
	if (p->commands[0] == 0) {
		result = true;
	} else if (index == 0) {
		for (int32 i = 0; i < 10 && p->commands[i] != 0; i++) {
			if (p->commands[i] == what) {
				result = true;
			}
		}
	}

	return(result);
}
//------------------------------------------------------------------------------
bool BPropertyInfo::FindSpecifier(uint32 form, property_info *p)
{
	bool result = false;
	
	if (p->specifiers[0] == 0) {
		result = true;
	} else {
		for (int32 i = 0; i < 10 && p->specifiers[i] != 0; i++) {
			if (p->specifiers[i] == form) {
				result = true;
			}
		}
	}

	return(result);
}
//------------------------------------------------------------------------------
void BPropertyInfo::_ReservedPropertyInfo1() {}
void BPropertyInfo::_ReservedPropertyInfo2() {}
void BPropertyInfo::_ReservedPropertyInfo3() {}
void BPropertyInfo::_ReservedPropertyInfo4() {}
//------------------------------------------------------------------------------
BPropertyInfo::BPropertyInfo(const BPropertyInfo &)
{
}
//------------------------------------------------------------------------------
BPropertyInfo &BPropertyInfo::operator=(const BPropertyInfo &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BPropertyInfo::FreeMem()
{
	int i, j, k;
	
	if (!fInHeap) {
		return;
	}
	
	if (fPropInfo != NULL) {
		for(i = 0; i < fPropCount; i++) {
			if (fPropInfo[i].name != NULL) {
				free(fPropInfo[i].name);
			}
			if (fPropInfo[i].usage != NULL) {
				free(fPropInfo[i].usage);
			}
			for(j = 0; j < 3; j++) {
				for(k = 0; k < 5; k++) {
					if (fPropInfo[i].ctypes[j].pairs[k].name == NULL) {
						break;
					} else {
						free(fPropInfo[i].ctypes[j].pairs[k].name);
					}
				}
				if (fPropInfo[i].ctypes[j].pairs[0].name == NULL) {
					break;
				}
			}
		}
		free(fPropInfo);
		fPropInfo = NULL;
		fPropCount = 0;
	}
	
	if (fValueInfo != NULL) {
		for(i = 0; i < fValueCount; i++) {
			if (fValueInfo[i].name != NULL) {
				free(fValueInfo[i].name);
			}
			if (fValueInfo[i].usage != NULL) {
				free(fValueInfo[i].usage);
			}
		}
		free(fValueInfo);
		fValueInfo = NULL;
		fValueCount = 0;
	}
	
	fInHeap = false;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
