//
//  FakeResources.h
//  ReClassicfication
//
//  Created by Uli Kusterer on 21.02.13.
//  Copyright (c) 2013 Uli Kusterer. All rights reserved.
//

#ifndef ReClassicfication_FakeResources_h
#define ReClassicfication_FakeResources_h


#include "FakeHandles.h"


// Possible return values of FakeResError():
#ifndef __MACERRORS__
enum
{
	resNotFound		= -192,
	resFNotFound	= -193,
	addResFailed	= -194,
	rmvResFailed	= -196,
	eofErr			= -39,
	fnfErr			= -43
};
#endif /* __MACERRORS__ */


#ifndef __RESOURCES__
// Resource attribute bit flags:
enum
{
	resReserved		= (1 << 0),	// Apparently not yet used.
	resChanged		= (1 << 1),
	resPreload		= (1 << 2),
	resProtected	= (1 << 3),
	resLocked		= (1 << 4),
	resPurgeable	= (1 << 5),
	resSysHeap		= (1 << 6),
	resReserved2	= (1 << 7)	// Apparently not yet used.
};
#endif


int16_t	FakeOpenResFile( const char* inPath );
void	FakeCloseResFile( int16_t resRefNum );
Handle	FakeGet1Resource( uint32_t resType, int16_t resID );
Handle	FakeGetResource( uint32_t resType, int16_t resID );
void	FakeUseResFile( int16_t resRefNum );


int16_t	FakeResError();


#endif
