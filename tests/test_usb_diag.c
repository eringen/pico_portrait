#include "diagnostic.h"
#include "usb_diag.h"
#include "tusb.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
uint32_t fake_time;
static bool attached, submit_ok = true;
static void *buffer;
static uint16_t requested;
static unsigned submitted;
static tuh_xfer_cb_t callback;
static uintptr_t token;
void tuh_mount_cb(uint8_t);
void tuh_umount_cb(uint8_t);
bool tuh_init(uint8_t port) { assert(port==0); return true; }
void tuh_task_ext(uint32_t timeout, bool isr) { assert(timeout==0 && !isr); }
bool hcd_port_connect_status(uint8_t port) { assert(port==0); return attached; }
bool tuh_edpt_abort_xfer(uint8_t addr, uint8_t ep) { assert(addr==1 && ep==0); return true; }
bool tuh_descriptor_get_device(uint8_t addr, void *buf, uint16_t len, tuh_xfer_cb_t cb, uintptr_t data) {
    assert(addr==1 && len<=DESC_CAP); buffer=buf; requested=len; callback=cb; token=data; ++submitted; return submit_ok;
}
bool tuh_descriptor_get_configuration(uint8_t addr, uint8_t index, void *buf, uint16_t len, tuh_xfer_cb_t cb, uintptr_t data) {
    assert(index==0); return tuh_descriptor_get_device(addr,buf,len,cb,data);
}
static void deliver(const void *bytes, size_t n) {
    assert(n<=requested);
    memcpy(buffer,bytes,n);
    tuh_xfer_t x={.daddr=1,.result=XFER_RESULT_SUCCESS,.actual_len=(uint32_t)n,.user_data=token};
    callback(&x);
}
static void mount(void) { attached=true; usb_diag_task(); tuh_mount_cb(1); }
static void unplug(void) { attached=false; usb_diag_task(); tuh_umount_cb(1); }
int main(void) {
    const uint8_t device[]={18,1,0,2,0,0,0,64,0x4d,0x0b,0x23,0x11,0,1,1,2,0,1};
    const uint8_t config[]={9,2,25,0,1,1,0,0x80,50,9,4,0,0,1,7,1,2,0,7,5,0x89,2,64,0,0};
    usb_diag_init(); assert(diagnostic.usb.host_enabled);
    mount(); assert(requested==18);
    deliver(device,18); assert(requested==9 && diagnostic.usb.candidate);
    deliver(config,9); assert(requested==25);
    deliver(config,25); assert(!strcmp(diagnostic.usb.state,"descriptors_ready"));
    assert(diagnostic.usb.configuration[20]==0x89); // observed endpoint, no hardcoding
    assert(submitted==3); // descriptor reads only
    unplug(); assert(!diagnostic.usb.candidate && diagnostic.usb.vid==0 && diagnostic.usb.configuration_len==0);
    assert(diagnostic.usb.disconnects==1); // callback + polling cannot double-count
    mount(); deliver(device,7); assert(!strcmp(diagnostic.usb.state,"bad_device_descriptor")); unplug();
    mount(); deliver(device,18);
    uint8_t huge[]={9,2,1,2,1,1,0,0x80,50}; deliver(huge,9);
    assert(!strcmp(diagnostic.usb.state,"configuration_too_large_or_invalid")); unplug();
    mount(); uintptr_t old_token=token;
    fake_time+=3001; usb_diag_task(); assert(!strcmp(diagnostic.usb.state,"descriptor_timeout_replug_required"));
    unsigned before=submitted; deliver(device,18); assert(submitted==before); // late completion ignored
    unplug(); mount();
    tuh_xfer_t stale={.daddr=1,.result=0,.actual_len=18,.user_data=old_token}; callback(&stale);
    assert(!diagnostic.usb.device_len);
    unplug(); submit_ok=false; mount();
    assert(!strcmp(diagnostic.usb.state,"descriptor_submit_failed")); unplug();
    attached=true; usb_diag_task(); fake_time+=5001; usb_diag_task();
    assert(!strcmp(diagnostic.usb.state,"enumeration_timeout_replug_required"));
    puts("PASS: mocked USB lifecycle, descriptors, limits, failures, timeout, stale callbacks");
}
