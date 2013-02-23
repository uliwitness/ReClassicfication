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
	FILE*							fileDescriptor;
	int16_t							fileRefNum;
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


struct FakeResourceMap	*	gResourceMap = NULL;		// Linked list.
struct FakeResourceMap	*	gCurrResourceMap = NULL;	// Start search of map here.
int16_t						gFileRefNumSeed = 0;
int16_t						gFakeResError = noErr;


size_t	FakeFWriteUInt32BE( uint32_t inInt, FILE* theFile )
{
	inInt = BIG_ENDIAN_32(inInt);
	return fwrite( &inInt, sizeof(inInt), 1, theFile );
}


size_t	FakeFWriteInt16BE( int16_t inInt, FILE* theFile )
{
	inInt = BIG_ENDIAN_16(inInt);
	return fwrite( &inInt, sizeof(inInt), 1, theFile );
}


size_t	FakeFWriteUInt16BE( uint16_t inInt, FILE* theFile )
{
	inInt = BIG_ENDIAN_16(inInt);
	return fwrite( &inInt, sizeof(inInt), 1, theFile );
}


int16_t	FakeResError()
{
	return gFakeResError;
}


struct FakeResourceMap*	FakeFindResourceMap( int16_t inFileRefNum, struct FakeResourceMap*** outPrevMapPtr )
{
	struct FakeResourceMap*	currMap = gResourceMap;
	if( outPrevMapPtr )
		*outPrevMapPtr = &gResourceMap;
	
	while( currMap != NULL && currMap->fileRefNum != inFileRefNum )
	{
		if( outPrevMapPtr )
			*outPrevMapPtr = &currMap->nextResourceMap;
		currMap = currMap->nextResourceMap;
	}
	return currMap;
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
	newMap->fileDescriptor = theFile;
	newMap->fileRefNum = gFileRefNumSeed++;
	
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
		uint32_t	currType = 0;
		fread( &currType, 1, sizeof(uint32_t), theFile );	// Read type code (4CC).
		char		typeStr[5] = {0};
		memmove( typeStr, &currType, 4 );
		printf( "currType '%s'\n", typeStr );
		currType = BIG_ENDIAN_32( currType );
		newMap->typeList[x].resourceType = currType;
		
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
			newMap->typeList[x].resourceList[y].resourceHandle = FakeNewHandle(dataLength);
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
	
	gCurrResourceMap = gResourceMap;
	
	return newMap;
}


int16_t	FakeOpenResFile( const unsigned char* inPath )
{
	char		thePath[256] = {0};
	memmove(thePath,inPath +1,inPath[0]);
	struct FakeResourceMap*	theMap = FakeResFileOpen( thePath, "rw" );
	if( !theMap )
		theMap = FakeResFileOpen( thePath, "r" );
	if( theMap )
		return theMap->fileRefNum;
	else
		return gFakeResError;
}


int16_t	FakeHomeResFile( Handle theResource )
{
	struct FakeResourceMap*		currMap = gResourceMap;
	while( currMap != NULL )
	{
		for( int x = 0; x < currMap->numTypes; x++ )
		{
			for( int y = 0; y < currMap->typeList[x].numberOfResourcesOfType; y++ )
			{
				if( currMap->typeList[x].resourceList[y].resourceHandle == theResource )
				{
					gFakeResError = noErr;
					return currMap->fileRefNum;
				}
			}
		}
		currMap = currMap->nextResourceMap;
	}
	
	gFakeResError = resNotFound;
	return -1;
}


void	FakeUpdateResFile( int16_t inFileRefNum )
{
	struct FakeResourceMap*		currMap = FakeFindResourceMap( inFileRefNum, NULL );
	long						headerLength = 4 + 4 + 4 + 4 +112 +128;
	uint32_t					resMapOffset = 0;
	long						refListSize = 0;
	
	// Write header:
	fseek( currMap->fileDescriptor, 0, SEEK_SET );
	uint32_t    resDataOffset = (uint32_t)headerLength;
	FakeFWriteUInt32BE( resDataOffset, currMap->fileDescriptor );
	fseek( currMap->fileDescriptor, headerLength, SEEK_SET );
	
	resMapOffset = (uint32_t)headerLength;
	
	// Write out data for each resource and calculate space needed:
	for( int x = 0; x < currMap->numTypes; x++ )
	{
		for( int y = 0; y < currMap->typeList[x].numberOfResourcesOfType; y++ )
		{
			uint32_t	theSize = (uint32_t)FakeGetHandleSize( currMap->typeList[x].resourceList[y].resourceHandle );
			FakeFWriteUInt32BE( theSize, currMap->fileDescriptor );
			resMapOffset += 4;
			fwrite( *currMap->typeList[x].resourceList[y].resourceHandle, 1, theSize, currMap->fileDescriptor );
			resMapOffset += theSize;
			
			refListSize += currMap->typeList[x].numberOfResourcesOfType * (2 + 2 + 1 + 3 + 4);
		}
	}
	
	// Write out what we know into the header now:
	fseek( currMap->fileDescriptor, 4, SEEK_SET );
	FakeFWriteUInt32BE( resMapOffset, currMap->fileDescriptor );
	uint32_t	resDataLength = resMapOffset -(uint32_t)headerLength;
	FakeFWriteUInt32BE( resDataLength, currMap->fileDescriptor );
	
	// Start writing resource map after data:
	uint32_t		resMapLength = 0;
	fseek( currMap->fileDescriptor, resMapOffset, SEEK_SET );
	
	fseek( currMap->fileDescriptor, 4 + 4 + 4 + 4 + 4 + 2, SEEK_CUR );
	resMapLength += 4 + 4 + 4 + 4 + 4 + 2;
	FakeFWriteUInt16BE( currMap->resFileAttributes, currMap->fileDescriptor );
	resMapLength += sizeof(uint16_t);
	
	uint16_t		typeListOffset = ftell(currMap->fileDescriptor) +2L +2L -resMapOffset;
	FakeFWriteUInt16BE( typeListOffset, currMap->fileDescriptor );	// Res map relative.
	long			refListStartPosition = typeListOffset + 2 + currMap->numTypes * (4 + 2 + 2);	// Calc where we'll start to put resource lists (right after types).
	uint16_t		nameListOffset = refListStartPosition +refListSize;		// Calc where we'll start to put name lists (right after resource lists).
	FakeFWriteUInt16BE( nameListOffset, currMap->fileDescriptor );	// Res map relative.

	// Now write type list and ref lists:
	uint32_t		nameListStartOffset = 0;
	uint16_t		numTypes = currMap->numTypes -1;
	FakeFWriteUInt16BE( numTypes, currMap->fileDescriptor );
	uint32_t		resDataCurrOffset = resDataOffset;	// Keep track of where we wrote the associated data for each resource.
	
	for( int x = 0; x < currMap->numTypes; x++ )
	{
		// Write entry for this type:
		uint32_t	currType = BIG_ENDIAN_32(currMap->typeList[x].resourceType);
		FakeFWriteUInt32BE( currType, currMap->fileDescriptor );
		
		uint16_t	numResources = currMap->typeList[x].numberOfResourcesOfType -1;
		FakeFWriteUInt16BE( numResources, currMap->fileDescriptor );
		
		uint16_t	refListOffset = refListStartPosition;
		FakeFWriteUInt16BE( refListOffset, currMap->fileDescriptor );
		
		// Jump to ref list location and write ref list out:
		long		oldOffsetAfterPrevType = ftell(currMap->fileDescriptor);
		
		for( int y = 0; y < currMap->typeList[x].numberOfResourcesOfType; y++ )
		{
			FakeFWriteInt16BE( currMap->typeList[x].resourceList[y].resourceID, currMap->fileDescriptor );
			
			// Write name to name table:
			if( currMap->typeList[x].resourceList[y].resourceName[0] == 0 )
				FakeFWriteInt16BE( -1, currMap->fileDescriptor );	// Don't have a name, mark as -1.
			else
			{
				FakeFWriteUInt16BE( nameListStartOffset, currMap->fileDescriptor );	// Associate name in name table with this.
				
				long oldOffsetAfterNameOffset = ftell( currMap->fileDescriptor );
				fseek( currMap->fileDescriptor, resMapOffset +typeListOffset +nameListOffset +nameListStartOffset, SEEK_SET );
				fwrite( currMap->typeList[x].resourceList[y].resourceName, currMap->typeList[x].resourceList[y].resourceName[0] +1, sizeof(uint8_t), currMap->fileDescriptor );

				long	currMapLen = (ftell(currMap->fileDescriptor) -resMapOffset);
				if( currMapLen > resMapLength )
					resMapLength = (uint32_t)currMapLen;

				fseek( currMap->fileDescriptor, oldOffsetAfterNameOffset, SEEK_SET );
				nameListStartOffset += currMap->typeList[x].resourceList[y].resourceName[0] +1;	// Make sure we write next name *after* this one.
			}
			
			fwrite( &currMap->typeList[x].resourceList[y].resourceAttributes, 1, sizeof(uint8_t), currMap->fileDescriptor );
			uint32_t	resDataCurrOffsetBE = BIG_ENDIAN_32(resDataCurrOffset);
			fwrite( ((uint8_t*)&resDataCurrOffsetBE) +1, 1, 3, currMap->fileDescriptor );
			resDataCurrOffset += 4 + FakeGetHandleSize(currMap->typeList[x].resourceList[y].resourceHandle);
			FakeFWriteUInt32BE( 0, currMap->fileDescriptor );	// Handle placeholder.
			
			long	currMapLen = (ftell(currMap->fileDescriptor) -resMapOffset);
			if( currMapLen > resMapLength )
				resMapLength = (uint32_t)currMapLen;
		}
		
		refListStartPosition += currMap->typeList[x].numberOfResourcesOfType * (2 + 2 + 1 + 3 + 4);
		
		// Jump back to after our type entry so we can write the next one:
		fseek( currMap->fileDescriptor, oldOffsetAfterPrevType, SEEK_SET );
	}

	// Write res map length:
	fseek( currMap->fileDescriptor, 4 + 4 + 4, SEEK_SET );
	FakeFWriteUInt32BE( resMapLength, currMap->fileDescriptor );
}


void	FakeRedirectResFileToPath( int16_t inFileRefNum, const char* cPath )
{
	struct FakeResourceMap**	prevMapPtr = NULL;
	struct FakeResourceMap*		currMap = FakeFindResourceMap( inFileRefNum, &prevMapPtr );
	if( currMap )
	{
		fclose( currMap->fileDescriptor );
		currMap->fileDescriptor = fopen( cPath, "w" );
	}
}


void	FakeCloseResFile( int16_t inFileRefNum )
{
	struct FakeResourceMap**	prevMapPtr = NULL;
	struct FakeResourceMap*		currMap = FakeFindResourceMap( inFileRefNum, &prevMapPtr );
	if( currMap )
	{
		*prevMapPtr = currMap->nextResourceMap;	// Remove this from the linked list.
		if( gCurrResourceMap == currMap )
			gCurrResourceMap = currMap->nextResourceMap;
		
		for( int x = 0; x < currMap->numTypes; x++ )
		{
			for( int y = 0; y < currMap->typeList[x].numberOfResourcesOfType; y++ )
			{
				FakeDisposeHandle( currMap->typeList[x].resourceList[y].resourceHandle );
			}
			free( currMap->typeList[x].resourceList );
		}
		free( currMap->typeList );
		
		fclose( currMap->fileDescriptor );
		free( currMap );
	}
}


Handle	FakeGet1ResourceFromMap( uint32_t resType, int16_t resID, struct FakeResourceMap* inMap )
{
	if( inMap != NULL )
	{
		for( int x = 0; x < inMap->numTypes; x++ )
		{
			uint32_t		currType = inMap->typeList[x].resourceType;
			if( currType == resType )
			{
				for( int y = 0; y < inMap->typeList[x].numberOfResourcesOfType; y++ )
				{
					if( inMap->typeList[x].resourceList[y].resourceID == resID )
					{
						return inMap->typeList[x].resourceList[y].resourceHandle;
					}
				}
				break;
			}
		}
	}
	
	gFakeResError = resNotFound;
	
	return NULL;
}


Handle	FakeGet1Resource( uint32_t resType, int16_t resID )
{
	return FakeGet1ResourceFromMap( resType, resID, gCurrResourceMap );
}


Handle	FakeGetResource( uint32_t resType, int16_t resID )
{
	struct FakeResourceMap *	currMap = gCurrResourceMap;
	Handle						theRes = NULL;
	
	while( theRes == NULL && currMap != NULL )
	{
		theRes = FakeGet1ResourceFromMap( resType, resID, currMap );
		if( theRes != NULL )
			return theRes;
		
		currMap	= currMap->nextResourceMap;
	}
	
	gFakeResError = resNotFound;
	
	return NULL;
}


void	FakeUseResFile( int16_t resRefNum )
{
	struct FakeResourceMap*	currMap = FakeFindResourceMap( resRefNum, NULL );
	if( !currMap )
		currMap = gResourceMap;
	
	gCurrResourceMap = currMap;
}




