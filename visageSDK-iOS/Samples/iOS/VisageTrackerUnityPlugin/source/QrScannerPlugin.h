//
//  QrScannerPlugin
//
//

#import <Foundation/Foundation.h>
#define EXPORT_API 

extern "C" {
	
	EXPORT_API void _initScanner();
	
	/** Releases memory allocated by the scanner in the initScanner function.
	 */
	EXPORT_API void _releaseScanner();
	
    EXPORT_API void _toggleTorch(int on);

    // side 0 -> front and 1 -> back camera
    EXPORT_API void _switchCamera(int side);
     
}

