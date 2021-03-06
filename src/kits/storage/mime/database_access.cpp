//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file database_access.cpp
	Mime database atomic read functions
*/

#include <Bitmap.h>
#include <Entry.h>
#include <Directory.h>
#include <Message.h>
#include <mime/database_support.h>
#include <Node.h>
#include <Path.h>
#include <RegistrarDefs.h>
#include <String.h>
#include <storage_support.h>

#include <fs_attr.h>	// For struct attr_info
#include <iostream>
#include <new>			// For new(nothrow)
#include <stdio.h>
#include <string>

#include "mime/database_access.h"

#define DBG(x) x
//#define DBG(x)
#define OUT printf

namespace BPrivate {
namespace Storage {
namespace Mime {

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// get_app_hint
//!	Fetches the application hint for the given MIME type.
/*!	The entry_ref pointed to by \c ref must be pre-allocated.
	
	\param type The MIME type of interest
	\param ref Pointer to a pre-allocated \c entry_ref struct into
	           which the location of the hint application is copied.

	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No app hint exists for the given type
	- "error code": Failure
*/
status_t
get_app_hint(const char *type, entry_ref *ref)
{
	char path[B_MIME_TYPE_LENGTH];
	BEntry entry;
	ssize_t err = ref ? B_OK : B_BAD_VALUE;
	if (!err)
		err = read_mime_attr(type, kAppHintAttr, path, B_MIME_TYPE_LENGTH, kAppHintType);
	if (err >= 0)
		err = entry.SetTo(path);
	if (!err)
		err = entry.GetRef(ref);		
	return err;
}

// get_attr_info
/*! \brief Fetches from the MIME database a BMessage describing the attributes
	typically associated with files of the given MIME type
	
	The attribute information is returned in a pre-allocated BMessage pointed to by
	the \c info parameter (note that the any prior contents of the message
	will be destroyed). Please see BMimeType::SetAttrInfo() for a description
	of the expected format of such a message.
		
	\param info Pointer to a pre-allocated BMessage into which information about
	            the MIME type's associated file attributes is stored.
	\return
	- \c B_OK: Success
	- "error code": Failure
*/
status_t
get_attr_info(const char *type, BMessage *info)
{
	status_t err = read_mime_attr_message(type, kAttrInfoAttr, info);
	if (!err) {
		info->what = 233;	// Don't know why, but that's what R5 does.
		err = info->AddString("type", type);
	}
	return err;	
}

// get_short_description
//!	Fetches the short description for the given MIME type.
/*!	The string pointed to by \c description must be long enough to
	hold the short description; a length of \c B_MIME_TYPE_LENGTH is
	recommended.
	
	\param type The MIME type of interest
	\param description Pointer to a pre-allocated string into which the short
	                   description is copied. If the function fails, the contents
	                   of the string are undefined.

	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No short description exists for the given type
	- "error code": Failure
*/
status_t
get_short_description(const char *type, char *description)
{
///	DBG(OUT("Mime::Database::get_short_description()\n"));
	ssize_t err = read_mime_attr(type, kShortDescriptionAttr, description, B_MIME_TYPE_LENGTH, kShortDescriptionType);
	return err >= 0 ? B_OK : err ;
}

// get_long_description
//!	Fetches the long description for the given MIME type.
/*!	The string pointed to by \c description must be long enough to
	hold the long description; a length of \c B_MIME_TYPE_LENGTH is
	recommended.
	
	\param type The MIME type of interest
	\param description Pointer to a pre-allocated string into which the long
	                   description is copied. If the function fails, the contents
	                   of the string are undefined.

	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No long description exists for the given type
	- "error code": Failure
*/
status_t
get_long_description(const char *type, char *description)
{
//	DBG(OUT("Mime::Database::get_long_description()\n"));
	ssize_t err = read_mime_attr(type, kLongDescriptionAttr, description, B_MIME_TYPE_LENGTH, kLongDescriptionType);
	return err >= 0 ? B_OK : err ;
}

// get_file_extensions
//! Fetches a BMessage describing the MIME type's associated filename extensions
/*! The list of extensions is returned in a pre-allocated BMessage pointed to by
	the \c extensions parameter (note that the any prior contents of the message
	will be destroyed). Please see BMimeType::GetFileExtensions() for a description
	of the message format.
	  
	\param extensions Pointer to a pre-allocated BMessage into which the MIME
	                  type's associated file extensions will be stored.
	\return
	- \c B_OK: Success
	- "error code": Failure
*/
status_t
get_file_extensions(const char *type, BMessage *extensions)
{
	status_t err = read_mime_attr_message(type, kFileExtensionsAttr, extensions);
	if (!err) {
		extensions->what = 234;	// Don't know why, but that's what R5 does.
		err = extensions->AddString("type", type);
	}
	return err;	
}

// get_icon
//! Fetches the icon of given size associated with the given MIME type
/*! The bitmap pointed to by \c icon must be of the proper size (\c 32x32
	for \c B_LARGE_ICON, \c 16x16 for \c B_MINI_ICON) and color depth
	(\c B_CMAP8).
	
	\param type The mime type
	\param icon Pointer to a pre-allocated bitmap of proper dimensions and color depth
	\param size The size icon you're interested in (\c B_LARGE_ICON or \c B_MINI_ICON)
*/
status_t
get_icon(const char *type, BBitmap *icon, icon_size which)
{
	return get_icon_for_type(type, NULL, icon, which);
}

// get_icon_for_type
/*! \brief Fetches the large or mini icon used by an application of this type for files of the
	given type.
	
	The type of the \c BMimeType object is not required to actually be a subtype of
	\c "application/"; that is the intended use however, and calling \c get_icon_for_type()
	on a non-application type will likely return \c B_ENTRY_NOT_FOUND.
	
	The icon is copied into the \c BBitmap pointed to by \c icon. The bitmap must
	be the proper size: \c 32x32 for the large icon, \c 16x16 for the mini icon.	
	
	\param type The MIME type
	\param type Pointer to a pre-allocated string containing the MIME type whose
	            custom icon you wish to fetch. If NULL, works just like get_icon().
	\param icon Pointer to a pre-allocated \c BBitmap of proper size and colorspace into
				which the icon is copied.
	\param icon_size Value that specifies which icon to return. Currently \c B_LARGE_ICON
					 and \c B_MINI_ICON are supported.
	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No icon of the given size exists for the given type
	- "error code": Failure	

*/
status_t
get_icon_for_type(const char *type, const char *fileType, BBitmap *icon,
	  						   icon_size which)
{
	std::string attr;		
	attr_info info;
	ssize_t err;
	BNode node;
	uint32 attrType = 0;
	ssize_t attrSize = 0;
	BRect bounds;

	err = type && icon ? B_OK : B_BAD_VALUE;
	
	// Figure out what kind of data we *should* find
	if (!err) {
		switch (which) {
			case B_MINI_ICON:
				bounds.Set(0, 0, 15, 15);
				attrType = kMiniIconType;
				attrSize = 16 * 16;
				break;
			case B_LARGE_ICON:
				bounds.Set(0, 0, 31, 31);
				attrType = kLargeIconType;
				attrSize = 32 * 32;
				break;
			default:
				err = B_BAD_VALUE;
				break;
		}
	}
	// Construct our attribute name
	if (fileType) {
		attr = (which == B_MINI_ICON
	              ? kMiniIconAttrPrefix
	                : kLargeIconAttrPrefix)
	                  + BPrivate::Storage::to_lower(fileType);
	} else {
		attr = (which == B_MINI_ICON) ? kMiniIconAttr : kLargeIconAttr;
	}
	// Check the icon and attribute to see if they match
	if (!err)
		err = (icon->InitCheck() == B_OK && icon->Bounds() == bounds) ? B_OK : B_BAD_VALUE;
	if (!err) 
		err = open_type(type, &node);
	if (!err) 
		err = node.GetAttrInfo(attr.c_str(), &info);
	if (!err)
		err = (attrType == info.type && attrSize == info.size) ? B_OK : B_BAD_VALUE;
	// read the attribute
	if (!err) {
		bool otherColorSpace = (icon->ColorSpace() != B_CMAP8);
		char *buffer = NULL;
		if (otherColorSpace) {
			// other color space than stored in attribute
			buffer = new(std::nothrow) char[attrSize];
			if (!buffer)
				err = B_NO_MEMORY;
			if (!err) 
				err = node.ReadAttr(attr.c_str(), attrType, 0, buffer, attrSize);			
		} else {
			// same color space, just read direct
			err = node.ReadAttr(attr.c_str(), attrType, 0, icon->Bits(), attrSize);
		}
		if (err >= 0)
			err = (err == attrSize) ? (status_t)B_OK : (status_t)B_FILE_ERROR;
		if (otherColorSpace) {
			if (!err) {
				err = icon->ImportBits(buffer, attrSize, B_ANY_BYTES_PER_ROW,
									   0, B_CMAP8);
			}
			delete[] buffer;
		}
	}

	return err;
}

// get_preferred_app
//!	Fetches signature of the MIME type's preferred application for the given action.
/*!	The string pointed to by \c signature must be long enough to
	hold the short description; a length of \c B_MIME_TYPE_LENGTH is
	recommended.
	
	Currently, the only supported app verb is \c B_OPEN.
	
	\param type The MIME type of interest
	\param description Pointer to a pre-allocated string into which the preferred
	                   application's signature is copied. If the function fails, the
	                   contents of the string are undefined.
	\param verb \c The action of interest

	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No such preferred application exists
	- "error code": Failure
*/
status_t
get_preferred_app(const char *type, char *signature, app_verb verb = B_OPEN)
{
	// Since B_OPEN is the currently the only app_verb, it is essentially ignored
	ssize_t err = read_mime_attr(type, kPreferredAppAttr, signature, B_MIME_TYPE_LENGTH, kPreferredAppType);
	return err >= 0 ? B_OK : err ;
}

// get_sniffer_rule
/*! \brief Fetches the sniffer rule for the given MIME type.
	\param type The MIME type of interest
	\param result Pointer to a pre-allocated BString into which the type's
	              sniffer rule is copied.
	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No such preferred application exists
	- "error code": Failure
*/
status_t
get_sniffer_rule(const char *type, BString *result)
{
	return read_mime_attr_string(type, kSnifferRuleAttr, result);
}

// get_supported_types
status_t
get_supported_types(const char *type, BMessage *types)
{
	status_t err = read_mime_attr_message(type, kSupportedTypesAttr, types);
	if (!err) {
		types->what = 0;	
		err = types->AddString("type", type);
	}
	return err;	
}

// is_installed
//! Checks if the given MIME type is present in the database
bool
is_installed(const char *type)
{
	BNode node;
	return open_type(type, &node) == B_OK;
}

// get_icon_data
/*! \brief Returns properly formatted raw bitmap data, ready to be shipped off to the hacked
	up 4-parameter version of Database::SetIcon()
	
	This function exists as something of a hack until an OBOS::BBitmap implementation is
	available. It takes the given bitmap, converts it to the B_CMAP8 color space if necessary
	and able, and returns said bitmap data in a newly allocated array pointed to by the
	pointer that's pointed to by \c data. The length of the array is stored in the integer
	pointed to by \c dataSize. The array is allocated with \c new[], and it's your responsibility
	to \c delete[] it when you're finished.
	
*/
status_t
get_icon_data(const BBitmap *icon, icon_size which, void **data, int32 *dataSize)
{
	ssize_t err = (icon && data && dataSize && icon->InitCheck() == B_OK) ? B_OK : B_BAD_VALUE;

	BRect bounds;	
	BBitmap *icon8 = NULL;
	void *srcData = NULL;
	bool otherColorSpace = false;

	// Figure out what kind of data we *should* have
	if (!err) {
		switch (which) {
			case B_MINI_ICON:
				bounds.Set(0, 0, 15, 15);
				break;
			case B_LARGE_ICON:
				bounds.Set(0, 0, 31, 31);
				break;
			default:
				err = B_BAD_VALUE;
				break;
		}
	}
	// Check the icon
	if (!err)
		err = (icon->Bounds() == bounds) ? B_OK : B_BAD_VALUE;
	// Convert to B_CMAP8 if necessary
	if (!err) {
		otherColorSpace = (icon->ColorSpace() != B_CMAP8);
		if (otherColorSpace) {
			icon8 = new(std::nothrow) BBitmap(bounds, B_CMAP8);
			if (!icon8)
				err = B_NO_MEMORY;
			if (!err)
				err = icon8->ImportBits(icon);
			if (!err) {
				srcData = icon8->Bits();
				*dataSize = icon8->BitsLength();
			}
		} else {
			srcData = icon->Bits();
			*dataSize = icon->BitsLength();
		}		
	}
	// Alloc a new data buffer
	if (!err) {
		*data = new(std::nothrow) char[*dataSize];
		if (!*data)
			err = B_NO_MEMORY;
	}
	// Copy the data into it.
	if (!err)
		memcpy(*data, srcData, *dataSize);	
	if (otherColorSpace)
		delete icon8;
	return err;	
}

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

