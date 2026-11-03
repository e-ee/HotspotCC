#import <mach/mach.h>
#import <mach/mach_vm.h>
#import <mach-o/dyld.h>
#import <TargetConditionals.h>
static BOOL HPReadSceneMemory(uint64_t address,void *out,size_t size){mach_vm_size_t read=0;return address&&mach_vm_read_overwrite(mach_task_self(),address,size,(mach_vm_address_t)out,&read)==KERN_SUCCESS&&read==size;}
#if TARGET_OS_SIMULATOR
static int HPTestSceneSpan=100;
void HPSimulatorSceneSpan(int span){HPTestSceneSpan=span;}
#endif
static int HPNativeSceneSpan(void){
#if TARGET_OS_SIMULATOR
 return HPTestSceneSpan;
#else
 // Exact original 3.0.2 Swift layout. Check its native CTL sender before using
 // these offsets; every pointer/optional read is checked with the VM API.
 uint64_t base=(uint64_t)_dyld_get_image_header(0);unsigned char signature[12];const unsigned char expected[]={0xff,0x83,0x07,0xd1,0xfc,0x6f,0x18,0xa9,0xfa,0x67,0x19,0xa9};
 if(!HPReadSceneMemory(base+0x9690c,signature,12)||memcmp(signature,expected,12))return 100;
 uint64_t manager=0,configOffset=0,config=0,maxOffset=0,minOffset=0;
 if(!HPReadSceneMemory(base+0x66b960,&manager,8)||!HPReadSceneMemory(base+0x65b478,&configOffset,8)||configOffset>4096||!HPReadSceneMemory(manager+configOffset,&config,8)||!config)return 100;
 if(!HPReadSceneMemory(base+0x65b4c0,&maxOffset,8)||!HPReadSceneMemory(base+0x65b4c8,&minOffset,8)||maxOffset>4096||minOffset>4096)return 100;
 struct {int64_t value;unsigned char absent;} maximum,minimum;
 if(!HPReadSceneMemory(config+maxOffset,&maximum,9)||!HPReadSceneMemory(config+minOffset,&minimum,9)||maximum.absent||minimum.absent)return 100;
 if(minimum.value<0||maximum.value>100||maximum.value<=minimum.value)return 100;
 return (int)(maximum.value-minimum.value);
#endif
}
