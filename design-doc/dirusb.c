
#include <stdio.h>
#include <stdlib.h>

#include <usb.h>

#define USBLCD_VENDOR_ID  0x03eb
#define USBLCD_PRODUCT_ID 0x6119

extern int usb_debug;

static usb_dev_handle *lcd;
static int interface;
static char *Buffer;
static char *BufPtr;

int drv_UL_open(void) {

  struct usb_bus *busses, *bus;
  struct usb_device *dev;

  lcd = NULL;

  usb_debug = 0;

  usb_init();
  usb_find_busses();
  usb_find_devices();
  busses = usb_get_busses();

  for (bus = busses; bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev = dev->next) {
      if ((dev->descriptor.idVendor == USBLCD_VENDOR_ID) &&
          (dev->descriptor.idProduct == USBLCD_PRODUCT_ID)) {
        printf ("USB LCD gefunden\n");
    lcd = usb_open(dev);
    if (usb_claim_interface(lcd, interface) < 0) {
      printf ("FEHLER : usb_claim_interface\n");
      return -1;
      }
    else {
      printf ("usb_claim_interface erfolgreich\n");
      
    }
      }
    }
  }
  return -1;
}

int drv_UL_close(void) {
  usb_release_interface (lcd, interface);
  usb_close(lcd);
  printf (" LCD wieder freigegeben\n");
}

int drv_CMD(void) {
  char *cmdBuf, *cmdPtr, *inBuf, *inPtr;
  int rv, i, inrv;

  inBuf = (char *) malloc(1024);
  if (inBuf == NULL) {
    printf ("kann speicher nicht reservieren!\n");
    return -1;
  }
  inrv = usb_bulk_read(lcd, 0x81, inBuf, 1000, 1000);
  printf ("Ergebnis von usb_bulk_read: %d\n", inrv);

  cmdBuf = (char *) malloc(1024);
  if (cmdBuf == NULL) {
    printf ("Kann Speicher für cmd nicht reservieren!\n");
    return -1;
  } 

  cmdPtr=cmdBuf;
  *cmdPtr++=0x04;
  *cmdPtr++=0x00;
  *cmdPtr++=0x03;
  *cmdPtr++=0x00;
  *cmdPtr++=0x00;
  *cmdPtr++=0x4c;
  rv=usb_bulk_write (lcd, 0x02, cmdBuf, 6, 1000);
  printf ("ergebnis von usb_bulk_write2 %d\n", rv);

  cmdPtr=cmdBuf;
  *cmdPtr++=0x08;
  *cmdPtr++=0x03;
  *cmdPtr++=0xFD;
  *cmdPtr++=0x54;
  *cmdPtr++=0x68;
  *cmdPtr++=0x65;
  *cmdPtr++=0x6f;
  *cmdPtr++=0x20;
  *cmdPtr++=0x53;
  *cmdPtr++=0x63;
  *cmdPtr++=0x68;
  *cmdPtr++=0x6e;
  *cmdPtr++=0x65;
  *cmdPtr++=0x69;
  *cmdPtr++=0x64;
  *cmdPtr++=0x65;
  *cmdPtr++=0x72;
  for (i=1; i <=1007; ++i) {
    *cmdPtr++=0x20;
  }
  rv=usb_bulk_write (lcd, 0x02, cmdBuf, 1024, 1000);
  printf ("ergebnis von usb_bulk_write3 %d\n", rv);

  inPtr=inBuf;
  for (i=1; i <= inrv; ++i) {
   printf ("Inhalt inBuf %x\n", *inPtr++);
  }

  free (inBuf);
  free (cmdBuf);
}

int main(int argc, char *argv[])
{
  drv_UL_open();
  drv_CMD();
  drv_UL_close();

  return EXIT_SUCCESS;
}