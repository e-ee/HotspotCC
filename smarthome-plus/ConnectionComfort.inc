#include <string.h>
// Names and ABIs verified in the supplied native executable, not inferred from new SDKs.
static IMP HPOriginalScanSuccess;
static char HPAcceleratedScanCache;
static void HPScanSuccess(id self,SEL selector){
    ((void(*)(id,SEL))HPOriginalScanSuccess)(self,selector);
    dispatch_async(dispatch_get_main_queue(),^{
        NSDictionary *candidates=HPGet(self,@"scanedPeripheralDict");
        if(![candidates isKindOfClass:NSDictionary.class]||!candidates.count||objc_getAssociatedObject(self,&HPAcceleratedScanCache)==candidates)return;
        int best=-127;for(NSNumber *rssi in candidates.allValues)if([rssi isKindOfClass:NSNumber.class])best=MAX(best,rssi.intValue);
        if(best< -75)return; // Keep the native longer scan for weak links.
        SEL connect=NSSelectorFromString(@"connectRssiHightestPeripheral");if(![self respondsToSelector:connect])return;
        objc_setAssociatedObject(self,&HPAcceleratedScanCache,candidates,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [NSObject cancelPreviousPerformRequestsWithTarget:self selector:connect object:nil];
        [self performSelector:connect withObject:nil afterDelay:.30];
    });
}
static void HPInstallConnectionComfort(void){
    Method method=class_getInstanceMethod(NSClassFromString(@"SigBearer"),NSSelectorFromString(@"scanProvisionedDevicesSuccess"));
    if(method&&strcmp(method_getTypeEncoding(method),"v16@0:8")==0)HPOriginalScanSuccess=method_setImplementation(method,(IMP)HPScanSuccess);
}
