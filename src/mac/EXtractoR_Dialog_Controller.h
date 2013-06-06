#import <Cocoa/Cocoa.h>

#if !NSINTEGER_DEFINED
typedef int NSInteger;
#define NSINTEGER_DEFINED 1
#endif

@interface EXtractoR_Dialog_Controller : NSObject {
	IBOutlet NSPopUpButton *layerMenu;
    IBOutlet NSPopUpButton *redMenu;
    IBOutlet NSPopUpButton *greenMenu;
    IBOutlet NSPopUpButton *blueMenu;
    IBOutlet NSPopUpButton *alphaMenu;
	IBOutlet NSWindow *window;
	
	NSDictionary *layers;
}

- (id)init:(NSArray *)chans
	layers:(NSDictionary *)lays
	red:(NSString *)r
	green:(NSString *)g
	blue:(NSString *)b
	alpha:(NSString *)a;

- (IBAction)clickOK:(NSButton *)sender;
- (IBAction)clickCancel:(NSButton *)sender;

- (IBAction)trackLayerMenu:(id)sender;

- (NSWindow *)getWindow;

- (NSString *)getRed;
- (NSString *)getGreen;
- (NSString *)getBlue;
- (NSString *)getAlpha;

@end

