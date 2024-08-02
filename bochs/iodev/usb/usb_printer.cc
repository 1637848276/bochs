/////////////////////////////////////////////////////////////////////////
<<<<<<< HEAD
// $Id: usb_printer.cc 12488 2014-09-22 19:49:39Z vruppert $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2009       Benjamin D Lunt (fys at frontiernet net)
//                2009-2014  The Bochs Project
=======
// $Id: usb_printer.cc 13054 2017-01-29 08:48:08Z vruppert $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2009-2016  Benjamin D Lunt (fys [at] fysnet [dot] net)
//                2009-2017  The Bochs Project
>>>>>>> version-2.6.9
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//
/////////////////////////////////////////////////////////////////////////

// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE

#include "iodev.h"

#if BX_SUPPORT_PCI && BX_SUPPORT_PCIUSB
#include "usb_common.h"
#include "usb_printer.h"

#define LOG_THIS

static const Bit8u bx_printer_dev_descriptor[] = {
  0x12,       /*  u8 bLength; */
  0x01,       /*  u8 bDescriptorType; Device */
  0x10, 0x01, /*  u16 bcdUSB; v1.10 */

  0x00,       /*  u8  bDeviceClass; */
  0x00,       /*  u8  bDeviceSubClass; */
  0x00,       /*  u8  bDeviceProtocol; [ low/full speeds only ] */
  0x08,       /*  u8  bMaxPacketSize0; 8 Bytes */

  0xF0, 0x03, /*  u16 idVendor; */
  0x04, 0x15, /*  u16 idProduct; */
  0x00, 0x01, /*  u16 bcdDevice */

  0x01,       /*  u8  iManufacturer; */
  0x02,       /*  u8  iProduct; */
  0x03,       /*  u8  iSerialNumber; */
  0x01        /*  u8  bNumConfigurations; */
};

static const Bit8u bx_printer_config_descriptor[] = {
  /* one configuration */
  0x09,       /*  u8  bLength; */
  0x02,       /*  u8  bDescriptorType; Configuration */
  0x20, 0x00, /*  u16 wTotalLength; */
  0x01,       /*  u8  bNumInterfaces; (1) */
  0x01,       /*  u8  bConfigurationValue; */
  0x00,       /*  u8  iConfiguration string desc.; */
  0xC0,       /*  u8  bmAttributes;
			 Bit 7: must be set,
			     6: Self-powered,
			     5: Remote wakeup,
			     4..0: resvd */
  0x02,       /*  u8  MaxPower; */

  /* one interface */
  0x09,       /*  u8  if_bLength; */
  0x04,       /*  u8  if_bDescriptorType; Interface */
  0x00,       /*  u8  if_bInterfaceNumber; */
  0x00,       /*  u8  if_bAlternateSetting; */
  0x02,       /*  u8  if_bNumEndpoints; */
  0x07,       /*  u8  if_bInterfaceClass; */
  0x01,       /*  u8  if_bInterfaceSubClass; */
  0x02,       /*  u8  if_bInterfaceProtocol; */
  0x00,       /*  u8  if_iInterface; string desc. */
  
  /* first endpoint */
  0x07,       /*  u8  ep_bLength; */
  0x05,       /*  u8  ep_bDescriptorType; Endpoint */
  0x81,       /*  u8  ep_bEndpointAddress; IN Endpoint 1 */
  0x02,       /*  u8  ep_bmAttributes; Bulk */
  0x40, 0x00, /*  u16 ep_wMaxPacketSize; */
  0x00,       /*  u8  ep_bInterval; */

  /* second endpoint */
  0x07,       /*  u8  ep_bLength; */
  0x05,       /*  u8  ep_bDescriptorType; Endpoint */
  0x02,       /*  u8  ep_bEndpointAddress; OUT Endpoint 2 */
  0x02,       /*  u8  ep_bmAttributes; Bulk */
  0x40, 0x00, /*  u16 ep_wMaxPacketSize; */
  0x00,       /*  u8  ep_bInterval; */
};

// Length word calculated at run time
static const Bit8u bx_device_id_string[] =
  "\0\0"  // len field is calculated at run-time
  "MFG:HEWLETT-PACKARD;"
  "MDL:DESKJET 920C;"
  "CMD:MLC,PCL,PML;"
  "CLASS:PRINTER;"
  "DESCRIPTION:Hewlett-Packard DeskJet 920C;"
  "SERN:CN21R1C0BPIS;"
  "VSTATUS:$HBO,$NCO,ff,DN,IDLE,CUT,K0,C0,SM,NR,KP093,CP097;"
  "VP:0800,FL,B0;"
  "VJ: ;";

<<<<<<< HEAD
usb_printer_device_c::usb_printer_device_c(usbdev_type type, const char *filename)
{
  d.type = type;
  d.maxspeed = USB_SPEED_FULL;
  d.speed = d.maxspeed;
  memset((void*)&s, 0, sizeof(s));
  strcpy(d.devname, "USB Printer");
  s.fname = filename;
  s.fp = NULL;

=======
static int usb_printer_count = 0;

usb_printer_device_c::usb_printer_device_c(usbdev_type type, const char *filename)
{
  char pname[10];
  char label[32];
  bx_param_string_c *fname;

  d.type = type;
  d.speed = d.minspeed = d.maxspeed = USB_SPEED_FULL;
  memset((void*)&s, 0, sizeof(s));
  strcpy(d.devname, "USB Printer");
  d.dev_descriptor = bx_printer_dev_descriptor;
  d.config_descriptor = bx_printer_config_descriptor;
  d.device_desc_size = sizeof(bx_printer_dev_descriptor);
  d.config_desc_size = sizeof(bx_printer_config_descriptor);
  d.vendor_desc = "Hewlett-Packard";
  d.product_desc = "Deskjet 920C";
  d.serial_num = "HU18L6P2DNBI";
  s.fname = filename;
  s.fp = NULL;
  // config options
  bx_list_c *usb_rt = (bx_list_c*)SIM->get_param(BXPN_MENU_RUNTIME_USB);
  sprintf(pname, "printer%d", ++usb_printer_count);
  sprintf(label, "USB Printer #%d Configuration", usb_printer_count);
  s.config = new bx_list_c(usb_rt, pname, label);
  s.config->set_options(bx_list_c::SHOW_PARENT | bx_list_c::USE_BOX_TITLE);
  s.config->set_device_param(this);
  fname = new bx_param_filename_c(s.config, "file", "File", "", "", BX_PATHNAME_LEN);
  fname->set(s.fname);
  fname->set_handler(printfile_handler);
  if (SIM->is_wx_selected()) {
    bx_list_c *usb = (bx_list_c*)SIM->get_param("ports.usb");
    usb->add(s.config);
  }
>>>>>>> version-2.6.9
  put("usb_printer", "USBPRN");
}

usb_printer_device_c::~usb_printer_device_c(void)
{
<<<<<<< HEAD
  if (s.fp != NULL) {
    fclose(s.fp);
  }
=======
  d.sr->clear();
  if (s.fp != NULL) {
    fclose(s.fp);
  }
  if (SIM->is_wx_selected()) {
    bx_list_c *usb = (bx_list_c*)SIM->get_param("ports.usb");
    usb->remove(s.config->get_name());
  }
  bx_list_c *usb_rt = (bx_list_c*)SIM->get_param(BXPN_MENU_RUNTIME_USB);
  usb_rt->remove(s.config->get_name());
>>>>>>> version-2.6.9
}

bx_bool usb_printer_device_c::init()
{
  s.fp = fopen(s.fname, "w+b");
  if (s.fp == NULL) {
    BX_ERROR(("Could not create/open %s", s.fname));
    return 0;
  } else {
    sprintf(s.info_txt, "USB printer: file=%s", s.fname);
    d.connected = 1;
    return 1;
  }
}

const char* usb_printer_device_c::get_info()
{
  return s.info_txt;
}

void usb_printer_device_c::register_state_specific(bx_list_c *parent)
{
  bx_list_c *list = new bx_list_c(parent, "s", "USB PRINTER Device State");
<<<<<<< HEAD
  new bx_shadow_num_c(list, "printer_status", &s.printer_status);
=======
  BXRS_HEX_PARAM_FIELD(list, printer_status, s.printer_status);
>>>>>>> version-2.6.9
}

void usb_printer_device_c::handle_reset()
{
<<<<<<< HEAD
  BX_INFO(("Opened %s for USB HP Deskjet 920C printer emulation.", s.fname));
=======
>>>>>>> version-2.6.9
  BX_DEBUG(("Reset"));
}

int usb_printer_device_c::handle_control(int request, int value, int index, int length, Bit8u *data)
{
  int ret = 0;

  BX_DEBUG(("Printer: request: 0x%04X  value: 0x%04X  index: 0x%04X  len: %i", request, value, index, length));
<<<<<<< HEAD
  switch(request) {
    case DeviceRequest | USB_REQ_GET_STATUS:
      if (d.state == USB_STATE_DEFAULT)
        goto fail;
      else {
        data[0] = (1 << USB_DEVICE_SELF_POWERED) |
          (d.remote_wakeup << USB_DEVICE_REMOTE_WAKEUP);
        data[1] = 0x00;
        ret = 2;
      }
      break;
    case DeviceOutRequest | USB_REQ_CLEAR_FEATURE:
      if (value == USB_DEVICE_REMOTE_WAKEUP) {
        d.remote_wakeup = 0;
      } else {
        goto fail;
      }
      ret = 0;
      break;
    case DeviceOutRequest | USB_REQ_SET_FEATURE:
      if (value == USB_DEVICE_REMOTE_WAKEUP) {
        d.remote_wakeup = 1;
      } else {
        goto fail;
      }
      ret = 0;
      break;
    case DeviceOutRequest | USB_REQ_SET_ADDRESS:
      d.state = USB_STATE_ADDRESS;
      d.addr = value;
      ret = 0;
      break;
    case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
      switch(value >> 8) {
        case USB_DT_DEVICE:
          memcpy(data, bx_printer_dev_descriptor, sizeof(bx_printer_dev_descriptor));
          ret = sizeof(bx_printer_dev_descriptor);
          break;
        case USB_DT_CONFIG:
          memcpy(data, bx_printer_config_descriptor, sizeof(bx_printer_config_descriptor));
          ret = sizeof(bx_printer_config_descriptor);
          break;
        case USB_DT_STRING:
          switch(value & 0xff) {
            case 0:
              /* language ids */
              data[0] = 4;
              data[1] = 3;
              data[2] = 0x09;
              data[3] = 0x04;
              ret = 4;
              break;
            case 1:
              /* vendor description */
              ret = set_usb_string(data, "Hewlett-Packard");
              break;
            case 2:
              /* product description */
              ret = set_usb_string(data, "Deskjet 920C");
              break;
            case 3:
              /* serial number */
              ret = set_usb_string(data, "HU18L6P2DNBI");
              break;
            default:
              BX_ERROR(("USB Printer handle_control: unknown string descriptor 0x%02x", value & 0xff));
              goto fail;
          }
=======

  ret = handle_control_common(request, value, index, length, data);
  if (ret >= 0) {
    return ret;
  }

  ret = 0;
  switch(request) {
    case DeviceOutRequest | USB_REQ_CLEAR_FEATURE:
      goto fail;
      break;
    case DeviceOutRequest | USB_REQ_SET_FEATURE:
      goto fail;
      break;
    case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
      switch(value >> 8) {
        case USB_DT_STRING:
          BX_ERROR(("USB Printer handle_control: unknown string descriptor 0x%02x", value & 0xff));
          goto fail;
>>>>>>> version-2.6.9
          break;
        default:
          BX_ERROR(("USB Printer handle_control: unknown descriptor type 0x%02x", value >> 8));
          goto fail;
      }
      break;
<<<<<<< HEAD
    case DeviceRequest | USB_REQ_GET_CONFIGURATION:
      data[0] = 1;
      ret = 1;
      break;
    case DeviceOutRequest | USB_REQ_SET_CONFIGURATION:
      d.state = USB_STATE_CONFIGURED;
      ret = 0;
      break;
    case DeviceRequest | USB_REQ_GET_INTERFACE:
      data[0] = 0;
      ret = 1;
      break;
    case EndpointOutRequest | USB_REQ_SET_INTERFACE:
      ret = 0;
      break;
=======
>>>>>>> version-2.6.9

    /* printer specific requests */
    case InterfaceInClassRequest | 0x00:  // 1284 get device id string
      memcpy(data, bx_device_id_string, sizeof(bx_device_id_string));
      ret = sizeof(bx_device_id_string);
      data[0] = (Bit8u) (ret >> 8);   // len word is big endian
      data[1] = (Bit8u) (ret & 0xFF); // 
      break;
    case InterfaceInClassRequest | 0x01:  // Get Port Status
      s.printer_status = (0<<5) | (1<<4) | (1<<3);
      memcpy(data, &s.printer_status, 1);
      ret = 1;
      break;
    case InterfaceOutClassRequest | 0x02:  // soft reset
      // for now, just return
      ret = 0;
      break;
    default:
      BX_ERROR(("USB PRINTER handle_control: unknown request 0x%04x", request));
    fail:
      d.stall = 1;
      ret = USB_RET_STALL;
      break;
  }
  return ret;
}

int usb_printer_device_c::handle_data(USBPacket *p)
{
  int ret = 0;

  switch(p->pid) {
    case USB_TOKEN_IN:
      if (p->devep == 1) {
        BX_INFO(("Printer: handle_data: IN: len = %i", p->len));
        BX_INFO(("Printer: Ben: We need to find out what this is and send valid status back"));
        ret = p->len;
      } else {
        goto fail;
      }
      break;
    case USB_TOKEN_OUT:
      if (p->devep == 2) {
        BX_DEBUG(("Sent %i bytes to the 'usb printer': %s", p->len, s.fname));
        usb_dump_packet(p->data, p->len);
<<<<<<< HEAD
        fwrite(p->data, 1, p->len, s.fp);
=======
        if (s.fp != NULL) {
          fwrite(p->data, 1, p->len, s.fp);
        }
>>>>>>> version-2.6.9
        ret = p->len;
      } else {
        goto fail;
      }
      break;
    default:
fail:
      d.stall = 1;
      ret = USB_RET_STALL;
      break;
  }
  return ret;
}

<<<<<<< HEAD
=======
#undef LOG_THIS
#define LOG_THIS printer->

// USB printer runtime parameter handlers
const char *usb_printer_device_c::printfile_handler(bx_param_string_c *param, int set,
                                                    const char *oldval, const char *val,
                                                    int maxlen)
{
  usb_printer_device_c *printer;

  if (set) {
    if (strlen(val) < 1) {
      val = "none";
    }
    printer = (usb_printer_device_c*) param->get_parent()->get_device_param();
    if (printer != NULL) {
      if (printer->s.fp != NULL) {
        fclose(printer->s.fp);
      }
      printer->s.fp = fopen(val, "w+b");
      if (printer->s.fp == NULL) {
        BX_ERROR(("Could not create/open %s", val));
      }
    } else {
      BX_PANIC(("printfile_handler: printer not found"));
    }
  }
  return val;
}
>>>>>>> version-2.6.9
#endif // BX_SUPPORT_PCI && BX_SUPPORT_PCIUSB
