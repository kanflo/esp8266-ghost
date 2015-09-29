//
//  ViewController.m
//  Ghost
//
// Copyright (c) 2015 Johan Kanflo (github.com/kanflo)
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#import "ViewController.h"
#import <MQTTKit.h>
#import "NKOColorPickerView.h"
#import "UIImageEffects.h"

#warning "Change your MQTT broker here!"

#define kMQTTServerHost @"iot.eclipse.org"
#define kTopic @"ghost/led"

#define XMIT_DELTA_MS (0.200)

// mosquitto_pub -h 172.16.3.104 -t ghost/led -m "#000000"

@interface ViewController()

@property (nonatomic, weak) IBOutlet NKOColorPickerView *pickerView;
@property (nonatomic, strong) MQTTClient *client;
@property (strong, nonatomic) IBOutlet UIView *colorView;
@property (strong, nonatomic) IBOutlet UIImageView *ghostImageView;
@property (strong, nonatomic) IBOutlet UIImage *ghostImage;
@property (strong, nonatomic) IBOutlet UIImage *ghostMask;
@property (strong, nonatomic) UIColor *lastColor;
@property (assign) double lastXmitTime;
@property (assign) BOOL hasReceivedInitialColor;
@end

@implementation ViewController

- (void)viewDidLoad
{
  __weak ViewController *weakSelf = self;
  [super viewDidLoad];
  self.lastXmitTime = 0;
  self.lastColor = nil;

  self.ghostImage = [UIImage imageNamed:@"WhiteGhost"];
  self.ghostMask = [UIImage imageNamed:@"GhostMask"];
  self.hasReceivedInitialColor = false;
  
  NSString *clientID = [UIDevice currentDevice].identifierForVendor.UUIDString;
  self.client = [[MQTTClient alloc] initWithClientId:clientID];
  [self.client connectToHost:kMQTTServerHost completionHandler:^(MQTTConnectionReturnCode code) {
    if (code == ConnectionAccepted) {
      // The client is connected when this completion handler is called
      NSLog(@"client is connected with id %@", clientID);
      // Subscribe to the topic

      [self.client setMessageHandler:^(MQTTMessage *message) {
        NSString *text = message.payloadString;
        NSLog(@"received message %@", text);
        if (!weakSelf.hasReceivedInitialColor) {
          //          weakSelf.hasReceivedInitialColor = true;
          dispatch_async(dispatch_get_main_queue(), ^{
            if (text.length == 7) { // "#RRGGBB"
              UIColor *newColor = [weakSelf convertColorString:text];
              if (![newColor isEqual:weakSelf.lastColor]) {
                weakSelf.lastColor = newColor;
                weakSelf.pickerView.color = newColor;
                [weakSelf _tintGhost];
              }
            }
          });
        }
      }];
      
      [self.client subscribe:kTopic withCompletionHandler:^(NSArray *grantedQos) {
        // The client is effectively subscribed to the topic when this completion handler is called
        NSLog(@"subscribed to topic %@", kTopic);
      }];
    }
  }];
 
  [self.pickerView setDidChangeColorBlock:^(UIColor *color) {
    if (![color isEqual:weakSelf.lastColor]) {
      self.lastColor = color;
      [weakSelf _tintGhost];
      [weakSelf _publishColor];
    }
  }];
}

- (void)viewDidAppear:(BOOL)animated
{
}

- (void)dealloc
{
  // disconnect the MQTT client
  [self.client disconnectWithCompletionHandler:^(NSUInteger code) {
    // The client is disconnected when this completion handler is called
    NSLog(@"MQTT is disconnected");
  }];
}

// Convert "#rrggbb" to UIColor
- (UIColor*) convertColorString:(NSString*) string
{
  unsigned rgbValue = 0;
  NSScanner *scanner = [NSScanner scannerWithString:string];
  [scanner setScanLocation:1]; // bypass '#'
  [scanner scanHexInt:&rgbValue];
  return [UIColor colorWithRed:((rgbValue & 0xFF0000) >> 16)/255.0 green:((rgbValue & 0xFF00) >> 8)/255.0 blue:(rgbValue & 0xFF)/255.0 alpha:1.0];
}

#pragma mark - Private methods
- (void)_tintGhost
{
  CGFloat r, g, b, a;
  UIColor *newColor = self.pickerView.color;
  [newColor getRed:&r green:&g blue:&b alpha:&a];
  NSLog(@"Tinting ghost: %.2f %.2f %.2f", r, g, b);
  self.colorView.backgroundColor = newColor;
  // Tint the ghost
  self.ghostImageView.image = [UIImageEffects imageByApplyingTintEffectWithColor:newColor toImage:self.ghostImage maskImage:self.ghostMask];
}

- (void)_publishColor
{
  int r, g, b, a;
  CGFloat fr, fg, fb, fa;
  [self.pickerView.color getRed:&fr green:&fg blue:&fb alpha:&fa];
  
  r = (int)(255.0 * fr);
  g = (int)(255.0 * fg);
  b = (int)(255.0 * fb);
  a = (int)(255.0 * fa);
  
  [NSString stringWithFormat:@"%02x%02x%02x%02x", r, g, b, a];
  
  NSString *payload = [NSString stringWithFormat:@"#%02x%02x%02x", r, g, b];
  
  // Don't spam the little ghost
  if ([[NSDate date] timeIntervalSince1970] - self.lastXmitTime > XMIT_DELTA_MS) {
    // use the MQTT client to send a message with the switch status to the topic
    NSLog(@"> %@", payload);
    //    self.ghostImageView.tintColor = self.pickerView.color;
    [self.client publishString:payload
                       toTopic:kTopic
                       withQos:AtMostOnce
                        retain:YES
             completionHandler:nil];
    self.lastXmitTime = [[NSDate date] timeIntervalSince1970];
    // we passed nil to the completionHandler as we are not interested to know
    // when the message was effectively sent
  }
}

@end
