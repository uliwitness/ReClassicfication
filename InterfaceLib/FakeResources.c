//
//  FakeResources.c
//  ReClassicfication
//
//  Created by Uli Kusterer on 20.02.13.
//  Copyright (c) 2013 Uli Kusterer. All rights reserved.
//

#include <stdint.h>
#include <string.h>	// for memmove().
#include "FakeResources.h"
#include "EndianStuff.h"


/*
	resource data offset									  4 bytes
	resource map offset										  4 bytes
	resource data length									  4 bytes
	resource map length										  4 bytes
	Reserved for system use									112 bytes
	Application data										128 bytes
	resource data											...
	resource map											...


	Resource data is 4-byte-long-counted, followed by actual data.
	
	
	Resource map:
	
	copy of resource header	in RAM							 16 bytes
	next resource map in RAM								  4 bytes
	file reference number in RAM							  2 bytes
	Resource file attributes								  2 bytes
	type list offset (resource map-relative)				  2 bytes
	name list offset (resource map-relative)				  2 bytes
*/

struct FakeResourceMap
{
	struct FakeResourceMap*			nextResourceMap;
	FILE*							fileRefNum;
	uint16_t						resFileAttributes;
	uint16_t						numTypes;
	struct FakeTypeListEntry*		typeList;
};

/*
	Type list:
	
	number of types (-1)									  2 bytes
		resource type										  4 bytes
		number of resources (-1)							  2 bytes
		offset to reference list (type list relative)		  2 bytes
*/

struct FakeTypeListEntry
{
	uint32_t						resourceType;
	uint16_t						numberOfResourcesOfType;	// -1
	struct FakeReferenceListEntry*	resourceList;
};

/*
	Reference list:
		
	resource ID												  2 bytes
	resource name offset (relative to resource name list)	  2 bytes
	resource attributes										  1 byte
	resource data offset (resource data relative)			  3 bytes
	handle to resource in RAM								  4 bytes
*/

struct FakeReferenceListEntry
{
	int16_t				resourceID;
	uint8_t				resourceAttributes;
	Handle				resourceHandle;
	char				resourceName[257];	// 257 = 1 Pascal length byte, 255 characters for actual string, 1 byte for C terminator \0.
};

/*
	Resource names are stored as byte-counted strings. (I.e. packed P-Strings)
	Look-up by name is case-insensitive but case-preserving and diacritic-sensitive.
*/


struct FakeResourceMap	*	gResourceMap = NULL;	// Linked list.
int16_t						gFakeResError = noErr;


int16_t	FakeResError()
{
	return gFakeResError;
}


struct FakeResourceMap*	FakeResFileOpen( const char* inPath, const char* inMode )
{
	FILE		*			theFile = fopen( inPath, inMode );
	if( !theFile )
	{
		gFakeResError = fnfErr;
		return 0;
	}
	
	uint32_t			resourceDataOffset = 0;
	uint32_t			resourceMapOffset = 0;
	uint32_t			lengthOfResourceData = 0;
	uint32_t			lengthOfResourceMap = 0;
	
	struct FakeResourceMap	*	newMap = calloc( 1, sizeof(struct FakeResourceMap) );
	newMap->fileRefNum = theFile;
	
	if( fread( &resourceDataOffset, 1, sizeof(resourceDataOffset), theFile ) != sizeof(resourceDataOffset) )
	{
		gFakeResError = eofErr;
		free( newMap );
		newMap = NULL;
	}
	resourceDataOffset = BIG_ENDIAN_32(resourceDataOffset);
	printf("resourceDataOffset %d\n", resourceDataOffset);

	if( fread( &resourceMapOffset, 1, sizeof(resourceMapOffset), theFile ) != sizeof(resourceMapOffset) )
	{
		gFakeResError = eofErr;
		free( newMap );
		newMap = NULL;
	}
	resourceMapOffset = BIG_ENDIAN_32(resourceMapOffset);
	printf("resourceMapOffset %d\n", resourceMapOffset);
	
	if( fread( &lengthOfResourceData, 1, sizeof(lengthOfResourceData), theFile ) != sizeof(lengthOfResourceData) )
	{
		gFakeResError = eofErr;
		free( newMap );
		newMap = NULL;
	}
	lengthOfResourceData = BIG_ENDIAN_32(lengthOfResourceData);

	if( fread( &lengthOfResourceMap, 1, sizeof(lengthOfResourceMap), theFile ) != sizeof(lengthOfResourceMap) )
	{
		gFakeResError = eofErr;
		free( newMap );
		newMap = NULL;
	}
	lengthOfResourceMap = BIG_ENDIAN_32(lengthOfResourceMap);
	
	fseek( theFile, 112, SEEK_CUR );	// Skip system data.
	fseek( theFile, 128, SEEK_CUR );	// Skip application data.
	
	fseek( theFile, resourceMapOffset, SEEK_SET );
	fseek( theFile, 16, SEEK_CUR );		// Skip resource file header copy.
	fseek( theFile, 4, SEEK_CUR );		// Skip next resource map placeholder.
	fseek( theFile, 2, SEEK_CUR );		// Skip file ref num placeholder.
	fread( &newMap->resFileAttributes, 1, sizeof(uint16_t), theFile );		// Read file attributes.
	newMap->resFileAttributes = BIG_ENDIAN_16(newMap->resFileAttributes);
	printf("resFileAttributes %d\n", newMap->resFileAttributes);
	
	uint16_t		typeListOffset = 0;
	uint16_t		nameListOffset = 0;
	
	fread( &typeListOffset, 1, sizeof(typeListOffset), theFile );
	typeListOffset = BIG_ENDIAN_16(typeListOffset);
	fread( &nameListOffset, 1, sizeof(nameListOffset), theFile );
	nameListOffset = BIG_ENDIAN_16(nameListOffset);
	
	long		typeListSeekPos = resourceMapOffset +(long)typeListOffset;
	printf("typeListSeekPos %ld\n", typeListSeekPos);
	fseek( theFile, typeListSeekPos, SEEK_SET );
	
	uint16_t		numTypes = 0;
	fread( &numTypes, 1, sizeof(numTypes), theFile );
	numTypes = BIG_ENDIAN_16(numTypes);
	printf("numTypes %d\n", numTypes +1);
	
	newMap->typeList = calloc( ((int)numTypes) +1, sizeof(struct FakeTypeListEntry) );
	newMap->numTypes = numTypes;
	for( int x = 0; x < ((int)numTypes) +1; x++ )
	{
		fread( &newMap->typeList[x].resourceType, 1, sizeof(uint32_t), theFile );	// Read type code (4CC).
		uint32_t	currType = BIG_ENDIAN_32(newMap->typeList[x].resourceType);
		char		typeStr[5] = {0};
		memmove( typeStr, &currType, 4 );
		printf( "currType '%s'\n", typeStr );
		
		uint16_t		numResources = 0;
		fread( &numResources, 1, sizeof(numResources), theFile );
		numResources = BIG_ENDIAN_16(numResources);
		printf("\tnumResources %d\n", numResources +1);
		
		uint16_t		refListOffset = 0;
		fread( &refListOffset, 1, sizeof(refListOffset), theFile );
		refListOffset = BIG_ENDIAN_16(refListOffset);
		
		long	oldOffset = ftell(theFile);
		
		long		refListSeekPos = typeListSeekPos +(long)refListOffset;
		printf("\trefListSeekPos %ld\n", refListSeekPos);
		fseek( theFile, refListSeekPos, SEEK_SET );
				
		newMap->typeList[x].resourceList = calloc( ((int)numResources) +1, sizeof(struct FakeReferenceListEntry) );
		newMap->typeList[x].numberOfResourcesOfType = ((int)numResources) +1;
		for( int y = 0; y < ((int)numResources) +1; y++ )
		{
			fread( &newMap->typeList[x].resourceList[y].resourceID, 1, sizeof(uint16_t), theFile );
			newMap->typeList[x].resourceList[y].resourceID = BIG_ENDIAN_16(newMap->typeList[x].resourceList[y].resourceID);
			
			uint16_t	nameOffset = 0;
			fread( &nameOffset, 1, sizeof(nameOffset), theFile );
			nameOffset = BIG_ENDIAN_16(nameOffset);

			unsigned char	attributesAndDataOffset[4];
			uint32_t		dataOffset = 0;
			fread( &attributesAndDataOffset, 1, sizeof(attributesAndDataOffset), theFile );
			newMap->typeList[x].resourceList[y].resourceAttributes = attributesAndDataOffset[0];
			memmove( ((char*)&dataOffset) +1, attributesAndDataOffset +1, 3 );
			dataOffset = BIG_ENDIAN_32(dataOffset);
			fseek( theFile, 4, SEEK_CUR );		// Skip resource Handle placeholder.
		
			long		innerOldOffset = ftell(theFile);
			long		dataSeekPos = resourceDataOffset +(long)dataOffset;
			fseek( theFile, dataSeekPos, SEEK_SET );
			uint32_t	dataLength = 0;
			fread( &dataLength, 1, sizeof(dataLength), theFile );
			dataLength = BIG_ENDIAN_32(dataLength);
			newMap->typeList[x].resourceList[y].resourceHandle = NewFakeHandle(dataLength);
			fread( (*newMap->typeList[x].resourceList[y].resourceHandle), 1, dataLength, theFile );
			
			if( -1 != (long)nameOffset )
			{
				long		nameSeekPos = resourceMapOffset +(long)nameListOffset +(long)nameOffset;
				fseek( theFile, nameSeekPos, SEEK_SET );
				uint8_t	nameLength = 0;
				fread( &nameLength, 1, sizeof(nameLength), theFile );
				newMap->typeList[x].resourceList[y].resourceName[0] = nameLength;
				if( nameLength > 0 )
					fread( newMap->typeList[x].resourceList[y].resourceName +1, 1, nameLength, theFile );
			}
			
			printf( "\t%d: \"%s\"\n", newMap->typeList[x].resourceList[y].resourceID, newMap->typeList[x].resourceList[y].resourceName +1 );
			
			fseek( theFile, innerOldOffset, SEEK_SET );
		}
		
		fseek( theFile, oldOffset, SEEK_SET );
	}
	
	newMap->nextResourceMap = gResourceMap;
	gResourceMap = newMap;
	gFakeResError = noErr;
	
	return newMap;
}
