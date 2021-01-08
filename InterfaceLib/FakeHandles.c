/*	===========================================================================

	PROJECT:	FakeHandles
	
	FILE:		FakeHandles.c
	
	PURPOSE:	Simulate Handles on machines which only have ANSI-C to easily
				port some of the more simple Macintosh code fragments to other
				platforms.
		
	(C) Copyright 1998 by Uli Kusterer, all rights reserved.
				
	======================================================================== */

#pragma mark [Headers]


// -----------------------------------------------------------------------------
//	Headers:
// -----------------------------------------------------------------------------

#include "FakeHandles.h"
#include <string.h>


// -----------------------------------------------------------------------------
//	Globals:
// -----------------------------------------------------------------------------

/* We have a linked list of master pointer arrays in RAM, so we don't run out
	of master pointers easily. */
MasterPointerBlock		gMasterPointers = {};
long					gFakeHandleError = noErr;


/* -----------------------------------------------------------------------------
	FakeMoreMasters:
		Call this if you need more master pointers Called internally by
		FakeNewHandle() when it runs out of master pointers.
		
	REVISIONS:
		98-08-30	UK		Created.
   ----------------------------------------------------------------------------- */

void	FakeMoreMasters()
{
	MasterPointerBlock*	vMPtrBlock;
	MasterPointerBlock*	vCurrBlock;
	
	// Make a new master pointer block:
	vMPtrBlock = calloc( 1, sizeof(MasterPointerBlock) );
	if( vMPtrBlock == NULL )
	{
		gFakeHandleError = memFulErr;
		return;
	}
		
	// Find last master pointer in last master pointer block:
	vCurrBlock = &gMasterPointers;
	while( vCurrBlock->next != NULL )
		vCurrBlock = vCurrBlock->next;
	
	// Make this last master pointer point to our new block:
	vCurrBlock->next = vMPtrBlock;
	
	gFakeHandleError = noErr;
}


Handle	FakeNewEmptyHandle()
{
	Handle				theHandle = NULL;
	long				x;
	MasterPointerBlock*	vCurrBlock = &gMasterPointers; // TODO: Could optimize performance by remembering last freed MasterPointerBlock or even pointer in it.
	bool				notFound = true;
	
	gFakeHandleError = noErr;
	
	while( notFound )
	{
		for( x = 0; x < (MASTERPOINTER_CHUNK_SIZE-1); x++ )
		{
			if( !(vCurrBlock->pointers[x].used) )
			{
				vCurrBlock->pointers[x].used = true;
				vCurrBlock->pointers[x].memoryFlags = 0;
				vCurrBlock->pointers[x].size = 0;
				
				theHandle = (Handle) &(vCurrBlock->pointers[x]);
				notFound = false;
				break;
			}
		}
		
		if( !vCurrBlock->pointers[MASTERPOINTER_CHUNK_SIZE-1].used )	// Last is unused? We need a new master pointer block!
		{
			FakeMoreMasters();
			if( vCurrBlock->next == NULL )	// No new block added?!
				notFound = false;	// Terminate, it's very likely an error occurred.
		}
		vCurrBlock = vCurrBlock->next;	// Go next master pointer block.
	}
	
	return theHandle;
}


/* -----------------------------------------------------------------------------
	NewHandle:
		Create a new Handle. This creates a new entry in the Master Ptr array and
		allocates memory of the specified size for it. Then it returns a Ptr to
		this entry.
		
		Returns NULL if not successful. If MemError() is noErr upon a NULL return
		value, we are out of master pointers.
		
	REVISIONS:
		2001-02-16	UK		Added support for error codes.
		1998-08-30	UK		Created.
   ----------------------------------------------------------------------------- */

Handle	FakeNewHandle( long theSize )
{
	MasterPointer	*	theHandle = (MasterPointer*) FakeNewEmptyHandle();
	if( theHandle == NULL )
	{
		gFakeHandleError = noErr;
		return NULL;
	}
	
	theHandle->actualPointer = malloc( theSize );
	if( theHandle->actualPointer == NULL )
	{
		FakeDisposeHandle( (Handle) theHandle );
		gFakeHandleError = memFulErr;
	}
	else
		theHandle->size = theSize;
	
	return (Handle)theHandle;
}


/* -----------------------------------------------------------------------------
	DisposeHandle:
		Dispose an existing Handle. Only call this once or you might kill valid
		memory or worse.
		
		This frees the memory we use and marks the entry for the specified Handle
		as unused.
		
	REVISIONS:
		1998-08-30	UK		Created.
   ----------------------------------------------------------------------------- */

void	FakeDisposeHandle( Handle theHand )
{
	MasterPointer*		theEntry = (MasterPointer*) theHand;
	
	if( theEntry->actualPointer )
		free( theEntry->actualPointer );
	theEntry->used = false;
	theEntry->actualPointer = NULL;
	theEntry->memoryFlags = 0;
	theEntry->size = 0;
}


void	FakeEmptyHandle( Handle theHand )
{
	MasterPointer*		theEntry = (MasterPointer*) theHand;
	
	if( theEntry->actualPointer )
		free( theEntry->actualPointer );
	theEntry->actualPointer = NULL;
}


/* -----------------------------------------------------------------------------
	GetHandleSize:
		Return the size of an existing Handle. This simply examines the "size"
		field of the Handle's entry.
		
	REVISIONS:
		1998-08-30	UK		Created.
   ----------------------------------------------------------------------------- */

long	FakeGetHandleSize( Handle theHand )
{
	MasterPointer*		theEntry = (MasterPointer*) theHand;
	
	gFakeHandleError = noErr;
	
	return( theEntry->size );
}


/* -----------------------------------------------------------------------------
	SetHandleSize:
		Change the size of an existing Handle. This reallocates the Handle (keeping
		its data) and updates the size field of the Handle's entry accordingly.
		
	REVISIONS:
		1998-08-30	UK		Created.
   ----------------------------------------------------------------------------- */

void	FakeSetHandleSize( Handle theHand, long theSize )
{
	MasterPointer*	theEntry = (MasterPointer*) theHand;
	char*			thePtr = realloc( theEntry->actualPointer, theSize );
	
	if( thePtr )
	{
		theEntry->actualPointer = thePtr;
		theEntry->size = theSize;
		gFakeHandleError = noErr;
	}
	else
		gFakeHandleError = memFulErr;
}












