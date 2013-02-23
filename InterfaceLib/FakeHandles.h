/*	===========================================================================

	PROJECT:	FakeHandles
	
	FILE:		FakeHandles.h
	
	PURPOSE:	Simulate Handles on machines which only have ANSI-C to easily
				port some of the more simple Macintosh code fragments to other
				platforms.
		
	(C) Copyright 1998 by Uli Kusterer, all rights reserved.
				
	DIRECTIONS:
		A Handle is a memory block that remembers its size automatically.
		To the user, a Handle is simply a pointer to a pointer to the actual
		data. Dereference it twice to get at the actual data. Before you
		pass a once-dereferenced Handle to any other functions, you need to
		call HLock() on it to avoid that it moves. Call HUnlock() when you
		are finished with that.
		To create a Handle, use NewHandle(). To free a Handle, call
		DisposeHandle(). To resize use SetHandleSize() (the Handle itself
		will not change, but the pointer to the actual data may change),
		GetHandleSize() returns the actual size of the Handle.
		Before making any of these calls, you *must have* called
		InitHandles().
				
	======================================================================== */

#ifndef FAKEHANDLES_H
#define FAKEHANDLES_H

#pragma mark [Headers]


// -----------------------------------------------------------------------------
//	Headers:
// -----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>


// -----------------------------------------------------------------------------
//	Constants:
// -----------------------------------------------------------------------------

#ifndef NULL
  #define NULL	0L
#endif

#define MAX_HANDLE_COUNT		1024	// Max. number of Handles that may be created.


// Error codes MemError() may return after Handle calls:
enum
{
#ifndef __MACTYPES__
	noErr		= 0,	// No error, success.
#endif /* __MACTYPES__ */
	memFulErr	= -108	// Out of memory error.
};


// -----------------------------------------------------------------------------
//	Data Types:
// -----------------------------------------------------------------------------

// Data types special to Mac:

typedef	char**			Handle;
#ifndef __MACTYPES__
typedef unsigned char	Boolean;
#endif /* __MACTYPES__ */


// Private data structure used internally to keep track of Handles:
typedef struct MasterPointer
{
	char*		actualPointer;	// The actual Pointer we're pointing to.
	Boolean		used;			// Is this master Ptr being used?
	long		memoryFlags;	// Some flags for this Handle.
	long		size;			// The size of this Handle.
} MasterPointer;

// -----------------------------------------------------------------------------
//	Globals:
// -----------------------------------------------------------------------------

extern MasterPointer	gMasterPointers[MAX_HANDLE_COUNT];
extern long				gFakeHandleError;


// -----------------------------------------------------------------------------
//	Prototypes:
// -----------------------------------------------------------------------------

extern void		FakeInitHandles( MasterPointer* masterPtrArray );
extern Handle	FakeNewHandle( long theSize );
extern void		FakeDisposeHandle( Handle theHand );
extern long		FakeGetHandleSize( Handle theHand );
extern void		FakeSetHandleSize( Handle theHand, long theSize );
extern void		FakeMoreMasters( void );









#endif /*FAKEHANDLES_H*/
