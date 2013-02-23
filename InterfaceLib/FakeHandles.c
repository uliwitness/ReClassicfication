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


// -----------------------------------------------------------------------------
//	Globals:
// -----------------------------------------------------------------------------

/* The last entry in the master pointer array is mis-used to hold a pointer
	to another master pointer array. Thus, we have a linked list of master
	pointer arrays in RAM, and we don't run out of master pointers as easily. */
MasterPointer		gMasterPointers[MAX_HANDLE_COUNT];
long				gFakeHandleError = noErr;


/* -----------------------------------------------------------------------------
	FakeInitHandles:
		Call this to initialize the fake memory Manager at the start of your
		program. Only call this once or you'll lose all your Handles and will have
		stale memory lying around. Pass the global gMasterPointers in
		masterPtrArray.
		
	REVISIONS:
		98-08-30	UK		Created.
   ----------------------------------------------------------------------------- */

void	FakeInitHandles( MasterPointer* masterPtrArray )
{
	long		x;
	
	for( x = 0; x < MAX_HANDLE_COUNT; x++ )
	{
		masterPtrArray[x].actualPointer = NULL;
		masterPtrArray[x].used = false;
		masterPtrArray[x].memoryFlags = 0;
		masterPtrArray[x].size = 0;
	}
	
	gFakeHandleError = noErr;
}


/* -----------------------------------------------------------------------------
	FakeMoreMasters:
		Call this if you need more master pointers Called internally by
		FakeNewHandle() when it runs out of master pointers.
		
	REVISIONS:
		98-08-30	UK		Created.
   ----------------------------------------------------------------------------- */

void	FakeMoreMasters()
{
	long			x;
	MasterPointer*	vMPtrBlock;
	MasterPointer*	vCurrBlock;
	
	// Make a new master pointer block:
	vMPtrBlock = malloc( MAX_HANDLE_COUNT *sizeof(MasterPointer) );
	if( vMPtrBlock == NULL )
	{
		gFakeHandleError = memFulErr;
		return;
	}
	
	// Clear it:
	for( x = 0; x < MAX_HANDLE_COUNT; x++ )
	{
		vMPtrBlock[x].actualPointer = NULL;
		vMPtrBlock[x].used = false;
		vMPtrBlock[x].memoryFlags = 0;
		vMPtrBlock[x].size = 0;
	}
	
	// Find last master pointer in last master pointer block:
	vCurrBlock = gMasterPointers;
	while( vCurrBlock[MAX_HANDLE_COUNT -1].used == true )
		vCurrBlock = (MasterPointer*) vCurrBlock[MAX_HANDLE_COUNT-1].actualPointer;
	
	// Make this last master pointer point to our new block:
	vCurrBlock[MAX_HANDLE_COUNT-1].actualPointer = (char*) vMPtrBlock;
	vCurrBlock[MAX_HANDLE_COUNT-1].used = true;
	vMPtrBlock[MAX_HANDLE_COUNT-1].size = MAX_HANDLE_COUNT *sizeof(MasterPointer);
	
	gFakeHandleError = noErr;
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
	Handle			theHandle = NULL;
	long			x;
	MasterPointer*	vCurrBlock = gMasterPointers;
	bool			notFound = true;
	
	gFakeHandleError = noErr;
	
	while( notFound )
	{
		for( x = 0; x < (MAX_HANDLE_COUNT-1); x++ )
		{
			if( !(vCurrBlock[x].used) )
			{
				vCurrBlock[x].actualPointer = malloc( theSize );
				if( ((long) vCurrBlock[x].actualPointer) > 0 )
				{
					vCurrBlock[x].used = true;
					vCurrBlock[x].memoryFlags = 0;
					vCurrBlock[x].size = theSize;
					
					theHandle = (Handle) &(vCurrBlock[x]);
					notFound = false;
					break;
				}
				else
				{
					gFakeHandleError = memFulErr;
					notFound = false;
					break;
				}
			}
		}
		
		if( !vCurrBlock[MAX_HANDLE_COUNT-1].used )	// Last is unused? We need a new master pointer block!
		{
			FakeMoreMasters();
			if( !vCurrBlock[MAX_HANDLE_COUNT-1].used )	// No new block added?!
				notFound = false;	// Terminate, it's very likely an error occurred.
		}
		vCurrBlock = (MasterPointer*) vCurrBlock[MAX_HANDLE_COUNT-1].actualPointer;	// Go next master pointer block.
	}
	
	return theHandle;
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
	
	free( theEntry->actualPointer );
	theEntry->used = false;
	theEntry->actualPointer = NULL;
	theEntry->memoryFlags = 0;
	theEntry->size = 0;
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
	MasterPointer*		theEntry = (MasterPointer*) theHand;
	char*				thePtr;
	
	thePtr = theEntry->actualPointer;
	thePtr = realloc( thePtr, theSize );
	
	if( thePtr )
	{
		theEntry->actualPointer = thePtr;
		theEntry->size = theSize;
		gFakeHandleError = noErr;
	}
	else
		gFakeHandleError = memFulErr;
}












