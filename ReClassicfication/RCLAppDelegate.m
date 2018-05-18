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
	
	**resHandle = 0x44;
	FakeChangedResource(resHandle);

	// removes one of 3 'tdat' resources
	resHandle = FakeGet1Resource('tdat', 2);
	FakeRemoveResource(resHandle);
	FakeDisposeHandle(resHandle);

	// change one of the resource IDs and names
	resHandle = FakeGet1Resource('tdat', 3);
	FakeStr255  newName;
	strcpy((char*)&newName[1], "name");
	newName[0] = 4;
	FakeSetResInfo(resHandle, 33, newName);
	
	// removes the last resource type of 'tvar'
	resHandle = FakeGet1Resource('tvar', 128);
	FakeRemoveResource(resHandle);
	FakeDisposeHandle(resHandle);
	
	// add a couple of resources
	resHandle = FakeNewHandle(4);
	*(uint32_t*)*resHandle = 0x01020304;
	strcpy((char*)&newName[1], "fame");
	newName[0] = 4;
	FakeAddResource(resHandle, 'BFED', 333, newName);
	
	resHandle = FakeNewHandle(4);
	*(uint32_t*)*resHandle = 0xffeeddcc;
	newName[0] = 0;
	FakeAddResource(resHandle, 'BFED', 444, newName);
	
	// should close and update the file
	FakeCloseResFile( resFileRef );
	
	// re-open the file to check the updated resources
	resFileRef = FakeOpenResFile( path );
	resHandle = FakeGetResource( 'pxm#', 4290 );
	printf( "resHandle3 = %p\n", resHandle );
	
	if( resHandle )
	{
		printf( "**resHandle3 = 0x%02x\n", **resHandle);
	}

	resHandle = FakeGet1Resource('tdat', 2);
	printf("'tdat' #2 - err = %d, h=%p\n", FakeResError(), resHandle);

	resHandle = FakeGet1Resource('tdat', 3);
	printf("'tdat' #3 - err = %d, h=%p\n", FakeResError(), resHandle);

	resHandle = FakeGet1Resource('tdat', 33);
	printf("'tdat' #33 - err = %d, h=%p\n", FakeResError(), resHandle);
	
	if( resHandle )
	{
		int16_t    theID;
		uint32_t   theType;
		FakeStr255 resName;
		char       name[256];

		FakeGetResInfo(resHandle, &theID, &theType, &resName);

		memcpy(name, &resName[1], resName[0]);
		name[name[0]+1] = '\0';
		
		printf("  - theID = %d, theType = 0x%04x, name = \"%s\"\n", theID, theType, name);
	}

	resHandle = FakeGet1Resource('tvar', 128);
	printf("'tvar' #128 - err = %d, h=%p\n", FakeResError(), resHandle);
	
	
	resHandle = FakeGet1Resource('BFED', 333);
	printf("'BFED' #333 - err = %d, h=%p\n", FakeResError(), resHandle);
	if( resHandle )
	{
		int16_t    theID;
		uint32_t   theType;
		FakeStr255 resName;
		char       name[256];

		FakeGetResInfo(resHandle, &theID, &theType, &resName);
		
		memcpy(name, &resName[1], resName[0]);
		name[name[0]+1] = '\0';
		
		printf("  - theID = %d, theType = 0x%04x, name = \"%s\", value = 0x%04x\n", theID, theType, name, *(uint32*)*resHandle);
	}
	
	resHandle = FakeGet1Resource('BFED', 444);
	printf("'BFED' #444 - err = %d, h=%p\n", FakeResError(), resHandle);
	if( resHandle )
	{
		int16_t    theID;
		uint32_t   theType;
		FakeStr255 resName;
		char       name[256];

		FakeGetResInfo(resHandle, &theID, &theType, &resName);

		memcpy(name, &resName[1], resName[0]);
		name[name[0]+1] = '\0';
		
		printf("  - theID = %d, theType = 0x%04x, name = \"%s\", value = 0x%04x\n", theID, theType, name, *(uint32*)*resHandle);
	}
}

@end
