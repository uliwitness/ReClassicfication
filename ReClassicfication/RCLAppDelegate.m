//
//  RCLAppDelegate.m
//  ReClassicfication
//
//  Created by Uli Kusterer on 20.02.13.
//  Copyright (c) 2013 Uli Kusterer. All rights reserved.
//

#import "RCLAppDelegate.h"
#import "FakeHandles.h"
#import "FakeResources.h"


@implementation RCLAppDelegate

- (void)dealloc
{
    [super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	Handle		theHand;
	long		theSize;
	char		num;
	
	FakeInitHandles( gMasterPointers );
	
	theHand = FakeNewHandle( 2 );
	
	(**(short**)theHand) = -1024;
	
	printf( "The number is: %d\n", (**(short**)theHand) );
	
	theSize = FakeGetHandleSize( theHand );
	printf( "Current Size: %ld\n", theSize );
	
	num = ((**(short**)theHand) & 0xFF00) >> 8;
	
	theSize -= 1;
	FakeSetHandleSize( theHand, theSize );
	theSize = FakeGetHandleSize( theHand );
	printf( "New Size: %ld\n", theSize );
	
	printf( "The number is: %d (%d)\n", (**(char**)theHand), num );
	
	printf( "\n============================================================\n\n" );

	NSString	*	resFilePath = [[NSBundle mainBundle] bundlePath];
	resFilePath = [[resFilePath stringByDeletingLastPathComponent] stringByAppendingPathComponent: @"TestResFile.rsrc"];
	
	const char*	cPath = "/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/Resources/Extras2.rsrc";
	unsigned char	path[257] = {0};
	path[0] = strlen(cPath);
	memmove( path +1, cPath, path[0] );
	
	int16_t resFileRef = FakeOpenResFile( path );
	
	printf( "%d == %d\n", FakeCount1Types(), FakeCountTypes() );
	
	FakeRedirectResFileToPath( resFileRef, [resFilePath fileSystemRepresentation] );
	
	Handle resHandle = FakeGetResource( 'pxm#', 4290 );
	printf( "resHandle = %p\n", resHandle );
	
	FakeUpdateResFile( resFileRef );
	FakeCloseResFile( resFileRef );
	
	printf( "\n============================================================\n\n" );
	
	cPath = [resFilePath fileSystemRepresentation];
	memset( path, 0, sizeof(path) );
	path[0] = strlen(cPath);
	memmove( path +1, cPath, path[0] );
	resFileRef = FakeOpenResFile( path );
	resHandle = FakeGetResource( 'pxm#', 4290 );
	printf( "resHandle2 = %p\n", resHandle );
	FakeCloseResFile( resFileRef );
}

@end
